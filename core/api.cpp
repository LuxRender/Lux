/***************************************************************************
 *   Copyright (C) 1998-2007 by authors (see AUTHORS.txt )                 *
 *                                                                         *
 *   This file is part of Lux Renderer.                                    *
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

// api.cpp*
#include "api.h"
#include "paramset.h"
#include "color.h"
#include "scene.h"
#include "film.h"
#include "dynload.h"
#include "volume.h"
#include <string>
#include <map>
#include <boost/date_time/posix_time/posix_time.hpp>
using std::map;
#if (_MSC_VER >= 1400) // NOBOOK
#include <stdio.h>     // NOBOOK
#define snprintf _snprintf // NOBOOK
#endif // NOBOOK

//jromang : here is the 'current' scene (we will need to replace that by a context
Scene *luxCurrentScene=NULL;

// API Local Classes
struct RenderOptions {
	// RenderOptions Public Methods
	RenderOptions();
	Scene *MakeScene() const;
	// RenderOptions Public Data
	string FilterName;
	ParamSet FilterParams;
	string FilmName;
	ParamSet FilmParams;
	string SamplerName;
	ParamSet SamplerParams;
	string AcceleratorName;
	ParamSet AcceleratorParams;
	string SurfIntegratorName, VolIntegratorName;
	ParamSet SurfIntegratorParams, VolIntegratorParams;
	string CameraName;
	ParamSet CameraParams;
	Transform WorldToCamera;
	bool gotSearchPath;
	mutable vector<Light *> lights;
	mutable vector<Primitive* > primitives;
	mutable vector<VolumeRegion *> volumeRegions;
	map<string, vector<Primitive* > > instances;
	vector<Primitive* > *currentInstance;
};
RenderOptions::RenderOptions() {
	// RenderOptions Constructor Implementation
	FilterName = "mitchell";
	FilmName = "multiimage";
	SamplerName = "lowdiscrepancy";
	AcceleratorName = "kdtree";
	SurfIntegratorName = "path";
	VolIntegratorName = "emission";
	CameraName = "perspective";
	/*
	char *searchEnv = getenv("LUX_SEARCHPATH");
	if (searchEnv == NULL) {
		Warning("LUX_SEARCHPATH not set in your environment.\n"
			  "lux won't be able to find plugins if "
			  "no SearchPath in input file.\n"
			  "LUX_SEARCHPATH should be a "
			  "\"%s\"-separated list of directories.\n",
			  LUX_PATH_SEP);
		gotSearchPath = false;
	}
	else {
		UpdatePluginPath(searchEnv);
		gotSearchPath = true;
	}*/
	currentInstance = NULL;
}
struct GraphicsState {
	// Graphics State Methods
	GraphicsState();
	// Graphics State
	map<string, boost::shared_ptr<Texture<float> > >
		floatTextures;
	map<string, boost::shared_ptr<Texture<Spectrum> > >
		spectrumTextures;
	ParamSet materialParams;
	string material;
	ParamSet areaLightParams;
	string areaLight;
	string currentLight;
	Light* currentLightPtr;
	bool reverseOrientation;
};
GraphicsState::GraphicsState() {
	// GraphicsState Constructor Implementation
	material = "matte";
	reverseOrientation = false;
}
// API Static Data
#define STATE_UNINITIALIZED  0
#define STATE_OPTIONS_BLOCK  1
#define STATE_WORLD_BLOCK    2
static int currentApiState = STATE_UNINITIALIZED;
static Transform curTransform;
static map<string, Transform> namedCoordinateSystems;
static RenderOptions *renderOptions = NULL;
static GraphicsState graphicsState;
static vector<GraphicsState> pushedGraphicsStates;
static vector<Transform> pushedTransforms;
// API Macros
#define VERIFY_INITIALIZED(func) \
if (currentApiState == STATE_UNINITIALIZED) { \
	Error("luxInit() must be before calling \"%s()\". " \
		"Ignoring.", func); \
	return; \
} else /* swallow trailing semicolon */
#define VERIFY_OPTIONS(func) \
VERIFY_INITIALIZED(func); \
if (currentApiState == STATE_WORLD_BLOCK) { \
	Error("Options cannot be set inside world block; " \
		"\"%s\" not allowed.  Ignoring.", func); \
	return; \
} else /* swallow trailing semicolon */
#define VERIFY_WORLD(func) \
VERIFY_INITIALIZED(func); \
if (currentApiState == STATE_OPTIONS_BLOCK) { \
	Error("Scene description must be inside world block; " \
		"\"%s\" not allowed. Ignoring.", func); \
	return; \
} else /* swallow trailing semicolon */
// API Function Definitions
void luxInit() {
	// System-wide initialization
	// Make sure floating point unit's rounding stuff is set
	// as is expected by the fast FP-conversion routines.  In particular,
	// we want double precision on Linux, not extended precision!
	#ifdef FAST_INT
	#if defined(__linux__) && defined(__i386__)
	int cword = _FPU_MASK_DM | _FPU_MASK_ZM | _FPU_MASK_OM | _FPU_MASK_PM |
		_FPU_MASK_UM | _FPU_MASK_IM | _FPU_DOUBLE | _FPU_RC_NEAREST;
	_FPU_SETCW(cword);
	#endif
	#if defined(WIN32)
	_control87(_PC_53, MCW_PC);
	#endif
	#endif // FAST_INT
	// API Initialization
	if (currentApiState != STATE_UNINITIALIZED)
		Error("luxInit() has already been called.");
	currentApiState = STATE_OPTIONS_BLOCK;
	renderOptions = new RenderOptions;
	graphicsState = GraphicsState();
}
 void luxCleanup() {
	StatsCleanup();
	// API Cleanup
	if (currentApiState == STATE_UNINITIALIZED)
		Error("luxCleanup() called without luxInit().");
	else if (currentApiState == STATE_WORLD_BLOCK)
		Error("luxCleanup() called while inside world block.");
	currentApiState = STATE_UNINITIALIZED;
	delete renderOptions;
	renderOptions = NULL;
}
 void luxIdentity() {
	VERIFY_INITIALIZED("Identity");
	curTransform = Transform();
}
 void luxTranslate(float dx, float dy, float dz) {
	VERIFY_INITIALIZED("Translate");
	curTransform =
		curTransform * Translate(Vector(dx, dy, dz));
}
 void luxTransform(float tr[16]) {
	VERIFY_INITIALIZED("Transform");
	Matrix4x4Ptr o (new Matrix4x4(
		tr[0], tr[4], tr[8], tr[12],
		tr[1], tr[5], tr[9], tr[13],
		tr[2], tr[6], tr[10], tr[14],
		tr[3], tr[7], tr[11], tr[15]));
	curTransform = Transform(o);
}
void luxConcatTransform(float tr[16]) {
	VERIFY_INITIALIZED("ConcatTransform");
    Matrix4x4Ptr o (new Matrix4x4(tr[0], tr[4], tr[8], tr[12],
				 tr[1], tr[5], tr[9], tr[13],
				 tr[2], tr[6], tr[10], tr[14],
				 tr[3], tr[7], tr[11], tr[15]));
	curTransform = curTransform * Transform(o);
}
void luxRotate(float angle, float dx, float dy, float dz) {
	VERIFY_INITIALIZED("Rotate");
	curTransform = curTransform * Rotate(angle, Vector(dx, dy, dz));
}
void luxScale(float sx, float sy, float sz) {
	VERIFY_INITIALIZED("Scale");
	curTransform = curTransform * Scale(sx, sy, sz);
}
void luxLookAt(float ex, float ey, float ez, float lx, float ly,
	float lz, float ux, float uy, float uz) {
	VERIFY_INITIALIZED("LookAt");
	curTransform = curTransform * LookAt(Point(ex, ey, ez), Point(lx, ly, lz),
		Vector(ux, uy, uz));
}
void luxCoordinateSystem(const string &name) {
	VERIFY_INITIALIZED("CoordinateSystem");
	namedCoordinateSystems[name] = curTransform;
}
void luxCoordSysTransform(const string &name) {
	VERIFY_INITIALIZED("CoordSysTransform");
	if (namedCoordinateSystems.find(name) !=
	    namedCoordinateSystems.end())
		curTransform = namedCoordinateSystems[name];
}
void luxPixelFilter(const string &name,
                           const ParamSet &params) {
	VERIFY_OPTIONS("PixelFilter");
	renderOptions->FilterName = name;
	renderOptions->FilterParams = params;
}
void luxFilm(const string &type, const ParamSet &params) {
	VERIFY_OPTIONS("Film");
	renderOptions->FilmParams = params;
	renderOptions->FilmName = type;
}
void luxSampler(const string &name, const ParamSet &params) {
	VERIFY_OPTIONS("Sampler");
	renderOptions->SamplerName = name;
	renderOptions->SamplerParams = params;
}
void luxAccelerator(const string &name, const ParamSet &params) {
	VERIFY_OPTIONS("Accelerator");
	renderOptions->AcceleratorName = name;
	renderOptions->AcceleratorParams = params;
}
void luxSurfaceIntegrator(const string &name, const ParamSet &params) {
	VERIFY_OPTIONS("SurfaceIntegrator");
	renderOptions->SurfIntegratorName = name;
	renderOptions->SurfIntegratorParams = params;
}
void luxVolumeIntegrator(const string &name, const ParamSet &params) {
	VERIFY_OPTIONS("VolumeIntegrator");
	renderOptions->VolIntegratorName = name;
	renderOptions->VolIntegratorParams = params;
}
void luxCamera(const string &name,
                       const ParamSet &params) {
	VERIFY_OPTIONS("Camera");
	renderOptions->CameraName = name;
	renderOptions->CameraParams = params;
	renderOptions->WorldToCamera = curTransform;
	namedCoordinateSystems["camera"] =
		curTransform.GetInverse();
}
void luxWorldBegin() {
	VERIFY_OPTIONS("WorldBegin");
	currentApiState = STATE_WORLD_BLOCK;
	curTransform = Transform();
	namedCoordinateSystems["world"] = curTransform;
}
void luxAttributeBegin() {
	VERIFY_WORLD("AttributeBegin");
	pushedGraphicsStates.push_back(graphicsState);
	pushedTransforms.push_back(curTransform);
}
void luxAttributeEnd() {
	VERIFY_WORLD("AttributeEnd");
	if (!pushedGraphicsStates.size()) {
		Error("Unmatched luxAttributeEnd() encountered. "
			"Ignoring it.");
		return;
	}
	graphicsState = pushedGraphicsStates.back();
	curTransform = pushedTransforms.back();
	pushedGraphicsStates.pop_back();
	pushedTransforms.pop_back();
}
void luxTransformBegin() {
	VERIFY_WORLD("TransformBegin");
	pushedTransforms.push_back(curTransform);
}
void luxTransformEnd() {
	VERIFY_WORLD("TransformEnd");
	if (!pushedTransforms.size()) {
		Error("Unmatched luxTransformEnd() encountered. "
			"Ignoring it.");
		return;
	}
	curTransform = pushedTransforms.back();
	pushedTransforms.pop_back();
}
void luxTexture(const string &name,
                         const string &type,
						 const string &texname,
						 const ParamSet &params) {
	VERIFY_WORLD("Texture");
	TextureParams tp(params, params,
	                 graphicsState.floatTextures,
		             graphicsState.spectrumTextures);
	if (type == "float")  {
		// Create _float_ texture and store in _floatTextures_
		if (graphicsState.floatTextures.find(name) !=
		    graphicsState.floatTextures.end())
			Warning("Texture \"%s\" being redefined", name.c_str());
		boost::shared_ptr<Texture<float> > ft = MakeFloatTexture(texname,
			curTransform, tp);
		if (ft) graphicsState.floatTextures[name] = ft;
	}
	else if (type == "color")  {
		// Create _color_ texture and store in _spectrumTextures_
		if (graphicsState.spectrumTextures.find(name) != graphicsState.spectrumTextures.end())
			Warning("Texture \"%s\" being redefined", name.c_str());
		boost::shared_ptr<Texture<Spectrum> > st = MakeSpectrumTexture(texname,
			curTransform, tp);
		if (st) graphicsState.spectrumTextures[name] = st;
	}
	else
		Error("Texture type \"%s\" unknown.", type.c_str());
}
void luxMaterial(const string &name, const ParamSet &params) {
	VERIFY_WORLD("Material");
	graphicsState.material = name;
	graphicsState.materialParams = params;
}
void luxLightSource(const string &name,
                             const ParamSet &params) {
	VERIFY_WORLD("LightSource");

	if(name == "sunsky") {
		// SunSky light - create both sun & sky lightsources
		Light *lt_sun = MakeLight("sun", curTransform, params);
		if (lt_sun == NULL)
			Error("luxLightSource: light type sun unknown.");
		else {
			renderOptions->lights.push_back(lt_sun);
		}
		Light *lt_sky = MakeLight("sky", curTransform, params);
		if (lt_sky == NULL)
			Error("luxLightSource: light type sky unknown.");
		else {
			renderOptions->lights.push_back(lt_sky);
			graphicsState.currentLight = name;
			graphicsState.currentLightPtr = lt_sky;
		}
	} else {
		// other lightsource type
		Light *lt = MakeLight(name, curTransform, params);
		if (lt == NULL)
			Error("luxLightSource: light type "
			      "\"%s\" unknown.", name.c_str());
		else {
			renderOptions->lights.push_back(lt);
			graphicsState.currentLight = name;
			graphicsState.currentLightPtr = lt;
		}
	}
}

