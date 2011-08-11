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

#ifndef LUX_DYNLOAD_H
#define LUX_DYNLOAD_H

#include "api.h"
#include "lux.h"

#include <map>
#include <string>
using std::map;
using std::string;

namespace lux
{

// Runtime Loading Declarations
boost::shared_ptr<Shape> MakeShape(const string &name,
	const Transform &object2world, bool reverseOrientation,
	const ParamSet &paramSet);
boost::shared_ptr<Material> MakeMaterial(const string &name,
	const Transform &mtl2world, const ParamSet &mp);
boost::shared_ptr<Texture<float> > MakeFloatTexture(const string &name,
	const Transform &tex2world, const ParamSet &tp);
boost::shared_ptr<Texture<SWCSpectrum> > MakeSWCSpectrumTexture(const string &name,
	const Transform &tex2world, const ParamSet &tp);
boost::shared_ptr<Texture<FresnelGeneral> > MakeFresnelTexture(const string &name,
	const Transform &tex2world, const ParamSet &tp);
Light *MakeLight(const string &name, const Transform &light2world,
	const ParamSet &paramSet);
AreaLight *MakeAreaLight(const string &name,
	const Transform &light2world, const ParamSet &paramSet,
	const boost::shared_ptr<Primitive> &prim);
Region *MakeVolumeRegion(const string &name,
	const Transform &light2world, const ParamSet &paramSet);
boost::shared_ptr<Volume> MakeVolume(const string &name,
	const Transform &light2world, const ParamSet &paramSet);
SurfaceIntegrator *MakeSurfaceIntegrator(const string &name,
	const ParamSet &paramSet);
VolumeIntegrator *MakeVolumeIntegrator(const string &name,
	const ParamSet &paramSet);
boost::shared_ptr<Aggregate> MakeAccelerator(const string &name,
	const vector<boost::shared_ptr<Primitive> > &prims,
	const ParamSet &paramSet);
Camera *MakeCamera(const string &name, const Transform &world2cam,
	const Transform &world2camEnd, const ParamSet &paramSet, Film *film);
Sampler *MakeSampler(const string &name, const ParamSet &paramSet,
	const Film *film);
Filter *MakeFilter(const string &name, const ParamSet &paramSet);
ToneMap *MakeToneMap(const string &name, const ParamSet &paramSet);
Film *MakeFilm(const string &name, const ParamSet &paramSet, Filter *filt);
PixelSampler *MakePixelSampler(const string &name, int xstart, int xend, int ystart, int yend);
Renderer *MakeRenderer(const string &name, const ParamSet &paramSet);

class DynamicLoader {
	template <class T> class RegisterLoader {
	public:
		RegisterLoader<T>(map<string, T> &store, const string &name, T loader)
		{
			store[name] = loader;
		}
		virtual ~RegisterLoader<T>() {}
	};

public:
	typedef Shape *(*CreateShape)(const Transform&, bool, const ParamSet&);
	LUX_EXPORT static map<string, CreateShape> &registeredShapes();
	template <class T> class RegisterShape : public RegisterLoader<CreateShape> {
	public:
		RegisterShape<T>(const string &name) :
			RegisterLoader<CreateShape>(registeredShapes(), name, &T::CreateShape) {}
		virtual ~RegisterShape<T>() {}
	};

	typedef Material *(*CreateMaterial)(const Transform&,
		const ParamSet&);
	LUX_EXPORT static map<string, CreateMaterial> &registeredMaterials();
	template <class T> class RegisterMaterial : public RegisterLoader<CreateMaterial> {
	public:
		RegisterMaterial<T>(const string &name) :
			RegisterLoader<CreateMaterial>(registeredMaterials(), name, &T::CreateMaterial) {}
		virtual ~RegisterMaterial<T>() {}
	};

	typedef Texture<float> *(*CreateFloatTexture)(const Transform&,
		const ParamSet&);
	LUX_EXPORT static map<string, CreateFloatTexture> &registeredFloatTextures();
	template <class T> class RegisterFloatTexture : public RegisterLoader<CreateFloatTexture> {
	public:
		RegisterFloatTexture<T>(const string &name) :
			RegisterLoader<CreateFloatTexture>(registeredFloatTextures(), name, &T::CreateFloatTexture) {}
		virtual ~RegisterFloatTexture<T>() {}
	};

