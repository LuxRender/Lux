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

#ifndef LUX_RENDERFARM_H
#define LUX_RENDERFARM_H

#include <vector>
#include <string>
#include <sstream>

#include <boost/thread.hpp>
#include <boost/thread/xtime.hpp>

#include "fleximage.h"

namespace lux
{

class RenderFarm;

class FilmUpdaterThread : public boost::noncopyable {
public:
    FilmUpdaterThread(RenderFarm *renderFarm, Scene *scene) :
        renderFarm(renderFarm), scene(scene), thread(NULL), signal(SIG_NONE) { }

    ~FilmUpdaterThread() {
        delete thread;
    }

    void interrupt() {
        signal = SIG_EXIT;
        thread->join();
    }
    
    friend class RenderFarm;
private:
    static void updateFilm(FilmUpdaterThread *filmUpdaterThread);

    RenderFarm *renderFarm;
    Scene *scene;
    boost::thread *thread; // keep pointer to delete the thread object
    
    // Dade - used to send signals to the thread
    int signal;
    static const int SIG_NONE = 0;
    static const int SIG_EXIT = 1;
};

class RenderFarm {
public:
	RenderFarm() : serverUpdateInterval(3*60), filmUpdateThread(NULL) {}
        ~RenderFarm() {
            if (filmUpdateThread)
                delete filmUpdateThread;
        }

	bool connect(const string &serverName); //!< Connects to a new rendering server
        // Dade - Disconnect from all servers
        void disconnectAll();
	
	void send(const std::string &command);
	void send(const std::string &command, const std::string &name, const ParamSet &params);
	void send(const std::string &command, const std::string &name);
	void send(const std::string &command, float x, float y, float z);
	void send(const std::string &command, float a, float x, float y, float z);
	void send(const std::string &command, float ex, float ey, float ez, float lx, float ly, float lz, float ux, float uy, float uz);
	void send(const std::string &command, float tr[16]);
	void send(const std::string &command, const string &name, const string &type, const string &texname, const ParamSet &params);

    //!< Sends immediately all commands in the buffer to the servers
	void flush();

    int getServerCount() { return serverNameList.size(); }

    // Dade - used to periodically update the film
    void startFilmUpdater(Scene *scene);
    void stopFilmUpdater();
    //!<Gets the films from the network, and merge them to the film given in parameter
	void updateFilm(Scene *scene); 

public:
    // Dade - film update infromation
    int serverUpdateInterval;

private:
	std::vector<std::string> serverNameList;
	std::vector<std::string> serverPortList;
	std::vector<std::string> serverSIDList;
	std::stringstream netBuffer;

    // Dade - film update infromation
    FilmUpdaterThread *filmUpdateThread;
};

}//namespace lux

#endif //LUX_ERROR_H
