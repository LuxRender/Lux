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



#include "lux.h"
#include "scene.h"
#include "context.h"
#include "dynload.h"
#include "api.h"
#include "renderer.h"
#include "camera.h"
#include "light.h"
#include "shape.h"
#include "volume.h"
#include "material.h"
#include "stats.h"
#include "renderfarm.h"
#include "film/fleximage.h"
#include "epsilon.h"
#include "renderers/samplerrenderer.h"

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
		LOG(LUX_SEVERE,LUX_NOTSTARTED)<<"luxInit() must be called before calling  '"<<func<<"'. Ignoring."; \
	return; \
}
#define VERIFY_OPTIONS(func) \
VERIFY_INITIALIZED(func); \
if (currentApiState == STATE_WORLD_BLOCK) { \
		LOG(LUX_ERROR,LUX_NESTING)<<"Options cannot be set inside world block; '"<<func<<"' not allowed.  Ignoring."; \
	return; \
}
#define VERIFY_WORLD(func) \
VERIFY_INITIALIZED(func); \
if (currentApiState == STATE_OPTIONS_BLOCK) { \
	LOG(LUX_ERROR,LUX_NESTING)<<"Scene description must be inside world block; '"<<func<<"' not allowed.  Ignoring."; \
	return; \
}

boost::shared_ptr<lux::Texture<float> > Context::GetFloatTexture(const string &n) const
{
	if (n != "") {
		if (graphicsState->floatTextures.find(n) !=
			graphicsState->floatTextures.end())
			return graphicsState->floatTextures[n];
		LOG(LUX_ERROR,LUX_BADTOKEN) << "Couldn't find float texture named '" << n << "'";
	}
	return boost::shared_ptr<lux::Texture<float> >();
}
boost::shared_ptr<lux::Texture<SWCSpectrum> > Context::GetColorTexture(const string &n) const
{
	if (n != "") {
		if (graphicsState->colorTextures.find(n) !=
			graphicsState->colorTextures.end())
			return graphicsState->colorTextures[n];
		LOG(LUX_ERROR,LUX_BADTOKEN) << "Couldn't find color texture named '" << n << "'";
	}
	return boost::shared_ptr<lux::Texture<SWCSpectrum> >();
}
boost::shared_ptr<lux::Texture<FresnelGeneral> > Context::GetFresnelTexture(const string &n) const
{
	if (n != "") {
		if (graphicsState->fresnelTextures.find(n) !=
			graphicsState->fresnelTextures.end())
			return graphicsState->fresnelTextures[n];
		LOG(LUX_ERROR,LUX_BADTOKEN) << "Couldn't find fresnel texture named '" << n << "'";
	}
	return boost::shared_ptr<lux::Texture<FresnelGeneral> >();
}
boost::shared_ptr<lux::Material > Context::GetMaterial(const string &n) const
{
	if (n != "") {
		if (graphicsState->namedMaterials.find(n) !=
			graphicsState->namedMaterials.end())
			return graphicsState->namedMaterials[n];
		LOG(LUX_ERROR,LUX_BADTOKEN) << "Couldn't find material named '" << n << "'";
	}
	return boost::shared_ptr<lux::Material>();
}

void Context::Init() {
	// Dade - reinitialize
	terminated = false;
	currentApiState = STATE_OPTIONS_BLOCK;
	luxCurrentRenderer = NULL;
	luxCurrentScene = NULL;
	luxCurrentSceneReady = false;
	curTransform = lux::Transform();
	namedCoordinateSystems.clear();
	renderOptions = new RenderOptions;
	graphicsState = new GraphicsState;
	pushedGraphicsStates.clear();
	pushedTransforms.clear();
	renderFarm = new RenderFarm();
	filmOverrideParams = NULL;
}

void Context::Free() {
	// Dade - free memory
	luxCurrentSceneReady = false;

	delete luxCurrentRenderer;
	luxCurrentRenderer = NULL;

	delete luxCurrentScene;
	luxCurrentScene = NULL;

	delete renderOptions;
	renderOptions = NULL;

	delete graphicsState;
	graphicsState = NULL;

	delete renderFarm;
	renderFarm = NULL;

	delete filmOverrideParams;
	filmOverrideParams = NULL;
}

// API Function Definitions

void Context::AddServer(const string &n) {
	renderFarm->connect(n);

	// NOTE - Ratow - if this is the first server added during rendering, make sure update thread is started
	if (GetServerCount() == 1 && luxCurrentScene)
		renderFarm->startFilmUpdater(luxCurrentScene);
}

void Context::RemoveServer(const string &n) {
	renderFarm->disconnect(n);

	// NOTE - Ratow - if this is the last server, make sure update thread is stopped
	if (GetServerCount() == 0)
		renderFarm->stopFilmUpdater();
}

u_int Context::GetServerCount() {
	return renderFarm->getServerCount();
}

u_int Context::GetRenderingServersStatus(RenderingServerInfo *info, u_int maxInfoCount) {
	return renderFarm->getServersStatus(info, maxInfoCount);
}

