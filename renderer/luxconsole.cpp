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
#include <sstream>
#include <exception>
#include <vector>
#include <ctime>
#include <boost/bind.hpp>
#include <boost/program_options.hpp>
#include <boost/thread.hpp>
#include <boost/thread/xtime.hpp>
#include <boost/date_time/posix_time/posix_time.hpp>
#include <boost/filesystem/path.hpp>
#include <boost/filesystem/operations.hpp>

#include "include/asio.hpp"
#include "../core/paramset.h"
#include <fstream>
#include <boost/archive/text_oarchive.hpp>
#include <boost/archive/text_iarchive.hpp>
//#include <boost/archive/xml_oarchive.hpp>
//#include <boost/archive/xml_iarchive.hpp>
#include <boost/serialization/split_member.hpp>
#include <boost/serialization/vector.hpp>
#include <boost/serialization/string.hpp>

#include <boost/iostreams/filtering_stream.hpp>
//#include <boost/iostreams/copy.hpp>
#include <boost/iostreams/filter/zlib.hpp>

#include <iomanip>

#ifdef WIN32
#include "direct.h"
#define chdir _chdir
#endif

#include "multiimage.h"
#include "context.h"

using namespace boost::iostreams;
using namespace lux;
using asio::ip::tcp;
namespace po = boost::program_options;

std::string sceneFileName;
int threads;
bool parseError;

void engineThread() {

	luxInit();
	ParseFile(sceneFileName.c_str());
	if (luxStatistics("sceneIsReady") == false)
		parseError = true;
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

		std::cout<<'\r' <<' '<<progress[(pc++)%4] <<'['<<threads<<']' <<td <<" "
				<<(int)luxStatistics("samplesSec")<<" samples/sec " <<" "
				<<(float)luxStatistics("samplesPx")<<" samples/pix" <<"    "
				<<'\r'<<std::flush;
	}
}

void networkFilmUpdateThread() {
	while (true) {

		boost::xtime xt;
		boost::xtime_get(&xt, boost::TIME_UTC);
		xt.sec += 59;
		boost::thread::sleep(xt);

		boost::posix_time::time_duration td(0, 0,
				(int)luxStatistics("secElapsed"), 0);

		luxUpdateFilmFromNetwork();
	}
}

void processCommand(void (&f)(const string &, const ParamSet &), std::basic_istream<char> &stream) {
	std::string type;
	ParamSet params;
	stream>>type;
	boost::archive::text_iarchive ia(stream);
	ia>>params;
	//std::cout<<"params :"<<type<<", "<<params.ToString()<<std::endl;
	f(type.c_str(), params);
}

void processCommand(void (&f)(const string &), std::basic_istream<char> &stream) {
	std::string type;
	stream>>type;
	//std::cout<<"params :"<<type<<std::endl;
	f(type.c_str());
}

void processCommand(void (&f)(float,float,float), std::basic_istream<char> &stream) {
	float ax, ay, az;
	stream>>ax;
	stream>>ay;
	stream>>az;
	//std::cout<<"params :"<<ax<<", "<<ay<<", "<<az<<std::endl;
	f(ax, ay, az);
}

void processCommand(void (&f)(float[16]), std::basic_istream<char> &stream) {
	float t[16];
	for (int i=0; i<16; i++)
		stream>>t[i];
	//std::cout<<"params :"<<ax<<", "<<ay<<", "<<az<<std::endl;
	f(t);
}

/*
void processCommand(void (&f)(const char *, ...), std::basic_istream<char> &stream) {
	std::string type;
	ParamSet params;
	stream>>type;
	boost::archive::text_iarchive ia(stream);
	ia>>params;
	//std::cout<<"params :"<<type<<", "<<params.ToString()<<std::endl;
	f(type.c_str(), params);
}*/

