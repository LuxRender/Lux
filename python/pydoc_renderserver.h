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

// Python doc-strings for pylux.RenderServer*
// Try to keep within 80 columns

#ifndef LUX_PYDOC_RENDERSERVER_H
#define LUX_PYDOC_RENDERSERVER_H

const char * ds_pylux_RenderServerState =
"Valid states for LuxRender Network Server";

const char * ds_pylux_RenderServer =
"An instance of a LuxRender Network Server";

const char * ds_pylux_RenderServer_getServerPort =
"Get the port that this server is listening on";

const char * ds_pylux_RenderServer_getServerState =
"Get the state of this server";

const char * ds_pylux_RenderServer_start = 
"Start this server";

const char * ds_pylux_RenderServer_stop =
"Stop this server";

const char * ds_pylux_RenderServer_join =
"Join this server thread";

#endif	// LUX_PYDOC_RENDERSERVER_H