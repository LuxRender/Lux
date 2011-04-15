/***************************************************************************
 *   Copyright (C) 1998-2010 by authors (see AUTHORS.txt )                 *
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

#ifndef LUX_PYRENDERSERVER_H
#define LUX_PYRENDERSERVER_H

#include <iostream>

#include "server/renderserver.h"

#include "pydoc_renderserver.h"

namespace lux {

/**
 * Return a useful string to represent RenderingServerInfo objects:
 *  <RenderingServerInfo index:server:port>
 */
boost::python::str RenderingServerInfo_repr(RenderingServerInfo const &rsi)
{
	std::stringstream o(std::ios_base::out);
	o << "<RenderingServerInfo " << rsi.serverIndex << ":" << rsi.name << ":" << rsi.port << ">";
	return boost::python::str(o.str().c_str());
};

} //namespace lux

// Add RenderServer bindings class to pylux module definition
void export_PyRenderServer()
{
	using namespace boost::python;
	using namespace lux;

	class_<RenderingServerInfo>("RenderingServerInfo", ds_pylux_RenderingServerInfo, no_init)
		.def_readonly("serverIndex", &RenderingServerInfo::serverIndex)
		.def_readonly("name", &RenderingServerInfo::name)
		.def_readonly("port", &RenderingServerInfo::port)
		.def_readonly("sid", &RenderingServerInfo::sid)
		.def_readonly("numberOfSamplesReceived", &RenderingServerInfo::numberOfSamplesReceived)
		.def_readonly("secsSinceLastContact", &RenderingServerInfo::secsSinceLastContact)
		.def("__repr__", RenderingServerInfo_repr)
		;

	enum_<RenderServer::ServerState>(
		"RenderServerState", ds_pylux_RenderServerState
		)
		.value("UNSTARTED", RenderServer::UNSTARTED)
		.value("READY", RenderServer::READY)
		.value("BUSY", RenderServer::BUSY)
		.value("STOPPED", RenderServer::STOPPED)
		;
	
	class_<RenderServer, boost::noncopyable>(
		"RenderServer",
		ds_pylux_RenderServer,
		init<int, optional<int,bool> >(args("RenderServer", "threadCount", "tcpPort", "writeFlmFile"))
		)
		/* .def_readonly("DEFAULT_TCP_PORT", &RenderServer::DEFAULT_TCP_PORT) // Doesn't currently work */
		.def("getServerPort",
			&RenderServer::getServerPort,
			args("RenderServer"),
			ds_pylux_RenderServer_getServerPort
		)
		.def("getServerState",
			&RenderServer::getServerState,
			args("RenderServer"),
			ds_pylux_RenderServer_getServerState
		)
		.def("start",
			&RenderServer::start,
			args("RenderServer"),
			ds_pylux_RenderServer_start
		)
		.def("stop",
			&RenderServer::stop,
			args("RenderServer"),
			ds_pylux_RenderServer_stop
		)
		.def("join",
			&RenderServer::join,
			args("RenderServer"),
			ds_pylux_RenderServer_join
		)
		;
}

#endif	// LUX_PYRENDERSERVER_H