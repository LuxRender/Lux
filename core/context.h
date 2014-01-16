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

#ifndef LUX_CONTEXT_H
#define LUX_CONTEXT_H

#include "api.h"
#include "lux.h"
#include "geometry/transform.h"
#include "paramset.h"
#include "queryableregistry.h"
#include "renderer.h"
#include "motionsystem.h"

#include <boost/thread/mutex.hpp>
#include <map>
using std::map;

//TODO - jromang : convert to enum
#define STATE_UNINITIALIZED  0
#define STATE_OPTIONS_BLOCK  1
#define STATE_WORLD_BLOCK    2
#define STATE_PARSE_FAIL     3

namespace lux {

class LUX_EXPORT Context {
public:

	Context(std::string n = "Lux default context") : name(n) {}

	~Context() {
		Free();
	}

	//TODO jromang - const & reference
	static Context* GetActive() {
		return activeContext;
	}
	static void SetActive(Context *c) {
		activeContext = c;
	}

	static map<string, boost::shared_ptr<lux::Texture<float> > > *GetActiveFloatTextures() {
		return &(activeContext->graphicsState->floatTextures);
	}
	static map<string, boost::shared_ptr<lux::Texture<SWCSpectrum> > > *GetActiveColorTextures() {
		return &(activeContext->graphicsState->colorTextures);
	}
	static map<string, boost::shared_ptr<lux::Texture<FresnelGeneral> > > *GetActiveFresnelTextures() {
		return &(activeContext->graphicsState->fresnelTextures);
	}
	static u_int GetActiveLightGroup() {
		return activeContext->GetLightGroup();
	}

	boost::shared_ptr<lux::Texture<float> > GetFloatTexture(const string &n) const;
	boost::shared_ptr<lux::Texture<SWCSpectrum> > GetColorTexture(const string &n) const;
	boost::shared_ptr<lux::Texture<FresnelGeneral> > GetFresnelTexture(const string &n) const;
	boost::shared_ptr<lux::Material > GetMaterial(const string &n) const;

	void Init();
	void Cleanup();
	void Free();

	std::string GetName() { return name; }

	// API Function Declarations
	void Accelerator(const string &name, const ParamSet &params);
	void AreaLightSource(const string &name, const ParamSet &params);
	void AttributeBegin();
	void AttributeEnd();
	void Camera(const string &, const ParamSet &cameraParams);
	void ConcatTransform(float transform[16]);
	void CoordinateSystem(const string &);
	void CoordSysTransform(const string &);
	void Exterior(const string &name);
	void Film(const string &type, const ParamSet &params);
	void Identity();
	void Interior(const string &name);
	void LightGroup(const string &name, const ParamSet &params);
	void LightSource(const string &name, const ParamSet &params);
	void LookAt(float ex, float ey, float ez, float lx, float ly, float lz,
		float ux, float uy, float uz);
	void MakeNamedMaterial(const string &name, const ParamSet &params);
	void MakeNamedVolume(const string &id, const string &name,
		const ParamSet &params);
	void Material(const string &name, const ParamSet &params);
	void MotionBegin(u_int n, float *t);
	void MotionEnd();
	void MotionInstance(const string &name, float startTime, float endTime,
		const string &toTransform);
	void NamedMaterial(const string &name);
	void PixelFilter(const string &name, const ParamSet &params);
	void PortalShape(const string &name, const ParamSet &params);
	void ObjectBegin(const string &name);
	void ObjectEnd();
	void ObjectInstance(const string &name);
	void PortalInstance(const string &name);
	void Renderer(const string &, const ParamSet &params);
	void ReverseOrientation();
	void Rotate(float angle, float ax, float ay, float az);
	void Sampler(const string &name, const ParamSet &params);
	void Scale(float sx, float sy, float sz);
	void Shape(const string &name, const ParamSet &params);
	void SurfaceIntegrator(const string &name,
		const ParamSet &params);
	void Texture(const string &name, const string &type,
		const string &texname, const ParamSet &params);
	void Transform(float transform[16]);
	void TransformBegin();
	void TransformEnd();
	void Translate(float dx, float dy, float dz);
	void Volume(const string &name, const ParamSet &params);
	void VolumeIntegrator(const string &name, const ParamSet &params);
	void WorldBegin();
	void WorldEnd();
	// Used to end the parse phase after StartRenderingAfterParse(false)
	void ParseEnd();

