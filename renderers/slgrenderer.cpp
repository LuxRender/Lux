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

#include <iomanip>
#include <fstream>
#include <typeinfo>
#include <sstream>

#include <boost/lexical_cast.hpp>
#include <boost/regex.hpp>

#include "api.h"
#include "scene.h"
#include "camera.h"
#include "film.h"
#include "sampling.h"
#include "context.h"
#include "slgrenderer.h"
#include "renderers/statistics/slgstatistics.h"
#include "cameras/perspective.h"
#include "shape.h"
#include "volume.h"
#include "filter.h"

#include "samplers/metrosampler.h"
#include "samplers/random.h"

#include "integrators/path.h"
#include "integrators/bidirectional.h"

#include "textures/blackbody.h"
#include "textures/constant.h"
#include "textures/imagemap.h"
#include "textures/scale.h"

#include "light.h"
#include "lights/sun.h"
#include "lights/sky.h"
#include "lights/sky2.h"
#include "lights/infinite.h"
#include "lights/infinitesample.h"

#include "materials/matte.h"
#include "materials/mirror.h"
#include "materials/glass.h"
#include "materials/glass2.h"
#include "materials/glossy2.h"
#include "materials/metal.h"

#include "volumes/clearvolume.h"

#include "luxrays/core/context.h"
#include "luxrays/utils/core/exttrianglemesh.h"
#include "luxrays/opencl/utils.h"
#include "luxrays/opencl/utils.h"
#include "rendersession.h"

using namespace lux;

template <class T> static string ToClassName(T *ptr) {
	if (ptr)
		return typeid(*ptr).name();
	else
		return "NULL";
}

template <class T> static string ToString(T &v) {
	return boost::lexical_cast<string>(v);
}

static string ToString(float v) {
	std::ostringstream ss;
    ss << std::setprecision(24) << v;

	return ss.str();
}

//------------------------------------------------------------------------------
// SLGHostDescription
//------------------------------------------------------------------------------

SLGHostDescription::SLGHostDescription(SLGRenderer *r, const string &n) : renderer(r), name(n) {
	SLGDeviceDescription *desc = new SLGDeviceDescription(this, "Test");
	devs.push_back(desc);
}

SLGHostDescription::~SLGHostDescription() {
	for (size_t i = 0; i < devs.size(); ++i)
		delete devs[i];
}

void SLGHostDescription::AddDevice(SLGDeviceDescription *devDesc) {
	devs.push_back(devDesc);
}

//------------------------------------------------------------------------------
// SLGRenderer
//------------------------------------------------------------------------------

SLGRenderer::SLGRenderer(const luxrays::Properties &config) : Renderer() {
	state = INIT;

	SLGHostDescription *host = new SLGHostDescription(this, "Localhost");
	hosts.push_back(host);

	preprocessDone = false;
	suspendThreadsWhenDone = false;

	previousEyeBufferRadiance = NULL;
	previousEyeWeight = NULL;
	previousLightBufferRadiance = NULL;
	previousLightWeight = NULL;
	previousAlphaBuffer = NULL;

	AddStringConstant(*this, "name", "Name of current renderer", "slg");

	rendererStatistics = new SLGStatistics(this);

	overwriteConfig = config;
}

SLGRenderer::~SLGRenderer() {
	boost::mutex::scoped_lock lock(classWideMutex);

	if ((state != TERMINATE) && (state != INIT))
		throw std::runtime_error("Internal error: called SLGRenderer::~SLGRenderer() while not in TERMINATE or INIT state.");

	delete rendererStatistics;
	delete previousEyeBufferRadiance;
	delete previousEyeWeight;
	delete previousLightBufferRadiance;
	delete previousLightWeight;
	delete previousAlphaBuffer;

	for (size_t i = 0; i < hosts.size(); ++i)
		delete hosts[i];
}

Renderer::RendererState SLGRenderer::GetState() const {
	boost::mutex::scoped_lock lock(classWideMutex);

	return state;
}

vector<RendererHostDescription *> &SLGRenderer::GetHostDescs() {
	boost::mutex::scoped_lock lock(classWideMutex);

	return hosts;
}

void SLGRenderer::SuspendWhenDone(bool v) {
	boost::mutex::scoped_lock lock(classWideMutex);
	suspendThreadsWhenDone = v;
}

//------------------------------------------------------------------------------
// Channels: integer types
//------------------------------------------------------------------------------

template <typename T, u_int channels> string GetSLGImageMapNameImpl(luxrays::sdl::Scene *slgScene,
		const MIPMapFastImpl<TextureColor<T, channels> > *mipMap,
		const float gamma) {
	// Check if the image map has already been defined
	const string imageMapName = mipMap->GetName();
	if (slgScene->imgMapCache.IsImageMapDefined(imageMapName))
		return imageMapName;

	const BlockedArray<TextureColor<T, channels> > *map = mipMap->GetSingleMap();

	float *slgMap = new float[map->uSize() * map->vSize() * channels];
	float *mapPtr = slgMap;
	for (u_int y = 0; y < map->vSize(); ++y) {
		for (u_int x = 0; x < map->uSize(); ++x) {
			const TextureColor<T, channels> &col = (*map)(x, y);

			for (u_int i = 0; i < channels; ++i)
				*mapPtr++ = col.c[i] / 255.f;
		}
	}

	luxrays::sdl::ImageMap *imageMap = new luxrays::sdl::ImageMap(slgMap, gamma, channels, (u_int)map->uSize(), (u_int)map->vSize());
	slgScene->DefineImageMap(imageMapName, imageMap);

	return imageMapName;
}

//------------------------------------------------------------------------------
// Channels: floats
// 
// Partial function specialization are not allowed in C++
//------------------------------------------------------------------------------

template <> string GetSLGImageMapNameImpl<float, 1>(luxrays::sdl::Scene *slgScene,
		const MIPMapFastImpl<TextureColor<float, 1> > *mipMap,
		const float gamma) {
	// Check if the image map has already been defined
	const string imageMapName = mipMap->GetName();
	if (slgScene->imgMapCache.IsImageMapDefined(imageMapName))
		return imageMapName;

	const BlockedArray<TextureColor<float, 1> > *map = mipMap->GetSingleMap();

	float *slgMap = new float[map->uSize() * map->vSize() * 1];
	float *mapPtr = slgMap;
	for (u_int y = 0; y < map->vSize(); ++y) {
		for (u_int x = 0; x < map->uSize(); ++x) {
			const TextureColor<float, 1> &col = (*map)(x, y);

			*mapPtr++ = col.c[0];
		}
	}

	luxrays::sdl::ImageMap *imageMap = new luxrays::sdl::ImageMap(slgMap, gamma, 1, (u_int)map->uSize(), (u_int)map->vSize());
	slgScene->DefineImageMap(imageMapName, imageMap);

	return imageMapName;
}

template <> string GetSLGImageMapNameImpl<float, 3>(luxrays::sdl::Scene *slgScene,
		const MIPMapFastImpl<TextureColor<float, 3> > *mipMap,
		const float gamma) {
	// Check if the image map has already been defined
	const string imageMapName = mipMap->GetName();
	if (slgScene->imgMapCache.IsImageMapDefined(imageMapName))
		return imageMapName;

	const BlockedArray<TextureColor<float, 3> > *map = mipMap->GetSingleMap();

	float *slgMap = new float[map->uSize() * map->vSize() * 3];
	float *mapPtr = slgMap;
	for (u_int y = 0; y < map->vSize(); ++y) {
		for (u_int x = 0; x < map->uSize(); ++x) {
			const TextureColor<float, 3> &col = (*map)(x, y);

			*mapPtr++ = col.c[0];
			*mapPtr++ = col.c[1];
			*mapPtr++ = col.c[2];
		}
	}

	luxrays::sdl::ImageMap *imageMap = new luxrays::sdl::ImageMap(slgMap, gamma, 3, (u_int)map->uSize(), (u_int)map->vSize());
	slgScene->DefineImageMap(imageMapName, imageMap);

	return imageMapName;
}

template <> string GetSLGImageMapNameImpl<float, 4>(luxrays::sdl::Scene *slgScene,
		const MIPMapFastImpl<TextureColor<float, 4> > *mipMap,
		const float gamma) {
	// Check if the image map has already been defined
	const string imageMapName = mipMap->GetName();
	if (slgScene->imgMapCache.IsImageMapDefined(imageMapName))
		return imageMapName;

	const BlockedArray<TextureColor<float, 4> > *map = mipMap->GetSingleMap();

	float *slgMap = new float[map->uSize() * map->vSize() * 4];
	float *mapPtr = slgMap;
	for (u_int y = 0; y < map->vSize(); ++y) {
		for (u_int x = 0; x < map->uSize(); ++x) {
			const TextureColor<float, 4> &col = (*map)(x, y);

			*mapPtr++ = col.c[0];
			*mapPtr++ = col.c[1];
			*mapPtr++ = col.c[2];
			*mapPtr++ = col.c[3];
		}
	}

	luxrays::sdl::ImageMap *imageMap = new luxrays::sdl::ImageMap(slgMap, gamma, 4, (u_int)map->uSize(), (u_int)map->vSize());
	slgScene->DefineImageMap(imageMapName, imageMap);

	return imageMapName;
}

//------------------------------------------------------------------------------

static string GetSLGDefaultImageMap(luxrays::sdl::Scene *slgScene) {
	if (!slgScene->imgMapCache.IsImageMapDefined("imagemap_default")) {
		float *map = new float[1];
		map[0] = 1.f;
		luxrays::sdl::ImageMap *imageMap = new luxrays::sdl::ImageMap(map, 1.f, 1, 1, 1);
		slgScene->DefineImageMap("imagemap_default", imageMap);
	}

	return "imagemap_default";
}

