/***************************************************************************
 *   Copyright (C) 1998-2008 by authors (see AUTHORS.txt )                 *
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

#include "lux.h"
#include "api.h"
#include "context.h"
#include "error.h"
#include "renderserver.h"
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

std::string sceneFileName;
int threads;
bool parseError;

void engineThread() {
    ParseFile(sceneFileName.c_str());
    if (luxStatistics("sceneIsReady") == 0.)
        parseError = true;
}

void infoThread() {
    while (true) {
        boost::xtime xt;
        boost::xtime_get(&xt, boost::TIME_UTC);
        xt.sec += 5;
        boost::thread::sleep(xt);

        boost::posix_time::time_duration td(0, 0,
                (int) luxStatistics("secElapsed"), 0);

        int sampleSec = (int)luxStatistics("samplesSec");
        // Dade - print only if we are rendering something
        if (sampleSec > 0) {
            std::stringstream ss;
            ss << '[' << threads << " threads] " << td << " "
                    << sampleSec << " samples/sec " << " "
                    << (int) luxStatistics("samplesTotSec") << " samples/totsec " << " "
                    << (float) luxStatistics("samplesPx") << " samples/pix";
            luxError(LUX_NOERROR, LUX_INFO, ss.str().c_str());
        }
    }
}

int main(int ac, char *av[]) {
	// Dade - initialize rand() number generator
	srand(time(NULL));

    bool useServer = false;
    luxInit();

    try {
        std::stringstream ss;

        // Declare a group of options that will be
        // allowed only on command line
        po::options_description generic("Generic options");
        generic.add_options()
                ("version,v", "Print version string") ("help", "Produce help message")
                ("server,s", "Launch in server mode")
				("bindump,b", "Dump binary RGB framebuffer to stdout when finished")
                ("debug,d", "Enable debug mode")
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
        store(po::command_line_parser(ac, av).
                options(cmdline_options).positional(p).run(), vm);

        std::ifstream
        ifs("luxconsole.cfg");
        store(parse_config_file(ifs, config_file_options), vm);
        notify(vm);

        if (vm.count("help")) {
            ss.str("");
            ss << "Usage: luxconsole [options] file...\n" << visible;
            luxError(LUX_SYSTEM, LUX_ERROR, ss.str().c_str());
            return 0;
        }

        ss.str("");
        ss << "Lux version " << LUX_VERSION << " of " << __DATE__ << " at " << __TIME__;
        luxError(LUX_NOERROR, LUX_INFO, ss.str().c_str());
        if (vm.count("version"))
            return 0;

        if (vm.count("threads"))
            threads = vm["threads"].as<int>();
		else {
			// Dade - check for the hardware concurrency available
			threads = osHardwareConcurrency();
			if (threads == 0)
				threads = 1;
		}

        ss.str("");
        ss << "Threads: " << threads;
        luxError(LUX_NOERROR, LUX_INFO, ss.str().c_str());

        if (vm.count("debug")) {
            luxError(LUX_NOERROR, LUX_INFO, "Debug mode enabled");
            luxEnableDebugMode();
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

            useServer = true;
            
            ss.str("");
            ss << "Server requests interval: " << serverInterval << " secs";
            luxError(LUX_NOERROR, LUX_INFO, ss.str().c_str());
        }

		int serverPort = RenderServer::DEFAULT_TCP_PORT;
		if (vm.count("serverport"))
            serverPort = vm["serverport"].as<int>();
			
        if (vm.count("input-file")) {
            const std::vector<std::string> &v = vm["input-file"].as < vector<string> > ();
            for (unsigned int i = 0; i < v.size(); i++) {
                //change the working directory
                boost::filesystem::path fullPath(boost::filesystem::initial_path());
                fullPath = boost::filesystem::system_complete(boost::filesystem::path(v[i], boost::filesystem::native));

                if (!boost::filesystem::exists(fullPath) && v[i] != "-") {
                    ss.str("");
                    ss << "Unable to open scenefile '" << fullPath.string() << "'";
                    luxError(LUX_NOFILE, LUX_SEVERE, ss.str().c_str());
                    continue;
                }

                sceneFileName = fullPath.leaf();
                if (!chdir(fullPath.branch_path().string().c_str())) {
			ss.str("");
			ss << "Unable to go into directory '" << fullPath.branch_path().string() << "'";
			luxError(LUX_NOFILE, LUX_SEVERE, ss.str().c_str());
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
                    std::stringstream ss;
                    ss << "Skipping invalid scenefile '" << fullPath.string() << "'";
                    luxError(LUX_BADFILE, LUX_SEVERE, ss.str().c_str());
                    continue;
                }

                //add rendering threads
                int threadsToAdd = threads;
                while (--threadsToAdd)
                    Context::luxAddThread();

                //launch info printing thread
                boost::thread info(&infoThread);

                // Dade - wait for the end of the rendering
                luxWait();
                luxExit();

                // Dade - print the total rendering time
                boost::posix_time::time_duration td(0, 0,
                        (int) luxStatistics("secElapsed"), 0);

                ss.str("");
                ss << "100% rendering done [" << threads << " threads] " << td;
                luxError(LUX_NOERROR, LUX_INFO, ss.str().c_str());

				if (vm.count("bindump")) {
					// Get pointer to framebuffer data if needed
					unsigned char* fb = luxFramebuffer();

					int w = luxStatistics("filmXres"), h = luxStatistics("filmYres");
					luxUpdateFramebuffer();

#if defined(WIN32) && !defined(__CYGWIN__) /* On WIN32 we need to set stdout to binary */
					_setmode( _fileno( stdout ), _O_BINARY );
#endif

					// Dump RGB imagebuffer data to stdout
					for(int i=0; i<w*h*3; i++)
						std::cout << fb[i];
				}

                luxCleanup();
            }
        } else if (vm.count("server")) {
            RenderServer *renderServer = new RenderServer(threads, serverPort);
            renderServer->start();
            renderServer->join();
            delete renderServer;
        } else {
            ss.str("");
            ss << "luxconsole: no input file";
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