void startServer(int listenPort=18018) {
	
	luxInit();
	//std::cout<<"luxconsole: launching server mode..."<<std::endl;
	{
		std::stringstream ss;
		ss<<"Launching server mode on port '"<<listenPort<<"'.";
		luxError(LUX_NOERROR, LUX_INFO, ss.str().c_str());
	}

	try
	{
		asio::io_service io_service;
		tcp::endpoint endpoint(tcp::v4(), listenPort);
		tcp::acceptor acceptor(io_service, endpoint);

		for (;;)
		{
			tcp::iostream stream;
			acceptor.accept(*stream.rdbuf());
			stream.setf(std::ios::scientific, std::ios::floatfield);
			stream.precision(16);

			//filtering_stream<bidirectional> stream;
			//stream.push(zlib_decompressor());
			//stream.push(tcpStream);
			
			//reading the command
			std::string command;
			while(std::getline(stream, command))
			{
				if((command!="")&&(command!=" ")) std::cout<<"server processing command : '"<<command<<"'"<<std::endl;
				//processing the command
				if(command==""); //skip
				else if(command==" "); //skip
				else if(command=="luxInit") luxError(LUX_BUG,LUX_SEVERE,"Server already initialized");//luxInit();
				else if(command=="luxTranslate") processCommand(Context::luxTranslate,stream);
				else if(command=="luxRotate")
				{
					float angle, ax, ay, az;
					stream>>angle;
					stream>>ax;
					stream>>ay;
					stream>>az;
					std::cout<<"params :"<<angle<<", "<<ax<<", "<<ay<<", "<<az<<std::endl;
					luxRotate(angle,ax,ay,az);
				}
				else if(command=="luxScale") processCommand(Context::luxScale,stream);
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
				else if(command=="luxConcatTransform") processCommand(Context::luxConcatTransform,stream);
				else if(command=="luxTransform") processCommand(Context::luxTransform,stream);
				else if(command=="luxIdentity") Context::luxIdentity();
				else if(command=="luxCoordinateSystem") processCommand(Context::luxCoordinateSystem,stream);
				else if(command=="luxCoordSysTransform") processCommand(Context::luxCoordSysTransform,stream);
				else if(command=="luxPixelFilter") processCommand(Context::luxPixelFilter,stream);
				else if(command=="luxFilm") processCommand(Context::luxFilm,stream);
				else if(command=="luxSampler") processCommand(Context::luxSampler,stream);
				else if(command=="luxAccelerator") processCommand(Context::luxAccelerator,stream);
				else if(command=="luxSurfaceIntegrator") processCommand(Context::luxSurfaceIntegrator,stream);
				else if(command=="luxVolumeIntegrator") processCommand(Context::luxVolumeIntegrator,stream);
				else if(command=="luxCamera") processCommand(Context::luxCamera,stream);
				//else if(command=="luxSearchPath") processCommand(luxSearchPath,stream);
				else if(command=="luxWorldBegin") Context::luxWorldBegin();
				else if(command=="luxAttributeBegin") Context::luxAttributeBegin();
				else if(command=="luxAttributeEnd") Context::luxAttributeEnd();
				else if(command=="luxTransformBegin") Context::luxTransformBegin();
				else if(command=="luxTransformEnd") Context::luxTransformEnd();
				else if(command=="luxTexture")
				{
					std::string name,type,texname;
					ParamSet params;
					stream>>name;
					stream>>type;
					stream>>texname;
					boost::archive::text_iarchive ia(stream);
					ia>>params;
					std::cout<<"params :"<<name<<", "<<type<<", "<<texname<<", "<<params.ToString()<<std::endl;

					std::string file="";
					file=params.FindOneString(std::string("filename"),file);
					if(file.size())
					{
						std::cout<<"receiving file..."<<file<<std::endl;
						bool first=true;
						std::string s;
						std::ofstream out(file.c_str(),std::ios::out|std::ios::binary);
						while(getline(stream,s) && s!="LUX_END_FILE")
						{
							if(!first)out<<"\n";
							first=false;
							out<<s;
						}
						out.flush();
					}

					Context::luxTexture(name.c_str(),type.c_str(),texname.c_str(),params);
				}
				else if(command=="luxMaterial") processCommand(Context::luxMaterial,stream);
				else if(command=="luxLightSource") processCommand(Context::luxLightSource,stream);
				else if(command=="luxAreaLightSource") processCommand(Context::luxAreaLightSource,stream);
				else if(command=="luxPortalShape") processCommand(Context::luxPortalShape,stream);
				else if(command=="luxShape") processCommand(Context::luxShape,stream);
				else if(command=="luxReverseOrientation") Context::luxReverseOrientation();
				else if(command=="luxVolume") processCommand(Context::luxVolume,stream);
				else if(command=="luxObjectBegin") processCommand(Context::luxObjectBegin,stream);
				else if(command=="luxObjectEnd") Context::luxObjectEnd();
				else if(command=="luxObjectInstance") processCommand(Context::luxObjectInstance,stream);
				else if(command=="luxWorldEnd")
				{
					boost::thread t(&luxWorldEnd);
					

					//wait the scene parsing to finish
					while(!luxStatistics("sceneIsReady") && !parseError)
					{
						boost::xtime xt;
						boost::xtime_get(&xt, boost::TIME_UTC);
						xt.sec += 1;
						boost::thread::sleep(xt);
					}

					boost::thread j(&infoThread);
					
					//add rendering threads
					int threadsToAdd=threads;
					while(--threadsToAdd)
					{
						Context::luxAddThread();
					}
				}
				else if(command=="luxGetFilm")
				{
					std::cout<<"transmitting film...."<<std::endl;
					Context::getActive()->getFilm(stream);
					stream.close();
					std::cout<<"...ok"<<std::endl;
				}
				else
				{
					std::stringstream ss;
					ss<<"Unknown command '"<<command<<"'. Ignoring.";
					luxError(LUX_BUG,LUX_SEVERE,ss.str().c_str());
				}

				std::cout<<"command processed"<<std::endl;
				//END OF COMMAND PROCESSING
			}
		}

	}
	catch (std::exception& e)
	{
		luxError(LUX_BUG,LUX_ERROR,e.what());
		//std::cerr << e.what() << std::endl;
	}
}

