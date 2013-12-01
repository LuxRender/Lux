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

#include <iomanip>
#include <fstream>
#include <typeinfo>
#include <sstream>

#include <boost/regex.hpp>
#include <boost/format.hpp>
#include <boost/lexical_cast.hpp>
#include <boost/algorithm/string/replace.hpp>

#include "luxrays/core/context.h"
#include "luxrays/core/exttrianglemesh.h"
#include "luxrays/utils/ocl.h"
#include "luxrays/core/virtualdevice.h"
#include "luxcore/luxcore.h"

#include "api.h"
#include "scene.h"
#include "camera.h"
#include "film.h"
#include "sampling.h"
#include "context.h"
#include "luxcorerenderer.h"
#include "renderers/statistics/luxcorestatistics.h"
#include "cameras/perspective.h"
#include "shape.h"
#include "volume.h"
#include "filter.h"

#include "accelerators/qbvhaccel.h"
#include "accelerators/bvhaccel.h"
#include "accelerators/tabreckdtreeaccel.h"

#include "samplers/metrosampler.h"
#include "samplers/random.h"
#include "samplers/lowdiscrepancy.h"
#include "samplers/sobol.h"

#include "integrators/path.h"
#include "integrators/bidirectional.h"

#include "light.h"
#include "lights/sun.h"
#include "lights/sky.h"
#include "lights/sky2.h"
#include "lights/infinite.h"
#include "lights/infinitesample.h"
#include "lights/pointlight.h"
#include "lights/spot.h"
#include "lights/projection.h"
#include "lights/distant.h"

#include "materials/matte.h"
#include "materials/mirror.h"
#include "materials/glass.h"
#include "materials/glass2.h"
#include "materials/glossy2.h"
#include "materials/metal.h"
#include "materials/mattetranslucent.h"
#include "materials/null.h"
#include "materials/mixmaterial.h"
#include "materials/metal2.h"
#include "materials/roughglass.h"

#include "textures/tabulatedfresnel.h"
#include "textures/fresnelcolor.h"
#include "textures/checkerboard.h"
#include "textures/mix.h"
#include "textures/fbm.h"
#include "textures/marble.h"
#include "textures/blackbody.h"
#include "textures/constant.h"
#include "textures/imagemap.h"
#include "textures/scale.h"
#include "textures/dots.h"
#include "textures/brick.h"
#include "textures/add.h"
#include "textures/windy.h"
#include "textures/wrinkled.h"
#include "textures/uv.h"
#include "textures/band.h"
#include "textures/hitpointcolor.h"
#include "textures/blender_wood.h"

#include "volumes/clearvolume.h"
#include "film/fleximage.h"
#include "sphericalfunction.h"

using namespace lux;

template <class T> static string ToClassName(T *ptr) {
	if (ptr)
		return typeid(*ptr).name();
	else
		return "NULL";
}

//------------------------------------------------------------------------------
// LuxCoreHostDescription
//------------------------------------------------------------------------------

LuxCoreHostDescription::LuxCoreHostDescription(LuxCoreRenderer *r, const string &n) : renderer(r), name(n) {
	LuxCoreDeviceDescription *desc = new LuxCoreDeviceDescription(this, "LuxCore");
	devs.push_back(desc);
}

LuxCoreHostDescription::~LuxCoreHostDescription() {
	for (size_t i = 0; i < devs.size(); ++i)
		delete devs[i];
}

void LuxCoreHostDescription::AddDevice(LuxCoreDeviceDescription *devDesc) {
	devs.push_back(devDesc);
}

//------------------------------------------------------------------------------
// LuxCoreRenderer
//------------------------------------------------------------------------------

LuxCoreRenderer::LuxCoreRenderer(const luxrays::Properties &config) : Renderer() {
	state = INIT;

	LuxCoreHostDescription *host = new LuxCoreHostDescription(this, "Localhost");
	hosts.push_back(host);

	preprocessDone = false;
	suspendThreadsWhenDone = false;
	previousFilm_ALPHA = NULL;
	previousFilm_DEPTH = NULL;

	AddStringConstant(*this, "name", "Name of current renderer", "luxcore");

	rendererStatistics = new LuxCoreStatistics(this);

	overwriteConfig = config;
	renderEngineType = "PATHOCL";
}

LuxCoreRenderer::~LuxCoreRenderer() {
	boost::mutex::scoped_lock lock(classWideMutex);

	if ((state != TERMINATE) && (state != INIT))
		throw std::runtime_error("Internal error: called LuxCoreRenderer::~LuxCoreRenderer() while not in TERMINATE or INIT state.");

	delete rendererStatistics;
	BOOST_FOREACH(float *ptr, previousFilm_RADIANCE_PER_PIXEL_NORMALIZEDs)
		delete[] ptr;
	BOOST_FOREACH(float *ptr, previousFilm_RADIANCE_PER_SCREEN_NORMALIZEDs)
		delete[] ptr;
	delete[] previousFilm_ALPHA;
	delete[] previousFilm_DEPTH;

	for (size_t i = 0; i < hosts.size(); ++i)
		delete hosts[i];
}

Renderer::RendererState LuxCoreRenderer::GetState() const {
	boost::mutex::scoped_lock lock(classWideMutex);

	return state;
}

vector<RendererHostDescription *> &LuxCoreRenderer::GetHostDescs() {
	boost::mutex::scoped_lock lock(classWideMutex);

	return hosts;
}

void LuxCoreRenderer::SuspendWhenDone(bool v) {
	boost::mutex::scoped_lock lock(classWideMutex);
	suspendThreadsWhenDone = v;
}
			
//------------------------------------------------------------------------------
// Channels: integer types
//------------------------------------------------------------------------------

template <typename T, u_int channels> string GetLuxCoreImageMapNameImpl(luxcore::Scene *lcScene,
		const MIPMapFastImpl<TextureColor<T, channels> > *mipMap,
		const float gamma) {
	// Check if the image map has already been defined
	const string imageMapName = mipMap->GetName();
	if (lcScene->IsImageMapDefined(imageMapName))
		return imageMapName;

	const BlockedArray<TextureColor<T, channels> > *map = mipMap->GetSingleMap();

	float *lcMap = new float[map->uSize() * map->vSize() * channels];
	float *mapPtr = lcMap;
	for (u_int y = 0; y < map->vSize(); ++y) {
		for (u_int x = 0; x < map->uSize(); ++x) {
			const TextureColor<T, channels> &col = (*map)(x, y);

			for (u_int i = 0; i < channels; ++i)
				*mapPtr++ = powf(col.c[i] / 255.f, gamma);
		}
	}

	lcScene->DefineImageMap(imageMapName, lcMap, gamma, channels,
			(u_int)map->uSize(), (u_int)map->vSize());

	return imageMapName;
}

//------------------------------------------------------------------------------
// Channels: floats
// 
// Partial function specialization are not allowed in C++
//------------------------------------------------------------------------------

template <> string GetLuxCoreImageMapNameImpl<float, 1>(luxcore::Scene *lcScene,
		const MIPMapFastImpl<TextureColor<float, 1> > *mipMap,
		const float gamma) {
	// Check if the image map has already been defined
	const string imageMapName = mipMap->GetName();
	if (lcScene->IsImageMapDefined(imageMapName))
		return imageMapName;

	const BlockedArray<TextureColor<float, 1> > *map = mipMap->GetSingleMap();

	float *lcMap = new float[map->uSize() * map->vSize() * 1];
	float *mapPtr = lcMap;
	for (u_int y = 0; y < map->vSize(); ++y) {
		for (u_int x = 0; x < map->uSize(); ++x) {
			const TextureColor<float, 1> &col = (*map)(x, y);

			*mapPtr++ = powf(col.c[0], gamma);
		}
	}

	lcScene->DefineImageMap(imageMapName, lcMap, gamma, 1,
			(u_int)map->uSize(), (u_int)map->vSize());

	return imageMapName;
}

template <> string GetLuxCoreImageMapNameImpl<float, 3>(luxcore::Scene *lcScene,
		const MIPMapFastImpl<TextureColor<float, 3> > *mipMap,
		const float gamma) {
	// Check if the image map has already been defined
	const string imageMapName = mipMap->GetName();
	if (lcScene->IsImageMapDefined(imageMapName))
		return imageMapName;

	const BlockedArray<TextureColor<float, 3> > *map = mipMap->GetSingleMap();

	float *lcMap = new float[map->uSize() * map->vSize() * 3];
	float *mapPtr = lcMap;
	for (u_int y = 0; y < map->vSize(); ++y) {
		for (u_int x = 0; x < map->uSize(); ++x) {
			const TextureColor<float, 3> &col = (*map)(x, y);

			*mapPtr++ = powf(col.c[0], gamma);
			*mapPtr++ = powf(col.c[1], gamma);
			*mapPtr++ = powf(col.c[2], gamma);
		}
	}

	lcScene->DefineImageMap(imageMapName, lcMap, gamma, 3,
			(u_int)map->uSize(), (u_int)map->vSize());

	return imageMapName;
}

template <> string GetLuxCoreImageMapNameImpl<float, 4>(luxcore::Scene *lcScene,
		const MIPMapFastImpl<TextureColor<float, 4> > *mipMap,
		const float gamma) {
	// Check if the image map has already been defined
	const string imageMapName = mipMap->GetName();
	if (lcScene->IsImageMapDefined(imageMapName))
		return imageMapName;

	const BlockedArray<TextureColor<float, 4> > *map = mipMap->GetSingleMap();

	float *lcMap = new float[map->uSize() * map->vSize() * 4];
	float *mapPtr = lcMap;
	for (u_int y = 0; y < map->vSize(); ++y) {
		for (u_int x = 0; x < map->uSize(); ++x) {
			const TextureColor<float, 4> &col = (*map)(x, y);

			*mapPtr++ = powf(col.c[0], gamma);
			*mapPtr++ = powf(col.c[1], gamma);
			*mapPtr++ = powf(col.c[2], gamma);
			*mapPtr++ = col.c[3];
		}
	}

	lcScene->DefineImageMap(imageMapName, lcMap, gamma, 4,
			(u_int)map->uSize(), (u_int)map->vSize());

	return imageMapName;
}

//------------------------------------------------------------------------------

static string GetLuxCoreDefaultImageMap(luxcore::Scene *lcScene) {
	if (!lcScene->IsImageMapDefined("imagemap_default")) {
		float *map = new float[1];
		map[0] = 1.f;
		lcScene->DefineImageMap("imagemap_default", map, 1.f, 1, 1, 1);
	}

	return "imagemap_default";
}

static string GetLuxCoreImageMapName(luxcore::Scene *lcScene,
		const MIPMap *mipMap, const float gamma) {
	if (!mipMap)
		return GetLuxCoreDefaultImageMap(lcScene);

	//--------------------------------------------------------------------------
	// Channels: unsigned char
	//--------------------------------------------------------------------------
	if (dynamic_cast<const MIPMapFastImpl<TextureColor<unsigned char, 1> > *>(mipMap))
		return GetLuxCoreImageMapNameImpl(lcScene, (MIPMapImpl<TextureColor<unsigned char, 1> > *)mipMap, gamma);
	if (dynamic_cast<const MIPMapFastImpl<TextureColor<unsigned char, 3> > *>(mipMap))
		return GetLuxCoreImageMapNameImpl(lcScene, (MIPMapImpl<TextureColor<unsigned char, 3> > *)mipMap, gamma);
	if (dynamic_cast<const MIPMapFastImpl<TextureColor<unsigned char, 4> > *>(mipMap))
		return GetLuxCoreImageMapNameImpl(lcScene, (MIPMapImpl<TextureColor<unsigned char, 4> > *)mipMap, gamma);
	//--------------------------------------------------------------------------
	// Channels: unsigned short
	//--------------------------------------------------------------------------
	if (dynamic_cast<const MIPMapFastImpl<TextureColor<unsigned short, 1> > *>(mipMap))
		return GetLuxCoreImageMapNameImpl(lcScene, (MIPMapImpl<TextureColor<unsigned short, 1> > *)mipMap, gamma);
	if (dynamic_cast<const MIPMapFastImpl<TextureColor<unsigned short, 3> > *>(mipMap))
		return GetLuxCoreImageMapNameImpl(lcScene, (MIPMapImpl<TextureColor<unsigned short, 3> > *)mipMap, gamma);
	if (dynamic_cast<const MIPMapFastImpl<TextureColor<unsigned short, 4> > *>(mipMap))
		return GetLuxCoreImageMapNameImpl(lcScene, (MIPMapImpl<TextureColor<unsigned short, 4> > *)mipMap, gamma);
	//--------------------------------------------------------------------------
	// Channels: float
	//--------------------------------------------------------------------------
	else if (dynamic_cast<const MIPMapFastImpl<TextureColor<float, 1> > *>(mipMap))
		return GetLuxCoreImageMapNameImpl(lcScene, (MIPMapImpl<TextureColor<float, 1> > *)mipMap, gamma);
	else if (dynamic_cast<const MIPMapFastImpl<TextureColor<float, 3> > *>(mipMap))
		return GetLuxCoreImageMapNameImpl(lcScene, (MIPMapImpl<TextureColor<float, 3> > *)mipMap, gamma);
	else if (dynamic_cast<const MIPMapFastImpl<TextureColor<float, 4> > *>(mipMap))
		return GetLuxCoreImageMapNameImpl(lcScene, (MIPMapImpl<TextureColor<float, 4> > *)mipMap, gamma);
	else {
		// Unsupported type
		LOG(LUX_WARNING, LUX_UNIMPLEMENT) << "LuxCoreRenderer supports only RGB(A) float texture maps (i.e. not " <<
					ToClassName(mipMap) << "). Replacing an unsupported texture map with a white texture.";
		return GetLuxCoreDefaultImageMap(lcScene);
	}
}

//------------------------------------------------------------------------------

template<class T> luxrays::Properties GetLuxCoreTexMapping(const T *mapping, const string &prefix) {
	if (mapping) {
		if (dynamic_cast<const UVMapping2D *>(mapping)) {
			const UVMapping2D *uvMapping2D = dynamic_cast<const UVMapping2D *>(mapping);
			return luxrays::Properties() <<
					luxrays::Property(prefix + ".mapping.type")("uvmapping2d") <<
					luxrays::Property(prefix + ".mapping.uvscale")(uvMapping2D->GetUScale(), uvMapping2D->GetVScale()) <<
					luxrays::Property(prefix + ".mapping.uvdelta")(uvMapping2D->GetUDelta(), uvMapping2D->GetVDelta());
		} else if (dynamic_cast<const UVMapping3D *>(mapping)) {
			const UVMapping3D *uvMapping3D = dynamic_cast<const UVMapping3D *>(mapping);
			return prefix + ".mapping.type = uvmapping3d\n" +
					prefix + ".mapping.transformation = " + ToString(uvMapping3D->WorldToTexture.m) + "\n";
		} else if (dynamic_cast<const GlobalMapping3D *>(mapping)) {
			const GlobalMapping3D *globalMapping3D = dynamic_cast<const GlobalMapping3D *>(mapping);
			return luxrays::Properties() <<
					luxrays::Property(prefix + ".mapping.type")("globalmapping3d") <<
					luxrays::Property(prefix + ".mapping.transformation")(globalMapping3D->WorldToTexture.m);
		} else {
			LOG(LUX_WARNING, LUX_UNIMPLEMENT) << "LuxCoreRenderer supports only texture coordinate mapping with UVMapping2D, UVMapping3D and GlobalMapping3D (i.e. not " <<
					ToClassName(mapping) << "). Ignoring the mapping.";
		}
	}

	return luxrays::Properties();
}

