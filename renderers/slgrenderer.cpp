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

#include "samplers/metrosampler.h"
#include "samplers/random.h"

#include "integrators/path.h"
#include "integrators/bidirectional.h"

#include "textures/blackbody.h"
#include "textures/constant.h"

#include "light.h"
#include "lights/sun.h"
#include "lights/sky.h"
#include "lights/sky2.h"
#include "lights/infinite.h"
#include "lights/infinitesample.h"

#include "materials/matte.h"
#include "materials/mirror.h"
#include "materials/glass.h"
#include "materials/glossy2.h"

#include "luxrays/core/context.h"
#include "luxrays/utils/core/exttrianglemesh.h"
#include "luxrays/opencl/utils.h"
#include "rendersession.h"


using namespace lux;

template <class T> static std::string ToClassName(T *ptr) {
	if (ptr)
		return typeid(*ptr).name();
	else
		return "NULL";
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

	AddStringConstant(*this, "name", "Name of current renderer", "slg");

	rendererStatistics = new SLGStatistics(this);

	overwriteConfig = config;
}

SLGRenderer::~SLGRenderer() {
	boost::mutex::scoped_lock lock(classWideMutex);

	delete rendererStatistics;

	if ((state != TERMINATE) && (state != INIT))
		throw std::runtime_error("Internal error: called SLGRenderer::~SLGRenderer() while not in TERMINATE or INIT state.");

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

void SLGRenderer::DefineSLGDefaultTexMap(luxrays::sdl::Scene *slgScene) {
	if (!slgScene->texMapCache->FindTextureMap("tex_default", 1.f)) {
		luxrays::Spectrum *defaultTexMap = new luxrays::Spectrum[1];
		defaultTexMap[0].r = 1.f;
		defaultTexMap[0].g = 1.f;
		defaultTexMap[0].b = 1.f;
		slgScene->DefineTexMap("tex_default", defaultTexMap, 1.f, 1, 1);
	}
}

string SLGRenderer::GetSLGTexName(luxrays::sdl::Scene *slgScene,
		MIPMap *mipMap, const float gamma) {
	if (!mipMap) {
		DefineSLGDefaultTexMap(slgScene);
		return "tex_default";
	}

	// Check if the texture map has already been defined
	const string texName = mipMap->GetName();
	if (slgScene->texMapCache->FindTextureMap(texName, gamma))
		return texName;

	if (dynamic_cast<const MIPMapFastImpl<TextureColor<float, 3> > *>(mipMap))
		return GetSLGTexName(slgScene, (MIPMapImpl<TextureColor<float, 3> > *)mipMap, gamma);
	else if (dynamic_cast<const MIPMapFastImpl<TextureColor<float, 4> > *>(mipMap))
		return GetSLGTexName(slgScene, (MIPMapImpl<TextureColor<float, 4> > *)mipMap, gamma);
	else {
		// Unsupported type
		LOG(LUX_WARNING, LUX_UNIMPLEMENT) << "SLGrenderer supports only RGB(A) float texture maps (i.e. not " <<
					ToClassName(mipMap) << "). Replacing an unsupported texture map with a white texture.";
		DefineSLGDefaultTexMap(slgScene);
		return "tex_default";
	}
}

string SLGRenderer::GetSLGTexName(luxrays::sdl::Scene *slgScene,
		MIPMapFastImpl<TextureColor<float, 3> > *mipMap, const float gamma) {
	const BlockedArray<TextureColor<float, 3> > *map = mipMap->GetSingleMap();

	luxrays::Spectrum *slgRGBMap = new luxrays::Spectrum[map->uSize() * map->vSize()];

	for (u_int y = 0; y < map->vSize(); ++y) {
		for (u_int x = 0; x < map->uSize(); ++x) {
			const TextureColor<float, 3> &col = (*map)(x, y);

			const u_int index = (x + y * map->uSize());
			slgRGBMap[index].r = col.c[0];
			slgRGBMap[index].g = col.c[1];
			slgRGBMap[index].b = col.c[2];
		}
	}

	const string texName = mipMap->GetName();
	slgScene->DefineTexMap(texName,
			slgRGBMap, gamma, (u_int)map->uSize(), (u_int)map->vSize());

	return texName;
}

string SLGRenderer::GetSLGTexName(luxrays::sdl::Scene *slgScene,
		MIPMapFastImpl<TextureColor<float, 4> > *mipMap, const float gamma) {
	const BlockedArray<TextureColor<float, 4> > *map = mipMap->GetSingleMap();

	luxrays::Spectrum *slgRGBMap = new luxrays::Spectrum[map->uSize() * map->vSize()];
	float *slgAlphaMap = new float[map->uSize() * map->vSize()];

	for (u_int y = 0; y < map->vSize(); ++y) {
		for (u_int x = 0; x < map->uSize(); ++x) {
			const TextureColor<float, 4> &col = (*map)(x, y);

			const u_int index = (x + y * map->uSize());
			slgRGBMap[index].r = col.c[0];
			slgRGBMap[index].g = col.c[1];
			slgRGBMap[index].b = col.c[2];
			slgAlphaMap[index] = col.c[3];
		}
	}

	const string texName = mipMap->GetName();
	slgScene->DefineTexMap(texName,
			slgRGBMap, slgAlphaMap, gamma, (u_int)map->uSize(), (u_int)map->vSize());

	return texName;
}

string SLGRenderer::GetSLGMaterialName(luxrays::sdl::Scene *slgScene, const Primitive *prim) {
	LOG(LUX_DEBUG, LUX_NOERROR) << "Primitive type: " << ToClassName(prim);

	Material *mat = NULL;
	string matName;

	//----------------------------------------------------------------------
	// Check if it is a Shape
	//----------------------------------------------------------------------
	if (dynamic_cast<const Shape *>(prim)) {
		const Shape *shape = dynamic_cast<const Shape *>(prim);
		mat = shape->GetMaterial();
		if (!mat)
			return "mat_default";
		matName = mat->GetName();
	} else
	//----------------------------------------------------------------------
	// Check if it is an InstancePrimitive
	//----------------------------------------------------------------------
	if (dynamic_cast<const InstancePrimitive *>(prim)) {
		const InstancePrimitive *instance = dynamic_cast<const InstancePrimitive *>(prim);
		mat = instance->GetMaterial();
		if (!mat)
			return "mat_default";
		matName = mat->GetName();
	} else
	//----------------------------------------------------------------------
	// Check if it is an AreaLight
	//----------------------------------------------------------------------
	if (dynamic_cast<const AreaLightPrimitive *>(prim)) {
		const AreaLightPrimitive *alPrim = dynamic_cast<const AreaLightPrimitive *>(prim);
		AreaLight *al = alPrim->GetAreaLight();
		matName = al->GetName();

		// Check if I haven't already defined this AreaLight
		if (slgScene->materialIndices.count(matName) < 1) {
			// Define a new area light material

			Texture<SWCSpectrum> *tex = al->GetTexture();

			const float gain = (*al)["gain"].FloatValue();
			const float power = (*al)["power"].FloatValue();
			const float efficacy = (*al)["efficacy"].FloatValue();
			const float area = (*al)["area"].FloatValue();

			// Check the type of texture used
			LOG(LUX_DEBUG, LUX_NOERROR) << "AreaLight texture type: " << ToClassName(tex);
			ConstantRGBColorTexture *constRGBTex = dynamic_cast<ConstantRGBColorTexture *>(tex);
			BlackBodyTexture *blackBodyTexture = dynamic_cast<BlackBodyTexture *>(tex);

			if (constRGBTex) {
				luxrays::Spectrum rgb(
						(*constRGBTex)["color.r"].FloatValue(),
						(*constRGBTex)["color.g"].FloatValue(),
						(*constRGBTex)["color.b"].FloatValue());

				const float gainFactor = power * efficacy /
					(area * M_PI * rgb.Y());
				if (gainFactor > 0.f && !isinf(gainFactor))
					rgb *= gain * gainFactor * M_PI;
				else
					rgb *= gain * area * M_PI * rgb.Y();

				slgScene->AddMaterials(
					"scene.materials.light." + matName +" = " +
						boost::lexical_cast<string>(rgb.r) + " " +
						boost::lexical_cast<string>(rgb.g) + " " +
						boost::lexical_cast<string>(rgb.b) + "\n"
					);
			} else if (blackBodyTexture) {
				luxrays::Spectrum rgb(1.f);

				const float gainFactor = power * efficacy;
				if (gainFactor > 0.f && !isinf(gainFactor))
					rgb *= gain * gainFactor * M_PI;
				else
					rgb *= gain * area * M_PI * blackBodyTexture->Y();

				slgScene->AddMaterials(
					"scene.materials.light." + matName +" = " +
						boost::lexical_cast<string>(rgb.r) + " " +
						boost::lexical_cast<string>(rgb.g) + " " +
						boost::lexical_cast<string>(rgb.b) + "\n"
					);
			} else {
				LOG(LUX_WARNING, LUX_UNIMPLEMENT) << "SLGrenderer supports only area lights with constant ConstantRGBColorTexture or BlackBodyTexture (i.e. not " <<
					ToClassName(tex) << "). Replacing an unsupported area light material with matte.";
				return "mat_default";
			}
		}

		return matName;
	} else
	//----------------------------------------------------------------------
	// Primitive is not supported, use the default material
	//----------------------------------------------------------------------
	{
		LOG(LUX_WARNING, LUX_UNIMPLEMENT) << "SLGrenderer doesn't support material conversion for primitive " << ToClassName(prim);
		return "mat_default";
	}

	LOG(LUX_DEBUG, LUX_NOERROR) << "Material type: " << ToClassName(mat);

	// Check if the material has already been defined
	if (slgScene->materialIndices.count(matName) < 1) {
		// I have to create a new material definition

		//------------------------------------------------------------------
		// Check if it is material Matte
		//------------------------------------------------------------------
		if (dynamic_cast<Matte *>(mat)) {
			// Define the material
			Matte *matte = dynamic_cast<Matte *>(mat);
			matName = matte->GetName();

			// Check the type of texture
			Texture<SWCSpectrum> *tex = matte->GetTexture();
			LOG(LUX_DEBUG, LUX_NOERROR) << "Texture type: " << ToClassName(tex);
			ConstantRGBColorTexture *rgbTex = dynamic_cast<ConstantRGBColorTexture *>(tex);

			if (rgbTex) {
				luxrays::Spectrum rgb(
						(*rgbTex)["color.r"].FloatValue(),
						(*rgbTex)["color.g"].FloatValue(),
						(*rgbTex)["color.b"].FloatValue());

				slgScene->AddMaterials(
					"scene.materials.matte." + matName +" = " +
						boost::lexical_cast<string>(rgb.r) + " " +
						boost::lexical_cast<string>(rgb.g) + " " +
						boost::lexical_cast<string>(rgb.b) + "\n"
					);
			} else {
				LOG(LUX_WARNING, LUX_UNIMPLEMENT) << "SLGrenderer supports only Matte material with ConstantRGBColorTexture (i.e. not " <<
					ToClassName(tex) << "). Replacing an unsupported material with matte.";
				return "mat_default";
			}
		} else
		//------------------------------------------------------------------
		// Check if it is material Mirror
		//------------------------------------------------------------------
		if(dynamic_cast<Mirror *>(mat)) {
			// Define the material
			Mirror *mirror = dynamic_cast<Mirror *>(mat);
			matName = mirror->GetName();

			// Check the type of texture
			Texture<SWCSpectrum> *tex = mirror->GetTexture();
			LOG(LUX_DEBUG, LUX_NOERROR) << "Texture type: " << ToClassName(tex);
			ConstantRGBColorTexture *rgbTex = dynamic_cast<ConstantRGBColorTexture *>(tex);

			if (rgbTex) {
				luxrays::Spectrum rgb(
						(*rgbTex)["color.r"].FloatValue(),
						(*rgbTex)["color.g"].FloatValue(),
						(*rgbTex)["color.b"].FloatValue());

				slgScene->AddMaterials(
					"scene.materials.mirror." + matName +" = " +
						boost::lexical_cast<string>(rgb.r) + " " +
						boost::lexical_cast<string>(rgb.g) + " " +
						boost::lexical_cast<string>(rgb.b) + " 1\n"
					);
			} else {
				LOG(LUX_WARNING, LUX_UNIMPLEMENT) << "SLGrenderer supports only Mirror material with ConstantRGBColorTexture (i.e. not " <<
					ToClassName(tex) << "). Ignoring unsupported texture.";
				slgScene->AddMaterials(
					"scene.materials.mirror." + matName +" = 1.0 1.0 1.0 1\n");
			}
		} else
		//------------------------------------------------------------------
		// Check if it is material Glass
		//------------------------------------------------------------------
		if(dynamic_cast<Glass *>(mat)) {
			// Define the material
			Glass *glass = dynamic_cast<Glass *>(mat);
			matName = glass->GetName();

			// Check the type of textures

			// Kr
			Texture<SWCSpectrum> *krTex = glass->GetKrTexture();
			LOG(LUX_DEBUG, LUX_NOERROR) << "Kr Texture type: " << ToClassName(krTex);
			ConstantRGBColorTexture *krRGBTex = dynamic_cast<ConstantRGBColorTexture *>(krTex);

			luxrays::Spectrum krRGB;
			if (krRGBTex) {
				krRGB.r = (*krRGBTex)["color.r"].FloatValue();
				krRGB.g = (*krRGBTex)["color.g"].FloatValue();
				krRGB.b = (*krRGBTex)["color.b"].FloatValue();
			} else {
				LOG(LUX_WARNING, LUX_UNIMPLEMENT) << "SLGrenderer supports only Glass material with ConstantRGBColorTexture (i.e. not " <<
					ToClassName(krRGBTex) << "). Ignoring unsupported texture.";
				krRGB.r = 1.f;
				krRGB.g = 1.f;
				krRGB.b = 1.f;
			}

			// Kt
			Texture<SWCSpectrum> *ktTex = glass->GetKtTexture();
			LOG(LUX_DEBUG, LUX_NOERROR) << "Kt Texture type: " << ToClassName(ktTex);
			ConstantRGBColorTexture *ktRGBTex = dynamic_cast<ConstantRGBColorTexture *>(ktTex);

			luxrays::Spectrum ktRGB;
			if (ktRGBTex) {
				ktRGB.r = (*ktRGBTex)["color.r"].FloatValue();
				ktRGB.g = (*ktRGBTex)["color.g"].FloatValue();
				ktRGB.b = (*ktRGBTex)["color.b"].FloatValue();
			} else {
				LOG(LUX_WARNING, LUX_UNIMPLEMENT) << "SLGrenderer supports only Glass material with ConstantRGBColorTexture (i.e. not " <<
					ToClassName(ktRGBTex) << "). Ignoring unsupported texture.";
				ktRGB.r = 1.f;
				ktRGB.g = 1.f;
				ktRGB.b = 1.f;
			}

			// Index
			Texture<float> *indexTex = glass->GetIndexTexture();
			LOG(LUX_DEBUG, LUX_NOERROR) << "Index Texture type: " << ToClassName(indexTex);
			ConstantFloatTexture *indexFloatTex = dynamic_cast<ConstantFloatTexture *>(indexTex);

			float index;
			if (indexFloatTex)
				index = (*indexFloatTex)["value"].FloatValue();
			else {
				LOG(LUX_WARNING, LUX_UNIMPLEMENT) << "SLGrenderer supports only Glass material with ConstantFloatTexture (i.e. not " <<
					ToClassName(indexFloatTex) << "). Ignoring unsupported texture and using 1.41 value.";
				index = 1.41f;
			}

			// Check if it is architectural glass
			const bool architectural = (*glass)["architectural"].BoolValue();
			LOG(LUX_DEBUG, LUX_NOERROR) << "Architectural glass: " << architectural;
			if (architectural) {
				slgScene->AddMaterials(
						"scene.materials.archglass." + matName +" = " +
							boost::lexical_cast<string>(krRGB.r) + " " +
							boost::lexical_cast<string>(krRGB.g) + " " +
							boost::lexical_cast<string>(krRGB.b) + " " +
							boost::lexical_cast<string>(ktRGB.r) + " " +
							boost::lexical_cast<string>(ktRGB.g) + " " +
							boost::lexical_cast<string>(ktRGB.b) + " " +
							" 1 1\n"
						);
			} else {
				slgScene->AddMaterials(
						"scene.materials.glass." + matName +" = " +
							boost::lexical_cast<string>(krRGB.r) + " " +
							boost::lexical_cast<string>(krRGB.g) + " " +
							boost::lexical_cast<string>(krRGB.b) + " " +
							boost::lexical_cast<string>(ktRGB.r) + " " +
							boost::lexical_cast<string>(ktRGB.g) + " " +
							boost::lexical_cast<string>(ktRGB.b) + " " +
							" 1.0 " + boost::lexical_cast<string>(index) + " 1 1\n"
						);
			}
		} else
		//------------------------------------------------------------------
		// Check if it is material Glossy2
		//------------------------------------------------------------------
		if (dynamic_cast<Glossy2 *>(mat)) {
			// Define the material
			Glossy2 *glossy2 = dynamic_cast<Glossy2 *>(mat);
			matName = glossy2->GetName();

			// Check the type of texture

			// Kd
			Texture<SWCSpectrum> *kdTex = glossy2->GetKdTexture();
			LOG(LUX_DEBUG, LUX_NOERROR) << "Kd Texture type: " << ToClassName(kdTex);
			ConstantRGBColorTexture *kdRGBTex = dynamic_cast<ConstantRGBColorTexture *>(kdTex);

			luxrays::Spectrum kdRGB;
			if (kdRGBTex) {
				kdRGB.r = (*kdRGBTex)["color.r"].FloatValue();
				kdRGB.g = (*kdRGBTex)["color.g"].FloatValue();
				kdRGB.b = (*kdRGBTex)["color.b"].FloatValue();
			} else {
				LOG(LUX_WARNING, LUX_UNIMPLEMENT) << "SLGrenderer supports only Glossy2 material with ConstantRGBColorTexture (i.e. not " <<
					ToClassName(kdRGBTex) << "). Ignoring unsupported texture.";
				kdRGB.r = 1.f;
				kdRGB.g = 1.f;
				kdRGB.b = 1.f;
			}

			// Ks
			Texture<SWCSpectrum> *ksTex = glossy2->GetKsTexture();
			LOG(LUX_DEBUG, LUX_NOERROR) << "Ks Texture type: " << ToClassName(ksTex);
			ConstantRGBColorTexture *ksRGBTex = dynamic_cast<ConstantRGBColorTexture *>(ksTex);

			luxrays::Spectrum ksRGB;
			if (ksRGBTex) {
				ksRGB.r = (*ksRGBTex)["color.r"].FloatValue();
				ksRGB.g = (*ksRGBTex)["color.g"].FloatValue();
				ksRGB.b = (*ksRGBTex)["color.b"].FloatValue();
			} else {
				LOG(LUX_WARNING, LUX_UNIMPLEMENT) << "SLGrenderer supports only Glossy2 material with ConstantRGBColorTexture (i.e. not " <<
					ToClassName(ksRGBTex) << "). Ignoring unsupported texture.";
				ksRGB.r = 1.f;
				ksRGB.g = 1.f;
				ksRGB.b = 1.f;
			}

			// Try to guess the exponent from the roughness of the surface in the u direction
			Texture<float> *uroughnessTex = glossy2->GetNuTexture();
			LOG(LUX_DEBUG, LUX_NOERROR) << "Nu Texture type: " << ToClassName(uroughnessTex);
			ConstantFloatTexture *uroughnessFloatTex = dynamic_cast<ConstantFloatTexture *>(uroughnessTex);

			float uroughness;
			if (uroughnessFloatTex)
				uroughness = Clamp((*uroughnessFloatTex)["value"].FloatValue(), 6e-3f, 1.f);
			else {
				LOG(LUX_WARNING, LUX_UNIMPLEMENT) << "SLGrenderer supports only Glossy2 material with ConstantFloatTexture (i.e. not " <<
					ToClassName(uroughnessFloatTex) << "). Ignoring unsupported texture and using 0.1 value.";
				uroughness = .1f;
			}
			const float exponent = 10.f / uroughness;

			slgScene->AddMaterials(
					"scene.materials.mattemetal." + matName +" = " +
						boost::lexical_cast<string>(kdRGB.r) + " " +
						boost::lexical_cast<string>(kdRGB.g) + " " +
						boost::lexical_cast<string>(kdRGB.b) + " " +
						boost::lexical_cast<string>(ksRGB.r) + " " +
						boost::lexical_cast<string>(ksRGB.g) + " " +
						boost::lexical_cast<string>(ksRGB.b) + " " +
						boost::lexical_cast<string>(exponent) + " 1\n"
					);
		} else
		//------------------------------------------------------------------
		// Material is not supported, use the default one
		//------------------------------------------------------------------
		{
			LOG(LUX_WARNING, LUX_UNIMPLEMENT) << "SLGrenderer supports only Matte, Mirror, Glass and Glossy2 material (i.e. not " <<
				ToClassName(mat) << "). Replacing an unsupported material with matte.";
			return "mat_default";
		}
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

		slgScene->AddSunLight(
			"scene.sunlight.dir = " +
				boost::lexical_cast<string>(dirX) + " " +
				boost::lexical_cast<string>(dirY) + " " +
				boost::lexical_cast<string>(dirZ) + "\n"
			"scene.sunlight.turbidity = " + boost::lexical_cast<string>(turbidity) + "\n"
			"scene.sunlight.relsize = " + boost::lexical_cast<string>(relSize) + "\n"
			"scene.sunlight.gain = " +
				boost::lexical_cast<string>(gain) + " " +
				boost::lexical_cast<string>(gain) + " " +
				boost::lexical_cast<string>(gain) + "\n"
			);
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
			LOG(LUX_WARNING, LUX_UNIMPLEMENT) << "SLGrenderer doesn't support Sky2 light. It will use Sky instead.";

			const float dirX = (*sky2Light)["dir.x"].FloatValue();
			const float dirY = (*sky2Light)["dir.y"].FloatValue();
			const float dirZ = (*sky2Light)["dir.z"].FloatValue();
			const float turbidity = (*sky2Light)["turbidity"].FloatValue();
			// Note: (1000000000.0f / (M_PI * 100.f * 100.f)) is in SLG code
			// for compatibility with past scene
			const float gain = (*sky2Light)["gain"].FloatValue() * gainAdjustFactor;

			slgScene->AddSkyLight(
				"scene.skylight.dir = " +
					boost::lexical_cast<string>(dirX) + " " +
					boost::lexical_cast<string>(dirY) + " " +
					boost::lexical_cast<string>(dirZ) + "\n"
				"scene.skylight.turbidity = " + boost::lexical_cast<string>(turbidity) + "\n"
				"scene.skylight.gain = " +
					boost::lexical_cast<string>(gain) + " " +
					boost::lexical_cast<string>(gain) + " " +
					boost::lexical_cast<string>(gain) + "\n");
		} else {
			const float dirX = (*skyLight)["dir.x"].FloatValue();
			const float dirY = (*skyLight)["dir.y"].FloatValue();
			const float dirZ = (*skyLight)["dir.z"].FloatValue();
			const float turbidity = (*skyLight)["turbidity"].FloatValue();
			// Note: (1000000000.0f / (M_PI * 100.f * 100.f)) is in SLG code
			// for compatibility with past scene
			const float gain = (*skyLight)["gain"].FloatValue() * gainAdjustFactor;

			slgScene->AddSkyLight(
				"scene.skylight.dir = " +
					boost::lexical_cast<string>(dirX) + " " +
					boost::lexical_cast<string>(dirY) + " " +
					boost::lexical_cast<string>(dirZ) + "\n"
				"scene.skylight.turbidity = " + boost::lexical_cast<string>(turbidity) + "\n"
				"scene.skylight.gain = " +
					boost::lexical_cast<string>(gain) + " " +
					boost::lexical_cast<string>(gain) + " " +
					boost::lexical_cast<string>(gain) + "\n");
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
			LOG(LUX_WARNING, LUX_UNIMPLEMENT) << "SLGrenderer supports only one single environmental light. Using sky light and ignoring infinite lights";
		else {
			const float gainAdjustFactor = INV_PI;

			if (infiniteAreaLight) {
				const float colorR = (*infiniteAreaLight)["color.r"].FloatValue();
				const float colorG = (*infiniteAreaLight)["color.g"].FloatValue();
				const float colorB = (*infiniteAreaLight)["color.b"].FloatValue();
				const float gain = (*infiniteAreaLight)["gain"].FloatValue() * gainAdjustFactor;

				const float gamma = (*infiniteAreaLight)["gamma"].FloatValue();

				MIPMap *mipMap = infiniteAreaLight->GetRadianceMap();
				const string texName = GetSLGTexName(slgScene, mipMap, gamma);

				slgScene->AddInfiniteLight(
					"scene.infinitelight.file = " + texName + "\n"
					"scene.infinitelight.gamma = " + boost::lexical_cast<string>(gamma) + "\n"
					"scene.infinitelight.gain = " +
						boost::lexical_cast<string>(gain * colorR) + " " +
						boost::lexical_cast<string>(gain * colorG) + " " +
						boost::lexical_cast<string>(gain * colorB) + "\n");
			} else {
				const float colorR = (*infiniteAreaLightIS)["color.r"].FloatValue();
				const float colorG = (*infiniteAreaLightIS)["color.g"].FloatValue();
				const float colorB = (*infiniteAreaLightIS)["color.b"].FloatValue();
				const float gain = (*infiniteAreaLightIS)["gain"].FloatValue() * gainAdjustFactor;

				const float gamma = (*infiniteAreaLightIS)["gamma"].FloatValue();

				MIPMap *mipMap = infiniteAreaLightIS->GetRadianceMap();
				const string texName = GetSLGTexName(slgScene, mipMap, gamma);

				slgScene->AddInfiniteLight(
					"scene.infinitelight.file = " + texName + "\n"
					"scene.infinitelight.gamma = " + boost::lexical_cast<string>(gamma) + "\n"
					"scene.infinitelight.gain = " +
						boost::lexical_cast<string>(gain * colorR) + " " +
						boost::lexical_cast<string>(gain * colorG) + " " +
						boost::lexical_cast<string>(gain * colorB) + "\n");
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

		const string meshName = "Mesh-" + boost::lexical_cast<string>(*mesh);
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
				std::stringstream ss;
				for (int j = 0; j < 4; ++j)
					for (int i = 0; i < 4; ++i)
						ss << trans.m.m[i][j] << " ";
				string transString = ss.str();
	
				// Add the object
				for (vector<luxrays::ExtTriangleMesh *>::const_iterator mesh = meshList.begin(); mesh != meshList.end(); ++mesh) {
					// Define an instance of the mesh
					const string objName = "InstanceObject-" + boost::lexical_cast<string>(prim) + "-" +
						boost::lexical_cast<string>(*mesh);
					const string meshName = "Mesh-" + boost::lexical_cast<string>(*mesh);

					slgScene->AddObject(objName, matName, meshName,
							"scene.objects." + matName + "." + objName + ".transformation = " + transString + "\n"
							"scene.objects." + matName + "." + objName + ".useplynormals = 1\n"
							);
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
				const string objName = "Object-" + boost::lexical_cast<string>(prim) + "-" +
					boost::lexical_cast<string>(*mesh);
				const string meshName = "Mesh-" + boost::lexical_cast<string>(*mesh);
				slgScene->AddObject(objName, matName, meshName,
						"scene.objects." + matName + "." + objName + ".useplynormals = 1\n"
						);
			}
		}
	}

	if (slgScene->objects.size() == 0)
		throw std::runtime_error("SLGRenderer can not render an empty scene");
}

luxrays::sdl::Scene *SLGRenderer::CreateSLGScene(const luxrays::Properties &slgConfigProps) {
	const int accelType = slgConfigProps.GetInt("accelerator.type", -1);
	luxrays::sdl::Scene *slgScene = new luxrays::sdl::Scene(accelType);

	// Tell to the cache to not delete mesh data (they are pointed by Lux
	// primitives too and they will be deleted by Lux Context)
	slgScene->extMeshCache->SetDeleteMeshData(false);

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

	slgScene->CreateCamera(
		"scene.camera.lookat = " + 
			boost::lexical_cast<string>(orig.x) + " " +
			boost::lexical_cast<string>(orig.y) + " " +
			boost::lexical_cast<string>(orig.z) + " " +
			boost::lexical_cast<string>(target.x) + " " +
			boost::lexical_cast<string>(target.y) + " " +
			boost::lexical_cast<string>(target.z) + "\n"
		"scene.camera.up = " +
			boost::lexical_cast<string>(up.x) + " " +
			boost::lexical_cast<string>(up.y) + " " +
			boost::lexical_cast<string>(up.z) + "\n"
		"scene.camera.fieldofview = " + boost::lexical_cast<string>(Degrees((scene->camera)["fov"].FloatValue())) + "\n"
		"scene.camera.lensradius = " + boost::lexical_cast<string>((scene->camera)["LensRadius"].FloatValue()) + "\n"
		"scene.camera.focaldistance = " + boost::lexical_cast<string>((scene->camera)["FocalDistance"].FloatValue()) + "\n"
		);

	//--------------------------------------------------------------------------
	// Setup default material
	//--------------------------------------------------------------------------

	slgScene->AddMaterials(
		"scene.materials.matte.mat_default = 0.75 0.75 0.75\n"
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
	std::stringstream ss;

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
			"path.maxdepth = " << maxEyeDepth << "\n";
	} else {
		// Unmapped surface integrator, just use path tracing
		throw std::runtime_error("SLGRenderer doesn't support the SurfaceIntegrator, falling back to path tracing");
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

	ss << "image.width = " + boost::lexical_cast<string>(imageWidth) + "\n"
			"image.height = " + boost::lexical_cast<string>(imageHeight) + "\n";

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
				"sampler.maxconsecutivereject = " + boost::lexical_cast<string>(maxRejects) + "\n"
				"sampler.largesteprate = " + boost::lexical_cast<string>(pLarge) + "\n"
				"sampler.imagemutationrate = " + boost::lexical_cast<string>(range) + "\n";
	} else if (dynamic_cast<RandomSampler *>(scene->sampler)) {
		ss << "sampler.type = INLINED_RANDOM\n";
	} else {
		// Unmapped sampler, just use random
		throw std::runtime_error("SLGRenderer doesn't support the Sampler, falling back to random sampler");
		ss << "sampler.type = INLINED_RANDOM\n";
	}

	//--------------------------------------------------------------------------

	luxrays::Properties config;
	config.LoadFromString(ss.str());

	// Add overwrite properties
	config.Load(overwriteConfig);

	return config;
}

void SLGRenderer::Render(Scene *s) {
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
		} /*catch (std::exception err) {
			LOG(LUX_SEVERE, LUX_SYSTEM) << "ERROR: " << err.what();
			state = TERMINATE;
			return;
		}*/

		// start the timer
		rendererStatistics->start();

		// Dade - preprocessing done
		preprocessDone = true;
		scene->SetReady();
	}

	//----------------------------------------------------------------------
	// Do the render
	//----------------------------------------------------------------------

	try {
		slg::RenderConfig *config = new slg::RenderConfig(slgConfigProps, *slgScene);
		slg::RenderSession *session = new slg::RenderSession(config);
		slg::RenderEngine *engine = session->renderEngine;

		unsigned int haltTime = config->cfg.GetInt("batch.halttime", 0);
		unsigned int haltSpp = config->cfg.GetInt("batch.haltspp", 0);
		float haltThreshold = config->cfg.GetFloat("batch.haltthreshold", -1.f);

		// Start the rendering
		session->Start();
		const double startTime = luxrays::WallClockTime();

		double lastFilmUpdate = startTime;
		char buf[512];
		Film *film = scene->camera()->film;
		int xStart, xEnd, yStart, yEnd;
		film->GetSampleExtent(&xStart, &xEnd, &yStart, &yEnd);
		const luxrays::utils::Film *slgFilm = session->film; 
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
				// TODO: use Film write mutex
				ColorSystem colorSpace = film->GetColorSpace();
				for (int y = yStart; y <= yEnd; ++y) {
					for (int x = xStart; x <= xEnd; ++x) {
						const luxrays::utils::SamplePixel *sp = slgFilm->GetSamplePixel(
							luxrays::utils::PER_PIXEL_NORMALIZED, x - xStart, y - yStart);
						const float alpha = slgFilm->IsAlphaChannelEnabled() ?
							(slgFilm->GetAlphaPixel(x - xStart, y - yStart)->alpha) : 0.f;

						XYZColor xyz = colorSpace.ToXYZ(RGBColor(sp->radiance.r, sp->radiance.g, sp->radiance.b));
						// Flip the image upside down
						Contribution contrib(x, yEnd - 1 - y, xyz, alpha, 0.f, sp->weight);
						film->SetSample(&contrib);
					}
				}
				film->SetSampleCount(session->renderEngine->GetTotalSampleCount());

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

			// Print some information about the rendering progress
			sprintf(buf, "[Elapsed time: %3d/%dsec][Samples %4d/%d][Convergence %f%%][Avg. samples/sec % 3.2fM on %.1fK tris]",
					int(elapsedTime), int(haltTime), pass, haltSpp, 100.f * convergence, engine->GetTotalSamplesSec() / 1000000.0,
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
	const string configFile = params.FindOneString("configfile", "");
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
