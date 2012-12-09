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
#include "slgrenderer.h"
#include "context.h"
#include "light.h"
#include "lights/sun.h"
#include "lights/sky2.h"
#include "lights/infinite.h"
#include "lights/infinitesample.h"
#include "renderers/statistics/slgstatistics.h"
#include "cameras/perspective.h"
#include "textures/constant.h"
#include "materials/matte.h"
#include "shape.h"

#include "luxrays/core/context.h"
#include "luxrays/utils/core/exttrianglemesh.h"
#include "luxrays/opencl/utils.h"
#include "rendersession.h"
#include "textures/blackbody.h"
#include "lights/sky.h"
#include "materials/mirror.h"
#include "integrators/path.h"
#include "integrators/bidirectional.h"


using namespace lux;

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
	
	if (dynamic_cast<const MIPMapFastImpl<TextureColor<float, 4> > *>(mipMap))
		return GetSLGTexName(slgScene, (MIPMapImpl<TextureColor<float, 4> > *)mipMap, gamma);
	else {
		// Unsupported type
		DefineSLGDefaultTexMap(slgScene);
		return "tex_default";
	}
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

string SLGRenderer::GetSLGMaterialName(luxrays::sdl::Scene *slgScene, Primitive *prim) {
	LOG(LUX_DEBUG, LUX_NOERROR) << "Primitive type: " << typeid(*prim).name();

	string matName;

	// Check if it is an AreaLightPrimitive
	AreaLightPrimitive *alPrim = dynamic_cast<AreaLightPrimitive *>(prim);

	if (alPrim) {
		AreaLight *al = alPrim->GetAreaLight();
		matName = al->GetName();

		//----------------------------------------------------------------------
		// Check if I haven't already defined this AreaLight
		//----------------------------------------------------------------------

		if (slgScene->materialIndices.count(matName) < 1) {
			// Define a new area light material

			Texture<SWCSpectrum> *tex = al->GetTexture();

			const float gain = (*al)["gain"].FloatValue();
			const float power = (*al)["power"].FloatValue();
			const float efficacy = (*al)["efficacy"].FloatValue();
			const float area = (*al)["area"].FloatValue();

			// Check the type of texture used
			LOG(LUX_DEBUG, LUX_NOERROR) << "AreaLight texture type: " << typeid(*tex).name();
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
					typeid(*tex).name() << "). Replacing an unsupported area light material with matte.";
				matName = "mat_default";
			}
		}
	} else {
		// Check if it is a Shape
		Shape *shape = dynamic_cast<Shape *>(prim);
		if (shape) {
			// Get the material a try a conversion
			Material *mat = shape->GetMaterial();
			LOG(LUX_DEBUG, LUX_NOERROR) << "Material type: " << typeid(*mat).name();

			matName = mat->GetName();
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
					LOG(LUX_DEBUG, LUX_NOERROR) << "Texture type: " << typeid(*tex).name();
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
							typeid(*tex).name() << "). Replacing an unsupported material with matte.";
						matName = "mat_default";
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
					LOG(LUX_DEBUG, LUX_NOERROR) << "Texture type: " << typeid(*tex).name();
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
							typeid(*tex).name() << "). Replacing an unsupported material with matte.";
						matName = "mat_default";
					}
				} else
				//------------------------------------------------------------------
				// Material is not supported, use the default one
				//------------------------------------------------------------------
				{
					LOG(LUX_WARNING, LUX_UNIMPLEMENT) << "SLGrenderer supports only Matte material (i.e. not " <<
						typeid(*mat).name() << "). Replacing an unsupported material with matte.";
					matName = "mat_default";
				}
			}
		} else {
			LOG(LUX_WARNING, LUX_UNIMPLEMENT) << "SLGrenderer doesn't support material conversion for " << typeid(*prim).name();
			matName = "mat_default";
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

luxrays::sdl::Scene *SLGRenderer::CreateSLGScene() {
	luxrays::sdl::Scene *slgScene = new luxrays::sdl::Scene();

	// Tell to the cache to not delete mesh data (they are pointed by Lux
	// primitives too and they will be deleted by Lux Context)
	slgScene->extMeshCache->SetDeleteMeshData(false);

	LOG(LUX_DEBUG, LUX_NOERROR) << "Camera type: " << typeid(*(scene->camera)).name();
	PerspectiveCamera *perpCamera = dynamic_cast<PerspectiveCamera *>(scene->camera);
	if (!perpCamera)
		throw std::runtime_error("SLGRenderer supports only PerspectiveCamera");

	//--------------------------------------------------------------------------
	// Setup the camera
	//--------------------------------------------------------------------------

	const Point orig(
			(*perpCamera)["Position.x"].FloatValue(),
			(*perpCamera)["Position.y"].FloatValue(),
			(*perpCamera)["Position.z"].FloatValue());
	const Point target= orig + Vector(
			(*perpCamera)["Normal.x"].FloatValue(),
			(*perpCamera)["Normal.y"].FloatValue(),
			(*perpCamera)["Normal.z"].FloatValue());
	const Vector up(
			(*perpCamera)["Up.x"].FloatValue(),
			(*perpCamera)["Up.y"].FloatValue(),
			(*perpCamera)["Up.z"].FloatValue());

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
		"scene.camera.fieldofview = " + boost::lexical_cast<string>(Degrees((*perpCamera)["fov"].FloatValue())) + "\n"
		"scene.camera.lensradius = " + boost::lexical_cast<string>((*perpCamera)["LensRadius"].FloatValue()) + "\n"
		"scene.camera.focaldistance = " + boost::lexical_cast<string>((*perpCamera)["FocalDistance"].FloatValue()) + "\n"
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

	LOG(LUX_INFO, LUX_NOERROR) << "Tesselating " << scene->primitives.size() << " primitives";

	u_int objNumber = 0;
	for (size_t i = 0; i < scene->primitives.size(); ++i) {
		vector<luxrays::ExtTriangleMesh *> meshList;
		scene->primitives[i]->ExtTesselate(&meshList, &scene->tesselatedPrimitives);

		const string matName = GetSLGMaterialName(slgScene, scene->primitives[i].get());

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

			const string objName = "Object" + boost::lexical_cast<string>(objNumber);
			slgScene->DefineObject(objName, *mesh);
			slgScene->AddObject(objName, matName,
					"scene.objects." + matName + "." + objName + ".useplynormals = 1\n"
					);
			++objNumber;
		}
	}

	if (slgScene->objects.size() == 0)
		throw std::runtime_error("SLGRenderer can not render an empty scene");

	return slgScene;
}

