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

// This include must come before lux.h
#include <boost/serialization/vector.hpp>

#include "lux.h"
#include "scene.h"
#include "api.h"
#include "error.h"
#include "version.h"
#include "paramset.h"
#include "renderfarm.h"
#include "camera.h"
#include "color.h"
#include "film.h"
#include "osfunc.h"
#include "streamio.h"
#include "filedata.h"
#include "tigerhash.h"

#include <algorithm>
#include <fstream>
#include <sstream>
#include <exception> 
#include <boost/asio.hpp>
#include <boost/archive/text_oarchive.hpp>
#include <boost/bind.hpp>
#include <boost/iostreams/device/file.hpp>
#include <boost/iostreams/stream.hpp>
#include <boost/iostreams/filtering_stream.hpp>
#include <boost/iostreams/filtering_streambuf.hpp>
#include <boost/iostreams/copy.hpp>
#include <boost/iostreams/filter/zlib.hpp>
#include <boost/iostreams/filter/bzip2.hpp>
#include <boost/iostreams/filter/gzip.hpp>
#include <boost/iostreams/positioning.hpp>
#include <boost/cstdint.hpp>
#include <boost/algorithm/string.hpp>
#include <boost/thread/xtime.hpp>
#include <boost/date_time/posix_time/posix_time.hpp>

using namespace boost::iostreams;
using namespace boost::posix_time;
using namespace lux;
using namespace std;
using boost::asio::ip::tcp;

static bool read_response(std::iostream &stream, const std::string &expected_response) {
	
	// flush any output
	stream << std::flush;

	std::string response;
	if (!getline(stream, response)) {
		LOG( LUX_ERROR,LUX_SYSTEM) << "Error reading from slave";
		return false;
	}

	if (response != expected_response) {
		LOG( LUX_ERROR,LUX_SYSTEM) << "Wrong response from slave, expected '" << expected_response << "', got '" << response << "'";
		return false;
	}

	return true;
}

static std::string get_response(std::iostream &stream) {
	// flush any output
	stream << std::flush;

	std::string response;
	if (!getline(stream, response)) {
		LOG( LUX_ERROR,LUX_SYSTEM) << "Error reading from slave";
		return "";
	}

	return response;
}

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


bool RenderFarm::ExtRenderingServerInfo::sameServer(const std::string &name, const std::string &port) const {
	return boost::iequals(this->name, name) && boost::equals(this->port, port);
}

bool RenderFarm::ExtRenderingServerInfo::sameServer(const RenderFarm::ExtRenderingServerInfo &other) const {
	return sameServer(other.name, other.port);
}

RenderFarm::CompiledFile::CompiledFile(const std::string &filename) : fname(filename) {
	fhash = digest_string(file_hash<tigerhash>(filename));
}

bool RenderFarm::CompiledFile::send(std::iostream &stream) const {
	LOG(LUX_DEBUG,LUX_NOERROR) << "Sending file '" << filename() << "'";

	std::ifstream in(filename().c_str(), std::ios::in | std::ios::binary);

	// Get length of file
	in.seekg(0, std::ifstream::end);
	boost::uint64_t len = boost::iostreams::position_to_offset(in.tellg());
	in.seekg(0, std::ifstream::beg);

	if (in.fail()) {
		// AdjustFilename should guarantee that file exists, if not return just normalized filename
		LOG( LUX_ERROR,LUX_SYSTEM) << "There was an error while checking the size of file '" << filename() << "'";

		// Send an empty file ot the server
		stream << "\n0\n";
	} else {
		// Send the file length
		stream << filename() << "\n";
		stream << len << "\n";

		// Send the file
		vector<char> buffer(1 * 1024 * 1024, 0);
		while (len > 0) {
			const std::streamsize rs = static_cast<std::streamsize>(min(static_cast<boost::uint64_t>(buffer.size()), len));
			in.read(&buffer[0], rs);
			stream.write(&buffer[0], rs);
			len -= rs;
		}

		if (in.bad()) {
			LOG( LUX_ERROR,LUX_SYSTEM) << "There was an error sending file '" << filename() << "'";
			return false;
		}
	}

	in.close();

	return true;
}

RenderFarm::CompiledFile RenderFarm::CompiledFiles::add(const std::string &filename) {
	// 
	if (nameIndex.find(filename) != nameIndex.end())
		return files[nameIndex[filename]];

	CompiledFile cf(filename);

	size_t index = files.size();
	files.push_back(cf);

	nameIndex[cf.filename()] = index;
	hashIndex[cf.hash()] = index;

	return cf;
}

const RenderFarm::CompiledFile& RenderFarm::CompiledFiles::fromFilename(std::string filename) const {
	name_index_t::const_iterator it = nameIndex.find(filename);
	if (it == nameIndex.end())
		throw std::range_error("Invalid filename lookup in CompiledFiles: '" + filename + "'");
	return files[it->second];
}

