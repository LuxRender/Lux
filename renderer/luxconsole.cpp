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
#include <boost/serialization/split_member.hpp>
#include <boost/serialization/vector.hpp>
#include <boost/serialization/string.hpp>

#include <boost/iostreams/filtering_stream.hpp>
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

	//luxInit();
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
	f(type.c_str(), params);
}

void processCommand(void (&f)(const string &), std::basic_istream<char> &stream) {
	std::string type;
	stream>>type;
	f(type.c_str());
}

void processCommand(void (&f)(float,float,float), std::basic_istream<char> &stream) {
	float ax, ay, az;
	stream>>ax;
	stream>>ay;
	stream>>az;
	f(ax, ay, az);
}

void processCommand(void (&f)(float[16]), std::basic_istream<char> &stream) {
	float t[16];
	for (int i=0; i<16; i++)
		stream>>t[i];
	f(t);
}



void startServer(int listenPort=18018) {
	
	//Command identifiers (hashed strings)
	//These identifiers are used in the switch statement to avoid string comparisons (fastest)
	static const unsigned int CMD_LUXINIT=2531407474U, CMD_LUXTRANSLATE=2039302476U, CMD_LUXROTATE=3982485453U, CMD_LUXSCALE=1943523046U,
	 CMD_LUXLOOKAT=3747502632U, CMD_LUXCONCATTRANSFORM=449663410U, CMD_LUXTRANSFORM=2039102042U, CMD_LUXIDENTITY=97907816U,
	 CMD_LUXCOORDINATESYSTEM=1707244427U, CMD_LUXCOORDSYSTRANSFORM=2024449520U, CMD_LUXPIXELFILTER=2384561510U,
	 CMD_LUXFILM=2531294310U, CMD_LUXSAMPLER=3308802546U, CMD_LUXACCELERATOR=1613429731U, CMD_LUXSURFACEINTEGRATOR=4011931910U,
	 CMD_LUXVOLUMEINTEGRATOR=2954204437U, CMD_LUXCAMERA=3378604391U, CMD_LUXWORLDBEGIN=1247285547U,
	 CMD_LUXATTRIBUTEBEGIN=684297207U, CMD_LUXATTRIBUTEEND=3427929065U, CMD_LUXTRANSFORMBEGIN=567425599U,
	 CMD_LUXTRANSFORMEND=2773125169U, CMD_LUXTEXTURE=475043887U, CMD_LUXMATERIAL=4064803661U, CMD_LUXLIGHTSOURCE=130489799U,
	 CMD_LUXAREALIGHTSOURCE=515057184U, CMD_LUXPORTALSHAPE=3416559329U, CMD_LUXSHAPE=1943702863U,
	 CMD_LUXREVERSEORIENTATION=2027239206U, CMD_LUXVOLUME=4138761078U, CMD_LUXOBJECTBEGIN=1097337658U,
	 CMD_LUXOBJECTEND=229760620U, CMD_LUXOBJECTINSTANCE=4125664042U, CMD_LUXWORLDEND=1582674973U, CMD_LUXGETFILM=859419430U,
	 CMD_VOID=5381U, CMD_SPACE=177605U;

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
			
			//reading the command
			std::string command;
			while(std::getline(stream, command))
			{
				if((command!="")&&(command!=" "))
				{
					std::stringstream ss;
					ss << "Server processing command : '" <<command<< "'";
					luxError(LUX_NOERROR, LUX_INFO, ss.str ().c_str());
				}
				
				//processing the command
				switch (DJBHash(command))
				{
					case CMD_VOID:
					case CMD_SPACE:
						break;
					case CMD_LUXINIT:	 luxError(LUX_BUG,LUX_SEVERE,"Server already initialized"); break;
					case CMD_LUXTRANSLATE: processCommand(Context::luxTranslate,stream); break;
					case CMD_LUXROTATE:
										{
											float angle, ax, ay, az;
											stream>>angle;
											stream>>ax;
											stream>>ay;
											stream>>az;
											//std::cout<<"params :"<<angle<<", "<<ax<<", "<<ay<<", "<<az<<std::endl;
											luxRotate(angle,ax,ay,az);
										}
										break;
					case CMD_LUXSCALE: processCommand(Context::luxScale,stream); break;
					case CMD_LUXLOOKAT :
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
											//std::cout<<"params :"<<ex<<", "<<ey<<", "<<ez<<", "<<lx<<", "<<ly<<", "<<lz<<", "<<ux<<", "<<uy<<", "<<uz<<std::endl;
											luxLookAt(ex, ey, ez, lx, ly, lz, ux, uy, uz);
										}
										break;
					case CMD_LUXCONCATTRANSFORM : processCommand(Context::luxConcatTransform,stream); break;
					case CMD_LUXTRANSFORM : processCommand(Context::luxTransform,stream); break;
					case CMD_LUXIDENTITY : Context::luxIdentity(); break;
					case CMD_LUXCOORDINATESYSTEM : processCommand(Context::luxCoordinateSystem,stream); break;
					case CMD_LUXCOORDSYSTRANSFORM : processCommand(Context::luxCoordSysTransform,stream); break;
					case CMD_LUXPIXELFILTER :  processCommand(Context::luxPixelFilter,stream); break;
					case CMD_LUXFILM : processCommand(Context::luxFilm,stream); break;
					case CMD_LUXSAMPLER : processCommand(Context::luxSampler,stream); break;
					case CMD_LUXACCELERATOR : processCommand(Context::luxAccelerator,stream); break;
					case CMD_LUXSURFACEINTEGRATOR : processCommand(Context::luxSurfaceIntegrator,stream); break;
					case CMD_LUXVOLUMEINTEGRATOR : processCommand(Context::luxVolumeIntegrator,stream); break;
					case CMD_LUXCAMERA : processCommand(Context::luxCamera,stream); break;
					case CMD_LUXWORLDBEGIN : Context::luxWorldBegin(); break;
					case CMD_LUXATTRIBUTEBEGIN : Context::luxAttributeBegin(); break;
					case CMD_LUXATTRIBUTEEND : Context::luxAttributeEnd(); break;
					case CMD_LUXTRANSFORMBEGIN : Context::luxTransformBegin(); break;
					case CMD_LUXTRANSFORMEND : Context::luxTransformEnd(); break;
					case CMD_LUXTEXTURE :
										{
											std::string name,type,texname;
											ParamSet params;
											stream>>name;
											stream>>type;
											stream>>texname;
											boost::archive::text_iarchive ia(stream);
											ia>>params;
											//std::cout<<"params :"<<name<<", "<<type<<", "<<texname<<", "<<params.ToString()<<std::endl;

											std::string file="";
											file=params.FindOneString(std::string("filename"),file);
											if(file.size())
											{
												//std::cout<<"receiving file..."<<file<<std::endl;
												{
													std::stringstream ss;
													ss << "Receiving file : '" <<file<< "'";
													luxError(LUX_NOERROR, LUX_INFO, ss.str ().c_str());
												}
												
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
										break;
					case CMD_LUXMATERIAL: processCommand(Context::luxMaterial,stream); break;
					case CMD_LUXLIGHTSOURCE : processCommand(Context::luxLightSource,stream); break;
					case CMD_LUXAREALIGHTSOURCE : processCommand(Context::luxAreaLightSource,stream); break;
					case CMD_LUXPORTALSHAPE : processCommand(Context::luxPortalShape,stream); break;
					case CMD_LUXSHAPE : processCommand(Context::luxShape,stream); break;
					case CMD_LUXREVERSEORIENTATION : Context::luxReverseOrientation(); break;
					case CMD_LUXVOLUME : processCommand(Context::luxVolume,stream); break;
					case CMD_LUXOBJECTBEGIN : processCommand(Context::luxObjectBegin,stream); break;
					case CMD_LUXOBJECTEND : Context::luxObjectEnd(); break;
					case CMD_LUXOBJECTINSTANCE : processCommand(Context::luxObjectInstance,stream); break;
					case CMD_LUXWORLDEND :
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
										break;
					case CMD_LUXGETFILM :
										{
											//std::cout<<"transmitting film...."<<std::endl;
											luxError(LUX_NOERROR, LUX_INFO, "Transmitting film.");

											Context::getActive()->getFilm(stream);
											stream.close();
											//std::cout<<"...ok"<<std::endl;
											luxError(LUX_NOERROR, LUX_INFO, "Finished film transmission.");
										}
										break;
					default:
						std::stringstream ss;
						ss<<"Unknown command '"<<command<<"'. Ignoring.";
						luxError(LUX_BUG,LUX_SEVERE,ss.str().c_str());break;
				}
				
				//END OF COMMAND PROCESSING
			}
		}

	}
	catch (std::exception& e)
	{
		luxError(LUX_BUG,LUX_ERROR,e.what());
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
			std::cout<<"Testing..."<<std::endl;
						/*
						LookAt 0 10 100   0 -1 0 0 1 0
						Camera "perspective" "float fov" [30]
						PixelFilter "mitchell" "float xwidth" [2] "float ywidth" [2]
						Sampler "bestcandidate"
						Film "image" "string filename" ["simple.exr"]
						     "integer xresolution" [200] "integer yresolution" [200]
						
						WorldBegin
						
						AttributeBegin
						  CoordSysTransform "camera"
						  LightSource "distant" 
						              "point from" [0 0 0] "point to"   [0 0 1]
						              "color L"    [3 3 3]
						AttributeEnd
						
						AttributeBegin
						  Rotate 135 1 0 0
						  
						  Texture "checks" "color" "checkerboard" 
						          "float uscale" [4] "float vscale" [4] 
						          "color tex1" [1 0 0] "color tex2" [0 0 1]
						  
						  Material "matte" 
						           "texture Kd" "checks"
						  Shape "disk" "float radius" [20] "float height" [-1]
						AttributeEnd
						WorldEnd
						*/
						
						
						luxLookAt(0,10,100,0,-1,0,0,1,0);
						float fov=30;
						float size=2, radius=20, height=-1;
						int resolution=200;
						float from[3]= {0,0,0};
						float to[3]= {0,0,1};
						float color1[3]={1,0,0},color2[3]={0,0,1};
						float scale=4;
						char *filename="simple.png";
						luxCamera("perspective","fov",&fov,LUX_NULL);
						luxPixelFilter("mitchell","xwidth", &size, "ywidth" , &size,LUX_NULL);
						luxSampler("random",LUX_NULL);
						luxFilm("multiimage","filename",filename,"xresolution",&resolution,"yresolution",&resolution,LUX_NULL);
						luxWorldBegin();
						
						luxAttributeBegin();
						luxCoordSysTransform("camera");
						luxLightSource("distant" ,"from",from,"to",to,LUX_NULL);
						luxAttributeEnd();
						
						luxAttributeBegin();
						luxRotate(135,1,0,0);
						
						luxTexture("checks","color","checkerboard" ,"uscale",&scale,"vscale",&scale,"tex1",&color1, "tex2",&color2,LUX_NULL);
						luxMaterial("matte","Kd","checks",LUX_NULL);
						luxShape("disk","radius",&radius,"height",&height,LUX_NULL);
						luxAttributeEnd();
						luxWorldEnd();
						
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

			for(std::vector<std::string>::iterator i=names.begin();i<names.end();i++)
			{
				//std::cout<<"connecting to "<<(*i)<<std::endl;
				std::stringstream ss;
				ss<<"Connecting to server '"<<(*i)<<"'";
				luxError(LUX_NOERROR,LUX_INFO,ss.str().c_str());
				
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