luxrays::Properties SLGRenderer::CreateSLGConfig() {
	std::stringstream ss;

	ss << "renderengine.type = PATHOCL\n"
			"sampler.type = INLINED_RANDOM\n"
			"opencl.platform.index = -1\n"
			"opencl.cpu.use = 1\n"
			"opencl.gpu.use = 1\n"
			;

	//--------------------------------------------------------------------------
	// Surface integrator related settings
	//--------------------------------------------------------------------------

	LOG(LUX_DEBUG, LUX_NOERROR) << "Surface integrator type: " << typeid(*(scene->surfaceIntegrator)).name();
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

	Film *film = scene->camera->film;
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
		scene->sampler->SetFilm(scene->camera->film);
		scene->surfaceIntegrator->Preprocess(rng, *scene);
		scene->volumeIntegrator->Preprocess(rng, *scene);
		scene->camera->film->CreateBuffers();

		scene->surfaceIntegrator->RequestSamples(scene->sampler, *scene);
		scene->volumeIntegrator->RequestSamples(scene->sampler, *scene);

		// Dade - to support autofocus for some camera model
		scene->camera->AutoFocus(*scene);

		// TODO: extend SLG library to accept an handler for each rendering session
		luxrays::sdl::LuxRaysSDLDebugHandler = SDLDebugHandler;

		try {
			// Build the SLG scene to render
			slgScene = CreateSLGScene();

			// Build the SLG rendering configuration
			slgConfigProps.Load(CreateSLGConfig());
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
		Film *film = scene->camera->film;
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
	scene->camera->film->contribPool->Flush();
	scene->camera->film->contribPool->Delete();
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