void Context::Cleanup() {
	renderFarm->send("luxCleanup");

	StatsCleanup();
	// API Cleanup
	if (currentApiState == STATE_UNINITIALIZED)
		LOG(LUX_ERROR,LUX_NOTSTARTED)<< "luxCleanup() called without luxInit().";
	else if (currentApiState == STATE_WORLD_BLOCK)
		LOG(LUX_ERROR,LUX_ILLSTATE)<< "luxCleanup() called while inside world block.";
	
	// Dade - free memory
	Free();

	// Dade - reinitialize
	Init();
}

void Context::Identity() {
	VERIFY_INITIALIZED("Identity");
	renderFarm->send("luxIdentity");
	curTransform = lux::Transform();
}

void Context::Translate(float dx, float dy, float dz) {
	VERIFY_INITIALIZED("Translate");
	renderFarm->send("luxTranslate", dx, dy, dz);
	curTransform = curTransform * lux::Translate(Vector(dx, dy, dz));
}

void Context::Transform(float tr[16]) {
	VERIFY_INITIALIZED("Transform");
	renderFarm->send("luxTransform", tr);
	boost::shared_ptr<Matrix4x4> o(new Matrix4x4(
			tr[0], tr[4], tr[8], tr[12],
			tr[1], tr[5], tr[9], tr[13],
			tr[2], tr[6], tr[10], tr[14],
			tr[3], tr[7], tr[11], tr[15]));
	curTransform = lux::Transform(o);
}
void Context::ConcatTransform(float tr[16]) {
	VERIFY_INITIALIZED("ConcatTransform");
	renderFarm->send("luxConcatTransform", tr);
	boost::shared_ptr<Matrix4x4> o(new Matrix4x4(tr[0], tr[4], tr[8], tr[12],
			tr[1], tr[5], tr[9], tr[13],
			tr[2], tr[6], tr[10], tr[14],
			tr[3], tr[7], tr[11], tr[15]));
	curTransform = curTransform * lux::Transform(o);
}
void Context::Rotate(float angle, float dx, float dy, float dz) {
	VERIFY_INITIALIZED("Rotate");
	renderFarm->send("luxRotate", angle, dx, dy, dz);
	curTransform = curTransform * lux::Rotate(angle, Vector(dx, dy, dz));
}
void Context::Scale(float sx, float sy, float sz) {
	VERIFY_INITIALIZED("Scale");
	renderFarm->send("luxScale", sx, sy, sz);
	curTransform = curTransform * lux::Scale(sx, sy, sz);
}
void Context::LookAt(float ex, float ey, float ez, float lx, float ly, float lz,
	float ux, float uy, float uz) {
	VERIFY_INITIALIZED("LookAt");
	renderFarm->send("luxLookAt", ex, ey, ez, lx, ly, lz, ux, uy, uz);
	curTransform = curTransform * lux::LookAt(Point(ex, ey, ez),
		Point(lx, ly, lz), Vector(ux, uy, uz));
}
void Context::CoordinateSystem(const string &n) {
	VERIFY_INITIALIZED("CoordinateSystem");
	renderFarm->send("luxCoordinateSystem", n);
	namedCoordinateSystems[n] = curTransform;
}
void Context::CoordSysTransform(const string &n) {
	VERIFY_INITIALIZED("CoordSysTransform");
	renderFarm->send("luxCoordSysTransform", n);
	if (namedCoordinateSystems.find(n) != namedCoordinateSystems.end())
		curTransform = namedCoordinateSystems[n];
	else {
		LOG(LUX_ERROR,LUX_SYNTAX) << "Coordinate system '" << n << "' unknown";
	}
}
void Context::SetEpsilon(const float minValue, const float maxValue)
{
	VERIFY_INITIALIZED("SetEpsilon");
	renderFarm->send("luxSetEpsilon", minValue, maxValue);
	MachineEpsilon::SetMin(minValue);
	MachineEpsilon::SetMax(maxValue);
}
void Context::EnableDebugMode() {
    VERIFY_OPTIONS("EnableDebugMode");
    // Dade - I avoid to transmit the EnableDebugMode option to the renderFarm
    renderOptions->debugMode = true;
}

void Context::DisableRandomMode() {
    VERIFY_OPTIONS("DisableRandomMode");
    // Slaves needs random seeds
    renderOptions->randomMode = false;
}

