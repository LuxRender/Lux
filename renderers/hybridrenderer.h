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

	unsigned int GetAvailableUnitsCount() const {
		return max(boost::thread::hardware_concurrency(), 1u);
	}
	unsigned int GetUsedUnitsCount() const;
	void SetUsedUnitsCount(const unsigned int units) const;

	friend class HybridRenderer;
	friend class HRHostDescription;

private:
	HRDeviceDescription(HRHostDescription *h, const string &n) :
		host(h), name(n) { }
	~HRDeviceDescription() { }

	HRHostDescription *host;
	string name;
};

//------------------------------------------------------------------------------
// HRHostDescription
//------------------------------------------------------------------------------

class HRHostDescription : protected RendererHostDescription {
public:
	const string &GetName() const { return name; }

	const vector<RendererDeviceDescription *> &GetDeviceDescs() const { return devs; }

	friend class HybridRenderer;
	friend class HRDeviceDescription;

private:
	HRHostDescription(HybridRenderer *r, const string &n);
	~HRHostDescription();

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
	const vector<RendererHostDescription *> &GetHostDescs() const;
	void SuspendWhenDone(bool v);

	double Statistics(const string &statName);

	void Render(Scene *scene);

	void Pause();
	void Resume();
	void Terminate();

	friend class HRDeviceDescription;
	friend class HRHostDescription;

	static Renderer *CreateRenderer(const ParamSet &params);

private:
	//--------------------------------------------------------------------------
	// RenderThread
	//--------------------------------------------------------------------------

	class RenderThread : public boost::noncopyable {
	public:
		RenderThread(u_int index, HybridRenderer *renderer);
		~RenderThread();

		static void RenderImpl(RenderThread *r);

		u_int  n;
		HybridRenderer *renderer;
		boost::thread *thread; // keep pointer to delete the thread object
		double samples, blackSamples;
		fast_mutex statLock;
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
	vector<RendererHostDescription *> hosts;
	vector<RenderThread *> renderThreads;
	Scene *scene;

	fast_mutex sampPosMutex;
	u_int sampPos;

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
