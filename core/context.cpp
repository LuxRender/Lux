/***************************************************************************
 *   Copyright (C) 1998-2008 by authors (see AUTHORS.txt )                 *
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

#include "renderfarm.h"


#include "lux.h"
#include "context.h"
#include "dynload.h"
#include "api.h"
#include "camera.h"
#include "light.h"
#include "primitive.h"
#include "scene.h"
#include "volume.h"
#include "material.h"
#include "stats.h"

#include <boost/iostreams/filtering_stream.hpp>
#include <boost/iostreams/filtering_streambuf.hpp>
#include <boost/iostreams/copy.hpp>
#include <boost/iostreams/filter/zlib.hpp>

using namespace boost::iostreams;
using namespace lux;

Context *Context::activeContext;

// API Macros
#define VERIFY_INITIALIZED(func) \
if (currentApiState == STATE_UNINITIALIZED) { \
		std::stringstream ss; \
		ss<<"luxInit() must be called before calling  '"<<func<<"'. Ignoring."; \
		luxError(LUX_NOTSTARTED,LUX_SEVERE,ss.str().c_str()); \
	return; \
} else /* swallow trailing semicolon */
#define VERIFY_OPTIONS(func) \
VERIFY_INITIALIZED(func); \
if (currentApiState == STATE_WORLD_BLOCK) { \
		std::stringstream ss;  \
		ss<<"Options cannot be set inside world block; '"<<func<<"' not allowed.  Ignoring."; \
		luxError(LUX_NESTING,LUX_ERROR,ss.str().c_str()); \
	return; \
} else /* swallow trailing semicolon */
#define VERIFY_WORLD(func) \
VERIFY_INITIALIZED(func); \
if (currentApiState == STATE_OPTIONS_BLOCK) { \
	std::stringstream ss;  \
	ss<<"Scene description must be inside world block; '"<<func<<"' not allowed.  Ignoring."; \
	luxError(LUX_NESTING,LUX_ERROR,ss.str().c_str()); \
	return; \
} else /* swallow trailing semicolon */


void Context::init() {
    // Dade - reinitialize
    currentApiState = STATE_OPTIONS_BLOCK;
    luxCurrentScene = NULL;
    curTransform = Transform();
    namedCoordinateSystems.clear();
    renderOptions = new RenderOptions;
    graphicsState = new GraphicsState;
    namedmaterials.clear();
    pushedGraphicsStates.clear();
    pushedTransforms.clear();
    renderFarm = new RenderFarm();
}

void Context::free() {
    // Dade - free memory
    if (luxCurrentScene) {
        delete luxCurrentScene;
        luxCurrentScene = NULL;
    }

    if (renderOptions) {
        delete renderOptions;
        renderOptions = NULL;
    }

    if (graphicsState) {
        delete graphicsState;
        graphicsState = NULL;
    }

    if (renderFarm) {
        delete renderFarm;
        renderFarm = NULL;
    }
}

// API Function Definitions

void Context::addServer(const string &name) {
	//luxServerList.push_back(std::string(name));
	renderFarm->connect(name);
}

int Context::getRenderingServersStatus(RenderingServerInfo *info, int maxInfoCount) {
	if (!renderFarm)
		return 0;

	return renderFarm->getServersStatus(info, maxInfoCount);
}

void Context::cleanup() {
	renderFarm->send("luxCleanup");

	StatsCleanup();
	// API Cleanup
	if (currentApiState == STATE_UNINITIALIZED)
		luxError(LUX_NOTSTARTED,LUX_ERROR,"luxCleanup() called without luxInit().");
	else if (currentApiState == STATE_WORLD_BLOCK)
		luxError(LUX_ILLSTATE,LUX_ERROR,"luxCleanup() called while inside world block.");

        // Dade - free memory
        free();

        // Dade - reinitialize
        init();
}

void Context::identity() {
	VERIFY_INITIALIZED("Identity");
	renderFarm->send("luxIdentity");
	curTransform = Transform();
}

