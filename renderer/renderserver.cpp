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

// This include must come first (before lux.h)
#include <boost/serialization/vector.hpp>

#include "renderserver.h"
#include "api.h"
#include "context.h"
#include "paramset.h"
#include "error.h"
#include "color.h"
#include "osfunc.h"

#include <boost/version.hpp>
#if (BOOST_VERSION < 103401)
#include <boost/filesystem/operations.hpp>
#else
#include <boost/filesystem.hpp>
#endif
#include <fstream>
#include <boost/asio.hpp>
#include <boost/iostreams/filtering_stream.hpp>
#include <boost/iostreams/filtering_streambuf.hpp>
#include <boost/iostreams/copy.hpp>
#include <boost/iostreams/filter/zlib.hpp>
#include <boost/iostreams/filter/bzip2.hpp>
#include <boost/iostreams/filter/gzip.hpp>

using namespace lux;
using namespace boost::iostreams;
using namespace boost::filesystem;
using namespace std;
using boost::asio::ip::tcp;

//------------------------------------------------------------------------------
// RenderServer
//------------------------------------------------------------------------------

RenderServer::RenderServer(int tCount, int port) : threadCount(tCount),
	tcpPort(port), state(UNSTARTED), serverThread(NULL)
{
}

RenderServer::~RenderServer()
{
	if ((state == READY) && (state == BUSY))
		stop();
}

void RenderServer::start() {
	if (state != UNSTARTED) {
		stringstream ss;
		ss << "Can not start a rendering server in state: " << state;
		luxError(LUX_SYSTEM, LUX_ERROR, ss.str().c_str());

		return;
	}

	// Dade - start the tcp server thread
	serverThread = new NetworkRenderServerThread(this);
	serverThread->serverThread = new boost::thread(boost::bind(
		NetworkRenderServerThread::run, serverThread));

	state = READY;
}

void RenderServer::join()
{
	if ((state != READY) && (state != BUSY)) {
		stringstream ss;
		ss << "Can not join a rendering server in state: " << state;
		luxError(LUX_SYSTEM, LUX_ERROR, ss.str().c_str());

		return;
	}

	serverThread->join();
}

void RenderServer::stop()
{
	if ((state != READY) && (state != BUSY)) {
		stringstream ss;
		ss << "Can not stop a rendering server in state: " << state;
		luxError(LUX_SYSTEM, LUX_ERROR, ss.str().c_str());

		return;
	}

	serverThread->interrupt();
	serverThread->join();

	state = STOPPED;
}

//------------------------------------------------------------------------------
// NetworkRenderServerThread
//------------------------------------------------------------------------------

static void printInfoThread()
{
	while (true) {
		boost::xtime xt;
		boost::xtime_get(&xt, boost::TIME_UTC);
		xt.sec += 5;
		boost::thread::sleep(xt);

		boost::posix_time::time_duration td(0, 0,
			static_cast<int>(luxStatistics("secElapsed")), 0);

		int sampleSec = static_cast<int>(luxStatistics("samplesSec"));
		// Dade - print only if we are rendering something
		if (sampleSec > 0) {
			stringstream ss;
			ss << td << "  " << sampleSec << " samples/sec " << " "
				<< luxStatistics("samplesPx") << " samples/pix";
			luxError(LUX_NOERROR, LUX_INFO, ss.str().c_str());
		}
	}
}

static void processCommandParams(bool isLittleEndian,
		ParamSet &params, basic_istream<char> &stream) {
	stringstream uzos(stringstream::in | stringstream::out | stringstream::binary);
	{
		// Read the size of the compressed chunk
		uint32_t size = osReadLittleEndianUInt(isLittleEndian, stream);

		// Read the compressed chunk
		char *zbuf = new char[size];
		stream.read(zbuf, size);
		stringstream zos(stringstream::in | stringstream::out  | stringstream::binary);
		zos.write(zbuf, size);
		delete zbuf;

		// Uncompress the chunk
		filtering_stream<input> in;
		in.push(gzip_decompressor());
		in.push(zos);
		boost::iostreams::copy(in, uzos);
	}

	// Deserialize the parameters
	boost::archive::text_iarchive ia(uzos);
	ia >> params;
}

