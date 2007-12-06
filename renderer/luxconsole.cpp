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
#define NDEBUG 1

#include "lux.h"
#include "api.h"
#include "error.h"

#include <iostream>
#include <fstream>
#include <string>
#include <exception>
#include <ctime>
#include <boost/bind.hpp>
#include <boost/program_options.hpp>
#include <boost/thread.hpp>
#include <boost/thread/xtime.hpp>
#include <boost/date_time/posix_time/posix_time.hpp>
#include <boost/filesystem/path.hpp>
#include <boost/filesystem/operations.hpp>

#include "include/asio.hpp"
#include <boost/serialization/vector.hpp>
#include <boost/serialization/string.hpp>
#include "../core/paramset.h"
#include <fstream>
#include <boost/archive/text_oarchive.hpp>
#include <boost/archive/text_iarchive.hpp>
//#include <boost/archive/xml_oarchive.hpp>
//#include <boost/archive/xml_iarchive.hpp>
#include <boost/serialization/split_member.hpp>

#include <iomanip>

#ifdef WIN32
#include "direct.h"
#define chdir _chdir
#endif

using asio::ip::tcp;
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


void startServer() {
	std::cout<<"luxconsole: launching server mode..."<<std::endl;
	try
	{
		asio::io_service io_service;

		tcp::endpoint endpoint(tcp::v4(), 18018);
		tcp::acceptor acceptor(io_service, endpoint);

		for (;;)
		{
			tcp::iostream stream;
			acceptor.accept(*stream.rdbuf());

			//reading the command
			std::string command;
			while(std::getline(stream, command))
			{
				std::cout<<"server processing command :"<<command<<std::endl;
				//processing the command
				if(command=="luxInit")
				{
					luxInit();
				}
				else if(command=="luxLookAt")
				{
					float ex, ey, ez, lx, ly, lz, ux, uy, uz;
					stream>>ex;
					stream>>ey;
					stream>>ez;
					stream>>lx;
					stream>>ly;
					stream>>lz;
					stream>>ux;
					stream>>uy;
					stream>>uz;
					std::cout<<"params :"<<ex<<", "<<ey<<", "<<ez<<", "<<lx<<", "<<ly<<", "<<lz<<", "<<ux<<", "<<uy<<", "<<uz<<std::endl;
					luxLookAt(ex, ey, ez, lx, ly, lz, ux, uy, uz);
				}
				else if(command=="luxCamera") //luxCamera(const string &, const ParamSet &cameraParams)
				{
					std::string type;
					ParamSet cameraParams;
					stream>>type;
					boost::archive::text_iarchive ia(stream);
					ia>>cameraParams;
					std::cout<<"params :"<<type<<", "<<cameraParams.ToString()<<std::endl;
					luxCamera(type,cameraParams);
				}

				std::cout<<"command processed"<<std::endl;
				//END OF COMMAND PROCESSING
			}
		}

	}
	catch (std::exception& e)
	{
		std::cerr << e.what() << std::endl;
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
	
	/*
	ParamSet p;
	bool j=true;
	int i=45;
	std::string s("yeyooolooo");

	//Point p(3,4.121212545454548787878787,5);

	//std::cout<<p.y<<std::endl;

	p.AddBool("monbooleen", &j);
	p.AddInt("monint", &i);

	//std::cout<<p.ToString()<<std::endl;

	const ParamSetItem<std::string> pi("mystring", &s, 1);

	std::ofstream ofs("filename");
	//save to archive
	{
		const ParamSet cp(p);
		ofs.setf(std::ios::scientific, std::ios::floatfield);
		ofs.precision(16);
		boost::archive::text_oarchive oa(ofs);
		oa<<cp;
	}

	Point q;

	//read from archive

	//ParamSetItem<std::string> rpsi;
	ParamSet rpsi;
	std::ifstream ifs("filename", std::ios::binary);
	boost::archive::text_iarchive ia(ifs);
	ia>>rpsi;
	//std::cout.precision(24);
	std::cout<<"read "<<rpsi.ToString()<<std::endl;*/

	try
	{

		// Declare a group of options that will be
		// allowed only on command line
		po::options_description generic ("Generic options");
		generic.add_options ()
		("version,v", "print version string") ("help", "produce help message") ("server","Launch in server mode");

		// Declare a group of options that will be
		// allowed both on command line and in
		// config file
		po::options_description config ("Configuration");
		config.add_options ()
		("threads,t", po::value < int >(), "Specify the number of threads that Lux will run in parallel.")
		("useserver,u", po::value < std::string >(), "Specify the adress of a rendering server to use.")
		;

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

		if (vm.count("useserver"))
		{
			std::string name=vm["useserver"].as<std::string>();
			std::cout<<"connecting to "<<name<<std::endl;
			try {

				tcp::iostream s(name.c_str(), "18018");
				/*
				 std::string line;
				 std::getline(s, line);
				 std::cout << line << std::endl;*/

				//send the command
				s<<"luxInit"<<std::endl;
				s<<"luxLookAt"<<std::endl<<0<<' '<<10<<' '<<100<<' '<<0<<' '<<-1<<' '<<0<<' '<<0<<' '<<1<<' '<<0;

				{
					ParamSet p;
					float f=30;
					p.AddFloat("fov",&f,1);
					const ParamSet cp(p);
					s<<"luxCamera"<<std::endl<<"perspective ";
					boost::archive::text_oarchive oa(s);
					oa<<cp;
				}

			}
			catch (std::exception& e)
			{
				std::cerr << e.what() << std::endl;
			}

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
		else if (vm.count ("server"))
		{
			startServer();
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