void Context::translate(float dx, float dy, float dz) {
	VERIFY_INITIALIZED("Translate");
	renderFarm->send("luxTranslate", dx, dy, dz);
	curTransform = curTransform * Translate(Vector(dx, dy, dz));
}

void Context::transform(float tr[16]) {
	VERIFY_INITIALIZED("Transform");
	renderFarm->send("luxTransform", tr);
	boost::shared_ptr<Matrix4x4> o(new Matrix4x4(
			tr[0], tr[4], tr[8], tr[12],
			tr[1], tr[5], tr[9], tr[13],
			tr[2], tr[6], tr[10], tr[14],
			tr[3], tr[7], tr[11], tr[15]));
	curTransform = Transform(o);
}
void Context::concatTransform(float tr[16]) {
	VERIFY_INITIALIZED("ConcatTransform");
	renderFarm->send("luxConcatTransform", tr);
	boost::shared_ptr<Matrix4x4> o(new Matrix4x4(tr[0], tr[4], tr[8], tr[12],
			tr[1], tr[5], tr[9], tr[13],
			tr[2], tr[6], tr[10], tr[14],
			tr[3], tr[7], tr[11], tr[15]));
	curTransform = curTransform * Transform(o);
}
void Context::rotate(float angle, float dx, float dy, float dz) {
	VERIFY_INITIALIZED("Rotate");
	renderFarm->send("luxRotate", angle, dx, dy, dz);
	curTransform = curTransform * Rotate(angle, Vector(dx, dy, dz));
}
void Context::scale(float sx, float sy, float sz) {
	VERIFY_INITIALIZED("Scale");
	renderFarm->send("luxScale", sx, sy, sz);
	curTransform = curTransform * Scale(sx, sy, sz);
}
void Context::lookAt(float ex, float ey, float ez, float lx, float ly, float lz,
		float ux, float uy, float uz) {
	VERIFY_INITIALIZED("LookAt");
	renderFarm->send("luxLookAt", ex, ey, ez, lx, ly, lz, ux, uy, uz);

	curTransform = curTransform * LookAt(Point(ex, ey, ez), Point(lx, ly, lz),
			Vector(ux, uy, uz));
}
void Context::coordinateSystem(const string &name) {
	VERIFY_INITIALIZED("CoordinateSystem");
	renderFarm->send("luxCoordinateSystem", name);
	namedCoordinateSystems[name] = curTransform;
}
void Context::coordSysTransform(const string &name) {
	VERIFY_INITIALIZED("CoordSysTransform");
	renderFarm->send("luxCoordSysTransform", name);
	if (namedCoordinateSystems.find(name) != namedCoordinateSystems.end())
		curTransform = namedCoordinateSystems[name];
}
void Context::enableDebugMode() {
    VERIFY_OPTIONS("EnableDebugMode")
	;
    // Dade - I avoid to transmit the EnableDebugMode option to the renderFarm
    renderOptions->debugMode = true;
}
void Context::pixelFilter(const string &name, const ParamSet &params) {
	VERIFY_OPTIONS("PixelFilter")
	;
	renderFarm->send("luxPixelFilter", name, params);
	renderOptions->FilterName = name;
	renderOptions->FilterParams = params;
}
void Context::film(const string &type, const ParamSet &params) {
	VERIFY_OPTIONS("Film")
	;
	renderFarm->send("luxFilm", type, params);
	renderOptions->FilmParams = params;
	renderOptions->FilmName = type;
}
void Context::sampler(const string &name, const ParamSet &params) {
	VERIFY_OPTIONS("Sampler")
	;
	renderFarm->send("luxSampler", name, params);
	renderOptions->SamplerName = name;
	renderOptions->SamplerParams = params;
}
void Context::accelerator(const string &name, const ParamSet &params) {
	VERIFY_OPTIONS("Accelerator")
	;
	renderFarm->send("luxAccelerator", name, params);
	renderOptions->AcceleratorName = name;
	renderOptions->AcceleratorParams = params;
}
void Context::surfaceIntegrator(const string &name, const ParamSet &params) {
	VERIFY_OPTIONS("SurfaceIntegrator")
	;
	renderFarm->send("luxSurfaceIntegrator", name, params);
	renderOptions->SurfIntegratorName = name;
	renderOptions->SurfIntegratorParams = params;
}
void Context::volumeIntegrator(const string &name, const ParamSet &params) {
	VERIFY_OPTIONS("VolumeIntegrator")
	;
	renderFarm->send("luxVolumeIntegrator", name, params);
	renderOptions->VolIntegratorName = name;
	renderOptions->VolIntegratorParams = params;
}
void Context::camera(const string &name, const ParamSet &params) {
	VERIFY_OPTIONS("Camera")
	;
	renderFarm->send("luxCamera", name, params);

	renderOptions->CameraName = name;
	renderOptions->CameraParams = params;
	renderOptions->WorldToCamera = curTransform;
	namedCoordinateSystems["camera"] = curTransform.GetInverse();
}
void Context::worldBegin() {
	VERIFY_OPTIONS("WorldBegin")
	;
	renderFarm->send("luxWorldBegin");
	currentApiState = STATE_WORLD_BLOCK;
	curTransform = Transform();
	namedCoordinateSystems["world"] = curTransform;
}
void Context::attributeBegin() {
	VERIFY_WORLD("AttributeBegin")
	;
	renderFarm->send("luxAttributeBegin");
	pushedGraphicsStates.push_back(*graphicsState);
	pushedTransforms.push_back(curTransform);
}
void Context::attributeEnd() {
	VERIFY_WORLD("AttributeEnd")
	;
	renderFarm->send("luxAttributeEnd");
	if (!pushedGraphicsStates.size()) {
		luxError(LUX_ILLSTATE,LUX_ERROR,"Unmatched luxAttributeEnd() encountered. Ignoring it.");
		return;
	}
	*graphicsState = pushedGraphicsStates.back();
	curTransform = pushedTransforms.back();
	pushedGraphicsStates.pop_back();
	pushedTransforms.pop_back();
}
void Context::transformBegin() {
	VERIFY_WORLD("TransformBegin")
	;
	renderFarm->send("luxTransformBegin");
	pushedTransforms.push_back(curTransform);
}
void Context::transformEnd() {
	VERIFY_WORLD("TransformEnd")
	;
	renderFarm->send("luxTransformEnd");
	if (!pushedTransforms.size()) {
		luxError(LUX_ILLSTATE,LUX_ERROR,"Unmatched luxTransformEnd() encountered. Ignoring it.");
		return;
	}
	curTransform = pushedTransforms.back();
	pushedTransforms.pop_back();
}
void Context::texture(const string &name, const string &type, const string &texname,
		const ParamSet &params) {
	VERIFY_WORLD("Texture")
	;
	renderFarm->send("luxTexture", name, type, texname, params);

	TextureParams tp(params, params, graphicsState->floatTextures,
			graphicsState->RGBColorTextures);
	if (type == "float") {
		// Create _float_ texture and store in _floatTextures_
		if (graphicsState->floatTextures.find(name)
				!= graphicsState->floatTextures.end()) {
			//Warning("Texture \"%s\" being redefined", name.c_str());
			std::stringstream ss;
			ss<<"Texture '"<<name<<"' being redefined.";
			luxError(LUX_SYNTAX,LUX_WARNING,ss.str().c_str());
		}
		boost::shared_ptr<Texture<float> > ft = MakeFloatTexture(texname,
				curTransform, tp);
		if (ft)
			graphicsState->floatTextures[name] = ft;
	} else if (type == "color") {
		// Create _color_ texture and store in _RGBColorTextures_
		if (graphicsState->RGBColorTextures.find(name)
				!= graphicsState->RGBColorTextures.end()) {
			//Warning("Texture \"%s\" being redefined", name.c_str());
			std::stringstream ss;
			ss<<"Texture '"<<name<<"' being redefined.";
			luxError(LUX_SYNTAX,LUX_WARNING,ss.str().c_str());
		}
		boost::shared_ptr<Texture<RGBColor> > st = MakeRGBColorTexture(texname,
				curTransform, tp);
		if (st)
			graphicsState->RGBColorTextures[name] = st;
	} else {
		//Error("Texture type \"%s\" unknown.", type.c_str());
		std::stringstream ss;
		ss<<"Texture type '"<<type<<"' unknown";
		luxError(LUX_SYNTAX,LUX_ERROR,ss.str().c_str());
	}

}
void Context::material(const string &name, const ParamSet &params) {
	VERIFY_WORLD("Material")
	;
	renderFarm->send("luxMaterial", name, params);
	graphicsState->material = name;
	graphicsState->materialParams = params;
}