const RenderFarm::CompiledFile& RenderFarm::CompiledFiles::fromHash(filehash_t hash) const {
	hash_index_t::const_iterator it = hashIndex.find(hash);
	if (it == hashIndex.end())
		throw std::range_error("Invalid file hash lookup in CompiledFiles: '" + hash + "'");
	return files[it->second];
}

bool RenderFarm::CompiledFiles::send(std::iostream &stream) const {
	LOG(LUX_DEBUG,LUX_NOERROR) << "Sending files";

	stream << "BEGIN FILES" << "\n";

	if (!read_response(stream, "BEGIN FILES OK"))
		return false;

	std::string hash;
	while (true) {
		hash = get_response(stream);

		if (hash == "")
			return false;

		if (hash == "END FILES")
			break;

		LOG(LUX_DEBUG,LUX_NOERROR) << "File hash request: '" << hash << "'";

		// TODO - catch exception in case of invalid hash
		const CompiledFile &cf(fromHash(hash));

		if (!cf.send(stream))
			return false;

		std::string response = get_response(stream);
		if (response == "FILE OK")
			continue;

		if (response != "RESEND FILE") {
			LOG( LUX_ERROR,LUX_SYSTEM) << "Invalid response '" << response << "', expected 'RESEND FILE'";
			return false;
		}

		// resend file once
		if (!cf.send(stream))
			return false;
	}

	stream << "END FILES OK" << "\n";

	LOG(LUX_DEBUG,LUX_NOERROR) << "Sent files";

	return true;
}

RenderFarm::CompiledCommand::CompiledCommand(const std::string &cmd) 
	: command(cmd), hasParams(false), paramsBuf(std::stringstream::in | std::stringstream::out  | std::stringstream::binary) 
{
	// set precision for accurate transmission of floats
	paramsBuf << std::scientific << std::setprecision(16);
}

RenderFarm::CompiledCommand::CompiledCommand(const RenderFarm::CompiledCommand &other) 
	: command(other.command), hasParams(other.hasParams), paramsBuf(std::stringstream::in | std::stringstream::out  | std::stringstream::binary), files(other.files)
{
	// set precision for accurate transmission of floats
	paramsBuf << std::scientific << std::setprecision(16) << other.paramsBuf.str();
}

RenderFarm::CompiledCommand& RenderFarm::CompiledCommand::operator=(const RenderFarm::CompiledCommand &other) {
	if (this == &other)
		return *this;

	command = other.command;
	hasParams = other.hasParams;
	paramsBuf.str(other.paramsBuf.str());
	files.clear();
	files.assign(other.files.begin(), other.files.end());

	return *this;
}

std::ostream& RenderFarm::CompiledCommand::buffer() {
	return paramsBuf;
}

void RenderFarm::CompiledCommand::addParams(const ParamSet &params) {
	// Serialize the parameters
	stringstream zos(stringstream::in | stringstream::out | stringstream::binary);
	std::streamsize size;
	{
		stringstream os(stringstream::in | stringstream::out | stringstream::binary);
		{
			// Serialize the parameters
			boost::archive::text_oarchive oa(os);
			oa << params;
		}

		// Compress the parameters
		filtering_streambuf<input> in;
		in.push(gzip_compressor(9));
		in.push(os);
		size = boost::iostreams::copy(in , zos);
	}

	// Write the size of the compressed chunk
	osWriteLittleEndianUInt(osIsLittleEndian(), paramsBuf, size);
	// Copy the compressed parameters to the newtwork buffer
	paramsBuf << zos.str() << "\n";
	hasParams = true;
}

void RenderFarm::CompiledCommand::addFile(const std::string &paramName, const CompiledFile &cf) {
	files.push_back(std::make_pair(paramName, cf));
}

bool RenderFarm::CompiledCommand::send(std::iostream &stream) const {
	stream << command << "\n";
	string buf = paramsBuf.str();
	stream << buf;
	string response;

	// no params means no files
	if (!hasParams)
		return true;

	if (files.empty()) {
		stream << "FILE INDEX EMPTY" << "\n";
		return true;
	}

	LOG(LUX_DEBUG,LUX_NOERROR) << "Sending file index";
	stream << "BEGIN FILE INDEX" << "\n";

	if (!read_response(stream, "BEGIN FILE INDEX OK"))
		return false;

	LOG(LUX_DEBUG,LUX_NOERROR) << "File index size: " << files.size();
	for (size_t i = 0; i < files.size(); ++i) {		
		stream << files[i].first << "\n"; // param name
		stream << files[i].second.filename() << "\n";
		stream << files[i].second.hash() << "\n";
		stream << "\n";
	}

	stream << "END FILE INDEX" << "\n";

	if (!read_response(stream, "END FILE INDEX OK"))
		return false;

	LOG(LUX_DEBUG,LUX_NOERROR) << "File index sent ok";

	return true;
}

