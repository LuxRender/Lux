/***************************************************************************
 *   Copyright (C) 1998-2009 by authors (see AUTHORS.txt )                 *
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

#include "osfunc.h"

#include <vector>
#include <string>
#include <sstream>

#include <boost/thread.hpp>
#include <boost/thread/xtime.hpp>
#include <boost/date_time/posix_time/posix_time.hpp>

namespace lux
{

class RenderFarm;

class FilmUpdaterThread : public boost::noncopyable {
public:
    FilmUpdaterThread(RenderFarm *rFarm, Scene *scn) :
        renderFarm(rFarm), scene(scn), thread(NULL), signal(SIG_NONE) { }

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
	RenderFarm() : serverUpdateInterval(3 * 60), filmUpdateThread(NULL),
		netBufferComplete(false), isLittleEndian(osIsLittleEndian()) { }
	~RenderFarm() { delete filmUpdateThread; }

	bool connect(const string &serverName); //!< Connects to a new rendering server
	// Dade - Disconnect from all servers
	void disconnectAll();
	void disconnect(const string &serverName);
	void disconnect(const RenderingServerInfo &serverInfo);

	// signal that rendering is done
	void renderingDone() { netBufferComplete = false; };

	void send(const std::string &command);
	void send(const std::string &command,
		const std::string &name, const ParamSet &params);
	void send(const std::string &command, const std::string &id,
		const std::string &name, const ParamSet &params);
	void send(const std::string &command, const std::string &name);
	void send(const std::string &command, float x, float y);
	void send(const std::string &command, float x, float y, float z);
	void send(const std::string &command,
		float a, float x, float y, float z);
	void send(const std::string &command, float ex, float ey, float ez,
		float lx, float ly, float lz, float ux, float uy, float uz);
	void send(const std::string &command, float tr[16]);
	void send(const std::string &command, const string &name,
		const string &type, const string &texname,
		const ParamSet &params);
	void send(const std::string &command, const std::string &name,
		float a, float b, const std::string &transform);

	//!< Sends immediately all commands in the buffer to the servers
	void flush();

	u_int getServerCount() { return serverInfoList.size(); }
	u_int getServersStatus(RenderingServerInfo *info, u_int maxInfoCount);

	// Dade - used to periodically update the film
	void startFilmUpdater(Scene *scene);
	void stopFilmUpdater();
	//!<Gets the films from the network, and merge them to the film given in parameter
	void updateFilm(Scene *scene);

	//!<Gets the log from the network
	void updateLog();

public:
	// Dade - film update infromation
	int serverUpdateInterval;

private:
	struct ExtRenderingServerInfo {
		ExtRenderingServerInfo(string n, string p, string id) :
			timeLastContact(boost::posix_time::second_clock::local_time()),
			numberOfSamplesReceived(0.),
			name(n), port(p), sid(id), active(false), flushed(false) { }

		boost::posix_time::ptime timeLastContact;
		// to return the max. number of samples among
		// all buffer groups in the film
		double numberOfSamplesReceived;

		string name;
		string port;
		string sid;

		bool active;

		bool flushed;
	};

	static void decodeServerName(const string &serverName, string &name, string &port);
	bool connect(ExtRenderingServerInfo &serverInfo);
	void flushImpl();
	void disconnect(const ExtRenderingServerInfo &serverInfo);
	void sendParams(const ParamSet &params);
	void sendFile(const std::string &file);
	void reconnectFailed();

	// Any operation on servers must be synchronized via this mutex
	boost::mutex serverListMutex;
	std::vector<ExtRenderingServerInfo> serverInfoList;

	// Dade - film update information
	FilmUpdaterThread *filmUpdateThread;

	std::stringstream netBuffer;
	bool netBufferComplete; // Raise this flag if the scene is complete
	bool isLittleEndian;
};

}//namespace lux

#endif //LUX_RENDERFARM_H
