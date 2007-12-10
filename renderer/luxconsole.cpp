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
#include "../core/paramset.h"
#include <fstream>
#include <boost/archive/text_oarchive.hpp>
#include <boost/archive/text_iarchive.hpp>
//#include <boost/archive/xml_oarchive.hpp>
//#include <boost/archive/xml_iarchive.hpp>
#include <boost/serialization/split_member.hpp>
#include <boost/serialization/vector.hpp>
#include <boost/serialization/string.hpp>

#include <iomanip>

#ifdef WIN32
#include "direct.h"
#define chdir _chdir
#endif

#include "multiimage.h"

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

		std::cout<<'\r' <<progress[(pc++)%4] <<'['<<threads<<']' <<td <<" "
				<<(int)luxStatistics("samplesSec")<<" samples/sec " <<" "
				<<(float)luxStatistics("samplesPx")<<" samples/pix" <<"    "
				<<std::flush;
	}
}

void processCommand(void (&f)(const string &, const ParamSet &), tcp::iostream &stream) {
	std::string type;
	ParamSet params;
	stream>>type;
	boost::archive::text_iarchive ia(stream);
	ia>>params;
	//std::cout<<"params :"<<type<<", "<<params.ToString()<<std::endl;
	f(type, params);
}

void processCommand(void (&f)(const string &), tcp::iostream &stream) {
	std::string type;
	stream>>type;
	//std::cout<<"params :"<<type<<std::endl;
	f(type);
}

void processCommand(void (&f)(float,float,float), tcp::iostream &stream) {
	float ax, ay, az;
	stream>>ax;
	stream>>ay;
	stream>>az;
	//std::cout<<"params :"<<ax<<", "<<ay<<", "<<az<<std::endl;
	f(ax, ay, az);
}