string GetSLGImageMapName(luxrays::sdl::Scene *slgScene,
		const MIPMap *mipMap, const float gamma) {
	if (!mipMap)
		return GetSLGDefaultImageMap(slgScene);

	//--------------------------------------------------------------------------
	// Channels: unsigned char
	//--------------------------------------------------------------------------
	if (dynamic_cast<const MIPMapFastImpl<TextureColor<unsigned char, 1> > *>(mipMap))
		return GetSLGImageMapNameImpl(slgScene, (MIPMapImpl<TextureColor<unsigned char, 1> > *)mipMap, gamma);
	if (dynamic_cast<const MIPMapFastImpl<TextureColor<unsigned char, 3> > *>(mipMap))
		return GetSLGImageMapNameImpl(slgScene, (MIPMapImpl<TextureColor<unsigned char, 3> > *)mipMap, gamma);
	if (dynamic_cast<const MIPMapFastImpl<TextureColor<unsigned char, 4> > *>(mipMap))
		return GetSLGImageMapNameImpl(slgScene, (MIPMapImpl<TextureColor<unsigned char, 4> > *)mipMap, gamma);
	//--------------------------------------------------------------------------
	// Channels: unsigned short
	//--------------------------------------------------------------------------
	if (dynamic_cast<const MIPMapFastImpl<TextureColor<unsigned short, 1> > *>(mipMap))
		return GetSLGImageMapNameImpl(slgScene, (MIPMapImpl<TextureColor<unsigned short, 1> > *)mipMap, gamma);
	if (dynamic_cast<const MIPMapFastImpl<TextureColor<unsigned short, 3> > *>(mipMap))
		return GetSLGImageMapNameImpl(slgScene, (MIPMapImpl<TextureColor<unsigned short, 3> > *)mipMap, gamma);
	if (dynamic_cast<const MIPMapFastImpl<TextureColor<unsigned short, 4> > *>(mipMap))
		return GetSLGImageMapNameImpl(slgScene, (MIPMapImpl<TextureColor<unsigned short, 4> > *)mipMap, gamma);
	//--------------------------------------------------------------------------
	// Channels: float
	//--------------------------------------------------------------------------
	else if (dynamic_cast<const MIPMapFastImpl<TextureColor<float, 1> > *>(mipMap))
		return GetSLGImageMapNameImpl(slgScene, (MIPMapImpl<TextureColor<float, 1> > *)mipMap, gamma);
	else if (dynamic_cast<const MIPMapFastImpl<TextureColor<float, 3> > *>(mipMap))
		return GetSLGImageMapNameImpl(slgScene, (MIPMapImpl<TextureColor<float, 3> > *)mipMap, gamma);
	else if (dynamic_cast<const MIPMapFastImpl<TextureColor<float, 4> > *>(mipMap))
		return GetSLGImageMapNameImpl(slgScene, (MIPMapImpl<TextureColor<float, 4> > *)mipMap, gamma);
	else {
		// Unsupported type
		LOG(LUX_WARNING, LUX_UNIMPLEMENT) << "SLGRenderer supports only RGB(A) float texture maps (i.e. not " <<
					ToClassName(mipMap) << "). Replacing an unsupported texture map with a white texture.";
		return GetSLGDefaultImageMap(slgScene);
	}
}

//------------------------------------------------------------------------------

//bool SLGRenderer::GetSLGBumpNormalMapInfo(luxrays::sdl::Scene *slgScene, SLGMaterialInfo *matInfo,
//		const Texture<float> *tex) {
//	LOG(LUX_DEBUG, LUX_NOERROR) << "Bump/Normal map type: " << ToClassName(tex);
//
//	if (!tex)
//		return true;
//
//	if (dynamic_cast<const ScaleTexture<float, float> *>(tex)) {
//		// ScaleTexture<float, float> is for bump mapping
//		const ScaleTexture<float, float> *scaleTex = dynamic_cast<const ScaleTexture<float, float> *>(tex);
//
//		LOG(LUX_DEBUG, LUX_NOERROR) << "Bump map scale type: " << ToClassName(scaleTex->GetTex1());
//		const ConstantFloatTexture *constFloatTex = dynamic_cast<const ConstantFloatTexture *>(scaleTex->GetTex1());
//		LOG(LUX_DEBUG, LUX_NOERROR) << "Bump map bump type: " << ToClassName(scaleTex->GetTex2());
//		const ImageFloatTexture *imgTex = dynamic_cast<const ImageFloatTexture *>(scaleTex->GetTex2());
//
//		if (constFloatTex && imgTex) {
//			// Check the mapping
//			const TextureMapping2D *mapping = imgTex->GetTextureMapping2D();
//			if (mapping) {
//				if (dynamic_cast<const UVMapping2D *>(mapping)) {
//					const UVMapping2D *uvMapping2D = dynamic_cast<const UVMapping2D *>(mapping);
//					matInfo->bumpMap.uScale = uvMapping2D->GetUScale();
//					matInfo->bumpMap.vScale = uvMapping2D->GetVScale();
//					matInfo->bumpMap.uDelta = uvMapping2D->GetUDelta();
//					matInfo->bumpMap.vDelta = uvMapping2D->GetVDelta();
//				} else {
//					LOG(LUX_WARNING, LUX_UNIMPLEMENT) << "SLGRenderer supports only bump maps with UVMapping2D (i.e. not " <<
//							ToClassName(mapping) << "). Ignoring the mapping.";				
//				}
//			}
//
//			matInfo->bumpMap.name = GetSLGImageMapName(slgScene, imgTex->GetMIPMap(), imgTex->GetInfo().gamma, false);
//			matInfo->bumpMap.scale = 500.f * (*constFloatTex)["value"].FloatValue();
//			return true;
//		} else {
//			LOG(LUX_WARNING, LUX_UNIMPLEMENT) << "SLGRenderer supports only bump mapping with ScaleTexture<float, float> of ConstantFloatTexture and ImageFloatTexture (i.e. not " <<
//				ToClassName(scaleTex->GetTex1()) << " and " << ToClassName(scaleTex->GetTex2()) << ").";
//			return false;
//		}
//	} else if (dynamic_cast<const NormalMapTexture *>(tex)) {
//		// NormalMapTexture is for normal mapping
//		const NormalMapTexture *normalTex = dynamic_cast<const NormalMapTexture *>(tex);
//
//		LOG(LUX_DEBUG, LUX_NOERROR) << "Normal map type: " << ToClassName(normalTex);
//
//		if (normalTex) {
//			// Check the mapping
//			const TextureMapping2D *mapping = normalTex->GetTextureMapping2D();
//			if (mapping) {
//				if (dynamic_cast<const UVMapping2D *>(mapping)) {
//					const UVMapping2D *uvMapping2D = dynamic_cast<const UVMapping2D *>(mapping);
//					matInfo->normalMap.uScale = uvMapping2D->GetUScale();
//					matInfo->normalMap.vScale = uvMapping2D->GetVScale();
//					matInfo->normalMap.uDelta = uvMapping2D->GetUDelta();
//					matInfo->normalMap.vDelta = uvMapping2D->GetVDelta();
//				} else {
//					LOG(LUX_WARNING, LUX_UNIMPLEMENT) << "SLGRenderer supports only normal maps with UVMapping2D (i.e. not " <<
//							ToClassName(mapping) << "). Ignoring the mapping.";				
//				}
//			}
//
//			matInfo->normalMap.name = GetSLGImageMapName(slgScene, normalTex->GetMIPMap(), normalTex->GetInfo().gamma, false);
//			return true;
//		} else {
//			LOG(LUX_WARNING, LUX_UNIMPLEMENT) << "SLGRenderer supports only normal mapping with NormalMapTexture (i.e. not " <<
//				ToClassName(normalTex) << ").";
//			return false;
//		}
//	}
//
//	LOG(LUX_WARNING, LUX_UNIMPLEMENT) << "SLGRenderer supports only bump mapping with ScaleTexture<float, float> and normal mapping with NormalMapTexture (i.e. not " <<
//		ToClassName(tex) << ").";
//
//	return false;
//}

template<class T> string GetSLGTexName(luxrays::sdl::Scene *slgScene,
		const Texture<T> *tex) {
	LOG(LUX_DEBUG, LUX_NOERROR) << "Texture type: " << ToClassName(tex);

	const string texName = tex->GetName();
	// Check if the texture has already been defined
	if (!slgScene->texDefs.IsTextureDefined(texName)) {
		if (dynamic_cast<const ImageSpectrumTexture *>(tex)) {
			const ImageSpectrumTexture *imgTex = dynamic_cast<const ImageSpectrumTexture *>(tex);

			float uScale = 1.f;
			float vScale = 1.f;
			float uDelta = 0.f;
			float vDelta = 0.f;

			// Check the mapping
			const TextureMapping2D *mapping = imgTex->GetTextureMapping2D();
			if (mapping) {
				if (dynamic_cast<const UVMapping2D *>(mapping)) {
					const UVMapping2D *uvMapping2D = dynamic_cast<const UVMapping2D *>(mapping);
					uScale = uvMapping2D->GetUScale();
					vScale = uvMapping2D->GetVScale();
					uDelta = uvMapping2D->GetUDelta();
					vDelta = uvMapping2D->GetVDelta();
				} else {
					LOG(LUX_WARNING, LUX_UNIMPLEMENT) << "SLGRenderer supports only image maps with UVMapping2D (i.e. not " <<
							ToClassName(mapping) << "). Ignoring the mapping.";				
				}
			}

			const TexInfo &texInfo = imgTex->GetInfo();
			const string imageMapName = GetSLGImageMapName(slgScene, imgTex->GetMIPMap(), texInfo.gamma);

			slgScene->DefineTextures(
					"scene.textures." + texName + ".type = imagemap\n"
					"scene.textures." + texName + ".file = " + imageMapName + "\n"
					"scene.textures." + texName + ".gamma = " + ToString(texInfo.gamma) + "\n"
					"scene.textures." + texName + ".gain = " + ToString(texInfo.gain) + "\n"
					"scene.textures." + texName + ".uvscale = " +
						ToString(uScale) + " " +
						ToString(vScale) + "\n"
					"scene.textures." + texName + ".uvdelta = " +
						ToString(uDelta) + " " +
						ToString(vDelta) + "\n"

					);
		} else if (dynamic_cast<const ImageFloatTexture *>(tex)) {
			const ImageFloatTexture *imgTex = dynamic_cast<const ImageFloatTexture *>(tex);

			float uScale = 1.f;
			float vScale = 1.f;
			float uDelta = 0.f;
			float vDelta = 0.f;

			// Check the mapping
			const TextureMapping2D *mapping = imgTex->GetTextureMapping2D();
			if (mapping) {
				if (dynamic_cast<const UVMapping2D *>(mapping)) {
					const UVMapping2D *uvMapping2D = dynamic_cast<const UVMapping2D *>(mapping);
					uScale = uvMapping2D->GetUScale();
					vScale = uvMapping2D->GetVScale();
					uDelta = uvMapping2D->GetUDelta();
					vDelta = uvMapping2D->GetVDelta();
				} else {
					LOG(LUX_WARNING, LUX_UNIMPLEMENT) << "SLGRenderer supports only image maps with UVMapping2D (i.e. not " <<
							ToClassName(mapping) << "). Ignoring the mapping.";				
				}
			}

			const TexInfo &texInfo = imgTex->GetInfo();
			const string imageMapName = GetSLGImageMapName(slgScene, imgTex->GetMIPMap(), texInfo.gamma);

			slgScene->DefineTextures(
					"scene.textures." + texName + ".type = imagemap\n"
					"scene.textures." + texName + ".file = " + imageMapName + "\n"
					"scene.textures." + texName + ".gamma = " + ToString(texInfo.gamma) + "\n"
					"scene.textures." + texName + ".gain = " + ToString(texInfo.gain) + "\n"
					"scene.textures." + texName + ".uvscale = " +
						ToString(uScale) + " " +
						ToString(vScale) + "\n"
					"scene.textures." + texName + ".uvdelta = " +
						ToString(uDelta) + " " +
						ToString(vDelta) + "\n"

					);
		} else if (dynamic_cast<const ConstantRGBColorTexture *>(tex)) {
			const ConstantRGBColorTexture *constRGBTex = dynamic_cast<const ConstantRGBColorTexture *>(tex);

			slgScene->DefineTextures(
					"scene.textures." + texName + ".type = constfloat3\n"
					"scene.textures." + texName + ".value = " +
						ToString((*constRGBTex)["color.r"].FloatValue()) + " " +
						ToString((*constRGBTex)["color.g"].FloatValue()) + " " +
						ToString((*constRGBTex)["color.b"].FloatValue()) + "\n"
					);
		} else if (dynamic_cast<const ConstantFloatTexture *>(tex)) {
			const ConstantFloatTexture *constFloatTex = dynamic_cast<const ConstantFloatTexture *>(tex);

			slgScene->DefineTextures(
					"scene.textures." + texName + ".type = constfloat1\n"
					"scene.textures." + texName + ".value = " +
						ToString((*constFloatTex)["value"].FloatValue()) + "\n"
					);
		} else {
			LOG(LUX_WARNING, LUX_UNIMPLEMENT) << "SLGRenderer supports only materials with ImageSpectrumTexture, ImageFloatTexture, ConstantRGBColorTexture or ConstantFloatTexture (i.e. not " <<
					ToClassName(tex) << ").";

			slgScene->DefineTextures(
					"scene.textures." + texName + ".type = constfloat1\n"
					"scene.textures." + texName + ".value = 1.0\n"
					);
		}
	}

	return texName;
}

