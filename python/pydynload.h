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
 *   Lux Renderer website : http://www.luxrender.org                       *
 ***************************************************************************/

#ifndef LUX_PYDYNLOAD_H
#define LUX_PYDYNLOAD_H


#include "dynload.h"

namespace lux {

/*

USE pydynload_generator.py TO GENERATE THIS FILE !!

*/

boost::python::list py_getRegisteredVolumeRegions()
{
	boost::python::list names;
	map<string, DynamicLoader::CreateVolumeRegion> &pluginMap = DynamicLoader::registeredVolumeRegions();
	map<string, DynamicLoader::CreateVolumeRegion>::iterator mapit;
	for (mapit=pluginMap.begin(); mapit != pluginMap.end(); mapit++)
	{
		names.append( (*mapit).first );
	}

	return names;
};


boost::python::list py_getRegisteredFloatTextures()
{
	boost::python::list names;
	map<string, DynamicLoader::CreateFloatTexture> &pluginMap = DynamicLoader::registeredFloatTextures();
	map<string, DynamicLoader::CreateFloatTexture>::iterator mapit;
	for (mapit=pluginMap.begin(); mapit != pluginMap.end(); mapit++)
	{
		names.append( (*mapit).first );
	}

	return names;
};


boost::python::list py_getRegisteredFilters()
{
	boost::python::list names;
	map<string, DynamicLoader::CreateFilter> &pluginMap = DynamicLoader::registeredFilters();
	map<string, DynamicLoader::CreateFilter>::iterator mapit;
	for (mapit=pluginMap.begin(); mapit != pluginMap.end(); mapit++)
	{
		names.append( (*mapit).first );
	}

	return names;
};


boost::python::list py_getRegisteredSurfaceIntegrators()
{
	boost::python::list names;
	map<string, DynamicLoader::CreateSurfaceIntegrator> &pluginMap = DynamicLoader::registeredSurfaceIntegrators();
	map<string, DynamicLoader::CreateSurfaceIntegrator>::iterator mapit;
	for (mapit=pluginMap.begin(); mapit != pluginMap.end(); mapit++)
	{
		names.append( (*mapit).first );
	}

	return names;
};


boost::python::list py_getRegisteredRenderer()
{
	boost::python::list names;
	map<string, DynamicLoader::CreateRenderer> &pluginMap = DynamicLoader::registeredRenderer();
	map<string, DynamicLoader::CreateRenderer>::iterator mapit;
	for (mapit=pluginMap.begin(); mapit != pluginMap.end(); mapit++)
	{
		names.append( (*mapit).first );
	}

	return names;
};


boost::python::list py_getRegisteredFresnelTextures()
{
	boost::python::list names;
	map<string, DynamicLoader::CreateFresnelTexture> &pluginMap = DynamicLoader::registeredFresnelTextures();
	map<string, DynamicLoader::CreateFresnelTexture>::iterator mapit;
	for (mapit=pluginMap.begin(); mapit != pluginMap.end(); mapit++)
	{
		names.append( (*mapit).first );
	}

	return names;
};


boost::python::list py_getRegisteredCameras()
{
	boost::python::list names;
	map<string, DynamicLoader::CreateCamera> &pluginMap = DynamicLoader::registeredCameras();
	map<string, DynamicLoader::CreateCamera>::iterator mapit;
	for (mapit=pluginMap.begin(); mapit != pluginMap.end(); mapit++)
	{
		names.append( (*mapit).first );
	}

	return names;
};


boost::python::list py_getRegisteredPixelSamplers()
{
	boost::python::list names;
	map<string, DynamicLoader::CreatePixelSampler> &pluginMap = DynamicLoader::registeredPixelSamplers();
	map<string, DynamicLoader::CreatePixelSampler>::iterator mapit;
	for (mapit=pluginMap.begin(); mapit != pluginMap.end(); mapit++)
	{
		names.append( (*mapit).first );
	}

	return names;
};


boost::python::list py_getRegisteredToneMaps()
{
	boost::python::list names;
	map<string, DynamicLoader::CreateToneMap> &pluginMap = DynamicLoader::registeredToneMaps();
	map<string, DynamicLoader::CreateToneMap>::iterator mapit;
	for (mapit=pluginMap.begin(); mapit != pluginMap.end(); mapit++)
	{
		names.append( (*mapit).first );
	}

	return names;
};


boost::python::list py_getRegisteredShapes()
{
	boost::python::list names;
	map<string, DynamicLoader::CreateShape> &pluginMap = DynamicLoader::registeredShapes();
	map<string, DynamicLoader::CreateShape>::iterator mapit;
	for (mapit=pluginMap.begin(); mapit != pluginMap.end(); mapit++)
	{
		names.append( (*mapit).first );
	}

	return names;
};


boost::python::list py_getRegisteredLights()
{
	boost::python::list names;
	map<string, DynamicLoader::CreateLight> &pluginMap = DynamicLoader::registeredLights();
	map<string, DynamicLoader::CreateLight>::iterator mapit;
	for (mapit=pluginMap.begin(); mapit != pluginMap.end(); mapit++)
	{
		names.append( (*mapit).first );
	}

	return names;
};


boost::python::list py_getRegisteredMaterials()
{
	boost::python::list names;
	map<string, DynamicLoader::CreateMaterial> &pluginMap = DynamicLoader::registeredMaterials();
	map<string, DynamicLoader::CreateMaterial>::iterator mapit;
	for (mapit=pluginMap.begin(); mapit != pluginMap.end(); mapit++)
	{
		names.append( (*mapit).first );
	}

	return names;
};


boost::python::list py_getRegisteredFilms()
{
	boost::python::list names;
	map<string, DynamicLoader::CreateFilm> &pluginMap = DynamicLoader::registeredFilms();
	map<string, DynamicLoader::CreateFilm>::iterator mapit;
	for (mapit=pluginMap.begin(); mapit != pluginMap.end(); mapit++)
	{
		names.append( (*mapit).first );
	}

	return names;
};


boost::python::list py_getRegisteredVolumeIntegrators()
{
	boost::python::list names;
	map<string, DynamicLoader::CreateVolumeIntegrator> &pluginMap = DynamicLoader::registeredVolumeIntegrators();
	map<string, DynamicLoader::CreateVolumeIntegrator>::iterator mapit;
	for (mapit=pluginMap.begin(); mapit != pluginMap.end(); mapit++)
	{
		names.append( (*mapit).first );
	}

	return names;
};


boost::python::list py_getRegisteredVolumes()
{
	boost::python::list names;
	map<string, DynamicLoader::CreateVolume> &pluginMap = DynamicLoader::registeredVolumes();
	map<string, DynamicLoader::CreateVolume>::iterator mapit;
	for (mapit=pluginMap.begin(); mapit != pluginMap.end(); mapit++)
	{
		names.append( (*mapit).first );
	}

	return names;
};


boost::python::list py_getRegisteredSamplers()
{
	boost::python::list names;
	map<string, DynamicLoader::CreateSampler> &pluginMap = DynamicLoader::registeredSamplers();
	map<string, DynamicLoader::CreateSampler>::iterator mapit;
	for (mapit=pluginMap.begin(); mapit != pluginMap.end(); mapit++)
	{
		names.append( (*mapit).first );
	}

	return names;
};


boost::python::list py_getRegisteredAccelerators()
{
	boost::python::list names;
	map<string, DynamicLoader::CreateAccelerator> &pluginMap = DynamicLoader::registeredAccelerators();
	map<string, DynamicLoader::CreateAccelerator>::iterator mapit;
	for (mapit=pluginMap.begin(); mapit != pluginMap.end(); mapit++)
	{
		names.append( (*mapit).first );
	}

	return names;
};


boost::python::list py_getRegisteredAreaLights()
{
	boost::python::list names;
	map<string, DynamicLoader::CreateAreaLight> &pluginMap = DynamicLoader::registeredAreaLights();
	map<string, DynamicLoader::CreateAreaLight>::iterator mapit;
	for (mapit=pluginMap.begin(); mapit != pluginMap.end(); mapit++)
	{
		names.append( (*mapit).first );
	}

	return names;
};


boost::python::list py_getRegisteredSWCSpectrumTextures()
{
	boost::python::list names;
	map<string, DynamicLoader::CreateSWCSpectrumTexture> &pluginMap = DynamicLoader::registeredSWCSpectrumTextures();
	map<string, DynamicLoader::CreateSWCSpectrumTexture>::iterator mapit;
	for (mapit=pluginMap.begin(); mapit != pluginMap.end(); mapit++)
	{
		names.append( (*mapit).first );
	}

	return names;
};

void export_PyDynload()
{
	using namespace boost::python;
	using namespace lux;

	// Set up a fake module for these items
	object dynload(handle<>(borrowed(PyImport_AddModule("pylux.Dynload"))));
	scope().attr("Dynload") = dynload;
	scope dynloadScope = dynload;

	def("registeredVolumeRegions",
		py_getRegisteredVolumeRegions,
		"Return a list of registered VolumeRegion names"
	);


	def("registeredFloatTextures",
		py_getRegisteredFloatTextures,
		"Return a list of registered FloatTexture names"
	);


	def("registeredFilters",
		py_getRegisteredFilters,
		"Return a list of registered Filter names"
	);


	def("registeredSurfaceIntegrators",
		py_getRegisteredSurfaceIntegrators,
		"Return a list of registered SurfaceIntegrator names"
	);


	def("registeredRenderer",
		py_getRegisteredRenderer,
		"Return a list of registered Renderer names"
	);


	def("registeredFresnelTextures",
		py_getRegisteredFresnelTextures,
		"Return a list of registered FresnelTexture names"
	);


	def("registeredCameras",
		py_getRegisteredCameras,
		"Return a list of registered Camera names"
	);


	def("registeredPixelSamplers",
		py_getRegisteredPixelSamplers,
		"Return a list of registered PixelSampler names"
	);


	def("registeredToneMaps",
		py_getRegisteredToneMaps,
		"Return a list of registered ToneMap names"
	);


	def("registeredShapes",
		py_getRegisteredShapes,
		"Return a list of registered Shape names"
	);


	def("registeredLights",
		py_getRegisteredLights,
		"Return a list of registered Light names"
	);


	def("registeredMaterials",
		py_getRegisteredMaterials,
		"Return a list of registered Material names"
	);


	def("registeredFilms",
		py_getRegisteredFilms,
		"Return a list of registered Film names"
	);


	def("registeredVolumeIntegrators",
		py_getRegisteredVolumeIntegrators,
		"Return a list of registered VolumeIntegrator names"
	);


	def("registeredVolumes",
		py_getRegisteredVolumes,
		"Return a list of registered Volume names"
	);


	def("registeredSamplers",
		py_getRegisteredSamplers,
		"Return a list of registered Sampler names"
	);


	def("registeredAccelerators",
		py_getRegisteredAccelerators,
		"Return a list of registered Accelerator names"
	);


	def("registeredAreaLights",
		py_getRegisteredAreaLights,
		"Return a list of registered AreaLight names"
	);


	def("registeredSWCSpectrumTextures",
		py_getRegisteredSWCSpectrumTextures,
		"Return a list of registered SWCSpectrumTexture names"
	);

};

}

#endif	// LUX_PYFLEXIMAGE_H