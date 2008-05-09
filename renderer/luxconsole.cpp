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

//// luxgui.cpp*
#define NDEBUG 1

#include "lux.h"
#include "api.h"
#include "error.h"
#include "paramset.h"
#include "include/asio.hpp"

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

#include <fstream>
#include <boost/archive/text_oarchive.hpp>
#include <boost/archive/text_iarchive.hpp>
#include <boost/serialization/split_member.hpp>
#include <boost/serialization/vector.hpp>
#include <boost/serialization/string.hpp>

#include <boost/iostreams/filtering_stream.hpp>
#include <boost/iostreams/filter/zlib.hpp>

#include <iomanip>

#if defined(WIN32) && !defined(__CYGWIN__)
#include "direct.h"
#define chdir _chdir
#endif

#include "fleximage.h"
#include "context.h"

using namespace boost::iostreams;
using namespace lux;
using asio::ip::tcp;
namespace po = boost::program_options;

std::string sceneFileName;
int threads;
int serverInterval;
bool parseError;

void engineThread() {
    //luxInit();
    ParseFile(sceneFileName.c_str());
    if (luxStatistics("sceneIsReady") == false)
        parseError = true;
    luxCleanup();
}

void infoThread() {
    while (true) {
        boost::xtime xt;
        boost::xtime_get(&xt, boost::TIME_UTC);
        xt.sec += 5;
        boost::thread::sleep(xt);

        boost::posix_time::time_duration td(0, 0,
                (int) luxStatistics("secElapsed"), 0);

        std::stringstream ss;
        ss << '[' << threads << " threads] " << td << " "
                << (int) luxStatistics("samplesSec") << " samples/sec " << " "
                << (int) luxStatistics("samplesTotSec") << " samples/totsec " << " "
                << (float) luxStatistics("samplesPx") << " samples/pix";
        luxError(LUX_NOERROR, LUX_INFO, ss.str().c_str());
    }
}

void networkFilmUpdateThread() {
    boost::xtime xt;
    boost::xtime_get(&xt, boost::TIME_UTC);
    xt.sec += serverInterval;

    while (true) {
        boost::thread::sleep(xt);
        boost::xtime_get(&xt, boost::TIME_UTC);
        xt.sec += serverInterval;

        luxUpdateFilmFromNetwork();
    }
}

void processCommandFilm(void (&f)(const string &, const ParamSet &), std::basic_istream<char> &stream) {
    std::string type;
    ParamSet params;
    stream >> type;

    if((type != "fleximage") && (type != "multiimage")) {
        std::stringstream ss;
        ss << "Unsupported film type for server rendering: " << type;
        luxError(LUX_SYSTEM, LUX_ERROR, ss.str().c_str());

        return;
    }

    boost::archive::text_iarchive ia(stream);
    ia >> params;

    // Dade - overwrite some option for the servers

    params.EraseBool("write_tonemapped_exr");
    params.EraseBool("write_untonemapped_exr");
    params.EraseBool("write_untonemapped_igi");
    params.EraseBool("write_tonemapped_tga");

    params.AddBool("write_tonemapped_exr", new bool(false));
    params.AddBool("write_untonemapped_exr", new bool(false));
    params.AddBool("write_untonemapped_igi", new bool(false));
    params.AddBool("write_tonemapped_tga", new bool(false));

    f(type.c_str(), params);
}

void processCommand(void (&f)(const string &, const ParamSet &), std::basic_istream<char> &stream) {
    std::string type;
    ParamSet params;
    stream >> type;
    boost::archive::text_iarchive ia(stream);
    ia >> params;
    f(type.c_str(), params);
}

void processCommand(void (&f)(const string &), std::basic_istream<char> &stream) {
    std::string type;
    stream >> type;
    f(type.c_str());
}

void processCommand(void (&f)(float, float, float), std::basic_istream<char> &stream) {
    float ax, ay, az;
    stream >> ax;
    stream >> ay;
    stream >> az;
    f(ax, ay, az);
}

void processCommand(void (&f)(float[16]), std::basic_istream<char> &stream) {
    float t[16];
    for (int i = 0; i < 16; i++)
        stream >> t[i];
    f(t);
}