string GetSLGMaterialName(luxrays::sdl::Scene *slgScene, const Primitive *prim) {
	LOG(LUX_DEBUG, LUX_NOERROR) << "Primitive type: " << ToClassName(prim);
	
	Material *mat = NULL;
	string matName = "mat_default";
	string emissionTexName = "0.0 0.0 0.0";

	//----------------------------------------------------------------------
	// Check if it is a Shape
	//----------------------------------------------------------------------
	if (dynamic_cast<const Shape *>(prim)) {
		const Shape *shape = dynamic_cast<const Shape *>(prim);
		mat = shape->GetMaterial();
	} else
	//----------------------------------------------------------------------
	// Check if it is an InstancePrimitive
	//----------------------------------------------------------------------
	if (dynamic_cast<const InstancePrimitive *>(prim)) {
		const InstancePrimitive *instance = dynamic_cast<const InstancePrimitive *>(prim);
		mat = instance->GetMaterial();
	} else
	//----------------------------------------------------------------------
	// Check if it is an AreaLight
	//----------------------------------------------------------------------
	if (dynamic_cast<const AreaLightPrimitive *>(prim)) {
		const AreaLightPrimitive *alPrim = dynamic_cast<const AreaLightPrimitive *>(prim);
		AreaLight *al = alPrim->GetAreaLight();

		Texture<SWCSpectrum> *tex = al->GetTexture();

		const float gain = (*al)["gain"].FloatValue();
		const float power = (*al)["power"].FloatValue();
		const float efficacy = (*al)["efficacy"].FloatValue();
		const float area = (*al)["area"].FloatValue();

		// Check the type of texture used
		LOG(LUX_DEBUG, LUX_NOERROR) << "AreaLight texture type: " << ToClassName(tex);
		ConstantRGBColorTexture *constRGBTex = dynamic_cast<ConstantRGBColorTexture *>(tex);
		BlackBodyTexture *blackBodyTexture = dynamic_cast<BlackBodyTexture *>(tex);

		luxrays::Spectrum emission;
		if (constRGBTex) {
			emission = luxrays::Spectrum(
					(*constRGBTex)["color.r"].FloatValue(),
					(*constRGBTex)["color.g"].FloatValue(),
					(*constRGBTex)["color.b"].FloatValue());

			const float gainFactor = power * efficacy /
				(area * M_PI * emission.Y());
			if (gainFactor > 0.f && !isinf(gainFactor))
				emission *= gain * gainFactor;
			else
				emission *= gain;
		} else if (blackBodyTexture) {
			luxrays::Spectrum emission(1.f);

			const float gainFactor = power * efficacy;
			if (gainFactor > 0.f && !isinf(gainFactor))
				emission *= gain * gainFactor;
			else
				emission *= gain;
		} else {
			LOG(LUX_WARNING, LUX_UNIMPLEMENT) << "SLGRenderer supports only area lights with constant ConstantRGBColorTexture or BlackBodyTexture (i.e. not " <<
				ToClassName(tex) << "). Ignoring emission of unsupported area light.";
		}

		emissionTexName = ToString(emission.r) + " " + ToString(emission.g) + " " + ToString(emission.b);

		const Primitive *p = alPrim->GetPrimitive().get();
		if (dynamic_cast<const Shape *>(p)) {
			const Shape *shape = dynamic_cast<const Shape *>(p);
			mat = shape->GetMaterial();
		} else if (dynamic_cast<const InstancePrimitive *>(p)) {
			const InstancePrimitive *instance = dynamic_cast<const InstancePrimitive *>(p);
			mat = instance->GetMaterial();
		} else {
			LOG(LUX_WARNING, LUX_UNIMPLEMENT) << "SLGRenderer doesn't support material conversion for area light primitive " << ToClassName(prim);
			return "mat_default";
		}
	} else
	//----------------------------------------------------------------------
	// Primitive is not supported, use the default material
	//----------------------------------------------------------------------
	{
		LOG(LUX_WARNING, LUX_UNIMPLEMENT) << "SLGRenderer doesn't support material conversion for primitive " << ToClassName(prim);
		return "mat_default";
	}

	if (!mat)
		return "mat_default";

	// Retrieve bump/normal mapping information too
//	if (mat)
//		GetSLGBumpNormalMapInfo(slgScene, &matInfo, mat->bumpMap.get());

	LOG(LUX_DEBUG, LUX_NOERROR) << "Material type: " << ToClassName(mat);

	//------------------------------------------------------------------
	// Check if it is material Matte
	//------------------------------------------------------------------
	if (dynamic_cast<Matte *>(mat)) {
		// Define the material
		Matte *matte = dynamic_cast<Matte *>(mat);
		matName = matte->GetName();

		// Check if the material has already been defined
		if (!slgScene->matDefs.IsMaterialDefined(matName)) {
			const string texName = GetSLGTexName(slgScene, matte->GetTexture());

			const string matProp = "scene.materials." + matName +".type = matte\n" +
				"scene.materials." + matName +".emission = " + emissionTexName + "\n"
				"scene.materials." + matName +".kd = " + texName + "\n";
			LOG(LUX_DEBUG, LUX_NOERROR) << "Defining material " << matName << ": [\n" << matProp << "]";
			slgScene->DefineMaterials(matProp);
		}
	} else
	//------------------------------------------------------------------
	// Check if it is material Mirror
	//------------------------------------------------------------------
	if (dynamic_cast<Mirror *>(mat)) {
		// Define the material
		Mirror *mirror = dynamic_cast<Mirror *>(mat);
		matName = mirror->GetName();

		// Check if the material has already been defined
		if (!slgScene->matDefs.IsMaterialDefined(matName)) {
			const string texName = GetSLGTexName(slgScene, mirror->GetTexture());

			const string matProp = "scene.materials." + matName +".type = mirror\n"
				"scene.materials." + matName +".emission = " + emissionTexName + "\n"
				"scene.materials." + matName +".kd = " + texName + "\n";
			LOG(LUX_DEBUG, LUX_NOERROR) << "Defining material " << matName << ": [\n" << matProp << "]";
			slgScene->DefineMaterials(matProp);
		}
	} else
	//------------------------------------------------------------------
	// Check if it is material Glass
	//------------------------------------------------------------------
	if (dynamic_cast<Glass *>(mat)) {
		// Define the material
		Glass *glass = dynamic_cast<Glass *>(mat);
		matName = glass->GetName();

		// Textures
		const string ktTexName = GetSLGTexName(slgScene, glass->GetKtTexture());
		const string krTexName = GetSLGTexName(slgScene, glass->GetKrTexture());

		// Check if it is architectural glass
		const bool architectural = (*glass)["architectural"].BoolValue();
		LOG(LUX_DEBUG, LUX_NOERROR) << "Architectural glass: " << architectural;
		// Check if the material has already been defined
		if (!slgScene->matDefs.IsMaterialDefined(matName)) {
			string matProp;
			if (architectural) {
				matProp = "scene.materials." + matName +".type = archglass\n"
						"scene.materials." + matName +".kr = " + krTexName + "\n"
						"scene.materials." + matName +".kt = " + ktTexName + "\n";
			} else {
				const string indexTexName = GetSLGTexName(slgScene, glass->GetIndexTexture());
				matProp = "scene.materials." + matName +".type = glass\n"
						"scene.materials." + matName +".kr = " + krTexName + "\n"
						"scene.materials." + matName +".kt = " + ktTexName + "\n"
						"scene.materials." + matName +".ioroutside = 1.0\n"
						"scene.materials." + matName +".iorinside = " + indexTexName + "\n";
			}
			LOG(LUX_DEBUG, LUX_NOERROR) << "Defining material " << matName << ": [\n" << matProp << "]";
			slgScene->DefineMaterials(matProp);
		}
	} else
	//------------------------------------------------------------------
	// Check if it is material Glass2
	//------------------------------------------------------------------
	if (dynamic_cast<Glass2 *>(mat)) {
		// Define the material
		Glass2 *glass2 = dynamic_cast<Glass2 *>(mat);
		matName = glass2->GetName();

		luxrays::Spectrum krRGB(1.f);
		luxrays::Spectrum ktRGB(1.f);
		float index = 1.41f;

		const Volume *intVol = prim->GetInterior();
		LOG(LUX_DEBUG, LUX_NOERROR) << "Glass2 interior volume type: " << ToClassName(intVol);

		if (dynamic_cast<const ClearVolume *>(intVol)) {
			const ClearVolume *clrVol = dynamic_cast<const ClearVolume *>(intVol);

			// Try to extract the index from Volume information
			const Texture<FresnelGeneral> *fresnelTex = clrVol->GetFresnelTexture();
			LOG(LUX_DEBUG, LUX_NOERROR) << "FresnelGeneral Texture type: " << ToClassName(fresnelTex);
			if (dynamic_cast<const ConstantFresnelTexture *>(fresnelTex)) {
				const ConstantFresnelTexture *constFresnelTex = 
					dynamic_cast<const ConstantFresnelTexture *>(fresnelTex);
				index = (*constFresnelTex)["value"].FloatValue();
			}

			// Kt
			const Texture<SWCSpectrum> *absorbTex = clrVol->GetAbsorptionTexture();
			LOG(LUX_DEBUG, LUX_NOERROR) << "Absorption Texture type: " << ToClassName(absorbTex);
			const ConstantRGBColorTexture *absorbRGBTex = dynamic_cast<const ConstantRGBColorTexture *>(absorbTex);

			if (absorbRGBTex) {
				ktRGB.r = (*absorbRGBTex)["color.r"].FloatValue();
				ktRGB.g = (*absorbRGBTex)["color.g"].FloatValue();
				ktRGB.b = (*absorbRGBTex)["color.b"].FloatValue();

				ktRGB = ktRGB * ktRGB;
				ktRGB.r = Clamp(ktRGB.r, 0.f, 20.f) / 20.f;
				ktRGB.g = Clamp(ktRGB.g, 0.f, 20.f) / 20.f;
				ktRGB.b = Clamp(ktRGB.b, 0.f, 20.f) / 20.f;

				ktRGB.r = 1.f - ktRGB.r;
				ktRGB.g = 1.f - ktRGB.g;
				ktRGB.b = 1.f - ktRGB.b;
			} else {
				LOG(LUX_WARNING, LUX_UNIMPLEMENT) << "SLGRenderer supports only Glass2 material with ConstantRGBColorTexture (i.e. not " <<
					ToClassName(absorbRGBTex) << "). Ignoring unsupported texture.";
			}
		}

		// Check if it is architectural glass
		const bool architectural = (*glass2)["architectural"].BoolValue();
		LOG(LUX_DEBUG, LUX_NOERROR) << "Architectural glass: " << architectural;
		// Check if the material has already been defined
		if (!slgScene->matDefs.IsMaterialDefined(matName)) {
			string matProp;
			if (architectural) {
				matProp = "scene.materials." + matName +".type = archglass\n"
						"scene.materials." + matName +".kr = " +
							ToString(krRGB.r) + " " +
							ToString(krRGB.g) + " " +
							ToString(krRGB.b) + "\n"
						"scene.materials." + matName +".kt = " +
							ToString(ktRGB.r) + " " +
							ToString(ktRGB.g) + " " +
							ToString(ktRGB.b) + "\n";
			} else {
				matProp = "scene.materials." + matName +".type = glass\n"
						"scene.materials." + matName +".kr = " +
							ToString(krRGB.r) + " " +
							ToString(krRGB.g) + " " +
							ToString(krRGB.b) + "\n"
						"scene.materials." + matName +".kt = " +
							ToString(ktRGB.r) + " " +
							ToString(ktRGB.g) + " " +
							ToString(ktRGB.b) + "\n"
						"scene.materials." + matName +".ioroutside = 1.0\n"
						"scene.materials." + matName +".iorinside = " + ToString(index) + "\n";
			}
			LOG(LUX_DEBUG, LUX_NOERROR) << "Defining material " << matName << ": [\n" << matProp << "]";
			slgScene->DefineMaterials(matProp);

			LOG(LUX_WARNING, LUX_UNIMPLEMENT) << "SLGRenderer doesn't support Glass2 material, trying an emulation with Glass1.";
		}
	} else
	//------------------------------------------------------------------
	// Check if it is material Metal
	//------------------------------------------------------------------
	if (dynamic_cast<Metal *>(mat)) {
		// Define the material
		Metal *metal = dynamic_cast<Metal *>(mat);
		matName = metal->GetName();

		// Check if the material has already been defined
		if (!slgScene->matDefs.IsMaterialDefined(matName)) {
			// Try to guess the exponent from the roughness of the surface in the u direction
			Texture<float> *uroughnessTex = metal->GetNuTexture();
			LOG(LUX_DEBUG, LUX_NOERROR) << "Nu Texture type: " << ToClassName(uroughnessTex);
			ConstantFloatTexture *uroughnessFloatTex = dynamic_cast<ConstantFloatTexture *>(uroughnessTex);

			float uroughness;
			if (uroughnessFloatTex)
				uroughness = Clamp((*uroughnessFloatTex)["value"].FloatValue(), 6e-3f, 1.f);
			else {
				LOG(LUX_WARNING, LUX_UNIMPLEMENT) << "SLGRenderer supports only Metal material with ConstantFloatTexture (i.e. not " <<
					ToClassName(uroughnessFloatTex) << "). Ignoring unsupported texture and using 0.1 value.";
				uroughness = .1f;
			}
			const float exponent = 1.f / uroughness;

			string matProp;
			// Retrieve the metal name
			const string metalName = (*metal)["metalName"].StringValue();
			if (metalName == "amorphous carbon")
				matProp =
					"scene.materials." + matName +".type = metal\n"
					"scene.materials." + matName +".kr = 0.1 0.1 0.1\n"
					"scene.materials." + matName +".exp = " +ToString(exponent) + "\n";
			else if (metalName == "silver")
				matProp =
					"scene.materials." + matName + "_mat1.type = metal\n"
					"scene.materials." + matName + "_mat1.kr = 0.9 0.9 0.9\n"
					"scene.materials." + matName + "_mat1.exp = " +ToString(exponent) + "\n"
					"scene.materials." + matName + "_mat2.type = matte\n"
					"scene.materials." + matName + "_mat2.kd = 0.75 0.75 0.75\n"
					"scene.materials." + matName +".type = mix\n"
					"scene.materials." + matName +".material1 = " + matName + "_mat1\n"
					"scene.materials." + matName +".material2 = " + matName + "_mat2\n"
					"scene.materials." + matName +".amount = 0.5\n";
			else if (metalName == "gold")
				matProp =
					"scene.materials." + matName + "_mat1.type = metal\n"
					"scene.materials." + matName + "_mat1.kr = 0.9 0.55 0.05\n"
					"scene.materials." + matName + "_mat1.exp = " +ToString(exponent) + "\n"
					"scene.materials." + matName + "_mat2.type = matte\n"
					"scene.materials." + matName + "_mat2.kd = 0.09 0.055 0.005\n"
					"scene.materials." + matName +".type = mix\n"
					"scene.materials." + matName +".material1 = " + matName + "_mat1\n"
					"scene.materials." + matName +".material2 = " + matName + "_mat2\n"
					"scene.materials." + matName +".amount = 0.5\n";
			else if (metalName == "copper")
				matProp =
					"scene.materials." + matName + "_mat1.type = metal\n"
					"scene.materials." + matName + "_mat1.kr = 0.9 0.7 0.6\n"
					"scene.materials." + matName + "_mat1.exp = " +ToString(exponent) + "\n"
					"scene.materials." + matName + "_mat2.type = matte\n"
					"scene.materials." + matName + "_mat2.kd = 0.4 0.25 0.2\n"
					"scene.materials." + matName +".type = mix\n"
					"scene.materials." + matName +".material1 = " + matName + "_mat1\n"
					"scene.materials." + matName +".material2 = " + matName + "_mat2\n"
					"scene.materials." + matName +".amount = 0.5\n";
			else {
				if (metalName != "aluminium")
					LOG(LUX_WARNING, LUX_UNIMPLEMENT) << "SLGRenderer supports only Metal material of name 'amorphous carbon', 'silver', 'gold', 'copper' and 'aluminium' (i.e. not " <<
						metalName << "). Replacing an unsupported material with metal 'aluminium'.";

				matProp =
					"scene.materials." + matName + "_mat1.type = metal\n"
					"scene.materials." + matName + "_mat1.kr = 0.9 0.9 0.9\n"
					"scene.materials." + matName + "_mat1.exp = " +ToString(exponent) + "\n"
					"scene.materials." + matName + "_mat2.type = matte\n"
					"scene.materials." + matName + "_mat2.kd = 0.75 0.75 0.75\n"
					"scene.materials." + matName +".type = mix\n"
					"scene.materials." + matName +".material1 = " + matName + "_mat1\n"
					"scene.materials." + matName +".material2 = " + matName + "_mat2\n"
					"scene.materials." + matName +".amount = 0.5\n";
			}

			LOG(LUX_DEBUG, LUX_NOERROR) << "Defining material " << matName << ": [\n" << matProp << "]";
			slgScene->DefineMaterials(matProp);

			LOG(LUX_WARNING, LUX_UNIMPLEMENT) << "SLGRenderer doesn't support Metal material, trying an emulation with SLG Metal.";
		}
	} else
	//------------------------------------------------------------------
	// Material is not supported, use the default one
	//------------------------------------------------------------------
	{
		LOG(LUX_WARNING, LUX_UNIMPLEMENT) << "SLGRenderer supports only Matte, Mirror, Glass, Glass2, Glossy2 and Metal material (i.e. not " <<
			ToClassName(mat) << "). Replacing an unsupported material with matte.";
		return "mat_default";
	}

	return matName;
}

