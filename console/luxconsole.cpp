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

#include "api.h"
#include "error.h"
#include "server/renderserver.h"

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

std::string sceneFileName;
int threads;
bool parseError;

void engineThread() {
	// NOTE - lordcrc - initialize rand()
	srand(time(NULL));

    luxParse(sceneFileName.c_str());
    if (luxStatistics("sceneIsReady") == 0.)
        parseError = true;
}

void infoThread() {
	while (!boost::this_thread::interruption_requested()) {
		try {
			boost::xtime xt;
			boost::xtime_get(&xt, boost::TIME_UTC);
			xt.sec += 5;
			boost::thread::sleep(xt);

			LOG(LUX_INFO,LUX_NOERROR) << luxPrintableStatistics(false);
		} catch(boost::thread_interrupted ex) {
			break;
		}
	}
}

int main(int ac, char *av[]) {
	// Dade - initialize rand() number generator
	srand(time(NULL));

	luxInit();

	try {
		// Declare a group of options that will be
		// allowed only on command line
		po::options_description generic("Generic options");
		generic.add_options()
				("version,v", "Print version string")
				("help,h", "Produce help message")
				("resume,r", po::value< std::string >()->implicit_value(""), "Resume from FLM")
				("overrideresume,R", po::value< std::string >(), "Resume from specified FLM")
				("server,s", "Launch in server mode")
				("bindump,b", "Dump binary RGB framebuffer to stdout when finished")
				("debug,d", "Enable debug mode")
				("fixedseed,f", "Disable random seed mode")
				("minepsilon,e", po::value< float >(), "Set minimum epsilon")
				("maxepsilon,E", po::value< float >(), "Set maximum epsilon")
				("verbose,V", "Increase output verbosity (show DEBUG messages)")
				("quiet,q", "Reduce output verbosity (hide INFO messages)") // (give once for WARNING only, twice for ERROR only)")
				;

		// Declare a group of options that will be
		// allowed both on command line and in
		// config file
		po::options_description config("Configuration");
		config.add_options()
				("threads,t", po::value < int >(), "Specify the number of threads that Lux will run in parallel.")
				("useserver,u", po::value< std::vector<std::string> >()->composing(), "Specify the adress of a rendering server to use.")
				("serverinterval,i", po::value < int >(), "Specify the number of seconds between requests to rendering servers.")
				("serverport,p", po::value < int >(), "Specify the tcp port used in server mode.")
				("serverwriteflm,W", "Write film to disk before transmitting in server mode.")
				;

		// Hidden options, will be allowed both on command line and
		// in config file, but will not be shown to the user.
		po::options_description hidden("Hidden options");
		hidden.add_options()
				("input-file", po::value< vector<string> >(), "input file")
				("test", "debug test mode")
				;

		po::options_description cmdline_options;
		cmdline_options.add(generic).add(config).add(hidden);

		po::options_description config_file_options;
		config_file_options.add(config).add(hidden);

		po::options_description visible("Allowed options");
		visible.add(generic).add(config);

		po::positional_options_description p;

		p.add("input-file", -1);

		po::variables_map vm;
		// disable guessing of option names
		int cmdstyle = po::command_line_style::default_style & ~po::command_line_style::allow_guessing;
		store(po::command_line_parser(ac, av).
			style(cmdstyle).options(cmdline_options).positional(p).run(), vm);

		std::ifstream
		ifs("luxconsole.cfg");
		store(parse_config_file(ifs, config_file_options), vm);
		notify(vm);

		if (vm.count("help")) {
			LOG(LUX_ERROR,LUX_SYSTEM)<< "Usage: luxconsole [options] file...\n" << visible;
			return 0;
		}

		LOG(LUX_INFO,LUX_NOERROR) << "Lux version " << luxVersion() << " of " << __DATE__ << " at " << __TIME__;
		
		if (vm.count("version"))
			return 0;

		if (vm.count("threads"))
			threads = vm["threads"].as<int>();
		else {
			// Dade - check for the hardware concurrency available
			threads = boost::thread::hardware_concurrency();
			if (threads == 0)
				threads = 1;
		}

		LOG(LUX_INFO,LUX_NOERROR) << "Threads: " << threads;

		if (vm.count("debug")) {
			LOG(LUX_INFO,LUX_NOERROR)<< "Debug mode enabled";
			luxEnableDebugMode();
		}

		if (vm.count("verbose")) {
			luxErrorFilter(LUX_DEBUG);
		}

		if (vm.count("quiet")) {
			luxErrorFilter(LUX_WARNING);
		}

		/*
		// cannot get po to parse multiple occurrences
		if (vm.count("quiet")==2) {
			luxErrorFilter(LUX_ERROR);
		}
		*/

		if (vm.count("fixedseed")) {
			if (!vm.count("server"))
				luxDisableRandomMode();
			else // Slaves should always have a different seed than the master
				LOG(LUX_WARNING,LUX_CONSISTENCY)<< "Using random seed for server";
		}

		int serverInterval;
		if (vm.count("serverinterval")) {
			serverInterval = vm["serverinterval"].as<int>();
			luxSetNetworkServerUpdateInterval(serverInterval);
		} else
			serverInterval = luxGetNetworkServerUpdateInterval();

		if (vm.count("useserver")) {
			std::vector<std::string> names = vm["useserver"].as<std::vector<std::string> >();

			for (std::vector<std::string>::iterator i = names.begin(); i < names.end(); i++)
				luxAddServer((*i).c_str());

			LOG(LUX_INFO,LUX_NOERROR) << "Server requests interval: " << serverInterval << " secs";
		}

		int serverPort = RenderServer::DEFAULT_TCP_PORT;
		if (vm.count("serverport"))
			serverPort = vm["serverport"].as<int>();

		// Any call to Lux API must be done _after_ luxAddServer
		if (vm.count("minepsilon")) {
			const float mine = vm["minepsilon"].as<float>();
			if (vm.count("maxepsilon")) {
				const float maxe = vm["maxepsilon"].as<float>();
				luxSetEpsilon(mine, maxe);
			} else
				luxSetEpsilon(mine, -1.f);
		} else {
			if (vm.count("maxepsilon")) {
				const float maxe = vm["maxepsilon"].as<float>();
				luxSetEpsilon(-1.f, maxe);
			} else
				luxSetEpsilon(-1.f, -1.f);
		}

		if (vm.count("resume")) {
			luxOverrideResumeFLM("");
		}

		if (vm.count("overrideresume")) {
			std::string resumefile = vm["overrideresume"].as<std::string>();

			boost::filesystem::path resumePath(resumefile);
			if (!boost::filesystem::exists(resumePath)) {
				LOG(LUX_WARNING,LUX_NOFILE) << "Could not find resume file '" << resumefile << "', using filename in scene";
				resumefile = "";
			}

			luxOverrideResumeFLM(resumefile.c_str());
		}

		if (vm.count("input-file")) {
			const std::vector<std::string> &v = vm["input-file"].as < vector<string> > ();
			for (unsigned int i = 0; i < v.size(); i++) {
				//change the working directory
				boost::filesystem::path fullPath(boost::filesystem::initial_path());
				fullPath = boost::filesystem::system_complete(boost::filesystem::path(v[i], boost::filesystem::native));

				if (!boost::filesystem::exists(fullPath) && v[i] != "-") {
					LOG(LUX_SEVERE,LUX_NOFILE) << "Unable to open scenefile '" << fullPath.string() << "'";
					continue;
				}

				sceneFileName = fullPath.leaf();
				if (chdir(fullPath.branch_path().string().c_str())) {
					LOG(LUX_SEVERE,LUX_NOFILE) << "Unable to go into directory '" << fullPath.branch_path().string() << "'";
				}

				parseError = false;
				boost::thread engine(&engineThread);

				//wait the scene parsing to finish
				while (!luxStatistics("sceneIsReady") && !parseError) {
					boost::xtime xt;
					boost::xtime_get(&xt, boost::TIME_UTC);
					xt.sec += 1;
					boost::thread::sleep(xt);
				}

				if (parseError) {
					LOG(LUX_SEVERE,LUX_BADFILE) << "Skipping invalid scenefile '" << fullPath.string() << "'";
					continue;
				}

				//add rendering threads
				int threadsToAdd = threads;
				while (--threadsToAdd)
					luxAddThread();

				//launch info printing thread
				boost::thread info(&infoThread);

				// Dade - wait for the end of the rendering
				luxWait();

				// We have to stop the info thread before to call luxExit()/luxCleanup()
				info.interrupt();
				info.join();

				luxExit();

				// Dade - print the total rendering time
				boost::posix_time::time_duration td(0, 0,
						(int) luxStatistics("secElapsed"), 0);

				LOG(LUX_INFO,LUX_NOERROR) << "100% rendering done [" << threads << " threads] " << td;

				if (vm.count("bindump")) {
					// Get pointer to framebuffer data if needed
					unsigned char* fb = luxFramebuffer();

					int w = luxGetIntAttribute("film", "xResolution");
					int h = luxGetIntAttribute("film", "yResolution");
					luxUpdateFramebuffer();

#if defined(WIN32) && !defined(__CYGWIN__) /* On WIN32 we need to set stdout to binary */
					_setmode(_fileno(stdout), _O_BINARY);
#endif

					// Dump RGB imagebuffer data to stdout
					for (int i = 0; i < w * h * 3; i++)
						std::cout << fb[i];
				}

				luxCleanup();
			}
		} else if (vm.count("server")) {
			bool writeFlmFile = vm.count("serverwriteflm") != 0;
			RenderServer *renderServer = new RenderServer(threads, serverPort, writeFlmFile);
			renderServer->start();
			renderServer->join();
			delete renderServer;
		} else {
			LOG(LUX_ERROR,LUX_SYSTEM) << "luxconsole: no input file";
		}

	} catch (std::exception & e) {
		LOG(LUX_SEVERE,LUX_SYNTAX) << "Command line argument parsing failed with error '" << e.what() << "', please use the --help option to view the allowed syntax.";
		return 1;
	}
	return 0;
}