void Context::makenamedmaterial(const string &name, const ParamSet &params) {
	VERIFY_WORLD("MakeNamedMaterial")
	;
    renderFarm->send("luxMakeNamedMaterial", name, params);
	NamedMaterial nm;
	nm.material = name;
	nm.materialParams = params;
	namedmaterials.push_back(nm);
}

void Context::namedmaterial(const string &name, const ParamSet &params) {
	VERIFY_WORLD("NamedMaterial")
	;
    renderFarm->send("luxNamedMaterial", name, params);
	bool found = false;
	for(unsigned int i=0; i<namedmaterials.size(); i++)
		if(namedmaterials[i].material == name) {
			string type = namedmaterials[i].materialParams.FindOneString("type", "matte");
			ParamSet nparams = namedmaterials[i].materialParams;
			nparams.EraseString("type");
			material(type, nparams);
			found = true;
		}

	if(!found) {
		std::stringstream ss;
		ss<<"NamedMaterial named '"<<name<<"' unknown";
		luxError(LUX_SYNTAX,LUX_ERROR,ss.str().c_str());
	}
}

void Context::lightSource(const string &name, const ParamSet &params) {
	VERIFY_WORLD("LightSource")
	;
	renderFarm->send("luxLightSource", name, params);

	if (name == "sunsky") {
		//SunSky light - create both sun & sky lightsources
		Light *lt_sun = MakeLight("sun", curTransform, params);
		if (lt_sun == NULL) {
			luxError(LUX_SYNTAX,LUX_ERROR,"luxLightSource: light type sun unknown.");
			graphicsState->currentLightPtr0 = NULL;
		} else {
			renderOptions->lights.push_back(lt_sun);
			graphicsState->currentLight = name;
			graphicsState->currentLightPtr0 = lt_sun;
		}
		Light *lt_sky = MakeLight("sky", curTransform, params);
		if (lt_sky == NULL) {
			luxError(LUX_SYNTAX,LUX_ERROR,"luxLightSource: light type sky unknown.");
			graphicsState->currentLightPtr1 = NULL;
		} else {
			renderOptions->lights.push_back(lt_sky);
			graphicsState->currentLight = name;
			graphicsState->currentLightPtr1 = lt_sky;
		}
	} else {
		// other lightsource type
		Light *lt = MakeLight(name, curTransform, params);
		if (lt == NULL) {
			//Error("luxLightSource: light type "
			//      "\"%s\" unknown.", name.c_str());
			std::stringstream ss;
			ss<<"luxLightSource: light type  '"<<name<<"' unknown";
			luxError(LUX_SYNTAX,LUX_ERROR,ss.str().c_str());
		} else {
			renderOptions->lights.push_back(lt);
			graphicsState->currentLight = name;
			graphicsState->currentLightPtr0 = lt;
			graphicsState->currentLightPtr1 = NULL;
		}
	}
}

