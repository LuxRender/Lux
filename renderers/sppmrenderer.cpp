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

#include "api.h"
#include "scene.h"
#include "camera.h"
#include "film.h"
#include "sampling.h"
#include "randomgen.h"
#include "context.h"
#include "sppmrenderer.h"
#include "integrators/sppm.h"

using namespace lux;

//------------------------------------------------------------------------------
// SPPMRDeviceDescription
//------------------------------------------------------------------------------

unsigned int SPPMRDeviceDescription::GetUsedUnitsCount() const {
	boost::mutex::scoped_lock lock(host->renderer->renderThreadsMutex);
	return host->renderer->renderThreads.size();
}

void SPPMRDeviceDescription::SetUsedUnitsCount(const unsigned int units) {
}

//------------------------------------------------------------------------------
// SPPMRHostDescription
//------------------------------------------------------------------------------

SPPMRHostDescription::SPPMRHostDescription(SPPMRenderer *r, const string &n) : renderer(r), name(n) {
	SPPMRDeviceDescription *desc = new SPPMRDeviceDescription(this, "CPUs");
	devs.push_back(desc);
}

SPPMRHostDescription::~SPPMRHostDescription() {
	for (size_t i = 0; i < devs.size(); ++i)
		delete devs[i];
}

//------------------------------------------------------------------------------
// SPPMRenderer
//------------------------------------------------------------------------------

SPPMRenderer::SPPMRenderer() : Renderer() {
	state = INIT;

	SPPMRHostDescription *host = new SPPMRHostDescription(this, "Localhost");
	hosts.push_back(host);

	preprocessDone = false;
	suspendThreadsWhenDone = false;

	AddStringConstant(*this, "name", "Name of current renderer", "sppm");
}

SPPMRenderer::~SPPMRenderer() {
	boost::mutex::scoped_lock lock(classWideMutex);

	if ((state != TERMINATE) && (state != INIT))
		throw std::runtime_error("Internal error: called SPPMRenderer::~SPPMRenderer() while not in TERMINATE or INIT state.");

	if (renderThreads.size() > 0)
		throw std::runtime_error("Internal error: called SPPMRenderer::~SPPMRenderer() while list of renderThread sis not empty.");

	for (size_t i = 0; i < hosts.size(); ++i)
		delete hosts[i];
}

Renderer::RendererType SPPMRenderer::GetType() const {
	boost::mutex::scoped_lock lock(classWideMutex);

	return SPPM;
}

Renderer::RendererState SPPMRenderer::GetState() const {
	boost::mutex::scoped_lock lock(classWideMutex);

	return state;
}

vector<RendererHostDescription *> &SPPMRenderer::GetHostDescs() {
	boost::mutex::scoped_lock lock(classWideMutex);

	return hosts;
}

void SPPMRenderer::SuspendWhenDone(bool v) {
	boost::mutex::scoped_lock lock(classWideMutex);
	suspendThreadsWhenDone = v;
}

void SPPMRenderer::Render(Scene *s) {
	{
		// Section under mutex
		boost::mutex::scoped_lock lock(classWideMutex);

		scene = s;

		// TODO: At the moment I have no way to check if the SPPM integrator
		// has been really used
		sppmi = (SPPMIntegrator *)scene->surfaceIntegrator;

		if (scene->IsFilmOnly()) {
			state = TERMINATE;
			return;
		}

		if (scene->lights.size() == 0) {
			LOG(LUX_SEVERE,LUX_MISSINGDATA)<< "No light sources defined in scene; nothing to render.";
			state = TERMINATE;
			return;
		}

		state = RUN;

		// Initialize the stats
		lastSamples = 0.;
		lastTime = 0.;
		stat_Samples = 0.;
		stat_blackSamples = 0.;
		s_Timer.Reset();
	
		// Dade - I have to do initiliaziation here for the current thread.
		// It can be used by the Preprocess() methods.

		// initialize the thread's rangen
		u_long seed = scene->seedBase - 1;
		LOG(LUX_INFO, LUX_NOERROR) << "Preprocess thread uses seed: " << seed;

		RandomGenerator rng(seed);

		// integrator preprocessing
		scene->sampler->SetFilm(scene->camera->film);
		scene->surfaceIntegrator->Preprocess(rng, *scene);
		scene->volumeIntegrator->Preprocess(rng, *scene);
		scene->camera->film->CreateBuffers();

		// Dade - to support autofocus for some camera model
		scene->camera->AutoFocus(*scene);

		sampPos = 0;

		size_t threadCount = boost::thread::hardware_concurrency();
		LOG(LUX_INFO, LUX_NOERROR) << "Hardware concurrency: " << threadCount;

		// Create synchronization barriers
		barrier = new boost::barrier(threadCount);

		photonTracedTotal = 0;
		photonTracedPass = 0;

		// start the timer
		s_Timer.Start();

		// Dade - preprocessing done
		preprocessDone = true;
		Context::GetActive()->SceneReady();

		// Start all threads
		for (size_t i = 0; i < threadCount; ++i) {
			RenderThread *rt = new  RenderThread(i, this);

			renderThreads.push_back(rt);
			rt->thread = new boost::thread(boost::bind(RenderThread::RenderImpl, rt));
		}
	}

	if (renderThreads.size() > 0) {
		// The first thread can not be removed
		// it will terminate when the rendering is finished
		renderThreads[0]->thread->join();

		// rendering done, now I can remove all rendering threads
		{
			boost::mutex::scoped_lock lock(renderThreadsMutex);

			// wait for all threads to finish their job
			for (u_int i = 0; i < renderThreads.size(); ++i) {
				renderThreads[i]->thread->join();
				delete renderThreads[i];
			}
			renderThreads.clear();

			// I change the current signal to exit in order to disable the creation
			// of new threads after this point
			Terminate();
		}

		// Flush the contribution pool
		scene->camera->film->contribPool->Flush();
		scene->camera->film->contribPool->Delete();
	}
}

