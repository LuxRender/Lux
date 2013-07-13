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
#include <boost/foreach.hpp>
#include <boost/algorithm/string/replace.hpp>

#include "lux.h"
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

static void exec(const string &cmd) {
	string execCmd = cmd;
#if defined(WIN32)
	// Wrap whole command line in quotes so it becomes one parameter
	execCmd = "\"" + execCmd + "\"";
#endif
	LOG(LUX_DEBUG, LUX_NOERROR) << "Exec: [" << execCmd << "]" ;

	FILE *pipe = POPEN(execCmd.c_str(), "r");
	if (!pipe)
		throw runtime_error("Unable to execute command: " + cmd);

	LOG(LUX_INFO, LUX_NOERROR) << "Command output:";

	string line = "";
	for (;;) {
		int c = fgetc(pipe);
		if (c == EOF) {
			LOG(LUX_INFO, LUX_NOERROR) << line;
			break;
		} else if (c == '\r') {
			// Nothing to do
		} else if (c == '\n') {
			LOG(LUX_INFO, LUX_NOERROR) << line;
			line = "";
		} else
			line += boost::lexical_cast<unsigned char>((unsigned char)c);
		
	}
	PCLOSE(pipe);
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
			("convert-only,c", "Convert the scene in SLG format and stop")
			("telnet,T", "Enable SLG telnet interface")
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
		infile.close();

		//----------------------------------------------------------------------
		// Forward all Renderer command configuration options
		//----------------------------------------------------------------------

		// Check if there is a Renderer command
		string rendererName;
		string rendererOptions;
		boost::regex rendererExpression("Renderer[\\s\\R]+\"([[:lower:]]+)\""
			"([\\s\\R]+\"string[\\s]+config\"[\\s\\R]+\\[(.+?)\\])?");
		{		
			std::string::const_iterator start = filetext.begin();
			std::string::const_iterator end = filetext.end();
			boost::match_results<std::string::const_iterator> what;
			boost::match_flag_type flags = boost::match_default;

			LOG(LUX_DEBUG, LUX_NOERROR) << "Looking for LXS Renderer command";
			while (boost::regex_search(start, end, what, rendererExpression, flags)) {
				LOG(LUX_DEBUG, LUX_NOERROR) << "LXS Renderer command match:";
				for (u_int i = 1; i < what.size(); ++i)
					LOG(LUX_DEBUG, LUX_NOERROR) << "  [" << what[i] << "]";

				rendererName = what[1];
				rendererOptions = what[3];

				// Update search position: 
				start = what[0].second; 
				// Update flags:
				flags |= boost::match_prev_avail;
				flags |= boost::match_not_bob;
			}
		}

		string changed;
		if (rendererName == "") {
			// There isn't a Renderer command, just add one
			changed = "Renderer \"slg\" \"string config\" [\"renderengine.type = FILESAVER\" \"filesaver.directory = luxvr-scene\"]\n" + filetext;
		} else {
			// Get the list of options 
			std::string::const_iterator start = rendererOptions.begin();
			std::string::const_iterator end = rendererOptions.end();
			boost::match_results<std::string::const_iterator> what;
			boost::regex expression("(\".+?\")");
			boost::match_flag_type flags = boost::match_default;

			LOG(LUX_DEBUG, LUX_NOERROR) << "Looking for Renderer options";
			vector<string> opts;
			while (boost::regex_search(start, end, what, expression, flags)) {
				LOG(LUX_DEBUG, LUX_NOERROR) << "Renderer options match:";
				for (u_int i = 1; i < what.size(); ++i)
					LOG(LUX_DEBUG, LUX_NOERROR) << "  [" << what[i] << "]";

				opts.push_back(what[1]);

				// Update search position: 
				start = what[0].second; 
				// Update flags:
				flags |= boost::match_prev_avail;
				flags |= boost::match_not_bob;
			}

			string luxvrRenderer = "Renderer \"slg\" \"string config\" [\"renderengine.type = FILESAVER\" \"filesaver.directory = luxvr-scene\" ";
			if (opts.size() > 0) {
				BOOST_FOREACH(string &opt, opts) {
					luxvrRenderer += opt + " ";
				}
			}
			luxvrRenderer += "]";
			LOG(LUX_DEBUG, LUX_NOERROR) << "New Renderer command: " << luxvrRenderer;
			changed = boost::regex_replace(filetext, rendererExpression, luxvrRenderer, boost::match_default | boost::format_all);
		}

		ofstream outfile(lxsCopy.c_str(), ios_base::out | ios_base::trunc);
		if (!outfile.is_open())
			throw runtime_error("Unable to write temporary copy of LXS file: " + lxsCopy);
		outfile << changed;
		outfile.close();
		
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

		const string luxconsoleCmd = "\"" + luxconsole + "\" " + (commandLineOpts.count("verbose") ? "-V " : "") + "\"" + lxsCopy + "\"" + " 2>&1";
		LOG(LUX_DEBUG, LUX_NOERROR) << "LuxConsole command: " << luxconsoleCmd;
		exec(luxconsoleCmd);

		//----------------------------------------------------------------------
		// Delete temporary LXS copy
		//----------------------------------------------------------------------

		boost::filesystem::remove(lxsCopyPath);

		//----------------------------------------------------------------------
		// Execute SLG GUI
		//----------------------------------------------------------------------

		// Check if I have to execute SLG GUI
		if (!commandLineOpts.count("convert-only")) {
			//------------------------------------------------------------------
			// Looks for SLG command
			//------------------------------------------------------------------

			boost::filesystem::path slgPath = exePath / "slg4";
			if (!boost::filesystem::exists(slgPath))
				slgPath = exePath / "slg4.exe";
			if (!boost::filesystem::exists(slgPath))
				slgPath = exePath / "slg";
			if (!boost::filesystem::exists(slgPath))
				slgPath = exePath / "slg.exe";
			// On Apple slg inside bundle has nasty sideeffects, so we look in default release location
			if (!boost::filesystem::exists(slgPath))
				slgPath = exePath / "../../../SmallluxGPU/slg4";
			if (!boost::filesystem::exists(slgPath))
				throw runtime_error("Unable to find slg executable");
			LOG(LUX_DEBUG, LUX_NOERROR) << "SLG path: [" << slgPath << "]";
			const string slg = slgPath.make_preferred().string();
			LOG(LUX_DEBUG, LUX_NOERROR) << "SLG native path: [" << slg << "]";

			const string slgCmd = "\"" + slg + "\""
				" -R" // Use LuxVR name
				" -D renderengine.type RTPATHOCL"
				" -D sampler.type RANDOM"
				" -D film.alphachannel.enable 0" // Alpha channel is useless for LuxVR
				" -d \"" + slgScene + "\""
				+ (commandLineOpts.count("telnet") ? " -T" : "") +
				" render.cfg 2>&1";
			LOG(LUX_DEBUG, LUX_NOERROR) << "SLG command: " << slgCmd;
			exec(slgCmd);
		}

		LOG(LUX_INFO, LUX_NOERROR) << "Done.";
	} catch (runtime_error err) {
		LOG(LUX_ERROR, LUX_SYSTEM) << "RUNTIME ERROR: " << err.what();
		return EXIT_FAILURE;
	} catch (exception err) {
		LOG(LUX_ERROR, LUX_SYSTEM) << "ERROR: " << err.what();
		return EXIT_FAILURE;
	}

	return EXIT_SUCCESS;
}