static void processCommandFilm(bool isLittleEndian,
		void (&f)(const string &, const ParamSet &), basic_istream<char> &stream)
{
	string type;
	getline(stream, type);

	if((type != "fleximage") && (type != "multiimage")) {
		stringstream ss;
		ss << "Unsupported film type for server rendering: " << type;
		luxError(LUX_SYSTEM, LUX_ERROR, ss.str().c_str());

		return;
	}

	ParamSet params;
	processCommandParams(isLittleEndian, params, stream);

	// Dade - overwrite some option for the servers

	params.EraseBool("write_exr");
	params.EraseBool("write_exr_ZBuf");
	params.EraseBool("write_png");
	params.EraseBool("write_png_ZBuf");
	params.EraseBool("write_tga");
	params.EraseBool("write_tga_ZBuf");
	params.EraseBool("write_resume_flm");

	bool no = false;
	params.AddBool("write_exr", &no);
	params.AddBool("write_exr_ZBuf", &no);
	params.AddBool("write_png", &no);
	params.AddBool("write_png_ZBuf", &no);
	params.AddBool("write_tga", &no);
	params.AddBool("write_tga_ZBuf", &no);
	params.AddBool("write_resume_flm", &no);

	f(type.c_str(), params);
}

static void processFile(const string &fileParam, ParamSet &params, vector<string> &tmpFileList, basic_istream<char> &stream)
{
	string originalFile = params.FindOneString(fileParam, "");
	if (originalFile.size()) {
		// Dade - look for file extension
		string fileExt = "";
		size_t idx = originalFile.find_last_of('.');
		if (idx != string::npos)
			fileExt = originalFile.substr(idx);

		// Dade - replace the file name with a temporary name
		char buf[64];
		if (tmpFileList.size())
			snprintf(buf, 64, "%5s_%08lu%s", tmpFileList[0].c_str(),
				tmpFileList.size(), fileExt.c_str()); // %08zu should be ued but it could be not supported by the compiler
		else
			snprintf(buf, 64, "00000_%08lu%s",
				tmpFileList.size(), fileExt.c_str()); // %08zu should be ued but it could be not supported by the compiler
		string file = string(buf);

		// Dade - replace the filename parameter
		params.AddString(fileParam, &file);

		stringstream ss("");
		ss << "Receiving file: '" << originalFile << "' (in '" <<
			file << "')";
		luxError(LUX_NOERROR, LUX_INFO, ss.str().c_str());

		bool first = true;
		string s;
		ofstream out;
		while (getline(stream, s) && (s != "LUX_END_FILE")) {
			if (!first)
				out << "\n";
			else {
				// Dade - fix for bug 514: avoid to create the file if it is empty
				out.open(file.c_str(), ios::out | ios::binary);
				first = false;
			}

			out << s;
		}

		if (!first) {
			out.flush();
			tmpFileList.push_back(file);
		}
	}
}

static void processCommand(bool isLittleEndian,
		void (&f)(const string &, const ParamSet &), vector<string> &tmpFileList, basic_istream<char> &stream)
{
	string type;
	getline(stream, type);

	ParamSet params;
	processCommandParams(isLittleEndian, params, stream);

	processFile("mapname", params, tmpFileList, stream);
	processFile("iesName", params, tmpFileList, stream);

	f(type.c_str(), params);
}

static void processCommand(void (&f)(const string &), basic_istream<char> &stream)
{
	string type;
	getline(stream, type);

	f(type.c_str());
}

static void processCommand(void (&f)(float, float), basic_istream<char> &stream)
{
	float x, y;
	stream >> x;
	stream >> y;
	f(x, y);
}

static void processCommand(void (&f)(float, float, float), basic_istream<char> &stream)
{
	float ax, ay, az;
	stream >> ax;
	stream >> ay;
	stream >> az;
	f(ax, ay, az);
}

static void processCommand(void (&f)(float[16]), basic_istream<char> &stream)
{
	float t[16];
	for (int i = 0; i < 16; ++i)
		stream >> t[i];
	f(t);
}

static void processCommand(void (&f)(const string &, float, float, const string &), basic_istream<char> &stream)
{
	string name, transform;
	float a, b;

	stream >> name;
	stream >> a;
	stream >> b;
	stream >> transform;

	f(name, a, b, transform);
}