void Context::areaLightSource(const string &name, const ParamSet &params) {
	VERIFY_WORLD("AreaLightSource")
	;
	renderFarm->send("luxAreaLightSource", name, params);

	graphicsState->areaLight = name;
	graphicsState->areaLightParams = params;
}

void Context::portalShape(const string &name, const ParamSet &params) {
	VERIFY_WORLD("PortalShape")
	;
	renderFarm->send("luxPortalShape", name, params);

	boost::shared_ptr<Shape> shape = MakeShape(name, curTransform,
			graphicsState->reverseOrientation, params, &graphicsState->floatTextures);
	if (!shape)
		return;
	params.ReportUnused();
	// Initialize area light for shape									// TODO - radiance - add portalshape to area light & cleanup
	AreaLight *area= NULL;
	//if (graphicsState->areaLight != "")
	//	area = MakeAreaLight(graphicsState->areaLight,
	//	curTransform, graphicsState->areaLightParams, shape);

	if (graphicsState->currentLight != "") {
		if (graphicsState->currentLight == "sunsky"
				|| graphicsState->currentLight == "infinite") {
			if (graphicsState->currentLightPtr0)
				graphicsState->currentLightPtr0->AddPortalShape(shape);

			if (graphicsState->currentLightPtr1)
				graphicsState->currentLightPtr1->AddPortalShape(shape);
		} else {
			//Warning("LightType '%s' does not support PortalShape(s).\n",  graphicsState->currentLight.c_str());
			std::stringstream ss;
			ss<<"LightType '"<<graphicsState->currentLight
					<<" does not support PortalShape(s).";
			luxError(LUX_UNIMPLEMENT,LUX_WARNING,ss.str().c_str());
			return;
		}
	}

	// Initialize material for shape (dummy)
	TextureParams mp(params, graphicsState->materialParams,
			graphicsState->floatTextures, graphicsState->RGBColorTextures);
	boost::shared_ptr<Texture<float> > bump;
	boost::shared_ptr<Material> mtl = MakeMaterial("matte", curTransform, mp);

	// Create primitive (for refining) (dummy)
	Primitive* prim(new GeometricPrimitive(shape, mtl, area));
}