RenderFarm::CompiledCommand& RenderFarm::CompiledCommands::add(const std::string command) {
	commands.push_back(CompiledCommand(command));
	return commands.back();
}

RenderFarm::RenderFarm() : serverUpdateInterval(3 * 60), filmUpdateThread(NULL), flushThread(NULL),
		netBufferComplete(false), isLittleEndian(osIsLittleEndian())
{
}


// Dade - used to periodically update the film
void RenderFarm::start(Scene *scene) {
	boost::mutex::scoped_lock lock(serverListMutex);

	if (serverInfoList.empty() || !scene)
		return;

	// already started
	if (filmUpdateThread)
		return;

	filmUpdateThread = new FilmUpdaterThread(this, scene);
	filmUpdateThread->thread = new boost::thread(boost::bind(
		FilmUpdaterThread::updateFilm, filmUpdateThread));
}

void RenderFarm::stop() {
	boost::mutex::scoped_lock lock(serverListMutex);

	if (serverInfoList.empty())
		return;

	if (filmUpdateThread) {
		filmUpdateThread->interrupt();
		delete filmUpdateThread;
		filmUpdateThread = NULL;
	}

	if (flushThread) {
		flushThread->interrupt();
		flushThread->join();
		delete flushThread;
		flushThread = NULL;
	}

	// Dade - stopFilmUpdater() is called multiple times at the moment (for instance
	// haltspp + luxconsole)
	/*else {
		LOG(LUX_ERROR,LUX_ILLSTATE)<<"RenderFarm::stopFilmUpdater() called but update thread not started.";
	}*/
}

bool RenderFarm::decodeServerName(const string &serverName, string &name, string &port) {
	// Dade - check if the server name includes the port
	size_t idx = serverName.find_last_of(':');
	size_t idx_v6 = serverName.rfind("::");
	if (idx != string::npos && idx != idx_v6+1) {
		// Dade - the server name includes the port number

		try {
			name = serverName.substr(0, idx);
			// convert to int and back to get a standardized representation for comparison
			port = boost::lexical_cast<std::string>(boost::lexical_cast<int>(serverName.substr(idx + 1)));
		} catch (boost::bad_lexical_cast &) {
			LOG(LUX_ERROR, LUX_SYSTEM) << "Unable to decode server name: '" << serverName << "'";
			return false;
		}
	} else {
		name = serverName;
		port = "18018";
	}

	return true;
}

bool RenderFarm::connect(ExtRenderingServerInfo &serverInfo) {

	// check to see if we're already connected (active), if so ignore
	for (vector<ExtRenderingServerInfo>::iterator it = serverInfoList.begin(); it < serverInfoList.end(); it++ ) {
		if (serverInfo.sameServer(*it)) {
			return false;
		}
	}

	// initialize server info
	serverInfo.sid = "";
	serverInfo.active = false;
	serverInfo.flushed = false;

	stringstream ss;
	string serverName = serverInfo.name + ":" + serverInfo.port;

	try {
		LOG( LUX_INFO,LUX_NOERROR) << "Connecting server: " << serverName;

		tcp::iostream stream(serverInfo.name, serverInfo.port);
		stream << "ServerConnect" << std::endl;

		// Check if the server accepted the connection
		string result;
		if (!getline(stream, result)) {
			LOG( LUX_ERROR,LUX_SYSTEM) << "Unable to connect server: " << serverName;
			return false;
		}

		LOG( LUX_INFO,LUX_NOERROR) << "Server connect result: " << result;
		
		if ("OK" != result) {
			LOG( LUX_ERROR,LUX_SYSTEM) << "Unable to connect server: " << serverName;

			return false;
		}

		// Read the server version
		if (!getline(stream, result)) {
			LOG( LUX_ERROR,LUX_SYSTEM) << "Unable read version from server: " << serverName;
			return false;
		}
		// search for protocol field in version string
		// if not found we're dealing with a pre 0.8 server
		if (result.find("protocol") == string::npos) {
			LOG( LUX_ERROR,LUX_SYSTEM) << "Server returned invalid version string, this is most likely due to an old server executable, got '" << result << "', expected '" << LUX_SERVER_VERSION_STRING << "'";
			return false;
		}
		if (result != LUX_SERVER_VERSION_STRING) {
			LOG( LUX_ERROR,LUX_SYSTEM) << "Version mismatch, server reports version '" << result << "', required version is '" << LUX_SERVER_VERSION_STRING << "'";
			return false;
		}
		// Read the session ID
		if (!getline(stream, result)) {
			LOG( LUX_ERROR,LUX_SYSTEM) << "Unable read session ID from server: " << serverName;
			return false;
		}

		string sid = result;

		// Perform handshake
		stream << sid << endl;
		stream.flush();

		if (!getline(stream, result)) {
			LOG( LUX_ERROR,LUX_SYSTEM) << "Unable read handshake from server: " << serverName;
			return false;
		}

		if (!stream.good() || result != "CONNECTED") {
			LOG( LUX_ERROR,LUX_SYSTEM) << "Error connecting to server: " << serverName;
			return false;
		}

		LOG( LUX_INFO,LUX_NOERROR) << "Server session ID: " << sid;

		serverInfo.sid = sid;
		serverInfo.active = true;
	} catch (exception& e) {
		LOG(LUX_ERROR,LUX_SYSTEM) << "Unable to connect server: " << serverName;
		LOG(LUX_ERROR,LUX_SYSTEM)<< e.what();
		return false;
	}

	return true;
}

