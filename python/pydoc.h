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

// Python doc-strings for pylux module
// Try to keep within 80 columns

// ref: http://epydoc.sourceforge.net/manual-epytext.html

#ifndef LUX_PYDOC_H
#define LUX_PYDOC_H

const char * ds_pylux =
"LuxRender Python Bindings\n\n"
"Provides access to the LuxRender API in python\n\n"
"TODO: Docstrings marked (+) need verification.";

const char * ds_pylux_version =
"Returns the pylux/LuxRender version";

const char * ds_pylux_ThreadSignals =
"Valid states for rendering threads";

const char * ds_pylux_RenderingThreadInfo =
"Container class for information about rendering threads";

const char * ds_pylux_Dynload =
"Module for LuxRender compiled-in plugins introspection";

const char * ds_pylux_Component =
"(+) LuxRender Components available to modify at render-time";

const char * ds_pylux_ComponentParameters =
"(+) Parameters of luxComponents available to modify at render time";

const char * ds_pylux_RenderingServerInfo =
"Container class for information about rendering servers";

const char * ds_pylux_errorHandler =
"Specify an alternate error handler (logging) function for LuxRender engine\n"
"output. By default the render engine will print to stdout";

const char * ds_pylux_ErrorSeverity =
"Error severity levels";

const char * ds_pylux_errorFilter =
"Specify an error severity level filter. Errors with severity less than the\n"
"specified level will be suppressed";

const char * ds_pylux_FlexImageFilm =
"Information about the FlexImage film.";

#endif	// LUX_PYDOC_H