void luxAreaLightSource(const string &name,
                                 const ParamSet &params) {
	VERIFY_WORLD("AreaLightSource");
	graphicsState.areaLight = name;
	graphicsState.areaLightParams = params;
}

void luxPortalShape(const string &name,
                       const ParamSet &params) {
	VERIFY_WORLD("PortalShape");
	ShapePtr shape = MakeShape(name,
		curTransform, graphicsState.reverseOrientation,
		params);
	if (!shape) return;
	params.ReportUnused();
	// Initialize area light for shape									// TODO - radiance - add portalshape to area light & cleanup
	AreaLight *area = NULL;
	//if (graphicsState.areaLight != "")
	//	area = MakeAreaLight(graphicsState.areaLight,
	//	curTransform, graphicsState.areaLightParams, shape);
	
	if (graphicsState.currentLight != "") {
		if(graphicsState.currentLight == "sunsky" 
			|| graphicsState.currentLight == "infinite")
			graphicsState.currentLightPtr->AddPortalShape( shape );
		else {
			Warning("LightType '%s' does not support PortalShape(s).\n",  graphicsState.currentLight.c_str());
			return;
		}
	}

	// Initialize material for shape (dummy)
	TextureParams mp(params,
	                 graphicsState.materialParams,
					 graphicsState.floatTextures,
					 graphicsState.spectrumTextures);
	boost::shared_ptr<Texture<float> > bump;
	MaterialPtr mtl = MakeMaterial("matte", curTransform, mp);

	// Create primitive (for refining) (dummy)
	Primitive* prim (new GeometricPrimitive(shape, mtl, area));
}

