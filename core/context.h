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

#ifndef LUX_CONTEXT_H
#define LUX_CONTEXT_H

#include "lux.h"
#include "renderfarm.h"

#include <boost/thread/mutex.hpp>

//TODO - jromang : convert to enum
#define STATE_UNINITIALIZED  0
#define STATE_OPTIONS_BLOCK  1
#define STATE_WORLD_BLOCK    2

namespace lux {

class Context {
public:

	Context(std::string n="Lux default context") : name(n) {
            init();
	}

        ~Context() {
            free();
        }

	//static bool checkMode(unsigned char modeMask, std::string callerName, int errorCode); //!< Check the graphics state mode in the active context

	//GState graphicsState;

	//TODO jromang - const & reference
	static Context* getActive() {
		return activeContext;
	}
	static void setActive(Context *c) {
		activeContext=c;
	}

	static map<string, boost::shared_ptr<Texture<float> > > *getActiveFloatTextures() {
		return &(activeContext->graphicsState->floatTextures);
	}
	static map<string, boost::shared_ptr<Texture<SWCSpectrum> > > *getActiveSWCSpectrumTextures() {
		return &(activeContext->graphicsState->colorTextures);
	}

	//'static' API
	//static void luxPixelFilter(const char *, const ParamSet &params) { activeContext->pixelFilter(std::string(name), params); }
	static void luxIdentity() { activeContext->identity(); }
	static void luxTranslate(float dx, float dy, float dz) { activeContext->translate(dx, dy, dz); }
	static void luxRotate(float angle, float ax, float ay, float az) { activeContext->rotate(angle, ax, ay, az); }
	static void luxScale(float sx, float sy, float sz) { activeContext->scale(sx, sy, sz); }
	static void luxLookAt(float ex, float ey, float ez, float lx, float ly, float lz, float ux, float uy, float uz) { activeContext->lookAt(ex, ey, ez, lx, ly, lz, ux, uy, uz) ; }
	static void luxConcatTransform(float transform[16]) { activeContext->concatTransform(transform); }
	static void luxTransform(float transform[16]) { activeContext->transform(transform); }
	static void luxCoordinateSystem(const string &s) { activeContext->coordinateSystem(s) ; }
	static void luxCoordSysTransform(const string &s) { activeContext->coordSysTransform(s); }
	static void luxPixelFilter(const string &name, const ParamSet &params) { activeContext->pixelFilter(name, params); }
	static void luxFilm(const string &type, const ParamSet &params) { activeContext->film(type, params); }
	static void luxSampler(const string &name, const ParamSet &params) { activeContext->sampler(name, params); }
	static void luxAccelerator(const string &name, const ParamSet &params) { activeContext->accelerator(name, params); }
	static void luxSurfaceIntegrator(const string &name, const ParamSet &params) { activeContext->surfaceIntegrator(name, params); }
	static void luxVolumeIntegrator(const string &name, const ParamSet &params) { activeContext->volumeIntegrator(name, params); }
	static void luxCamera(const string &s, const ParamSet &cameraParams) { activeContext->camera(s, cameraParams); }
	static void luxWorldBegin() { activeContext->worldBegin(); }
	static void luxAttributeBegin() { activeContext->attributeBegin(); }
	static void luxAttributeEnd() { activeContext->attributeEnd(); }
	static void luxTransformBegin() { activeContext->transformBegin(); }
	static void luxTransformEnd() { activeContext->transformEnd(); }
	static void luxTexture(const string &name, const string &type, const string &texname, const ParamSet &params) { activeContext->texture(name, type, texname, params); }
	static void luxMaterial(const string &name, const ParamSet &params) { activeContext->material(name, params); }
	static void luxMakeNamedMaterial(const string &name, const ParamSet &params) { activeContext->makenamedmaterial(name, params); }
	static void luxNamedMaterial(const string &name, const ParamSet &params) { activeContext->namedmaterial(name, params); }
	static void luxLightSource(const string &name, const ParamSet &params) { activeContext->lightSource(name, params); }
	static void luxAreaLightSource(const string &name, const ParamSet &params) { activeContext->areaLightSource(name, params); }
	static void luxPortalShape(const string &name, const ParamSet &params) { activeContext->portalShape(name, params); }
	static void luxShape(const string &name, const ParamSet &params) { activeContext->shape(name, params); }
	static void luxReverseOrientation() { activeContext->reverseOrientation(); }
	static void luxVolume(const string &name, const ParamSet &params) { activeContext->volume(name, params); }
	static void luxObjectBegin(const string &name) { activeContext->objectBegin(name); }
	static void luxObjectEnd() { activeContext->objectEnd(); }
	static void luxObjectInstance(const string &name) { activeContext->objectInstance(name); }
	static void luxWorldEnd() { activeContext->worldEnd(); }


