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

#include "lux.h"
#include "api.h"
#include "error.h"
#include "paramset.h"
#include "renderfarm.h"
#include "scene.h"
#include "camera.h"

#include "../renderer/include/asio.hpp"

#include <fstream>
#include <boost/iostreams/filtering_stream.hpp>
#include <boost/bind.hpp>

using namespace boost::iostreams;
using namespace lux;
using asio::ip::tcp;

void FilmUpdaterThread::updateFilm(FilmUpdaterThread *filmUpdaterThread) {
    // Dade - thread to update the film with data from servers

    boost::xtime reft;
    boost::xtime_get(&reft, boost::TIME_UTC);

    while (filmUpdaterThread->signal == SIG_NONE) {
        // Dade - check signal every 1 sec

        for(;;) {
            // Dade - sleep for 1 sec
            boost::xtime xt;
            boost::xtime_get(&xt, boost::TIME_UTC);
            xt.sec += 1;
            boost::thread::sleep(xt);

            if (filmUpdaterThread->signal == SIG_EXIT)
                break;
           
            if (xt.sec - reft.sec > filmUpdaterThread->renderFarm->serverUpdateInterval) {
                reft = xt;
                break;
            }
        }

        if (filmUpdaterThread->signal == SIG_EXIT)
            break;

        filmUpdaterThread->renderFarm->updateFilm(filmUpdaterThread->scene);
    }
}

// Dade - used to periodically update the film
void RenderFarm::startFilmUpdater(Scene *scene) {
    filmUpdateThread = new FilmUpdaterThread(this, scene);
    filmUpdateThread->thread = new boost::thread(boost::bind(
            FilmUpdaterThread::updateFilm, filmUpdateThread));    
}

void RenderFarm::stopFilmUpdater() {
    if (filmUpdateThread != NULL) {
        filmUpdateThread->interrupt();
        delete filmUpdateThread;
        filmUpdateThread = NULL;
    }
}

bool RenderFarm::connect(const string &serverName) {
    serverList.push_back(std::string(serverName));

    return true;
}

void RenderFarm::disconnectAll() {
    std::stringstream ss;
    for (vector<string>::iterator server = serverList.begin(); server
            != serverList.end(); ++server) {
        try {
            ss.str("");
            ss << "Disconnect from server: " << (*server);
            luxError(LUX_NOERROR, LUX_INFO, ss.str().c_str());

            tcp::iostream stream((*server).c_str(), "18018");
            stream << "luxExit" << std::endl;
        } catch (std::exception& e) {
            luxError(LUX_SYSTEM, LUX_ERROR, e.what());
        }
    }
}

void RenderFarm::flush() {
    std::stringstream ss;
    // Dade - the buffers with all commands
    string commands = netBuffer.str();

    //flush network buffer
    for (vector<string>::iterator server = serverList.begin(); server
            != serverList.end(); ++server) {
        try {
            ss.str("");
            ss << "Sending commands to server: " << (*server);
            luxError(LUX_NOERROR, LUX_INFO, ss.str().c_str());

            tcp::iostream stream((*server).c_str(), "18018");
            stream << commands << std::endl;
        } catch (std::exception& e) {
            luxError(LUX_SYSTEM, LUX_ERROR, e.what());
        }
    }

    // Dade - write info only if there was the communication with some server
    if (serverList.size() > 0) {
        ss.str("");
        ss << "All servers are aligned";
        luxError(LUX_NOERROR, LUX_INFO, ss.str().c_str());
    }
}

void RenderFarm::updateFilm(Scene *scene) {
    // Dade - network rendering supports only FlexImageFilm
    FlexImageFilm *film = (FlexImageFilm *)(scene->camera->film);

    std::stringstream ss;
    for (vector<string>::iterator server = serverList.begin(); server
            != serverList.end(); ++server) {
        try {
            ss.str("");
            ss << "Getting samples from '" << (*server) << "'";
            luxError(LUX_NOERROR, LUX_INFO, ss.str().c_str());

            tcp::iostream stream((*server).c_str(), "18018");
            stream << "luxGetFilm" << std::endl;

            film->UpdateFilm(scene, stream);

            ss.str("");
            ss << "Samples received from '" << (*server) << "'";
            luxError(LUX_NOERROR, LUX_INFO, ss.str().c_str());
        } catch (std::exception& e) {
            luxError(LUX_SYSTEM, LUX_ERROR, e.what());
        }
    }
}