void luxShape(const string &name,
                       const ParamSet &params) {
	VERIFY_WORLD("Shape");
	ShapePtr shape = MakeShape(name,
		curTransform, graphicsState.reverseOrientation,
		params);
	if (!shape) return;
	params.ReportUnused();
	// Initialize area light for shape
	AreaLight *area = NULL;
	if (graphicsState.areaLight != "")
		area = MakeAreaLight(graphicsState.areaLight,
		curTransform, graphicsState.areaLightParams, shape);
	// Initialize material for shape
	TextureParams mp(params,
	                 graphicsState.materialParams,
					 graphicsState.floatTextures,
					 graphicsState.spectrumTextures);
	boost::shared_ptr<Texture<float> > bump;
	MaterialPtr mtl =
		MakeMaterial(graphicsState.material,
		             curTransform, mp);
	if (!mtl)
		mtl = MakeMaterial("matte", curTransform, mp);
	if (!mtl)
		Severe("Unable to create \"matte\" material?!");
	// Create primitive and add to scene or current instance
	Primitive* prim (new GeometricPrimitive(shape, mtl, area));
	if (renderOptions->currentInstance) {
		if (area)
			Warning("Area lights not supported "
			        "with object instancing");
		renderOptions->currentInstance->push_back(prim);
	}
	else {
		renderOptions->primitives.push_back(prim);
		if (area != NULL) {
			// Add area light for primitive to light vector
			renderOptions->lights.push_back(area);
		}
	}
}
void luxReverseOrientation() {
	VERIFY_WORLD("ReverseOrientation");
	graphicsState.reverseOrientation =
		!graphicsState.reverseOrientation;
}
void luxVolume(const string &name,
                        const ParamSet &params) {
	VERIFY_WORLD("Volume");
	VolumeRegion *vr = MakeVolumeRegion(name,
		curTransform, params);
	if (vr) renderOptions->volumeRegions.push_back(vr);
}
void luxObjectBegin(const string &name) {
	VERIFY_WORLD("ObjectBegin");
	luxAttributeBegin();
	if (renderOptions->currentInstance)
		Error("ObjectBegin called inside "
		      "of instance definition");
	renderOptions->instances[name] =
		vector<Primitive* >();
	renderOptions->currentInstance =
		&renderOptions->instances[name];
}
void luxObjectEnd() {
	VERIFY_WORLD("ObjectEnd");
	if (!renderOptions->currentInstance)
		Error("ObjectEnd called outside "
		      "of instance definition");
	renderOptions->currentInstance = NULL;
	luxAttributeEnd();
}
void luxObjectInstance(const string &name) {
	VERIFY_WORLD("ObjectInstance");
	// Object instance error checking
	if (renderOptions->currentInstance) {
		Error("ObjectInstance can't be called inside instance definition");
		return;
	}
	if (renderOptions->instances.find(name) == renderOptions->instances.end()) {
		Error("Unable to find instance named \"%s\"", name.c_str());
		return;
	}
	vector<Primitive* > &in =
		renderOptions->instances[name];
	if (in.size() == 0) return;
	if (in.size() > 1 || !in[0]->CanIntersect()) {
		// Refine instance _Primitive_s and create aggregate
		Primitive* accel =
			MakeAccelerator(renderOptions->AcceleratorName,
			               in, renderOptions->AcceleratorParams);
		if (!accel)
			accel = MakeAccelerator("kdtree", in, ParamSet());
		if (!accel)
			Severe("Unable to find \"kdtree\" accelerator");
		in.erase(in.begin(), in.end());
		in.push_back(accel);
	}
    Primitive* o (new InstancePrimitive(in[0], curTransform));
	Primitive* prim = o;
	renderOptions->primitives.push_back(prim);
}
void luxWorldEnd() {
	VERIFY_WORLD("WorldEnd");
	// Ensure the search path was set
	/*if (!renderOptions->gotSearchPath)
		Severe("LUX_SEARCHPATH environment variable "
		                  "wasn't set and a plug-in\n"
			              "search path wasn't given in the "
						  "input (with the SearchPath "
						  "directive).\n");*/
	// Ensure there are no pushed graphics states
	while (pushedGraphicsStates.size()) {
		Warning("Missing end to luxAttributeBegin()");
		pushedGraphicsStates.pop_back();
		pushedTransforms.pop_back();
	}
	// Create scene and render
	Scene *scene = renderOptions->MakeScene();
	if (scene) scene->Render();
	delete scene;
	// Clean up after rendering
	currentApiState = STATE_OPTIONS_BLOCK;
	StatsPrint(stdout);
	curTransform = Transform();
	namedCoordinateSystems.erase(namedCoordinateSystems.begin(),
		namedCoordinateSystems.end());
}
Scene *RenderOptions::MakeScene() const {
	// Create scene objects from API settings
	Filter *filter = MakeFilter(FilterName, FilterParams);
	Film *film = MakeFilm(FilmName, FilmParams, filter);
	if(std::string(FilmName)=="film")
		Warning("Warning: Legacy PBRT 'film' does not provide tonemapped output or GUI film display.\nUse 'multifilm' instead.\n");
	Camera *camera = MakeCamera(CameraName, CameraParams,
		WorldToCamera, film);
	Sampler *sampler = MakeSampler(SamplerName, SamplerParams, film);
	SurfaceIntegrator *surfaceIntegrator = MakeSurfaceIntegrator(SurfIntegratorName,
		SurfIntegratorParams);
	VolumeIntegrator *volumeIntegrator = MakeVolumeIntegrator(VolIntegratorName,
		VolIntegratorParams);
	Primitive *accelerator = MakeAccelerator(AcceleratorName,
		primitives, AcceleratorParams);
	if (!accelerator) {
		ParamSet ps;
		accelerator = MakeAccelerator("kdtree", primitives, ps);
	}
	if (!accelerator)
		Severe("Unable to find \"kdtree\" accelerator");
	// Initialize _volumeRegion_ from volume region(s)
	VolumeRegion *volumeRegion;
	if (volumeRegions.size() == 0)
		volumeRegion = NULL;
	else if (volumeRegions.size() == 1)
		volumeRegion = volumeRegions[0];
	else
		volumeRegion = new AggregateVolume(volumeRegions);
	// Make sure all plugins initialized properly
	if (!camera || !sampler || !film || !accelerator ||
		!filter || !surfaceIntegrator || !volumeIntegrator) {
		Severe("Unable to create scene due "
		       "to missing plug-ins");
		return NULL;
	}
	Scene *ret = new Scene(camera,
	    surfaceIntegrator, volumeIntegrator,
		sampler, accelerator, lights, volumeRegion);
	// Erase primitives, lights, and volume regions from _RenderOptions_
	primitives.erase(primitives.begin(),
	                primitives.end());
	lights.erase(lights.begin(),
	            lights.end());
	volumeRegions.erase(volumeRegions.begin(),
	                  volumeRegions.end());
	return ret;
}

