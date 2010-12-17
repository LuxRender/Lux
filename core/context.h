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

#ifndef LUX_CONTEXT_H
#define LUX_CONTEXT_H

#include "lux.h"
#include "stats.h"
#include "geometry/transform.h"
#include "paramset.h"
#include "queryableregistry.h"

#include <boost/thread/mutex.hpp>
#include <map>
using std::map;

//TODO - jromang : convert to enum
#define STATE_UNINITIALIZED  0
#define STATE_OPTIONS_BLOCK  1
#define STATE_WORLD_BLOCK    2
#define STATE_PARSE_FAIL     3

namespace lux {

class Context {
public:

	Context(std::string n = "Lux default context") : name(n) {
		Init();
	}

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

	// Load/save FLM file
	void LoadFLM(const string &name);
	void SaveFLM(const string &name);
	void OverrideResumeFLM(const string &name);

	// Save OpenEXR image
	void SaveEXR(const string &name, bool useHalfFloat, bool includeZBuffer, int compressionType, bool tonemapped);	
	
	//CORE engine control
	//user interactive thread functions
	void Resume();
	void Pause();
	void Wait();
	void Exit();

	void SetHaltSamplesPerPixel(int haltspp, bool haveEnoughSamplesPerPixel,
		bool suspendThreadsWhenDone);

	//controlling number of threads
	u_int AddThread();
	void RemoveThread();


	//framebuffer access
	void UpdateFramebuffer();
	unsigned char* Framebuffer();
	float* FloatFramebuffer();
	float* AlphaBuffer();

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
	void SetNetworkServerUpdateInterval(int updateInterval);
	int GetNetworkServerUpdateInterval();
	void TransmitFilm(std::basic_ostream<char> &stream);
	void TransmitFilm(std::basic_ostream<char> &stream, bool useCompression, bool directWrite);
	void AddServer(const string &name);
	void RemoveServer(const RenderingServerInfo &rsi);
	void RemoveServer(const string &name);
	u_int GetServerCount();
	u_int GetRenderingServersStatus(RenderingServerInfo *info, u_int maxInfoCount);

	//statistics
	double Statistics(const string &statName);
	void SceneReady();

	const char* PrintableStatistics(const bool add_total);
	const char* CustomStatistics(const string custom_template);

	void EnableDebugMode();
	void DisableRandomMode();

	void SetEpsilon(const float minValue, const float maxValue);

	//! Registry containing all queryable objects of the current context
	//! \author jromang
	QueryableRegistry registry;

	int currentApiState;

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
			currentInstance = NULL;
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
		lux::Transform worldToCamera;
		lux::Transform worldToCameraEnd;
		mutable vector<Light *> lights;
		mutable vector<boost::shared_ptr<Primitive> > primitives;
		mutable vector<Region *> volumeRegions;
		mutable map<string, vector<boost::shared_ptr<Primitive> > > instances;
		mutable vector<string> lightGroups;
		mutable vector<boost::shared_ptr<Primitive> > *currentInstance;
		bool gotSearchPath;
		bool debugMode;
		bool randomMode;
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
 		Light* currentLightPtr0;
 		Light* currentLightPtr1;
		bool reverseOrientation;
	};

	static Context *activeContext;
	string name;
	lux::Renderer *luxCurrentRenderer;
	Scene *luxCurrentScene;
	lux::Transform curTransform;
	map<string, lux::Transform> namedCoordinateSystems;
	RenderOptions *renderOptions;
	GraphicsState *graphicsState;
	vector<GraphicsState> pushedGraphicsStates;
	vector<lux::Transform> pushedTransforms;
	RenderFarm *renderFarm;

	ParamSet *filmOverrideParams;
	
	// Dade - mutex used to wait the end of the rendering
	mutable boost::mutex renderingMutex;
	bool luxCurrentSceneReady;
	bool terminated;

	StatsData *statsData;
};

}

#endif //LUX_CONTEXT_H