void Context::PixelFilter(const string &n, const ParamSet &params) {
	VERIFY_OPTIONS("PixelFilter");
	renderFarm->send("luxPixelFilter", n, params);
	renderOptions->filterName = n;
	renderOptions->filterParams = params;
}
void Context::Film(const string &type, const ParamSet &params) {
	VERIFY_OPTIONS("Film");
	renderFarm->send("luxFilm", type, params);
	renderOptions->filmParams = params;
	renderOptions->filmName = type;
	if (filmOverrideParams)
		renderOptions->filmParams.Add(*filmOverrideParams);
}
void Context::Sampler(const string &n, const ParamSet &params) {
	VERIFY_OPTIONS("Sampler");
	renderFarm->send("luxSampler", n, params);
	renderOptions->samplerName = n;
	renderOptions->samplerParams = params;
}
void Context::Accelerator(const string &n, const ParamSet &params) {
	VERIFY_OPTIONS("Accelerator");
	renderFarm->send("luxAccelerator", n, params);
	renderOptions->acceleratorName = n;
	renderOptions->acceleratorParams = params;
}
void Context::SurfaceIntegrator(const string &n, const ParamSet &params) {
	VERIFY_OPTIONS("SurfaceIntegrator");
	renderFarm->send("luxSurfaceIntegrator", n, params);
	renderOptions->surfIntegratorName = n;
	renderOptions->surfIntegratorParams = params;
}
void Context::VolumeIntegrator(const string &n, const ParamSet &params) {
	VERIFY_OPTIONS("VolumeIntegrator");
	renderFarm->send("luxVolumeIntegrator", n, params);
	renderOptions->volIntegratorName = n;
	renderOptions->volIntegratorParams = params;
}
void Context::Camera(const string &n, const ParamSet &params) {
	VERIFY_OPTIONS("Camera");
	renderFarm->send("luxCamera", n, params);
	renderOptions->cameraName = n;
	renderOptions->cameraParams = params;
	renderOptions->worldToCamera = curTransform;
	namedCoordinateSystems["camera"] = curTransform.GetInverse();

	string endTransform = renderOptions->cameraParams.FindOneString("endtransform", "");

	if (namedCoordinateSystems.find(endTransform) != namedCoordinateSystems.end())
		renderOptions->worldToCameraEnd = namedCoordinateSystems[endTransform];
	else
		renderOptions->worldToCameraEnd = curTransform;

}
void Context::WorldBegin() {
	VERIFY_OPTIONS("WorldBegin");
	renderFarm->send("luxWorldBegin");
	currentApiState = STATE_WORLD_BLOCK;
	curTransform = lux::Transform();
	namedCoordinateSystems["world"] = curTransform;
}
void Context::AttributeBegin() {
	VERIFY_WORLD("AttributeBegin");
	renderFarm->send("luxAttributeBegin");
	pushedGraphicsStates.push_back(*graphicsState);
	pushedTransforms.push_back(curTransform);
}
void Context::AttributeEnd() {
	VERIFY_WORLD("AttributeEnd");
	renderFarm->send("luxAttributeEnd");
	if (!pushedGraphicsStates.size()) {
		LOG(LUX_ERROR,LUX_ILLSTATE)<<"Unmatched luxAttributeEnd() encountered. Ignoring it.";
		return;
	}
	*graphicsState = pushedGraphicsStates.back();
	curTransform = pushedTransforms.back();
	pushedGraphicsStates.pop_back();
	pushedTransforms.pop_back();
}
void Context::TransformBegin() {
	VERIFY_INITIALIZED("TransformBegin");
	renderFarm->send("luxTransformBegin");
	pushedTransforms.push_back(curTransform);
}
void Context::TransformEnd() {
	VERIFY_INITIALIZED("TransformEnd");
	renderFarm->send("luxTransformEnd");
	if (!pushedTransforms.size()) {
		LOG(LUX_ERROR,LUX_ILLSTATE)<< "Unmatched luxTransformEnd() encountered. Ignoring it.";
		return;
	}
	curTransform = pushedTransforms.back();
	pushedTransforms.pop_back();
}
void Context::Texture(const string &n, const string &type,
	const string &texname, const ParamSet &params) {
	VERIFY_WORLD("Texture");
	renderFarm->send("luxTexture", n, type, texname, params);
	if (type == "float") {
		// Create _float_ texture and store in _floatTextures_
		if (graphicsState->floatTextures.find(n) !=
			graphicsState->floatTextures.end()) {
			LOG(LUX_WARNING,LUX_SYNTAX) << "Float texture '" << n << "' being redefined.";
		}
		boost::shared_ptr<lux::Texture<float> > ft(
			MakeFloatTexture(texname, curTransform, params));
		if (ft)
			graphicsState->floatTextures[n] = ft;
	} else if (type == "color") {
		// Create _color_ texture and store in _colorTextures_
		if (graphicsState->colorTextures.find(n) !=
			graphicsState->colorTextures.end()) {
			LOG(LUX_WARNING,LUX_SYNTAX) << "Color texture '" << n << "' being redefined.";
		}
		boost::shared_ptr<lux::Texture<SWCSpectrum> > st(
			MakeSWCSpectrumTexture(texname, curTransform, params));
		if (st)
			graphicsState->colorTextures[n] = st;
	} else if (type == "fresnel") {
		// Create _fresnel_ texture and store in _fresnelTextures_
		if (graphicsState->fresnelTextures.find(n) !=
			graphicsState->fresnelTextures.end()) {
			LOG(LUX_WARNING,LUX_SYNTAX) << "Fresnel texture '" << n << "' being redefined.";
		}
		boost::shared_ptr<lux::Texture<FresnelGeneral> > fr(
			MakeFresnelTexture(texname, curTransform, params));
		if (fr)
			graphicsState->fresnelTextures[n] = fr;
	} else {
		LOG(LUX_ERROR,LUX_SYNTAX) << "Texture type '" << type << "' unknown";
	}
}
void Context::Material(const string &n, const ParamSet &params) {
	VERIFY_WORLD("Material");
	renderFarm->send("luxMaterial", n, params);
	graphicsState->material = MakeMaterial(n, curTransform, params);
}

