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

#define NDEBUG 1

#include <iomanip>
#include <fstream>
#include <string>
#include <sstream>
#include <exception>
#include <iostream>

#include "api.h"
#include "film/fleximage.h"

#include <boost/program_options.hpp>
#include <boost/thread.hpp>
#include <boost/date_time/posix_time/posix_time.hpp>
#include <boost/filesystem.hpp>
#include <boost/scoped_ptr.hpp>

#if defined(WIN32) && !defined(__CYGWIN__) /* We need the following two to set stdout to binary */
#include <io.h>
#include <fcntl.h>
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
				("save-png,s", "Output PNG tone-mapped image")
				("verbose,V", "Increase output verbosity (show DEBUG messages)")
				("quiet,q", "Reduce output verbosity (hide INFO messages)") // (give once for WARNING only, twice for ERROR only)")
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
			LOG( LUX_ERROR,LUX_SYSTEM) << "Usage: luxmerger [options] file...\n" << visible;
			return 0;
		}

		LOG(LUX_INFO,LUX_NOERROR) << "Lux version " << luxVersion() << " of " << __DATE__ << " at " << __TIME__;
		if (vm.count("version"))
			return 0;

		if (vm.count("debug")) {
			LOG( LUX_INFO,LUX_NOERROR)<< "Debug mode enabled";
		}

		if (vm.count("verbose")) {
			luxErrorFilter(LUX_DEBUG);
		}

		if (vm.count("quiet")) {
			luxErrorFilter(LUX_WARNING);
		}

		string outputFileName = vm["output"].as<string>();

		boost::scoped_ptr<FlexImageFilm> film;
		int mergedCount = 0;

		luxInit();

		if (vm.count("input-file")) {
			const std::vector<std::string> &v = vm["input-file"].as < vector<string> > ();
			for (unsigned int i = 0; i < v.size(); i++) {
				boost::filesystem::path fullPath(boost::filesystem::system_complete(v[i]));

				if (!boost::filesystem::exists(fullPath) && v[i] != "-") {
					LOG(LUX_SEVERE,LUX_NOFILE) << "Unable to open file '" << fullPath.string() << "'";
					continue;
				}

				std::string flmFileName = fullPath.string();

				if (!film) {
					// initial flm file
					film.reset((FlexImageFilm*)FlexImageFilm::CreateFilmFromFLM(flmFileName));
					if (!film) {
						LOG( LUX_SEVERE,LUX_NOFILE) << "Error reading FLM file '" << flmFileName << "'";
						continue;
					}
				} else {
					// additional flm file
					std::ifstream ifs(flmFileName.c_str(), std::ios_base::in | std::ios_base::binary);

					if(ifs.good()) {
						// read the data
						LOG( LUX_INFO,LUX_NOERROR)<< "Merging FLM file " << flmFileName;
						float newSamples = film->MergeFilmFromStream(ifs);
						if (newSamples <= 0) {
							LOG( LUX_SEVERE,LUX_NOFILE) << "Error reading FLM file '" << flmFileName << "'";
							ifs.close();
							continue;
						} else {
							LOG( LUX_DEBUG,LUX_NOERROR) << "Merged " << newSamples << " samples from FLM file";
						}
					}

					ifs.close();
				}

				mergedCount++;
			}

			luxCleanup();

			if (!film) {
				LOG( LUX_WARNING,LUX_NOERROR) << "No files merged";
				return 2;
			}

			LOG( LUX_INFO,LUX_NOERROR) << "Merged " << mergedCount << " FLM files, writing merged FLM to " << outputFileName;

			film->WriteFilmToFile(outputFileName);

			if (vm.count("save-png")) {
				(*film)["write_PNG"] = true;
				film->WriteImage(IMAGE_FILEOUTPUT);
			}
		} else {
			LOG( LUX_ERROR,LUX_SYSTEM) << "luxmerger: no input file";
		}

	} catch (std::exception & e) {
		LOG( LUX_SEVERE,LUX_SYNTAX)
			<< "Command line argument parsing failed with error '" << e.what()
			<< "', please use the --help option to view the allowed syntax.";
		return 1;
	}
	return 0;
}