	// Load/save FLM file
	void LoadFLM(const string &name);
	void SaveFLM(const string &name);
	void OverrideResumeFLM(const string &name);
	void OverrideFilename(const string &filename);

	unsigned char* SaveFLMToStream(unsigned int& size);
	void LoadFLMFromStream(char* buffer, unsigned int bufSize, const string &name);
	double UpdateFilmFromStream(std::basic_istream<char> &ifs);
	void ResetFLM();

	// Save OpenEXR image
	bool SaveEXR(const string &name, bool useHalfFloat, bool includeZBuffer, int compressionType, bool tonemapped);
	
	//CORE engine control
	//user interactive thread functions
	void Resume();
	void Pause();
	void Wait();
	void Exit();
	void Abort();

	void SetHaltSamplesPerPixel(int haltspp, bool haveEnoughSamplesPerPixel,
		bool suspendThreadsWhenDone);
	// Must be called after Init() otherwise the Renderer is NULL
	Renderer::RendererType GetRendererType() const;

	//controlling number of threads
	u_int AddThread();
	void RemoveThread();


	//framebuffer access
	void UpdateFramebuffer();
	unsigned char* Framebuffer();
	float* FloatFramebuffer();
	float* AlphaBuffer();
	float* ZBuffer();

	//histogram access
	void GetHistogramImage(unsigned char *outPixels, u_int width, u_int height, int options);

	// Parameter Access functions
	void SetParameterValue(luxComponent comp, luxComponentParameters param,
		double value, u_int index);
	double GetParameterValue(luxComponent comp,
		luxComponentParameters param, u_int index);
	double GetDefaultParameterValue(luxComponent comp,
		luxComponentParameters param, u_int index);
	void SetStringParameterValue(luxComponent comp,
		luxComponentParameters param, const string& value, u_int index);
	string GetStringParameterValue(luxComponent comp,
		luxComponentParameters param, u_int index);
	string GetDefaultStringParameterValue(luxComponent comp,
		luxComponentParameters param, u_int index);

	u_int GetLightGroup();

	// Dade - network rendering
	void UpdateFilmFromNetwork();
	void UpdateLogFromNetwork();
	void WriteFilmToStream(std::basic_ostream<char> &stream);
	void WriteFilmToStream(std::basic_ostream<char> &stream, bool directWrite);
	void AddServer(const string &name);
	void RemoveServer(const RenderingServerInfo &rsi);
	void RemoveServer(const string &name);
	void ResetServer(const string &name, const string &password);
	u_int GetRenderingServersStatus(RenderingServerInfo *info, u_int maxInfoCount);

	//statistics
	double Statistics(const string &statName);
	void SceneReady();

	void UpdateStatisticsWindow();
	bool IsRendering();

	void EnableDebugMode();
	void DisableRandomMode();

	void SetEpsilon(const float minValue, const float maxValue);
	// NOTE: this feature is not currently supported by network rendering
	void StartRenderingAfterParse(const bool start);

	void UpdateNetworkNoiseAwareMap();
	void SetNoiseAwareMap(const float *map);
	// NOTE: returns a copy of the map, it is up to the caller to free the allocated memory !
	float *GetNoiseAwareMap();
	void SetUserSamplingMap(const float *map);
	// NOTE: returns a copy of the map, it is up to the caller to free the allocated memory !
	float *GetUserSamplingMap();

	//! Registry containing all queryable objects of the current context
	//! \author jromang
	QueryableRegistry registry;

	int currentApiState;

	friend class RenderFarm;

private:
	// API Local Classes
	struct RenderOptions {
		// RenderOptions Public Methods
		RenderOptions() {
			// RenderOptions Constructor Implementation
			filterName = "mitchell";
			filmName = "fleximage";
			samplerName = "random";
			acceleratorName = "kdtree";
			surfIntegratorName = "path";
			volIntegratorName = "emission";
			cameraName = "perspective";
			rendererName = "sampler";
			currentInstanceRefined = NULL;
			currentInstanceSource = NULL;
			currentLightInstance = NULL;
			currentAreaLightInstance = NULL;
			debugMode = false;
			randomMode = true;
		}