void Context::MakeNamedMaterial(const string &n, const ParamSet &_params)
{
	VERIFY_WORLD("MakeNamedMaterial");
	ParamSet params=_params;
	renderFarm->send("luxMakeNamedMaterial", n, params);
	if (graphicsState->namedMaterials.find(n) !=
		graphicsState->namedMaterials.end()) {
		LOG(LUX_WARNING,LUX_SYNTAX) << "Named material '" << n << "' being redefined.";
	}
	string type = params.FindOneString("type", "matte");
	params.EraseString("type");
	graphicsState->namedMaterials[n] = MakeMaterial(type, curTransform,
		params);
}

void Context::MakeNamedVolume(const string &id, const string &name,
	const ParamSet &params)
{
	VERIFY_WORLD("MakeNamedVolume");
	renderFarm->send("luxMakeNamedVolume", id, name, params);
	if (graphicsState->namedVolumes.find(id) !=
		graphicsState->namedVolumes.end()) {
		LOG(LUX_WARNING, LUX_SYNTAX) << "Named volume '" << id <<
			"' being redefined.";
	}
	graphicsState->namedVolumes[id] = MakeVolume(name, curTransform, params);
}

void Context::NamedMaterial(const string &n) {
	VERIFY_WORLD("NamedMaterial");
	renderFarm->send("luxNamedMaterial", n);
	if (graphicsState->namedMaterials.find(n) !=
		graphicsState->namedMaterials.end()) {
		// Create a temporary to increase share count
		// The copy operator is just a swap
		boost::shared_ptr<lux::Material> m(graphicsState->namedMaterials[n]);
		graphicsState->material = m;
	} else {
		LOG(LUX_ERROR,LUX_SYNTAX) << "Named material '" << n << "' unknown";
	}
}

void Context::LightGroup(const string &n, const ParamSet &params)
{
	VERIFY_WORLD("LightGroup");
	renderFarm->send("luxLightGroup", n, params);
	u_int i = 0;
	for (;i < renderOptions->lightGroups.size(); ++i) {
		if (n == renderOptions->lightGroups[i])
			break;
	}
	if (i == renderOptions->lightGroups.size())
		renderOptions->lightGroups.push_back(n);
	graphicsState->currentLightGroup = n;
}


void Context::LightSource(const string &n, const ParamSet &params) {
	VERIFY_WORLD("LightSource");
	renderFarm->send("luxLightSource", n, params);
	u_int lg = GetLightGroup();

	if (n == "sunsky") {
		//SunSky light - create both sun & sky lightsources
		Light *lt_sun = MakeLight("sun", curTransform, params);
		if (lt_sun == NULL) {
			LOG(LUX_ERROR,LUX_SYNTAX)<< "luxLightSource: light type sun unknown.";
			graphicsState->currentLightPtr0 = NULL;
		} else {
			renderOptions->lights.push_back(lt_sun);
			graphicsState->currentLight = n;
			graphicsState->currentLightPtr0 = lt_sun;
			lt_sun->group = lg;
		}
		Light *lt_sky = MakeLight("sky", curTransform, params);
		if (lt_sky == NULL) {
			LOG(LUX_ERROR,LUX_SYNTAX)<< "luxLightSource: light type sky unknown.";
			graphicsState->currentLightPtr1 = NULL;
		} else {
			renderOptions->lights.push_back(lt_sky);
			graphicsState->currentLight = n;
			graphicsState->currentLightPtr1 = lt_sky;
			lt_sky->group = lg;
		}
	} else {
		// other lightsource type
		Light *lt = MakeLight(n, curTransform, params);
		if (lt == NULL) {
			LOG(LUX_ERROR,LUX_SYNTAX) << "luxLightSource: light type '" << n << "' unknown";
		} else {
			renderOptions->lights.push_back(lt);
			graphicsState->currentLight = n;
			graphicsState->currentLightPtr0 = lt;
			graphicsState->currentLightPtr1 = NULL;
			lt->group = lg;
		}
	}
}

void Context::AreaLightSource(const string &n, const ParamSet &params) {
	VERIFY_WORLD("AreaLightSource");
	renderFarm->send("luxAreaLightSource", n, params);
	graphicsState->areaLight = n;
	graphicsState->areaLightParams = params;
}

void Context::PortalShape(const string &n, const ParamSet &params) {
	VERIFY_WORLD("PortalShape");
	renderFarm->send("luxPortalShape", n, params);
	boost::shared_ptr<Primitive> sh(MakeShape(n, curTransform,
		graphicsState->reverseOrientation, params));
	if (!sh)
		return;
	params.ReportUnused();

	if (graphicsState->currentLight != "") {
		if (graphicsState->currentLightPtr0)
			graphicsState->currentLightPtr0->AddPortalShape(sh);

		if (graphicsState->currentLightPtr1)
			graphicsState->currentLightPtr1->AddPortalShape(sh);
	}
}