RenderFarm::reconnect_status_t RenderFarm::reconnect(ExtRenderingServerInfo &serverInfo)
{
	stringstream ss;
	string serverName = serverInfo.name + ":" + serverInfo.port;

	try {
		LOG( LUX_INFO,LUX_NOERROR) << "Reconnecting to server: " << serverName;

		tcp::iostream stream(serverInfo.name, serverInfo.port);
		stream << "ServerReconnect" << std::endl;
		stream << serverInfo.sid << std::endl;

		// Check if the server accepted the connection
		string result;
		if (!getline(stream, result)) {
			LOG( LUX_ERROR,LUX_SYSTEM) << "Unable to reconnect server: " << serverName;
			serverInfo.calculatedSamplesPerSecond = 0;
			return reconnect_status::error;
		}

		LOG( LUX_INFO,LUX_NOERROR) << "Server reconnect result: " << result;
		
		if ("CONNECTED" != result) {
			// slave rejected reconnect attempt, signal by setting active to false
			serverInfo.active = false;
			serverInfo.calculatedSamplesPerSecond = 0;
			return reconnect_status::rejected;
		}

		serverInfo.active = true;
		serverInfo.flushed = true;
	} catch (exception& e) {
		LOG(LUX_ERROR,LUX_SYSTEM) << "Unable to reconnect server: " << serverName;
		LOG(LUX_ERROR,LUX_SYSTEM)<< e.what();
		serverInfo.calculatedSamplesPerSecond = 0;
		return reconnect_status::error;
	}

	serverInfo.timeLastSamples = boost::posix_time::second_clock::local_time();
	return reconnect_status::success;
}

bool RenderFarm::connect(const string &serverName) {
	{
		boost::mutex::scoped_lock lock(serverListMutex);

		stringstream ss;
		try {
			string name, port;
			if (!decodeServerName(serverName, name, port))
				return false;			

			ExtRenderingServerInfo serverInfo(name, port);
			if (!connect(serverInfo)) {
				if (serverInfo.active)
					disconnect(serverInfo);

				return false;
			}

			serverInfoList.push_back(serverInfo);
		} catch (exception& e) {
			LOG(LUX_ERROR,LUX_SYSTEM) << "Unable to connect server: " << serverName;
			LOG(LUX_ERROR,LUX_SYSTEM)<< e.what();
			return false;
		}
	}

	if (netBufferComplete)
		flush();  // Only flush if scene is complete

	return true;
}

void RenderFarm::disconnectAll() {
	boost::mutex::scoped_lock lock(serverListMutex);

	for (size_t i = 0; i < serverInfoList.size(); i++)
		disconnect(serverInfoList[i]);
	serverInfoList.clear();
}

void RenderFarm::disconnect(const string &serverName) {
	boost::mutex::scoped_lock lock(serverListMutex);

	string name, port;
	if (!decodeServerName(serverName, name, port))
		return;

	for (vector<ExtRenderingServerInfo>::iterator it = serverInfoList.begin(); it < serverInfoList.end(); it++ ) {
		if (it->sameServer(name, port)) {
			disconnect(*it);
			serverInfoList.erase(it);
			break;
		}
	}
}

void RenderFarm::disconnect(const RenderingServerInfo &serverInfo) {
	stringstream ss;
	try {
		LOG( LUX_INFO,LUX_NOERROR)
			<< "Disconnect from server: "
			<< serverInfo.name << ":" << serverInfo.port;

		tcp::iostream stream(serverInfo.name, serverInfo.port);
		stream << "ServerDisconnect" << endl;
		stream << serverInfo.sid << endl;
	} catch (exception& e) {
		LOG(LUX_ERROR,LUX_SYSTEM)<< e.what();
	}
}

