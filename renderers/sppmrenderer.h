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

#ifndef LUX_SPPMRenderer_H
#define LUX_SPPMRenderer_H

#include <vector>
#include <boost/thread.hpp>

#include "lux.h"
#include "renderer.h"
#include "fastmutex.h"
#include "timer.h"
#include "dynload.h"
#include "sppm/hitpoints.h"
#include "sppm/photonsampler.h"
#include "core/scheduler.h"

namespace lux
{

class PhotonSampler;
class SPPMRenderer;
class SPPMRHostDescription;
class SPPMIntegrator;

//------------------------------------------------------------------------------
// SPPMRDeviceDescription
//------------------------------------------------------------------------------

class SPPMRDeviceDescription : protected RendererDeviceDescription {
public:
	const string &GetName() const { return name; }

	u_int GetAvailableUnitsCount() const {
		return max(boost::thread::hardware_concurrency(), 1u);
	}
	u_int GetUsedUnitsCount() const;
	void SetUsedUnitsCount(const u_int units);

	friend class SPPMRenderer;
	friend class SPPMRHostDescription;

private:
	SPPMRDeviceDescription(SPPMRHostDescription *h, const string &n) :
		host(h), name(n) { }
	~SPPMRDeviceDescription() { }

	SPPMRHostDescription *host;
	string name;
};

//------------------------------------------------------------------------------
// SPPMRHostDescription
//------------------------------------------------------------------------------

class SPPMRHostDescription : protected RendererHostDescription {
public:
	const string &GetName() const { return name; }

	vector<RendererDeviceDescription *> &GetDeviceDescs() { return devs; }

	friend class SPPMRenderer;
	friend class SPPMRDeviceDescription;

private:
	SPPMRHostDescription(SPPMRenderer *r, const string &n);
	~SPPMRHostDescription();

	SPPMRenderer *renderer;
	string name;
	vector<RendererDeviceDescription *> devs;
};

//------------------------------------------------------------------------------
// SPPMRenderer
//------------------------------------------------------------------------------
class SPPMRenderer : public Renderer {
public:
	SPPMRenderer();
	~SPPMRenderer();

	RendererType GetType() const;

	RendererState GetState() const;
	vector<RendererHostDescription *> &GetHostDescs();
	void SuspendWhenDone(bool v);

	void Render(Scene *scene);
	void RenderMain(Scene *scene);

	void Pause();
	void Resume();
	void Terminate();

	friend class SPPMRDeviceDescription;
	friend class SPPMRHostDescription;

	static Renderer *CreateRenderer(const ParamSet &params);

	friend class HitPoints;
	friend class PhotonSampler;
	friend class SPPMRStatistics;

	// loop if renderer is in pause and return true if renderer is terminate
	bool paused()
	{
		while (state == PAUSE && !boost::this_thread::interruption_requested()) {
			boost::xtime xt;
			boost::xtime_get(&xt, boost::TIME_UTC);
			xt.sec += 1;
			boost::thread::sleep(xt);
		}
		return ((state == TERMINATE) || boost::this_thread::interruption_requested());
	}

	float GetScaleFactor(double const scale) const;

private:
	void TracePhotons(scheduling::Range *range);

	class ScaleUpdaterSPPM : public PerScreenNormalizedBufferScaled::ScaleUpdateInterface
	{
		public:
			ScaleUpdaterSPPM(SPPMRenderer *renderer_): renderer(renderer_) {}

			virtual float GetScaleFactor(double const scale)
			{
				return renderer->GetScaleFactor(scale);
			}

			SPPMRenderer *renderer;
	};
	//--------------------------------------------------------------------------
	// Render threads
	//--------------------------------------------------------------------------

	class RenderThread : public boost::noncopyable, public scheduling::Thread {
	public:
		RenderThread(SPPMRenderer *renderer);
		~RenderThread();

		void TracePhotons(PhotonSampler &sampler, Sample *sample);
		void TracePhoton(
			PhotonSampler &sampler,
			Sample *sample,
			Scene &scene);

		void Init();
		void End();

		SPPMRenderer *renderer;

		RandomGenerator *threadRng;
		Distribution1D *lightCDF;
		PhotonSampler* sampler;

		Sample sample, eyeSample;
	};

	//--------------------------------------------------------------------------

	mutable boost::mutex classWideMutex;
	mutable boost::mutex renderThreadsMutex;

	RendererState state;
	vector<RendererHostDescription *> hosts;

	Scene *scene;
	SPPMIntegrator *sppmi;
	HitPoints *hitPoints;

	// Statistics
	double photonHitEfficiency;

	friend class AMCMCPhotonSampler;
	// Used by AMC Photon Sampler
	u_int uniformCount;

	// Put them last for better data alignment
	// used to suspend render threads until the preprocessing phase is done
	bool preprocessDone;
	bool suspendThreadsWhenDone;

	scheduling::Scheduler *scheduler;
	RandomGenerator* rng;
};

}//namespace lux

#endif // LUX_SPPMRenderer_H