void Context::Shape(const string &n, const ParamSet &params) {
	VERIFY_WORLD("Shape");
	renderFarm->send("luxShape", n, params);
	boost::shared_ptr<lux::Shape> sh(MakeShape(n, curTransform,
		graphicsState->reverseOrientation, params));
	if (!sh)
		return;
	params.ReportUnused();

	// Initialize area light for shape
	AreaLight *area = NULL;
	if (graphicsState->areaLight != "") {
		u_int lg = GetLightGroup();
		area = MakeAreaLight(graphicsState->areaLight, curTransform,
			graphicsState->areaLightParams, sh);
		if (area)
			area->group = lg;
	}

	// Lotus - Set the material
	if (graphicsState->material)
		sh->SetMaterial(graphicsState->material);
	else {
		boost::shared_ptr<lux::Material> m(MakeMaterial("matte",
			curTransform, ParamSet()));
		sh->SetMaterial(m);
	}
	sh->SetExterior(graphicsState->exterior);
	sh->SetInterior(graphicsState->interior);

	// Create primitive and add to scene or current instance
	boost::shared_ptr<Primitive> pr(sh);
	if (renderOptions->currentInstance) {
		if (area)
			LOG(LUX_WARNING,LUX_UNIMPLEMENT)<<"Area lights not supported with object instancing";
		if (!pr->CanIntersect())
			pr->Refine(*(renderOptions->currentInstance),
				PrimitiveRefinementHints(false), pr);
		else
			renderOptions->currentInstance->push_back(pr);
	} else if (area) {
		// Lotus - add a decorator to set the arealight field
		boost::shared_ptr<Primitive> prim(new AreaLightPrimitive(pr,
			area));
		renderOptions->primitives.push_back(prim);
		// Add area light for primitive to light vector
		renderOptions->lights.push_back(area);
	} else
		renderOptions->primitives.push_back(pr);
}
void Context::Renderer(const string &n, const ParamSet &params) {
	VERIFY_OPTIONS("Renderer");
	renderFarm->send("luxRenderer", n, params);
	renderOptions->rendererName = n;
	renderOptions->rendererParams = params;
}
void Context::ReverseOrientation() {
	VERIFY_WORLD("ReverseOrientation");
	renderFarm->send("luxReverseOrientation");
	graphicsState->reverseOrientation = !graphicsState->reverseOrientation;
}
void Context::Volume(const string &n, const ParamSet &params) {
	VERIFY_WORLD("Volume");
	renderFarm->send("luxVolume", n, params);
	Region *vr = MakeVolumeRegion(n, curTransform, params);
	if (vr)
		renderOptions->volumeRegions.push_back(vr);
}
void Context::Exterior(const string &n) {
	VERIFY_WORLD("Exterior");
	renderFarm->send("luxExterior", n);
	if (n == "")
		graphicsState->interior = boost::shared_ptr<lux::Volume>();
	else if (graphicsState->namedVolumes.find(n) !=
		graphicsState->namedVolumes.end()) {
		// Create a temporary to increase share count
		// The copy operator is just a swap
		boost::shared_ptr<lux::Volume> v(graphicsState->namedVolumes[n]);
		graphicsState->exterior = v;
	} else {
		LOG(LUX_ERROR, LUX_SYNTAX) << "Exterior named volume '" << n <<
			"' unknown";
	}
}
void Context::Interior(const string &n) {
	VERIFY_WORLD("Interior");
	renderFarm->send("luxInterior", n);
	if (n == "")
		graphicsState->interior = boost::shared_ptr<lux::Volume>();
	else if (graphicsState->namedVolumes.find(n) !=
		graphicsState->namedVolumes.end()) {
		// Create a temporary to increase share count
		// The copy operator is just a swap
		boost::shared_ptr<lux::Volume> v(graphicsState->namedVolumes[n]);
		graphicsState->interior = v;
	} else {
		LOG(LUX_ERROR, LUX_SYNTAX) << "Interior named volume '" << n <<
			"' unknown";
	}
}
void Context::ObjectBegin(const string &n) {
	VERIFY_WORLD("ObjectBegin");
	renderFarm->send("luxObjectBegin", n);
	AttributeBegin();
	if (renderOptions->currentInstance)
		LOG(LUX_ERROR,LUX_NESTING)<< "ObjectBegin called inside of instance definition";
	renderOptions->instances[n] = vector<boost::shared_ptr<Primitive> >();
	renderOptions->currentInstance = &renderOptions->instances[n];
}
void Context::ObjectEnd() {
	VERIFY_WORLD("ObjectEnd");
	renderFarm->send("luxObjectEnd");
	if (!renderOptions->currentInstance)
		LOG(LUX_ERROR,LUX_NESTING)<< "ObjectEnd called outside of instance definition";
	renderOptions->currentInstance = NULL;
	AttributeEnd();
}
void Context::ObjectInstance(const string &n) {
	VERIFY_WORLD("ObjectInstance");
	renderFarm->send("luxObjectInstance", n);
	// Object instance error checking
	if (renderOptions->currentInstance) {
		LOG(LUX_ERROR,LUX_NESTING)<< "ObjectInstance can't be called inside instance definition";
		return;
	}
	if (renderOptions->instances.find(n) == renderOptions->instances.end()) {
		LOG(LUX_ERROR,LUX_BADTOKEN) << "Unable to find instance named '" << n << "'";
		return;
	}
	vector<boost::shared_ptr<Primitive> > &in = renderOptions->instances[n];
	if (in.size() == 0)
		return;
	if (in.size() > 1 || !in[0]->CanIntersect()) {
		// Refine instance _Primitive_s and create aggregate
		boost::shared_ptr<Primitive> accel(
			MakeAccelerator(renderOptions->acceleratorName, in,
			renderOptions->acceleratorParams));
		if (!accel)
			accel = MakeAccelerator("kdtree", in, ParamSet());
		if (!accel)
			LOG(LUX_SEVERE,LUX_BUG)<< "Unable to find \"kdtree\" accelerator";
		in.clear();
		in.push_back(accel);
	}

	// Initialize material for instance
	boost::shared_ptr<Primitive> o(new InstancePrimitive(in[0],
		curTransform, graphicsState->material,
		graphicsState->exterior, graphicsState->interior));
	renderOptions->primitives.push_back(o);
}

