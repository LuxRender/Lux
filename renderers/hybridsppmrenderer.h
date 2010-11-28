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

#ifndef LUX_HYBRIDSPPM_H
#define LUX_HYBRIDSPPM_H

#include <vector>
#include <boost/thread.hpp>

#include "lux.h"
#include "renderer.h"
#include "fastmutex.h"
#include "timer.h"
#include "dynload.h"
#include "hybridrenderer.h"
#include "hybridsppm/hitpoints.h"

#include "luxrays/luxrays.h"
#include "luxrays/core/device.h"
#include "luxrays/core/intersectiondevice.h"

namespace lux
{

//------------------------------------------------------------------------------
// HybridSPPM
//------------------------------------------------------------------------------

#define SPPM_DEVICE_RENDER_BUFFER_COUNT 4

class HybridSPPMRenderer : public HybridRenderer {
public:
	HybridSPPMRenderer();
	~HybridSPPMRenderer();

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

	friend class HitPoints;

private:
	//--------------------------------------------------------------------------
	// RenderThread
	//--------------------------------------------------------------------------

	class RenderThread : public boost::noncopyable {
	public:
		RenderThread(const u_int index, HybridSPPMRenderer *renderer, luxrays::IntersectionDevice *idev);
		~RenderThread();

		void UpdateFilm();

		static void RenderImpl(RenderThread *r);

		u_int  n;
		boost::thread *thread; // keep pointer to delete the thread object
		HybridSPPMRenderer *renderer;
		luxrays::IntersectionDevice * iDevice;
		luxrays::RayBuffer *rayBufferHitPoints;
		std::vector<luxrays::RayBuffer *> rayBuffersList;
		std::vector<std::vector<PhotonPath> *> photonPathsList;

		// Rendering statistics
		fast_mutex statLock;
		double samples, blackSamples;
	};

	void CreateRenderThread();
	void PrivateCreateRenderThread();
	void RemoveRenderThread();
	void PrivateRemoveRenderThread();
	size_t GetRenderThreadCount() const  { return requestedRenderThreadsCount; }

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
	u_long requestedRenderThreadsCount;
	Scene *scene;
	u_long lastUsedSeed;
	u_int bufferId;

	HitPoints *hitPoints;
	LookUpAccelType lookupAccelType;
	// double instead of float because photon counters declared as int 64bit
	double photonAlpha;
	float photonStartRadiusScale;
	unsigned int maxEyePathDepth;
	unsigned int maxPhotonPathDepth;
	unsigned int stochasticInterval;
	bool useDirectLightSampling;

	boost::barrier *barrier;
	boost::barrier *barrierExit;

	Timer s_Timer;
	double lastSamples, lastTime;
	double stat_Samples, stat_blackSamples;

	// Put them last for better data alignment
	// used to suspend render threads until the preprocessing phase is done
	bool preprocessDone;
	bool suspendThreadsWhenDone;
};

}//namespace lux

#endif // LUX_HYBRIDSPPM_H