int main(int ac, char *av[]) {

	bool useServer=false;
	luxInit();
	//test();
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
		("useserver,u", po::value< std::vector<std::string> >()->composing(), "Specify the adress of a rendering server to use.")
		;

		// Hidden options, will be allowed both on command line and
		// in config file, but will not be shown to the user.
		po::options_description hidden ("Hidden options");
		hidden.add_options ()
		("input-file", po::value< vector<string> >(), "input file") ("test","debug test mode");

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

		if (vm.count ("test"))
		{
			std::cout << "getting film..."<<std::endl;
			tcp::iostream stream("127.0.0.1", "18018");
			std::cout << "connected"<<std::endl;
			stream<<"luxGetFilm"<<std::endl;
			boost::archive::text_iarchive ia(stream);
			MultiImageFilm m(320,200);
			Point p;
			ia>>m;
			std::cout<<"ok, i got the film!";
			//m.WriteImage();
			return 0;
		}

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
			threads=1;
		}

		if (vm.count("useserver"))
		{
			std::vector<std::string> names=vm["useserver"].as<std::vector<std::string> >();
			//std::cout<<"connecting to "<<name<<std::endl;

			for(std::vector<std::string>::iterator i=names.begin();i<names.end();i++)
			{
				std::cout<<"connecting to "<<(*i)<<std::endl;
				//TODO jromang : try to connect to the server, and get version number. display message to see if it was successfull		
				luxAddServer((*i).c_str());
			}
			

			useServer=true;
		}

		if (vm.count ("input-file"))
		{
			const std::vector<std::string> &v = vm["input-file"].as < vector<string> > ();
			for (unsigned int i = 0; i < v.size(); i++)
			{
				//change the working directory
				boost::filesystem::path fullPath( boost::filesystem::initial_path() );
				fullPath = boost::filesystem::system_complete( boost::filesystem::path( v[i], boost::filesystem::native ) );

				if(!boost::filesystem::exists(fullPath) && v[i] != "-")
				{
					std::stringstream ss;
					ss<<"Unable to open scenefile '"<<fullPath.string()<<"'";
					luxError(LUX_NOFILE, LUX_SEVERE, ss.str ().c_str());
					continue;
				}

				sceneFileName=fullPath.leaf();
				chdir (fullPath.branch_path().string().c_str());

				parseError = false;
				boost::thread t(&engineThread);

				//wait the scene parsing to finish
				while(!luxStatistics("sceneIsReady") && !parseError)
				{
					boost::xtime xt;
					boost::xtime_get(&xt, boost::TIME_UTC);
					xt.sec += 1;
					boost::thread::sleep(xt);
				}

				if(parseError)
				{
					std::stringstream ss;
					ss<<"Skipping invalid scenefile '"<<fullPath.string()<<"'";
					luxError(LUX_BADFILE, LUX_SEVERE, ss.str ().c_str());
					continue;
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
				if(useServer) boost::thread k(&networkFilmUpdateThread);

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
		std::stringstream ss;
		ss << "Command line argument parsing failed with error '" <<e.what ()<< "', please use the --help option to view the allowed syntax.";
		luxError(LUX_SYNTAX, LUX_SEVERE, ss.str ().c_str());
		return 1;
	}
	return 0;
}
