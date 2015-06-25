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
#include "renderfarm.h"
#include "film/fleximage.h"
#include "luxrays/core/epsilon.h"
using luxrays::MachineEpsilon;
#include "renderers/samplerrenderer.h"

#include <boost/iostreams/filtering_stream.hpp>
#include <boost/iostreams/filtering_streambuf.hpp>
#include <boost/iostreams/copy.hpp>
#include <boost/iostreams/filter/zlib.hpp>
#include <boost/filesystem.hpp>

using namespace boost::iostreams;
using namespace lux;

lux::Context *lux::Context::activeContext;

// API Macros
// for transforms which can be inside motion blocks
#define VERIFY_INITIALIZED_TRANSFORMS(func) \
if (currentApiState == STATE_UNINITIALIZED) { \
		LOG(LUX_SEVERE,LUX_NOTSTARTED)<<"luxInit() must be called before calling  '"<<func<<"'. Ignoring."; \
	return; \
}
#define VERIFY_INITIALIZED(func) \
VERIFY_INITIALIZED_TRANSFORMS(func); \
if (inMotionBlock) { \
	LOG(LUX_ERROR,LUX_NESTING)<<"'"<<func<<"' not allowed allowed inside motion block. Ignoring."; \
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
} \
if (inMotionBlock) { \
	LOG(LUX_ERROR,LUX_NESTING)<<"'"<<func<<"' not allowed allowed inside motion block. Ignoring."; \
	return; \
}

boost::shared_ptr<lux::Texture<float> > lux::Context::GetFloatTexture(const string &n) const
{
	if (n != "") {
		if (graphicsState->floatTextures.find(n) !=
			graphicsState->floatTextures.end())
			return graphicsState->floatTextures[n];
		LOG(LUX_ERROR,LUX_BADTOKEN) << "Couldn't find float texture named '" << n << "'";
	}
	return boost::shared_ptr<lux::Texture<float> >();
}
boost::shared_ptr<lux::Texture<SWCSpectrum> > lux::Context::GetColorTexture(const string &n) const
{
	if (n != "") {
		if (graphicsState->colorTextures.find(n) !=
			graphicsState->colorTextures.end())
			return graphicsState->colorTextures[n];
		LOG(LUX_ERROR,LUX_BADTOKEN) << "Couldn't find color texture named '" << n << "'";
	}
	return boost::shared_ptr<lux::Texture<SWCSpectrum> >();
}
boost::shared_ptr<lux::Texture<FresnelGeneral> > lux::Context::GetFresnelTexture(const string &n) const
{
	if (n != "") {
		if (graphicsState->fresnelTextures.find(n) !=
			graphicsState->fresnelTextures.end())
			return graphicsState->fresnelTextures[n];
		LOG(LUX_ERROR,LUX_BADTOKEN) << "Couldn't find fresnel texture named '" << n << "'";
	}
	return boost::shared_ptr<lux::Texture<FresnelGeneral> >();
}
boost::shared_ptr<lux::Material > lux::Context::GetMaterial(const string &n) const
{
	if (n != "") {
		if (graphicsState->namedMaterials.find(n) !=
			graphicsState->namedMaterials.end())
			return graphicsState->namedMaterials[n];
		LOG(LUX_ERROR,LUX_BADTOKEN) << "Couldn't find material named '" << n << "'";
	}
	return boost::shared_ptr<lux::Material>();
}

void lux::Context::Init() {
	// Dade - reinitialize
	aborted = false;
	terminated = false;
	currentApiState = STATE_OPTIONS_BLOCK;
	startRenderingAfterParse = true;
	inMotionBlock = false;
	luxCurrentRenderer = NULL;
	luxCurrentScene = NULL;
	curTransform = lux::Transform();
	namedCoordinateSystems.clear();
	renderOptions = new RenderOptions;
	graphicsState = new GraphicsState;
	pushedGraphicsStates.clear();
	pushedTransforms.clear();
	renderFarm = new RenderFarm(this);
	filmOverrideParams = NULL;
	shapeNo = 0;
}

void lux::Context::ResetFLM()
{
	if (luxCurrentScene != 0)
		luxCurrentScene->camera()->film->ClearBuffers();
}