void RenderFarm::disconnect(const ExtRenderingServerInfo &serverInfo) {
	stringstream ss;
	try {
		LOG( LUX_INFO,LUX_NOERROR)
			<< "Disconnect from server: "
			<< serverInfo.name << ":" << serverInfo.port;

		tcp::iostream stream(serverInfo.name, serverInfo.port);
		stream << "ServerDisconnect" << endl;
		stream << serverInfo.sid << endl;
	} catch (exception& e) {
		LOG(LUX_ERROR,LUX_SYSTEM)<< e.what();
	}
}

bool RenderFarm::sessionReset(const string &serverName, const string &password) {
	boost::mutex::scoped_lock lock(serverListMutex);

	string name, port;
	if (!decodeServerName(serverName, name, port))
		return false;

	string formattedServerName = name + ":" + port;

	LOG( LUX_INFO,LUX_NOERROR) << "Resetting server: " << formattedServerName;

	// check to see if we're already connected, if so try to reconnect, if failed reset
	for (vector<ExtRenderingServerInfo>::iterator it = serverInfoList.begin(); it < serverInfoList.end(); it++ ) {
		if (it->sameServer(name, port)) {			
			LOG( LUX_DEBUG,LUX_NOERROR) << "Attempting to recover existing session with server: " << formattedServerName;
			if (reconnect(*it) == reconnect_status::success) {
				LOG( LUX_INFO,LUX_NOERROR) << "Server reconnected successfully, aborting reset of server: " << formattedServerName;
				return true;
			}
			serverInfoList.erase(it);
			break;
		}
	}

	try {
		tcp::iostream stream(name, port);
		stream << "ServerReset" << std::endl << std::flush;

		// Check if the server accepted the connection
		string result;
		if (!getline(stream, result)) {
			LOG( LUX_ERROR,LUX_SYSTEM) << "Error resetting server: " << formattedServerName;
			return false;
		}

		LOG( LUX_DEBUG,LUX_NOERROR) << "Server reset response: " << result;
		
		if ("IDLE" == result) {
			// slave is idle, nothing to reset
			return true;
		}

		if ("CHALLENGE" != result) {
			// unknown response
			LOG( LUX_ERROR,LUX_SYSTEM) << "Unable to reset server: " << formattedServerName << " (response: '" << result << "')";
			return false;
		}

		string salt;
		if (!getline(stream, salt)) {
			LOG( LUX_ERROR,LUX_SYSTEM) << "Error resetting server: " << formattedServerName;
			return false;
		}

		const string hashedpass = digest_string(string_hash<tigerhash>(
				salt + password + salt));

		stream << hashedpass << std::endl << std::flush;

		if (!getline(stream, result)) {
			LOG( LUX_ERROR,LUX_SYSTEM) << "Error resetting server: " << formattedServerName;
			return false;
		}

		if ("DENIED" == result) {
			LOG( LUX_ERROR,LUX_SYSTEM) << "Authentication failed trying to reset server: " << formattedServerName;
			return false;
		}

		if ("RESET" != result) {
			LOG( LUX_ERROR,LUX_SYSTEM) << "Unable to reset server: " << formattedServerName << " (response: '" << result << "')";
			return false;
		}
		
		LOG( LUX_INFO,LUX_NOERROR) << "Server reset: "  << formattedServerName;

	} catch (exception& e) {
		LOG(LUX_ERROR,LUX_SYSTEM) << "Unable to reset server: " << formattedServerName;
		LOG(LUX_ERROR,LUX_SYSTEM)<< e.what();
		return false;
	}

	return true;
}
void RenderFarm::flushImpl() {
	//flush network buffer
	for (size_t i = 0; i < serverInfoList.size(); i++) {
		if(serverInfoList[i].active && !serverInfoList[i].flushed) {
			try {
				LOG( LUX_INFO,LUX_NOERROR) << "Sending commands to server: " <<
						serverInfoList[i].name << ":" << serverInfoList[i].port;

				tcp::iostream stream(serverInfoList[i].name, serverInfoList[i].port);
				stream.rdbuf()->set_option(tcp::no_delay(true));
				//stream << commands << endl;
				for (size_t j = 0; j < compiledCommands.size(); j++) {
					// send command
					if (!compiledCommands[j].send(stream))
						break;

					// and then send any requested files
					if (!compiledCommands[j].sendFiles())
						continue;

					if (!compiledFiles.send(stream))
						break;
				}

				serverInfoList[i].flushed = true;
			} catch (exception& e) {
				LOG(LUX_ERROR,LUX_SYSTEM)<< e.what();
			}
		}
	}

	// Dade - write info only if there was the communication with some server
	if (serverInfoList.size() > 0) {
		LOG( LUX_DEBUG,LUX_NOERROR) << "All servers are aligned";
	}
}

void RenderFarm::flush() {
	boost::mutex::scoped_lock lock(serverListMutex);

	flushImpl();
}

