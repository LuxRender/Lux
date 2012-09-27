/***************************************************************************
 *   Copyright (C) 1998-2012 by authors (see AUTHORS.txt )                 *
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

#ifndef LUXCOMMANDLINE_H
#define LUXCOMMANDLINE_H

#include <iostream>
#include <sstream>
#include <string>
#include <vector>

#include <boost/filesystem/operations.hpp>

#include "api.h"

struct clConfig
{
	clConfig() :
		slave(false), binDump(false), log2console(false), writeFlmFile(false),
		verbosity(0), pollInterval(luxGetIntAttribute("render_farm", "pollingInterval")),
		tcpPort(luxGetIntAttribute("render_farm", "defaultTcpPort")), threadCount(0) {};

	bool slave;
	bool binDump;
	bool log2console;
	bool writeFlmFile;
	int verbosity;
	int pollInterval;
	unsigned int tcpPort;
	unsigned int threadCount;
	std::string password;
	std::string queueFile;
	std::string cacheDir;
	std::vector< std::string > inputFiles;
	std::vector< std::string > slaveNodeList;
};

namespace featureSet {
	enum {
		RENDERER     = (1u << 0),
		MASTERNODE   = (1u << 1),
		SLAVENODE    = (1u << 2),
		INTERACTIVE  = (1u << 3),
	};
};

boost::filesystem::path getDefaultWorkingDirectory();
bool ProcessCommandLine(int argc, char** argv, clConfig& config, unsigned int features, std::streambuf* infoBuf = std::cout.rdbuf(), std::streambuf* warnBuf = std::cerr.rdbuf());

#endif // LUXCOMMANDLINE_H