void lux::Context::Free() {
	// Dade - free memory

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

void lux::Context::AddServer(const string &n) {
	if (!renderFarm->connect(n))
		return;
	
	// if this is the first server added during rendering, make sure update thread is started
	renderFarm->start(luxCurrentScene);
}

void lux::Context::RemoveServer(const RenderingServerInfo &rsi) {
	renderFarm->disconnect(rsi);

	// if this is the last server, make sure update thread is stopped
	renderFarm->stop();
}

void lux::Context::RemoveServer(const string &n) {
	renderFarm->disconnect(n);

	// if this is the last server, make sure update thread is stopped
	renderFarm->stop();
}

void lux::Context::ResetServer(const string &n, const string &p) {
	renderFarm->sessionReset(n, p);
}

u_int lux::Context::GetRenderingServersStatus(RenderingServerInfo *info, u_int maxInfoCount) {
	return renderFarm->getServersStatus(info, maxInfoCount);
}

void lux::Context::Cleanup() {
	renderFarm->send("luxCleanup");

	// API Cleanup
	if (currentApiState == STATE_UNINITIALIZED)
		LOG(LUX_ERROR,LUX_NOTSTARTED)<< "luxCleanup() called without luxInit().";
	else if (currentApiState == STATE_WORLD_BLOCK)
		LOG(LUX_ERROR,LUX_ILLSTATE)<< "luxCleanup() called while inside world block.";
	
	renderFarm->disconnectAll();

	// Dade - free memory
	Free();

	// Dade - reinitialize
	Init();
}

void lux::Context::Identity() {
	VERIFY_INITIALIZED_TRANSFORMS("Identity");
	renderFarm->send("luxIdentity");
	lux::Transform t;
	if (inMotionBlock)
		motionBlockTransforms.push_back(t);
	else
		curTransform = t;
}

void lux::Context::Translate(float dx, float dy, float dz) {
	VERIFY_INITIALIZED_TRANSFORMS("Translate");
	renderFarm->send("luxTranslate", dx, dy, dz);
	lux::Transform t = lux::Translate(Vector(dx, dy, dz));
	if (inMotionBlock)
		motionBlockTransforms.push_back(t);
	else
		curTransform = curTransform * t;
}

void lux::Context::Transform(float tr[16]) {
	VERIFY_INITIALIZED_TRANSFORMS("Transform");
	renderFarm->send("luxTransform", tr);
	::Transform t(Matrix4x4(tr[0], tr[4], tr[8], tr[12],
		tr[1], tr[5], tr[9], tr[13],
		tr[2], tr[6], tr[10], tr[14],
		tr[3], tr[7], tr[11], tr[15]));
	if (inMotionBlock)
		motionBlockTransforms.push_back(t);
	else
		curTransform = t;
}
void lux::Context::ConcatTransform(float tr[16]) {
	VERIFY_INITIALIZED_TRANSFORMS("ConcatTransform");
	renderFarm->send("luxConcatTransform", tr);
	::Transform t(Matrix4x4(tr[0], tr[4], tr[8], tr[12],
		tr[1], tr[5], tr[9], tr[13],
		tr[2], tr[6], tr[10], tr[14],
		tr[3], tr[7], tr[11], tr[15]));
	if (inMotionBlock)
		motionBlockTransforms.push_back(t);
	else
		curTransform = curTransform * t;
}
void lux::Context::Rotate(float angle, float dx, float dy, float dz) {
	VERIFY_INITIALIZED_TRANSFORMS("Rotate");
	renderFarm->send("luxRotate", angle, dx, dy, dz);
	::Transform t(::Rotate(angle, Vector(dx, dy, dz)));
	if (inMotionBlock)
		motionBlockTransforms.push_back(t);
	else
		curTransform = curTransform * t;
}
void lux::Context::Scale(float sx, float sy, float sz) {
	VERIFY_INITIALIZED_TRANSFORMS("Scale");
	renderFarm->send("luxScale", sx, sy, sz);
	::Transform t(::Scale(sx, sy, sz));
	if (inMotionBlock)
		motionBlockTransforms.push_back(t);
	else
		curTransform = curTransform * t;
}
void lux::Context::LookAt(float ex, float ey, float ez, float lx, float ly, float lz,
	float ux, float uy, float uz) {
	VERIFY_INITIALIZED_TRANSFORMS("LookAt");
	renderFarm->send("luxLookAt", ex, ey, ez, lx, ly, lz, ux, uy, uz);
	::Transform t(::LookAt(Point(ex, ey, ez), Point(lx, ly, lz),
		Vector(ux, uy, uz)));
	if (inMotionBlock)
		motionBlockTransforms.push_back(t);
	else
		curTransform = curTransform * t;
}
void lux::Context::CoordinateSystem(const string &n) {
	VERIFY_INITIALIZED("CoordinateSystem");
	renderFarm->send("luxCoordinateSystem", n);
	namedCoordinateSystems[n] = curTransform;
}
void lux::Context::CoordSysTransform(const string &n) {
	VERIFY_INITIALIZED_TRANSFORMS("CoordSysTransform");
	renderFarm->send("luxCoordSysTransform", n);
	if (namedCoordinateSystems.find(n) != namedCoordinateSystems.end()) {
		MotionTransform mt = namedCoordinateSystems[n];
		if (inMotionBlock) {
			if (mt.IsStatic())
				motionBlockTransforms.push_back(mt.StaticTransform());
			else {
				LOG(LUX_ERROR,LUX_NESTING) << "Cannot use motion coordinate system '" << n << "' inside Motion block, ignoring.";
			}
		} else{
			curTransform = mt;
		}
	} else {
		LOG(LUX_ERROR,LUX_SYNTAX) << "Coordinate system '" << n << "' unknown";
	}
}
void lux::Context::SetEpsilon(const float minValue, const float maxValue)
{
	VERIFY_INITIALIZED("SetEpsilon");
	renderFarm->send("luxSetEpsilon", minValue, maxValue);
	MachineEpsilon::SetMin(minValue);
	MachineEpsilon::SetMax(maxValue);
}
void lux::Context::StartRenderingAfterParse(const bool start) {
	VERIFY_INITIALIZED("StartRenderingAfterParse");
	// NOTE: this feature is not currently supported by network rendering
	startRenderingAfterParse = start;
}
void lux::Context::EnableDebugMode() {
    VERIFY_OPTIONS("EnableDebugMode");
    // Dade - I avoid to transmit the EnableDebugMode option to the renderFarm
    renderOptions->debugMode = true;
}

void lux::Context::DisableRandomMode() {
    VERIFY_OPTIONS("DisableRandomMode");
    // Slaves needs random seeds
    renderOptions->randomMode = false;
}

void lux::Context::PixelFilter(const string &n, const ParamSet &params) {
	VERIFY_OPTIONS("PixelFilter");
	renderFarm->send("luxPixelFilter", n, params);
	renderOptions->filterName = n;
	renderOptions->filterParams = params;
}
void lux::Context::Film(const string &type, const ParamSet &params) {
	VERIFY_OPTIONS("Film");
	// NOTE - luxFilm command doesn't cause "filename" file to be sent
	renderFarm->send("luxFilm", type, params);
	renderOptions->filmParams = params;
	renderOptions->filmName = type;
	if (filmOverrideParams)
		renderOptions->filmParams.Add(*filmOverrideParams);
}
void lux::Context::Sampler(const string &n, const ParamSet &params) {
	VERIFY_OPTIONS("Sampler");
	renderFarm->send("luxSampler", n, params);
	renderOptions->samplerName = n;
	renderOptions->samplerParams = params;
}
void lux::Context::Accelerator(const string &n, const ParamSet &params) {
	VERIFY_OPTIONS("Accelerator");
	renderFarm->send("luxAccelerator", n, params);
	renderOptions->acceleratorName = n;
	renderOptions->acceleratorParams = params;
}
void lux::Context::SurfaceIntegrator(const string &n, const ParamSet &params) {
	VERIFY_OPTIONS("SurfaceIntegrator");
	renderFarm->send("luxSurfaceIntegrator", n, params);
	renderOptions->surfIntegratorName = n;
	renderOptions->surfIntegratorParams = params;
}
void lux::Context::VolumeIntegrator(const string &n, const ParamSet &params) {
	VERIFY_OPTIONS("VolumeIntegrator");
	renderFarm->send("luxVolumeIntegrator", n, params);
	renderOptions->volIntegratorName = n;
	renderOptions->volIntegratorParams = params;
}
void lux::Context::Camera(const string &n, const ParamSet &params) {
	VERIFY_OPTIONS("Camera");
	renderFarm->send("luxCamera", n, params);
	renderOptions->cameraName = n;
	renderOptions->cameraParams = params;

	MotionTransform cameraTransform = curTransform;

	string endTransformName = renderOptions->cameraParams.FindOneString("endtransform", "");
	if (namedCoordinateSystems.find(endTransformName) != namedCoordinateSystems.end()) {
		if (curTransform.IsStatic()) {
			lux::Transform endTransform = namedCoordinateSystems[endTransformName].StaticTransform();

			// this isn't pretty, but it'll have to do until we can fully deprecate it
			vector<float> times;
			times.push_back(renderOptions->cameraParams.FindOneFloat("shutteropen", 0.f));
			times.push_back(renderOptions->cameraParams.FindOneFloat("shutterclose", 1.f));

			vector<lux::Transform> transforms;
			transforms.push_back(curTransform.StaticTransform());
			transforms.push_back(endTransform);

			cameraTransform = MotionTransform(times, transforms);
		} else {
			LOG(LUX_WARNING, LUX_CONSISTENCY) << "Both motion transform and endtransform specified for camera, ignoring endtransform";
		}
	} else if (endTransformName != "") {
		LOG(LUX_WARNING, LUX_CONSISTENCY) << "Invalid endtransform name for camera: '" << endTransformName << "'";
	}

	renderOptions->worldToCamera = cameraTransform;
	namedCoordinateSystems["camera"] = cameraTransform.GetInverse();
}
void lux::Context::WorldBegin() {
	VERIFY_OPTIONS("WorldBegin");
	renderFarm->send("luxWorldBegin");
	currentApiState = STATE_WORLD_BLOCK;
	curTransform = lux::Transform();
	namedCoordinateSystems["world"] = curTransform;
	shapeNo = 0;
}
void lux::Context::AttributeBegin() {
	VERIFY_WORLD("AttributeBegin");
	renderFarm->send("luxAttributeBegin");
	pushedGraphicsStates.push_back(*graphicsState);
	pushedTransforms.push_back(curTransform);
}
void lux::Context::AttributeEnd() {
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
void lux::Context::TransformBegin() {
	VERIFY_INITIALIZED("TransformBegin");
	renderFarm->send("luxTransformBegin");
	pushedTransforms.push_back(curTransform);
}
void lux::Context::TransformEnd() {
	VERIFY_INITIALIZED("TransformEnd");
	renderFarm->send("luxTransformEnd");
	if (!(pushedTransforms.size() > pushedGraphicsStates.size())) {
		LOG(LUX_ERROR,LUX_ILLSTATE)<< "Unmatched luxTransformEnd() encountered. Ignoring it.";
		return;
	}
	curTransform = pushedTransforms.back();
	pushedTransforms.pop_back();
}
void lux::Context::MotionBegin(u_int n, float *t) {
	VERIFY_INITIALIZED("MotionBegin");
	renderFarm->send("luxMotionBegin", n, t);
	motionBlockTimes.assign(t, t+n);
	motionBlockTransforms.clear();
	inMotionBlock = true;
}
void lux::Context::MotionEnd() {
	VERIFY_INITIALIZED_TRANSFORMS("MotionEnd");
	renderFarm->send("luxMotionEnd");
	if (!inMotionBlock) {
		LOG(LUX_ERROR,LUX_ILLSTATE)<< "Unmatched luxMotionEnd() encountered. Ignoring it.";
		return;
	}
	inMotionBlock = false;
	lux::MotionTransform motionTransform(motionBlockTimes, motionBlockTransforms);
	motionBlockTimes.clear();
	motionBlockTransforms.clear();
	if (!motionTransform.Valid()) {
		LOG(LUX_WARNING,LUX_CONSISTENCY)<< "Invalid Motion block, ignoring it.";
		return;
	}
	curTransform = curTransform * motionTransform;
}
void lux::Context::Texture(const string &n, const string &type,
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
			MakeFloatTexture(texname, curTransform.StaticTransform(), params));
		if (ft)
			graphicsState->floatTextures[n] = ft;
	} else if (type == "color") {
		// Create _color_ texture and store in _colorTextures_
		if (graphicsState->colorTextures.find(n) !=
			graphicsState->colorTextures.end()) {
			LOG(LUX_WARNING,LUX_SYNTAX) << "Color texture '" << n << "' being redefined.";
		}
		boost::shared_ptr<lux::Texture<SWCSpectrum> > st(
			MakeSWCSpectrumTexture(texname, curTransform.StaticTransform(), params));
		if (st)
			graphicsState->colorTextures[n] = st;
	} else if (type == "fresnel") {
		// Create _fresnel_ texture and store in _fresnelTextures_
		if (graphicsState->fresnelTextures.find(n) !=
			graphicsState->fresnelTextures.end()) {
			LOG(LUX_WARNING,LUX_SYNTAX) << "Fresnel texture '" << n << "' being redefined.";
		}
		boost::shared_ptr<lux::Texture<FresnelGeneral> > fr(
			MakeFresnelTexture(texname, curTransform.StaticTransform(), params));
		if (fr)
			graphicsState->fresnelTextures[n] = fr;
	} else {
		LOG(LUX_ERROR,LUX_SYNTAX) << "Texture type '" << type << "' unknown";
	}
}
void lux::Context::Material(const string &n, const ParamSet &params) {
	VERIFY_WORLD("Material");
	renderFarm->send("luxMaterial", n, params);
	graphicsState->material = MakeMaterial(n, curTransform.StaticTransform(), params);
}