void processCommand(void (&f)(float[16]), tcp::iostream &stream) {
	float t[16];
	for (int i=0; i<16; i++)
		stream>>t[i];
	//std::cout<<"params :"<<ax<<", "<<ay<<", "<<az<<std::endl;
	f(t);
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
				std::cout<<"server processing command : '"<<command<<"'"<<std::endl;
				//processing the command
				if(command==""); //skip
				else if(command=="luxInit") luxInit();
				else if(command=="luxTranslate") processCommand(luxTranslate,stream);
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
				else if(command=="luxScale") processCommand(luxScale,stream);
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
				else if(command=="luxConcatTransform") processCommand(luxConcatTransform,stream);
				else if(command=="luxTransform") processCommand(luxTransform,stream);
				else if(command=="luxIdentity") luxIdentity();
				else if(command=="luxCoordinateSystem") processCommand(luxCoordinateSystem,stream);
				else if(command=="luxCoordSysTransform") processCommand(luxCoordSysTransform,stream);
				else if(command=="luxPixelFilter") processCommand(luxPixelFilter,stream);
				else if(command=="luxFilm") processCommand(luxFilm,stream);
				else if(command=="luxSampler") processCommand(luxSampler,stream);
				else if(command=="luxAccelerator") processCommand(luxAccelerator,stream);
				else if(command=="luxSurfaceIntegrator") processCommand(luxSurfaceIntegrator,stream);
				else if(command=="luxVolumeIntegrator") processCommand(luxVolumeIntegrator,stream);
				else if(command=="luxCamera") processCommand(luxCamera,stream);
				//else if(command=="luxSearchPath") processCommand(luxSearchPath,stream);
				else if(command=="luxWorldBegin") luxWorldBegin();
				else if(command=="luxAttributeBegin") luxAttributeBegin();
				else if(command=="luxAttributeEnd") luxAttributeEnd();
				else if(command=="luxTransformBegin") luxTransformBegin();
				else if(command=="luxTransformEnd") luxTransformEnd();
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
					luxTexture(name,type,texname,params);
				}
				else if(command=="luxMaterial") processCommand(luxMaterial,stream);
				else if(command=="luxLightSource") processCommand(luxLightSource,stream);
				else if(command=="luxAreaLightSource") processCommand(luxAreaLightSource,stream);
				else if(command=="luxPortalShape") processCommand(luxPortalShape,stream);
				else if(command=="luxShape") processCommand(luxShape,stream);
				else if(command=="luxReverseOrientation") luxReverseOrientation();
				else if(command=="luxVolume") processCommand(luxVolume,stream);
				else if(command=="luxObjectBegin") processCommand(luxObjectBegin,stream);
				else if(command=="luxObjectEnd") luxObjectEnd();
				else if(command=="luxObjectInstance") processCommand(luxObjectInstance,stream);
				else if(command=="luxWorldEnd")
				{
					boost::thread t(&luxWorldEnd);
					boost::thread j(&infoThread);
				}
				else if(command=="luxFilm")
				{
					std::cout<<"transmitting film...."<<std::endl;
					luxFilm(stream);
					std::cout<<"...ok"<<std::endl;
				}
				else
				{
					std::cout<<"unknown command!"<<std::endl;
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

/*
 void test()
 {
 //send the command
 luxInit();
 //LookAt 0 10 100   0 -1 0 0 1 0
 luxLookAt(0,10,100,0,-1,0,0,1,0);
 //Camera "perspective" "float fov" [30]
 {
 ParamSet p;
 float f=30;
 p.AddFloat("fov",&f,1);
 const ParamSet cp(p);
 luxCamera("perspective",cp);
 }
 //PixelFilter "mitchell" "float xwidth" [2] "float ywidth" [2]
 {
 ParamSet p;
 float f=2;
 p.AddFloat("xwidth",&f,1);
 p.AddFloat("ywidth",&f,1);
 const ParamSet cp(p);
 luxPixelFilter("mitchell",cp);
 }
 //Sampler "lowdiscrepancy" "bool progressive" ["true"] "integer pixelsamples" [4]
 {
 ParamSet p;
 bool prog=true;
 int ps=4;
 p.AddBool("progressive",&prog,1);
 p.AddInt("pixelsamples",&ps,1);
 const ParamSet cp(p);
 luxSampler("lowdiscrepancy",cp);

 }
 //Film "multiimage" "integer xresolution" [200] "integer yresolution" [200]
 {
 ParamSet p;
 int r=200;
 p.AddInt("xresolution",&r,1);
 p.AddInt("yresolution",&r,1);
 const ParamSet cp(p);
 luxFilm("multiimage",cp);

 }
 //
 luxWorldBegin();
 luxAttributeBegin();
 luxCoordSysTransform("camera");

 //LightSource "distant"  "point from" [0 0 0] "point to"   [0 0 1] "color L"    [3 3 3]
 {
 ParamSet p;
 Point from(0,0,0),to(0,0,1);
 float cs[]= {3,3,3};
 Spectrum color(cs);
 p.AddPoint("from",&from,1);
 p.AddPoint("to",&to,1);
 p.AddSpectrum("L",&color,1);
 const ParamSet cp(p);
 luxLightSource("distant",cp);

 }

 luxRotate(135,1,0,0);

 //Texture "checks" "color" "checkerboard"
 //         "float uscale" [4] "float vscale" [4]
 //        "color tex1" [1 0 0] "color tex2" [0 0 1]
 {
 ParamSet p;
 float scale=4;
 float t1[]= {1,0,0},t2[]= {0,0,1};
 Spectrum c1(t1),c2(t2);
 p.AddFloat("uscale",&scale,1);
 p.AddFloat("vscale",&scale,1);
 p.AddSpectrum("tex1",&c1,1);
 p.AddSpectrum("tex2",&c2,1);
 const ParamSet cp(p);
 luxTexture("checks", "color", "checkerboard",cp);

 }
 //Material "matte"
 //           "texture Kd" "checks"
 {
 ParamSet p;
 p.AddTexture(std::string("Kd"),std::string("checks"));
 const ParamSet cp(p);
 luxMaterial("matte",cp);
 }
 //Shape "disk" "float radius" [20] "float height" [-1]
 {
 ParamSet p;
 float r=20,h=-1;
 p.AddFloat("radius",&r,1);
 p.AddFloat("height",&h,1);
 const ParamSet cp(p);
 luxShape("disk",cp);

 }
 luxAttributeEnd();
 luxWorldEnd();
 }*/

int main(int ac, char *av[]) {

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
		("useserver,u", po::value < std::string >(), "Specify the adress of a rendering server to use.")
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
			stream<<"luxFilm"<<std::endl;
			boost::archive::text_iarchive ia(stream);
			MultiImageFilm m(320,200);
			Point p;
			ia>>m;
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
			threads=1;;
		}

		if (vm.count("useserver"))
		{
			std::string name=vm["useserver"].as<std::string>();
			std::cout<<"connecting to "<<name<<std::endl;

			//TODO jromang : try to connect to the server, and get version number. display message to see if it was successfull
			luxAddServer(name);

			/*
			 std::string name=vm["useserver"].as<std::string>();
			 std::cout<<"connecting to "<<name<<std::endl;
			 try {

			 tcp::iostream s(name.c_str(), "18018");


			 //send the command
			 s<<"luxInit"<<std::endl;
			 //LookAt 0 10 100   0 -1 0 0 1 0
			 s<<"luxLookAt"<<std::endl<<0<<' '<<10<<' '<<100<<' '<<0<<' '<<-1<<' '<<0<<' '<<0<<' '<<1<<' '<<0;
			 //Camera "perspective" "float fov" [30]
			 {
			 ParamSet p;
			 float f=30;
			 p.AddFloat("fov",&f,1);
			 const ParamSet cp(p);
			 s<<"luxCamera"<<std::endl<<"perspective ";
			 boost::archive::text_oarchive oa(s);
			 oa<<cp;
			 }
			 //PixelFilter "mitchell" "float xwidth" [2] "float ywidth" [2]
			 {
			 ParamSet p;
			 float f=2;
			 p.AddFloat("xwidth",&f,1);
			 p.AddFloat("ywidth",&f,1);
			 const ParamSet cp(p);
			 s<<"luxPixelFilter"<<std::endl<<"mitchell ";
			 boost::archive::text_oarchive oa(s);
			 oa<<cp;
			 }
			 //Sampler "lowdiscrepancy" "bool progressive" ["true"] "integer pixelsamples" [4]
			 {
			 ParamSet p;
			 bool prog=true;
			 int ps=4;
			 p.AddBool("progressive",&prog,1);
			 p.AddInt("pixelsamples",&ps,1);
			 const ParamSet cp(p);
			 s<<"luxSampler"<<std::endl<<"lowdiscrepancy ";
			 boost::archive::text_oarchive oa(s);
			 oa<<cp;
			 }
			 //Film "multiimage" "integer xresolution" [200] "integer yresolution" [200]
			 {
			 ParamSet p;
			 int r=200;
			 p.AddInt("xresolution",&r,1);
			 p.AddInt("yresolution",&r,1);
			 const ParamSet cp(p);
			 s<<"luxFilm"<<std::endl<<"multiimage ";
			 boost::archive::text_oarchive oa(s);
			 oa<<cp;
			 }
			 //
			 s<<"luxWorldBegin"<<std::endl<<"luxAttributeBegin"<<std::endl<<"luxCoordSysTransform"<<std::endl<<"camera"<<std::endl;

			 //LightSource "distant"  "point from" [0 0 0] "point to"   [0 0 1] "color L"    [3 3 3]
			 {
			 ParamSet p;
			 Point from(0,0,0),to(0,0,1);
			 float cs[]= {3,3,3};
			 Spectrum color(cs);
			 p.AddPoint("from",&from,1);
			 p.AddPoint("to",&to,1);
			 p.AddSpectrum("L",&color,1);
			 const ParamSet cp(p);
			 s<<"luxLightSource"<<std::endl<<"distant ";
			 boost::archive::text_oarchive oa(s);
			 oa<<cp;
			 }

			 s<<"luxRotate"<<std::endl<<135<<' '<<1<<' '<<0<<' '<<0<<std::endl;

			 //Texture "checks" "color" "checkerboard"
			 //         "float uscale" [4] "float vscale" [4]
			 //        "color tex1" [1 0 0] "color tex2" [0 0 1]
			 {
			 ParamSet p;
			 float scale=4;
			 float t1[]= {1,0,0},t2[]= {0,0,1};
			 Spectrum c1(t1),c2(t2);
			 p.AddFloat("uscale",&scale,1);
			 p.AddFloat("vscale",&scale,1);
			 p.AddSpectrum("tex1",&c1,1);
			 p.AddSpectrum("tex2",&c2,1);
			 const ParamSet cp(p);
			 s<<"luxTexture"<<std::endl<<"checks color checkerboard ";
			 boost::archive::text_oarchive oa(s);
			 oa<<cp;
			 }
			 //Material "matte"
			 //           "texture Kd" "checks"
			 {
			 ParamSet p;
			 p.AddTexture(std::string("Kd"),std::string("checks"));
			 const ParamSet cp(p);
			 s<<"luxMaterial"<<std::endl<<"matte ";
			 boost::archive::text_oarchive oa(s);
			 oa<<cp;
			 }
			 //Shape "disk" "float radius" [20] "float height" [-1]
			 {
			 ParamSet p;
			 float r=20,h=-1;
			 p.AddFloat("radius",&r,1);
			 p.AddFloat("height",&h,1);
			 const ParamSet cp(p);
			 s<<"luxShape"<<std::endl<<"disk ";
			 boost::archive::text_oarchive oa(s);
			 oa<<cp;
			 }
			 s<<"luxAttributeEnd"<<std::endl<<"luxWorldEnd"<<std::endl;

			 }
			 catch (std::exception& e)
			 {
			 std::cerr << e.what() << std::endl;
			 }
			 */

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
