/***************************************************************************
 *   Copyright (C) 1998-2013 by authors (see AUTHORS.txt)                  *
 *                                                                         *
 *   This file is part of LuxRender.                                       *
 *                                                                         *
 *   Lux Renderer is free software; you can redistribute it and/or modify  *
 *   it under the terms of the GNU General Public License as published by  *
 *   the Free Software Foundation; either version 3 of the License, or     *
 *   (at your option) any later version.                                   *
 *                                                                         *
 *   Lux Renderer is distributed in the hope that it will be useful,       *
 *   but WITHOUT ANY WARRANTY; without even the implied warranty of        *
 *   MERCHANTABILITY or FITNESS FOR A PARTICULAR PURPOSE.  See the         *
 *   GNU General Public License for more details.                          *
 *                                                                         *
 *   You should have received a copy of the GNU General Public License     *
 *   along with this program.  If not, see <http://www.gnu.org/licenses/>. *
 *                                                                         *
 *   This project is based on PBRT ; see http://www.pbrt.org               *
 *   Lux Renderer website : http://www.luxrender.net                       *
 ***************************************************************************/

#include <cstdlib>
#include <stdexcept>
#include <string>
#include <iostream>
#include <fstream>
#include <stdio.h>

#include <boost/program_options.hpp>
#include <boost/filesystem.hpp>
#include <boost/regex.hpp>

#include "api.h"
#include "error.h"

using namespace std;
using namespace lux;

#if defined(WIN32)
#define POPEN _popen
#define PCLOSE _pclose
#else
#define POPEN popen
#define PCLOSE pclose
#endif

static std::string exec(const string &cmd) {
	FILE *pipe = POPEN(cmd.c_str(), "r");
	if (!pipe)
		return "ERROR";

	char buffer[128];
	std::string result = "";
	while (!feof(pipe)) {
		if (fgets(buffer, 128, pipe) != NULL)
			result += buffer;
	}
	PCLOSE(pipe);

	return result;
}