void SPPMRenderer::Pause() {
	boost::mutex::scoped_lock lock(classWideMutex);
	state = PAUSE;
}

void SPPMRenderer::Resume() {
	boost::mutex::scoped_lock lock(classWideMutex);
	state = RUN;
}

void SPPMRenderer::Terminate() {
	boost::mutex::scoped_lock lock(classWideMutex);
	state = TERMINATE;
}

//------------------------------------------------------------------------------
// Statistic methods
//------------------------------------------------------------------------------

// Statistics Access
double SPPMRenderer::Statistics(const string &statName) {
	if(statName=="secElapsed") {
		// Dade - s_Timer is inizialized only after the preprocess phase
		if (preprocessDone)
			return s_Timer.Time();
		else
			return 0.0;
	} else if(statName=="samplesSec")
		return Statistics_SamplesPSec();
	else if(statName=="samplesTotSec")
		return Statistics_SamplesPTotSec();
	else if(statName=="samplesPx")
		return Statistics_SamplesPPx();
	else if(statName=="efficiency")
		return Statistics_Efficiency();
	else if(statName=="displayInterval")
		return scene->DisplayInterval();
	else if(statName == "filmEV")
		return scene->camera->film->EV;
	else if(statName == "averageLuminance")
		return scene->camera->film->averageLuminance;
	else if(statName == "numberOfLocalSamples")
		return scene->camera->film->numberOfLocalSamples;
	else if (statName == "enoughSamples")
		return scene->camera->film->enoughSamplesPerPixel;
	else if (statName == "threadCount")
		return renderThreads.size();
	else {
		LOG(LUX_ERROR,LUX_BADTOKEN)<< "luxStatistics - requested an invalid data : "<< statName;
		return 0.;
	}
}

double SPPMRenderer::Statistics_GetNumberOfSamples() {
	if (s_Timer.Time() - lastTime > .5f) {
		boost::mutex::scoped_lock lock(renderThreadsMutex);

		for (u_int i = 0; i < renderThreads.size(); ++i) {
			fast_mutex::scoped_lock lockStats(renderThreads[i]->statLock);
			stat_Samples += renderThreads[i]->samples;
			stat_blackSamples += renderThreads[i]->blackSamples;
			renderThreads[i]->samples = 0.;
			renderThreads[i]->blackSamples = 0.;
		}
	}

	return stat_Samples + scene->camera->film->numberOfSamplesFromNetwork;
}

double SPPMRenderer::Statistics_SamplesPPx() {
	// divide by total pixels
	int xstart, xend, ystart, yend;
	scene->camera->film->GetSampleExtent(&xstart, &xend, &ystart, &yend);
	return Statistics_GetNumberOfSamples() / ((xend - xstart) * (yend - ystart));
}

double SPPMRenderer::Statistics_SamplesPSec() {
	// Dade - s_Timer is inizialized only after the preprocess phase
	if (!preprocessDone)
		return 0.0;

	double samples = Statistics_GetNumberOfSamples();
	double time = s_Timer.Time();
	double dif_samples = samples - lastSamples;
	double elapsed = time - lastTime;
	lastSamples = samples;
	lastTime = time;

	// return current samples / sec total
	if (elapsed == 0.0)
		return 0.0;
	else
		return dif_samples / elapsed;
}