void SLGRenderer::ConvertEnvLights(luxrays::sdl::Scene *slgScene) {
	// Check if there is a sun light source
	SunLight *sunLight = NULL;
	for (size_t i = 0; i < scene->lights.size(); ++i) {
		sunLight = dynamic_cast<SunLight *>(scene->lights[i]);
		if (sunLight)
			break;
	}

	if (sunLight) {
		// Add a SunLight to the scene
		const float dirX = (*sunLight)["dir.x"].FloatValue();
		const float dirY = (*sunLight)["dir.y"].FloatValue();
		const float dirZ = (*sunLight)["dir.z"].FloatValue();
		const float turbidity = (*sunLight)["turbidity"].FloatValue();
		const float relSize = (*sunLight)["relSize"].FloatValue();
		// Note: (1000000000.0f / (M_PI * 100.f * 100.f)) is in SLG code
		// for compatibility with past scene
		const float gain = (*sunLight)["gain"].FloatValue() * (1000000000.0f / (M_PI * 100.f * 100.f)) *
			INV_PI;

		const std::string createSunLightProp = "scene.sunlight.dir = " +
				ToString(dirX) + " " +
				ToString(dirY) + " " +
				ToString(dirZ) + "\n"
			"scene.sunlight.turbidity = " + ToString(turbidity) + "\n"
			"scene.sunlight.relsize = " + ToString(relSize) + "\n"
			"scene.sunlight.gain = " +
				ToString(gain) + " " +
				ToString(gain) + " " +
				ToString(gain) + "\n";
		LOG(LUX_DEBUG, LUX_NOERROR) << "Creating sunlight: [\n" << createSunLightProp << "]";
		slgScene->AddSunLight(createSunLightProp);
	}

	// Check if there is a sky or sky2 light source
	SkyLight *skyLight = NULL;
	Sky2Light *sky2Light = NULL;
	for (size_t i = 0; i < scene->lights.size(); ++i) {
		skyLight = dynamic_cast<SkyLight *>(scene->lights[i]);
		sky2Light = dynamic_cast<Sky2Light *>(scene->lights[i]);
		if (skyLight || sky2Light)
			break;
	}

	if (skyLight || sky2Light) {
		// Add a SkyLight to the scene

		// Note: (1000000000.0f / (M_PI * 100.f * 100.f)) is in SLG code
		// for compatibility with past scene
		const float gainAdjustFactor = (1000000000.0f / (M_PI * 100.f * 100.f)) * INV_PI;

		if (sky2Light) {
			LOG(LUX_WARNING, LUX_UNIMPLEMENT) << "SLGRenderer doesn't support Sky2 light. It will use Sky instead.";

			const float dirX = (*sky2Light)["dir.x"].FloatValue();
			const float dirY = (*sky2Light)["dir.y"].FloatValue();
			const float dirZ = (*sky2Light)["dir.z"].FloatValue();
			const float turbidity = (*sky2Light)["turbidity"].FloatValue();
			// Note: (1000000000.0f / (M_PI * 100.f * 100.f)) is in SLG code
			// for compatibility with past scene
			const float gain = (*sky2Light)["gain"].FloatValue() * gainAdjustFactor;

			const std::string createSkyLightProp = "scene.skylight.dir = " +
					ToString(dirX) + " " +
					ToString(dirY) + " " +
					ToString(dirZ) + "\n"
				"scene.skylight.turbidity = " + ToString(turbidity) + "\n"
				"scene.skylight.gain = " +
					ToString(gain) + " " +
					ToString(gain) + " " +
					ToString(gain) + "\n";
			LOG(LUX_DEBUG, LUX_NOERROR) << "Creating skylight: [\n" << createSkyLightProp << "]";
			slgScene->AddSkyLight(createSkyLightProp);
		} else {
			const float dirX = (*skyLight)["dir.x"].FloatValue();
			const float dirY = (*skyLight)["dir.y"].FloatValue();
			const float dirZ = (*skyLight)["dir.z"].FloatValue();
			const float turbidity = (*skyLight)["turbidity"].FloatValue();
			// Note: (1000000000.0f / (M_PI * 100.f * 100.f)) is in SLG code
			// for compatibility with past scene
			const float gain = (*skyLight)["gain"].FloatValue() * gainAdjustFactor;

			const std::string createSkyLightProp = "scene.skylight.dir = " +
					ToString(dirX) + " " +
					ToString(dirY) + " " +
					ToString(dirZ) + "\n"
				"scene.skylight.turbidity = " + ToString(turbidity) + "\n"
				"scene.skylight.gain = " +
					ToString(gain) + " " +
					ToString(gain) + " " +
					ToString(gain) + "\n";
			LOG(LUX_DEBUG, LUX_NOERROR) << "Creating skylight: [\n" << createSkyLightProp << "]";
			slgScene->AddSkyLight(createSkyLightProp);
		}
	}
	
	// Check if there is a sky or sky2 light source
	InfiniteAreaLight *infiniteAreaLight = NULL;
	InfiniteAreaLightIS *infiniteAreaLightIS = NULL;
	for (size_t i = 0; i < scene->lights.size(); ++i) {
		infiniteAreaLight = dynamic_cast<InfiniteAreaLight *>(scene->lights[i]);
		infiniteAreaLightIS = dynamic_cast<InfiniteAreaLightIS *>(scene->lights[i]);
		if (infiniteAreaLight || infiniteAreaLightIS)
			break;
	}

	if (infiniteAreaLight || infiniteAreaLightIS) {
		// Check if I have already a sky light
		if (skyLight || sky2Light)
			LOG(LUX_WARNING, LUX_UNIMPLEMENT) << "SLGRenderer supports only one single environmental light. Using sky light and ignoring infinite lights";
		else {
			if (infiniteAreaLight) {
				const float colorR = (*infiniteAreaLight)["color.r"].FloatValue();
				const float colorG = (*infiniteAreaLight)["color.g"].FloatValue();
				const float colorB = (*infiniteAreaLight)["color.b"].FloatValue();
				const float gain = (*infiniteAreaLight)["gain"].FloatValue();

				const float gamma = (*infiniteAreaLight)["gamma"].FloatValue();

				MIPMap *mipMap = infiniteAreaLight->GetRadianceMap();
				const string texName = GetSLGImageMapName(slgScene, mipMap, gamma);

				const std::string createInfiniteLightProp = "scene.infinitelight.file = " + texName + "\n"
					"scene.infinitelight.gamma = " + ToString(gamma) + "\n"
					"scene.infinitelight.shift = 0.5 0.0\n"
					"scene.infinitelight.gain = " +
						ToString(gain * colorR) + " " +
						ToString(gain * colorG) + " " +
						ToString(gain * colorB) + "\n";
				LOG(LUX_DEBUG, LUX_NOERROR) << "Creating infinitelight: [\n" << createInfiniteLightProp << "]";
				slgScene->AddInfiniteLight(createInfiniteLightProp);
			} else {
				const float colorR = (*infiniteAreaLightIS)["color.r"].FloatValue();
				const float colorG = (*infiniteAreaLightIS)["color.g"].FloatValue();
				const float colorB = (*infiniteAreaLightIS)["color.b"].FloatValue();
				const float gain = (*infiniteAreaLightIS)["gain"].FloatValue();

				const float gamma = (*infiniteAreaLightIS)["gamma"].FloatValue();

				MIPMap *mipMap = infiniteAreaLightIS->GetRadianceMap();
				const string texName = GetSLGImageMapName(slgScene, mipMap, gamma);

				const std::string createInfiniteLightProp = "scene.infinitelight.file = " + texName + "\n"
					"scene.infinitelight.gamma = " + ToString(gamma) + "\n"
					"scene.infinitelight.shift = 0.5 0.0\n"
					"scene.infinitelight.gain = " +
						ToString(gain * colorR) + " " +
						ToString(gain * colorG) + " " +
						ToString(gain * colorB) + "\n";
				LOG(LUX_DEBUG, LUX_NOERROR) << "Creating infinitelight: [\n" << createInfiniteLightProp << "]";
				slgScene->AddInfiniteLight(createInfiniteLightProp);
			}
		}
	}
}

