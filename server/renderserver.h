/***************************************************************************
 *   Copyright (C) 1998-2009 by authors (see AUTHORS.txt)                  *
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

#ifndef RENDER_SERVER_H
#define RENDER_SERVER_H

#include "lux.h"

#include <fstream>
#include <boost/thread/xtime.hpp>
#include <boost/date_time/posix_time/posix_time.hpp>
#include <boost/thread.hpp>
#include <boost/bind.hpp>
#include <boost/archive/text_oarchive.hpp>
#include <boost/archive/text_iarchive.hpp>
#include <boost/iostreams/filtering_stream.hpp>

namespace lux
{

class RenderServer;

class NetworkRenderServerThread : public boost::noncopyable {
public:
    NetworkRenderServerThread(RenderServer *server) :
        renderServer(server), serverThread(NULL), engineThread(NULL),
        infoThread(NULL), signal(SIG_NONE) { }

    ~NetworkRenderServerThread() {
        if (engineThread)
            delete engineThread;

        if (infoThread)
            delete infoThread;

        if (serverThread)
            delete serverThread;
    }

    void interrupt() {
        signal = SIG_EXIT;
    }

    void join() {
        serverThread->join();
    }

    static void run(NetworkRenderServerThread *serverThread);
    friend class RenderServer;

    RenderServer *renderServer;
    boost::thread *serverThread;
    boost::thread *engineThread;
    boost::thread *infoThread;

    // Dade - used to send signals to the thread
    enum ThreadSignal { SIG_NONE, SIG_EXIT };
    ThreadSignal signal;
};

// Dade - network rendering server
class RenderServer {
public:
    static const int DEFAULT_TCP_PORT = 18018;

    enum ServerState { UNSTARTED, READY, BUSY, STOPPED };

    RenderServer(int threadCount, int tcpPort = DEFAULT_TCP_PORT, bool writeFlmFile = false);
    ~RenderServer();

    void start();
    void join();
    void stop();

    int getServerPort() const { return tcpPort; }
    ServerState getServerState() const { return  state; }
	void setServerState(ServerState newState) {
		state = newState;
	}

	string getCurrentSID() const {
		return currentSID;
	}

	bool getWriteFlmFile() const {
		return writeFlmFile;
	}

	int getTcpPort() const {
		return tcpPort;
	}

	int getThreadCount() const {
		return threadCount;
	}

	void createNewSessionID();

	bool validateAccess(std::basic_istream<char> &stream) const;

    friend class NetworkRenderServerThread;

private:
    int threadCount;
    int tcpPort;
	bool writeFlmFile;
    ServerState state;
	string currentSID;
    NetworkRenderServerThread *serverThread;
};

}//namespace lux

#endif // RENDER_SERVER_H
