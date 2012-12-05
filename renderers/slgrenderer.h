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

#ifndef LUX_SLGRENDERER_H
#define LUX_SLGRENDERER_H

#include <vector>
#include <boost/thread.hpp>

#include "lux.h"
#include "renderer.h"
#include "fastmutex.h"
#include "timer.h"
#include "dynload.h"
#include "hybridrenderer.h"

#include "luxrays/luxrays.h"
#include "luxrays/core/device.h"
#include "luxrays/core/intersectiondevice.h"
#include "slg.h"
#include "luxrays/utils/sdl/scene.h"

extern void DebugHandler(const char *msg);
extern void SDLDebugHandler(const char *msg);
extern void SLGDebugHandler(const char *msg);

namespace lux
{

class SLGRenderer;
class SLGHostDescription;

//------------------------------------------------------------------------------
// SLGDeviceDescription
//------------------------------------------------------------------------------

class SLGDeviceDescription : protected RendererDeviceDescription {
public:
	const string &GetName() const { return name; }

	u_int GetAvailableUnitsCount() const { return 1; }
	u_int GetUsedUnitsCount() const { return 1; }
	void SetUsedUnitsCount(const u_int units) { }

	friend class SLGRenderer;
	friend class SLGHostDescription;

protected:
	SLGDeviceDescription(SLGHostDescription *h, const string &n) : host(h), name(n) { }

	SLGHostDescription *host;
	string name;
};

//------------------------------------------------------------------------------
// SLGHostDescription
//------------------------------------------------------------------------------

class SLGHostDescription : protected RendererHostDescription {
public:
	const string &GetName() const { return name; }

	vector<RendererDeviceDescription *> &GetDeviceDescs() { return devs; }

	friend class SLGRenderer;
	friend class SLGDeviceDescription;

private:
	SLGHostDescription(SLGRenderer *r, const string &n);
	~SLGHostDescription();

	void AddDevice(SLGDeviceDescription *devDesc);

	SLGRenderer *renderer;
	string name;
	vector<RendererDeviceDescription *> devs;
};

//------------------------------------------------------------------------------
// SLGRenderer
//------------------------------------------------------------------------------

class SLGRenderer : public Renderer {
public:
	SLGRenderer();
	~SLGRenderer();

	RendererType GetType() const { return SLG_TYPE; }

	RendererState GetState() const;
	vector<RendererHostDescription *> &GetHostDescs();
	void SuspendWhenDone(bool v);

	void Render(Scene *scene);

	void Pause();
	void Resume();
	void Terminate();

	friend class SLGDeviceDescription;
	friend class SLGHostDescription;
	friend class SLGStatistics;

	static Renderer *CreateRenderer(const ParamSet &params);

private:
	luxrays::sdl::Scene *CreateSLGScene();

	mutable boost::mutex classWideMutex;

	RendererState state;
	vector<RendererHostDescription *> hosts;

	Scene *scene;

	// Put them last for better data alignment
	// used to suspend render threads until the preprocessing phase is done
	bool preprocessDone;
	bool suspendThreadsWhenDone;
};

}//namespace lux

#endif // LUX_SLGRENDERER_H