int main(int argc, char **argv) {
	try {
		luxInit();

		LOG(LUX_INFO, LUX_NOERROR) << "LuxVR " << luxVersion();

		//----------------------------------------------------------------------
		// Parse command line options
		//----------------------------------------------------------------------

		boost::program_options::options_description opts("Generic options");
		opts.add_options()
			("input-file", boost::program_options::value<std::string>(), "Specify input file")
			("directory,d", boost::program_options::value<std::string>(), "Current directory path")
			("verbose,V", "Increase output verbosity (show DEBUG messages)")
			("version,v", "Print version string")
			("help,h", "Display this help and exit");

		boost::program_options::variables_map commandLineOpts;
		try {
			boost::program_options::positional_options_description optPositional;
			optPositional.add("input-file", -1);

			// Disable guessing of option names
			const int cmdstyle = boost::program_options::command_line_style::default_style &
				~boost::program_options::command_line_style::allow_guessing;
			boost::program_options::store(boost::program_options::command_line_parser(argc, argv).
				style(cmdstyle).options(opts).positional(optPositional).run(), commandLineOpts);

			if (commandLineOpts.count("directory"))
				boost::filesystem::current_path(boost::filesystem::path(commandLineOpts["directory"].as<std::string>()));

			if (commandLineOpts.count("help")) {
				LOG(LUX_INFO, LUX_NOERROR) << "Usage: luxvr [options] file" << std::endl << opts;
				exit(EXIT_SUCCESS);
			}
		} catch(boost::program_options::error &e) {
			LOG(LUX_ERROR, LUX_SYSTEM) << "COMMAND LINE ERROR: " << e.what() << std::endl << opts; 
			exit(EXIT_FAILURE);
		}

		if (commandLineOpts.count("verbose"))
			luxErrorFilter(LUX_DEBUG);

		if (commandLineOpts.count("version")) {
			LOG(LUX_INFO, LUX_NOERROR) << "LuxVR version " << luxVersion() << " of " << __DATE__ << " at " << __TIME__;
			return EXIT_SUCCESS;
		}

		// Print all arguments
		LOG(LUX_DEBUG, LUX_NOERROR) << "Command arguments:";
		for (int i = 0; i < argc; ++i) {
			LOG(LUX_DEBUG, LUX_NOERROR) << "  " << i << ": [" << argv[i] << "]";
		}

		// Check the arguments
		if (commandLineOpts.count("input-file") != 1)
			throw runtime_error("LuxVR accept only one input file");
		const string lxsFile = commandLineOpts["input-file"].as<std::string>();
		LOG(LUX_DEBUG, LUX_NOERROR) << "LXS name: [" << lxsFile << "]";

		// Look for the directory where Lux executable are
		boost::filesystem::path exePath(boost::filesystem::initial_path<boost::filesystem::path>());
		exePath = boost::filesystem::system_complete(boost::filesystem::path(argv[0])).parent_path();
		LOG(LUX_DEBUG, LUX_NOERROR) << "Lux executable path: [" << exePath << "]";

		//----------------------------------------------------------------------
		// Looks for LuxConsole command
		//----------------------------------------------------------------------

		boost::filesystem::path luxconsolePath = exePath / "luxconsole";
		if (!boost::filesystem::exists(luxconsolePath))
			luxconsolePath = exePath / "luxconsole.exe";
		if (!boost::filesystem::exists(luxconsolePath))
			throw runtime_error("Unable to find luxconsole executable");
		LOG(LUX_DEBUG, LUX_NOERROR) << "LuxConsole path: [" << luxconsolePath << "]";
		const string luxconsole = luxconsolePath.make_preferred().string();
		LOG(LUX_DEBUG, LUX_NOERROR) << "LuxConsole native path: [" << luxconsole << "]";

		//----------------------------------------------------------------------
		// Looks for SLG command
		//----------------------------------------------------------------------

		boost::filesystem::path slgPath = exePath / "slg4";
		if (!boost::filesystem::exists(slgPath))
			slgPath = exePath / "slg4";
		if (!boost::filesystem::exists(slgPath))
			slgPath = exePath / "slg.exe";
		if (!boost::filesystem::exists(slgPath))
			slgPath = exePath / "slg";
		if (!boost::filesystem::exists(slgPath))
			throw runtime_error("Unable to find slg executable");
		LOG(LUX_DEBUG, LUX_NOERROR) << "SLG path: [" << slgPath << "]";
		const string slg = slgPath.make_preferred().string();
		LOG(LUX_DEBUG, LUX_NOERROR) << "SLG native path: [" << slg << "]";

		//----------------------------------------------------------------------
		// Looks for name of the .lxs file
		//----------------------------------------------------------------------

		boost::filesystem::path lxsPath(boost::filesystem::initial_path<boost::filesystem::path>());
		lxsPath = boost::filesystem::system_complete(boost::filesystem::path(lxsFile));
		LOG(LUX_DEBUG, LUX_NOERROR) << "LXS path: [" << lxsPath << "]";
		const string lxs = lxsPath.make_preferred().string();
		LOG(LUX_DEBUG, LUX_NOERROR) << "LXS native path: [" << lxs << "]";
		boost::filesystem::path lxsCopyPath = lxsPath.parent_path() / "luxvr.lxs";
		const string lxsCopy = lxsCopyPath.make_preferred().string();
		LOG(LUX_DEBUG, LUX_NOERROR) << "LXS copy native path: [" << lxs << "]";

		//----------------------------------------------------------------------
		// Create a copy of the .lxs file with SLG FILESAVER options
		//----------------------------------------------------------------------

		ifstream infile(lxs.c_str());
		if (!infile)
			throw runtime_error("Unable to open LXS file: " + lxs);

		string filetext((istreambuf_iterator<char>(infile)),
				istreambuf_iterator<char>());
		string changed(boost::regex_replace(filetext,
				boost::regex("Renderer \"sampler\""),
				"Renderer \"slg\" \"string config\" [\"renderengine.type = FILESAVER\" \"filesaver.directory = luxvr-scene\"]"));
		infile.close();
		ofstream outfile(lxsCopy.c_str(), ios_base::out | ios_base::trunc);
		if (!outfile.is_open())
			throw runtime_error("Unable to write temporary copy of LXS file: " + lxsCopy);
		outfile << changed;
		outfile.close();

		// TODO: transport SLG renderer options
		
		//----------------------------------------------------------------------
		// Create the directory where to export the SLG scene
		//----------------------------------------------------------------------

		boost::filesystem::path slgScenePath = lxsPath.parent_path() / "luxvr-scene";
		LOG(LUX_DEBUG, LUX_NOERROR) << "SLG scene path: [" << slgScenePath << "]";
		const string slgScene = slgScenePath.make_preferred().string();
		LOG(LUX_DEBUG, LUX_NOERROR) << "SLG scene native path: [" << slgScene << "]";

		if (!boost::filesystem::exists(slgScenePath))
			boost::filesystem::create_directory(slgScenePath);

		//----------------------------------------------------------------------
		// Execute LuxConsole in order to translate the scene
		//----------------------------------------------------------------------

		const string luxconsoleCmd = luxconsole + " -V " + lxsCopy + " 2>&1";
		LOG(LUX_DEBUG, LUX_NOERROR) << "LuxConsole command: " << luxconsoleCmd;
		const string luxconsoleOuput = exec(luxconsoleCmd);
		LOG(LUX_DEBUG, LUX_NOERROR) << "LuxConsole output:" << endl << luxconsoleOuput;

		//----------------------------------------------------------------------
		// Delete temporary LXS copy
		//----------------------------------------------------------------------

		boost::filesystem::remove(lxsCopyPath);

		//----------------------------------------------------------------------
		// Execute SLG GUI
		//----------------------------------------------------------------------

		const string slgCmd = slg +
			" -D renderengine.type RTPATHOCL"
			" -D sampler.type RANDOM"
			" -d " + slgScene +
			" render.cfg 2>&1";
		LOG(LUX_DEBUG, LUX_NOERROR) << "SLG command: " << slgCmd;
		const string slgOuput = exec(slgCmd);
		LOG(LUX_DEBUG, LUX_NOERROR) << "SLG output:" << endl << slgOuput;

	} catch (exception err) {
		LOG(LUX_ERROR, LUX_SYSTEM) << "ERROR: " << err.what();
		return EXIT_FAILURE;
	}

	return EXIT_SUCCESS;
}
