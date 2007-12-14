/***************************************************************************
 *   Copyright (C) 1998-2007 by authors (see AUTHORS.txt )                 *
 *                                                                         *
 *   This file is part of Lux Renderer.                                    *
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
 *   Lux Renderer website : http://www.luxrender.org                       *
 ***************************************************************************/

#ifndef LUX_RENDERFARM_H
#define LUX_RENDERFARM_H

#include <vector>
#include <string>
#include <sstream>
#include "../film/multiimage.h"

class RenderFarm {
public:
	RenderFarm()
	{}
	bool connect(const string &serverName); //!< Connects to a new rendering server
	
	void send(const std::string &command);
	void send(const std::string &command, const std::string &name, const ParamSet &params);
	void send(const std::string &command, const std::string &name);
	void send(const std::string &command, float x, float y, float z);
	void send(const std::string &command, float a, float x, float y, float z);
	void send(const std::string &command, float ex, float ey, float ez, float lx, float ly, float lz, float ux, float uy, float uz);
	void send(const std::string &command, float tr[16]);
	void send(const std::string &command, const string &name, const string &type, const string &texname, const ParamSet &params);
	
	void flush(); //!< Sends immediately all commands in the buffer to the servers
	void updateFilm(MultiImageFilm *film); //!<Gets the films from the network, and merge them to the film given in parameter

private:
	std::vector<std::string> serverList;
	std::stringstream netBuffer;
};

#endif //LUX_ERROR_H
