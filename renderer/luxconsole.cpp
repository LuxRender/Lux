/***************************************************************************
 *   Copyright (C) 1998-2007 by authors (see AUTHORS.txt )                 *
 *                                                                         *
 *   This file is part of Lux Renderer.                                    *
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
 *   Lux Renderer website : http://www.luxrender.org                       *
 ***************************************************************************/

//// luxgui.cpp*
#include "lux.h"
#include "api.h"
#include "error.h"

#include <iostream>
#include <fstream>
#include <string>
#include <exception>
#include <boost/program_options.hpp>
#include <boost/thread.hpp>
#include <boost/thread/xtime.hpp>
#include <boost/date_time/posix_time/posix_time.hpp>
#include <boost/filesystem/path.hpp>
#include <boost/filesystem/operations.hpp>

#ifdef WIN32
#include "direct.h"
#define chdir _chdir
#endif

namespace po = boost::program_options;

std::string sceneFileName;
int threads;

void engineThread() {

	luxInit();
	ParseFile(sceneFileName.c_str() );
	luxCleanup();
}

void infoThread() {
	char progress[5] ="/-\\|";
	unsigned int pc=0;
	while (true) {

		boost::xtime xt;
		boost::xtime_get(&xt, boost::TIME_UTC);
		xt.sec += 1;
		boost::thread::sleep(xt);

		boost::posix_time::time_duration td(0, 0,
				(int)luxStatistics("secElapsed"), 0);

		std::cout<<'\r' <<progress[(pc++)%4] <<'['<<threads<<']' <<td <<" "
				<<(int)luxStatistics("samplesSec")<<" samples/sec " <<" "
				<<(float)luxStatistics("samplesPx")<<" samples/pix" <<"    "
				<<std::flush;
	}
}

int main(int ac, char *av[]) {
	/*
	 // Print welcome banner
	 printf("Lux Renderer version %1.3f of %s at %s\n", LUX_VERSION, __DATE__, __TIME__);     
	 printf("This program comes with ABSOLUTELY NO WARRANTY.\n");
	 printf("This is free software, covered by the GNU General Public License V3\n");
	 printf("You are welcome to redistribute it under certain conditions,\nsee COPYING.TXT for details.\n");    
	 fflush(stdout);
	 */

	try
	{

		// Declare a group of options that will be
		// allowed only on command line
		po::options_description generic ("Generic options");
		generic.add_options ()
		("version,v", "print version string") ("help", "produce help message");

		// Declare a group of options that will be
		// allowed both on command line and in
		// config file
		po::options_description config ("Configuration");
		config.add_options ()
		("threads,t", po::value < int >(),
				"Specify the number of threads that Lux will run in parallel.");

		// Hidden options, will be allowed both on command line and
		// in config file, but will not be shown to the user.
		po::options_description hidden ("Hidden options");
		hidden.add_options ()
		("input-file", po::value< vector<string> >(), "input file");

		po::options_description cmdline_options;
		cmdline_options.add (generic).add (config).add (hidden);

		po::options_description config_file_options;
		config_file_options.add (config).add (hidden);

		po::options_description visible ("Allowed options");
		visible.add (generic).add (config);

		po::positional_options_description p;

		p.add ("input-file", -1);

		po::variables_map vm;
		store (po::command_line_parser (ac, av).
				options (cmdline_options).positional (p).run (), vm);

		std::ifstream
		ifs ("luxconsole.cfg");
		store (parse_config_file (ifs, config_file_options), vm);
		notify (vm);

		if (vm.count ("help"))
		{
			std::cout << "Usage: luxconsole [options] file...\n";
			std::cout << visible << "\n";
			return 0;
		}

		if (vm.count ("version"))
		{
			std::cout << "Lux version "<<LUX_VERSION<<" of "<<__DATE__<<" at "<<__TIME__<<std::endl;
			return 0;
		}

		if (vm.count("threads"))
		{
			threads=vm["threads"].as<int>();
		}
		else
		{
			threads=1;;
		}

		if (vm.count ("input-file"))
		{
			const std::vector<std::string> &v = vm["input-file"].as < vector<string> > ();
			for (unsigned int i = 0; i < v.size(); i++)
			{
				//sceneFileName=v[i];

				//change the working directory
				boost::filesystem::path fullPath( boost::filesystem::initial_path() );
				fullPath = boost::filesystem::system_complete( boost::filesystem::path( v[i], boost::filesystem::native ) );
				sceneFileName=fullPath.leaf();
				chdir (fullPath.branch_path().string().c_str());

				boost::thread t(&engineThread);

				//wait the scene parsing to finish
				while(!luxStatistics("sceneIsReady"))
				{
					boost::xtime xt;
					boost::xtime_get(&xt, boost::TIME_UTC);
					xt.sec += 1;
					boost::thread::sleep(xt);
				}

				//add rendering threads
				int threadsToAdd=threads;
				while(--threadsToAdd)
				{
					luxAddThread();
				}

				//launch info printing thread
				//boost::thread *j=new 
				boost::thread j(&infoThread);

				//wait for threads to finish
				t.join();
			}

		}
		else
		{
			std::cout<< "luxconsole: no input file"<<std::endl;
		}

	}
	catch (std::exception & e)
	{
		std::cout << e.what () << std::endl;
		return 1;
	}
	return 0;
}