string RenderServer::createNewSessionID() {
	char buf[4 * 4 + 4];
	sprintf(buf, "%04d_%04d_%04d_%04d", rand() % 9999, rand() % 9999,
			rand() % 9999, rand() % 9999);

	return string(buf);
}

bool RenderServer::validateAccess(basic_istream<char> &stream) const {
	if (serverThread->renderServer->state != RenderServer::BUSY)
		return false;

	string sid;
	if (!getline(stream, sid))
		return false;

	stringstream ss;
    ss << "Validating SID: " << sid << " = " << currentSID;
    luxError(LUX_NOERROR, LUX_INFO, ss.str().c_str());

	return (sid == currentSID);
}

// Dade - TODO: support signals
void NetworkRenderServerThread::run(NetworkRenderServerThread *serverThread)
{
    //Command identifiers (hashed strings)
    //These identifiers are used in the switch statement to avoid string comparisons (fastest)
	static const unsigned int CMD_LUXINIT = 2531407474U,
		CMD_LUXTRANSLATE = 2039302476U,
		CMD_LUXROTATE = 3982485453U,
		CMD_LUXSCALE = 1943523046U,
		CMD_LUXLOOKAT = 3747502632U,
		CMD_LUXCONCATTRANSFORM = 449663410U,
		CMD_LUXTRANSFORM = 2039102042U,
		CMD_LUXIDENTITY = 97907816U,
		CMD_LUXCOORDINATESYSTEM = 1707244427U,
		CMD_LUXCOORDSYSTRANSFORM = 2024449520U,
		CMD_LUXPIXELFILTER = 2384561510U,
		CMD_LUXFILM = 2531294310U,
		CMD_LUXSAMPLER = 3308802546U,
		CMD_LUXACCELERATOR = 1613429731U,
		CMD_LUXSURFACEINTEGRATOR = 4011931910U,
		CMD_LUXVOLUMEINTEGRATOR = 2954204437U,
		CMD_LUXCAMERA = 3378604391U,
		CMD_LUXWORLDBEGIN = 1247285547U,
		CMD_LUXATTRIBUTEBEGIN = 684297207U,
		CMD_LUXATTRIBUTEEND = 3427929065U,
		CMD_LUXTRANSFORMBEGIN = 567425599U,
		CMD_LUXTRANSFORMEND = 2773125169U,
		CMD_LUXTEXTURE = 475043887U,
		CMD_LUXMATERIAL = 4064803661U,
		CMD_LUXLIGHTGROUP = 770727715U,
		CMD_LUXMAKENAMEDMATERIAL = 2355625968U,
		CMD_LUXNAMEDMATERIAL = 922480690U,
		CMD_LUXLIGHTSOURCE = 130489799U,
		CMD_LUXAREALIGHTSOURCE = 515057184U,
		CMD_LUXPORTALSHAPE = 3416559329U,
		CMD_LUXSHAPE = 1943702863U,
		CMD_LUXREVERSEORIENTATION = 2027239206U,
		CMD_LUXVOLUME = 4138761078U,
		CMD_LUXOBJECTBEGIN = 1097337658U,
		CMD_LUXOBJECTEND = 229760620U,
		CMD_LUXOBJECTINSTANCE = 4125664042U,
		CMD_LUXWORLDEND = 1582674973U,
		CMD_LUXGETFILM = 859419430U,
		CMD_SERVER_DISCONNECT = 2500584742U,
		CMD_SERVER_CONNECT = 332355398U,
		CMD_VOID = 5381U,
		CMD_SPACE = 177605U,
		CMD_MOTIONINSTANCE = 4223946185U,
		CMD_LUXSETEPSILON = 3945573060U;

	const int listenPort = serverThread->renderServer->tcpPort;
	const bool isLittleEndian = osIsLittleEndian();
	stringstream ss;
	ss << "Launching server [" << serverThread->renderServer->threadCount <<
		" threads] mode on port '" << listenPort << "'.";
	luxError(LUX_NOERROR, LUX_INFO, ss.str().c_str());

	vector<string> tmpFileList;
	try {
		boost::asio::io_service io_service;
		tcp::endpoint endpoint(tcp::v4(), listenPort);
		tcp::acceptor acceptor(io_service, endpoint);

		while (true) {
			tcp::iostream stream;
			acceptor.accept(*stream.rdbuf());
			stream.setf(ios::scientific, ios::floatfield);
			stream.precision(16);

			//reading the command
			string command;
			while (getline(stream, command)) {
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
				case CMD_SERVER_DISCONNECT:
					if (!serverThread->renderServer->validateAccess(stream))
						break;

					// Dade - stop the rendering and cleanup
					luxExit();
					luxWait();
					luxCleanup();

					// Dade - remove all temporary files
					for (size_t i = 1; i < tmpFileList.size(); i++)
						remove(tmpFileList[i]);

					serverThread->renderServer->state = RenderServer::READY;
					break;
				case CMD_SERVER_CONNECT:
					if (serverThread->renderServer->state == RenderServer::READY) {
						serverThread->renderServer->state = RenderServer::BUSY;
						stream << "OK" << endl;

						// Dade - generate the session ID
						serverThread->renderServer->currentSID =
							RenderServer::createNewSessionID();
						ss.str("");
						ss << "New session ID: " << serverThread->renderServer->currentSID;
						luxError(LUX_NOERROR, LUX_INFO, ss.str().c_str());
						stream << serverThread->renderServer->currentSID << endl;

						tmpFileList.clear();
						char buf[6];
						snprintf(buf, 6, "%05d", listenPort);
						tmpFileList.push_back(string(buf));
					} else
						stream << "BUSY" << endl;
					break;
				case CMD_LUXINIT:
					luxError(LUX_BUG, LUX_SEVERE, "Server already initialized");
					break;
				case CMD_LUXTRANSLATE:
					processCommand(Context::luxTranslate, stream);
					break;
				case CMD_LUXROTATE: {
					float angle, ax, ay, az;
					stream >> angle;
					stream >> ax;
					stream >> ay;
					stream >> az;
					luxRotate(angle, ax, ay, az);
					break;
				}
				case CMD_LUXSCALE:
					processCommand(Context::luxScale, stream);
					break;
				case CMD_LUXLOOKAT: {
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
					luxLookAt(ex, ey, ez, lx, ly, lz, ux, uy, uz);
					break;
				}
				case CMD_LUXCONCATTRANSFORM:
					processCommand(Context::luxConcatTransform, stream);
					break;
				case CMD_LUXTRANSFORM:
					processCommand(Context::luxTransform, stream);
					break;
				case CMD_LUXIDENTITY:
					Context::luxIdentity();
					break;
				case CMD_LUXCOORDINATESYSTEM:
					processCommand(Context::luxCoordinateSystem, stream);
					break;
				case CMD_LUXCOORDSYSTRANSFORM:
					processCommand(Context::luxCoordSysTransform, stream);
					break;
				case CMD_LUXPIXELFILTER:
					processCommand(isLittleEndian, Context::luxPixelFilter, tmpFileList, stream);
					break;
				case CMD_LUXFILM: {
					// Dade - Servers use a special kind of film to buffer the
					// samples. I overwrite some option here.

					processCommandFilm(isLittleEndian, Context::luxFilm, stream);
					break;
				}
				case CMD_LUXSAMPLER:
					processCommand(isLittleEndian, Context::luxSampler, tmpFileList, stream);
					break;
				case CMD_LUXACCELERATOR:
					processCommand(isLittleEndian, Context::luxAccelerator, tmpFileList, stream);
					break;
				case CMD_LUXSURFACEINTEGRATOR:
					processCommand(isLittleEndian, Context::luxSurfaceIntegrator, tmpFileList, stream);
					break;
				case CMD_LUXVOLUMEINTEGRATOR:
					processCommand(isLittleEndian, Context::luxVolumeIntegrator, tmpFileList, stream);
					break;
				case CMD_LUXCAMERA:
					processCommand(isLittleEndian, Context::luxCamera, tmpFileList, stream);
					break;
				case CMD_LUXWORLDBEGIN:
					Context::luxWorldBegin();
					break;
				case CMD_LUXATTRIBUTEBEGIN:
					Context::luxAttributeBegin();
					break;
				case CMD_LUXATTRIBUTEEND:
					Context::luxAttributeEnd();
					break;
				case CMD_LUXTRANSFORMBEGIN:
					Context::luxTransformBegin();
					break;
				case CMD_LUXTRANSFORMEND:
					Context::luxTransformEnd();
					break;
				case CMD_LUXTEXTURE: {
					string name, type, texname;
					ParamSet params;
					// Dade - fixed in bug 562: "Luxconsole -s (Linux 64) fails to network render when material names contain spaces"
					getline(stream, name);
					getline(stream, type);
					getline(stream, texname);

					processCommandParams(isLittleEndian, params, stream);

					processFile("filename", params, tmpFileList, stream);

					Context::luxTexture(name.c_str(), type.c_str(), texname.c_str(), params);
					break;
				}
				case CMD_LUXMATERIAL:
					processCommand(isLittleEndian, Context::luxMaterial, tmpFileList, stream);
					break;
				case CMD_LUXMAKENAMEDMATERIAL:
					processCommand(isLittleEndian, Context::luxMakeNamedMaterial, tmpFileList, stream);
					break;
				case CMD_LUXNAMEDMATERIAL:
					processCommand(isLittleEndian, Context::luxNamedMaterial, tmpFileList, stream);
					break;
				case CMD_LUXLIGHTGROUP:
					processCommand(isLittleEndian, Context::luxLightGroup, tmpFileList, stream);
					break;
				case CMD_LUXLIGHTSOURCE:
					processCommand(isLittleEndian, Context::luxLightSource, tmpFileList, stream);
					break;
				case CMD_LUXAREALIGHTSOURCE:
					processCommand(isLittleEndian, Context::luxAreaLightSource, tmpFileList, stream);
					break;
				case CMD_LUXPORTALSHAPE:
					processCommand(isLittleEndian, Context::luxPortalShape, tmpFileList, stream);
					break;
				case CMD_LUXSHAPE:
					processCommand(isLittleEndian, Context::luxShape, tmpFileList, stream);
					break;
				case CMD_LUXREVERSEORIENTATION:
					Context::luxReverseOrientation();
					break;
				case CMD_LUXVOLUME:
					processCommand(isLittleEndian, Context::luxVolume, tmpFileList, stream);
					break;
				case CMD_LUXOBJECTBEGIN:
					processCommand(Context::luxObjectBegin, stream);
					break;
				case CMD_LUXOBJECTEND:
					Context::luxObjectEnd();
					break;
				case CMD_LUXOBJECTINSTANCE:
					processCommand(Context::luxObjectInstance, stream);
					break;
				case CMD_MOTIONINSTANCE:
					processCommand(Context::luxMotionInstance, stream);
					break;
				case CMD_LUXWORLDEND: {
					serverThread->engineThread = new boost::thread(&luxWorldEnd);

					// Wait the scene parsing to finish
					while (!luxStatistics("sceneIsReady")) {
						boost::xtime xt;
						boost::xtime_get(&xt, boost::TIME_UTC);
						xt.sec += 1;
						boost::thread::sleep(xt);
					}

					// Dade - start the info thread only if it is not already running
					if(!serverThread->infoThread)
						serverThread->infoThread = new boost::thread(&printInfoThread);

					// Add rendering threads
					int threadsToAdd = serverThread->renderServer->threadCount;
					while (--threadsToAdd)
						Context::luxAddThread();
					break;
				}
				case CMD_LUXGETFILM: {
					if (!serverThread->renderServer->validateAccess(stream))
						break;

					// Dade - check if we are rendering something
					if (serverThread->renderServer->state == RenderServer::BUSY) {
						luxError(LUX_NOERROR, LUX_INFO, "Transmitting film samples");

						Context::luxTransmitFilm(stream);
						stream.close();

						luxError(LUX_NOERROR, LUX_INFO, "Finished film samples transmission");
					} else {
						luxError(LUX_SYSTEM, LUX_ERROR, "Received a GetFilm command after a ServerDisconnect");
						stream.close();
					}
					break;
				}
				case CMD_LUXSETEPSILON:
					processCommand(Context::luxSetEpsilon, stream);
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
	} catch (exception& e) {
		ss.str("");
		ss << "Internal error: " << e.what();
		luxError(LUX_BUG, LUX_ERROR, ss.str().c_str());
	}
}
