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

#include <iomanip>
#include <fstream>
#include <string>
#include <sstream>
#include <exception>
#include <iostream>

#include "lux.h"
#include "error.h"
#include "samplefile.h"

#include <boost/program_options.hpp>

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

enum ReconstructionTypes {
	TYPE_BOX = 0
};

int main(int ac, char *av[]) {
	try {
		// Declare a group of options that will be
		// allowed only on command line
		po::options_description generic("Generic options");
		generic.add_options()
				("version,v", "Print version string")
				("help,h", "Produce help message")
				("output,o", po::value< std::string >()->default_value("out.exr"), "Output file")
				("verbose,V", "Increase output verbosity (show DEBUG messages)")
				("quiet,q", "Reduce output verbosity (hide INFO messages)") // (give once for WARNING only, twice for ERROR only)")
				("type,t", po::value< int >(), "Select the type of reconstruction (0 = BOX)")
				;

		// Hidden options, will be allowed both on command line and
		// in config file, but will not be shown to the user.
		po::options_description hidden("Hidden options");
		hidden.add_options()
				("input-file", po::value< vector<string> >(), "input file")
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
			LOG(LUX_ERROR, LUX_SYSTEM) << "Usage: luxrecons [options] <samples file1> [<samples fileN>] <exr file>\n" << visible;
			return 0;
		}

		LOG(LUX_INFO, LUX_NOERROR) << "Lux version " << luxVersion() << " of " << __DATE__ << " at " << __TIME__;
		
		if (vm.count("version"))
			return 0;

		if (vm.count("verbose")) {
			luxErrorFilter(LUX_DEBUG);
		}

		if (vm.count("quiet")) {
			luxErrorFilter(LUX_WARNING);
		}

		ReconstructionTypes reconType = TYPE_BOX;
		if (vm.count("type"))
			reconType = (ReconstructionTypes)vm["type"].as<int>();
		
		LOG(LUX_INFO, LUX_NOERROR) << "Reconstruction type: " << reconType;
		
		if (vm.count("input-file")) {
			const std::vector<std::string> &v = vm["input-file"].as < vector<string> > ();

			for (size_t i = 0; i < v.size(); i++) {
				SampleFileReader sfr(v[i]);
			}
		} else {
			LOG(LUX_ERROR, LUX_SYSTEM) << "luxrecons: missing input/output files";
			return 1;
		}
	} catch (std::exception & e) {
		LOG(LUX_SEVERE, LUX_SYNTAX) << "Command line argument parsing failed with error '" << e.what() << "', please use the --help option to view the allowed syntax.";
		return 1;
	}

	return 0;
}