	typedef Texture<SWCSpectrum> *(*CreateSWCSpectrumTexture)(const Transform&,
		const ParamSet&);
	LUX_EXPORT static map<string, CreateSWCSpectrumTexture> &registeredSWCSpectrumTextures();
	template <class T> class RegisterSWCSpectrumTexture : public RegisterLoader<CreateSWCSpectrumTexture> {
	public:
		RegisterSWCSpectrumTexture<T>(const string &name) :
			RegisterLoader<CreateSWCSpectrumTexture>(registeredSWCSpectrumTextures(), name, &T::CreateSWCSpectrumTexture) {}
		virtual ~RegisterSWCSpectrumTexture<T>() {}
	};

	typedef Texture<FresnelGeneral> *(*CreateFresnelTexture)(const Transform&,
		const ParamSet&);
	LUX_EXPORT static map<string, CreateFresnelTexture> &registeredFresnelTextures();
	template <class T> class RegisterFresnelTexture : public RegisterLoader<CreateFresnelTexture> {
	public:
		RegisterFresnelTexture<T>(const string &name) :
			RegisterLoader<CreateFresnelTexture>(registeredFresnelTextures(), name, &T::CreateFresnelTexture) {}
		virtual ~RegisterFresnelTexture<T>() {}
	};

	typedef Light *(*CreateLight)(const Transform&, const ParamSet&);
	LUX_EXPORT static map<string, CreateLight> &registeredLights();
	template <class T> class RegisterLight : public RegisterLoader<CreateLight> {
	public:
		RegisterLight<T>(const string &name) :
			RegisterLoader<CreateLight>(registeredLights(), name, &T::CreateLight) {}
		virtual ~RegisterLight<T>() {}
	};

	typedef AreaLight *(*CreateAreaLight)(const Transform&, const ParamSet&,
		const boost::shared_ptr<Primitive>&);
	LUX_EXPORT static map<string, CreateAreaLight> &registeredAreaLights();
	template <class T> class RegisterAreaLight : public RegisterLoader<CreateAreaLight> {
	public:
		RegisterAreaLight<T>(const string &name) :
			RegisterLoader<CreateAreaLight>(registeredAreaLights(), name, &T::CreateAreaLight) {}
		virtual ~RegisterAreaLight<T>() {}
	};

	typedef Region *(*CreateVolumeRegion)(const Transform&,
		const ParamSet&);
	LUX_EXPORT static map<string, CreateVolumeRegion> &registeredVolumeRegions();
	template <class T> class RegisterVolumeRegion : public RegisterLoader<CreateVolumeRegion> {
	public:
		RegisterVolumeRegion<T>(const string &name) :
			RegisterLoader<CreateVolumeRegion>(registeredVolumeRegions(), name, &T::CreateVolumeRegion) {}
		virtual ~RegisterVolumeRegion<T>() {}
	};

	typedef Volume *(*CreateVolume)(const Transform&,
		const ParamSet&);
	LUX_EXPORT static map<string, CreateVolume> &registeredVolumes();
	template <class T> class RegisterVolume : public RegisterLoader<CreateVolume> {
	public:
		RegisterVolume<T>(const string &name) :
			RegisterLoader<CreateVolume>(registeredVolumes(), name, &T::CreateVolume) {}
		virtual ~RegisterVolume<T>() {}
	};

	typedef SurfaceIntegrator *(*CreateSurfaceIntegrator)(const ParamSet&);
	LUX_EXPORT static map<string, CreateSurfaceIntegrator> &registeredSurfaceIntegrators();
	template <class T> class RegisterSurfaceIntegrator : public RegisterLoader<CreateSurfaceIntegrator> {
	public:
		RegisterSurfaceIntegrator<T>(const string &name) :
			RegisterLoader<CreateSurfaceIntegrator>(registeredSurfaceIntegrators(), name, &T::CreateSurfaceIntegrator) {}
		virtual ~RegisterSurfaceIntegrator<T>() {}
	};