template<class T> string GetLuxCoreTexName(luxcore::Scene *lcScene,
		const Texture<T> *tex) {
	LOG(LUX_DEBUG, LUX_NOERROR) << "Texture type: " << ToClassName(tex);

	const string texName = tex->GetName();
	// Check if the texture has already been defined
	if (!lcScene->IsTextureDefined(texName)) {
		luxrays::Properties texProps;
		//----------------------------------------------------------------------
		// ImageMap texture
		//----------------------------------------------------------------------
		if (dynamic_cast<const ImageSpectrumTexture *>(tex)) {
			const ImageSpectrumTexture *imgTex = dynamic_cast<const ImageSpectrumTexture *>(tex);

			const TexInfo &texInfo = imgTex->GetInfo();
			const string imageMapName = GetLuxCoreImageMapName(lcScene, imgTex->GetMIPMap(), texInfo.gamma);

			texProps << luxrays::Property("scene.textures." + texName + ".type")("imagemap") <<
					luxrays::Property("scene.textures." + texName + ".file")(imageMapName) <<
					luxrays::Property("scene.textures." + texName + ".gamma")(texInfo.gamma) <<
					// LuxRender applies gain before gamma correction
					luxrays::Property("scene.textures." + texName + ".gain")(powf(texInfo.gain, texInfo.gamma)) <<
					GetLuxCoreTexMapping(imgTex->GetTextureMapping2D(), "scene.textures." + texName);
		} else if (dynamic_cast<const ImageFloatTexture *>(tex)) {
			const ImageFloatTexture *imgTex = dynamic_cast<const ImageFloatTexture *>(tex);

			const TexInfo &texInfo = imgTex->GetInfo();
			const string imageMapName = GetLuxCoreImageMapName(lcScene, imgTex->GetMIPMap(), texInfo.gamma);

			texProps << luxrays::Property("scene.textures." + texName + ".type")("imagemap") <<
					luxrays::Property("scene.textures." + texName + ".file")(imageMapName) <<
					luxrays::Property("scene.textures." + texName + ".gamma")(texInfo.gamma) <<
					// LuxRender applies gain before gamma correction
					luxrays::Property("scene.textures." + texName + ".gain")(powf(texInfo.gain, texInfo.gamma)) <<
					GetLuxCoreTexMapping(imgTex->GetTextureMapping2D(), "scene.textures." + texName);
		} else
		//----------------------------------------------------------------------
		// Constant texture
		//----------------------------------------------------------------------
		if (dynamic_cast<const ConstantRGBColorTexture *>(tex)) {
			const ConstantRGBColorTexture *constRGBTex = dynamic_cast<const ConstantRGBColorTexture *>(tex);

			texProps << luxrays::Property("scene.textures." + texName + ".type")("constfloat3") <<
					luxrays::Property("scene.textures." + texName + ".value")(
						(*constRGBTex)["color.r"].FloatValue(),
						(*constRGBTex)["color.g"].FloatValue(),
						(*constRGBTex)["color.b"].FloatValue());
		} else if (dynamic_cast<const ConstantFloatTexture *>(tex)) {
			const ConstantFloatTexture *constFloatTex = dynamic_cast<const ConstantFloatTexture *>(tex);

			texProps << luxrays::Property("scene.textures." + texName + ".type")("constfloat1") <<
					luxrays::Property("scene.textures." + texName + ".value")(
						(*constFloatTex)["value"].FloatValue());
		} else
		//----------------------------------------------------------------------
		// NormalMap texture
		//----------------------------------------------------------------------
		if (dynamic_cast<const NormalMapTexture *>(tex)) {
			const NormalMapTexture *normalTex = dynamic_cast<const NormalMapTexture *>(tex);

			const TexInfo &texInfo = normalTex->GetInfo();
			const string imageMapName = GetLuxCoreImageMapName(lcScene, normalTex->GetMIPMap(), texInfo.gamma);

			texProps << luxrays::Property("scene.textures." + texName + ".type")("imagemap") <<
					luxrays::Property("scene.textures." + texName + ".file")(imageMapName) <<
					luxrays::Property("scene.textures." + texName + ".gamma")(texInfo.gamma) <<
					luxrays::Property("scene.textures." + texName + ".gain")(texInfo.gain) <<
					GetLuxCoreTexMapping(normalTex->GetTextureMapping2D(), "scene.textures." + texName);
		} else
		//----------------------------------------------------------------------
		// Add texture
		//----------------------------------------------------------------------
		if (dynamic_cast<const AddTexture<T, T> *>(tex)) {
			const AddTexture<T, T> *scaleTex = dynamic_cast<const AddTexture<T, T> *>(tex);
			const string tex1Name = GetLuxCoreTexName(lcScene, scaleTex->GetTex1());
			const string tex2Name = GetLuxCoreTexName(lcScene, scaleTex->GetTex2());

			texProps << luxrays::Property("scene.textures." + texName + ".type")("add") <<
					luxrays::Property("scene.textures." + texName + ".texture1")(tex1Name) <<
					luxrays::Property("scene.textures." + texName + ".texture2")(tex2Name);
		} else
		//----------------------------------------------------------------------
		// Scale texture
		//
		// The following checks are required because VC++ seems unable to handle
		// two "if" with ScaleTexture<T, float>/ScaleTexture<T, SWCSpectrum>
		// conditions.
		//----------------------------------------------------------------------
		if (dynamic_cast<const ScaleTexture<T, T> *>(tex)) {
			const ScaleTexture<T, T> *scaleTex = dynamic_cast<const ScaleTexture<T, T> *>(tex);
			const string tex1Name = GetLuxCoreTexName(lcScene, scaleTex->GetTex1());
			const string tex2Name = GetLuxCoreTexName(lcScene, scaleTex->GetTex2());

			texProps << luxrays::Property("scene.textures." + texName + ".type")("scale") <<
					luxrays::Property("scene.textures." + texName + ".texture1")(tex1Name) <<
					luxrays::Property("scene.textures." + texName + ".texture2")(tex2Name);
		} else if (dynamic_cast<const ScaleTexture<float, SWCSpectrum> *>(tex)) {
			const ScaleTexture<float, SWCSpectrum> *scaleTex = dynamic_cast<const ScaleTexture<float, SWCSpectrum> *>(tex);
			const string tex1Name = GetLuxCoreTexName(lcScene, scaleTex->GetTex1());
			const string tex2Name = GetLuxCoreTexName(lcScene, scaleTex->GetTex2());

			texProps << luxrays::Property("scene.textures." + texName + ".type")("scale") <<
					luxrays::Property("scene.textures." + texName + ".texture1")(tex1Name) <<
					luxrays::Property("scene.textures." + texName + ".texture2")(tex2Name);
		} else
		//----------------------------------------------------------------------
		// Mix texture
		//----------------------------------------------------------------------
		if (dynamic_cast<const MixTexture<T> *>(tex)) {
			const MixTexture<T> *mixTex = dynamic_cast<const MixTexture<T> *>(tex);
			const string amountTexName = GetLuxCoreTexName(lcScene, mixTex->GetAmountTex());
			const string tex1Name = GetLuxCoreTexName(lcScene, mixTex->GetTex1());
			const string tex2Name = GetLuxCoreTexName(lcScene, mixTex->GetTex2());

			texProps << luxrays::Property("scene.textures." + texName + ".type")("mix") <<
					luxrays::Property("scene.textures." + texName + ".amount")(amountTexName) <<
					luxrays::Property("scene.textures." + texName + ".texture1")(tex1Name) <<
					luxrays::Property("scene.textures." + texName + ".texture2")(tex2Name);
		} else
		//----------------------------------------------------------------------
		// Procedural textures
		//----------------------------------------------------------------------
		if (dynamic_cast<const Checkerboard2D *>(tex)) {
			const Checkerboard2D *checkerboard2D = dynamic_cast<const Checkerboard2D *>(tex);

			const string tex1Name = GetLuxCoreTexName(lcScene, checkerboard2D->GetTex1());
			const string tex2Name = GetLuxCoreTexName(lcScene, checkerboard2D->GetTex2());

			texProps << luxrays::Property("scene.textures." + texName + ".type")("checkerboard2d") <<
					luxrays::Property("scene.textures." + texName + ".texture1")(tex1Name) <<
					luxrays::Property("scene.textures." + texName + ".texture2")(tex2Name) <<
					GetLuxCoreTexMapping(checkerboard2D->GetTextureMapping2D(), "scene.textures." + texName);
		} else if (dynamic_cast<const Checkerboard3D *>(tex)) {
			const Checkerboard3D *checkerboard3D = dynamic_cast<const Checkerboard3D *>(tex);

			const string tex1Name = GetLuxCoreTexName(lcScene, checkerboard3D->GetTex1());
			const string tex2Name = GetLuxCoreTexName(lcScene, checkerboard3D->GetTex2());

			texProps << luxrays::Property("scene.textures." + texName + ".type")("checkerboard3d") <<
					luxrays::Property("scene.textures." + texName + ".texture1")(tex1Name) <<
					luxrays::Property("scene.textures." + texName + ".texture2")(tex2Name) <<
					GetLuxCoreTexMapping(checkerboard3D->GetTextureMapping3D(), "scene.textures." + texName);
		} else if (dynamic_cast<const FBmTexture *>(tex)) {
			const FBmTexture *fbm = dynamic_cast<const FBmTexture *>(tex);

			texProps << luxrays::Property("scene.textures." + texName + ".type")("fbm") <<
					luxrays::Property("scene.textures." + texName + ".octaves")(fbm->GetOctaves()) <<
					luxrays::Property("scene.textures." + texName + ".roughness")(fbm->GetOmega()) <<
					GetLuxCoreTexMapping(fbm->GetTextureMapping3D(), "scene.textures." + texName);
		} else if (dynamic_cast<const MarbleTexture *>(tex)) {
			const MarbleTexture *marble = dynamic_cast<const MarbleTexture *>(tex);

			texProps << luxrays::Property("scene.textures." + texName + ".type")("marble") <<
					luxrays::Property("scene.textures." + texName + ".octaves")(marble->GetOctaves()) <<
					luxrays::Property("scene.textures." + texName + ".roughness")(marble->GetOmega()) <<
					luxrays::Property("scene.textures." + texName + ".scale")(marble->GetScale()) <<
					luxrays::Property("scene.textures." + texName + ".variation")(marble->GetVariation()) <<
					GetLuxCoreTexMapping(marble->GetTextureMapping3D(), "scene.textures." + texName);
		} else if (dynamic_cast<const DotsTexture *>(tex)) {
			const DotsTexture *dotsTex = dynamic_cast<const DotsTexture *>(tex);
			const string insideName = GetLuxCoreTexName(lcScene, dotsTex->GetInsideTex());
			const string outsideName = GetLuxCoreTexName(lcScene, dotsTex->GetOutsideTex());

			texProps << luxrays::Property("scene.textures." + texName + ".type")("dots") <<
					luxrays::Property("scene.textures." + texName + ".inside")(insideName) <<
					luxrays::Property("scene.textures." + texName + ".outside")(outsideName) <<
					GetLuxCoreTexMapping(dotsTex->GetTextureMapping2D(), "scene.textures." + texName);
		} else if (dynamic_cast<const BrickTexture3D<T> *>(tex)) {
			const BrickTexture3D<T> *brickTex = dynamic_cast<const BrickTexture3D<T> *>(tex);

			string brickbond = "running";
			switch (brickTex->GetBond()) {
				case FLEMISH:
					brickbond = "flemish";
					break;
				case RUNNING:
					brickbond = "running";
					break;
				case ENGLISH:
					brickbond = "english";
					break;
				case HERRINGBONE:
					brickbond = "herringbone";
					break;
				case BASKET:
					brickbond = "basket";
					break;
				case KETTING:
					brickbond = "ketting";
					break;
				default:
					brickbond = "running";
					break;
			}

			const string tex1Name = GetLuxCoreTexName(lcScene, brickTex->GetTex1());
			const string tex2Name = GetLuxCoreTexName(lcScene, brickTex->GetTex2());
			const string tex3Name = GetLuxCoreTexName(lcScene, brickTex->GetTex3());

			texProps << luxrays::Property("scene.textures." + texName + ".type")("brick") <<
					luxrays::Property("scene.textures." + texName + ".brickbond")(brickbond) <<
					luxrays::Property("scene.textures." + texName + ".brickwidth")(brickTex->GetBrickWidth()) <<
					luxrays::Property("scene.textures." + texName + ".brickheight")(brickTex->GetBrickHeight()) <<
					luxrays::Property("scene.textures." + texName + ".brickdepth")(brickTex->GetBrickDepth()) <<
					luxrays::Property("scene.textures." + texName + ".mortarsize")(brickTex->GetMortarSize()) <<
					luxrays::Property("scene.textures." + texName + ".brickrun")(brickTex->GetBrickRun()) <<
					luxrays::Property("scene.textures." + texName + ".brickbevel")(brickTex->GetBrickBevel()) <<
					luxrays::Property("scene.textures." + texName + ".bricktex")(tex1Name) <<
					luxrays::Property("scene.textures." + texName + ".mortartex")(tex2Name) <<
					luxrays::Property("scene.textures." + texName + ".brickmodtex")(tex3Name) <<
					GetLuxCoreTexMapping(brickTex->GetTextureMapping3D(), "scene.textures." + texName);
		} else if (dynamic_cast<const WindyTexture *>(tex)) {
			const WindyTexture *windy = dynamic_cast<const WindyTexture *>(tex);

			texProps << luxrays::Property("scene.textures." + texName + ".type")("windy") <<
					GetLuxCoreTexMapping(windy->GetTextureMapping3D(), "scene.textures." + texName);
		} else if (dynamic_cast<const BlenderWoodTexture3D *>(tex)) {
			const BlenderWoodTexture3D *wood = dynamic_cast<const BlenderWoodTexture3D *>(tex);
					std::string woodtype = "";
					switch(wood->GetType()) {
						default:
						case 0:
							woodtype = "bands";
							break;
						case 1:
							woodtype = "rings";
							break;
						case 2:
							woodtype = "bandnoise";
							break;
						case 3:
							woodtype = "ringnoise";
							break;
					}					
					
					std::string noisebasis2= "";
					switch(wood->GetNoiseBasis2()) {
						default:
						case 0:
							noisebasis2 = "sin";
							break;
						case 1:
							noisebasis2 = "saw";
							break;
						case 2:
							noisebasis2 = "tri";
							break;
					}
					std::string noisetype= "soft_noise";
					if(wood->GetNoiseT()) noisetype = "hard_noise";
					texProps << luxrays::Property("scene.textures." + texName + ".type")("wood") <<
					luxrays::Property("scene.textures." + texName + ".woodtype")(woodtype) <<
					luxrays::Property("scene.textures." + texName + ".noisebasis2")(noisebasis2) <<
					luxrays::Property("scene.textures." + texName + ".noisetype")(noisetype) <<
					luxrays::Property("scene.textures." + texName + ".bright")(ToString(wood->GetBright())) <<
					luxrays::Property("scene.textures." + texName + ".contrast")(ToString(wood->GetContrast())) <<
					luxrays::Property("scene.textures." + texName + ".noisesize")(ToString(wood->GetNoiseSize())) <<
					luxrays::Property("scene.textures." + texName + ".turbulence")(ToString(wood->GetTurbulence())) <<
					GetLuxCoreTexMapping(wood->GetTextureMapping3D(), "scene.textures." + texName);

		} else if (dynamic_cast<const WrinkledTexture *>(tex)) {
			const WrinkledTexture *wrinkTex = dynamic_cast<const WrinkledTexture *>(tex);

			texProps << luxrays::Property("scene.textures." + texName + ".type")("wrinkled") <<
					luxrays::Property("scene.textures." + texName + ".octaves")(ToString(wrinkTex->GetOctaves())) <<
					luxrays::Property("scene.textures." + texName + ".roughness")(ToString(wrinkTex->GetOmega())) <<
					GetLuxCoreTexMapping(wrinkTex->GetTextureMapping3D(), "scene.textures." + texName);
		} else if (dynamic_cast<const UVTexture *>(tex)) {
			const UVTexture *uvTex = dynamic_cast<const UVTexture *>(tex);

			texProps << luxrays::Property("scene.textures." + texName + ".type")("uv") <<
					GetLuxCoreTexMapping(uvTex->GetTextureMapping2D(), "scene.textures." + texName);
		} else if (dynamic_cast<const BandTexture<T> *>(tex)) {
			const BandTexture<T> *bandTex = dynamic_cast<const BandTexture<T> *>(tex);
			const string amountTexName = GetLuxCoreTexName(lcScene, bandTex->GetAmountTex());
			const vector<float> &offsets = bandTex->GetOffsets();
			const vector<boost::shared_ptr<Texture<T> > > &texs = bandTex->GetTextures();
			

			texProps << luxrays::Property("scene.textures." + texName + ".type")("band") <<
					luxrays::Property("scene.textures." + texName + ".amount")(amountTexName);

			for (u_int i = 0; i < offsets.size(); ++i) {
				const ConstantRGBColorTexture *constRGBTex = dynamic_cast<const ConstantRGBColorTexture *>(texs[i].get());
				const ConstantFloatTexture *constFloatTex = dynamic_cast<const ConstantFloatTexture *>(texs[i].get());
				if (!constRGBTex && !constFloatTex) {
					LOG(LUX_WARNING, LUX_UNIMPLEMENT) << "LuxCoreRenderer supports only BandTexture with constant values (i.e. not " <<
						ToClassName(texs[i].get()) << ").";
					texProps << luxrays::Property("scene.textures." + texName + ".type")("constfloat1") <<
							luxrays::Property("scene.textures." + texName + ".value")(.7f);
					break;
				}

				texProps << luxrays::Property("scene.textures." + texName + ".offset" + ToString(i))(offsets[i]);
				if (constRGBTex)
					texProps << luxrays::Property("scene.textures." + texName + ".value" + ToString(i))(
							(*constRGBTex)["color.r"].FloatValue(),
							(*constRGBTex)["color.g"].FloatValue(),
							(*constRGBTex)["color.b"].FloatValue());
				if (constFloatTex) {
					const float val = (*constFloatTex)["value"].FloatValue();
					texProps << luxrays::Property("scene.textures." + texName + ".value" + ToString(i))(val, val, val);
				}
			}
		} else if (dynamic_cast<const HitPointRGBColorTexture *>(tex)) {
			texProps << luxrays::Property("scene.textures." + texName + ".type")("hitpointcolor");
		} else if (dynamic_cast<const HitPointAlphaTexture *>(tex)) {
			texProps << luxrays::Property("scene.textures." + texName + ".type")("hitpointalpha");
		} else if (dynamic_cast<const HitPointGreyTexture *>(tex)) {
			const HitPointGreyTexture *hpTex = dynamic_cast<const HitPointGreyTexture *>(tex);

			const int channel = hpTex->GetChannel();
			texProps << luxrays::Property("scene.textures." + texName + ".type")("hitpointgrey") <<
					luxrays::Property("scene.textures." + texName + ".channel")(((channel != 0) && (channel != 1) && (channel != 2)) ?
						-1 : channel);
		} else {
			LOG(LUX_WARNING, LUX_UNIMPLEMENT) << "LuxCoreRenderer supports only ImageSpectrumTexture, ImageFloatTexture, ConstantRGBColorTexture, "
					"ConstantFloatTexture, ScaleTexture, MixTexture, Checkerboard2D, Checkerboard3D, "
					"FBmTexture, Marble, Dots, Brick, Windy, Wrinkled, UVTexture, BandTexture, HitPointRGBColorTexture, "
					"HitPointAlphaTexture, HitPointGreyTexture and BlenderWoodTexture3D"
					"(i.e. not " << ToClassName(tex) << ").";

			texProps << luxrays::Property("scene.textures." + texName + ".type")("constfloat1") <<
					luxrays::Property("scene.textures." + texName + ".value")(.7f);
		}

		LOG(LUX_DEBUG, LUX_NOERROR) << "Defining texture " << texName << ": [\n" << texProps << "]";
		lcScene->Parse(texProps);
	}

	return texName;
}