void lux::Context::MakeNamedMaterial(const string &n, const ParamSet &_params)
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
	graphicsState->namedMaterials[n] = MakeMaterial(type, curTransform.StaticTransform(),
		params);
}

void lux::Context::MakeNamedVolume(const string &id, const string &name,
	const ParamSet &params)
{
	VERIFY_WORLD("MakeNamedVolume");
	renderFarm->send("luxMakeNamedVolume", id, name, params);
	if (graphicsState->namedVolumes.find(id) !=
		graphicsState->namedVolumes.end()) {
		LOG(LUX_WARNING, LUX_SYNTAX) << "Named volume '" << id <<
			"' being redefined.";
	}
	graphicsState->namedVolumes[id] = MakeVolume(name, curTransform.StaticTransform(), params);
}

void lux::Context::NamedMaterial(const string &n) {
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

void lux::Context::LightGroup(const string &n, const ParamSet &params)
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


void lux::Context::LightSource(const string &n, const ParamSet &params) {
	VERIFY_WORLD("LightSource");
	renderFarm->send("luxLightSource", n, params);
	u_int lg = GetLightGroup();

	if (n == "sunsky") {
		//SunSky light - create both sun & sky lightsources

		// Stop the sun complaining about unused sky parameters
		ParamSet sunparams(params);
		sunparams.EraseFloat("horizonbrightness");
		sunparams.EraseFloat("horizonsize");
		sunparams.EraseFloat("sunhalobrightness");
		sunparams.EraseFloat("sunhalosize");
		sunparams.EraseFloat("backscattering");
		sunparams.EraseFloat("aconst");
		sunparams.EraseFloat("bconst");
		sunparams.EraseFloat("cconst");
		sunparams.EraseFloat("dconst");
		sunparams.EraseFloat("econst");

		boost::shared_ptr<Light> lt_sun(MakeLight("sun", curTransform.StaticTransform(), sunparams));
		if (!lt_sun) {
			LOG(LUX_ERROR,LUX_SYNTAX)<< "luxLightSource: light type sun unknown.";
			graphicsState->currentLightPtr0.reset();
		} else {
			lt_sun->group = lg;
			lt_sun->SetVolume(graphicsState->exterior);
			if (renderOptions->currentLightInstance)
				renderOptions->currentLightInstance->push_back(boost::shared_ptr<Light>(lt_sun));
			else
				renderOptions->lights.push_back(lt_sun);
			graphicsState->currentLight = n;
			graphicsState->currentLightPtr0 = lt_sun;
		}

		// Stop the sky complaining about unused sun params
		ParamSet skyparams(params);
		skyparams.EraseFloat("relsize");

		boost::shared_ptr<Light> lt_sky(MakeLight("sky", curTransform.StaticTransform(), skyparams));
		if (!lt_sky) {
			LOG(LUX_ERROR,LUX_SYNTAX)<< "luxLightSource: light type sky unknown.";
			graphicsState->currentLightPtr1.reset();
		} else {
			lt_sky->group = lg;
			lt_sky->SetVolume(graphicsState->exterior);
			if (renderOptions->currentLightInstance)
				renderOptions->currentLightInstance->push_back(boost::shared_ptr<Light>(lt_sky));
			else
				renderOptions->lights.push_back(lt_sky);
			graphicsState->currentLight = n;
			graphicsState->currentLightPtr1 = lt_sky;
		}
	} else if (n == "sunsky2") {
		//SunSky2 light - create both sun & sky2 lightsources

		ParamSet sunparams(params);

		boost::shared_ptr<Light> lt_sun(MakeLight("sun", curTransform.StaticTransform(), sunparams));
		if (!lt_sun) {
			LOG(LUX_ERROR,LUX_SYNTAX)<< "luxLightSource: light type sun unknown.";
			graphicsState->currentLightPtr0.reset();
		} else {
			lt_sun->group = lg;
			lt_sun->SetVolume(graphicsState->exterior);
			if (renderOptions->currentLightInstance)
				renderOptions->currentLightInstance->push_back(boost::shared_ptr<Light>(lt_sun));
			else
				renderOptions->lights.push_back(lt_sun);
			graphicsState->currentLight = n;
			graphicsState->currentLightPtr0 = lt_sun;
		}

		// Stop the sky complaining about unused sun params
		ParamSet skyparams(params);
		skyparams.EraseFloat("relsize");

		boost::shared_ptr<Light> lt_sky(MakeLight("sky2", curTransform.StaticTransform(), skyparams));
		if (!lt_sky) {
			LOG(LUX_ERROR,LUX_SYNTAX)<< "luxLightSource: light type sky2 unknown.";
			graphicsState->currentLightPtr1.reset();
		} else {
			lt_sky->group = lg;
			lt_sky->SetVolume(graphicsState->exterior);
			if (renderOptions->currentLightInstance)
				renderOptions->currentLightInstance->push_back(boost::shared_ptr<Light>(lt_sky));
			else
				renderOptions->lights.push_back(lt_sky);
			graphicsState->currentLight = n;
			graphicsState->currentLightPtr1 = lt_sky;
		}
	} else {
		// other lightsource type
		boost::shared_ptr<Light> lt(MakeLight(n, curTransform.StaticTransform(), params));
		if (!lt) {
			LOG(LUX_ERROR,LUX_SYNTAX) << "luxLightSource: light type '" << n << "' unknown";
		} else {
			lt->group = lg;
			lt->SetVolume(graphicsState->exterior);
			if (renderOptions->currentLightInstance)
				renderOptions->currentLightInstance->push_back(boost::shared_ptr<Light>(lt));
			else
				renderOptions->lights.push_back(lt);
			graphicsState->currentLight = n;
			graphicsState->currentLightPtr0 = lt;
			graphicsState->currentLightPtr1.reset();
		}
	}
}

void lux::Context::AreaLightSource(const string &n, const ParamSet &params) {
	VERIFY_WORLD("AreaLightSource");
	renderFarm->send("luxAreaLightSource", n, params);
	graphicsState->areaLight = n;
	graphicsState->areaLightParams = params;
}

void lux::Context::PortalShape(const string &n, const ParamSet &params) {
	VERIFY_WORLD("PortalShape");
	renderFarm->send("luxPortalShape", n, params);
	boost::shared_ptr<Primitive> sh(MakeShape(n, curTransform.StaticTransform(),
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

void lux::Context::Shape(const string &n, const ParamSet &params) {
	VERIFY_WORLD("Shape");
	renderFarm->send("luxShape", n, params);
	const u_int sIdx = shapeNo++;
	u_int nItems;
	const string *sn = params.FindString("name", &nItems);
	if (!sn || *sn == "") {
		// generate name based on shape type and declaration index
		std::stringstream ss("");
		ss << "#" << sIdx << " (" << n << ")";
		const string sname = ss.str();
		const_cast<ParamSet &>(params).AddString("name", &sname);
	} else if (sn) {
		const string sname = "'" + *sn + "'";
		const_cast<ParamSet &>(params).AddString("name", &sname);
	}
	boost::shared_ptr<lux::Shape> sh(MakeShape(n, curTransform.StaticTransform(),
		graphicsState->reverseOrientation, params));
	if (!sh)
		return;
	params.ReportUnused();

	// Lotus - Set the material
	if (graphicsState->material)
		sh->SetMaterial(graphicsState->material);
	else {
		boost::shared_ptr<lux::Material> m(MakeMaterial("matte",
			curTransform.StaticTransform(), ParamSet()));
		sh->SetMaterial(m);
	}
	sh->SetExterior(graphicsState->exterior);
	sh->SetInterior(graphicsState->interior);

	// Create primitive and add to scene or current instance
	if (renderOptions->currentInstanceRefined) {
		if (graphicsState->areaLight != "") {
			u_int lg = GetLightGroup();
			boost::shared_ptr<AreaLight> area(MakeAreaLight(graphicsState->areaLight,
				curTransform.StaticTransform(),
				graphicsState->areaLightParams, sh));
			if (area) {
				area->group = lg;
				area->SetVolume(graphicsState->exterior); //unused
			}
			// Lotus - add a decorator to set the arealight field
			boost::shared_ptr<Primitive> pr(sh);
			boost::shared_ptr<AreaLightPrimitive> prim(new AreaLightPrimitive(pr, area));
			vector<boost::shared_ptr<AreaLightPrimitive> > aList;
			if (!prim->CanIntersect()) {
				// When refining the primitive, there's no way
				// To tell that all refined primitives will be
				// AreaLightPrimitive, so do some pointer tricks
				// to add the refined primitives.
				vector<boost::shared_ptr<Primitive> > tmp;
				prim->Refine(tmp,
					PrimitiveRefinementHints(true), prim);
				for (u_int i = 0; i < tmp.size(); ++i)
					aList.push_back(*((boost::shared_ptr<AreaLightPrimitive> *)(&(tmp[i]))));
			} else
				aList.push_back(prim);
			renderOptions->currentAreaLightInstance->push_back(aList);
		} else {
			renderOptions->currentInstanceSource->push_back(sh);
			if (!sh->CanIntersect())
				sh->Refine(*(renderOptions->currentInstanceRefined),
					PrimitiveRefinementHints(false), sh);
			else
				renderOptions->currentInstanceRefined->push_back(sh);
		}
	} else if (graphicsState->areaLight != "") {
		u_int lg = GetLightGroup();
		boost::shared_ptr<AreaLight> area(MakeAreaLight(graphicsState->areaLight,
			curTransform.StaticTransform(),
			graphicsState->areaLightParams, sh));
		if (area) {
			area->group = lg;
			area->SetVolume(graphicsState->exterior); //unused
		}
		// Lotus - add a decorator to set the arealight field
		boost::shared_ptr<Primitive> pr(sh);
		boost::shared_ptr<Primitive> prim(new AreaLightPrimitive(pr,
			area));
		renderOptions->primitives.push_back(prim);
		// Add area light for primitive to light vector
		renderOptions->lights.push_back(area);
	} else
		renderOptions->primitives.push_back(sh);
}
void lux::Context::Renderer(const string &n, const ParamSet &params) {
	VERIFY_OPTIONS("Renderer");
	renderFarm->send("luxRenderer", n, params);
	renderOptions->rendererName = n;
	renderOptions->rendererParams = params;
}
void lux::Context::ReverseOrientation() {
	VERIFY_WORLD("ReverseOrientation");
	renderFarm->send("luxReverseOrientation");
	graphicsState->reverseOrientation = !graphicsState->reverseOrientation;
}
void lux::Context::Volume(const string &n, const ParamSet &params) {
	VERIFY_WORLD("Volume");
	renderFarm->send("luxVolume", n, params);
	Region *vr = MakeVolumeRegion(n, curTransform.StaticTransform(), params);
	if (vr)
		renderOptions->volumeRegions.push_back(vr);
}
void lux::Context::Exterior(const string &n) {
	VERIFY_WORLD("Exterior");
	renderFarm->send("luxExterior", n);
	if (n == "")
		graphicsState->exterior = boost::shared_ptr<lux::Volume>();
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
void lux::Context::Interior(const string &n) {
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
void lux::Context::ObjectBegin(const string &n) {
	VERIFY_WORLD("ObjectBegin");
	renderFarm->send("luxObjectBegin", n);
	AttributeBegin();
	if (renderOptions->currentInstanceRefined) {
		LOG(LUX_ERROR,LUX_NESTING) <<
			"ObjectBegin called inside of instance definition";
		return;
	}
	renderOptions->instancesSource[n] = vector<boost::shared_ptr<Primitive> >();
	renderOptions->instancesRefined[n] = vector<boost::shared_ptr<Primitive> >();
	renderOptions->currentInstanceSource = &renderOptions->instancesSource[n];
	renderOptions->currentInstanceRefined = &renderOptions->instancesRefined[n];
	renderOptions->lightInstances[n] = vector<boost::shared_ptr<Light> >();
	renderOptions->currentLightInstance = &renderOptions->lightInstances[n];
	renderOptions->areaLightInstances[n] = vector<vector<boost::shared_ptr<AreaLightPrimitive> > >();
	renderOptions->currentAreaLightInstance = &renderOptions->areaLightInstances[n];
}
void lux::Context::ObjectEnd() {
	VERIFY_WORLD("ObjectEnd");
	renderFarm->send("luxObjectEnd");
	if (!renderOptions->currentInstanceRefined) {
		LOG(LUX_ERROR,LUX_NESTING) <<
			"ObjectEnd called outside of instance definition";
		return;
	}
	renderOptions->currentInstanceSource = NULL;
	renderOptions->currentInstanceRefined = NULL;
	renderOptions->currentLightInstance = NULL;
	renderOptions->currentAreaLightInstance = NULL;
	AttributeEnd();
}
void lux::Context::ObjectInstance(const string &n) {
	VERIFY_WORLD("ObjectInstance");
	renderFarm->send("luxObjectInstance", n);
	// Object instance error checking
	if (renderOptions->instancesRefined.find(n) == renderOptions->instancesRefined.end()) {
		LOG(LUX_ERROR,LUX_BADTOKEN) << "Unable to find instance named '" << n << "'";
		return;
	}

	vector<boost::shared_ptr<Primitive> > &inSource = renderOptions->instancesSource[n];
	vector<boost::shared_ptr<Primitive> > &in = renderOptions->instancesRefined[n];
	if (renderOptions->currentInstanceRefined == &in) {
		LOG(LUX_ERROR,LUX_NESTING) << "ObjectInstance '" << n << "' self reference";
		return;
	}
	for (u_int i = 0; i < renderOptions->areaLightInstances[n].size(); ++i) {
		if (renderOptions->areaLightInstances[n][i].size() == 0)
			continue;
		boost::shared_ptr<AreaLight> li(renderOptions->areaLightInstances[n][i][0]->GetAreaLight());
		boost::shared_ptr<AreaLight> l;
		if (curTransform.IsStatic())
			l.reset(new InstanceAreaLight(curTransform.StaticTransform(), li));
		else
			l.reset(new MotionAreaLight(curTransform.GetMotionSystem(), li));
		// Add the instanced light only if not inside Object block
		if (!renderOptions->currentAreaLightInstance)
			renderOptions->lights.push_back(l);
		// Add the instanced primitives
		if (renderOptions->currentAreaLightInstance)
			renderOptions->currentAreaLightInstance->push_back(vector<boost::shared_ptr<AreaLightPrimitive> >());
		for (u_int j = 0; j < renderOptions->areaLightInstances[n][i].size(); ++j) {
			boost::shared_ptr<Primitive> pi(renderOptions->areaLightInstances[n][i][j]->GetPrimitive());
			AreaLightPrimitive *ap;
			vector<boost::shared_ptr<Primitive> > source;
			source.push_back(pi);
			if (curTransform.IsStatic()) {
				boost::shared_ptr<Primitive> p(new InstancePrimitive(source,
					pi, curTransform.StaticTransform(),
					graphicsState->material,
					graphicsState->exterior,
					graphicsState->interior));
				ap = new AreaLightPrimitive(p, l);
			} else {
				boost::shared_ptr<Primitive> p(new MotionPrimitive(source,
					pi, curTransform.GetMotionSystem(),
					graphicsState->material,
					graphicsState->exterior,
					graphicsState->interior));
				ap = new AreaLightPrimitive(p, l);
			}
			if (renderOptions->currentAreaLightInstance)
				renderOptions->currentAreaLightInstance->back().push_back(boost::shared_ptr<AreaLightPrimitive>(ap));
			else {
				// Area lights are already fully refined
				boost::shared_ptr<Primitive> sap(ap);
				inSource.push_back(sap);
				renderOptions->primitives.push_back(sap);
			}
		}
	}
	if (in.size() != 0) {
		if (in.size() > 1 || !in[0]->CanIntersect()) {
			// Refine instance _Primitive_s and create aggregate
			boost::shared_ptr<Primitive> accel(
				MakeAccelerator(renderOptions->acceleratorName,
				in, renderOptions->acceleratorParams));
			if (!accel)
				accel = MakeAccelerator("kdtree", in,
					ParamSet());
			if (!accel)
				LOG(LUX_SEVERE,LUX_BUG) <<
					"Unable to find \"kdtree\" accelerator";
			in.clear();
			in.push_back(accel);
		}

		boost::shared_ptr<Primitive> o;
		if (curTransform.IsStatic()) {
			o = boost::shared_ptr<Primitive>(new InstancePrimitive(inSource, in[0],
				curTransform.StaticTransform(),
				graphicsState->material,
				graphicsState->exterior,
				graphicsState->interior));
		} else {
			o = boost::shared_ptr<Primitive>(new MotionPrimitive(inSource, in[0],
				curTransform.GetMotionSystem(),
				graphicsState->material,
				graphicsState->exterior,
				graphicsState->interior));
		}
		if (renderOptions->currentInstanceRefined) {
			// Instance of instances case
			renderOptions->currentInstanceRefined->push_back(o);
		} else
			renderOptions->primitives.push_back(o);
	}
	vector<boost::shared_ptr<Light> > &li = renderOptions->lightInstances[n];
	for (u_int i = 0; i < li.size(); ++i) {
		boost::shared_ptr<Light> l;
		if (curTransform.IsStatic())
			l.reset(new InstanceLight(curTransform.StaticTransform(),
				li[i]));
		else
			l.reset(new MotionLight(curTransform.GetMotionSystem(),
				li[i]));
		if (renderOptions->currentLightInstance)
			renderOptions->currentLightInstance->push_back(boost::shared_ptr<Light>(l));
		else
			renderOptions->lights.push_back(l);
	}
}
void lux::Context::PortalInstance(const string &n) {
	VERIFY_WORLD("PortalInstance");
	renderFarm->send("luxPortalInstance", n);
	// Portal instance error checking
	if (renderOptions->instancesRefined.find(n) == renderOptions->instancesRefined.end()) {
		LOG(LUX_ERROR,LUX_BADTOKEN) << "Unable to find instance named '" << n << "'";
		return;
	}
	vector<boost::shared_ptr<Primitive> > &in = renderOptions->instancesRefined[n];
	if (renderOptions->currentInstanceRefined == &in) {
		LOG(LUX_ERROR,LUX_NESTING) << "PortalInstance '" << n << "' self reference";
		return;
	}

	if (graphicsState->currentLight == "")
		return;

	for (size_t i = 0; i < in.size(); ++i) {
		if (graphicsState->currentLightPtr0)
			graphicsState->currentLightPtr0->AddPortalShape(in[i]);

		if (graphicsState->currentLightPtr1)
			graphicsState->currentLightPtr1->AddPortalShape(in[i]);
	}
}
void lux::Context::MotionInstance(const string &n, float startTime, float endTime, const string &toTransform) {
	VERIFY_WORLD("MotionInstance");
	renderFarm->send("luxMotionInstance", n, startTime, endTime, toTransform);
	LOG(LUX_WARNING, LUX_SYNTAX) << "MotionInstance '" << n << "' is deprecated, use a MotionBegin/MotionEnd block with an ObjectInstance inside";
	// Object instance error checking
	if (renderOptions->instancesRefined.find(n) == renderOptions->instancesRefined.end()) {
		LOG(LUX_ERROR,LUX_BADTOKEN) << "Unable to find instance named '" << n << "'";
		return;
	}
	vector<boost::shared_ptr<Primitive> > &inSource = renderOptions->instancesSource[n];
	vector<boost::shared_ptr<Primitive> > &in = renderOptions->instancesRefined[n];
	if (renderOptions->currentInstanceRefined == &in) {
		LOG(LUX_ERROR,LUX_NESTING) << "MotionInstance '" << n << "' self reference";
		return;
	}
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
	lux::Transform endTransform;
	if (namedCoordinateSystems.find(toTransform) != namedCoordinateSystems.end())
		endTransform = namedCoordinateSystems[toTransform].StaticTransform();
	else {
		LOG(LUX_SEVERE,LUX_BUG)<< "Unable to find coordinate system named '" << n << "' for MotionInstance";
	}

	vector<float> times;
	times.push_back(startTime);
	times.push_back(endTime);

	vector<lux::Transform> transforms;
	transforms.push_back(curTransform.StaticTransform());
	transforms.push_back(endTransform);

	MotionSystem ms(times, transforms);

	boost::shared_ptr<Primitive> o(new MotionPrimitive(inSource, in[0], ms, graphicsState->material,
		graphicsState->exterior, graphicsState->interior));
	renderOptions->primitives.push_back(o);
}

void lux::Context::WorldEnd() {
	VERIFY_WORLD("WorldEnd");
	// renderfarm will flush when detecting WorldEnd
	renderFarm->send("luxWorldEnd");

	// Dade - get the lock, other thread can use this lock to wait the end
	// of the rendering
	boost::mutex::scoped_lock lock(renderingMutex);

	// Ensure there are no pushed graphics states
	while (pushedGraphicsStates.size()) {
		LOG(LUX_WARNING,LUX_NESTING)<<"Missing end to luxAttributeBegin()";
		pushedGraphicsStates.pop_back();
		pushedTransforms.pop_back();
	}

	// Clean up
	currentApiState = STATE_OPTIONS_BLOCK;
	curTransform = lux::Transform();
	namedCoordinateSystems.erase(namedCoordinateSystems.begin(),
		namedCoordinateSystems.end());

	if (startRenderingAfterParse)
		ParseEnd();
}

void lux::Context::ParseEnd() {
	if (!terminated) {
		// Create scene and render
		luxCurrentScene = renderOptions->MakeScene();
		if (luxCurrentScene && !terminated) {
			luxCurrentScene->camera()->SetVolume(graphicsState->exterior);

			luxCurrentRenderer = renderOptions->MakeRenderer();

			if (luxCurrentRenderer && !terminated) {
				// start the network rendering updater thread
				renderFarm->start(luxCurrentScene);

				luxCurrentRenderer->Render(luxCurrentScene);

				// Signal that rendering is done, so any slaves connected
				// after this won't start rendering
				activeContext->renderFarm->renderingDone();

				// Stop the film updating thread etc
				activeContext->renderFarm->stop();

				if (static_cast<u_int>((*(activeContext->renderFarm))["slaveNodeCount"].IntValue()) > 0) {
					// Update the film for the last time
					if (!aborted)
						activeContext->renderFarm->updateFilm(luxCurrentScene);
					// Disconnect from all servers
					activeContext->renderFarm->disconnectAll();
				}

				// Store final image
				if (!aborted)
					luxCurrentScene->camera()->film->WriteImage((ImageType)(IMAGE_FINAL|IMAGE_FILE_ALL|IMAGE_FRAMEBUFFER));
			}
		}
	}
}

Scene *lux::Context::RenderOptions::MakeScene() const {
	// Create scene objects from API settings
	lux::Filter *filter = MakeFilter(filterName, filterParams);
	lux::Film *film = (!filter) ? NULL : MakeFilm(filmName, filmParams, filter);
	lux::Camera *camera = (!film) ? NULL : MakeCamera(cameraName, worldToCamera.GetMotionSystem(), cameraParams, film);
	lux::Sampler *sampler = (!film) ? NULL : MakeSampler(samplerName, samplerParams, film);
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
	currentInstanceSource = NULL;
	currentInstanceRefined = NULL;
	currentLightInstance = NULL;
	instancesSource.clear();
	instancesRefined.clear();
	lightInstances.clear();

	// Set a fixed seed for animations or debugging
	if (debugMode || !randomMode)
		ret->seedBase = 1001;

	return ret;
}

Renderer *lux::Context::RenderOptions::MakeRenderer() const {
	return lux::MakeRenderer(rendererName, rendererParams);
}

// Load/save FLM file
void lux::Context::LoadFLM(const string &flmFileName) {
	// Create the film
	lux::Film* flm = FlexImageFilm::CreateFilmFromFLM(flmFileName);
	if (!flm) {
		LOG(LUX_SEVERE,LUX_BUG)<< "Unable to create film";
		return;
	}
	// Update lux::Context
	MotionSystem dummyTransform;
	ParamSet dummyParams;
	lux::Camera *cam = MakeCamera("perspective", dummyTransform, dummyParams, flm);
	if (!cam) {
		LOG(LUX_SEVERE,LUX_BUG)<< "Unable to create dummy camera";
		delete flm;
		return;
	}
	luxCurrentScene = new Scene(cam);
	luxCurrentScene->SetReady();
}
void lux::Context::SaveFLM(const string &flmFileName) {
	luxCurrentScene->camera()->film->WriteFilmToFile(flmFileName);
}

void lux::Context::LoadFLMFromStream(char* buffer, unsigned int bufSize, const string &name){
	// Create the film
	lux::Film* flm = FlexImageFilm::CreateFilmFromFLMFromStream(buffer, bufSize, name);
	if (!flm) {
		LOG(LUX_SEVERE,LUX_BUG)<< "Unable to create film";
		return;
	}
	// Update context
	MotionSystem dummyTransform;
	ParamSet dummyParams;
	lux::Camera *cam = MakeCamera("perspective", dummyTransform, dummyParams, flm);
	if (!cam) {
		LOG(LUX_SEVERE,LUX_BUG)<< "Unable to create dummy camera";
		delete flm;
		return;
	}
	luxCurrentScene = new Scene(cam);
	luxCurrentScene->SetReady();
}

unsigned char* lux::Context::SaveFLMToStream(unsigned int& size){
	return luxCurrentScene->SaveFLMToStream(size);
}

double lux::Context::UpdateFilmFromStream(std::basic_istream<char> &is){
	return luxCurrentScene->UpdateFilmFromStream(is);
}

// Save current film to OpenEXR image
bool lux::Context::SaveEXR(const string &name, bool useHalfFloat, bool includeZBuffer, int compressionType, bool tonemapped) {
	return luxCurrentScene->SaveEXR(name, useHalfFloat, includeZBuffer, compressionType, tonemapped);
}

void lux::Context::OverrideResumeFLM(const string &flmFileName) {
	if (!filmOverrideParams) {
		filmOverrideParams = new ParamSet();
	}
	const bool boolTrue = true;
	const bool boolFalse = false;
	filmOverrideParams->AddBool("write_resume_flm", &boolTrue);
	filmOverrideParams->AddBool("restart_resume_flm", &boolFalse);
	OverrideFilename(flmFileName);
}

void lux::Context::OverrideFilename(const string &filename) {
	if (!filmOverrideParams) {
		filmOverrideParams = new ParamSet();
	}
	if (filename != "") {
		boost::filesystem::path filePath(filename);

		const string basename = filePath.replace_extension("").string();
		filmOverrideParams->AddString("filename", &basename);
	}
}

//user interactive thread functions
void lux::Context::Resume() {
	luxCurrentRenderer->Resume();
}

void lux::Context::Pause() {
	luxCurrentRenderer->Pause();
}

void lux::Context::SetHaltSamplesPerPixel(int haltspp, bool haveEnoughSamplesPerPixel,
	bool suspendThreadsWhenDone) {
	lux::Film *film = luxCurrentScene->camera()->film;
	film->haltSamplesPerPixel = haltspp;
	film->enoughSamplesPerPixel = haveEnoughSamplesPerPixel;
	luxCurrentRenderer->SuspendWhenDone(suspendThreadsWhenDone);
}

Renderer::RendererType lux::Context::GetRendererType() const {
	assert (luxCurrentRenderer != NULL);
	return luxCurrentRenderer->GetType();
}

void lux::Context::Wait() {
    boost::mutex::scoped_lock lock(renderingMutex);
}

void lux::Context::Exit() {
	if (static_cast<u_int>((*(activeContext->renderFarm))["slaveNodeCount"].IntValue()) > 0) {
		// Dade - stop the render farm too
		activeContext->renderFarm->stop();
		// Dade - update the film for the last time
		if (!aborted)
			activeContext->renderFarm->updateFilm(luxCurrentScene);
		// Dade - disconnect from all servers
		activeContext->renderFarm->disconnectAll();
	}
	
	terminated = true;
	if (luxCurrentScene)
		// set this before Renderer::Terminate() as that call is blocking
		luxCurrentScene->terminated = true;

	// Reset Dynamic Epsilon values
	MachineEpsilon::SetMin(DEFAULT_EPSILON_MIN);
	MachineEpsilon::SetMax(DEFAULT_EPSILON_MAX);

	if (luxCurrentRenderer)
		luxCurrentRenderer->Terminate();
}

void lux::Context::Abort() {
	aborted = true;
	Exit();
}

//controlling number of threads
u_int lux::Context::AddThread() {
	const vector<RendererHostDescription *> &hosts = luxCurrentRenderer->GetHostDescs();

	RendererDeviceDescription *desc = hosts[0]->GetDeviceDescs()[0];
	desc->SetUsedUnitsCount(desc->GetUsedUnitsCount() + 1);

	return desc->GetUsedUnitsCount();
}

void lux::Context::RemoveThread() {
	const vector<RendererHostDescription *> &hosts = luxCurrentRenderer->GetHostDescs();

	RendererDeviceDescription *desc = hosts[0]->GetDeviceDescs()[0];
	desc->SetUsedUnitsCount(max(desc->GetUsedUnitsCount() - 1, 1u));
}

//framebuffer access
void lux::Context::UpdateFramebuffer() {
	luxCurrentScene->UpdateFramebuffer();
}

unsigned char* lux::Context::Framebuffer() {
	return luxCurrentScene->GetFramebuffer();
}

float* lux::Context::FloatFramebuffer() {
	return luxCurrentScene->GetFloatFramebuffer();
}

float* lux::Context::AlphaBuffer() {
	return luxCurrentScene->GetAlphaBuffer();
}

float* lux::Context::ZBuffer() {
	return luxCurrentScene->GetZBuffer();
}

//histogram access
void lux::Context::GetHistogramImage(unsigned char *outPixels, u_int width,
	u_int height, int options)
{
	luxCurrentScene->GetHistogramImage(outPixels, width, height, options);
}

// Parameter Access functions
void lux::Context::SetParameterValue(luxComponent comp, luxComponentParameters param,
	double value, u_int index)
{
	luxCurrentScene->SetParameterValue(comp, param, value, index);
}
double lux::Context::GetParameterValue(luxComponent comp,
	luxComponentParameters param, u_int index)
{
	return luxCurrentScene->GetParameterValue(comp, param, index);
}
double lux::Context::GetDefaultParameterValue(luxComponent comp,
	luxComponentParameters param, u_int index)
{
	return luxCurrentScene->GetDefaultParameterValue(comp, param, index);
}
void lux::Context::SetStringParameterValue(luxComponent comp,
	luxComponentParameters param, const string& value, u_int index)
{
	return luxCurrentScene->SetStringParameterValue(comp, param, value,
		index);
}
string lux::Context::GetStringParameterValue(luxComponent comp,
	luxComponentParameters param, u_int index)
{
	return luxCurrentScene->GetStringParameterValue(comp, param, index);
}
string lux::Context::GetDefaultStringParameterValue(luxComponent comp,
	luxComponentParameters param, u_int index)
{
	return luxCurrentScene->GetDefaultStringParameterValue(comp, param,
		index);
}

u_int lux::Context::GetLightGroup() {
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
			graphicsState->currentLightGroup = "";
			lg = GetLightGroup();
		}
	}
	return lg;
}

double lux::Context::Statistics(const string &statName) {
	if (statName == "sceneIsReady")
		return (luxCurrentScene != NULL && luxCurrentScene->IsReady() &&
			!luxCurrentScene->IsFilmOnly());
	else if (statName == "filmIsReady")
		return (luxCurrentScene != NULL &&
			luxCurrentScene->IsFilmOnly());
	else if (statName == "terminated")
		return terminated;
	else
		return 0;
}

void lux::Context::SceneReady() {
	//luxCurrentSceneReady = true;
	luxCurrentScene->SetReady();
}

void lux::Context::UpdateStatisticsWindow() {
	if (luxCurrentRenderer)
		luxCurrentRenderer->rendererStatistics->updateStatisticsWindow();
}

bool lux::Context::IsRendering() {
	return luxCurrentRenderer != NULL && luxCurrentRenderer->GetState() == Renderer::RUN;
}

void lux::Context::WriteFilmToStream(std::basic_ostream<char> &stream) {
	luxCurrentScene->camera()->film->WriteFilmToStream(stream);
}

void lux::Context::WriteFilmToStream(std::basic_ostream<char> &stream, bool directWrite) {
	luxCurrentScene->camera()->film->WriteFilmToStream(stream, true, false, directWrite);
}

void lux::Context::UpdateFilmFromNetwork() {
	renderFarm->updateFilm(luxCurrentScene);
}

void lux::Context::UpdateLogFromNetwork() {
	renderFarm->updateLog();
}

void lux::Context::UpdateNetworkNoiseAwareMap() {
	// Transmit the new map to the slaves
	renderFarm->updateNoiseAwareMap();
}

void lux::Context::SetNoiseAwareMap(const float *map) {
	luxCurrentScene->camera()->film->SetNoiseAwareMap(map);

	// Transmit the new map to the slaves
	renderFarm->updateNoiseAwareMap();
}

float *lux::Context::GetNoiseAwareMap() {
	return luxCurrentScene->camera()->film->GetNoiseAwareMap();
}

void lux::Context::SetUserSamplingMap(const float *map) {
	luxCurrentScene->camera()->film->SetUserSamplingMap(map);

	// Transmit the new map to the slaves
	renderFarm->updateUserSamplingMap();
}

float *lux::Context::GetUserSamplingMap() {
	return luxCurrentScene->camera()->film->GetUserSamplingMap();
}