vector<luxrays::ExtTriangleMesh *> SLGRenderer::DefinePrimitive(luxrays::sdl::Scene *slgScene, const Primitive *prim) {
	//LOG(LUX_DEBUG, LUX_NOERROR) << "Define primitive type: " << ToClassName(prim);

	vector<luxrays::ExtTriangleMesh *> meshList;
	prim->ExtTesselate(&meshList, &scene->tesselatedPrimitives);

	for (vector<luxrays::ExtTriangleMesh *>::const_iterator mesh = meshList.begin(); mesh != meshList.end(); ++mesh) {
		if (!(*mesh)->HasNormals()) {
			// SLG requires shading normals
			Normal *normals = (*mesh)->ComputeNormals();

			if (normals) {
				// I have to keep track of memory allocated for normals so, later, it
				// can be deleted
				alloctedMeshNormals.push_back(normals);
			}
		}

		const string meshName = "Mesh-" + ToString(*mesh);
		slgScene->DefineObject(meshName, *mesh);
	}

	return meshList;
}

void SLGRenderer::ConvertGeometry(luxrays::sdl::Scene *slgScene) {
	LOG(LUX_INFO, LUX_NOERROR) << "Tesselating " << scene->primitives.size() << " primitives";

	// To keep track of all primitive mesh lists
	map<const Primitive *, vector<luxrays::ExtTriangleMesh *> > primMeshLists;

	for (size_t i = 0; i < scene->primitives.size(); ++i) {
		const Primitive *prim = scene->primitives[i].get();
		//LOG(LUX_DEBUG, LUX_NOERROR) << "Primitive type: " << ToClassName(prim);

		// Instances require special care
		if (dynamic_cast<const InstancePrimitive *>(prim)) {
			const InstancePrimitive *instance = dynamic_cast<const InstancePrimitive *>(prim);
			const string matName = GetSLGMaterialName(slgScene, instance);

			const vector<boost::shared_ptr<Primitive> > &instanceSources = instance->GetInstanceSources();

			for (u_int i = 0; i < instanceSources.size(); ++i) {
				const Primitive *instancedSource = instanceSources[i].get();

				vector<luxrays::ExtTriangleMesh *> meshList;
				// Check if I have already defined one of the original primitive
				if (primMeshLists.count(instancedSource) < 1) {
					// I have to define the instanced primitive
					meshList = DefinePrimitive(slgScene, instancedSource);
					primMeshLists[instancedSource] = meshList;
				} else
					meshList = primMeshLists[instancedSource];

				if (meshList.size() == 0)
					continue;

				// Build transformation string
				const Transform trans = instance->GetTransform();
				std::ostringstream ss;
				for (int j = 0; j < 4; ++j)
					for (int i = 0; i < 4; ++i)
						ss << trans.m.m[i][j] << " ";
				string transString = ss.str();
	
				// Add the object
				for (vector<luxrays::ExtTriangleMesh *>::const_iterator mesh = meshList.begin(); mesh != meshList.end(); ++mesh) {
					// Define an instance of the mesh
					const string objName = "InstanceObject-" + ToString(prim) + "-" +
						ToString(*mesh);
					const string meshName = "Mesh-" + ToString(*mesh);

					std::ostringstream ss;
					const string prefix = "scene.objects." + objName;
					ss << prefix << ".material = " << matName << "\n";
					ss << prefix << ".transformation = " << transString << "\n";
//					if (matInfo.texMap.name != "") {
//						ss << prefix << ".texmap = " << matInfo.texMap.name << "\n";
//						ss << prefix << ".texmap.uscale = " << matInfo.texMap.uScale << "\n";
//						ss << prefix << ".texmap.vscale = " << matInfo.texMap.vScale << "\n";
//						ss << prefix << ".texmap.udelta = " << matInfo.texMap.uDelta << "\n";
//						ss << prefix << ".texmap.vdelta = " << matInfo.texMap.vDelta << "\n";
//					}
//					if (matInfo.bumpMap.name != "") {
//						ss << prefix << ".bumpmap = " << matInfo.bumpMap.name << "\n";
//						ss << prefix << ".bumpmap.uscale = " << matInfo.bumpMap.uScale << "\n";
//						ss << prefix << ".bumpmap.vscale = " << matInfo.bumpMap.vScale << "\n";
//						ss << prefix << ".bumpmap.udelta = " << matInfo.bumpMap.uDelta << "\n";
//						ss << prefix << ".bumpmap.vdelta = " << matInfo.bumpMap.vDelta << "\n";
//						ss << prefix << ".bumpmap.scale = " << matInfo.bumpMap.scale << "\n";
//					}
//					if (matInfo.normalMap.name != "") {
//						ss << prefix << ".normalmap = " << matInfo.normalMap.name << "\n";
//						ss << prefix << ".normalmap.uscale = " << matInfo.normalMap.uScale << "\n";
//						ss << prefix << ".normalmap.vscale = " << matInfo.normalMap.vScale << "\n";
//						ss << prefix << ".normalmap.udelta = " << matInfo.normalMap.uDelta << "\n";
//						ss << prefix << ".normalmap.vdelta = " << matInfo.normalMap.vDelta << "\n";
//					}
					ss << prefix << ".useplynormals = 1\n";

					const std::string createObjProp = ss.str();
					LOG(LUX_DEBUG, LUX_NOERROR) << "Creating object: [\n" << createObjProp << "]";
					slgScene->AddObject(objName, meshName, createObjProp);
				}
			}
		} else {
			vector<luxrays::ExtTriangleMesh *> meshList;
			if (primMeshLists.count(prim) < 1) {
				meshList = DefinePrimitive(slgScene, prim);
				primMeshLists[prim] = meshList;
			} else
				meshList = primMeshLists[prim];

			if (meshList.size() == 0)
				continue;

			// Add the object
			const string matName = GetSLGMaterialName(slgScene, prim);

			for (vector<luxrays::ExtTriangleMesh *>::const_iterator mesh = meshList.begin(); mesh != meshList.end(); ++mesh) {
				const string objName = "Object-" + ToString(prim) + "-" +
					ToString(*mesh);
				const string meshName = "Mesh-" + ToString(*mesh);
				
				std::ostringstream ss;
				const string prefix = "scene.objects." + objName;
				ss << prefix << ".material = " << matName << "\n";
//				if (matInfo.texMap.name != "") {
//					ss << prefix << ".texmap = " << matInfo.texMap.name << "\n";
//					ss << prefix << ".texmap.uscale = " << matInfo.texMap.uScale << "\n";
//					ss << prefix << ".texmap.vscale = " << matInfo.texMap.vScale << "\n";
//					ss << prefix << ".texmap.udelta = " << matInfo.texMap.uDelta << "\n";
//					ss << prefix << ".texmap.vdelta = " << matInfo.texMap.vDelta << "\n";
//				}
//				if (matInfo.bumpMap.name != "") {
//					ss << prefix << ".bumpmap = " << matInfo.bumpMap.name << "\n";
//					ss << prefix << ".bumpmap.uscale = " << matInfo.bumpMap.uScale << "\n";
//					ss << prefix << ".bumpmap.vscale = " << matInfo.bumpMap.vScale << "\n";
//					ss << prefix << ".bumpmap.udelta = " << matInfo.bumpMap.uDelta << "\n";
//					ss << prefix << ".bumpmap.vdelta = " << matInfo.bumpMap.vDelta << "\n";
//					ss << prefix << ".bumpmap.scale = " << matInfo.bumpMap.scale << "\n";
//				}
//				if (matInfo.normalMap.name != "") {
//					ss << prefix << ".normalmap = " << matInfo.normalMap.name << "\n";
//					ss << prefix << ".normalmap.uscale = " << matInfo.normalMap.uScale << "\n";
//					ss << prefix << ".normalmap.vscale = " << matInfo.normalMap.vScale << "\n";
//					ss << prefix << ".normalmap.udelta = " << matInfo.normalMap.uDelta << "\n";
//					ss << prefix << ".normalmap.vdelta = " << matInfo.normalMap.vDelta << "\n";
//				}
				ss << prefix << ".useplynormals = 1\n";

				const std::string createObjProp = ss.str();
				LOG(LUX_DEBUG, LUX_NOERROR) << "Creating object: [\n" << createObjProp << "]";
				slgScene->AddObject(objName, meshName, createObjProp);
			}
		}
	}

	if (slgScene->meshDefs.GetSize() == 0)
		throw std::runtime_error("SLGRenderer can not render an empty scene");
}