void startServer(int listenPort = 18018) {
    //Command identifiers (hashed strings)
    //These identifiers are used in the switch statement to avoid string comparisons (fastest)
    static const unsigned int CMD_LUXINIT = 2531407474U, CMD_LUXTRANSLATE = 2039302476U, CMD_LUXROTATE = 3982485453U, CMD_LUXSCALE = 1943523046U,
            CMD_LUXLOOKAT = 3747502632U, CMD_LUXCONCATTRANSFORM = 449663410U, CMD_LUXTRANSFORM = 2039102042U, CMD_LUXIDENTITY = 97907816U,
            CMD_LUXCOORDINATESYSTEM = 1707244427U, CMD_LUXCOORDSYSTRANSFORM = 2024449520U, CMD_LUXPIXELFILTER = 2384561510U,
            CMD_LUXFILM = 2531294310U, CMD_LUXSAMPLER = 3308802546U, CMD_LUXACCELERATOR = 1613429731U, CMD_LUXSURFACEINTEGRATOR = 4011931910U,
            CMD_LUXVOLUMEINTEGRATOR = 2954204437U, CMD_LUXCAMERA = 3378604391U, CMD_LUXWORLDBEGIN = 1247285547U,
            CMD_LUXATTRIBUTEBEGIN = 684297207U, CMD_LUXATTRIBUTEEND = 3427929065U, CMD_LUXTRANSFORMBEGIN = 567425599U,
            CMD_LUXTRANSFORMEND = 2773125169U, CMD_LUXTEXTURE = 475043887U, CMD_LUXMATERIAL = 4064803661U,
			CMD_LUXMAKENAMEDMATERIAL = 2355625968U, CMD_LUXNAMEDMATERIAL = 922480690U, CMD_LUXLIGHTSOURCE = 130489799U,
            CMD_LUXAREALIGHTSOURCE = 515057184U, CMD_LUXPORTALSHAPE = 3416559329U, CMD_LUXSHAPE = 1943702863U,
            CMD_LUXREVERSEORIENTATION = 2027239206U, CMD_LUXVOLUME = 4138761078U, CMD_LUXOBJECTBEGIN = 1097337658U,
            CMD_LUXOBJECTEND = 229760620U, CMD_LUXOBJECTINSTANCE = 4125664042U, CMD_LUXWORLDEND = 1582674973U, CMD_LUXGETFILM = 859419430U,
            CMD_VOID = 5381U, CMD_SPACE = 177605U;

    std::stringstream ss;
    ss << "Launching server mode on port '" << listenPort << "'.";
    luxError(LUX_NOERROR, LUX_INFO, ss.str().c_str());

    try {
        asio::io_service io_service;
        tcp::endpoint endpoint(tcp::v4(), listenPort);
        tcp::acceptor acceptor(io_service, endpoint);

        for (;;) {
            tcp::iostream stream;
            acceptor.accept(*stream.rdbuf());
            stream.setf(std::ios::scientific, std::ios::floatfield);
            stream.precision(16);

            //reading the command
            std::string command;
            while (std::getline(stream, command)) {
                unsigned int hash = DJBHash(command);

                if ((command != "") && (command != " ")) {
                    ss.str("");
                    ss << "Server processing command: '" << command <<
                            "' (hash: " << hash << ")";
                    luxError(LUX_NOERROR, LUX_INFO, ss.str().c_str());
                }

                //processing the command
                switch (hash) {
                    case CMD_VOID:
                    case CMD_SPACE:
                        break;
                    case CMD_LUXINIT:
                        luxError(LUX_BUG, LUX_SEVERE, "Server already initialized");
                        break;
                    case CMD_LUXTRANSLATE: processCommand(Context::luxTranslate, stream);
                        break;
                    case CMD_LUXROTATE:
                    {
                        float angle, ax, ay, az;
                        stream >> angle;
                        stream >> ax;
                        stream >> ay;
                        stream >> az;
                        //std::cout<<"params :"<<angle<<", "<<ax<<", "<<ay<<", "<<az<<std::endl;
                        luxRotate(angle, ax, ay, az);
                    }
                        break;
                    case CMD_LUXSCALE: processCommand(Context::luxScale, stream);
                        break;
                    case CMD_LUXLOOKAT:
                    {
                        float ex, ey, ez, lx, ly, lz, ux, uy, uz;
                        stream >> ex;
                        stream >> ey;
                        stream >> ez;
                        stream >> lx;
                        stream >> ly;
                        stream >> lz;
                        stream >> ux;
                        stream >> uy;
                        stream >> uz;
                        //std::cout<<"params :"<<ex<<", "<<ey<<", "<<ez<<", "<<lx<<", "<<ly<<", "<<lz<<", "<<ux<<", "<<uy<<", "<<uz<<std::endl;
                        luxLookAt(ex, ey, ez, lx, ly, lz, ux, uy, uz);
                    }
                        break;
                    case CMD_LUXCONCATTRANSFORM: processCommand(Context::luxConcatTransform, stream);
                        break;
                    case CMD_LUXTRANSFORM: processCommand(Context::luxTransform, stream);
                        break;
                    case CMD_LUXIDENTITY: Context::luxIdentity();
                        break;
                    case CMD_LUXCOORDINATESYSTEM: processCommand(Context::luxCoordinateSystem, stream);
                        break;
                    case CMD_LUXCOORDSYSTRANSFORM: processCommand(Context::luxCoordSysTransform, stream);
                        break;
                    case CMD_LUXPIXELFILTER: processCommand(Context::luxPixelFilter, stream);
                        break;
                    case CMD_LUXFILM:
                    {
                        // Dade - Servers use a special kind of film to buffer the
                        // samples. I overwrite some option here.

                        processCommandFilm(Context::luxFilm, stream);
                    }
                        break;
                    case CMD_LUXSAMPLER: processCommand(Context::luxSampler, stream);
                        break;
                    case CMD_LUXACCELERATOR: processCommand(Context::luxAccelerator, stream);
                        break;
                    case CMD_LUXSURFACEINTEGRATOR: processCommand(Context::luxSurfaceIntegrator, stream);
                        break;
                    case CMD_LUXVOLUMEINTEGRATOR: processCommand(Context::luxVolumeIntegrator, stream);
                        break;
                    case CMD_LUXCAMERA: processCommand(Context::luxCamera, stream);
                        break;
                    case CMD_LUXWORLDBEGIN: Context::luxWorldBegin();
                        break;
                    case CMD_LUXATTRIBUTEBEGIN: Context::luxAttributeBegin();
                        break;
                    case CMD_LUXATTRIBUTEEND: Context::luxAttributeEnd();
                        break;
                    case CMD_LUXTRANSFORMBEGIN: Context::luxTransformBegin();
                        break;
                    case CMD_LUXTRANSFORMEND: Context::luxTransformEnd();
                        break;
                    case CMD_LUXTEXTURE:
                    {
                        std::string name, type, texname;
                        ParamSet params;
                        stream >> name;
                        stream >> type;
                        stream >> texname;
                        boost::archive::text_iarchive ia(stream);
                        ia >> params;
                        //std::cout<<"params :"<<name<<", "<<type<<", "<<texname<<", "<<params.ToString()<<std::endl;

                        std::string file = "";
                        file = params.FindOneString(std::string("filename"), file);
                        if (file.size()) {
                            //std::cout<<"receiving file..."<<file<<std::endl;
                            {
                                std::stringstream ss;
                                ss << "Receiving file: '" << file << "'";
                                luxError(LUX_NOERROR, LUX_INFO, ss.str().c_str());
                            }

                            bool first = true;
                            std::string s;
                            std::ofstream out(file.c_str(), std::ios::out | std::ios::binary);
                            while (getline(stream, s) && s != "LUX_END_FILE") {
                                if (!first)out << "\n";
                                first = false;
                                out << s;
                            }
                            out.flush();
                        }

                        Context::luxTexture(name.c_str(), type.c_str(), texname.c_str(), params);
                    }
                        break;
                    case CMD_LUXMATERIAL: processCommand(Context::luxMaterial, stream);
                        break;
                    case CMD_LUXMAKENAMEDMATERIAL: processCommand(Context::luxMakeNamedMaterial, stream);
                        break;
                    case CMD_LUXNAMEDMATERIAL: processCommand(Context::luxNamedMaterial, stream);
                        break;
                    case CMD_LUXLIGHTSOURCE: processCommand(Context::luxLightSource, stream);
                        break;
                    case CMD_LUXAREALIGHTSOURCE: processCommand(Context::luxAreaLightSource, stream);
                        break;
                    case CMD_LUXPORTALSHAPE: processCommand(Context::luxPortalShape, stream);
                        break;
                    case CMD_LUXSHAPE: processCommand(Context::luxShape, stream);
                        break;
                    case CMD_LUXREVERSEORIENTATION: Context::luxReverseOrientation();
                        break;
                    case CMD_LUXVOLUME: processCommand(Context::luxVolume, stream);
                        break;
                    case CMD_LUXOBJECTBEGIN: processCommand(Context::luxObjectBegin, stream);
                        break;
                    case CMD_LUXOBJECTEND: Context::luxObjectEnd();
                        break;
                    case CMD_LUXOBJECTINSTANCE: processCommand(Context::luxObjectInstance, stream);
                        break;
                    case CMD_LUXWORLDEND:
                    {
                        boost::thread t(&luxWorldEnd);


                        //wait the scene parsing to finish
                        while (!luxStatistics("sceneIsReady") && !parseError) {
                            boost::xtime xt;
                            boost::xtime_get(&xt, boost::TIME_UTC);
                            xt.sec += 1;
                            boost::thread::sleep(xt);
                        }

                        boost::thread j(&infoThread);

                        //add rendering threads
                        int threadsToAdd = threads;
                        while (--threadsToAdd)
                            Context::luxAddThread();
                    }
                        break;
                    case CMD_LUXGETFILM:
                    {
                        luxError(LUX_NOERROR, LUX_INFO, "Transmitting film samples");

                        Context::getActive()->getFilm(stream);
                        stream.close();

                        luxError(LUX_NOERROR, LUX_INFO, "Finished film samples transmission");
                    }
                        break;
                    default:
                        ss.str("");
                        ss << "Unknown command '" << command << "'. Ignoring";
                        luxError(LUX_BUG, LUX_SEVERE, ss.str().c_str());
                        break;
                }

                //END OF COMMAND PROCESSING
            }
        }

    } catch (std::exception& e) {
        luxError(LUX_BUG, LUX_ERROR, e.what());
    }
}