void RenderFarm::send(const std::string &command) {
    //std::cout<<"sending "<<command<<std::endl;
    //Send command to the render servers
    //for (vector<string>::iterator server = serverList.begin(); server
    //		!= serverList.end(); ++server) {
    //	try
    //	{
    //std::cout<<(*server)<<std::endl;
    //tcp::iostream stream((*server).c_str(), "18018");
    netBuffer << command << std::endl;
    //	}
    //	catch (std::exception& e)
    //	{
    //		luxError(LUX_SYSTEM,LUX_ERROR,e.what());
    //	}
    //}
}

void RenderFarm::send(const std::string &command, const std::string &name,
        const ParamSet &params) {
    //Send command to the render servers
    //for (vector<string>::iterator server = serverList.begin(); server
    //		!= serverList.end(); ++server) {
    try {
        //tcp::iostream stream((*server).c_str(), "18018");
        netBuffer << command << std::endl << name << ' ';
        boost::archive::text_oarchive oa(netBuffer);
        oa << params;
    } catch (std::exception& e) {
        luxError(LUX_SYSTEM, LUX_ERROR, e.what());
    }
    //}
}

void RenderFarm::send(const std::string &command, const std::string &name) {
    //Send command to the render servers
    //for (vector<string>::iterator server = serverList.begin(); server
    //		!= serverList.end(); ++server) {
    try {
        //tcp::iostream stream((*server).c_str(), "18018");
        netBuffer << command << std::endl << name << std::endl;
    } catch (std::exception& e) {
        luxError(LUX_SYSTEM, LUX_ERROR, e.what());
    }
    //}
}

void RenderFarm::send(const std::string &command, float x, float y, float z) {
    //Send command to the render servers
    //for (vector<string>::iterator server = serverList.begin(); server
    //		!= serverList.end(); ++server) {
    try {
        //tcp::iostream stream((*server).c_str(), "18018");
        netBuffer << command << std::endl << x << ' ' << y << ' ' << z << std::endl;
    } catch (std::exception& e) {
        luxError(LUX_SYSTEM, LUX_ERROR, e.what());
    }
    //}
}

void RenderFarm::send(const std::string &command, float a, float x, float y,
        float z) {
    //Send command to the render servers
    //for (vector<string>::iterator server = serverList.begin(); server
    //		!= serverList.end(); ++server) {
    try {
        //tcp::iostream stream((*server).c_str(), "18018");
        netBuffer << command << std::endl << a << ' ' << x << ' ' << y << ' ' << z << std::endl;
    } catch (std::exception& e) {
        luxError(LUX_SYSTEM, LUX_ERROR, e.what());
    }
    //}
}

void RenderFarm::send(const std::string &command, float ex, float ey, float ez,
        float lx, float ly, float lz, float ux, float uy, float uz) {
    //Send command to the render servers
    //for (vector<string>::iterator server = serverList.begin(); server
    //		!= serverList.end(); ++server) {
    try {
        //tcp::iostream stream((*server).c_str(), "18018");
        netBuffer << command << std::endl << ex << ' ' << ey << ' ' << ez << ' ' << lx << ' ' << ly << ' ' << lz << ' ' << ux << ' ' << uy << ' ' << uz << std::endl;
    } catch (std::exception& e) {
        luxError(LUX_SYSTEM, LUX_ERROR, e.what());
    }
    //}
}

void RenderFarm::send(const std::string &command, float tr[16]) {
    //Send command to the render servers
    //for (vector<string>::iterator server = serverList.begin(); server
    //		!= serverList.end(); ++server) {
    try {
        //tcp::iostream stream((*server).c_str(), "18018");
        netBuffer << command << std::endl; //<<x<<' '<<y<<' '<<z<<' ';
        for (int i = 0; i < 16; i++)
            netBuffer << tr[i] << ' ';
        netBuffer << std::endl;
    } catch (std::exception& e) {
        luxError(LUX_SYSTEM, LUX_ERROR, e.what());
    }
    //}
}

void RenderFarm::send(const std::string &command, const string &name,
        const string &type, const string &texname, const ParamSet &params) {
    //Send command to the render servers
    //for (vector<string>::iterator server = serverList.begin(); server
    //		!= serverList.end(); ++server) {
    try {
        //tcp::iostream stream((*server).c_str(), "18018");
        netBuffer << command << std::endl << name << ' ' << type << ' ' << texname << ' ';
        boost::archive::text_oarchive oa(netBuffer);
        oa << params;

        //send the file
        std::string file = "";
        file = params.FindOneString(std::string("filename"), file);
        if (file.size()) {
            std::string s;
            std::ifstream in(file.c_str(), std::ios::out | std::ios::binary);
            while (getline(in, s))
                netBuffer << s << "\n";
            netBuffer << "LUX_END_FILE\n";
        }

    } catch (std::exception& e) {
        luxError(LUX_SYSTEM, LUX_ERROR, e.what());
    }
    //}
}
