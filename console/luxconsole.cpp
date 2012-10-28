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

#include <exception>
#include <iostream>
#include <string>
#include <vector>

#include "api.h"
#include "error.h"
#include "server/renderserver.h"
#include "commandline.h"

#include <boost/date_time/posix_time/posix_time.hpp>
#include <boost/filesystem.hpp>

#if defined(WIN32) && !defined(__CYGWIN__) /* We need the following two to set stdout to binary */
#include <io.h>
#include <fcntl.h>
#endif

using namespace lux;

std::string sceneFileName;
bool parseError;
RenderServer *renderServer;

void engineThread() {
	// NOTE - lordcrc - initialize rand()
	srand(time(NULL));

    luxParse(sceneFileName.c_str());
    if (luxStatistics("sceneIsReady") == 0.)
        parseError = true;
}

void infoThread() {
	std::vector<char> buf(1 << 16, '\0');
	while (!boost::this_thread::interruption_requested()) {
		try {
			boost::this_thread::sleep(boost::posix_time::seconds(5));

			luxUpdateStatisticsWindow();
			luxGetStringAttribute("renderer_statistics_formatted_short", "_recommended_string", &buf[0], static_cast<unsigned int>(buf.size()));
			LOG(LUX_INFO,LUX_NOERROR) << std::string(buf.begin(), buf.end());
		} catch(boost::thread_interrupted&) {
			break;
		}
	}
}

void addNetworkSlavesThread(std::vector<std::string> slaves) {
	for (std::vector<std::string>::iterator it = slaves.begin(); it < slaves.end(); it++) {
		try {
			if (boost::this_thread::interruption_requested())
				break;
			luxAddServer((*it).c_str());
		} catch(boost::thread_interrupted&) {
			break;
		}
	}
}

LuxErrorHandler prevErrorHandler;

void serverErrorHandler(int code, int severity, const char *msg) {
	if (renderServer)
		renderServer->errorHandler(code, severity, msg);

	prevErrorHandler(code, severity, msg);
}

int main(int argc, char **argv) {
	// Dade - initialize rand() number generator
	srand(time(NULL));

	luxInit();

	clConfig config;
	if (!ProcessCommandLine(argc, argv, config, featureSet::RENDERER | featureSet::MASTERNODE | featureSet::SLAVENODE))
		return 1;

	if (!config.slave) {
		// build queue
		std::vector<std::string> queue(config.inputFiles);
		if (!config.queueFile.empty()) {
			std::string sceneFile;
			std::ifstream queueFile(config.queueFile.c_str());
			while (std::getline (queueFile, sceneFile))
				queue.push_back(sceneFile);
		}

		// sanitize filenames in queue
		{
			std::vector<std::string> queueSanitized;
			for (std::vector<std::string>::iterator it = queue.begin(); it < queue.end(); it++) {
				if (*it != "-") {
					boost::filesystem::path sceneFileComplete(boost::filesystem::system_complete(*it));
					if (!sceneFileComplete.empty() && boost::filesystem::exists(sceneFileComplete))
						queueSanitized.push_back(sceneFileComplete.string());
					else
						LOG(LUX_ERROR,LUX_NOFILE) << "Could not find scene file '" << *it << "'";
				} else
					queueSanitized.push_back("-");
			}
			queue.swap(queueSanitized);
		}

		// process queue
		for (std::vector<std::string>::iterator it = queue.begin(); it < queue.end(); it++) {
			if (config.fixedSeed)
				luxDisableRandomMode();

			sceneFileName = *it;
			if (sceneFileName != "-") {
				LOG(LUX_INFO,LUX_NOERROR) << "Loading scene file: '" << sceneFileName << "'...";
				boost::filesystem::path workingDirectory = boost::filesystem::path(sceneFileName).parent_path();
				try {
					boost::filesystem::current_path(workingDirectory);
				} catch (boost::filesystem::filesystem_error &) {
					LOG(LUX_SEVERE,LUX_NOFILE) << "Unable to change to directory '" << workingDirectory.string() << "'";
					continue;
				}
			} else
				LOG(LUX_INFO,LUX_NOERROR) << "Loading piped scene...";

			parseError = false;
			boost::thread engine(&engineThread);

			// add slaves, need to do this for each scene file
			boost::thread addSlaves(boost::bind(addNetworkSlavesThread, config.slaveNodeList));

			// wait the scene parsing to finish
			while (!luxStatistics("sceneIsReady") && !parseError)
				boost::this_thread::sleep(boost::posix_time::seconds(1));

			if (parseError) {
				LOG(LUX_SEVERE,LUX_BADFILE) << "Skipping invalid scenefile '" << sceneFileName << "'";
				continue;
			}

			// add rendering threads
			int threadsToAdd = config.threadCount;
			while (--threadsToAdd)
				luxAddThread();

			// launch info printing thread
			boost::thread info(&infoThread);

			// Dade - wait for the end of the rendering
			luxWait();

			// We have to stop the info thread before to call luxExit()/luxCleanup()
			info.interrupt();
			// Stop adding slaves before proceeding
			addSlaves.interrupt();

			info.join();
			addSlaves.join();

			luxExit();

			// Dade - print the total rendering time
			boost::posix_time::time_duration td(0, 0, (int)luxStatistics("secElapsed"), 0);
			LOG(LUX_INFO,LUX_NOERROR) << "100% rendering done [" << config.threadCount << " threads] " << td;

			if (config.binDump) {
				// Get pointer to framebuffer data if needed
				unsigned char* fb = luxFramebuffer();

				int w = luxGetIntAttribute("film", "xPixelCount");
				int h = luxGetIntAttribute("film", "yPixelCount");
				luxUpdateFramebuffer();

#if defined(WIN32) && !defined(__CYGWIN__) /* On WIN32 we need to set stdout to binary */
				_setmode(_fileno(stdout), _O_BINARY);
#endif

				// Dump RGB imagebuffer data to stdout
				for (int i = 0; i < w * h * 3; i++)
					std::cout << fb[i];
			}

			luxCleanup();
		}
	} else {
		renderServer = new RenderServer(config.threadCount, config.password, config.tcpPort, config.writeFlmFile);

		prevErrorHandler = luxError;
		luxErrorHandler(serverErrorHandler);

		renderServer->start();

		// add slaves, no need to do this in a separate thread since we're just waiting for
		// the server to finish
		for (std::vector<std::string>::iterator it = config.slaveNodeList.begin(); it < config.slaveNodeList.end(); it++)
			luxAddServer((*it).c_str());

		renderServer->join();
		delete renderServer;
	}
	return 0;
}