void RenderFarm::reconnectFailed() {
	// NOTE - requires serverListMutex to be aquired by caller
	// attempts to reconnect to all failed (inactive) servers

	for (size_t i = 0; i < serverInfoList.size(); i++) {
		if (serverInfoList[i].active)
			continue;

		ExtRenderingServerInfo &serverInfo(serverInfoList[i]);
		try {
			LOG(LUX_INFO,LUX_NOERROR)
				<< "Trying to reconnect server: "
				<< serverInfo.name << ":" << serverInfo.port;

			// If reconnect() returns rejected, the slave actively
			// rejected the reconnection attempt, in which case we 
			// try to establish a new session
			if (this->reconnect(serverInfo) == reconnect_status::rejected) {
				LOG(LUX_INFO,LUX_NOERROR)
					<< "Reconnection failed, attemting to establish new session with server: "
					<< serverInfo.name << ":" << serverInfo.port;
				this->connect(serverInfo);
			}
		} catch (std::exception& e) {
			LOG(LUX_ERROR,LUX_SYSTEM)
				<< "Error while reconnecting with server: "
				<< serverInfo.name << ":" << serverInfo.port;
			LOG(LUX_ERROR,LUX_SYSTEM)<< e.what();
		}
	}

	// Send the scene to all servers
	flushImpl();
}

void RenderFarm::updateFilm(Scene *scene) {
	// Using the mutex in order to not allow server disconnection while
	// I'm downloading a film
	boost::mutex::scoped_lock lock(serverListMutex);

	// Dade - network rendering supports only FlexImageFilm
	Film *film = scene->camera->film;

	// first try to reconnect to failed servers which may be up now
	reconnectFailed();

	stringstream ss;
	for (size_t i = 0; i < serverInfoList.size(); i++) {
		if (!serverInfoList[i].active)
			// skip servers which are still down
			continue;

		try {
			LOG( LUX_INFO,LUX_NOERROR) << "Getting samples from: " <<
					serverInfoList[i].name << ":" << serverInfoList[i].port;

			tcp::iostream stream;
			stream.exceptions(tcp::iostream::failbit | tcp::iostream::badbit);

			stream.connect(serverInfoList[i].name, serverInfoList[i].port);

			// Enable keep alive option
			stream.rdbuf()->set_option(boost::asio::socket_base::keep_alive(true));
#if defined(__linux__) || defined(__MACOSX__)
			// Set keep alive parameters on *nix platforms
			const int nativeSocket = static_cast<int>(stream.rdbuf()->native());
			int optval = 3; // Retry count
			const socklen_t optlen = sizeof(optval);
			setsockopt(nativeSocket, SOL_TCP, TCP_KEEPCNT, &optval, optlen);
			optval = 30; // Keep alive interval
			setsockopt(nativeSocket, SOL_TCP, TCP_KEEPIDLE, &optval, optlen);
			optval = 5; // Time between retries
			setsockopt(nativeSocket, SOL_TCP, TCP_KEEPINTVL, &optval, optlen);
#endif

			// Send the command to get the film
			stream << "luxGetFilm" << std::endl;
			stream << serverInfoList[i].sid << std::endl;

			// Receive the film in a compressed format
			multibuffer_device mbdev;
			boost::iostreams::stream<multibuffer_device> compressedStream(mbdev);

			// Get the time here before we fetch the stream in case it takes
			// a very long time to transfer the data. This time will be used
			// to calculate the slave nodes samples per second.
			boost::posix_time::ptime samplesRetrievedTime = second_clock::local_time();

			compressedStream << stream.rdbuf();

			stream.close();

			std::streampos compressedSize = compressedStream.tellp();

			compressedStream.seekg(0, BOOST_IOS::beg);

			// Decopress and merge the film
			const double sampleCount = film->UpdateFilm(compressedStream);
			if (sampleCount == 0.)
				throw string("Received 0 samples from server");
			film->numberOfSamplesFromNetwork += sampleCount;
			serverInfoList[i].numberOfSamplesReceived += sampleCount;
			serverInfoList[i].calculatedSamplesPerSecond = sampleCount / (samplesRetrievedTime - serverInfoList[i].timeLastSamples).total_seconds();
			serverInfoList[i].timeLastSamples = samplesRetrievedTime;

			LOG( LUX_INFO,LUX_NOERROR) << "Samples received from '" <<
					serverInfoList[i].name << ":" << serverInfoList[i].port << "' (" <<
					(compressedSize / 1024) << " Kbytes)";

			serverInfoList[i].timeLastContact = second_clock::local_time();
		} catch (string s) {
			LOG(LUX_ERROR,LUX_SYSTEM)<< s.c_str();
			// Mark as failed (inactive)
			serverInfoList[i].active = false;
		} catch (std::exception& e) {
			LOG( LUX_ERROR,LUX_SYSTEM) << "Error while communicating with server: " <<
					serverInfoList[i].name << ":" << serverInfoList[i].port << " ( " << e.what() << ")";
			// Mark as failed (inactive)
			serverInfoList[i].active = false;
		}
	}

	// attempt to reconnect
	reconnectFailed();
}

