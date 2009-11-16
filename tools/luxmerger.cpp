/***************************************************************************
 *   Copyright (C) 1998-2009 by authors (see AUTHORS.txt )                 *
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

#define NDEBUG 1

#include <iomanip>
#include <fstream>
#include <string>
#include <sstream>
#include <exception>
#include <iostream>

#include <boost/program_options.hpp>
#include <boost/thread.hpp>
#include <boost/thread/xtime.hpp>
#include <boost/date_time/posix_time/posix_time.hpp>
#include <boost/filesystem/path.hpp>
#include <boost/filesystem/operations.hpp>
#include <boost/scoped_ptr.hpp>

#include "lux.h"
#include "api.h"
#include "context.h"
#include "film/fleximage.h"
#include "error.h"
#include "osfunc.h"

#if defined(WIN32) && !defined(__CYGWIN__) /* We need the following two to set stdout to binary */
#include <io.h>
#include <fcntl.h>
#endif

#if defined(WIN32) && !defined(__CYGWIN__)
#include "direct.h"
#define chdir _chdir
#endif

using namespace lux;
namespace po = boost::program_options;

int main(int ac, char *av[]) {

	try {
		std::stringstream ss;

		// Declare a group of options that will be
		// allowed only on command line
		po::options_description generic("Generic options");
		generic.add_options()
				("version,v", "Print version string")
				("help,h", "Produce help message")
				("debug,d", "Enable debug mode")
				("output,o", po::value< std::string >()->default_value("merged.flm"), "Output file")
				("verbosity,V", po::value< int >(), "Log output verbosity")
				;

		// Hidden options, will be allowed both on command line and
		// in config file, but will not be shown to the user.
		po::options_description hidden("Hidden options");
		hidden.add_options()
				("input-file", po::value< vector<string> >(), "input file")
				("test", "debug test mode")
				;

		po::options_description cmdline_options;
		cmdline_options.add(generic).add(hidden);

		po::options_description visible("Allowed options");
		visible.add(generic);

		po::positional_options_description p;

		p.add("input-file", -1);

		po::variables_map vm;
		store(po::command_line_parser(ac, av).
				options(cmdline_options).positional(p).run(), vm);

		if (vm.count("help")) {
			ss.str("");
			ss << "Usage: luxmerger [options] file...\n" << visible;
			luxError(LUX_SYSTEM, LUX_ERROR, ss.str().c_str());
			return 0;
		}

		if (vm.count("verbosity"))
			luxLogFilter = vm["verbosity"].as<int>();

		ss.str("");
		ss << "Lux version " << LUX_VERSION << " of " << __DATE__ << " at " << __TIME__;
		luxError(LUX_NOERROR, LUX_INFO, ss.str().c_str());
		if (vm.count("version"))
			return 0;

		if (vm.count("debug")) {
			luxError(LUX_NOERROR, LUX_INFO, "Debug mode enabled");
		}

		string outputFileName = vm["output"].as<string>();

		boost::scoped_ptr<FlexImageFilm> film;
		int mergedCount = 0;

		if (vm.count("input-file")) {
			const std::vector<std::string> &v = vm["input-file"].as < vector<string> > ();
			for (unsigned int i = 0; i < v.size(); i++) {
				boost::filesystem::path fullPath(boost::filesystem::initial_path());
				fullPath = boost::filesystem::system_complete(boost::filesystem::path(v[i], boost::filesystem::native));

				if (!boost::filesystem::exists(fullPath) && v[i] != "-") {
					ss.str("");
					ss << "Unable to open file '" << fullPath.string() << "'";
					luxError(LUX_NOFILE, LUX_SEVERE, ss.str().c_str());
					continue;
				}

				std::string flmFileName = fullPath.string();

				if (!film) {
					// initial flm file
					film.reset((FlexImageFilm*)FlexImageFilm::CreateFilmFromFLM(flmFileName));
					if (!film) {
						ss.str("");
						ss << "Error reading FLM file '" << flmFileName << "'";
						luxError(LUX_NOFILE, LUX_SEVERE, ss.str().c_str());
						continue;
					}
				} else {
					// additional flm file
					std::ifstream ifs(flmFileName.c_str(), std::ios_base::in | std::ios_base::binary);

					if(ifs.good()) {
						// read the data
						luxError(LUX_NOERROR, LUX_INFO, (std::string("Merging FLM file ") + flmFileName).c_str());
						float newSamples = film->UpdateFilm(ifs);
						if (newSamples <= 0) {
							ss.str("");
							ss << "Error reading FLM file '" << flmFileName << "'";
							luxError(LUX_NOFILE, LUX_SEVERE, ss.str().c_str());
							ifs.close();
							continue;
						} else {
							ss.str("");
							ss << "Merged " << newSamples << " samples from FLM file";
							luxError(LUX_NOERROR, LUX_DEBUG, ss.str().c_str());
						}
					}

					ifs.close();
				}

				mergedCount++;
			}

			if (!film) {
				ss.str("");
				ss << "No files merged";
				luxError(LUX_NOERROR, LUX_WARNING, ss.str().c_str());
				return 2;
			}

			ss.str("");
			ss << "Merged " << mergedCount << " FLM files, writing merged FLM to " << outputFileName;
			luxError(LUX_NOERROR, LUX_INFO, ss.str().c_str());

			film->WriteFilm(outputFileName);
		} else {
			ss.str("");
			ss << "luxmerger: no input file";
			luxError(LUX_SYSTEM, LUX_ERROR, ss.str().c_str());
		}

	} catch (std::exception & e) {
		std::stringstream ss;
		ss << "Command line argument parsing failed with error '" << e.what() << "', please use the --help option to view the allowed syntax.";
		luxError(LUX_SYNTAX, LUX_SEVERE, ss.str().c_str());
		return 1;
	}
	return 0;
}