void SLGRenderer::ConvertCamera(luxrays::sdl::Scene *slgScene) {
	LOG(LUX_DEBUG, LUX_NOERROR) << "Camera type: " << ToClassName(scene->camera());
	PerspectiveCamera *perpCamera = dynamic_cast<PerspectiveCamera *>(scene->camera());
	if (!perpCamera)
		throw std::runtime_error("SLGRenderer supports only PerspectiveCamera");

	//--------------------------------------------------------------------------
	// Setup the camera
	//--------------------------------------------------------------------------

	const Point orig(
			(scene->camera)["position.x"].FloatValue(),
			(scene->camera)["position.y"].FloatValue(),
			(scene->camera)["position.z"].FloatValue());
	const Point target= orig + Vector(
			(scene->camera)["normal.x"].FloatValue(),
			(scene->camera)["normal.y"].FloatValue(),
			(scene->camera)["normal.z"].FloatValue());
	const Vector up(
			(scene->camera)["up.x"].FloatValue(),
			(scene->camera)["up.y"].FloatValue(),
			(scene->camera)["up.z"].FloatValue());

	const string createCameraProp = "scene.camera.lookat = " + 
			ToString(orig.x) + " " +
			ToString(orig.y) + " " +
			ToString(orig.z) + " " +
			ToString(target.x) + " " +
			ToString(target.y) + " " +
			ToString(target.z) + "\n"
		"scene.camera.up = " +
			ToString(up.x) + " " +
			ToString(up.y) + " " +
			ToString(up.z) + "\n"
		"scene.camera.screenwindow = " +
			ToString((scene->camera)["ScreenWindow.0"].FloatValue()) + " " +
			ToString((scene->camera)["ScreenWindow.1"].FloatValue()) + " " +
			ToString((scene->camera)["ScreenWindow.2"].FloatValue()) + " " +
			ToString((scene->camera)["ScreenWindow.3"].FloatValue()) + "\n" +
		"scene.camera.fieldofview = " + ToString(Degrees((scene->camera)["fov"].FloatValue())) + "\n"
		"scene.camera.lensradius = " + ToString((scene->camera)["LensRadius"].FloatValue()) + "\n"
		"scene.camera.focaldistance = " + ToString((scene->camera)["FocalDistance"].FloatValue()) + "\n"
		"scene.camera.cliphither = " + ToString((scene->camera)["ClipHither"].FloatValue()) + "\n"
		"scene.camera.clipyon = " + ToString((scene->camera)["ClipYon"].FloatValue()) + "\n";

	LOG(LUX_DEBUG, LUX_NOERROR) << "Creating camera: [\n" << createCameraProp << "]";
	slgScene->CreateCamera(createCameraProp);
}

luxrays::sdl::Scene *SLGRenderer::CreateSLGScene(const luxrays::Properties &slgConfigProps) {
	const int accelType = slgConfigProps.GetInt("accelerator.type", -1);
	luxrays::sdl::Scene *slgScene = new luxrays::sdl::Scene(accelType);

	// Tell to the cache to not delete mesh data (they are pointed by Lux
	// primitives too and they will be deleted by Lux Context)
	slgScene->extMeshCache.SetDeleteMeshData(false);

	//--------------------------------------------------------------------------
	// Setup the camera
	//--------------------------------------------------------------------------

	ConvertCamera(slgScene);

	//--------------------------------------------------------------------------
	// Setup default material
	//--------------------------------------------------------------------------

	slgScene->DefineMaterials(
		"scene.materials.mat_default.type = matte\n"
		"scene.materials.mat_default.kd = 0.75 0.75 0.75\n"
		);

	//--------------------------------------------------------------------------
	// Setup lights
	//--------------------------------------------------------------------------

	ConvertEnvLights(slgScene);

	//--------------------------------------------------------------------------
	// Convert geometry
	//--------------------------------------------------------------------------

	ConvertGeometry(slgScene);

	return slgScene;
}

