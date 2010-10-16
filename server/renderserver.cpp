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

RenderServer::RenderServer(int tCount, int port, bool wFlmFile) : threadCount(tCount),
	tcpPort(port), writeFlmFile(wFlmFile), state(UNSTARTED), serverThread(NULL)
{
}

RenderServer::~RenderServer()
{
	if ((state == READY) && (state == BUSY))
		stop();
}

void RenderServer::start() {
	if (state != UNSTARTED) {
		LOG( LUX_ERROR,LUX_SYSTEM) << "Can not start a rendering server in state: " << state;
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
		LOG( LUX_ERROR,LUX_SYSTEM) << "Can not join a rendering server in state: " << state;
		return;
	}

	serverThread->join();
}

void RenderServer::stop()
{
	if ((state != READY) && (state != BUSY)) {
		LOG( LUX_ERROR,LUX_SYSTEM) << "Can not stop a rendering server in state: " << state;
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

		LOG( LUX_INFO,LUX_NOERROR) << luxPrintableStatistics(true);
	}
}

static void writeTransmitFilm(basic_ostream<char> &stream, const string &filename)
{
	string file = filename;
	string tempfile = file + ".temp";

	LOG( LUX_DEBUG,LUX_NOERROR) << "Writing film samples to file '" << tempfile << "'";

	ofstream out(tempfile.c_str(), ios::out | ios::binary);
	Context::GetActive()->TransmitFilm(out, true, false);
	out.close();

	if (!out.fail()) {
		remove(file.c_str());
		if (rename(tempfile.c_str(), file.c_str())) {
			LOG(LUX_ERROR, LUX_SYSTEM) << 
				"Failed to rename new film file, leaving new film file as '" << tempfile << "'";
			file = tempfile;
		}

		LOG( LUX_DEBUG,LUX_NOERROR) << "Transmitting film samples from file '" << file << "'";
		ifstream in(file.c_str(), ios::in | ios::binary);

		boost::iostreams::copy(in, stream);

		if (in.fail())
			LOG(LUX_ERROR, LUX_SYSTEM) << "There was an error while transmitting from file '" << file << "'";

		in.close();
	} else {
		LOG(LUX_ERROR, LUX_SYSTEM) << "There was an error while writing file '" << tempfile << "'";
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
		void (Context::*f)(const string &, const ParamSet &), basic_istream<char> &stream)
{
	string type;
	getline(stream, type);

	if((type != "fleximage") && (type != "multiimage")) {
		LOG( LUX_ERROR,LUX_SYSTEM) << "Unsupported film type for server rendering: " << type;
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

	(Context::GetActive()->*f)(type, params);
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
		// MSVC doesn't support z length modifier (size_t) for snprintf, so workaround by casting to unsigned long...
		if (tmpFileList.size())
			snprintf(buf, 64, "%5s_%08lu%s", tmpFileList[0].c_str(),
				((unsigned long)tmpFileList.size()), fileExt.c_str()); // %08zu should be ued but it could be not supported by the compiler
		else
			snprintf(buf, 64, "00000_%08lu%s",
				((unsigned long)tmpFileList.size()), fileExt.c_str()); // %08zu should be ued but it could be not supported by the compiler
		string file = string(buf);

		// Dade - replace the filename parameter
		params.AddString(fileParam, &file);

		// Read the file size
		string slen;
		getline(stream, slen); // Eat the \n
		getline(stream, slen);
		// Limiting the file size to 2G should be a problem
		int len = atoi(slen.c_str());

		LOG( LUX_INFO,LUX_NOERROR) << "Receiving file: '" << originalFile << "' (in '" <<
			file << "' size: " << (len / 1024) << " Kbytes)";

		// Dade - fix for bug 514: avoid to create the file if it is empty
		if (len > 0) {
			// Allocate a buffer to read all the file
			char *buf = new char[len];
			stream.read(buf, len);

			ofstream out(file.c_str(), ios::out | ios::binary);
			out.write(buf, len);
			out.flush();
			tmpFileList.push_back(file);

			if (out.fail()) {
				LOG( LUX_ERROR,LUX_SYSTEM) << "There was an error while writing file '" << file << "'";
			}

			delete buf;
		}
	}
}

static void processCommand(bool isLittleEndian,
	void (Context::*f)(const string &, const ParamSet &),
	vector<string> &tmpFileList, basic_istream<char> &stream)
{
	string type;
	getline(stream, type);

	ParamSet params;
	processCommandParams(isLittleEndian, params, stream);

	processFile("mapname", params, tmpFileList, stream);
	processFile("iesname", params, tmpFileList, stream);

	(Context::GetActive()->*f)(type, params);
}

static void processCommand(void (Context::*f)(const string &), basic_istream<char> &stream)
{
	string type;
	getline(stream, type);

	(Context::GetActive()->*f)(type);
}

static void processCommand(void (Context::*f)(float, float), basic_istream<char> &stream)
{
	float x, y;
	stream >> x;
	stream >> y;
	(Context::GetActive()->*f)(x, y);
}

static void processCommand(void (Context::*f)(float, float, float), basic_istream<char> &stream)
{
	float ax, ay, az;
	stream >> ax;
	stream >> ay;
	stream >> az;
	(Context::GetActive()->*f)(ax, ay, az);
}

static void processCommand(void (Context::*f)(float[16]), basic_istream<char> &stream)
{
	float t[16];
	for (int i = 0; i < 16; ++i)
		stream >> t[i];
	(Context::GetActive()->*f)(t);
}

static void processCommand(void (Context::*f)(const string &, float, float, const string &), basic_istream<char> &stream)
{
	string name, transform;
	float a, b;

	stream >> name;
	stream >> a;
	stream >> b;
	stream >> transform;

	(Context::GetActive()->*f)(name, a, b, transform);
}


string RenderServer::createNewSessionID() {
	char buf[4 * 4 + 4];
	sprintf(buf, "%04d_%04d_%04d_%04d", rand() % 9999, rand() % 9999,
			rand() % 9999, rand() % 9999);

	return string(buf);
}

bool RenderServer::validateAccess(basic_istream<char> &stream) const {
	if (serverThread->renderServer->state != RenderServer::BUSY) {
		LOG( LUX_INFO,LUX_NOERROR)<< "Slave does not have an active session";
		return false;
	}

	string sid;
	if (!getline(stream, sid))
		return false;

	LOG( LUX_INFO,LUX_NOERROR) << "Validating SID: " << sid << " = " << currentSID;

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
		CMD_LUXMAKENAMEDVOLUME = 2393963353U,
		CMD_LUXVOLUME = 4138761078U,
		CMD_LUXEXTERIOR = 2058668912U,
		CMD_LUXINTERIOR = 703971178U,
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
		CMD_LUXSETEPSILON = 3945573060U,
		CMD_LUXRENDERER = 3043102805U;

	const int listenPort = serverThread->renderServer->tcpPort;
	const bool isLittleEndian = osIsLittleEndian();
	LOG( LUX_INFO,LUX_NOERROR) << "Launching server [" << serverThread->renderServer->threadCount <<
		" threads] mode on port '" << listenPort << "'.";

	vector<string> tmpFileList;
	try {
		boost::asio::io_service io_service;
		tcp::endpoint endpoint(tcp::v4(), listenPort);
		tcp::acceptor acceptor(io_service, endpoint);

		while (serverThread->signal == SIG_NONE) {
			tcp::iostream stream;
			acceptor.accept(*stream.rdbuf());
			stream.setf(ios::scientific, ios::floatfield);
			stream.precision(16);

			//reading the command
			string command;
			LOG( LUX_INFO,LUX_NOERROR) << "Server receiving commands...";
			while (getline(stream, command)) {
				const unsigned int hash = DJBHash(command);

				if ((command != "") && (command != " ")) {
					LOG(LUX_DEBUG,LUX_NOERROR) << "... processing command: '" << command << "' (hash: " << hash << ")";
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
						LOG( LUX_INFO,LUX_NOERROR) << "New session ID: " << serverThread->renderServer->currentSID;
						stream << serverThread->renderServer->currentSID << endl;

						tmpFileList.clear();
						char buf[6];
						snprintf(buf, 6, "%05d", listenPort);
						tmpFileList.push_back(string(buf));
					} else
						stream << "BUSY" << endl;
					break;
				case CMD_LUXINIT:
					LOG( LUX_SEVERE,LUX_BUG)<< "Server already initialized";
					break;
				case CMD_LUXTRANSLATE:
					processCommand(&Context::Translate, stream);
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
					processCommand(&Context::Scale, stream);
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
					processCommand(&Context::ConcatTransform, stream);
					break;
				case CMD_LUXTRANSFORM:
					processCommand(&Context::Transform, stream);
					break;
				case CMD_LUXIDENTITY:
					luxIdentity();
					break;
				case CMD_LUXCOORDINATESYSTEM:
					processCommand(&Context::CoordinateSystem, stream);
					break;
				case CMD_LUXCOORDSYSTRANSFORM:
					processCommand(&Context::CoordSysTransform, stream);
					break;
				case CMD_LUXPIXELFILTER:
					processCommand(isLittleEndian, &Context::PixelFilter, tmpFileList, stream);
					break;
				case CMD_LUXFILM: {
					// Dade - Servers use a special kind of film to buffer the
					// samples. I overwrite some option here.

					processCommandFilm(isLittleEndian, &Context::Film, stream);
					break;
				}
				case CMD_LUXSAMPLER:
					processCommand(isLittleEndian, &Context::Sampler, tmpFileList, stream);
					break;
				case CMD_LUXACCELERATOR:
					processCommand(isLittleEndian, &Context::Accelerator, tmpFileList, stream);
					break;
				case CMD_LUXSURFACEINTEGRATOR:
					processCommand(isLittleEndian, &Context::SurfaceIntegrator, tmpFileList, stream);
					break;
				case CMD_LUXVOLUMEINTEGRATOR:
					processCommand(isLittleEndian, &Context::VolumeIntegrator, tmpFileList, stream);
					break;
				case CMD_LUXCAMERA:
					processCommand(isLittleEndian, &Context::Camera, tmpFileList, stream);
					break;
				case CMD_LUXWORLDBEGIN:
					luxWorldBegin();
					break;
				case CMD_LUXATTRIBUTEBEGIN:
					luxAttributeBegin();
					break;
				case CMD_LUXATTRIBUTEEND:
					luxAttributeEnd();
					break;
				case CMD_LUXTRANSFORMBEGIN:
					luxTransformBegin();
					break;
				case CMD_LUXTRANSFORMEND:
					luxTransformEnd();
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

					Context::GetActive()->Texture(name, type, texname, params);
					break;
				}
				case CMD_LUXMATERIAL:
					processCommand(isLittleEndian, &Context::Material, tmpFileList, stream);
					break;
				case CMD_LUXMAKENAMEDMATERIAL:
					processCommand(isLittleEndian, &Context::MakeNamedMaterial, tmpFileList, stream);
					break;
				case CMD_LUXNAMEDMATERIAL:
					processCommand(&Context::NamedMaterial, stream);
					break;
				case CMD_LUXLIGHTGROUP:
					processCommand(isLittleEndian, &Context::LightGroup, tmpFileList, stream);
					break;
				case CMD_LUXLIGHTSOURCE:
					processCommand(isLittleEndian, &Context::LightSource, tmpFileList, stream);
					break;
				case CMD_LUXAREALIGHTSOURCE:
					processCommand(isLittleEndian, &Context::AreaLightSource, tmpFileList, stream);
					break;
				case CMD_LUXPORTALSHAPE:
					processCommand(isLittleEndian, &Context::PortalShape, tmpFileList, stream);
					break;
				case CMD_LUXSHAPE:
					processCommand(isLittleEndian, &Context::Shape, tmpFileList, stream);
					break;
				case CMD_LUXREVERSEORIENTATION:
					luxReverseOrientation();
					break;
				case CMD_LUXMAKENAMEDVOLUME: {
					string id, name;
					ParamSet params;
					getline(stream, id);
					getline(stream, name);

					processCommandParams(isLittleEndian,
						params, stream);

					Context::GetActive()->MakeNamedVolume(id, name, params);
					break;
				}
				case CMD_LUXVOLUME:
					processCommand(isLittleEndian, &Context::Volume, tmpFileList, stream);
					break;
				case CMD_LUXEXTERIOR:
					processCommand(&Context::Exterior,
						stream);
					break;
				case CMD_LUXINTERIOR:
					processCommand(&Context::Interior,
						stream);
					break;
				case CMD_LUXOBJECTBEGIN:
					processCommand(&Context::ObjectBegin, stream);
					break;
				case CMD_LUXOBJECTEND:
					luxObjectEnd();
					break;
				case CMD_LUXOBJECTINSTANCE:
					processCommand(&Context::ObjectInstance, stream);
					break;
				case CMD_MOTIONINSTANCE:
					processCommand(&Context::MotionInstance, stream);
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
						luxAddThread();
					break;
				}
				case CMD_LUXGETFILM: {
					// Dade - check if we are rendering something
					if (serverThread->renderServer->state == RenderServer::BUSY) {
						if (!serverThread->renderServer->validateAccess(stream)) {
							LOG( LUX_ERROR,LUX_SYSTEM)<< "Unknown session ID";
							stream.close();
							break;
						}

						LOG( LUX_INFO,LUX_NOERROR)<< "Transmitting film samples";

						if (serverThread->renderServer->writeFlmFile) {
							string file = "server_resume";
							if (tmpFileList.size())
								file += "_" + tmpFileList[0];
							file += ".flm";

							writeTransmitFilm(stream, file);
						} else {
							Context::GetActive()->TransmitFilm(stream);
						}
						stream.close();

						LOG( LUX_INFO,LUX_NOERROR)<< "Finished film samples transmission";
					} else {
						LOG( LUX_ERROR,LUX_SYSTEM)<< "Received a GetFilm command after a ServerDisconnect";
						stream.close();
					}
					break;
				}
				case CMD_LUXSETEPSILON:
					processCommand(&Context::SetEpsilon, stream);
					break;
				case CMD_LUXRENDERER:
					processCommand(isLittleEndian, &Context::Renderer, tmpFileList, stream);
					break;
				default:
					LOG( LUX_SEVERE,LUX_BUG) << "Unknown command '" << command << "'. Ignoring";
					break;
				}

				//END OF COMMAND PROCESSING
			}
		}
	} catch (exception& e) {
		LOG(LUX_ERROR,LUX_BUG) << "Internal error: " << e.what();
	}
}
