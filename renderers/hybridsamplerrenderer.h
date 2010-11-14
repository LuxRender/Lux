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

#ifndef LUX_HYBRIDSAMPLERRENDERER_H
#define LUX_HYBRIDSAMPLERRENDERER_H

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

namespace lux
{

//------------------------------------------------------------------------------
// HybridSamplerRenderer
//------------------------------------------------------------------------------

class HybridSamplerRenderer : public HybridRenderer {
public:
	HybridSamplerRenderer();
	~HybridSamplerRenderer();

	RendererType GetType() const;

	RendererState GetState() const;
	vector<RendererHostDescription *> &GetHostDescs();
	void SuspendWhenDone(bool v);

	double Statistics(const string &statName);

	void Render(Scene *scene);

	void Pause();
	void Resume();
	void Terminate();

	static Renderer *CreateRenderer(const ParamSet &params);

private:
	//--------------------------------------------------------------------------
	// RenderThread
	//--------------------------------------------------------------------------

	class RenderThread : public boost::noncopyable {
	public:
		RenderThread(u_int index, HybridSamplerRenderer *renderer, luxrays::IntersectionDevice * idev);
		~RenderThread();

		static void RenderImpl(RenderThread *r);

		u_int  n;
		boost::thread *thread; // keep pointer to delete the thread object
		HybridSamplerRenderer *renderer;
		luxrays::IntersectionDevice * iDevice;

		// Rendering statistics
		fast_mutex statLock;
		double samples, blackSamples;
	};

	void CreateRenderThread();
	void RemoveRenderThread();
	size_t GetRenderThreadCount() const  { return renderThreads.size(); }

	double Statistics_GetNumberOfSamples();
	double Statistics_SamplesPSec();
	double Statistics_SamplesPTotSec();
	double Statistics_Efficiency();
	double Statistics_SamplesPPx();

	//--------------------------------------------------------------------------

	luxrays::Context *ctx;

	RendererState state;
	// LuxRays virtual device used to feed all HardwareIntersectionDevice
	luxrays::VirtualM2OHardwareIntersectionDevice *virtualIDevice;
	vector<RenderThread *> renderThreads;
	Scene *scene;
	u_long lastUsedSeed;

	Timer s_Timer;
	double lastSamples, lastTime;
	double stat_Samples, stat_blackSamples;

	// Put them last for better data alignment
	// used to suspend render threads until the preprocessing phase is done
	bool preprocessDone;
	bool suspendThreadsWhenDone;
};

}//namespace lux

#endif // LUX_HYBRIDSAMPLERRENDERER_H