	void makemixmaterial(const ParamSet shapeparams, const ParamSet materialparams, boost::shared_ptr<Material> mtl);

	//TODO - jromang replace by a destructor or remove
	static void luxCleanup() { activeContext->cleanup(); }

	//CORE engine control
	//user interactive thread functions
	static void luxStart() { activeContext->start(); }
	static void luxPause() { activeContext->pause(); }
	static void luxExit() { activeContext->exit(); }
	// Dade - wait for the end of the rendering
	static void luxWait() { activeContext->wait(); }

	static void luxSetHaltSamplePerPixel(int haltspp, bool haveEnoughSamplePerPixel, bool suspendThreadsWhenDone) {
		activeContext->setHaltSamplePerPixel(haltspp, haveEnoughSamplePerPixel,suspendThreadsWhenDone);
	}

	//controlling number of threads
	static int luxAddThread() { return activeContext->addThread(); }
	static void luxRemoveThread() { activeContext->removeThread(); }
	static int luxGetRenderingThreadsStatus(RenderingThreadInfo *info, int maxInfoCount) { return activeContext->getRenderingThreadsStatus(info, maxInfoCount); }

	//framebuffer access
	static void luxUpdateFramebuffer() { activeContext->updateFramebuffer(); }
	static unsigned char* luxFramebuffer() { return activeContext->framebuffer(); }
	//static int luxDisplayInterval() { return activeContext->displayInterval(); }
	//static int luxFilmXres() { return activeContext->filmXres(); }
	//static int luxFilmYres() { return activeContext->filmYres(); }

	//film access (networking)
	static void luxUpdateFilmFromNetwork() { activeContext->updateFilmFromNetwork(); }
	static void luxSetNetworkServerUpdateInterval(int updateInterval) { activeContext->renderFarm->serverUpdateInterval = updateInterval; }
	static int luxGetNetworkServerUpdateInterval() { return activeContext->renderFarm->serverUpdateInterval; }
    static void luxAddServer(const string &name) { activeContext->addServer(name); }
	static int luxGetRenderingServersStatus(RenderingServerInfo *info, int maxInfoCount) { return activeContext->getRenderingServersStatus(info, maxInfoCount); }

	//statistics
	static double luxStatistics(const string &statName) { return activeContext->statistics(statName); }

	//film access (networking)
    static void luxTransmitFilm(std::basic_ostream<char> &stream) { activeContext->transmitFilm(stream); }

    // dade enable debug mode
    static void luxEnableDebugMode() { activeContext->enableDebugMode(); }

private:
	static Context *activeContext;
	string name;
	Scene *luxCurrentScene;

        void init();
        void free();