void RenderFarm::updateLog() {
	// Using the mutex in order to not allow server disconnection while
	// I'm downloading a film
	boost::mutex::scoped_lock lock(serverListMutex);

	// first try to reconnect to failed servers which may be up now
	reconnectFailed();

	for (size_t i = 0; i < serverInfoList.size(); i++) {
		if (!serverInfoList[i].active)
			// skip servers which are still down
			continue;

		try {
			LOG( LUX_DEBUG,LUX_NOERROR) << "Getting log from: " <<
					serverInfoList[i].name << ":" << serverInfoList[i].port;

			// Connect to the server
			tcp::iostream stream;
			stream.exceptions(tcp::iostream::failbit | tcp::iostream::badbit);

			stream.connect(serverInfoList[i].name, serverInfoList[i].port);

			LOG( LUX_DEBUG,LUX_NOERROR) << "Connected to: " << stream.rdbuf()->remote_endpoint();

			// Send the command to get the log
			stream << "luxGetLog" << std::endl;
			stream << serverInfoList[i].sid << std::endl;

			// Receive the log
			std::stringstream log;
			log << stream.rdbuf();

			stream.close();

			int severityFilter = luxGetErrorFilter();

			while (log.good()) {
				int code, severity;
				string message;

				log >> severity;
				log >> code;
				log >> std::ws;
				getline(log, message);

				if (message == "")
					continue;

				// unless we're running in verbose mode, filter away debug and info messages from slaves
				if (severityFilter > LUX_DEBUG && severity < max(LUX_WARNING, severityFilter))
					continue;

				LOG(severity, code) << "[" << serverInfoList[i].name << ":" << serverInfoList[i].port << "] " 
					<< message;
			}

			serverInfoList[i].timeLastContact = second_clock::local_time();
		} catch (string s) {
			LOG(LUX_ERROR,LUX_SYSTEM)<< s.c_str();
			// Mark as failed (inactive)
			serverInfoList[i].active = false;
		} catch (std::exception& e) {
			LOG( LUX_ERROR,LUX_SYSTEM) << "Error while communicating with server: " <<
					serverInfoList[i].name << ":" << serverInfoList[i].port << " ( " << e.what() << ")";
			LOG(LUX_ERROR,LUX_SYSTEM)<< e.what();
			// Mark as failed (inactive)
			serverInfoList[i].active = false;
		}
	}

	// attempt to reconnect
	reconnectFailed();
}

// to catch the interrupted exception
static void flush_thread_func(RenderFarm *renderFarm) {
	try {
		renderFarm->flush();
	} catch (boost::thread_interrupted&) {
	}
}

void RenderFarm::send(const string &command) {
	compiledCommands.add(command);

	// Check if the scene is complete
	if (command == "luxWorldEnd") {
		netBufferComplete = true;
		// perform async flush
		//flushThread = new boost::thread(boost::bind(flush_thread_func, this));
		// synch flush
		flush();
	}
}

void RenderFarm::send(const string &command, const string &name,
		const ParamSet &params) {
	try {
		CompiledCommand &ccmd(compiledCommands.add(command));

		ccmd.buffer() << name << endl;

		ccmd.addParams(params);

		vector<string> fileParams;
		fileParams.push_back("mapname");
		fileParams.push_back("iesname");
		fileParams.push_back("configfile");
		if (command != "luxFilm")
			fileParams.push_back("filename");

		for (size_t i = 0; i < fileParams.size(); i++) {
			const string& paramName(fileParams[i]);
			//send the files
			string file;
			file = params.FindOneString(paramName, "");
			if (file == "" || FileData::present(params, paramName))
				continue;

			// silent replacement, since relevant plugin will report replacement
			const string real_filename = AdjustFilename(file, true);
			CompiledFile cf = compiledFiles.add(real_filename);

			ccmd.addFile(paramName, cf);
		}

	} catch (exception& e) {
		LOG(LUX_ERROR,LUX_SYSTEM)<< e.what();
	}
}

void RenderFarm::send(const string &command, const string &id,
	const string &name, const ParamSet &params)
{
	try {
		CompiledCommand &ccmd(compiledCommands.add(command));

		ccmd.buffer() << id << endl << name << endl;
		ccmd.addParams(params);
	} catch (exception& e) {
		LOG(LUX_ERROR,LUX_SYSTEM)<< e.what();
	}
}