		Scene *MakeScene() const;
		lux::Renderer *MakeRenderer() const;

		// RenderOptions Public Data
		string filterName;
		ParamSet filterParams;
		string filmName;
		ParamSet filmParams;
		string samplerName;
		ParamSet samplerParams;
		string acceleratorName;
		ParamSet acceleratorParams;
		string surfIntegratorName, volIntegratorName;
		ParamSet surfIntegratorParams, volIntegratorParams;
		string cameraName;
		ParamSet cameraParams;
		string rendererName;
		ParamSet rendererParams;
		MotionTransform worldToCamera;
		mutable vector<boost::shared_ptr<Light> > lights;
		mutable vector<boost::shared_ptr<Primitive> > primitives;
		mutable vector<Region *> volumeRegions;
		// Unrefined primitives
		mutable map<string, vector<boost::shared_ptr<Primitive> > > instancesSource;
		// Refined primitives
		mutable map<string, vector<boost::shared_ptr<Primitive> > > instancesRefined;
		mutable map<string, vector<boost::shared_ptr<Light> > > lightInstances;
		// Area light instances
		// Use a vector of vector to hold the list of refined primitives
		// for each light source
		// This way there is one vector item per instanced light source
		// and only one light source will get added when instancing
		// And all primitives are already refined
		// and can be instanced right away
		mutable map<string, vector<vector<boost::shared_ptr<AreaLightPrimitive> > > > areaLightInstances;
		mutable vector<string> lightGroups;
		// Refined primitives
		mutable vector<boost::shared_ptr<Primitive> > *currentInstanceSource;
		// Unrefined primitives
		mutable vector<boost::shared_ptr<Primitive> > *currentInstanceRefined;
		mutable vector<boost::shared_ptr<Light> > *currentLightInstance;
		mutable vector<vector<boost::shared_ptr<AreaLightPrimitive> > > *currentAreaLightInstance;
		bool gotSearchPath;
		bool debugMode;
		bool randomMode;
		static Context *activeContext;
	};

	struct GraphicsState {
		// Graphics State Methods
		GraphicsState() {
			// GraphicsState Constructor Implementation
			currentLightGroup = "";
			reverseOrientation = false;
		}
		// Graphics State
		map<string, boost::shared_ptr<lux::Texture<float> > > floatTextures;
		map<string, boost::shared_ptr<lux::Texture<SWCSpectrum> > > colorTextures;
		map<string, boost::shared_ptr<lux::Texture<FresnelGeneral> > > fresnelTextures;
		map<string, boost::shared_ptr<lux::Material> > namedMaterials;
		map<string, boost::shared_ptr<lux::Volume> > namedVolumes;
		boost::shared_ptr<lux::Volume> exterior;
		boost::shared_ptr<lux::Volume> interior;
		boost::shared_ptr<lux::Material> material;
		ParamSet areaLightParams;
		string areaLight;
		string currentLight;
		string currentLightGroup;
		// Dade - some light source like skysun is composed by 2 lights. So
		// we can have 2 current light sources (i.e. Portal have to be applied
		// to both sources, see bug #297)
		boost::shared_ptr<Light> currentLightPtr0;
		boost::shared_ptr<Light> currentLightPtr1;
		bool reverseOrientation;
	};

	static Context *activeContext;
	string name;
	u_int shapeNo; // used to identify anonymous shapes
	lux::Renderer *luxCurrentRenderer;
	Scene *luxCurrentScene;
	lux::MotionTransform curTransform;
	bool inMotionBlock;
	vector<float> motionBlockTimes; // holds time values for current motion block
	vector<lux::Transform> motionBlockTransforms; // holds transform for current motion block
	map<string, lux::MotionTransform> namedCoordinateSystems;
	RenderOptions *renderOptions;
	GraphicsState *graphicsState;
	vector<GraphicsState> pushedGraphicsStates;
	vector<lux::MotionTransform> pushedTransforms;
	RenderFarm *renderFarm;

	ParamSet *filmOverrideParams;
	
	// Dade - mutex used to wait the end of the rendering
	mutable boost::mutex renderingMutex;
	bool startRenderingAfterParse;
	bool terminated;
	bool aborted; // abort rendering
};

}

#endif //LUX_CONTEXT_H
