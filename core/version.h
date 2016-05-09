/***************************************************************************
 *   Copyright (C) 1998-2013 by authors (see AUTHORS.txt)                  *
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

#ifndef LUX_VERSION_H
#define LUX_VERSION_H

//! Macros that convert a number in input into a string. 
#define XVERSION_STR(v) #v
#define VERSION_STR(v) XVERSION_STR(v)

#define LUX_VN_MAJOR 1
#define LUX_VN_MINOR 6
#define LUX_VN_PATCH 0
#define LUX_VN_BUILD 0
#define LUX_VN_LABEL ""

#define LUX_SERVER_PROTOCOL_VERSION  1011

#define LUX_VERSION_STRING           VERSION_STR(LUX_VN_MAJOR)     \
                                     "." VERSION_STR(LUX_VN_MINOR) \
                                     "." VERSION_STR(LUX_VN_PATCH) \
                                     " " LUX_VN_LABEL \
                                     " Build " VERSION_STR(LUX_VN_BUILD)

//! Renderfarms rely on the 'protocol' part of in server version string
#define LUX_SERVER_VERSION_STRING    VERSION_STR(LUX_VN_MAJOR)     \
                                     "." VERSION_STR(LUX_VN_MINOR) \
                                     "." VERSION_STR(LUX_VN_PATCH) \
                                     " " LUX_VN_LABEL \
									 " (protocol: " VERSION_STR(LUX_SERVER_PROTOCOL_VERSION) ")"

#endif // LUX_VERSION_H