void RenderFarm::send(const string &command, const string &name) {
	try {
		CompiledCommand &ccmd(compiledCommands.add(command));

		ccmd.buffer() << name << endl;
	} catch (exception& e) {
		LOG(LUX_ERROR,LUX_SYSTEM)<< e.what();
	}
}

void RenderFarm::send(const string &command, float x, float y, float z) {
	try {
		CompiledCommand &ccmd(compiledCommands.add(command));

		ccmd.buffer() << x << ' ' << y << ' ' << z << endl;
	} catch (exception& e) {
		LOG(LUX_ERROR,LUX_SYSTEM)<< e.what();
	}
}

void RenderFarm::send(const string &command, float x, float y) {
	try {
		CompiledCommand &ccmd(compiledCommands.add(command));

		ccmd.buffer() << x << ' ' << y << ' ' << endl;
	} catch (exception& e) {
		LOG(LUX_ERROR,LUX_SYSTEM)<< e.what();
	}
}

void RenderFarm::send(const string &command, float a, float x, float y,
		float z) {
	try {
		CompiledCommand &ccmd(compiledCommands.add(command));

		ccmd.buffer() << a << ' ' << x << ' ' << y << ' ' << z << endl;
	} catch (exception& e) {
		LOG(LUX_ERROR,LUX_SYSTEM)<< e.what();
	}
}

void RenderFarm::send(const string &command, float ex, float ey, float ez,
		float lx, float ly, float lz, float ux, float uy, float uz) {
	try {
		CompiledCommand &ccmd(compiledCommands.add(command));

		ccmd.buffer() << ex << ' ' << ey << ' ' << ez << ' ' << lx << ' ' << ly << ' ' << lz << ' ' << ux << ' ' << uy << ' ' << uz << endl;
	} catch (exception& e) {
		LOG(LUX_ERROR,LUX_SYSTEM)<< e.what();
	}
}

void RenderFarm::send(const string &command, float tr[16]) {
	try {
		CompiledCommand &ccmd(compiledCommands.add(command));

		for (int i = 0; i < 16; i++)
			ccmd.buffer() << tr[i] << ' ';
		ccmd.buffer() << endl;
	} catch (exception& e) {
		LOG(LUX_ERROR,LUX_SYSTEM)<< e.what();
	}
}

void RenderFarm::send(const string &command, u_int n, float *d) {
	try {
		CompiledCommand &ccmd(compiledCommands.add(command));

		ccmd.buffer() << n << ' ';
		for (u_int i = 0; i < n; i++)
			ccmd.buffer() << d[i] << ' ';
		ccmd.buffer() << endl;
	} catch (exception& e) {
		LOG(LUX_ERROR,LUX_SYSTEM)<< e.what();
	}
}

void RenderFarm::send(const string &command, const string &name,
		const string &type, const string &texname, const ParamSet &params) {
	try {
		CompiledCommand &ccmd(compiledCommands.add(command));

		ccmd.buffer() << name << endl << type << endl << texname << endl;
		ccmd.addParams(params);

		const std::string paramName("filename");
		string file = params.FindOneString(paramName, "");
		if (file != "" && !FileData::present(params, paramName)) {
			// silent replacement, since relevant plugin will report replacement
			const string real_filename = AdjustFilename(file, true);
			CompiledFile cf = compiledFiles.add(real_filename);
			ccmd.addFile(paramName, cf);
		}
	} catch (std::exception& e) {
		LOG(LUX_ERROR,LUX_SYSTEM)<< e.what();
	}
}

void RenderFarm::send(const string &command, const string &name, float a, float b, const string &transform) {
	try {
		CompiledCommand &ccmd(compiledCommands.add(command));

		ccmd.buffer() << name << endl << a << " " << b << endl << transform << endl;
	} catch (exception& e) {
		LOG(LUX_ERROR,LUX_SYSTEM)<< e.what();
	}
}

u_int RenderFarm::getServerCount() const {
	return serverInfoList.size();
}

u_int RenderFarm::getServersStatus(RenderingServerInfo *info, u_int maxInfoCount) const {
	ptime now = second_clock::local_time();
	for (size_t i = 0; i < min<size_t>(serverInfoList.size(), maxInfoCount); ++i) {
		info[i].serverIndex = i;
		info[i].name = serverInfoList[i].name.c_str();
		info[i].port = serverInfoList[i].port.c_str();
		info[i].sid = serverInfoList[i].sid.c_str();

		info[i].secsSinceLastContact = time_duration(now - serverInfoList[i].timeLastContact).seconds();
		info[i].secsSinceLastSamples = time_duration(now - serverInfoList[i].timeLastSamples).seconds();
		info[i].numberOfSamplesReceived = serverInfoList[i].numberOfSamplesReceived;
		info[i].calculatedSamplesPerSecond = serverInfoList[i].calculatedSamplesPerSecond;
	}

	return serverInfoList.size();
}