//interactive control

//CORE engine control


//user interactive thread functions
void luxStart()
{
	luxCurrentScene->Start();
}

void luxPause()
{
	luxCurrentScene->Pause();
}

void luxExit()
{
	luxCurrentScene->Exit();
}

//controlling number of threads
int luxAddThread()
{
	return luxCurrentScene->AddThread();
}

void luxRemoveThread()
{
	luxCurrentScene->RemoveThread();
}

//framebuffer access
void luxUpdateFramebuffer()
{
	luxCurrentScene->UpdateFramebuffer();
}

unsigned char* luxFramebuffer()
{
	return luxCurrentScene->GetFramebuffer();
}

int luxDisplayInterval()
{
	return luxCurrentScene->DisplayInterval();
}

int luxFilmXres()
{
	return luxCurrentScene->FilmXres();
}

int luxFilmYres()
{
	return luxCurrentScene->FilmYres();
}

double luxStatistics(char *statName)
{
	if(std::string(statName)=="sceneIsReady") return(luxCurrentScene!=NULL);
	else return luxCurrentScene->Statistics(statName);
}

//error handling
LuxErrorHandler luxError=luxErrorPrint;
int luxLastError=LUX_NOERROR;

void luxErrorHandler (LuxErrorHandler handler)
{
    luxError=handler;
}

void luxErrorAbort (int code, int severity, const char *message)
{
    luxErrorPrint(code,severity,message);
    if(severity!=LUX_INFO)
    	exit(code);
}

void luxErrorIgnore (int code, int severity, const char *message)
{
    luxLastError=code;
}

void luxErrorPrint (int code, int severity, const char *message)
{
    luxLastError=code;
    std::cerr<<std::endl<<"[Lux ";
    std::cerr<<boost::posix_time::second_clock::local_time()<<' ';
    switch (severity)
    {
    case LUX_INFO:
        std::cerr<<"INFO";
        break;
    case LUX_WARNING:
        std::cerr<<"WARNING";
        break;
    case LUX_ERROR:
        std::cerr<<"ERROR";
        break;
    case LUX_SEVERE:
        std::cerr<<"SEVERE ERROR";
        break;
    }
    std::cerr<<" : "<<code<<" ] "<<message<<std::endl;
}