void Context::makemixmaterial(const ParamSet shapeparams, const ParamSet materialparams, boost::shared_ptr<Material> mtl) {
	// create 1st material
	string namedmaterial1 = materialparams.FindOneString("namedmaterial1", "-");
	bool found = false;
	for(unsigned int i=0; i<namedmaterials.size(); i++)
		if(namedmaterials[i].material == namedmaterial1) {
			string type = namedmaterials[i].materialParams.FindOneString("type", "matte");
			ParamSet nparams = namedmaterials[i].materialParams;
			nparams.EraseString("type");
			TextureParams mp1(shapeparams, nparams,
			graphicsState->floatTextures, graphicsState->RGBColorTextures);
			boost::shared_ptr<Material> mtl1 = MakeMaterial(type, curTransform, mp1);

			if(type == "mix")
				makemixmaterial(shapeparams, nparams, mtl1);

			mtl->SetChild1(mtl1);
			found = true;
		}
	if(!found) {
		std::stringstream ss;
		ss<<"MixMaterial: NamedMaterial1 named '"<<namedmaterial1<<"' unknown";
		luxError(LUX_SYNTAX,LUX_ERROR,ss.str().c_str());
	}

	// create 2nd material
	string namedmaterial2 = materialparams.FindOneString("namedmaterial2", "-");
	found = false;
	for(unsigned int i=0; i<namedmaterials.size(); i++)
		if(namedmaterials[i].material == namedmaterial2) {
			string type = namedmaterials[i].materialParams.FindOneString("type", "matte");
			ParamSet nparams = namedmaterials[i].materialParams;
			nparams.EraseString("type");
			TextureParams mp1(shapeparams, nparams,
			graphicsState->floatTextures, graphicsState->RGBColorTextures);
			boost::shared_ptr<Material> mtl2 = MakeMaterial(type, curTransform, mp1);

			if(type == "mix")
				makemixmaterial(shapeparams, nparams, mtl2);

			mtl->SetChild2(mtl2);
			found = true;
		}
	if(!found) {
		std::stringstream ss;
		ss<<"MixMaterial: NamedMaterial2 named '"<<namedmaterial1<<"' unknown";
		luxError(LUX_SYNTAX,LUX_ERROR,ss.str().c_str());
	}
}