void Context::MotionInstance(const string &n, float startTime, float endTime, const string &toTransform) {
	VERIFY_WORLD("MotionInstance");
	renderFarm->send("luxMotionInstance", n, startTime, endTime, toTransform);
	// Object instance error checking
	if (renderOptions->currentInstance) {
		LOG(LUX_ERROR,LUX_NESTING)<< "MotionInstance can't be called inside instance definition";
		return;
	}
	if (renderOptions->instances.find(n) == renderOptions->instances.end()) {
		LOG(LUX_ERROR,LUX_BADTOKEN) << "Unable to find instance named '" << n << "'";
		return;
	}
	vector<boost::shared_ptr<Primitive> > &in = renderOptions->instances[n];
	if (in.size() == 0)
		return;
	if (in.size() > 1 || !in[0]->CanIntersect()) {
		// Refine instance _Primitive_s and create aggregate
		boost::shared_ptr<Primitive> accel(
			MakeAccelerator(renderOptions->acceleratorName, in,
			renderOptions->acceleratorParams));
		if (!accel)
			accel = MakeAccelerator("kdtree", in, ParamSet());
		if (!accel)
			LOG(LUX_SEVERE,LUX_BUG)<< "Unable to find \"kdtree\" accelerator";
		in.clear();
		in.push_back(accel);
	}

	// Fetch named ToTransform coordinatesystem
	lux::Transform EndTransform;
	if (namedCoordinateSystems.find(toTransform) != namedCoordinateSystems.end())
		EndTransform = namedCoordinateSystems[toTransform];
	else {
		LOG(LUX_SEVERE,LUX_BUG)<< "Unable to find coordinate system named '" << n << "' for MotionInstance";
	}

	// Initialize material for instance
	boost::shared_ptr<Primitive> o(new MotionPrimitive(in[0], curTransform,
		EndTransform, startTime, endTime, graphicsState->material,
		graphicsState->exterior, graphicsState->interior));
	renderOptions->primitives.push_back(o);
}

void Context::WorldEnd() {
	VERIFY_WORLD("WorldEnd");
	renderFarm->send("luxWorldEnd");
	renderFarm->flush();

	// Dade - get the lock, other thread can use this lock to wait the end
	// of the rendering
	boost::mutex::scoped_lock lock(renderingMutex);

	// Ensure there are no pushed graphics states
	while (pushedGraphicsStates.size()) {
		LOG(LUX_WARNING,LUX_NESTING)<<"Missing end to luxAttributeBegin()";
		pushedGraphicsStates.pop_back();
		pushedTransforms.pop_back();
	}

	if (!terminated) {
		// Create scene and render
		luxCurrentScene = renderOptions->MakeScene();
		if (luxCurrentScene) {
			luxCurrentRenderer = renderOptions->MakeRenderer();

			// Dade - check if we have to start the network rendering updater thread
			if (renderFarm->getServerCount() > 0)
				renderFarm->startFilmUpdater(luxCurrentScene);

			luxCurrentRenderer->Render(luxCurrentScene);

			// Check if we have to stop the network rendering updater thread
			if (GetServerCount() > 0) {
				// Stop the render farm too
				activeContext->renderFarm->stopFilmUpdater();
				// Update the film for the last time
				activeContext->renderFarm->updateFilm(luxCurrentScene);
				// Disconnect from all servers
				activeContext->renderFarm->disconnectAll();
			}

			// Store final image
			luxCurrentScene->camera->film->WriteImage((ImageType)(IMAGE_FILEOUTPUT|IMAGE_FRAMEBUFFER));
		}
	}

	//delete scene;
	// Clean up after rendering
	currentApiState = STATE_OPTIONS_BLOCK;
	StatsPrint(std::cout);
	curTransform = lux::Transform();
	namedCoordinateSystems.erase(namedCoordinateSystems.begin(),
		namedCoordinateSystems.end());
}

