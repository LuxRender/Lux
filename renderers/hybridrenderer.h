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

#ifndef LUX_HYBRIDRENDERER_H
#define LUX_HYBRIDRENDERER_H

#include <vector>
#include <boost/thread.hpp>

#include "lux.h"
#include "renderer.h"
#include "fastmutex.h"
#include "timer.h"
#include "dynload.h"

#include "luxrays/luxrays.h"
#include "luxrays/core/device.h"
#include "luxrays/core/intersectiondevice.h"

namespace lux
{

class HybridRenderer;
class HRHostDescription;

//------------------------------------------------------------------------------
// HRDeviceDescription
//------------------------------------------------------------------------------

class HRDeviceDescription : protected RendererDeviceDescription {
public:
	const string &GetName() const { return name; }

	friend class HybridRenderer;
	friend class HRHostDescription;

protected:
	HRDeviceDescription(HRHostDescription *h, const string &n) : host(h), name(n) { }

	HRHostDescription *host;
	string name;
};

class HRHardwareDeviceDescription : protected HRDeviceDescription {
public:
	unsigned int GetAvailableUnitsCount() const { return 1;	}
	unsigned int GetUsedUnitsCount() const { return enabled ? 1 : 0; }
	void SetUsedUnitsCount(const unsigned int units);

	friend class HybridRenderer;
	friend class HRHostDescription;

private:
	HRHardwareDeviceDescription(HRHostDescription *h, luxrays::DeviceDescription *desc);
	~HRHardwareDeviceDescription() { }

	luxrays::DeviceDescription *devDesc;
	bool enabled;
};

class HRVirtualDeviceDescription : protected HRDeviceDescription {
public:
	unsigned int GetAvailableUnitsCount() const {
		return max(boost::thread::hardware_concurrency(), 1u);
	}
	unsigned int GetUsedUnitsCount() const;
	void SetUsedUnitsCount(const unsigned int units);

	friend class HybridRenderer;
	friend class HRHostDescription;

private:
	HRVirtualDeviceDescription(HRHostDescription *h, const string &n);
	~HRVirtualDeviceDescription() { }
};

//------------------------------------------------------------------------------
// HRHostDescription
//------------------------------------------------------------------------------

class HRHostDescription : protected RendererHostDescription {
public:
	const string &GetName() const { return name; }

	vector<RendererDeviceDescription *> &GetDeviceDescs() { return devs; }

	friend class HybridRenderer;
	friend class HRDeviceDescription;
	friend class HRHardwareDeviceDescription;
	friend class HRVirtualDeviceDescription;

private:
	HRHostDescription(HybridRenderer *r, const string &n);
	~HRHostDescription();

	void AddDevice(HRDeviceDescription *devDesc);

	HybridRenderer *renderer;
	string name;
	vector<RendererDeviceDescription *> devs;
};

//------------------------------------------------------------------------------
// HybridRenderer
//------------------------------------------------------------------------------

class HybridRenderer : public Renderer {
public:
	HybridRenderer();
	~HybridRenderer();

	RendererType GetType() const;

	RendererState GetState() const;
	vector<RendererHostDescription *> &GetHostDescs();
	void SuspendWhenDone(bool v);

	double Statistics(const string &statName);

	void Render(Scene *scene);

	void Pause();
	void Resume();
	void Terminate();

	friend class HRDeviceDescription;
	friend class HRHardwareDeviceDescription;
	friend class HRVirtualDeviceDescription;
	friend class HRHostDescription;

	static Renderer *CreateRenderer(const ParamSet &params);

private:
	//--------------------------------------------------------------------------
	// RenderThread
	//--------------------------------------------------------------------------

	class RenderThread : public boost::noncopyable {
	public:
		RenderThread(u_int index, HybridRenderer *renderer, luxrays::IntersectionDevice * idev);
		~RenderThread();

		static void RenderImpl(RenderThread *r);

		u_int  n;
		boost::thread *thread; // keep pointer to delete the thread object
		HybridRenderer *renderer;
		luxrays::IntersectionDevice * iDevice;

		// Rendering statistics
		fast_mutex statLock;
		double samples, blackSamples;
	};

	void CreateRenderThread();
	void RemoveRenderThread();

	double Statistics_GetNumberOfSamples();
	double Statistics_SamplesPSec();
	double Statistics_SamplesPTotSec();
	double Statistics_Efficiency();
	double Statistics_SamplesPPx();

	//--------------------------------------------------------------------------

	mutable boost::mutex classWideMutex;

	luxrays::Context *ctx;

	RendererState state;
	// LuxRays virtual device used to feed all HardwareIntersectionDevice
	luxrays::VirtualM2OHardwareIntersectionDevice *virtualIDevice;
	vector<RendererHostDescription *> hosts;
	vector<RenderThread *> renderThreads;
	Scene *scene;
	vector<const Primitive *> primitiveList;

	Timer s_Timer;
	double lastSamples, lastTime;
	double stat_Samples, stat_blackSamples;

	// Put them last for better data alignment
	// used to suspend render threads until the preprocessing phase is done
	bool preprocessDone;
	bool suspendThreadsWhenDone;
};

}//namespace lux

#endif // LUX_HYBRIDRENDERER_H