void Context::shape(const string &name, const ParamSet &params) {
	VERIFY_WORLD("Shape")
	;
	renderFarm->send("luxShape", name, params);

	boost::shared_ptr<Shape> shape = MakeShape(
			name,
			curTransform,
			graphicsState->reverseOrientation,
			params,
			&graphicsState->floatTextures);
	if (!shape)
		return;
	params.ReportUnused();
	// Initialize area light for shape
	AreaLight *area= NULL;
	if (graphicsState->areaLight != "")
		area = MakeAreaLight(graphicsState->areaLight, curTransform,
				graphicsState->areaLightParams, shape);
	// Initialize material for shape
	TextureParams mp(params, graphicsState->materialParams,
			graphicsState->floatTextures, graphicsState->RGBColorTextures);
	boost::shared_ptr<Texture<float> > bump;
	boost::shared_ptr<Material> mtl = MakeMaterial(graphicsState->material, curTransform, mp);
	if (!mtl)
		mtl = MakeMaterial("matte", curTransform, mp);
	if (!mtl)
		luxError(LUX_BUG,LUX_SEVERE,"Unable to create \"matte\" material?!");

	// Set child materials if mix material

	if(graphicsState->material == "mix") {
		makemixmaterial(params, graphicsState->materialParams, mtl);
	}

	// Create primitive and add to scene or current instance
	Primitive* prim(new GeometricPrimitive(shape, mtl, area));
	if (renderOptions->currentInstance) {
		if (area)
			luxError(LUX_UNIMPLEMENT,LUX_WARNING,"Area lights not supported with object instancing");
		renderOptions->currentInstance->push_back(prim);
	} else {
		renderOptions->primitives.push_back(prim);
		if (area != NULL) {
			// Add area light for primitive to light vector
			renderOptions->lights.push_back(area);
		}
	}
}
void Context::reverseOrientation() {
	VERIFY_WORLD("ReverseOrientation")
	;
	renderFarm->send("luxReverseOrientation");
	graphicsState->reverseOrientation = !graphicsState->reverseOrientation;
}
void Context::volume(const string &name, const ParamSet &params) {
	VERIFY_WORLD("Volume")
	;
	renderFarm->send("luxVolume", name, params);
	VolumeRegion *vr = MakeVolumeRegion(name, curTransform, params);
	if (vr)
		renderOptions->volumeRegions.push_back(vr);
}
void Context::objectBegin(const string &name) {
	VERIFY_WORLD("ObjectBegin")
	;
	renderFarm->send("luxObjectBegin", name);
	luxAttributeBegin();
	if (renderOptions->currentInstance)
		luxError(LUX_NESTING,LUX_ERROR,"ObjectBegin called inside of instance definition");
	renderOptions->instances[name] = vector<Primitive*>();
	renderOptions->currentInstance = &renderOptions->instances[name];
}
void Context::objectEnd() {
	VERIFY_WORLD("ObjectEnd")
	;
	renderFarm->send("luxObjectEnd");
	if (!renderOptions->currentInstance)
		luxError(LUX_NESTING,LUX_ERROR,"ObjectEnd called outside of instance definition");
	renderOptions->currentInstance = NULL;
	luxAttributeEnd();
}
void Context::objectInstance(const string &name) {
	VERIFY_WORLD("ObjectInstance")
	;
	renderFarm->send("luxObjectInstance", name);
	// Object instance error checking
	if (renderOptions->currentInstance) {
		luxError(LUX_NESTING,LUX_ERROR,"ObjectInstance can't be called inside instance definition");
		return;
	}
	if (renderOptions->instances.find(name) == renderOptions->instances.end()) {
		//Error("Unable to find instance named \"%s\"", name.c_str());
		std::stringstream ss;
		ss<<"Unable to find instance named '"<<name<<"'";
		luxError(LUX_BADTOKEN,LUX_ERROR,ss.str().c_str());
		return;
	}
	vector<Primitive* > &in = renderOptions->instances[name];
	if (in.size() == 0)
		return;
	if (in.size() > 1 || !in[0]->CanIntersect()) {
		// Refine instance _Primitive_s and create aggregate
		Primitive* accel = MakeAccelerator(renderOptions->AcceleratorName, in,
				renderOptions->AcceleratorParams);
		if (!accel)
			accel = MakeAccelerator("kdtree", in, ParamSet());
		if (!accel)
			luxError(LUX_BUG,LUX_SEVERE,"Unable to find \"kdtree\" accelerator");
		in.erase(in.begin(), in.end());
		in.push_back(accel);
	}
	Primitive* o(new InstancePrimitive(in[0], curTransform));
	Primitive* prim = o;
	renderOptions->primitives.push_back(prim);
}