Scene *Context::RenderOptions::MakeScene() const {
	// Create scene objects from API settings
	lux::Filter *filter = MakeFilter(filterName, filterParams);
	lux::Film *film = MakeFilm(filmName, filmParams, filter);
	lux::Camera *camera = MakeCamera(cameraName, worldToCamera,
		worldToCameraEnd, cameraParams, film);
	lux::Sampler *sampler = MakeSampler(samplerName, samplerParams, film);
	lux::SurfaceIntegrator *surfaceIntegrator = MakeSurfaceIntegrator(
		surfIntegratorName, surfIntegratorParams);
	lux::VolumeIntegrator *volumeIntegrator = MakeVolumeIntegrator(
		volIntegratorName, volIntegratorParams);
	boost::shared_ptr<Primitive> accelerator(MakeAccelerator(acceleratorName,
		primitives, acceleratorParams));
	if (!accelerator) {
		ParamSet ps;
		accelerator = MakeAccelerator("kdtree", primitives, ps);
	}
	if (!accelerator)
		LOG(LUX_SEVERE,LUX_BUG)<< "Unable to find \"kdtree\" accelerator";
	// Initialize _volumeRegion_ from volume region(s)
	Region *volumeRegion;
	if (volumeRegions.size() == 0)
		volumeRegion = NULL;
	else if (volumeRegions.size() == 1)
		volumeRegion = volumeRegions[0];
	else
		volumeRegion = new AggregateRegion(volumeRegions);
	// Make sure all plugins initialized properly
	if (!camera || !sampler || !film || !accelerator || !filter ||
		!surfaceIntegrator || !volumeIntegrator) {
		LOG(LUX_SEVERE,LUX_BUG)<< "Unable to create scene due to missing plug-ins";
		return NULL;
	}

	Scene *ret = new Scene(camera, surfaceIntegrator, volumeIntegrator,
		sampler, primitives, accelerator, lights, lightGroups, volumeRegion);
	// Erase primitives, lights, volume regions and instances from _RenderOptions_
	primitives.clear();
	lights.clear();
	volumeRegions.clear();
	currentInstance = NULL;
	instances.clear();

	// Set a fixed seed for animations or debugging
	if (debugMode || !randomMode)
		ret->seedBase = 1000;

	return ret;
}

Renderer *Context::RenderOptions::MakeRenderer() const {
	return lux::MakeRenderer(rendererName, rendererParams);
}

// Load/save FLM file
void Context::LoadFLM(const string &flmFileName) {
	// Create the film
	lux::Film* flm = FlexImageFilm::CreateFilmFromFLM(flmFileName);
	if (!flm) {
		LOG(LUX_SEVERE,LUX_BUG)<< "Unable to create film";
		return;
	}
	// Update context
	lux::Transform dummyTransform;
	ParamSet dummyParams;
	lux::Camera *cam = MakeCamera("perspective", dummyTransform,
		dummyTransform, dummyParams, flm);
	if (!cam) {
		LOG(LUX_SEVERE,LUX_BUG)<< "Unable to create dummy camera";
		delete flm;
		return;
	}
	luxCurrentScene = new Scene(cam);
	SceneReady();
}
void Context::SaveFLM(const string &flmFileName) {
	luxCurrentScene->SaveFLM(flmFileName);
}

void Context::OverrideResumeFLM(const string &flmFileName) {
	if (!filmOverrideParams) {
		filmOverrideParams = new ParamSet();
	}
	const bool boolTrue = true;
	const bool boolFalse = false;
	const string filename = flmFileName.substr(0, flmFileName.length() - 4);
	filmOverrideParams->AddBool("write_resume_flm", &boolTrue);
	filmOverrideParams->AddBool("restart_resume_flm", &boolFalse);
	filmOverrideParams->AddString("filename", &filename);
}

//user interactive thread functions
void Context::Resume() {
	luxCurrentRenderer->Resume();
}

void Context::Pause() {
	luxCurrentRenderer->Pause();
}

void Context::SetHaltSamplePerPixel(int haltspp, bool haveEnoughSamplePerPixel,
	bool suspendThreadsWhenDone) {
	lux::Film *film = luxCurrentScene->camera->film;
	film->haltSamplePerPixel = haltspp;
	film->enoughSamplePerPixel = haveEnoughSamplePerPixel;
	luxCurrentRenderer->SuspendWhenDone(suspendThreadsWhenDone);
}

void Context::Wait() {
    boost::mutex::scoped_lock lock(renderingMutex);
}

