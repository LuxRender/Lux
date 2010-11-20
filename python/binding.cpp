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

// boost headers
#include <boost/python.hpp>
#include <boost/python/object.hpp>
#include <boost/thread/once.hpp>

// Lux headers
#include "api.h"

// pylux headers
#include "binding.h"
#include "pydoc.h"
#include "pycontext.h"
#include "pyfleximage.h"
// #include "pyrenderer.h"
#include "pyrenderserver.h"

namespace lux{

//Error handling
boost::python::object pythonErrorHandler;

void luxErrorPython(int code, int severity, const char *message)
{
	pythonErrorHandler(code, severity, message);
}

void pyLuxErrorHandler(boost::python::object handler)
{
	//System wide init (usefull if you set the handler before any context is created)
	boost::call_once(&luxInit, luxInitFlag);

	pythonErrorHandler=handler;
	luxErrorHandler(luxErrorPython);
}

}//namespace lux

/*
 *  Finally, we create the python module using boost/python !
 */
BOOST_PYTHON_MODULE(pylux)
{
	using namespace boost::python;
	using namespace lux;
	docstring_options doc_options(
		true	/* show user defined docstrings */,
		true	/* show python signatures */,
		false	/* show c++ signatures */
	);

	// This 'module' is actually a fake package
	object package = scope();
	package.attr("__path__") = "pylux";
	package.attr("__doc__") = ds_pylux;

	//Direct python module calls
	def("version", luxVersion, ds_pylux_version);

	// Parameter access
	enum_<luxComponent>("Component", ds_pylux_Component)
		.value("LUX_FILM", LUX_FILM)
		;

	enum_<luxComponentParameters>("ComponentParameters", ds_pylux_ComponentParameters)
		.value("LUX_FILM_TM_TONEMAPKERNEL",LUX_FILM_TM_TONEMAPKERNEL)
		.value("LUX_FILM_TM_REINHARD_PRESCALE",LUX_FILM_TM_REINHARD_PRESCALE)
		.value("LUX_FILM_TM_REINHARD_POSTSCALE",LUX_FILM_TM_REINHARD_POSTSCALE)
		.value("LUX_FILM_TM_REINHARD_BURN",LUX_FILM_TM_REINHARD_BURN)
		.value("LUX_FILM_TM_LINEAR_SENSITIVITY",LUX_FILM_TM_LINEAR_SENSITIVITY)
		.value("LUX_FILM_TM_LINEAR_EXPOSURE",LUX_FILM_TM_LINEAR_EXPOSURE)
		.value("LUX_FILM_TM_LINEAR_FSTOP",LUX_FILM_TM_LINEAR_FSTOP)
		.value("LUX_FILM_TM_LINEAR_GAMMA",LUX_FILM_TM_LINEAR_GAMMA)
		.value("LUX_FILM_TM_CONTRAST_YWA",LUX_FILM_TM_CONTRAST_YWA)
		.value("LUX_FILM_TORGB_X_WHITE",LUX_FILM_TORGB_X_WHITE)
		.value("LUX_FILM_TORGB_Y_WHITE",LUX_FILM_TORGB_Y_WHITE)
		.value("LUX_FILM_TORGB_X_RED",LUX_FILM_TORGB_X_RED)
		.value("LUX_FILM_TORGB_Y_RED",LUX_FILM_TORGB_Y_RED)
		.value("LUX_FILM_TORGB_X_GREEN",LUX_FILM_TORGB_X_GREEN)
		.value("LUX_FILM_TORGB_Y_GREEN",LUX_FILM_TORGB_Y_GREEN)
		.value("LUX_FILM_TORGB_X_BLUE",LUX_FILM_TORGB_X_BLUE)
		.value("LUX_FILM_TORGB_Y_BLUE",LUX_FILM_TORGB_Y_BLUE)
		.value("LUX_FILM_TORGB_GAMMA",LUX_FILM_TORGB_GAMMA)
		.value("LUX_FILM_UPDATEBLOOMLAYER",LUX_FILM_UPDATEBLOOMLAYER)
		.value("LUX_FILM_DELETEBLOOMLAYER",LUX_FILM_DELETEBLOOMLAYER)
		.value("LUX_FILM_BLOOMRADIUS",LUX_FILM_BLOOMRADIUS)
		.value("LUX_FILM_BLOOMWEIGHT",LUX_FILM_BLOOMWEIGHT)
		.value("LUX_FILM_VIGNETTING_ENABLED",LUX_FILM_VIGNETTING_ENABLED)
		.value("LUX_FILM_VIGNETTING_SCALE",LUX_FILM_VIGNETTING_SCALE)
		.value("LUX_FILM_ABERRATION_ENABLED",LUX_FILM_ABERRATION_ENABLED)
		.value("LUX_FILM_ABERRATION_AMOUNT",LUX_FILM_ABERRATION_AMOUNT)
		.value("LUX_FILM_UPDATEGLARELAYER",LUX_FILM_UPDATEGLARELAYER)
		.value("LUX_FILM_DELETEGLARELAYER",LUX_FILM_DELETEGLARELAYER)
		.value("LUX_FILM_GLARE_AMOUNT",LUX_FILM_GLARE_AMOUNT)
		.value("LUX_FILM_GLARE_RADIUS",LUX_FILM_GLARE_RADIUS)
		.value("LUX_FILM_GLARE_BLADES",LUX_FILM_GLARE_BLADES)
		.value("LUX_FILM_GLARE_THRESHOLD",LUX_FILM_GLARE_THRESHOLD)
		.value("LUX_FILM_HISTOGRAM_ENABLED",LUX_FILM_HISTOGRAM_ENABLED)
		.value("LUX_FILM_NOISE_CHIU_ENABLED",LUX_FILM_NOISE_CHIU_ENABLED)
		.value("LUX_FILM_NOISE_CHIU_RADIUS",LUX_FILM_NOISE_CHIU_RADIUS)
		.value("LUX_FILM_NOISE_CHIU_INCLUDECENTER",LUX_FILM_NOISE_CHIU_INCLUDECENTER)
		.value("LUX_FILM_NOISE_GREYC_ENABLED",LUX_FILM_NOISE_GREYC_ENABLED)
		.value("LUX_FILM_NOISE_GREYC_AMPLITUDE",LUX_FILM_NOISE_GREYC_AMPLITUDE)
		.value("LUX_FILM_NOISE_GREYC_NBITER",LUX_FILM_NOISE_GREYC_NBITER)
		.value("LUX_FILM_NOISE_GREYC_SHARPNESS",LUX_FILM_NOISE_GREYC_SHARPNESS)
		.value("LUX_FILM_NOISE_GREYC_ANISOTROPY",LUX_FILM_NOISE_GREYC_ANISOTROPY)
		.value("LUX_FILM_NOISE_GREYC_ALPHA",LUX_FILM_NOISE_GREYC_ALPHA)
		.value("LUX_FILM_NOISE_GREYC_SIGMA",LUX_FILM_NOISE_GREYC_SIGMA)
		.value("LUX_FILM_NOISE_GREYC_FASTAPPROX",LUX_FILM_NOISE_GREYC_FASTAPPROX)
		.value("LUX_FILM_NOISE_GREYC_GAUSSPREC",LUX_FILM_NOISE_GREYC_GAUSSPREC)
		.value("LUX_FILM_NOISE_GREYC_DL",LUX_FILM_NOISE_GREYC_DL)
		.value("LUX_FILM_NOISE_GREYC_DA",LUX_FILM_NOISE_GREYC_DA)
		.value("LUX_FILM_NOISE_GREYC_INTERP",LUX_FILM_NOISE_GREYC_INTERP)
		.value("LUX_FILM_NOISE_GREYC_TILE",LUX_FILM_NOISE_GREYC_TILE)
		.value("LUX_FILM_NOISE_GREYC_BTILE",LUX_FILM_NOISE_GREYC_BTILE)
		.value("LUX_FILM_NOISE_GREYC_THREADS",LUX_FILM_NOISE_GREYC_THREADS)
		.value("LUX_FILM_LG_COUNT",LUX_FILM_LG_COUNT)
		.value("LUX_FILM_LG_ENABLE",LUX_FILM_LG_ENABLE)
		.value("LUX_FILM_LG_NAME",LUX_FILM_LG_NAME)
		.value("LUX_FILM_LG_SCALE",LUX_FILM_LG_SCALE)
		.value("LUX_FILM_LG_SCALE_RED",LUX_FILM_LG_SCALE_RED)
		.value("LUX_FILM_LG_SCALE_BLUE",LUX_FILM_LG_SCALE_BLUE)
		.value("LUX_FILM_LG_SCALE_GREEN",LUX_FILM_LG_SCALE_GREEN)
		.value("LUX_FILM_LG_TEMPERATURE",LUX_FILM_LG_TEMPERATURE)
		.value("LUX_FILM_LG_SCALE_X",LUX_FILM_LG_SCALE_X)
		.value("LUX_FILM_LG_SCALE_Y",LUX_FILM_LG_SCALE_Y)
		.value("LUX_FILM_LG_SCALE_Z",LUX_FILM_LG_SCALE_Z)
		;

	//Error handling in python
	def("errorHandler",
		pyLuxErrorHandler,
		args("function"),
		ds_pylux_errorHandler
	);
	
	// Add definitions given in other header files
	export_PyContext();
	export_PyFlexImageFilm();
	// export_PyRenderer();
	export_PyRenderServer();
}