int main(int ac, char *av[]) {

    bool useServer = false;
    luxInit();

    try {
        std::stringstream ss;

        // Declare a group of options that will be
        // allowed only on command line
        po::options_description generic("Generic options");
        generic.add_options()
                ("version,v", "print version string") ("help", "produce help message")
                ("server,s", "Launch in server mode")
                ;

        // Declare a group of options that will be
        // allowed both on command line and in
        // config file
        po::options_description config("Configuration");
        config.add_options()
                ("threads,t", po::value < int >(), "Specify the number of threads that Lux will run in parallel.")
                ("useserver,u", po::value< std::vector<std::string> >()->composing(), "Specify the adress of a rendering server to use.")
                ("serverinterval,i", po::value < int >(), "Specify the number of seconds between requests to rendering servers.")
                ;

        // Hidden options, will be allowed both on command line and
        // in config file, but will not be shown to the user.
        po::options_description hidden("Hidden options");
        hidden.add_options()
                ("input-file", po::value< vector<string> >(), "input file") ("test", "debug test mode");

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
        else
            threads = 1;

        if (vm.count("serverinterval"))
            serverInterval = vm["serverinterval"].as<int>();
        else
            serverInterval = 3*60;

        ss.str("");
        ss << "Threads: " << threads;
        luxError(LUX_NOERROR, LUX_INFO, ss.str().c_str());

        if (vm.count("useserver")) {
            std::vector<std::string> names = vm["useserver"].as<std::vector<std::string> >();

            for (std::vector<std::string>::iterator i = names.begin(); i < names.end(); i++) {
                ss.str("");
                ss << "Connecting to server '" << (*i) << "'";
                luxError(LUX_NOERROR, LUX_INFO, ss.str().c_str());

                //TODO jromang : try to connect to the server, and get version number. display message to see if it was successfull		
                luxAddServer((*i).c_str());
            }

            useServer = true;
            
            ss.str("");
            ss << "Server requests interval: " << serverInterval << " secs";
            luxError(LUX_NOERROR, LUX_INFO, ss.str().c_str());
        }

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
                chdir(fullPath.branch_path().string().c_str());

                parseError = false;
                boost::thread t(&engineThread);

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
                boost::thread j(&infoThread);
                if (useServer) boost::thread k(&networkFilmUpdateThread);

                //wait for threads to finish
                t.join();

                // Dade - print the total rendering time
                boost::posix_time::time_duration td(0, 0,
                        (int) luxStatistics("secElapsed"), 0);

                ss.str("");
                ss << "100% rendering done [" << threads << " threads] " << td;
                luxError(LUX_NOERROR, LUX_INFO, ss.str().c_str());
            }
        } else if (vm.count("server")) {
            startServer();
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