	// API Function Declarations
	void identity();
	void translate(float dx, float dy, float dz);
	void rotate(float angle, float ax, float ay, float az);
	void scale(float sx, float sy, float sz);
	void lookAt(float ex, float ey, float ez, float lx, float ly, float lz,
			float ux, float uy, float uz);
	void concatTransform(float transform[16]);
	void transform(float transform[16]);
	void coordinateSystem(const string &);
	void coordSysTransform(const string &);
	void pixelFilter(const string &name, const ParamSet &params);
	void film(const string &type, const ParamSet &params);
	void sampler(const string &name, const ParamSet &params);
	void accelerator(const string &name, const ParamSet &params);
	void surfaceIntegrator(const string &name, const ParamSet &params);
	void volumeIntegrator(const string &name, const ParamSet &params);
	void camera(const string &, const ParamSet &cameraParams);
	void worldBegin();
	void attributeBegin();
	void attributeEnd();
	void transformBegin();
	void transformEnd();
	void texture(const string &name, const string &type, const string &texname,
			const ParamSet &params);
	void material(const string &name, const ParamSet &params);
	void makenamedmaterial(const string &name, const ParamSet &params);
	void namedmaterial(const string &name, const ParamSet &params);
	void lightSource(const string &name, const ParamSet &params);
	void areaLightSource(const string &name, const ParamSet &params);
	void portalShape(const string &name, const ParamSet &params);
	void shape(const string &name, const ParamSet &params);
	void reverseOrientation();
	void volume(const string &name, const ParamSet &params);
	void objectBegin(const string &name);
	void objectEnd();
	void objectInstance(const string &name);
	void worldEnd();

	//TODO - jromang replace by a destructor or remove
	void cleanup();

	//CORE engine control
	//user interactive thread functions
	void start();
	void pause();
	void exit();
	void wait();

	void setHaltSamplePerPixel(int haltspp, bool haveEnoughSamplePerPixel,
		bool suspendThreadsWhenDone);

	//controlling number of threads
	int addThread();
	void removeThread();
	int getRenderingThreadsStatus(RenderingThreadInfo *info, int maxInfoCount);

	//framebuffer access
	void updateFramebuffer();
	unsigned char* framebuffer();
	/*
	int displayInterval();
	int filmXres();
	int filmYres();*/

	// Dade - network rendering
	void updateFilmFromNetwork();
    void transmitFilm(std::basic_ostream<char> &stream);

	//statistics
	double statistics(const string &statName);
	void addServer(const string &name);
	int getRenderingServersStatus(RenderingServerInfo *info, int maxInfoCount);

    void enableDebugMode();

	// API Local Classes
	struct RenderOptions {
		// RenderOptions Public Methods
		RenderOptions() {
			// RenderOptions Constructor Implementation
			FilterName = "mitchell";
			FilmName = "multiimage";
			SamplerName = "random";
			AcceleratorName = "kdtree";
			SurfIntegratorName = "path";
			VolIntegratorName = "emission";
			CameraName = "perspective";
			currentInstance = NULL;
            debugMode = false;
		}

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
		mutable vector<boost::shared_ptr<Primitive> > primitives;
		mutable vector<VolumeRegion *> volumeRegions;
		mutable map<string, vector<boost::shared_ptr<Primitive> > > instances;
		mutable vector<boost::shared_ptr<Primitive> > *currentInstance;
        bool debugMode;
	};

	struct NamedMaterial {
		NamedMaterial() {};

		ParamSet materialParams;
		string material;
	};

	struct GraphicsState {
		// Graphics State Methods
		GraphicsState() {
			// GraphicsState Constructor Implementation
			material = "matte";
			reverseOrientation = false;
		}
		// Graphics State
		map<string, boost::shared_ptr<Texture<float> > > floatTextures;
		map<string, boost::shared_ptr<Texture<SWCSpectrum> > > colorTextures;
		ParamSet materialParams;
		string material;
		ParamSet areaLightParams;
		string areaLight;
		string currentLight;
		// Dade - some light source like skysun is composed by 2 lights. So
		// we can have 2 current light sources (i.e. Portal have to be applied
		// to both sources, see bug #297)
 		Light* currentLightPtr0;
 		Light* currentLightPtr1;
		bool reverseOrientation;
	};

	int currentApiState;
	Transform curTransform;
	map<string, Transform> namedCoordinateSystems;
	RenderOptions *renderOptions;
	GraphicsState *graphicsState;
	vector<NamedMaterial> namedmaterials;
	vector<GraphicsState> pushedGraphicsStates;
	vector<Transform> pushedTransforms;
	RenderFarm *renderFarm;

        // Dade - mutex used to wait the end of the rendering
        mutable boost::mutex renderingMutex;
};

}

#endif //LUX_CONTEXT_H
