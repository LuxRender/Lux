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

// dynload.cpp*

#include "lux.h"
#include "dynload.h"
#include "paramset.h"
#include "error.h"
#include "shape.h"
#include "material.h"
#include "texture.h"

namespace lux {

static void LoadError(const string &type, const string &name)
{
	std::stringstream ss;
	ss << "Static loading of " << type << " '" << name << "' failed.";
	luxError(LUX_BUG, LUX_ERROR, ss.str().c_str());

}

boost::shared_ptr<Shape> MakeShape(const string &name,
	const Transform &object2world, bool reverseOrientation,
	const ParamSet &paramSet)
{
	if (DynamicLoader::registeredShapes().find(name) !=
		DynamicLoader::registeredShapes().end()) {
		boost::shared_ptr<Shape> ret(DynamicLoader::registeredShapes()[name](object2world, reverseOrientation, paramSet));
		paramSet.ReportUnused();
		return ret;
	}

	LoadError("shape", name);
	return boost::shared_ptr<Shape>();
}

boost::shared_ptr<Material> MakeMaterial(const string &name,
	const Transform &mtl2world, const TextureParams &mp)
{
	if (DynamicLoader::registeredMaterials().find(name) !=
		DynamicLoader::registeredMaterials().end()) {
		boost::shared_ptr<Material> ret(DynamicLoader::registeredMaterials()[name](mtl2world, mp));
		ret->InitGeneralParams(mp);
		mp.ReportUnused();
		return ret;
	}

	if (name != "")
		LoadError("material", name);
	return boost::shared_ptr<Material>();
}

boost::shared_ptr<Texture<float> > MakeFloatTexture(const string &name,
	const Transform &tex2world, const TextureParams &tp)
{
	if (DynamicLoader::registeredFloatTextures().find(name) !=
		DynamicLoader::registeredFloatTextures().end()) {
		boost::shared_ptr<Texture<float> > ret(DynamicLoader::registeredFloatTextures()[name](tex2world, tp));
		tp.ReportUnused();
		return ret;
	}

	LoadError("float texture", name);
	return boost::shared_ptr<Texture<float> >();
}

boost::shared_ptr<Texture<SWCSpectrum> > MakeSWCSpectrumTexture(const string &name,
	const Transform &tex2world, const TextureParams &tp)
{
	if (DynamicLoader::registeredSWCSpectrumTextures().find(name) !=
		DynamicLoader::registeredSWCSpectrumTextures().end()) {
		boost::shared_ptr<Texture<SWCSpectrum> > ret(DynamicLoader::registeredSWCSpectrumTextures()[name](tex2world, tp));
		tp.ReportUnused();
		return ret;
	}

	LoadError("color texture", name);
	return boost::shared_ptr<Texture<SWCSpectrum> >();
}

Light *MakeLight(const string &name,
	const Transform &light2world, const ParamSet &paramSet, const TextureParams &tp)
{
	if (DynamicLoader::registeredLights().find(name) !=
		DynamicLoader::registeredLights().end()) {
		Light *ret = DynamicLoader::registeredLights()[name](light2world,
			paramSet, tp);
		paramSet.ReportUnused();
		return ret;
	}

	LoadError("light", name);
	return NULL;
}

AreaLight *MakeAreaLight(const string &name,
	const Transform &light2world, const ParamSet &paramSet, const TextureParams &tp,
	const boost::shared_ptr<Primitive> &prim)
{
	if (DynamicLoader::registeredAreaLights().find(name) !=
		DynamicLoader::registeredAreaLights().end()) {
		AreaLight *ret =
			DynamicLoader::registeredAreaLights()[name](light2world,
				paramSet, tp, prim);
		paramSet.ReportUnused();
		return ret;
	}

	LoadError("area light", name);
	return NULL;
}

VolumeRegion *MakeVolumeRegion(const string &name,
	const Transform &volume2world, const ParamSet &paramSet)
{
	if (DynamicLoader::registeredVolumeRegions().find(name) !=
		DynamicLoader::registeredVolumeRegions().end()) {
		VolumeRegion *ret =
			DynamicLoader::registeredVolumeRegions()[name](volume2world,
				paramSet);
		paramSet.ReportUnused();
		return ret;
	}

	LoadError("volume region", name);
	return NULL;
}

SurfaceIntegrator *MakeSurfaceIntegrator(const string &name,
	const ParamSet &paramSet)
{
	if (DynamicLoader::registeredSurfaceIntegrators().find(name) !=
		DynamicLoader::registeredSurfaceIntegrators().end()) {
		SurfaceIntegrator *ret =
			DynamicLoader::registeredSurfaceIntegrators()[name](paramSet);
		paramSet.ReportUnused();
		return ret;
	}

	LoadError("surface integrator", name);
	return NULL;
}

VolumeIntegrator *MakeVolumeIntegrator(const string &name,
	const ParamSet &paramSet)
{
	if (DynamicLoader::registeredVolumeIntegrators().find(name) !=
		DynamicLoader::registeredVolumeIntegrators().end()) {
		VolumeIntegrator *ret =
			DynamicLoader::registeredVolumeIntegrators()[name](paramSet);
		paramSet.ReportUnused();
		return ret;
	}

	LoadError("volume integrator", name);
	return NULL;
}

boost::shared_ptr<Aggregate> MakeAccelerator(const string &name,
	const vector<boost::shared_ptr<Primitive> > &prims,
	const ParamSet &paramSet)
{
	if (DynamicLoader::registeredAccelerators().find(name) !=
		DynamicLoader::registeredAccelerators().end()) {
		boost::shared_ptr<Aggregate> ret(
			DynamicLoader::registeredAccelerators()[name](prims,
				paramSet));
		paramSet.ReportUnused();
		return ret;
	}

	LoadError("accelerator", name);
	return boost::shared_ptr<Aggregate>();
}

Camera *MakeCamera(const string &name,
	const Transform &world2cam, const Transform &world2camEnd, 
	const ParamSet &paramSet, Film *film)
{
	if (DynamicLoader::registeredCameras().find(name) !=
		DynamicLoader::registeredCameras().end()) {
		Camera *ret = DynamicLoader::registeredCameras()[name](world2cam,
			world2camEnd, paramSet, film);
		paramSet.ReportUnused();
		return ret;
	}

	LoadError("camera", name);
	return NULL;
}

Sampler *MakeSampler(const string &name,
	const ParamSet &paramSet, const Film *film)
{
	if (DynamicLoader::registeredSamplers().find(name) !=
		DynamicLoader::registeredSamplers().end()) {
		Sampler *ret = DynamicLoader::registeredSamplers()[name](paramSet,
			film);
		paramSet.ReportUnused();
		return ret;
	}

	LoadError("sampler", name);
	return NULL;
}

Filter *MakeFilter(const string &name,
	const ParamSet &paramSet)
{
	if (DynamicLoader::registeredFilters().find(name) !=
		DynamicLoader::registeredFilters().end()) {
		Filter *ret = DynamicLoader::registeredFilters()[name](paramSet);
		paramSet.ReportUnused();
		return ret;
	}

	LoadError("filter", name);
	return NULL;
}

ToneMap *MakeToneMap(const string &name,
	const ParamSet &paramSet)
{
	if (DynamicLoader::registeredToneMaps().find(name) !=
		DynamicLoader::registeredToneMaps().end()) {
		ToneMap *ret = DynamicLoader::registeredToneMaps()[name](paramSet);
		paramSet.ReportUnused();
		return ret;
	}

	LoadError("tonemap", name);
	return NULL;
}

Film *MakeFilm(const string &name,
	const ParamSet &paramSet, Filter *filter)
{
	if (DynamicLoader::registeredFilms().find(name) !=
		DynamicLoader::registeredFilms().end()) {
		Film *ret = DynamicLoader::registeredFilms()[name](paramSet,
			filter);
		paramSet.ReportUnused();
		return ret;
	}

	LoadError("film", name);
	return NULL;
}

PixelSampler *MakePixelSampler(const string &name,
	const ParamSet &paramSet)
{
	if (DynamicLoader::registeredPixelSamplers().find(name) !=
		DynamicLoader::registeredPixelSamplers().end()) {
		PixelSampler *ret = DynamicLoader::registeredPixelSamplers()[name](paramSet);
		paramSet.ReportUnused();
		return ret;
	}

	LoadError("pixel sampler", name);
	return NULL;
}

map<string, DynamicLoader::CreateShape> &DynamicLoader::registeredShapes()
{
	static map<string, DynamicLoader::CreateShape> *Map = new map<string, DynamicLoader::CreateShape>;
	return *Map;
}
map<string, DynamicLoader::CreateMaterial> &DynamicLoader::registeredMaterials()
{
	static map<string, DynamicLoader::CreateMaterial> *Map = new map<string, DynamicLoader::CreateMaterial>;
	return *Map;
}
map<string, DynamicLoader::CreateFloatTexture> &DynamicLoader::registeredFloatTextures()
{
	static map<string, DynamicLoader::CreateFloatTexture> *Map = new map<string, DynamicLoader::CreateFloatTexture>;
	return *Map;
}
map<string, DynamicLoader::CreateSWCSpectrumTexture> &DynamicLoader::registeredSWCSpectrumTextures()
{
	static map<string, DynamicLoader::CreateSWCSpectrumTexture> *Map = new map<string, DynamicLoader::CreateSWCSpectrumTexture>;
	return *Map;
}
map<string, DynamicLoader::CreateLight> &DynamicLoader::registeredLights()
{
	static map<string, DynamicLoader::CreateLight> *Map = new map<string, DynamicLoader::CreateLight>;
	return *Map;
}
map<string, DynamicLoader::CreateAreaLight> &DynamicLoader::registeredAreaLights()
{
	static map<string, DynamicLoader::CreateAreaLight> *Map = new map<string, DynamicLoader::CreateAreaLight>;
	return *Map;
}
map<string, DynamicLoader::CreateVolumeRegion> &DynamicLoader::registeredVolumeRegions()
{
	static map<string, DynamicLoader::CreateVolumeRegion> *Map = new map<string, DynamicLoader::CreateVolumeRegion>;
	return *Map;
}
map<string, DynamicLoader::CreateSurfaceIntegrator> &DynamicLoader::registeredSurfaceIntegrators()
{
	static map<string, DynamicLoader::CreateSurfaceIntegrator> *Map = new map<string, DynamicLoader::CreateSurfaceIntegrator>;
	return *Map;
}
map<string, DynamicLoader::CreateVolumeIntegrator> &DynamicLoader::registeredVolumeIntegrators()
{
	static map<string, DynamicLoader::CreateVolumeIntegrator> *Map = new map<string, DynamicLoader::CreateVolumeIntegrator>;
	return *Map;
}
map<string, DynamicLoader::CreateAccelerator> &DynamicLoader::registeredAccelerators()
{
	static map<string, DynamicLoader::CreateAccelerator> *Map = new map<string, DynamicLoader::CreateAccelerator>;
	return *Map;
}
map<string, DynamicLoader::CreateCamera> &DynamicLoader::registeredCameras()
{
	static map<string, DynamicLoader::CreateCamera> *Map = new map<string, DynamicLoader::CreateCamera>;
	return *Map;
}
map<string, DynamicLoader::CreateSampler> &DynamicLoader::registeredSamplers()
{
	static map<string, DynamicLoader::CreateSampler> *Map = new map<string, DynamicLoader::CreateSampler>;
	return *Map;
}
map<string, DynamicLoader::CreateFilter> &DynamicLoader::registeredFilters()
{
	static map<string, DynamicLoader::CreateFilter> *Map = new map<string, DynamicLoader::CreateFilter>;
	return *Map;
}
map<string, DynamicLoader::CreateToneMap> &DynamicLoader::registeredToneMaps()
{
	static map<string, DynamicLoader::CreateToneMap> *Map = new map<string, DynamicLoader::CreateToneMap>;
	return *Map;
}
map<string, DynamicLoader::CreateFilm> &DynamicLoader::registeredFilms()
{
	static map<string, DynamicLoader::CreateFilm> *Map = new map<string, DynamicLoader::CreateFilm>;
	return *Map;
}
map<string, DynamicLoader::CreatePixelSampler> &DynamicLoader::registeredPixelSamplers()
{
	static map<string, DynamicLoader::CreatePixelSampler> *Map = new map<string, DynamicLoader::CreatePixelSampler>;
	return *Map;
}

}//namespace lux