void Context::worldEnd() {
    VERIFY_WORLD("WorldEnd")
            ;
    renderFarm->send("luxWorldEnd");
    renderFarm->flush();

    // Dade - get the lock, other thread can use this lock to wait the end of
    // the rendering
    boost::mutex::scoped_lock lock(renderingMutex);

    // Ensure the search path was set
    /*if (!renderOptions->gotSearchPath)
     Severe("LUX_SEARCHPATH environment variable "
     "wasn't set and a plug-in\n"
     "search path wasn't given in the "
     "input (with the SearchPath "
     "directive).\n");*/
    // Ensure there are no pushed graphics states
    while (pushedGraphicsStates.size()) {
        luxError(LUX_NESTING, LUX_WARNING, "Missing end to luxAttributeBegin()");
        pushedGraphicsStates.pop_back();
        pushedTransforms.pop_back();
    }
    // Create scene and render
    luxCurrentScene = renderOptions->MakeScene();
    if (luxCurrentScene) {
        // Dade - check if we have to start the network rendering updater thread
        if (renderFarm->getServerCount() > 0)
            renderFarm->startFilmUpdater(luxCurrentScene);

        luxCurrentScene->Render();

        // Dade - check if we have to stop the network rendering updater thread
        if (renderFarm->getServerCount() > 0)
            renderFarm->stopFilmUpdater();
    }

    //delete scene;
    // Clean up after rendering
    currentApiState = STATE_OPTIONS_BLOCK;
    StatsPrint(stdout);
    curTransform = Transform();
    namedCoordinateSystems.erase(namedCoordinateSystems.begin(),
            namedCoordinateSystems.end());
}