void Context::Exit() {
	if (GetServerCount() > 0) {
		// Dade - stop the render farm too
		activeContext->renderFarm->stopFilmUpdater();
		// Dade - update the film for the last time
		activeContext->renderFarm->updateFilm(luxCurrentScene);
		// Dade - disconnect from all servers
		activeContext->renderFarm->disconnectAll();
	}
	
	terminated = true;

	// Reset Dynamic Epsilon values
	MachineEpsilon::SetMin(DEFAULT_EPSILON_MIN);
	MachineEpsilon::SetMax(DEFAULT_EPSILON_MAX);

	if (luxCurrentRenderer)
		luxCurrentRenderer->Terminate();
}

//controlling number of threads
u_int Context::AddThread() {
	const vector<RendererHostDescription *> &hosts = luxCurrentRenderer->GetHostDescs();

	//FIXME
	SRDeviceDescription *desc = (SRDeviceDescription *)hosts[0]->GetDeviceDescs()[0];
	desc->SetUsedUnitsCount(desc->GetUsedUnitsCount() + 1);

	return desc->GetUsedUnitsCount();
}

void Context::RemoveThread() {
	const vector<RendererHostDescription *> &hosts = luxCurrentRenderer->GetHostDescs();

	//FIXME
	SRDeviceDescription *desc = (SRDeviceDescription *)hosts[0]->GetDeviceDescs()[0];
	desc->SetUsedUnitsCount(max(desc->GetUsedUnitsCount() - 1, 1u));
}

//framebuffer access
void Context::UpdateFramebuffer() {
	luxCurrentScene->UpdateFramebuffer();
}

unsigned char* Context::Framebuffer() {
	return luxCurrentScene->GetFramebuffer();
}

//histogram access
void Context::GetHistogramImage(unsigned char *outPixels, u_int width,
	u_int height, int options)
{
	luxCurrentScene->GetHistogramImage(outPixels, width, height, options);
}

// Parameter Access functions
void Context::SetParameterValue(luxComponent comp, luxComponentParameters param,
	double value, u_int index)
{
	luxCurrentScene->SetParameterValue(comp, param, value, index);
}
double Context::GetParameterValue(luxComponent comp,
	luxComponentParameters param, u_int index)
{
	return luxCurrentScene->GetParameterValue(comp, param, index);
}
double Context::GetDefaultParameterValue(luxComponent comp,
	luxComponentParameters param, u_int index)
{
	return luxCurrentScene->GetDefaultParameterValue(comp, param, index);
}
void Context::SetStringParameterValue(luxComponent comp,
	luxComponentParameters param, const string& value, u_int index)
{
	return luxCurrentScene->SetStringParameterValue(comp, param, value,
		index);
}
string Context::GetStringParameterValue(luxComponent comp,
	luxComponentParameters param, u_int index)
{
	return luxCurrentScene->GetStringParameterValue(comp, param, index);
}
string Context::GetDefaultStringParameterValue(luxComponent comp,
	luxComponentParameters param, u_int index)
{
	return luxCurrentScene->GetDefaultStringParameterValue(comp, param,
		index);
}

u_int Context::GetLightGroup() {
	if (graphicsState->currentLightGroup == "")
		graphicsState->currentLightGroup = "default";
	u_int lg = 0;
	for (;lg < renderOptions->lightGroups.size(); ++lg) {
		if (graphicsState->currentLightGroup == renderOptions->lightGroups[lg])
			break;
	}
	if (lg == renderOptions->lightGroups.size()) {
		if(graphicsState->currentLightGroup == "default") {
			renderOptions->lightGroups.push_back("default");
			lg = renderOptions->lightGroups.size() - 1;
		} else {
			LOG(LUX_ERROR,LUX_BADFILE) << "Undefined lightgroup '" <<
				graphicsState->currentLightGroup <<
				"', using 'default' instead";
			graphicsState->currentLightGroup == "";
			lg = GetLightGroup();
		}
	}
	return lg;
}

double Context::Statistics(const string &statName) {
	if (statName == "sceneIsReady")
		return (luxCurrentScene != NULL && luxCurrentSceneReady &&
			!luxCurrentScene->IsFilmOnly());
	else if (statName == "filmIsReady")
		return (luxCurrentScene != NULL &&
			luxCurrentScene->IsFilmOnly());
	else if (statName == "terminated")
		return terminated;
	else if (luxCurrentRenderer != NULL)
		return luxCurrentRenderer->Statistics(statName);
	else
		return 0;
}
void Context::SceneReady() {
	luxCurrentSceneReady = true;
}

void Context::TransmitFilm(std::basic_ostream<char> &stream) {
	luxCurrentScene->camera->film->TransmitFilm(stream);
}

void Context::TransmitFilm(std::basic_ostream<char> &stream, bool useCompression, bool directWrite) {
	luxCurrentScene->camera->film->TransmitFilm(stream, true, false, useCompression, directWrite);
}

void Context::UpdateFilmFromNetwork() {
	renderFarm->updateFilm(luxCurrentScene);
}
void Context::SetNetworkServerUpdateInterval(int updateInterval)
{
	activeContext->renderFarm->serverUpdateInterval = updateInterval;
}
int Context::GetNetworkServerUpdateInterval()
{
	return activeContext->renderFarm->serverUpdateInterval;
}