	typedef VolumeIntegrator *(*CreateVolumeIntegrator)(const ParamSet&);
	LUX_EXPORT static map<string, CreateVolumeIntegrator> &registeredVolumeIntegrators();
	template <class T> class RegisterVolumeIntegrator : public RegisterLoader<CreateVolumeIntegrator> {
	public:
		RegisterVolumeIntegrator<T>(const string &name) :
			RegisterLoader<CreateVolumeIntegrator>(registeredVolumeIntegrators(), name, &T::CreateVolumeIntegrator) {}
		virtual ~RegisterVolumeIntegrator<T>() {}
	};

	typedef Aggregate *(*CreateAccelerator)(const vector<boost::shared_ptr<Primitive> >&,
		const ParamSet&);
	LUX_EXPORT static map<string, CreateAccelerator> &registeredAccelerators();
	template <class T> class RegisterAccelerator : public RegisterLoader<CreateAccelerator> {
	public:
		RegisterAccelerator<T>(const string &name) :
			RegisterLoader<CreateAccelerator>(registeredAccelerators(), name, &T::CreateAccelerator) {}
		virtual ~RegisterAccelerator<T>() {}
	};

	typedef Camera *(*CreateCamera)(const Transform&, const Transform&, const ParamSet&,
		Film*);
	LUX_EXPORT static map<string, CreateCamera> &registeredCameras();
	template <class T> class RegisterCamera : public RegisterLoader<CreateCamera> {
	public:
		RegisterCamera<T>(const string &name) :
			RegisterLoader<CreateCamera>(registeredCameras(), name, &T::CreateCamera) {}
		virtual ~RegisterCamera<T>() {}
	};

	typedef Sampler *(*CreateSampler)(const ParamSet&, const Film*);
	LUX_EXPORT static map<string, CreateSampler> &registeredSamplers();
	template <class T> class RegisterSampler : public RegisterLoader<CreateSampler> {
	public:
		RegisterSampler<T>(const string &name) :
			RegisterLoader<CreateSampler>(registeredSamplers(), name, &T::CreateSampler) {}
		virtual ~RegisterSampler<T>() {}
	};

	typedef Filter *(*CreateFilter)(const ParamSet&);
	LUX_EXPORT static map<string, CreateFilter> &registeredFilters();
	template <class T> class RegisterFilter : public RegisterLoader<CreateFilter> {
	public:
		RegisterFilter<T>(const string &name) :
			RegisterLoader<CreateFilter>(registeredFilters(), name, &T::CreateFilter) {}
		virtual ~RegisterFilter<T>() {}
	};

	typedef ToneMap *(*CreateToneMap)(const ParamSet&);
	LUX_EXPORT static map<string, CreateToneMap> &registeredToneMaps();
	template <class T> class RegisterToneMap : public RegisterLoader<CreateToneMap> {
	public:
		RegisterToneMap<T>(const string &name) :
			RegisterLoader<CreateToneMap>(registeredToneMaps(), name, &T::CreateToneMap) {}
		virtual ~RegisterToneMap<T>() {}
	};

	typedef Film *(*CreateFilm)(const ParamSet&, Filter*);
	LUX_EXPORT static map<string, CreateFilm> &registeredFilms();
	template <class T> class RegisterFilm : public RegisterLoader<CreateFilm> {
	public:
		RegisterFilm<T>(const string &name) :
			RegisterLoader<CreateFilm>(registeredFilms(), name, &T::CreateFilm) {}
		virtual ~RegisterFilm<T>() {}
	};

	typedef PixelSampler *(*CreatePixelSampler)(int, int, int, int);
	LUX_EXPORT static map<string, CreatePixelSampler> &registeredPixelSamplers();
	template <class T> class RegisterPixelSampler : public RegisterLoader<CreatePixelSampler> {
	public:
		RegisterPixelSampler<T>(const string &name) :
			RegisterLoader<CreatePixelSampler>(registeredPixelSamplers(), name, &T::CreatePixelSampler) {}
		virtual ~RegisterPixelSampler<T>() {}
	};

	typedef Renderer *(*CreateRenderer)(const ParamSet&);
	LUX_EXPORT static map<string, CreateRenderer> &registeredRenderer();
	template <class T> class RegisterRenderer : public RegisterLoader<CreateRenderer> {
	public:
		RegisterRenderer<T>(const string &name) :
			RegisterLoader<CreateRenderer>(registeredRenderer(), name, &T::CreateRenderer) {}
		virtual ~RegisterRenderer<T>() {}
	};
};

}//namespace lux

#endif // LUX_DYNLOAD_H