Scene *Context::RenderOptions::MakeScene() const {
	// Create scene objects from API settings
	Filter *filter = MakeFilter(FilterName, FilterParams);
	Film *film = MakeFilm(FilmName, FilmParams, filter);
	if (std::string(FilmName)=="film")
		luxError(LUX_NOERROR,LUX_WARNING,"Warning: Legacy PBRT 'film' does not provide tonemapped output or GUI film display. Use 'multifilm' instead.");
	Camera *camera = MakeCamera(CameraName, CameraParams, WorldToCamera, film);
	Sampler *sampler = MakeSampler(SamplerName, SamplerParams, film);
	SurfaceIntegrator *surfaceIntegrator = MakeSurfaceIntegrator(
			SurfIntegratorName, SurfIntegratorParams);
	VolumeIntegrator *volumeIntegrator = MakeVolumeIntegrator(
			VolIntegratorName, VolIntegratorParams);
	Primitive *accelerator = MakeAccelerator(AcceleratorName, primitives,
			AcceleratorParams);
	if (!accelerator) {
		ParamSet ps;
		accelerator = MakeAccelerator("kdtree", primitives, ps);
	}
	if (!accelerator)
		luxError(LUX_BUG,LUX_SEVERE,"Unable to find \"kdtree\" accelerator");
	// Initialize _volumeRegion_ from volume region(s)
	VolumeRegion *volumeRegion;
	if (volumeRegions.size() == 0)
		volumeRegion = NULL;
	else if (volumeRegions.size() == 1)
		volumeRegion = volumeRegions[0];
	else
		volumeRegion = new AggregateVolume(volumeRegions);
	// Make sure all plugins initialized properly
	if (!camera || !sampler || !film || !accelerator || !filter
			|| !surfaceIntegrator || !volumeIntegrator) {
		luxError(LUX_BUG,LUX_SEVERE,"Unable to create scene due to missing plug-ins");
		return NULL;
	}
	Scene *ret = new Scene(camera,
			surfaceIntegrator, volumeIntegrator,
			sampler, accelerator, lights, volumeRegion);
	// Erase primitives, lights, and volume regions from _RenderOptions_
	primitives.erase(primitives.begin(), primitives.end());
	lights.erase(lights.begin(), lights.end());
	volumeRegions.erase(volumeRegions.begin(), volumeRegions.end());

    // Dade - enable debug mode
    if (debugMode) {
        // Dade -  set the scene seed to zero
        ret->seedBase = 0;
    }

	return ret;
}

//user interactive thread functions
void Context::start() {
	luxCurrentScene->Start();
}

void Context::pause() {
	luxCurrentScene->Pause();
}

void Context::setHaltSamplePerPixel(int haltspp, bool haveEnoughSamplePerPixel,
		bool suspendThreadsWhenDone) {
	FlexImageFilm *fif = (FlexImageFilm *)luxCurrentScene->camera->film;
	fif->haltSamplePerPixel = haltspp;
	fif->enoughSamplePerPixel = haveEnoughSamplePerPixel;
	luxCurrentScene->suspendThreadsWhenDone = suspendThreadsWhenDone;
}

void Context::wait() {
    boost::mutex::scoped_lock lock(renderingMutex);
}

void Context::exit() {
    // Dade - stop the render farm too
    activeContext->renderFarm->stopFilmUpdater();
    // Dade - update the film for the last time
    activeContext->renderFarm->updateFilm(luxCurrentScene);
    // Dade - disconnect from all servers
    activeContext->renderFarm->disconnectAll();

    luxCurrentScene->Exit();
}

//controlling number of threads
int Context::addThread() {
	return luxCurrentScene->AddThread();
}

void Context::removeThread() {
	luxCurrentScene->RemoveThread();
}

int Context::getRenderingThreadsStatus(RenderingThreadInfo *info, int maxInfoCount) {
	if (!luxCurrentScene)
		return 0;

	return luxCurrentScene->getThreadsStatus(info, maxInfoCount);
}

//framebuffer access
void Context::updateFramebuffer() {
	luxCurrentScene->UpdateFramebuffer();
}

unsigned char* Context::framebuffer() {
	return luxCurrentScene->GetFramebuffer();
}

double Context::statistics(const string &statName) {
	if (statName=="sceneIsReady") return (luxCurrentScene!=NULL);
	else if (luxCurrentScene!=NULL) return luxCurrentScene->Statistics(statName);
	else return 0;
}

void Context::transmitFilm(std::basic_ostream<char> &stream) {
	//jromang TODO : fix this hack !
	FlexImageFilm *fif = (FlexImageFilm *)luxCurrentScene->camera->film;

    fif->TransmitFilm(stream);
}

void Context::updateFilmFromNetwork() {
	renderFarm->updateFilm(luxCurrentScene);
}
