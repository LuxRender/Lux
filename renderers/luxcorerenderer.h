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

#ifndef LUX_LUXCORERENDERER_H
#define LUX_LUXCORERENDERER_H

#include <vector>
#include <boost/thread.hpp>

#include "luxrays/luxrays.h"
#include "luxrays/core/device.h"
#include "luxrays/core/intersectiondevice.h"
#include "luxcore/luxcore.h"

#include "lux.h"
#include "renderer.h"
#include "fastmutex.h"
#include "timer.h"
#include "dynload.h"
#include "hybridrenderer.h"
#include "mipmap.h"

extern void LuxCoreDebugHandler(const char *msg);

namespace lux
{

class LuxCoreRenderer;
class LuxCoreHostDescription;

//------------------------------------------------------------------------------
// LuxCoreDeviceDescription
//------------------------------------------------------------------------------

class LuxCoreDeviceDescription : protected RendererDeviceDescription {
public:
	const string &GetName() const { return name; }

	u_int GetAvailableUnitsCount() const { return 1; }
	u_int GetUsedUnitsCount() const { return 1; }
	void SetUsedUnitsCount(const u_int units) { }

	friend class LuxCoreRenderer;
	friend class LuxCoreHostDescription;

protected:
	LuxCoreDeviceDescription(LuxCoreHostDescription *h, const string &n) : host(h), name(n) { }

	LuxCoreHostDescription *host;
	string name;
};

//------------------------------------------------------------------------------
// LuxCoreHostDescription
//------------------------------------------------------------------------------

class LuxCoreHostDescription : protected RendererHostDescription {
public:
	const string &GetName() const { return name; }

	vector<RendererDeviceDescription *> &GetDeviceDescs() { return devs; }

	friend class LuxCoreRenderer;
	friend class LuxCoreDeviceDescription;

private:
	LuxCoreHostDescription(LuxCoreRenderer *r, const string &n);
	~LuxCoreHostDescription();

	void AddDevice(LuxCoreDeviceDescription *devDesc);

	LuxCoreRenderer *renderer;
	string name;
	vector<RendererDeviceDescription *> devs;
};

//------------------------------------------------------------------------------
// LuxCoreRenderer
//------------------------------------------------------------------------------

class LuxCoreRenderer : public Renderer {
public:
	LuxCoreRenderer(const luxrays::Properties &overwriteConfig);
	~LuxCoreRenderer();

	RendererType GetType() const { return LUXCORE_TYPE; }

	RendererState GetState() const;
	vector<RendererHostDescription *> &GetHostDescs();
	void SuspendWhenDone(bool v);

	void Render(Scene *scene);

	void Pause();
	void Resume();
	void Terminate();

	friend class LuxCoreDeviceDescription;
	friend class LuxCoreHostDescription;
	friend class LuxCoreStatistics;

	static Renderer *CreateRenderer(const ParamSet &params);

private:
	void ConvertCamera(luxcore::Scene *lcScene);
	void ConvertLights(luxcore::Scene *lcScene);
	vector<luxrays::ExtTriangleMesh *> DefinePrimitive(luxcore::Scene *lcScene, const Primitive *prim);
	void ConvertGeometry(luxcore::Scene *lcScene, ColorSystem &colorSpace);
	luxcore::Scene *CreateLuxCoreScene(const luxrays::Properties &lcConfigProps, ColorSystem &colorSpace);
	luxrays::Properties CreateLuxCoreConfig();

	void UpdateLuxFilm(luxcore::RenderSession *session);

	mutable boost::mutex classWideMutex;

	RendererState state;
	vector<RendererHostDescription *> hosts;

	luxrays::Properties overwriteConfig;
	string renderEngineType;
	Scene *scene;
	// Used to feed LuxRender film with only the delta information from the previous update
	vector<float *> previousFilm_RADIANCE_PER_PIXEL_NORMALIZEDs;
	vector<float *> previousFilm_RADIANCE_PER_SCREEN_NORMALIZEDs;
	float *previousFilm_ALPHA;
	float *previousFilm_DEPTH;
	double previousFilmSampleCount;

	// Put them last for better data alignment
	// used to suspend render threads until the preprocessing phase is done
	bool preprocessDone;
	bool suspendThreadsWhenDone;
};

}//namespace lux

#endif // LUX_LUXCORERENDERER_H