double SPPMRenderer::Statistics_SamplesPTotSec() {
	// Dade - s_Timer is inizialized only after the preprocess phase
	if (!preprocessDone)
		return 0.0;

	double samples = Statistics_GetNumberOfSamples();
	double time = s_Timer.Time();

	// return current samples / total elapsed secs
	return samples / time;
}

double SPPMRenderer::Statistics_Efficiency() {
	Statistics_GetNumberOfSamples(); // required before eff can be calculated.

	if (stat_Samples == 0.0)
		return 0.0;

	return (100.f * stat_blackSamples) / stat_Samples;
}

//------------------------------------------------------------------------------
// Private methods
//------------------------------------------------------------------------------

void SPPMRenderer::UpdateFilm() {
	const u_int width = scene->camera->film->GetXPixelCount();
	const u_int bufferId = sppmi->bufferId;

	for (unsigned int i = 0; i < hitPoints->GetSize(); ++i) {
		HitPoint *hp = hitPoints->GetHitPoint(i);
		const float scrX = i % width;
		const float scrY = i / width;

		Contribution contrib(scrX, scrY, hp->radiance, hp->eyeAlpha,
				hp->eyeDistance, 0.f, bufferId);
		scene->camera->film->SetSample(&contrib);
	}
}

//------------------------------------------------------------------------------
// RenderThread methods
//------------------------------------------------------------------------------

SPPMRenderer::RenderThread::RenderThread(u_int index, SPPMRenderer *r) :
	n(index), renderer(r), thread(NULL), samples(0.), blackSamples(0.) {
}

SPPMRenderer::RenderThread::~RenderThread() {
}

void SPPMRenderer::RenderThread::RenderImpl(RenderThread *myThread) {
	SPPMRenderer *renderer = myThread->renderer;
	boost::barrier *barrier = renderer->barrier;

	Scene &scene(*(renderer->scene));
	if (scene.IsFilmOnly())
		return;

	// To avoid interrupt exception
	boost::this_thread::disable_interruption di;

	// Dade - wait the end of the preprocessing phase
	while (!renderer->preprocessDone) {
		boost::xtime xt;
		boost::xtime_get(&xt, boost::TIME_UTC);
		++xt.sec;
		boost::thread::sleep(xt);
	}

	// Initialize the thread's rangen
	u_long seed = scene.seedBase + myThread->n;
	LOG(LUX_INFO, LUX_NOERROR) << "Thread " << myThread->n << " uses seed: " << seed;
	RandomGenerator rng(seed);

	HitPoints *hitPoints = NULL;

	//--------------------------------------------------------------------------
	// First eye pass
	//--------------------------------------------------------------------------

	if (myThread->n == 0) {
		// One thread initialize the hit points
		renderer->hitPoints = new HitPoints(renderer);
		hitPoints = renderer->hitPoints;

		hitPoints->SetHitPoints(&rng);
		hitPoints->Init();
	}

	// Wait for other threads
	barrier->wait();

	// Last step of the initialization
	hitPoints = renderer->hitPoints;
	hitPoints->RefreshAccelParallel(myThread->n, renderer->renderThreads.size());

	// Wait for other threads
	barrier->wait();

	// Trace rays: The main loop
	while (true) {
		while (renderer->state == PAUSE && !boost::this_thread::interruption_requested()) {
			boost::xtime xt;
			boost::xtime_get(&xt, boost::TIME_UTC);
			xt.sec += 1;
			boost::thread::sleep(xt);
		}
		if ((renderer->state == TERMINATE) || boost::this_thread::interruption_requested())
			break;

		//----------------------------------------------------------------------
		// Trace photons
		//----------------------------------------------------------------------

		// TODO

		//----------------------------------------------------------------------
		// End of the pass
		//----------------------------------------------------------------------

		// Wait for other threads
		barrier->wait();

		// The first thread has to do some special task for the eye pass
		if (myThread->n == 0) {
			// First thread only tasks
			const long long count = renderer->photonTracedTotal + renderer->photonTracedPass;
			hitPoints->AccumulateFlux(count);

			hitPoints->SetHitPoints(&rng);
			hitPoints->UpdatePointsInformation();
			hitPoints->IncPass();
			hitPoints->RefreshAccelMutex();

			// Update the frame buffer
			renderer->UpdateFilm();

			renderer->photonTracedTotal = count;
			renderer->photonTracedPass = 0;
		}

		// Wait for other threads
		barrier->wait();

		hitPoints->RefreshAccelParallel(myThread->n, renderer->renderThreads.size());

		// Wait for other threads
		barrier->wait();
	}
}

Renderer *SPPMRenderer::CreateRenderer(const ParamSet &params) {
	return new SPPMRenderer();
}

static DynamicLoader::RegisterRenderer<SPPMRenderer> r("sppm");