static luxrays::Properties GetLuxCoreCommonMatProps(const string &matName,
		const string &emissionTexName, const float emissionGain,const float emissionPower,
		const float emissionEfficency, const string &emissionMapName, const u_int lightID,
		const string &bumpTex, const string &normalTex) {
	const string prefix = "scene.materials." + matName;

	luxrays::Properties props;
	if (emissionTexName != "0.0 0.0 0.0") {
		props <<
				luxrays::Property(prefix + ".emission")(emissionTexName) <<
				luxrays::Property(prefix + ".emission.gain")(emissionGain, emissionGain, emissionGain) <<
				luxrays::Property(prefix + ".emission.power")(emissionPower) <<
				luxrays::Property(prefix + ".emission.efficency")(emissionEfficency) <<
				luxrays::Property(prefix + ".emission.id")(lightID) <<
				luxrays::Property(prefix + ".emission.mapfile")(emissionMapName);
	}
	if (bumpTex != "")
		props << luxrays::Property(prefix + ".bumptex")(bumpTex);
	if (normalTex != "")
		props << luxrays::Property(prefix + ".normaltex")(normalTex);

	return props;
}

static string GetLuxCoreMaterialName(luxcore::Scene *lcScene, Material *mat,
		const Primitive *prim, const string &emissionTexName,
		const float emissionGain,const float emissionPower, const float emissionEfficency,
		const string &emissionMapName, const u_int lightID, ColorSystem &colorSpace) {
	if (!mat)
		return "mat_default";

	//--------------------------------------------------------------------------
	// Retrieve bump/normal mapping information too
	//--------------------------------------------------------------------------
	string bumpTex = "";
	string normalTex = "";
	const Texture<float> *bumpNormalTex = mat->bumpMap.get();
	if (bumpNormalTex) {
		LOG(LUX_DEBUG, LUX_NOERROR) << "Bump/Normal map type: " << ToClassName(bumpNormalTex);

		if (dynamic_cast<const NormalMapTexture *>(bumpNormalTex)) {
			// NormalMapTexture is for normal mapping
			normalTex = GetLuxCoreTexName(lcScene, bumpNormalTex);
		} else {
			// Anything else (i.e. ScaleTexture<float, float>) is for bump mapping
			bumpTex = GetLuxCoreTexName(lcScene, bumpNormalTex);
		}
	}

	LOG(LUX_DEBUG, LUX_NOERROR) << "Material type: " << ToClassName(mat);
	string matName = "mat_default";

	//------------------------------------------------------------------
	// Check if it is material Matte
	//------------------------------------------------------------------
	if (dynamic_cast<Matte *>(mat)) {
		// Define the material
		Matte *matte = dynamic_cast<Matte *>(mat);
		matName = matte->GetName();

		// Check if the material has already been defined
		if (!lcScene->IsMaterialDefined(matName)) {
			const string texName = GetLuxCoreTexName(lcScene, matte->GetTexture());

			const luxrays::Properties matProps(
				luxrays::Property("scene.materials." + matName +".type")("matte") <<
				 GetLuxCoreCommonMatProps(matName, emissionTexName, emissionGain, emissionPower, 
					emissionEfficency, emissionMapName, lightID, bumpTex, normalTex) <<
				luxrays::Property("scene.materials." + matName +".kd")(texName));
			LOG(LUX_DEBUG, LUX_NOERROR) << "Defining material " << matName << ": [\n" << matProps << "]";
			lcScene->Parse(matProps);
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
		if (!lcScene->IsMaterialDefined(matName)) {
			const string texName = GetLuxCoreTexName(lcScene, mirror->GetTexture());

			const luxrays::Properties matProps(
				luxrays::Property("scene.materials." + matName +".type")("mirror") <<
				GetLuxCoreCommonMatProps(matName, emissionTexName, emissionGain, emissionPower, 
					emissionEfficency, emissionMapName, lightID, bumpTex, normalTex) <<
				luxrays::Property("scene.materials." + matName +".kr")(texName));
			LOG(LUX_DEBUG, LUX_NOERROR) << "Defining material " << matName << ": [\n" << matProps << "]";
			lcScene->Parse(matProps);
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
		const string ktTexName = GetLuxCoreTexName(lcScene, glass->GetKtTexture());
		const string krTexName = GetLuxCoreTexName(lcScene, glass->GetKrTexture());

		// Check if it is architectural glass
		const bool architectural = (*glass)["architectural"].BoolValue();
		LOG(LUX_DEBUG, LUX_NOERROR) << "Architectural glass: " << architectural;
		// Check if the material has already been defined
		if (!lcScene->IsMaterialDefined(matName)) {
			const string indexTexName = GetLuxCoreTexName(lcScene, glass->GetIndexTexture());

			const luxrays::Properties matProps(
				luxrays::Property("scene.materials." + matName +".type")(architectural ? "archglass" : "glass") <<
				GetLuxCoreCommonMatProps(matName, emissionTexName, emissionGain, emissionPower, 
					emissionEfficency, emissionMapName, lightID, bumpTex, normalTex) <<
				luxrays::Property("scene.materials." + matName +".kr")(krTexName) <<
				luxrays::Property("scene.materials." + matName +".kt")(ktTexName) <<
				luxrays::Property("scene.materials." + matName +".ioroutside")(1.f) <<
				luxrays::Property("scene.materials." + matName +".iorinside")(indexTexName));
			LOG(LUX_DEBUG, LUX_NOERROR) << "Defining material " << matName << ": [\n" << matProps << "]";
			lcScene->Parse(matProps);
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
				LOG(LUX_WARNING, LUX_UNIMPLEMENT) << "LuxCoreRenderer supports only Glass2 material with ConstantRGBColorTexture (i.e. not " <<
					ToClassName(absorbRGBTex) << "). Ignoring unsupported texture.";
			}
		}

		// Check if it is architectural glass
		const bool architectural = (*glass2)["architectural"].BoolValue();
		LOG(LUX_DEBUG, LUX_NOERROR) << "Architectural glass: " << architectural;
		// Check if the material has already been defined
		if (!lcScene->IsMaterialDefined(matName)) {
			luxrays::Properties matProps;
			if (architectural) {
				matProps << luxrays::Property("scene.materials." + matName +".type")("archglass") <<
						GetLuxCoreCommonMatProps(matName, emissionTexName, emissionGain, emissionPower, 
							emissionEfficency, emissionMapName, lightID, bumpTex, normalTex) <<
						luxrays::Property("scene.materials." + matName +".kr")(krRGB);
			} else {
				matProps << luxrays::Property("scene.materials." + matName +".type")("glass") <<
						GetLuxCoreCommonMatProps(matName, emissionTexName, emissionGain, emissionPower, 
							emissionEfficency, emissionMapName, lightID, bumpTex, normalTex) <<
						luxrays::Property("scene.materials." + matName +".kr")(krRGB.r) <<
						luxrays::Property("scene.materials." + matName +".kt")(ktRGB) <<
						luxrays::Property("scene.materials." + matName +".ioroutside")(1.f) <<
						luxrays::Property("scene.materials." + matName +".iorinside")(index);
			}
			LOG(LUX_DEBUG, LUX_NOERROR) << "Defining material " << matName << ": [\n" << matProps << "]";
			lcScene->Parse(matProps);

			LOG(LUX_WARNING, LUX_UNIMPLEMENT) << "LuxCoreRenderer doesn't support Glass2 material, trying an emulation with Glass1.";
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
		if (!lcScene->IsMaterialDefined(matName)) {
			SPD *N = metal->GetNSPD();
			SPD *K = metal->GetKSPD();

			const RGBColor Nrgb = colorSpace.ToRGBConstrained(N->ToNormalizedXYZ());
			LOG(LUX_DEBUG, LUX_NOERROR) << "Metal N color: " << Nrgb;

			const RGBColor Krgb = colorSpace.ToRGBConstrained(K->ToNormalizedXYZ());
			LOG(LUX_DEBUG, LUX_NOERROR) << "Metal K color: " << Krgb;

			const string nuTexName = GetLuxCoreTexName(lcScene, metal->GetNuTexture());
			const string nvTexName = GetLuxCoreTexName(lcScene, metal->GetNvTexture());

			// Emulating Metal with Metal2 material
			const luxrays::Properties matProps(
				luxrays::Property("scene.materials." + matName +".type")("metal2") <<
				GetLuxCoreCommonMatProps(matName, emissionTexName, emissionGain, emissionPower, 
					emissionEfficency, emissionMapName, lightID, bumpTex, normalTex) <<
				luxrays::Property("scene.materials." + matName +".n")(Nrgb.c[0], Nrgb.c[1], Nrgb.c[2]) <<
				luxrays::Property("scene.materials." + matName +".k")(Krgb.c[0], Krgb.c[1], Krgb.c[2]) <<
				luxrays::Property("scene.materials." + matName +".uroughness")(nuTexName) <<
				luxrays::Property("scene.materials." + matName +".vroughness")(nvTexName));
			LOG(LUX_DEBUG, LUX_NOERROR) << "Defining material " << matName << ": [\n" << matProps << "]";
			lcScene->Parse(matProps);
		}
	} else
	//------------------------------------------------------------------
	// Check if it is material MatteTranslucent
	//------------------------------------------------------------------
	if (dynamic_cast<MatteTranslucent *>(mat)) {
		// Define the material
		MatteTranslucent *matteTranslucent = dynamic_cast<MatteTranslucent *>(mat);
		matName = matteTranslucent->GetName();

		// Check if the material has already been defined
		if (!lcScene->IsMaterialDefined(matName)) {
			const string krTexName = GetLuxCoreTexName(lcScene, matteTranslucent->GetKrTexture());
			const string ktTexName = GetLuxCoreTexName(lcScene, matteTranslucent->GetKtTexture());

			const luxrays::Properties matProps(
				luxrays::Property("scene.materials." + matName +".type")("mattetranslucent") <<
				GetLuxCoreCommonMatProps(matName, emissionTexName, emissionGain, emissionPower, 
					emissionEfficency, emissionMapName, lightID, bumpTex, normalTex) <<
				luxrays::Property("scene.materials." + matName +".kr")(krTexName) <<
				luxrays::Property("scene.materials." + matName +".kt")(ktTexName));
			LOG(LUX_DEBUG, LUX_NOERROR) << "Defining material " << matName << ": [\n" << matProps << "]";
			lcScene->Parse(matProps);
		}
	} else
	//------------------------------------------------------------------
	// Check if it is material Null
	//------------------------------------------------------------------
	if (dynamic_cast<Null *>(mat)) {
		// Define the material
		Null *null = dynamic_cast<Null *>(mat);
		matName = null->GetName();

		// Check if the material has already been defined
		if (!lcScene->IsMaterialDefined(matName)) {
			const luxrays::Properties matProps(
				luxrays::Property("scene.materials." + matName +".type")("null") <<
				GetLuxCoreCommonMatProps(matName, emissionTexName, emissionGain, emissionPower, 
					emissionEfficency, emissionMapName, lightID, bumpTex, normalTex));
			LOG(LUX_DEBUG, LUX_NOERROR) << "Defining material " << matName << ": [\n" << matProps << "]";
			lcScene->Parse(matProps);
		}
	} else
	//------------------------------------------------------------------
	// Check if it is material Mix
	//------------------------------------------------------------------
	if (dynamic_cast<MixMaterial *>(mat)) {
		// Define the material
		MixMaterial *mix = dynamic_cast<MixMaterial *>(mat);
		matName = mix->GetName();

		// Check if the material has already been defined
		if (!lcScene->IsMaterialDefined(matName)) {
			Texture<float> *amount = mix->GetAmmountTexture();
			Material *mat1 = mix->GetMaterial1();
			Material *mat2 = mix->GetMaterial2();

			const luxrays::Properties matProps(
				luxrays::Property("scene.materials." + matName +".type")("mix") <<
				GetLuxCoreCommonMatProps(matName, emissionTexName, emissionGain, emissionPower, 
					emissionEfficency, emissionMapName, lightID, bumpTex, normalTex) <<
				luxrays::Property("scene.materials." + matName +".amount")(GetLuxCoreTexName(lcScene, amount)) <<
				luxrays::Property("scene.materials." + matName +".material1")(GetLuxCoreMaterialName(
					lcScene, mat1, prim, "0.0 0.0 0.0", 0.f, 0.f, 0.f, "", 0, colorSpace)) <<
				luxrays::Property("scene.materials." + matName +".material2")(GetLuxCoreMaterialName(
					lcScene, mat2, prim, "0.0 0.0 0.0", 0.f, 0.f, 0.f, "", 0, colorSpace)));
			LOG(LUX_DEBUG, LUX_NOERROR) << "Defining material " << matName << ": [\n" << matProps << "]";
			lcScene->Parse(matProps);
		}
	} else
	//------------------------------------------------------------------
	// Check if it is material Glossy2
	//------------------------------------------------------------------
	if (dynamic_cast<Glossy2 *>(mat)) {
		// Define the material
		Glossy2 *glossy2 = dynamic_cast<Glossy2 *>(mat);
		matName = glossy2->GetName();

		// Check if the material has already been defined
		if (!lcScene->IsMaterialDefined(matName)) {
			const string kdTexName = GetLuxCoreTexName(lcScene, glossy2->GetKdTexture());
			const string ksTexName = GetLuxCoreTexName(lcScene, glossy2->GetKsTexture());
			const string kaTexName = GetLuxCoreTexName(lcScene, glossy2->GetKaTexture());
			const string nuTexName = GetLuxCoreTexName(lcScene, glossy2->GetNuTexture());
			const string nvTexName = GetLuxCoreTexName(lcScene, glossy2->GetNvTexture());
			const string depthTexName = GetLuxCoreTexName(lcScene, glossy2->GetDepthTexture());
			const string indexTexName = GetLuxCoreTexName(lcScene, glossy2->GetIndexTexture());
			const bool isMultibounce = glossy2->IsMultiBounce();

			const luxrays::Properties matProps(
				luxrays::Property("scene.materials." + matName +".type")("glossy2") <<
				GetLuxCoreCommonMatProps(matName, emissionTexName, emissionGain, emissionPower, 
					emissionEfficency, emissionMapName, lightID, bumpTex, normalTex) <<
				luxrays::Property("scene.materials." + matName +".kd")(kdTexName) <<
				luxrays::Property("scene.materials." + matName +".ks")(ksTexName) <<
				luxrays::Property("scene.materials." + matName +".ka")(kaTexName) <<
				luxrays::Property("scene.materials." + matName +".uroughness")(nuTexName) <<
				luxrays::Property("scene.materials." + matName +".vroughness")(nvTexName) <<
				luxrays::Property("scene.materials." + matName +".d")(depthTexName) <<
				luxrays::Property("scene.materials." + matName +".index")(indexTexName) <<
				luxrays::Property("scene.materials." + matName +".multibounce")(isMultibounce ? "1" : "0"));
			LOG(LUX_DEBUG, LUX_NOERROR) << "Defining material " << matName << ": [\n" << matProps << "]";
			lcScene->Parse(matProps);
		}
	} else
	//------------------------------------------------------------------
	// Check if it is material Metal2
	//------------------------------------------------------------------
	if (dynamic_cast<Metal2 *>(mat)) {
		// Define the material
		Metal2 *metal2 = dynamic_cast<Metal2 *>(mat);
		matName = metal2->GetName();

		// Check if the material has already been defined
		if (!lcScene->IsMaterialDefined(matName)) {
			Texture<FresnelGeneral> *fresnelTex = metal2->GetFresnelTexture();

			const string nuTexName = GetLuxCoreTexName(lcScene, metal2->GetNuTexture());
			const string nvTexName = GetLuxCoreTexName(lcScene, metal2->GetNvTexture());

			if (dynamic_cast<TabulatedFresnel *>(fresnelTex)) {
				TabulatedFresnel *tabFresnelTex = dynamic_cast<TabulatedFresnel *>(fresnelTex);

				IrregularSPD *N = tabFresnelTex->GetNSPD();
				IrregularSPD *K = tabFresnelTex->GetKSPD();

				const RGBColor Nrgb = colorSpace.ToRGBConstrained(N->ToNormalizedXYZ());
				LOG(LUX_DEBUG, LUX_NOERROR) << "Metal2 N color: " << Nrgb;

				const RGBColor Krgb = colorSpace.ToRGBConstrained(K->ToNormalizedXYZ());
				LOG(LUX_DEBUG, LUX_NOERROR) << "Metal2 K color: " << Krgb;

				const luxrays::Properties matProps(
					luxrays::Property("scene.materials." + matName +".type")("metal2") <<
					GetLuxCoreCommonMatProps(matName, emissionTexName, emissionGain, emissionPower, 
					emissionEfficency, emissionMapName, lightID, bumpTex, normalTex) <<
					luxrays::Property("scene.materials." + matName +".n")(Nrgb.c[0], Nrgb.c[1], Nrgb.c[2]) <<
					luxrays::Property("scene.materials." + matName +".k")(Krgb.c[0], Krgb.c[1], Krgb.c[2]) <<
					luxrays::Property("scene.materials." + matName +".uroughness")(nuTexName) <<
					luxrays::Property("scene.materials." + matName +".vroughness")(nvTexName));
				LOG(LUX_DEBUG, LUX_NOERROR) << "Defining material " << matName << ": [\n" << matProps << "]";
				lcScene->Parse(matProps);
			} else if (dynamic_cast<FresnelColorTexture *>(fresnelTex)) {
				FresnelColorTexture *fresnelCol = dynamic_cast<FresnelColorTexture *>(fresnelTex);

				const string colTexName = GetLuxCoreTexName(lcScene, fresnelCol->GetColorTexture());

				// Define FresnellApproxN and FresnellApproxK textures
				const luxrays::Properties texProps(
					luxrays::Property("scene.textures.fresnelapproxn-" + matName + ".type")("fresnelapproxn") <<
					luxrays::Property("scene.textures.fresnelapproxn-" + matName + ".texture")(colTexName) <<
					luxrays::Property("scene.textures.fresnelapproxk-" + matName + ".type")("fresnelapproxk") <<
					luxrays::Property("scene.textures.fresnelapproxk-" + matName + ".texture")(colTexName));
				LOG(LUX_DEBUG, LUX_NOERROR) << "Defining textures for material " << matName << ": [\n" << texProps << "]";
				lcScene->Parse(texProps);

				const luxrays::Properties matProps(
					luxrays::Property("scene.materials." + matName +".type")("metal2") <<
					GetLuxCoreCommonMatProps(matName, emissionTexName, emissionGain, emissionPower, 
					emissionEfficency, emissionMapName, lightID, bumpTex, normalTex) <<
					luxrays::Property("scene.materials." + matName +".n")("fresnelapproxn-" + matName) <<
					luxrays::Property("scene.materials." + matName +".k")("fresnelapproxk-" + matName) <<
					luxrays::Property("scene.materials." + matName +".uroughness")(nuTexName) <<
					luxrays::Property("scene.materials." + matName +".vroughness")(nvTexName));
				LOG(LUX_DEBUG, LUX_NOERROR) << "Defining material " << matName << ": [\n" << matProps << "]";
				lcScene->Parse(matProps);
			} else {
				LOG(LUX_WARNING, LUX_UNIMPLEMENT) << "LuxCoreRenderer supports only Metal2 material with tabular data or fresnel color texture (i.e. not " <<
						ToClassName(fresnelTex) << "). Replacing an unsupported material with matte.";
				return "mat_default";
			}
		}
	} else
	//------------------------------------------------------------------
	// Check if it is material RoughGlass
	//------------------------------------------------------------------
	if (dynamic_cast<RoughGlass *>(mat)) {
		// Define the material
		RoughGlass *roughglass = dynamic_cast<RoughGlass *>(mat);
		matName = roughglass->GetName();

		// Check if the material has already been defined
		if (!lcScene->IsMaterialDefined(matName)) {
			// Textures
			const string ktTexName = GetLuxCoreTexName(lcScene, roughglass->GetKtTexture());
			const string krTexName = GetLuxCoreTexName(lcScene, roughglass->GetKrTexture());
			const string indexTexName = GetLuxCoreTexName(lcScene, roughglass->GetIndexTexture());
			const string nuTexName = GetLuxCoreTexName(lcScene, roughglass->GetNuTexture());
			const string nvTexName = GetLuxCoreTexName(lcScene, roughglass->GetNvTexture());

			const luxrays::Properties matProps(
					luxrays::Property("scene.materials." + matName +".type")("roughglass") <<
					GetLuxCoreCommonMatProps(matName, emissionTexName, emissionGain, emissionPower, 
					emissionEfficency, emissionMapName, lightID, bumpTex, normalTex) <<
					luxrays::Property("scene.materials." + matName +".kr")(krTexName) <<
					luxrays::Property("scene.materials." + matName +".kt")(ktTexName) <<
					luxrays::Property("scene.materials." + matName +".ioroutside")(1.f) <<
					luxrays::Property("scene.materials." + matName +".iorinside")(indexTexName) <<
					luxrays::Property("scene.materials." + matName +".uroughness")(nuTexName) <<
					luxrays::Property("scene.materials." + matName +".vroughness")(nvTexName));
			LOG(LUX_DEBUG, LUX_NOERROR) << "Defining material " << matName << ": [\n" << matProps << "]";
			lcScene->Parse(matProps);
		}
	} else
	//------------------------------------------------------------------
	// Material is not supported, use the default one
	//------------------------------------------------------------------
	{
		LOG(LUX_WARNING, LUX_UNIMPLEMENT) << "LuxCoreRenderer supports only Matte, Mirror, Glass, Glass2, Metal, MatteTranslucent, Null, Mix, Glossy2, Metal2 and RoughGlass material (i.e. not " <<
			ToClassName(mat) << "). Replacing an unsupported material with matte.";
		return "mat_default";
	}

	return matName;
}

static string GetLuxCoreMaterialName(luxcore::Scene *lcScene, const Primitive *prim,
		ColorSystem &colorSpace) {
	LOG(LUX_DEBUG, LUX_NOERROR) << "Primitive type: " << ToClassName(prim);
	
	Material *mat = NULL;
	float emissionGain = 1.f;
	float emissionPower = 0.f;
	float emissionEfficency = 0.f;
	string emissionTexName = "0.0 0.0 0.0";
	string emissionMapName = "";
	u_int lightID = 0;
	

	//--------------------------------------------------------------------------
	// Check if it is a Shape
	//--------------------------------------------------------------------------
	if (dynamic_cast<const Shape *>(prim)) {
		const Shape *shape = dynamic_cast<const Shape *>(prim);
		mat = shape->GetMaterial();
	} else
	//--------------------------------------------------------------------------
	// Check if it is an InstancePrimitive
	//--------------------------------------------------------------------------
	if (dynamic_cast<const InstancePrimitive *>(prim)) {
		const InstancePrimitive *instance = dynamic_cast<const InstancePrimitive *>(prim);
		mat = instance->GetMaterial();
	} else
	//--------------------------------------------------------------------------
	// Check if it is an InstancePrimitive
	//--------------------------------------------------------------------------
	if (dynamic_cast<const MotionPrimitive *>(prim)) {
		const MotionPrimitive *motionPrim = dynamic_cast<const MotionPrimitive *>(prim);
		mat = motionPrim->GetMaterial();
	} else
	//--------------------------------------------------------------------------
	// Check if it is an AreaLight
	//--------------------------------------------------------------------------
	if (dynamic_cast<const AreaLightPrimitive *>(prim)) {
		const AreaLightPrimitive *alPrim = dynamic_cast<const AreaLightPrimitive *>(prim);
		AreaLight *al = alPrim->GetAreaLight().get();

		Texture<SWCSpectrum> *tex = al->GetTexture();

		emissionGain = (*al)["gain"].FloatValue();
		emissionPower = (*al)["power"].FloatValue();
		emissionEfficency = (*al)["efficacy"].FloatValue();
		lightID = (*al)["group"].IntValue();

		// Check if it use an emission map
		const SampleableSphericalFunction *ssf = al->GetFunc();
		if (ssf) {
			const SphericalFunction *sf = ssf->GetFunc();
			if (dynamic_cast<const MipMapSphericalFunction *>(sf)) {
				const MipMapSphericalFunction *mmsf = dynamic_cast<const MipMapSphericalFunction *>(sf);

				emissionMapName = GetLuxCoreImageMapName(lcScene, mmsf->GetMipMap(), 2.2f);
			} else {
				LOG(LUX_WARNING, LUX_UNIMPLEMENT) << "Unsupported type of SphericalFunction in an area light (i.e. " <<
					ToClassName(sf) << "). Ignoring the unsupported feature.";
			}
		}

		// Check the type of texture used
		LOG(LUX_DEBUG, LUX_NOERROR) << "AreaLight texture type: " << ToClassName(tex);

		if (dynamic_cast<ConstantRGBColorTexture *>(tex)) {
			ConstantRGBColorTexture *constRGBTex = dynamic_cast<ConstantRGBColorTexture *>(tex);

			const SPD *spd = constRGBTex->GetRGBSPD();
			RGBColor rgb = colorSpace.ToRGBConstrained(spd->ToXYZ());

			emissionTexName = ToString(rgb.c[0]) + " " + ToString(rgb.c[1]) + " " + ToString(rgb.c[2]);
		} else if (dynamic_cast<BlackBodyTexture *>(tex)) {
			BlackBodyTexture *bbTex = dynamic_cast<BlackBodyTexture *>(tex);

			const SPD *spd = bbTex->GetBlackBodySPD();
			RGBColor rgb = colorSpace.ToRGBConstrained(spd->ToXYZ());

			emissionTexName = ToString(rgb.c[0]) + " " + ToString(rgb.c[1]) + " " + ToString(rgb.c[2]);
		} else {
			emissionTexName = GetLuxCoreTexName(lcScene, tex);
		}

		LOG(LUX_DEBUG, LUX_NOERROR) << "AreaLight emission: " << emissionTexName;

		const Primitive *p = alPrim->GetPrimitive().get();
		if (dynamic_cast<const Shape *>(p)) {
			const Shape *shape = dynamic_cast<const Shape *>(p);
			mat = shape->GetMaterial();
		} else if (dynamic_cast<const InstancePrimitive *>(p)) {
			const InstancePrimitive *instance = dynamic_cast<const InstancePrimitive *>(p);
			mat = instance->GetMaterial();
		} else {
			LOG(LUX_WARNING, LUX_UNIMPLEMENT) << "LuxCoreRenderer doesn't support material conversion for area light primitive " << ToClassName(prim);
			return "mat_default";
		}
	} else
	//--------------------------------------------------------------------------
	// Primitive is not supported, use the default material
	//--------------------------------------------------------------------------
	{
		LOG(LUX_WARNING, LUX_UNIMPLEMENT) << "LuxCoreRenderer doesn't support material conversion for primitive " << ToClassName(prim);
		return "mat_default";
	}

	return GetLuxCoreMaterialName(lcScene, mat, prim, emissionTexName,
			emissionGain, emissionPower, emissionEfficency, emissionMapName,
			lightID, colorSpace);
}

void LuxCoreRenderer::ConvertLights(luxcore::Scene *lcScene) {
	for (size_t i = 0; i < scene->lights.size(); ++i) {
		//----------------------------------------------------------------------
		// Check if it is a sun light source
		//----------------------------------------------------------------------
		SunLight *sunLight = dynamic_cast<SunLight *>(scene->lights[i].get());
		if (sunLight) {
			// Add a SunLight to the scene
			const float dirX = (*sunLight)["dir.x"].FloatValue();
			const float dirY = (*sunLight)["dir.y"].FloatValue();
			const float dirZ = (*sunLight)["dir.z"].FloatValue();
			const float turbidity = (*sunLight)["turbidity"].FloatValue();
			const float relSize = (*sunLight)["relSize"].FloatValue();
			// Note: (1000000000.0f / (M_PI * 100.f * 100.f)) is in LuxCore code
			// for compatibility with past scene
			const float gain = (*sunLight)["gain"].FloatValue() * (1000000000.0f / (M_PI * 100.f * 100.f)) *
				INV_PI;
			const u_int lightId = (*sunLight)["group"].IntValue();

			const Transform &light2World = sunLight->GetTransform();

			const string prefix = "scene.lights.sunlight_" + ToString(i);
			const luxrays::Properties createSunLightProps(
				luxrays::Property(prefix + ".type")("sun") <<
				luxrays::Property(prefix + ".dir")(dirX, dirY, dirZ) <<
				luxrays::Property(prefix + ".turbidity")(turbidity) <<
				luxrays::Property(prefix + ".relsize")(relSize) <<
				luxrays::Property(prefix + ".gain")(gain, gain, gain) <<
				luxrays::Property(prefix + ".transformation")(light2World.m) <<
				luxrays::Property(prefix + ".id")(lightId));
			LOG(LUX_DEBUG, LUX_NOERROR) << "Creating sunlight: [\n" << createSunLightProps << "]";
			lcScene->Parse(createSunLightProps);
			continue;
		}

		//----------------------------------------------------------------------
		// Check if it is a sky light source
		//----------------------------------------------------------------------
		SkyLight *skyLight = dynamic_cast<SkyLight *>(scene->lights[i].get());
		if (skyLight) {
			// Add a SkyLight to the scene

			// Note: (1000000000.0f / (M_PI * 100.f * 100.f)) is in LuxCore code
			// for compatibility with past scene
			const float gainAdjustFactor = (1000000000.0f / (M_PI * 100.f * 100.f)) * INV_PI;

			const float dirX = (*skyLight)["dir.x"].FloatValue();
			const float dirY = (*skyLight)["dir.y"].FloatValue();
			const float dirZ = (*skyLight)["dir.z"].FloatValue();
			const float turbidity = (*skyLight)["turbidity"].FloatValue();
			// Note: (1000000000.0f / (M_PI * 100.f * 100.f)) is in LuxCore code
			// for compatibility with past scene
			const float gain = (*skyLight)["gain"].FloatValue() * gainAdjustFactor;
			const u_int lightId = (*skyLight)["group"].IntValue();

			const Transform &light2World = skyLight->GetTransform();

			const string prefix = "scene.lights.skylight_" + ToString(i);
			const luxrays::Properties createSkyLightProps(
				luxrays::Property(prefix + ".type")("sky") <<
				luxrays::Property(prefix + ".dir")(dirX, dirY, dirZ) <<
				luxrays::Property(prefix + ".turbidity")(turbidity) <<
				luxrays::Property(prefix + ".gain")(gain, gain, gain) <<
				luxrays::Property(prefix + ".transformation")(light2World.m) <<
				luxrays::Property(prefix + ".id")(lightId));
			LOG(LUX_DEBUG, LUX_NOERROR) << "Creating skylight: [\n" << createSkyLightProps << "]";
			lcScene->Parse(createSkyLightProps);

			continue;
		}

		//----------------------------------------------------------------------
		// Check if it is a sky2 light source
		//----------------------------------------------------------------------
		Sky2Light *sky2Light = dynamic_cast<Sky2Light *>(scene->lights[i].get());
		if (sky2Light) {
			// Add a SkyLight to the scene

			// Note: (1000000000.0f / (M_PI * 100.f * 100.f)) is in LuxCore code
			// for compatibility with past scene
			const float gainAdjustFactor = (1000000000.0f / (M_PI * 100.f * 100.f)) * INV_PI;

			LOG(LUX_WARNING, LUX_UNIMPLEMENT) << "LuxCoreRenderer doesn't support Sky2 light. It will use Sky instead.";

			const float dirX = (*sky2Light)["dir.x"].FloatValue();
			const float dirY = (*sky2Light)["dir.y"].FloatValue();
			const float dirZ = (*sky2Light)["dir.z"].FloatValue();
			const float turbidity = (*sky2Light)["turbidity"].FloatValue();
			// Note: (1000000000.0f / (M_PI * 100.f * 100.f)) is in LuxCore code
			// for compatibility with past scene
			const float gain = (*sky2Light)["gain"].FloatValue() * gainAdjustFactor;
			const u_int lightId = (*sky2Light)["group"].IntValue();

			const Transform &light2World = sky2Light->GetTransform();

			const string prefix = "scene.lights.skylight2_" + ToString(i);
			const luxrays::Properties createSkyLightProps(
				luxrays::Property(prefix + ".type")("sky") <<
				luxrays::Property(prefix + ".dir")(dirX, dirY, dirZ) <<
				luxrays::Property(prefix + ".turbidity")(turbidity) <<
				luxrays::Property(prefix + ".gain")(gain, gain, gain) <<
				luxrays::Property(prefix + ".transformation")(light2World.m) <<
				luxrays::Property(prefix + ".id")(lightId));
			LOG(LUX_DEBUG, LUX_NOERROR) << "Creating skylight: [\n" << createSkyLightProps << "]";
			lcScene->Parse(createSkyLightProps);

			continue;
		}

		//----------------------------------------------------------------------
		// Check if it is an infinite light source
		//----------------------------------------------------------------------
		InfiniteAreaLight *infiniteAreaLight = dynamic_cast<InfiniteAreaLight *>(scene->lights[i].get());
		if (infiniteAreaLight) {
			const float colorR = (*infiniteAreaLight)["color.r"].FloatValue();
			const float colorG = (*infiniteAreaLight)["color.g"].FloatValue();
			const float colorB = (*infiniteAreaLight)["color.b"].FloatValue();
			const float gain = (*infiniteAreaLight)["gain"].FloatValue();
			const u_int lightId = (*infiniteAreaLight)["group"].IntValue();

			MIPMap *mipMap = infiniteAreaLight->GetRadianceMap();
			if (mipMap) {
				const float gamma = (*infiniteAreaLight)["gamma"].FloatValue();
				const string imageMapName = GetLuxCoreImageMapName(lcScene, mipMap, gamma);

				const Transform &light2World = infiniteAreaLight->GetTransform();

				const string prefix = "scene.lights.infinitelight_" + ToString(i);
				const luxrays::Properties createInfiniteLightProps(
					luxrays::Property(prefix + ".type")("infinite") <<
					luxrays::Property(prefix + ".file")(imageMapName) <<
					luxrays::Property(prefix + ".gamma")(gamma) <<
					luxrays::Property(prefix + ".shift")(0.f, 0.f) <<
					luxrays::Property(prefix + ".gain")(gain * colorR, gain * colorG, gain * colorB) <<
					luxrays::Property(prefix + ".transformation")(light2World.m) <<
					luxrays::Property(prefix + ".id")(lightId));
				LOG(LUX_DEBUG, LUX_NOERROR) << "Creating infinitelight: [\n" << createInfiniteLightProps << "]";
				lcScene->Parse(createInfiniteLightProps);
			} else {
				const string prefix = "scene.lights.constantinfinitelight_" + ToString(i);
				const luxrays::Properties createInfiniteLightProps(
					luxrays::Property(prefix + ".type")("constantinfinite") <<
					luxrays::Property(prefix + ".gain")(gain, gain, gain) <<
					luxrays::Property(prefix + ".color")(colorR, colorG, colorB) <<
					luxrays::Property(prefix + ".id")(lightId));
				LOG(LUX_DEBUG, LUX_NOERROR) << "Creating constantinfinitelight: [\n" << createInfiniteLightProps << "]";
				lcScene->Parse(createInfiniteLightProps);
			}

			continue;
		}

		//----------------------------------------------------------------------
		// Check if it is an infinite light source
		//----------------------------------------------------------------------
		InfiniteAreaLightIS *infiniteAreaLightIS = dynamic_cast<InfiniteAreaLightIS *>(scene->lights[i].get());
		if (infiniteAreaLightIS) {
			const float colorR = (*infiniteAreaLightIS)["color.r"].FloatValue();
			const float colorG = (*infiniteAreaLightIS)["color.g"].FloatValue();
			const float colorB = (*infiniteAreaLightIS)["color.b"].FloatValue();
			const float gain = (*infiniteAreaLightIS)["gain"].FloatValue();
			const u_int lightId = (*infiniteAreaLightIS)["group"].IntValue();

			MIPMap *mipMap = infiniteAreaLightIS->GetRadianceMap();
			if (mipMap) {
				const float gamma = (*infiniteAreaLightIS)["gamma"].FloatValue();
				const string imageMapName = GetLuxCoreImageMapName(lcScene, mipMap, gamma);

				const Transform &light2World = infiniteAreaLightIS->GetTransform();

				const string prefix = "scene.lights.infinitelightIS_" + ToString(i);
				const luxrays::Properties createInfiniteLightProps(
					luxrays::Property(prefix + ".type")("infinite") <<
					luxrays::Property(prefix + ".file")(imageMapName) <<
					luxrays::Property(prefix + ".gamma")(gamma) <<
					luxrays::Property(prefix + ".shift")(0.f, 0.f) <<
					luxrays::Property(prefix + ".gain")(gain * colorR, gain * colorG, gain * colorB) <<
					luxrays::Property(prefix + ".transformation")(light2World.m) <<
					luxrays::Property(prefix + ".id")(lightId));
				LOG(LUX_DEBUG, LUX_NOERROR) << "Creating infinitelight: [\n" << createInfiniteLightProps << "]";
				lcScene->Parse(createInfiniteLightProps);
			} else {
				const string prefix = "scene.lights.constantinfinitelightIS_" + ToString(i);
				const luxrays::Properties createInfiniteLightProps(
					luxrays::Property(prefix + ".type")("constantinfinite") <<
					luxrays::Property(prefix + ".gain")(gain, gain, gain) <<
					luxrays::Property(prefix + ".color")(colorR, colorG, colorB) <<
					luxrays::Property(prefix + ".id")(lightId));
				LOG(LUX_DEBUG, LUX_NOERROR) << "Creating constantinfinitelight: [\n" << createInfiniteLightProps << "]";
				lcScene->Parse(createInfiniteLightProps);
			}

			continue;
		}
		
		//----------------------------------------------------------------------
		// Check if it is an point light source
		//----------------------------------------------------------------------
		PointLight *pointLight = dynamic_cast<PointLight *>(scene->lights[i].get());
		if (pointLight) {
			float colorR, colorG, colorB;
			if (dynamic_cast<const ConstantRGBColorTexture *>(pointLight->GetLbaseTexture())) {
				const ConstantRGBColorTexture *constRGBTex = dynamic_cast<const ConstantRGBColorTexture *>(pointLight->GetLbaseTexture());

				colorR = (*constRGBTex)["color.r"].FloatValue();
				colorG = (*constRGBTex)["color.g"].FloatValue();
				colorB = (*constRGBTex)["color.b"].FloatValue();
			} else {
				LOG(LUX_WARNING, LUX_UNIMPLEMENT) << "LuxCoreRenderer supports only point light with constant color. (i.e. not " <<
					ToClassName(pointLight->GetLbaseTexture()) << "). Ignoring the unsupported feature.";
				colorR = 1.f;
				colorG = 1.f;
				colorB = 1.f;
			}

			const float gain = (*pointLight)["gain"].FloatValue();
			const u_int lightId = (*pointLight)["group"].IntValue();

			const Transform &light2World = pointLight->GetTransform();

			const string prefix = "scene.lights.pointlight_" + ToString(i);
			luxrays::Properties createPointLightProps;
			// Check if it is a Point or MapPoint light
			const SampleableSphericalFunction *ssf = pointLight->GetFunc();
			if (ssf) {
				createPointLightProps << luxrays::Property(prefix + ".type")("mappoint");

				const SphericalFunction *sf = ssf->GetFunc();
				if (dynamic_cast<const MipMapSphericalFunction *>(sf)) {
					const MipMapSphericalFunction *mmsf = dynamic_cast<const MipMapSphericalFunction *>(sf);

					const string imageMapName = GetLuxCoreImageMapName(lcScene, mmsf->GetMipMap(), 2.2f);
					createPointLightProps << luxrays::Property(prefix + ".type")("mappoint") <<
							luxrays::Property(prefix + ".mapfile")(imageMapName);
				} else {
					LOG(LUX_WARNING, LUX_UNIMPLEMENT) << "Unsupported type of SphericalFunction in a point light (i.e. " <<
						ToClassName(sf) << "). Ignoring the unsupported feature.";
					createPointLightProps << luxrays::Property(prefix + ".type")("point");
				}
			} else
				createPointLightProps << luxrays::Property(prefix + ".type")("point");
			
			createPointLightProps <<
				luxrays::Property(prefix + ".gain")(gain, gain, gain) <<
				luxrays::Property(prefix + ".color")(colorR, colorG, colorB) <<
				luxrays::Property(prefix + ".transformation")(light2World.m) <<
				luxrays::Property(prefix + ".id")(lightId);
			LOG(LUX_DEBUG, LUX_NOERROR) << "Creating pointlight: [\n" << createPointLightProps << "]";
			lcScene->Parse(createPointLightProps);

			continue;
		}

		//----------------------------------------------------------------------
		// Check if it is a spot light source
		//----------------------------------------------------------------------
		SpotLight *spotLight = dynamic_cast<SpotLight *>(scene->lights[i].get());
		if (spotLight) {
			float colorR, colorG, colorB;
			if (dynamic_cast<const ConstantRGBColorTexture *>(spotLight->GetLbaseTexture())) {
				const ConstantRGBColorTexture *constRGBTex = dynamic_cast<const ConstantRGBColorTexture *>(spotLight->GetLbaseTexture());

				colorR = (*constRGBTex)["color.r"].FloatValue();
				colorG = (*constRGBTex)["color.g"].FloatValue();
				colorB = (*constRGBTex)["color.b"].FloatValue();
			} else {
				LOG(LUX_WARNING, LUX_UNIMPLEMENT) << "LuxCoreRenderer supports only spot light with constant color. (i.e. not " <<
					ToClassName(pointLight->GetLbaseTexture()) << "). Ignoring the unsupported feature.";
				colorR = 1.f;
				colorG = 1.f;
				colorB = 1.f;
			}

			const float gain = (*spotLight)["gain"].FloatValue();
			const u_int lightId = (*spotLight)["group"].IntValue();
			const float coneAngle = (*spotLight)["coneangle"].FloatValue();
			const float coneDeltaAngle = (*spotLight)["conedeltaangle"].FloatValue();

			const Transform &light2World = spotLight->GetTransform();

			const string prefix = "scene.lights.spotlight_" + ToString(i);

			const luxrays::Properties createSpotLightProps(
				luxrays::Property(prefix + ".type")("spot") <<
				luxrays::Property(prefix + ".conedeltaangle")(coneDeltaAngle) <<
				luxrays::Property(prefix + ".coneangle")(coneAngle) <<
				luxrays::Property(prefix + ".conedeltaangle")(coneDeltaAngle) <<
				luxrays::Property(prefix + ".gain")(gain, gain, gain) <<
				luxrays::Property(prefix + ".color")(colorR, colorG, colorB) <<
				luxrays::Property(prefix + ".transformation")(light2World.m) <<
				luxrays::Property(prefix + ".id")(lightId));
			LOG(LUX_DEBUG, LUX_NOERROR) << "Creating spotlight: [\n" << createSpotLightProps << "]";
			lcScene->Parse(createSpotLightProps);

			continue;
		}

		//----------------------------------------------------------------------
		// Check if it is a projection light source
		//----------------------------------------------------------------------
		ProjectionLight *projectionLight = dynamic_cast<ProjectionLight *>(scene->lights[i].get());
		if (projectionLight) {
			float colorR, colorG, colorB;
			if (dynamic_cast<const ConstantRGBColorTexture *>(projectionLight->GetLbaseTexture())) {
				const ConstantRGBColorTexture *constRGBTex = dynamic_cast<const ConstantRGBColorTexture *>(projectionLight->GetLbaseTexture());

				colorR = (*constRGBTex)["color.r"].FloatValue();
				colorG = (*constRGBTex)["color.g"].FloatValue();
				colorB = (*constRGBTex)["color.b"].FloatValue();
			} else {
				LOG(LUX_WARNING, LUX_UNIMPLEMENT) << "LuxCoreRenderer supports only projection light with constant color. (i.e. not " <<
					ToClassName(pointLight->GetLbaseTexture()) << "). Ignoring the unsupported feature.";
				colorR = 1.f;
				colorG = 1.f;
				colorB = 1.f;
			}

			const float gain = (*projectionLight)["gain"].FloatValue();
			const u_int lightId = (*projectionLight)["group"].IntValue();
			const float fov = (*projectionLight)["fov"].FloatValue();
			const string imageMapName = GetLuxCoreImageMapName(lcScene, projectionLight->GetMap(), 2.2f);

			const Transform &light2World = projectionLight->GetTransform();

			const string prefix = "scene.lights.projectionlight_" + ToString(i);

			const luxrays::Properties createProjectionLightProps(
				luxrays::Property(prefix + ".type")("projection") <<
				luxrays::Property(prefix + ".mapfile")(imageMapName) <<
				luxrays::Property(prefix + ".fov")(fov) <<
				luxrays::Property(prefix + ".gain")(gain, gain, gain) <<
				luxrays::Property(prefix + ".color")(colorR, colorG, colorB) <<
				luxrays::Property(prefix + ".transformation")(light2World.m) <<
				luxrays::Property(prefix + ".id")(lightId));
			LOG(LUX_DEBUG, LUX_NOERROR) << "Creating projectionlight: [\n" << createProjectionLightProps << "]";
			lcScene->Parse(createProjectionLightProps);

			continue;
		}

		//----------------------------------------------------------------------
		// Check if it is a distant light source
		//----------------------------------------------------------------------
		DistantLight *distantLight = dynamic_cast<DistantLight *>(scene->lights[i].get());
		if (distantLight) {
			float colorR, colorG, colorB;
			if (dynamic_cast<const ConstantRGBColorTexture *>(distantLight->GetLbaseTexture())) {
				const ConstantRGBColorTexture *constRGBTex = dynamic_cast<const ConstantRGBColorTexture *>(distantLight->GetLbaseTexture());

				colorR = (*constRGBTex)["color.r"].FloatValue();
				colorG = (*constRGBTex)["color.g"].FloatValue();
				colorB = (*constRGBTex)["color.b"].FloatValue();
			} else {
				LOG(LUX_WARNING, LUX_UNIMPLEMENT) << "LuxCoreRenderer supports only distant light with constant color. (i.e. not " <<
					ToClassName(pointLight->GetLbaseTexture()) << "). Ignoring the unsupported feature.";
				colorR = 1.f;
				colorG = 1.f;
				colorB = 1.f;
			}

			const float theta = distantLight->GetTheta();
			const float gain = (*distantLight)["gain"].FloatValue();
			const u_int lightId = (*distantLight)["group"].IntValue();

			const Transform &light2World = distantLight->GetTransform();

			const string prefix = "scene.lights.distantlight_" + ToString(i);

			luxrays::Properties createDistantLightProps;
			if (theta == 0.f) {
				createDistantLightProps <<
						luxrays::Property(prefix + ".type")("sharpdistant");
			} else {
				createDistantLightProps <<
						luxrays::Property(prefix + ".type")("distant") <<
						luxrays::Property(prefix + ".type")(theta);
			}

			createDistantLightProps <<
				luxrays::Property(prefix + ".direction")(distantLight->GetDirection()) <<
				luxrays::Property(prefix + ".gain")(gain, gain, gain) <<
				luxrays::Property(prefix + ".color")(colorR, colorG, colorB) <<
				luxrays::Property(prefix + ".transformation")(light2World.m) <<
				luxrays::Property(prefix + ".id")(lightId);
			LOG(LUX_DEBUG, LUX_NOERROR) << "Creating distantlight: [\n" << createDistantLightProps << "]";
			lcScene->Parse(createDistantLightProps);

			continue;
		}

		//----------------------------------------------------------------------
		// Check if it is an area light source
		//----------------------------------------------------------------------	
		AreaLight *areaLight = dynamic_cast<AreaLight *>(scene->lights[i].get());
		if (areaLight) {
			// I can just ignore area lights
			continue;
		}

		LOG(LUX_WARNING, LUX_UNIMPLEMENT) << "LuxCoreRenderer supports only infinite, sky, sky2, sun and point light source (i.e. not " <<
				ToClassName(scene->lights[i].get()) << "). Ignoring the unsupported light source.";
	}
}

vector<luxrays::ExtTriangleMesh *> LuxCoreRenderer::DefinePrimitive(luxcore::Scene *lcScene, const Primitive *prim) {
	//LOG(LUX_DEBUG, LUX_NOERROR) << "Define primitive type: " << ToClassName(prim);

	vector<luxrays::ExtTriangleMesh *> meshList;
	prim->ExtTessellate(&meshList, &scene->tessellatedPrimitives);

	for (vector<luxrays::ExtTriangleMesh *>::const_iterator mesh = meshList.begin(); mesh != meshList.end(); ++mesh) {
		const string meshName = "Mesh-" + ToString(*mesh);
		lcScene->DefineMesh(meshName, *mesh);
	}

	return meshList;
}

void LuxCoreRenderer::ConvertGeometry(luxcore::Scene *lcScene, ColorSystem &colorSpace) {
	LOG(LUX_INFO, LUX_NOERROR) << "Tessellating " << scene->primitives.size() << " primitives";

	// To keep track of all primitive mesh lists
	map<const Primitive *, vector<luxrays::ExtTriangleMesh *> > primMeshLists;

	for (size_t i = 0; i < scene->primitives.size(); ++i) {
		const Primitive *prim = scene->primitives[i].get();
		//LOG(LUX_DEBUG, LUX_NOERROR) << "Primitive type: " << ToClassName(prim);

		// InstancePrimitive and MotionPrimitive require special care
		if (dynamic_cast<const InstancePrimitive *>(prim)) {
			const InstancePrimitive *instance = dynamic_cast<const InstancePrimitive *>(prim);
			const string matName = GetLuxCoreMaterialName(lcScene, instance, colorSpace);

			const vector<boost::shared_ptr<Primitive> > &instanceSources = instance->GetInstanceSources();

			for (u_int i = 0; i < instanceSources.size(); ++i) {
				const Primitive *instancedSource = instanceSources[i].get();

				vector<luxrays::ExtTriangleMesh *> meshList;
				// Check if I have already defined one of the original primitive
				if (primMeshLists.count(instancedSource) < 1) {
					// I have to define the instanced primitive
					meshList = DefinePrimitive(lcScene, instancedSource);
					primMeshLists[instancedSource] = meshList;
				} else
					meshList = primMeshLists[instancedSource];

				if (meshList.size() == 0)
					continue;
	
				// Add the object
				for (vector<luxrays::ExtTriangleMesh *>::const_iterator mesh = meshList.begin(); mesh != meshList.end(); ++mesh) {
					// Define an instance of the mesh
					const string objName = "InstanceObject-" + ToString(prim) + "-" +
						ToString(*mesh);
					const string meshName = "Mesh-" + ToString(*mesh);

					const luxrays::Properties createObjProps(
						luxrays::Property("scene.objects." + objName + ".ply")(meshName) <<
						luxrays::Property("scene.objects." + objName + ".material")(matName) <<
						luxrays::Property("scene.objects." + objName + ".transformation")(instance->GetTransform().m));
					LOG(LUX_DEBUG, LUX_NOERROR) << "Creating object: [\n" << createObjProps << "]";
					lcScene->Parse(createObjProps);
				}
			}
		} else if (dynamic_cast<const MotionPrimitive *>(prim)) {
			const MotionPrimitive *motionPrim = dynamic_cast<const MotionPrimitive *>(prim);
			const string matName = GetLuxCoreMaterialName(lcScene, motionPrim, colorSpace);

			const vector<boost::shared_ptr<Primitive> > &instanceSources = motionPrim->GetInstanceSources();

			for (u_int i = 0; i < instanceSources.size(); ++i) {
				const Primitive *instancedSource = instanceSources[i].get();

				vector<luxrays::ExtTriangleMesh *> meshList;
				// Check if I have already defined one of the original primitive
				if (primMeshLists.count(instancedSource) < 1) {
					// I have to define the instanced primitive
					meshList = DefinePrimitive(lcScene, instancedSource);
					primMeshLists[instancedSource] = meshList;
				} else
					meshList = primMeshLists[instancedSource];

				if (meshList.size() == 0)
					continue;
	
				// Add the object
				for (vector<luxrays::ExtTriangleMesh *>::const_iterator mesh = meshList.begin(); mesh != meshList.end(); ++mesh) {
					// Define an instance of the mesh
					const string objName = "MotionInstanceObject-" + ToString(prim) + "-" +
						ToString(*mesh);
					const string meshName = "Mesh-" + ToString(*mesh);

					const luxrays::Properties createObjProps(
						luxrays::Property("scene.objects." + objName + ".ply")(meshName) <<
						luxrays::Property("scene.objects." + objName + ".material")(matName) <<
						luxrays::Property("scene.objects." + objName + ".transformation")(motionPrim->GetTransform(0.f).m));
					LOG(LUX_DEBUG, LUX_NOERROR) << "Creating object: [\n" << createObjProps << "]";
					lcScene->Parse(createObjProps);
				}
			}
		} else {
			vector<luxrays::ExtTriangleMesh *> meshList;
			if (primMeshLists.count(prim) < 1) {
				meshList = DefinePrimitive(lcScene, prim);
				primMeshLists[prim] = meshList;
			} else
				meshList = primMeshLists[prim];

			if (meshList.size() == 0)
				continue;

			// Add the object
			const string matName = GetLuxCoreMaterialName(lcScene, prim, colorSpace);

			for (vector<luxrays::ExtTriangleMesh *>::const_iterator mesh = meshList.begin(); mesh != meshList.end(); ++mesh) {
				const string objName = "Object-" + ToString(prim) + "-" +
					ToString(*mesh);
				const string meshName = "Mesh-" + ToString(*mesh);

				const luxrays::Properties createObjProps(
						luxrays::Property("scene.objects." + objName + ".ply")(meshName) <<
						luxrays::Property("scene.objects." + objName + ".material")(matName));
				LOG(LUX_DEBUG, LUX_NOERROR) << "Creating object: [\n" << createObjProps << "]";
				lcScene->Parse(createObjProps);
			}
		}
	}

	if (lcScene->GetObjectCount() == 0)
		throw std::runtime_error("LuxCoreRenderer can not render an empty scene");
}

void LuxCoreRenderer::ConvertCamera(luxcore::Scene *lcScene) {
	LOG(LUX_DEBUG, LUX_NOERROR) << "Camera type: " << ToClassName(scene->camera());
	PerspectiveCamera *perpCamera = dynamic_cast<PerspectiveCamera *>(scene->camera());
	if (!perpCamera)
		throw std::runtime_error("LuxCoreRenderer supports only PerspectiveCamera");

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
	Vector up(
			(scene->camera)["up.x"].FloatValue(),
			(scene->camera)["up.y"].FloatValue(),
			(scene->camera)["up.z"].FloatValue());
	if (renderEngineType == "FILESAVER") {
		// I snap the up vector to the Z axis so moving inside LuxVR
		// is a lot easier and work as expected
		up.x = 0.f;
		up.y = 0.f;
		up.z = 1.f;
	}

	const luxrays::Properties createCameraProps(
		luxrays::Property("scene.camera.lookat.orig")(orig) <<
		luxrays::Property("scene.camera.lookat.target")(target) <<
		luxrays::Property("scene.camera.up")(up) <<
		luxrays::Property("scene.camera.screenwindow")(
			(scene->camera)["ScreenWindow.0"].FloatValue(),
			(scene->camera)["ScreenWindow.1"].FloatValue(),
			(scene->camera)["ScreenWindow.2"].FloatValue(),
			(scene->camera)["ScreenWindow.3"].FloatValue()) <<
		luxrays::Property("scene.camera.fieldofview")(Degrees((scene->camera)["fov"].FloatValue())) <<
		luxrays::Property("scene.camera.lensradius")((scene->camera)["LensRadius"].FloatValue()) <<
		luxrays::Property("scene.camera.focaldistance")((scene->camera)["FocalDistance"].FloatValue()) <<
		luxrays::Property("scene.camera.cliphither")((scene->camera)["ClipHither"].FloatValue()) <<
		luxrays::Property("scene.camera.clipyon")((scene->camera)["ClipYon"].FloatValue()));
	LOG(LUX_DEBUG, LUX_NOERROR) << "Creating camera: [\n" << createCameraProps << "]";
	lcScene->Parse(createCameraProps);
}

luxcore::Scene *LuxCoreRenderer::CreateLuxCoreScene(const luxrays::Properties &lcConfigProps, ColorSystem &colorSpace) {
	luxcore::Scene *lcScene = new luxcore::Scene();

	// Tell to the cache to not delete mesh data (they are pointed by Lux
	// primitives too and they will be deleted by Lux Context)
	lcScene->SetDeleteMeshData(false);

	//--------------------------------------------------------------------------
	// Setup the camera
	//--------------------------------------------------------------------------

	ConvertCamera(lcScene);

	//--------------------------------------------------------------------------
	// Setup default material
	//--------------------------------------------------------------------------

	lcScene->Parse(luxrays::Property("scene.materials.mat_default.type")("matte") <<
		luxrays::Property("scene.materials.mat_default.kd")(.9f, .9f, .9f));

	//--------------------------------------------------------------------------
	// Setup lights
	//--------------------------------------------------------------------------

	ConvertLights(lcScene);

	//--------------------------------------------------------------------------
	// Convert geometry
	//--------------------------------------------------------------------------

	ConvertGeometry(lcScene, colorSpace);

	return lcScene;
}

luxrays::Properties LuxCoreRenderer::CreateLuxCoreConfig() {
	luxrays::Properties cfgProps;

	cfgProps <<
			luxrays::Property("opencl.platform.index")(-1) <<
			luxrays::Property("opencl.cpu.use")(true) <<
			luxrays::Property("opencl.gpu.use")(true);

	//--------------------------------------------------------------------------
	// Accelerator related settings
	//--------------------------------------------------------------------------

	LOG(LUX_DEBUG, LUX_NOERROR) << "Accelerator type: " << ToClassName(scene->aggregate.get());
	if (((dynamic_cast<TaBRecKdTreeAccel *>(scene->aggregate.get())) != 0) || ((dynamic_cast<BVHAccel *>(scene->aggregate.get())) != NULL)) {
		// Map kdtree and bvh to luxrays' bvh accel
		cfgProps << luxrays::Property("accelerator.type")("BVH");
	}

	//--------------------------------------------------------------------------
	// Epsilon related settings
	//--------------------------------------------------------------------------

	cfgProps <<
			luxrays::Property("scene.epsilon.min")(MachineEpsilon::GetMin()) <<
			luxrays::Property("scene.epsilon.max")(MachineEpsilon::GetMax());

	//--------------------------------------------------------------------------
	// Surface integrator related settings
	//--------------------------------------------------------------------------

	LOG(LUX_DEBUG, LUX_NOERROR) << "Surface integrator type: " << ToClassName(scene->surfaceIntegrator);
	if (dynamic_cast<PathIntegrator *>(scene->surfaceIntegrator)) {
		// Path tracing
		PathIntegrator *path = dynamic_cast<PathIntegrator *>(scene->surfaceIntegrator);
		const int maxDepth = (*path)["maxDepth"].IntValue();

		cfgProps <<
			luxrays::Property("renderengine.type")("PATHOCL") <<
			luxrays::Property("path.maxdepth")(maxDepth);
	} else if (dynamic_cast<BidirIntegrator *>(scene->surfaceIntegrator)) {
		// Bidirectional path tracing
		BidirIntegrator *bidir = dynamic_cast<BidirIntegrator *>(scene->surfaceIntegrator);
		const int maxEyeDepth = (*bidir)["maxEyeDepth"].IntValue();
		const int maxLightDepth = (*bidir)["maxLightDepth"].IntValue();

		cfgProps <<
			luxrays::Property("renderengine.type")("BIDIRVMCPU") <<
			luxrays::Property("path.maxdepth")(maxLightDepth) <<
			luxrays::Property("light.maxdepth")(maxEyeDepth);
	} else {
		// Unmapped surface integrator, just use path tracing
		LOG(LUX_WARNING, LUX_UNIMPLEMENT) << "LuxCoreRenderer doesn't support the SurfaceIntegrator, falling back to path tracing";
		cfgProps <<
			luxrays::Property("renderengine.type")("PATHOCL");
	}

	//--------------------------------------------------------------------------
	// Film related settings
	//--------------------------------------------------------------------------

	Film *film = scene->camera()->film;
	const int imageWidth = film->GetXPixelCount();
	const int imageHeight = film->GetYPixelCount();

	float cropWindow[4] = {
		(*film)["cropWindow.0"].FloatValue(),
		(*film)["cropWindow.1"].FloatValue(),
		(*film)["cropWindow.2"].FloatValue(),
		(*film)["cropWindow.3"].FloatValue()
	};
	if ((cropWindow[0] != 0.f) || (cropWindow[1] != 1.f) ||
			(cropWindow[2] != 0.f) || (cropWindow[3] != 1.f))
		throw std::runtime_error("LuxCoreRenderer doesn't yet support border rendering");

	cfgProps <<
			luxrays::Property("film.width")(imageWidth) <<
			luxrays::Property("film.height")(imageHeight) <<
			// Alpha channel is always enabled in LuxRender
			luxrays::Property("film.outputs.LuxCoreRenderer_0.type")("ALPHA") <<
			luxrays::Property("film.outputs.LuxCoreRenderer_0.filename")("image.exr") <<
			luxrays::Property("film.outputs.LuxCoreRenderer_1.type")("DEPTH") <<
			luxrays::Property("film.outputs.LuxCoreRenderer_1.filename")("image.exr");

	//--------------------------------------------------------------------------
	// Sampler related settings
	//--------------------------------------------------------------------------

	LOG(LUX_DEBUG, LUX_NOERROR) << "Sampler type: " << ToClassName(scene->sampler);
	if (dynamic_cast<MetropolisSampler *>(scene->sampler)) {
		MetropolisSampler *sampler = dynamic_cast<MetropolisSampler *>(scene->sampler);
		const int maxRejects = (*sampler)["maxRejects"].IntValue();
		const float pLarge = (*sampler)["pLarge"].FloatValue();
		const float range = (*sampler)["range"].FloatValue() * 2.f / imageHeight;

		cfgProps <<
			luxrays::Property("sampler.type")("METROPOLIS") <<
			luxrays::Property("sampler.metropolis.maxconsecutivereject")(maxRejects) <<
			luxrays::Property("sampler.metropolis.largesteprate")(pLarge) <<
			luxrays::Property("sampler.metropolis.imagemutationrate")(range);
	} else if (dynamic_cast<LDSampler *>(scene->sampler) || dynamic_cast<SobolSampler *>(scene->sampler)) {
		cfgProps << luxrays::Property("sampler.type")("SOBOL");
	} else if (dynamic_cast<RandomSampler *>(scene->sampler)) {
		cfgProps << luxrays::Property("sampler.type")("RANDOM");
	} else {
		// Unmapped sampler, just use random
		LOG(LUX_WARNING, LUX_UNIMPLEMENT) << "LuxCoreRenderer doesn't support the Sampler, falling back to random sampler";
		cfgProps <<
			luxrays::Property("sampler.type")("RANDOM");
	}

	//--------------------------------------------------------------------------
	// Pixel filter related settings
	//--------------------------------------------------------------------------

	const Filter *filter = film->GetFilter();
	const string filterType = (*filter)["type"].StringValue();
	if (filterType == "box")
		cfgProps << luxrays::Property("film.filter.type")("BOX");
	else if (filterType == "gaussian")
		cfgProps << luxrays::Property("film.filter.type")("GAUSSIAN");
	else if (filterType == "mitchell")
		cfgProps << luxrays::Property("film.filter.type")("MITCHELL");
	else if (filterType == "mitchell_ss")
		cfgProps << luxrays::Property("film.filter.type")("MITCHELL_SS");
	else {
		LOG(LUX_WARNING, LUX_UNIMPLEMENT) << "LuxCoreRenderer doesn't support the filter type " << filterType <<
				", using MITCHELL filter instead";
		cfgProps << luxrays::Property("film.filter.type")("MITCHELL");
	}

	//--------------------------------------------------------------------------

	LOG(LUX_DEBUG, LUX_NOERROR) << "LuxCore configuration: [\n" << cfgProps << "]";

	// Add overwrite properties
	cfgProps.Set(overwriteConfig);

	// Check if light buffer is needed and required
	renderEngineType = cfgProps.Get(luxrays::Property("renderengine.type")("PATHOCL")).Get<string>();
	if (((renderEngineType == "LIGHTCPU") ||
		(renderEngineType == "BIDIRCPU") ||
		(renderEngineType == "BIDIRHYBRID") ||
		(renderEngineType == "CBIDIRHYBRID") ||
		(renderEngineType == "BIDIRVMCPU")) && !dynamic_cast<BidirIntegrator *>(scene->surfaceIntegrator)) {
		throw std::runtime_error("You have to select bidirectional surface integrator in order to use the selected render engine");
	}

	//--------------------------------------------------------------------------
	// Tone mapping related settings
	//
	// They are exported only if using FILESAVER rendering engine otherwise LuxCore
	// uses Lux image pipeline and it is not in charge of tone mapping. I handle
	// only linear tone mapping because it is the only one supported by
	// RTPATHOCL (i.e. LuxVR)
	//--------------------------------------------------------------------------

	// Avoid to overwrite an "overwrite" setting
	if ((renderEngineType == "FILESAVER") && !overwriteConfig.IsDefined("film.tonemap.type")) {
		const int type = (*film)["TonemapKernel"].IntValue();

		if (type == FlexImageFilm::TMK_Linear) {
			// Translate linear tone mapping settings
			const float sensitivity = (*film)["LinearSensitivity"].FloatValue();
			const float exposure = (*film)["LinearExposure"].FloatValue();
			const float fstop = (*film)["LinearFStop"].FloatValue();
			const float gamma = (*film)["LinearGamma"].FloatValue();

			// Check LinearOp class for an explanation of the following formula
			const float factor = exposure / (fstop * fstop) * sensitivity * 0.65f / 10.f * powf(118.f / 255.f, gamma);

			cfgProps.Set(luxrays::Property("film.imagepipeline.0.type")("LINEAR"));
			cfgProps.Set(luxrays::Property("film.imagepipeline.0.linear.scale")(factor));
			cfgProps.Set(luxrays::Property("film.imagepipeline.1.type")("GAMMA_CORRECTION"));
			cfgProps.Set(luxrays::Property("film.imagepipeline.1.value")(2.2f));
//		} else if (type == FlexImageFilm::TMK_AutoLinear) {
//			cfgProps.Set(luxrays::Property("film.tonemap.type")("AUTOLINEAR"));
		} else
			LOG(LUX_WARNING, LUX_UNIMPLEMENT) << "LuxVR supports only linear tone mapping, ignoring tone mapping settings";
	}

	return cfgProps;
}

void LuxCoreRenderer::UpdateLuxFilm(luxcore::RenderSession *session) {
	const luxcore::Film &lcFilm = session->GetFilm();

	Film *film = scene->camera()->film;
	ColorSystem colorSpace = film->GetColorSpace();
	const u_int width = film->GetXPixelCount();
	const u_int height = film->GetYPixelCount();

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

	// Lock LuxCore film
//	boost::unique_lock<boost::mutex> lock(session->filmMutex);

	if (previousFilm_RADIANCE_PER_PIXEL_NORMALIZEDs.size() > 0) {
		// Copy the information from PER_PIXEL_NORMALIZED buffer

		for (u_int i = 0; i < previousFilm_RADIANCE_PER_PIXEL_NORMALIZEDs.size(); ++i) {
			const float *channel_RADIANCE_PER_PIXEL_NORMALIZED = lcFilm.GetChannel<float>(luxcore::Film::CHANNEL_RADIANCE_PER_PIXEL_NORMALIZED, i);
			const float *channel_ALPHA = lcFilm.GetChannel<float>(luxcore::Film::CHANNEL_ALPHA);
			const float *channel_DEPTH = lcFilm.GetChannel<float>(luxcore::Film::CHANNEL_DEPTH);

			for (u_int pixelY = 0; pixelY < height; ++pixelY) {
				for (u_int pixelX = 0; pixelX < width; ++pixelX) {
					const float *spNew = &channel_RADIANCE_PER_PIXEL_NORMALIZED[(pixelX + pixelY * width) * 4];
					const luxrays::Spectrum newRadiance(spNew[0], spNew[1], spNew[2]);
					const float &newWeight = spNew[3];

					const float *spOld = &(previousFilm_RADIANCE_PER_PIXEL_NORMALIZEDs[i][(pixelX + pixelY * width) * 4]);
					const luxrays::Spectrum oldRadiance(spOld[0], spOld[1], spOld[2]);
					const float &oldWeight = spOld[3];

					luxrays::Spectrum deltaRadiance = newRadiance - oldRadiance;
					const float deltaWeight = newWeight - oldWeight;

					// Delay the update if deltaWeight is < 0.0
					if (deltaWeight > 0.f) {
						deltaRadiance /= deltaWeight;

						const float newAlpha = previousFilm_ALPHA ?
							channel_ALPHA[(pixelX + pixelY * width) * 2] : 1.f;
						// I have to clamp alpha values because then can be outside the [0.0, 1.0]
						// range (i.e. some pixel filter can have negative weights and lead
						// to negative values)
						const float deltaAlpha = max(newAlpha - previousFilm_ALPHA[(pixelX + pixelY * width) * 2], 0.f) / deltaWeight;

						const float newDepth = previousFilm_DEPTH ?
							channel_DEPTH[pixelX + pixelY * width] : 0.f;

						XYZColor xyz = colorSpace.ToXYZ(RGBColor(deltaRadiance.r, deltaRadiance.g, deltaRadiance.b));

						if (xyz.Y() >= 0.f) {
							// Flip the image upside down
							Contribution contrib(pixelX, height - 1 - pixelY, xyz, deltaAlpha, newDepth, deltaWeight, eyeBufferId, i);
							film->AddSampleNoFiltering(&contrib);
						}
					}
				}
			}
		}
	}

	if (previousFilm_RADIANCE_PER_SCREEN_NORMALIZEDs.size() > 0) {
		// Copy the information from PER_SCREEN_NORMALIZED buffer

		for (u_int i = 0; i <previousFilm_RADIANCE_PER_SCREEN_NORMALIZEDs.size(); ++i) {
			const float *channel_RADIANCE_PER_SCREEN_NORMALIZED = lcFilm.GetChannel<float>(luxcore::Film::CHANNEL_RADIANCE_PER_SCREEN_NORMALIZED, i);
			const float *channel_ALPHA = lcFilm.GetChannel<float>(luxcore::Film::CHANNEL_ALPHA);
			const float *channel_DEPTH = lcFilm.GetChannel<float>(luxcore::Film::CHANNEL_DEPTH);

			for (u_int pixelY = 0; pixelY < height; ++pixelY) {
				for (u_int pixelX = 0; pixelX < width; ++pixelX) {
					const float *spNew = &channel_RADIANCE_PER_SCREEN_NORMALIZED[(pixelX + pixelY * width) * 3];
					const luxrays::Spectrum newRadiance(spNew[0], spNew[1], spNew[2]);
					const float &newWeight = spNew[3];

					const float *spOld = &(previousFilm_RADIANCE_PER_SCREEN_NORMALIZEDs[i][(pixelX + pixelY * width) * 3]);
					const luxrays::Spectrum oldRadiance(spOld[0], spOld[1], spOld[2]);
					const float &oldWeight = spOld[3];

					luxrays::Spectrum deltaRadiance = newRadiance - oldRadiance;
					const float deltaWeight = newWeight - oldWeight;

					// Delay the update if deltaWeight is < 0.0
					if (deltaWeight > 0.f) {
						// This is required to cancel the "* weight" inside AddSampleNoFiltering()
						deltaRadiance /= deltaWeight;

						const float newAlpha = previousFilm_ALPHA ?
							channel_ALPHA[(pixelX + pixelY * width) * 2] : 1.f;
						// I have to clamp alpha values because then can be outside the [0.0, 1.0]
						// range (i.e. some pixel filter can have negative weights and lead
						// to negative values)
						const float deltaAlpha = max(newAlpha - previousFilm_ALPHA[(pixelX + pixelY * width) * 2], 0.f) / deltaWeight;

						const float newDepth = previousFilm_DEPTH ?
							channel_DEPTH[pixelX + pixelY * width] : 0.f;

						XYZColor xyz = colorSpace.ToXYZ(RGBColor(deltaRadiance.r, deltaRadiance.g, deltaRadiance.b));

						if (xyz.Y() >= 0.f) {
							// Flip the image upside down
							Contribution contrib(pixelX, height - 1 - pixelY, xyz, deltaAlpha, newDepth, deltaWeight, lightBufferId, i);
							film->AddSampleNoFiltering(&contrib);
						}
					}
				}
			}
		}
	}

	const double newSampleCount = lcFilm.GetTotalSampleCount();
	film->AddSampleCount(newSampleCount - previousFilmSampleCount);
	previousFilmSampleCount = newSampleCount;

	// Copy the LuxCore film channels for the next update
	if (previousFilm_RADIANCE_PER_SCREEN_NORMALIZEDs.size() > 0) {
		for (u_int i = 0; i < previousFilm_RADIANCE_PER_SCREEN_NORMALIZEDs.size(); ++i) {
			const float *channel_RADIANCE_PER_SCREEN_NORMALIZED = lcFilm.GetChannel<float>(luxcore::Film::CHANNEL_RADIANCE_PER_SCREEN_NORMALIZED, i);
			std::copy(channel_RADIANCE_PER_SCREEN_NORMALIZED, channel_RADIANCE_PER_SCREEN_NORMALIZED + width * height * 4,
					previousFilm_RADIANCE_PER_SCREEN_NORMALIZEDs[i]);
		}
	}

	if (previousFilm_RADIANCE_PER_PIXEL_NORMALIZEDs.size() > 0) {
		for (u_int i = 0; i < previousFilm_RADIANCE_PER_PIXEL_NORMALIZEDs.size(); ++i) {
			const float *channel_RADIANCE_PER_PIXEL_NORMALIZED = lcFilm.GetChannel<float>(luxcore::Film::CHANNEL_RADIANCE_PER_PIXEL_NORMALIZED, i);
			std::copy(channel_RADIANCE_PER_PIXEL_NORMALIZED, channel_RADIANCE_PER_PIXEL_NORMALIZED + width * height * 4,
					previousFilm_RADIANCE_PER_PIXEL_NORMALIZEDs[i]);
		}
	}

	if (previousFilm_ALPHA) {
		const float *channel_ALPHA = lcFilm.GetChannel<float>(luxcore::Film::CHANNEL_ALPHA);
		std::copy(channel_ALPHA, channel_ALPHA + width * height * 2,
					previousFilm_ALPHA);
	}

	if (previousFilm_DEPTH) {
		const float *channel_DEPTH = lcFilm.GetChannel<float>(luxcore::Film::CHANNEL_DEPTH);		
		std::copy(channel_DEPTH, channel_DEPTH + width * height,
					previousFilm_DEPTH);
	}

}

void LuxCoreRenderer::Render(Scene *s) {
	try {
		std::auto_ptr<luxcore::Scene> lcScene;
		luxrays::Properties lcConfigProps;

		luxcore::Init(::LuxCoreDebugHandler);

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

			try {
				// Build the LuxCore rendering configuration
				lcConfigProps.Set(CreateLuxCoreConfig());

				// Build the LuxCore scene to render
				ColorSystem colorSpace = scene->camera()->film->GetColorSpace();
				lcScene.reset(CreateLuxCoreScene(lcConfigProps, colorSpace));

				if (lcScene->GetLightCount() == 0)
					throw std::runtime_error("The scene doesn't include any light source");
#if !defined(LUXRAYS_DISABLE_OPENCL)
			} catch (cl::Error err) {
				LOG(LUX_SEVERE, LUX_SYSTEM) << "OpenCL ERROR: " << err.what() << "(" << luxrays::oclErrorString(err.err()) << ")";
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

		LuxCoreStatistics *lcStats = static_cast<LuxCoreStatistics *>(rendererStatistics);

		std::auto_ptr<luxcore::RenderConfig> config(new luxcore::RenderConfig(lcConfigProps, lcScene.get()));
		std::auto_ptr<luxcore::RenderSession> session(new luxcore::RenderSession(config.get()));

		// Statistic information about the devices will be available only after
		// the start of the RenderSession
		lcStats->deviceCount = 0;
		lcStats->deviceNames = "";

		//----------------------------------------------------------------------

		// start the timer
		lcStats->start();

		// Dade - preprocessing done
		preprocessDone = true;
		scene->SetReady();

		//----------------------------------------------------------------------
		// Start the rendering
		//----------------------------------------------------------------------

		session->Start();

		//----------------------------------------------------------------------
		// Initialize the statistics
		//----------------------------------------------------------------------

		session->UpdateStats();
		const luxrays::Properties &stats = session->GetStats();
		luxrays::Property devices = stats.Get("stats.renderengine.devices");
		lcStats->triangleCount = stats.Get("stats.dataset.trianglecount").Get<u_longlong>();

		lcStats->deviceCount = devices.GetSize();
		if (lcStats->deviceCount > 0) {
			// Build the list of device names used
			stringstream ss;
			for (u_int i = 0; i < luxrays::Min<u_int>(lcStats->deviceCount, lcStats->deviceMaxMemory.size()); ++i) {
				if (i != 0)
					ss << ",";

				string name = devices.Get<string>(i);
				lcStats->deviceMaxMemory[i] = stats.Get("stats.renderengine.devices." + name +".memory.total").Get<u_longlong>();
				lcStats->deviceMemoryUsed[i] = stats.Get("stats.renderengine.devices." + name +".memory.used").Get<u_longlong>();

				// I'm paranoid...				
				boost::replace_all(name, ",", "_");
				ss << name;
			}
			lcStats->deviceNames = ss.str();
		}

		// I don't really need to run the rendering if I'm using the FileSaverRenderEngine
		if (config->GetProperty("renderengine.type").Get<string>() != "FILESAVER") {
			const double startTime = luxrays::WallClockTime();

			// Not declared const because they can be overwritten
			u_int haltTime = config->GetProperty("batch.halttime").Get<u_int>();
			u_int haltSpp = config->GetProperty("batch.haltspp").Get<u_int>();
			float haltThreshold = config->GetProperty("batch.haltthreshold").Get<float>();

			double lastFilmUpdate = startTime - 2.0; // -2.0 is to anticipate the first display update by 2 secs
			Film *film = scene->camera()->film;
			int xStart, xEnd, yStart, yEnd;
			film->GetSampleExtent(&xStart, &xEnd, &yStart, &yEnd);

			// Used to feed LuxRender film with only the delta information from the previous update
			const u_int lcFilmWidth = session->GetFilm().GetWidth();
			const u_int lcFilmHeight = session->GetFilm().GetHeight();

			previousFilm_RADIANCE_PER_PIXEL_NORMALIZEDs.resize(session->GetFilm().GetChannelCount(
				luxcore::Film::CHANNEL_RADIANCE_PER_PIXEL_NORMALIZED));
			for (u_int i = 0; i < previousFilm_RADIANCE_PER_PIXEL_NORMALIZEDs.size(); ++i) {
				previousFilm_RADIANCE_PER_PIXEL_NORMALIZEDs[i] = new float[lcFilmWidth * lcFilmHeight * 4];
				std::fill(previousFilm_RADIANCE_PER_PIXEL_NORMALIZEDs[i], previousFilm_RADIANCE_PER_PIXEL_NORMALIZEDs[i] +
					lcFilmWidth * lcFilmHeight * 4, 0.f);
			}
			previousFilm_RADIANCE_PER_SCREEN_NORMALIZEDs.resize(session->GetFilm().GetChannelCount(
				luxcore::Film::CHANNEL_RADIANCE_PER_SCREEN_NORMALIZED));
			for (u_int i = 0; i < previousFilm_RADIANCE_PER_SCREEN_NORMALIZEDs.size(); ++i) {
				previousFilm_RADIANCE_PER_SCREEN_NORMALIZEDs[i] = new float[lcFilmWidth * lcFilmHeight * 3];
				std::fill(previousFilm_RADIANCE_PER_SCREEN_NORMALIZEDs[i], previousFilm_RADIANCE_PER_SCREEN_NORMALIZEDs[i] +
					lcFilmWidth * lcFilmHeight * 3, 0.f);
			}
			if (session->GetFilm().GetChannelCount(luxcore::Film::CHANNEL_ALPHA) > 0) {
				previousFilm_ALPHA = new float[lcFilmWidth * lcFilmHeight * 2];
				std::fill(previousFilm_ALPHA, previousFilm_ALPHA +
						lcFilmWidth * lcFilmHeight * 2, 0.f);
			}
			if (session->GetFilm().GetChannelCount(luxcore::Film::CHANNEL_DEPTH) > 0) {
				previousFilm_DEPTH = new float[lcFilmWidth * lcFilmHeight];
				std::fill(previousFilm_DEPTH, previousFilm_DEPTH +
						lcFilmWidth * lcFilmHeight, 0.f);
			}
			previousFilmSampleCount = 0.0;

			for (;;) {
				if (state == PAUSE) {
					session->BeginSceneEdit();
					while (state == PAUSE && !boost::this_thread::interruption_requested())
						boost::this_thread::sleep(boost::posix_time::seconds(1));
					session->EndSceneEdit();
				}
				if ((state == TERMINATE) || boost::this_thread::interruption_requested())
					break;

				boost::this_thread::sleep(boost::posix_time::millisec(1000));

				session->UpdateStats();

				if (luxrays::WallClockTime() - lastFilmUpdate > film->getldrDisplayInterval()) {
					// Update LuxRender film too
					UpdateLuxFilm(session.get());

					lastFilmUpdate =  luxrays::WallClockTime();
				}

				const double now = luxrays::WallClockTime();
				const double elapsedTime = now - startTime;
				const u_int pass = stats.Get("stats.renderengine.pass").Get<u_int>();
				// Convergence test has been updated inside UpdateStats()
				const float convergence = stats.Get("stats.renderengine.convergence").Get<float>();
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
				lcStats->averageSampleSec = stats.Get("stats.renderengine.total.samplesec").Get<double>();
				for (u_int i = 0; i < luxrays::Min<u_int>(lcStats->deviceCount, lcStats->deviceMaxMemory.size()); ++i) {
					string name = devices.Get<string>(i);					
					lcStats->deviceRaySecs[i] = stats.Get("stats.renderengine.devices." + name +".performance.total").Get<double>();
					lcStats->deviceMemoryUsed[i] = stats.Get("stats.renderengine.devices." + name +".memory.used").Get<u_longlong>();
				}

				// Print some information about the rendering progress
				LC_LOG(boost::str(boost::format(
						"[Elapsed time: %3d/%dsec][Samples %4d/%d][Convergence %f%%][Avg. samples/sec % 3.2fM on %.1fK tris]") %
						int(elapsedTime) % int(haltTime) % pass % haltSpp % (100.f * convergence) %
						(lcStats->averageSampleSec / 1000000.0) %
						(stats.Get("stats.dataset.trianglecount").Get<u_longlong>() / 1000.0)));

				film->CheckWriteOuputInterval();
			}
		} else
			Terminate();

		// Stop the rendering
		session->Stop();
#if !defined(LUXRAYS_DISABLE_OPENCL)
	} catch (cl::Error err) {
		LOG(LUX_SEVERE, LUX_SYSTEM) << "OpenCL ERROR: " << err.what() << "(" << luxrays::oclErrorString(err.err()) << ")";
#endif
	} catch (std::runtime_error err) {
		LOG(LUX_SEVERE, LUX_SYSTEM) << "RUNTIME ERROR: " << err.what();
	} catch (std::exception err) {
		LOG(LUX_SEVERE, LUX_SYSTEM) << "ERROR: " << err.what();
	}

//	delete previousFilm;
//	previousFilm = NULL;

	LC_LOG("Done.");

	// I change the current signal to exit in order to disable the creation
	// of new threads after this point
	Terminate();

	// Flush the contribution pool
	scene->camera()->film->contribPool->Flush();
	scene->camera()->film->contribPool->Delete();
}

void LuxCoreRenderer::Pause() {
	boost::mutex::scoped_lock lock(classWideMutex);
	state = PAUSE;
	rendererStatistics->stop();
}

void LuxCoreRenderer::Resume() {
	boost::mutex::scoped_lock lock(classWideMutex);
	state = RUN;
	rendererStatistics->start();
}

void LuxCoreRenderer::Terminate() {
	boost::mutex::scoped_lock lock(classWideMutex);
	state = TERMINATE;
}

//------------------------------------------------------------------------------

void LuxCoreDebugHandler(const char *msg) {
	LOG(LUX_DEBUG, LUX_NOERROR) << msg;
}

Renderer *LuxCoreRenderer::CreateRenderer(const ParamSet &params) {
	luxrays::Properties config;

	// Local (for network rendering) host configuration file. It is a properties
	// file that can be used overwrite settings.
	const string configFile = params.FindOneString("localconfigfile", "");
	if (configFile != "")
		config.SetFromFile(configFile);

	// A list of properties that can be used to overwrite generated properties
	u_int nItems;
	const string *items = params.FindString("config", &nItems);
	if (items) {
		for (u_int i = 0; i < nItems; ++i)
			config.SetFromString(items[i] + "\n");
	}

	return new LuxCoreRenderer(config);
}

// For compatibility with the past
static DynamicLoader::RegisterRenderer<LuxCoreRenderer> r("slg");
static DynamicLoader::RegisterRenderer<LuxCoreRenderer> r2("luxcore");