luxrays::Properties SLGRenderer::CreateSLGConfig() {
	std::ostringstream ss;

	ss << "opencl.platform.index = -1\n"
			"opencl.cpu.use = 1\n"
			"opencl.gpu.use = 1\n"
			;

	//--------------------------------------------------------------------------
	// Surface integrator related settings
	//--------------------------------------------------------------------------

	LOG(LUX_DEBUG, LUX_NOERROR) << "Surface integrator type: " << ToClassName(scene->surfaceIntegrator);
	if (dynamic_cast<PathIntegrator *>(scene->surfaceIntegrator)) {
		// Path tracing
		PathIntegrator *path = dynamic_cast<PathIntegrator *>(scene->surfaceIntegrator);
		const int maxDepth = (*path)["maxDepth"].IntValue();

		ss << "renderengine.type = PATHOCL\n" <<
				"path.maxdepth = " << maxDepth << "\n";
	} else if (dynamic_cast<BidirIntegrator *>(scene->surfaceIntegrator)) {
		// Bidirectional path tracing
		BidirIntegrator *bidir = dynamic_cast<BidirIntegrator *>(scene->surfaceIntegrator);
		const int maxEyeDepth = (*bidir)["maxEyeDepth"].IntValue();
		const int maxLightDepth = (*bidir)["maxLightDepth"].IntValue();

		ss << "renderengine.type = BIDIRVMCPU\n" <<
			"path.maxdepth = " << maxLightDepth << "\n" <<
			"light.maxdepth = " << maxEyeDepth << "\n";
	} else {
		// Unmapped surface integrator, just use path tracing
		LOG(LUX_WARNING, LUX_UNIMPLEMENT) << "SLGRenderer doesn't support the SurfaceIntegrator, falling back to path tracing";
		ss << "renderengine.type = PATHOCL\n";
	}

	//--------------------------------------------------------------------------
	// Film related settings
	//--------------------------------------------------------------------------

	Film *film = scene->camera()->film;
	int xStart, xEnd, yStart, yEnd;
	film->GetSampleExtent(&xStart, &xEnd, &yStart, &yEnd);
	const int imageWidth = xEnd - xStart + 1;
	const int imageHeight = yEnd - yStart + 1;

	float cropWindow[4] = {
		(*film)["cropWindow.0"].FloatValue(),
		(*film)["cropWindow.1"].FloatValue(),
		(*film)["cropWindow.2"].FloatValue(),
		(*film)["cropWindow.3"].FloatValue()
	};
	if ((cropWindow[0] != 0.f) || (cropWindow[1] != 1.f) ||
			(cropWindow[2] != 0.f) || (cropWindow[3] != 1.f))
		throw std::runtime_error("SLGRenderer doesn't yet support border rendering");

	ss << "image.width = " + ToString(imageWidth) + "\n"
			"image.height = " + ToString(imageHeight) + "\n";

	//--------------------------------------------------------------------------
	// Sampler related settings
	//--------------------------------------------------------------------------

	LOG(LUX_DEBUG, LUX_NOERROR) << "Sampler type: " << ToClassName(scene->sampler);
	if (dynamic_cast<MetropolisSampler *>(scene->sampler)) {
		MetropolisSampler *sampler = dynamic_cast<MetropolisSampler *>(scene->sampler);
		const int maxRejects = (*sampler)["maxRejects"].IntValue();
		const float pLarge = (*sampler)["pLarge"].FloatValue();
		const float range = (*sampler)["range"].FloatValue() * 2.f / (xEnd - xStart);

		ss << "sampler.type = METROPOLIS\n"
				"sampler.maxconsecutivereject = " + ToString(maxRejects) + "\n"
				"sampler.largesteprate = " + ToString(pLarge) + "\n"
				"sampler.imagemutationrate = " + ToString(range) + "\n";
	} else if (dynamic_cast<RandomSampler *>(scene->sampler)) {
		ss << "sampler.type = INLINED_RANDOM\n";
	} else {
		// Unmapped sampler, just use random
		LOG(LUX_WARNING, LUX_UNIMPLEMENT) << "SLGRenderer doesn't support the Sampler, falling back to random sampler";
		ss << "sampler.type = INLINED_RANDOM\n";
	}

	//--------------------------------------------------------------------------
	// Pixel filter related settings
	//--------------------------------------------------------------------------

	const Filter *filter = film->GetFilter();
	const string filterType = (*filter)["type"].StringValue();
	if (filterType == "box")
		ss << "film.filter.type = BOX\n";
	else if (filterType == "gaussian")
		ss << "film.filter.type = GAUSSIAN\n";
	else if (filterType == "mitchell")
		ss << "film.filter.type = MITCHELL\n";
	else if (filterType == "mitchell_ss")
		ss << "film.filter.type = MITCHELL_SS\n";
	else {
		LOG(LUX_WARNING, LUX_UNIMPLEMENT) << "SLGRenderer doesn't support the filter type " << filterType <<
				", using MITCHELL filter instead";
		ss << "film.filter.type = MITCHELL\n";
	}

	//--------------------------------------------------------------------------

	const string configString = ss.str();
	LOG(LUX_DEBUG, LUX_NOERROR) << "SLG configuration: [\n" << configString << "]";
	luxrays::Properties config;
	config.LoadFromString(configString);

	// Add overwrite properties
	config.Load(overwriteConfig);

	// Check if light buffer is needed and required
	slg::RenderEngineType type = slg::RenderEngine::String2RenderEngineType(config.GetString("renderengine.type", "PATHOCL"));
	if (((type == slg::LIGHTCPU) ||
		(type == slg::BIDIRCPU) ||
		(type == slg::BIDIRHYBRID) ||
		(type == slg::CBIDIRHYBRID) ||
		(type == slg::BIDIRVMCPU)) && !dynamic_cast<BidirIntegrator *>(scene->surfaceIntegrator)) {
		throw std::runtime_error("You have to select bidirectional surface integrator in order to use the selected render engine");
	}

	return config;
}

void SLGRenderer::UpdateLuxFilm(slg::RenderSession *session) {
	luxrays::utils::Film *slgFilm = session->film;

	Film *film = scene->camera()->film;
	ColorSystem colorSpace = film->GetColorSpace();
	int xStart, xEnd, yStart, yEnd;
	film->GetSampleExtent(&xStart, &xEnd, &yStart, &yEnd);

	// Recover the ID of buffers
	const PathIntegrator *path = dynamic_cast<const PathIntegrator *>(scene->surfaceIntegrator);
	const BidirIntegrator *bidir = dynamic_cast<const BidirIntegrator *>(scene->surfaceIntegrator);
	u_int eyeBufferId, lightBufferId;
	if (path) {
		eyeBufferId = path->bufferId;
		lightBufferId = eyeBufferId;
	} else if (bidir) {
		eyeBufferId = bidir->eyeBufferId;
		lightBufferId = bidir->lightBufferId;		
	} else
		throw std::runtime_error("Internal error: surfaceIntegretor is not PathIntegrator or BidirIntegrator");

	// Lock the contribution pool in order to have exclusive
	// access to the film
	ScopedPoolLock poolLock(film->contribPool);

	if (slgFilm->HasPerPixelNormalizedBuffer()) {
		// Copy the information from PER_PIXEL_NORMALIZED buffer

		for (int y = yStart; y <= yEnd; ++y) {
			for (int x = xStart; x <= xEnd; ++x) {
				// I have to update LuxRender Film only with the new samples
				const u_int pixelX = x - xStart;
				const u_int pixelY = y - yStart;

				const luxrays::utils::SamplePixel *spNew = slgFilm->GetSamplePixel(
					luxrays::utils::PER_PIXEL_NORMALIZED, pixelX, pixelY);

				luxrays::Spectrum deltaRadiance = spNew->radiance - (*previousEyeBufferRadiance)(pixelX, pixelY);
				const float deltaWeight = spNew->weight - (*previousEyeWeight)(pixelX, pixelY);

				(*previousEyeBufferRadiance)(pixelX, pixelY) = spNew->radiance;
				(*previousEyeWeight)(pixelX, pixelY) = spNew->weight;

				const float alphaNew = slgFilm->IsAlphaChannelEnabled() ?
					(slgFilm->GetAlphaPixel(pixelX, pixelY)->alpha) : 0.f;
				float deltaAlpha = alphaNew - (*previousAlphaBuffer)(pixelX, pixelY);

				(*previousAlphaBuffer)(pixelX, pixelY) = alphaNew;

				if (deltaWeight > 0.f) {
					deltaRadiance /= deltaWeight;
					deltaAlpha /= deltaWeight;

					XYZColor xyz = colorSpace.ToXYZ(RGBColor(deltaRadiance.r, deltaRadiance.g, deltaRadiance.b));
					// Flip the image upside down
					Contribution contrib(x, yEnd - 1 - y, xyz, deltaAlpha, 0.f, deltaWeight, eyeBufferId);
					film->AddSampleNoFiltering(&contrib);
				}
			}
		}
	}

	if (slgFilm->HasPerScreenNormalizedBuffer()) {
		// Copy the information from PER_SCREEN_NORMALIZED buffer

		for (int y = yStart; y <= yEnd; ++y) {
			for (int x = xStart; x <= xEnd; ++x) {
				// I have to update LuxRender Film only with the new samples
				const u_int pixelX = x - xStart;
				const u_int pixelY = y - yStart;

				const luxrays::utils::SamplePixel *spNew = slgFilm->GetSamplePixel(
					luxrays::utils::PER_SCREEN_NORMALIZED, pixelX, pixelY);

				luxrays::Spectrum deltaRadiance = spNew->radiance - (*previousLightBufferRadiance)(pixelX, pixelY);
				const float deltaWeight = spNew->weight - (*previousLightWeight)(pixelX, pixelY);

				(*previousLightBufferRadiance)(pixelX, pixelY) = spNew->radiance;
				(*previousLightWeight)(pixelX, pixelY) = spNew->weight;

				if (deltaWeight > 0.f) {
					// This is required to cancel the "* weight" inside AddSampleNoFiltering()
					deltaRadiance /= deltaWeight;

					XYZColor xyz = colorSpace.ToXYZ(RGBColor(deltaRadiance.r, deltaRadiance.g, deltaRadiance.b));
					// Flip the image upside down
					Contribution contrib(x, yEnd - 1 - y, xyz, 1.f, 0.f, deltaWeight, lightBufferId);
					film->AddSampleNoFiltering(&contrib);
				}
			}
		}
	}

	const float newSampleCount = session->renderEngine->GetTotalSampleCount(); 
	film->AddSampleCount(newSampleCount - previousSampleCount);
	previousSampleCount = newSampleCount;
}

void SLGRenderer::Render(Scene *s) {
	try {
		luxrays::sdl::Scene *slgScene = NULL;
		luxrays::Properties slgConfigProps;

		{
			// Section under mutex
			boost::mutex::scoped_lock lock(classWideMutex);

			scene = s;

			if (scene->IsFilmOnly()) {
				state = TERMINATE;
				return;
			}

			if (scene->lights.size() == 0) {
				LOG(LUX_SEVERE, LUX_MISSINGDATA) << "No light sources defined in scene; nothing to render.";
				state = TERMINATE;
				return;
			}

			state = RUN;

			// Initialize the stats
			rendererStatistics->reset();

			// Dade - I have to do initiliaziation here for the current thread.
			// It can be used by the Preprocess() methods.

			// initialize the thread's rangen
			u_long seed = scene->seedBase - 1;
			LOG(LUX_DEBUG, LUX_NOERROR) << "Preprocess thread uses seed: " << seed;

			RandomGenerator rng(seed);

			// integrator preprocessing
			scene->sampler->SetFilm(scene->camera()->film);
			scene->surfaceIntegrator->Preprocess(rng, *scene);
			scene->volumeIntegrator->Preprocess(rng, *scene);
			scene->camera()->film->CreateBuffers();

			scene->surfaceIntegrator->RequestSamples(scene->sampler, *scene);
			scene->volumeIntegrator->RequestSamples(scene->sampler, *scene);

			// Dade - to support autofocus for some camera model
			scene->camera()->AutoFocus(*scene);

			// TODO: extend SLG library to accept an handler for each rendering session
			luxrays::sdl::LuxRaysSDLDebugHandler = SDLDebugHandler;

			try {
				// Build the SLG rendering configuration
				slgConfigProps.Load(CreateSLGConfig());

				// Build the SLG scene to render
				slgScene = CreateSLGScene(slgConfigProps);
#if !defined(LUXRAYS_DISABLE_OPENCL)
			} catch (cl::Error err) {
				LOG(LUX_SEVERE, LUX_SYSTEM) << "OpenCL ERROR: " << err.what() << "(" << luxrays::utils::oclErrorString(err.err()) << ")";
				state = TERMINATE;
				return;
#endif
			} catch (std::runtime_error err) {
				LOG(LUX_SEVERE, LUX_SYSTEM) << "RUNTIME ERROR: " << err.what();
				state = TERMINATE;
				return;
			} catch (std::exception err) {
				LOG(LUX_SEVERE, LUX_SYSTEM) << "ERROR: " << err.what();
				state = TERMINATE;
				return;
			}
		}

		//----------------------------------------------------------------------
		// Initialize the render session
		//----------------------------------------------------------------------

		SLGStatistics *slgStats = static_cast<SLGStatistics *>(rendererStatistics);

		slg::RenderConfig *config = new slg::RenderConfig(slgConfigProps, *slgScene);
		slg::RenderSession *session = new slg::RenderSession(config);
		slg::RenderEngine *engine = session->renderEngine;
		engine->SetSeed(scene->seedBase);

		const vector<luxrays::IntersectionDevice *> &devices = engine->GetIntersectionDevices();
		slgStats->deviceCount = devices.size();
		if (slgStats->deviceCount) {
			// I assume the amount of used memory is the same on all devices. It is
			// always true for the moment.
			slgStats->deviceMemoryUsed = devices[0]->GetUsedMemory();
		}

		// start the timer
		slgStats->start();

		// Dade - preprocessing done
		preprocessDone = true;
		scene->SetReady();

		//----------------------------------------------------------------------
		// Start the rendering
		//----------------------------------------------------------------------

		session->Start();
		const double startTime = luxrays::WallClockTime();

		unsigned int haltTime = config->cfg.GetInt("batch.halttime", 0);
		unsigned int haltSpp = config->cfg.GetInt("batch.haltspp", 0);
		float haltThreshold = config->cfg.GetFloat("batch.haltthreshold", -1.f);

		double lastFilmUpdate = startTime - 2.0; // -2.0 is to anticipate the first display update by 2 secs
		char buf[512];
		Film *film = scene->camera()->film;
		int xStart, xEnd, yStart, yEnd;
		film->GetSampleExtent(&xStart, &xEnd, &yStart, &yEnd);

		// Used to feed LuxRender film with only the delta information from the previous update
		const u_int slgFilmWidth = session->film->GetWidth();
		const u_int slgFilmHeight = session->film->GetHeight();

		if (session->film->HasPerPixelNormalizedBuffer()) {
			previousEyeBufferRadiance = new BlockedArray<luxrays::Spectrum>(slgFilmWidth, slgFilmHeight);
			previousEyeWeight = new BlockedArray<float>(slgFilmWidth, slgFilmHeight);
			previousAlphaBuffer = new BlockedArray<float>(slgFilmWidth, slgFilmHeight);

			for (u_int y = 0; y < slgFilmHeight; ++y) {
				for (u_int x = 0; x < slgFilmWidth; ++x) {
					(*previousEyeWeight)(x, y) = 0.f;
					(*previousAlphaBuffer)(x, y) = 0.f;
				}
			}
		}

		if (session->film->HasPerScreenNormalizedBuffer()) {
			previousLightBufferRadiance = new BlockedArray<luxrays::Spectrum>(slgFilmWidth, slgFilmHeight);
			previousLightWeight = new BlockedArray<float>(slgFilmWidth, slgFilmHeight);

			for (u_int y = 0; y < slgFilmHeight; ++y) {
				for (u_int x = 0; x < slgFilmWidth; ++x)
					(*previousLightWeight)(x, y) = 0.f;
			}
		}

		previousSampleCount = 0.0;

		for (;;) {
			if (state == PAUSE) {
				session->BeginEdit();
				while (state == PAUSE && !boost::this_thread::interruption_requested())
					boost::this_thread::sleep(boost::posix_time::seconds(1));
				session->EndEdit();
			}
			if ((state == TERMINATE) || boost::this_thread::interruption_requested())
				break;

			boost::this_thread::sleep(boost::posix_time::millisec(1000));

			// Film update may be required by some render engine to
			// update statistics, convergence test and more
			if (luxrays::WallClockTime() - lastFilmUpdate > film->getldrDisplayInterval()) {
				session->renderEngine->UpdateFilm();

				// Update LuxRender film too
				UpdateLuxFilm(session);

				lastFilmUpdate =  luxrays::WallClockTime();
			}

			const double now = luxrays::WallClockTime();
			const double elapsedTime = now - startTime;
			const unsigned int pass = engine->GetPass();
			// Convergence test is update inside UpdateFilm()
			const float convergence = engine->GetConvergence();
			if (((film->enoughSamplesPerPixel)) ||
					((haltTime > 0) && (elapsedTime >= haltTime)) ||
					((haltSpp > 0) && (pass >= haltSpp)) ||
					((haltThreshold >= 0.f) && (1.f - convergence <= haltThreshold))) {

				if (suspendThreadsWhenDone) {
					// Dade - wait for a resume rendering or exit
					Pause();
					while (state == PAUSE)
						boost::this_thread::sleep(boost::posix_time::millisec(1000));

					if (state == TERMINATE)
						break;
					else {
						// Cancel all halt conditions
						haltTime = 0;
						haltSpp = 0;
						haltThreshold = -1.f;
					}
				} else {
					Terminate();
					break;
				}
				break;
			}

			// Update statistics
			slgStats->averageSampleSec = engine->GetTotalSamplesSec();

			// Print some information about the rendering progress
			sprintf(buf, "[Elapsed time: %3d/%dsec][Samples %4d/%d][Convergence %f%%][Avg. samples/sec % 3.2fM on %.1fK tris]",
					int(elapsedTime), int(haltTime), pass, haltSpp, 100.f * convergence, slgStats->averageSampleSec / 1000000.0,
					config->scene->dataSet->GetTotalTriangleCount() / 1000.0);

			SLG_LOG(buf);

			film->CheckWriteOuputInterval();
		}

		// Stop the rendering
		session->Stop();
		delete session;
#if !defined(LUXRAYS_DISABLE_OPENCL)
	} catch (cl::Error err) {
		LOG(LUX_SEVERE, LUX_SYSTEM) << "OpenCL ERROR: " << err.what() << "(" << luxrays::utils::oclErrorString(err.err()) << ")";
#endif
	} catch (std::runtime_error err) {
		LOG(LUX_SEVERE, LUX_SYSTEM) << "RUNTIME ERROR: " << err.what();
	} catch (std::exception err) {
		LOG(LUX_SEVERE, LUX_SYSTEM) << "ERROR: " << err.what();
	}

	delete previousEyeBufferRadiance;
	previousEyeBufferRadiance = NULL;
	delete previousEyeWeight;
	previousEyeWeight = NULL;
	delete previousLightBufferRadiance;
	previousLightBufferRadiance = NULL;
	delete previousLightWeight;
	previousLightWeight = NULL;

	// Free allocated normals
	for (u_int i = 0; i < alloctedMeshNormals.size(); ++i)
		delete[] alloctedMeshNormals[i];

	SLG_LOG("Done.");

	// I change the current signal to exit in order to disable the creation
	// of new threads after this point
	Terminate();

	// Flush the contribution pool
	scene->camera()->film->contribPool->Flush();
	scene->camera()->film->contribPool->Delete();
}

void SLGRenderer::Pause() {
	boost::mutex::scoped_lock lock(classWideMutex);
	state = PAUSE;
	rendererStatistics->stop();
}

void SLGRenderer::Resume() {
	boost::mutex::scoped_lock lock(classWideMutex);
	state = RUN;
	rendererStatistics->start();
}

void SLGRenderer::Terminate() {
	boost::mutex::scoped_lock lock(classWideMutex);
	state = TERMINATE;
}

//------------------------------------------------------------------------------

void DebugHandler(const char *msg) {
	LOG(LUX_DEBUG, LUX_NOERROR) << "[LuxRays] " << msg;
}

void SDLDebugHandler(const char *msg) {
	LOG(LUX_DEBUG, LUX_NOERROR) << "[LuxRays::SDL] " << msg;
}

void SLGDebugHandler(const char *msg) {
	LOG(LUX_DEBUG, LUX_NOERROR) << "[SLG] " << msg;
}

Renderer *SLGRenderer::CreateRenderer(const ParamSet &params) {
	luxrays::Properties config;

	// Local (for network rendering) host configuration file. It is a properties
	// file that can be used overwrite settings.
	const string configFile = params.FindOneString("localconfigfile", "");
	if (configFile != "")
		config.LoadFromFile(configFile);

	// A list of properties that can be used to overwrite generated properties
	u_int nItems;
	const string *items = params.FindString("config", &nItems);
	if (items) {
		for (u_int i = 0; i < nItems; ++i)
			config.LoadFromString(items[i] + "\n");
	}

	return new SLGRenderer(config);
}

static DynamicLoader::RegisterRenderer<SLGRenderer> r("slg");
