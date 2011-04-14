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
#include "samplerrenderer.h"
#include "randomgen.h"
#include "context.h"

using namespace lux;

//------------------------------------------------------------------------------
// SRDeviceDescription
//------------------------------------------------------------------------------

unsigned int SRDeviceDescription::GetUsedUnitsCount() const {
	boost::mutex::scoped_lock lock(host->renderer->renderThreadsMutex);
	return host->renderer->renderThreads.size();
}

void SRDeviceDescription::SetUsedUnitsCount(const unsigned int units) {
	boost::mutex::scoped_lock lock(host->renderer->renderThreadsMutex);

	unsigned int target = max(units, 1u);
	size_t current = host->renderer->renderThreads.size();

	if (current > target) {
		for (unsigned int i = 0; i < current - target; ++i)
			host->renderer->RemoveRenderThread();
	} else if (current < target) {
		for (unsigned int i = 0; i < target - current; ++i)
			host->renderer->CreateRenderThread();
	}
}

//------------------------------------------------------------------------------
// SRHostDescription
//------------------------------------------------------------------------------

SRHostDescription::SRHostDescription(SamplerRenderer *r, const string &n) : renderer(r), name(n) {
	SRDeviceDescription *desc = new SRDeviceDescription(this, "CPUs");
	devs.push_back(desc);
}

SRHostDescription::~SRHostDescription() {
	for (size_t i = 0; i < devs.size(); ++i)
		delete devs[i];
}

//------------------------------------------------------------------------------
// SamplerRenderer
//------------------------------------------------------------------------------

SamplerRenderer::SamplerRenderer() : Renderer() {
	state = INIT;

	SRHostDescription *host = new SRHostDescription(this, "Localhost");
	hosts.push_back(host);

	preprocessDone = false;
	suspendThreadsWhenDone = false;

	AddStringConstant(*this, "name", "Name of current renderer", "sampler");
}

SamplerRenderer::~SamplerRenderer() {
	boost::mutex::scoped_lock lock(classWideMutex);

	if ((state != TERMINATE) && (state != INIT))
		throw std::runtime_error("Internal error: called SamplerRenderer::~SamplerRenderer() while not in TERMINATE or INIT state.");

	if (renderThreads.size() > 0)
		throw std::runtime_error("Internal error: called SamplerRenderer::~SamplerRenderer() while list of renderThread sis not empty.");

	for (size_t i = 0; i < hosts.size(); ++i)
		delete hosts[i];
}

Renderer::RendererType SamplerRenderer::GetType() const {
	boost::mutex::scoped_lock lock(classWideMutex);

	return SAMPLER;
}

Renderer::RendererState SamplerRenderer::GetState() const {
	boost::mutex::scoped_lock lock(classWideMutex);

	return state;
}

vector<RendererHostDescription *> &SamplerRenderer::GetHostDescs() {
	boost::mutex::scoped_lock lock(classWideMutex);

	return hosts;
}

void SamplerRenderer::SuspendWhenDone(bool v) {
	boost::mutex::scoped_lock lock(classWideMutex);
	suspendThreadsWhenDone = v;
}

void SamplerRenderer::Render(Scene *s) {
	{
		// Section under mutex
		boost::mutex::scoped_lock lock(classWideMutex);

		scene = s;

		if (scene->IsFilmOnly()) {
			state = TERMINATE;
			return;
		}

		if (scene->lights.size() == 0) {
			LOG( LUX_SEVERE,LUX_MISSINGDATA)<< "No light sources defined in scene; nothing to render.";
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
		LOG( LUX_INFO,LUX_NOERROR) << "Preprocess thread uses seed: " << seed;

		RandomGenerator rng(seed);

		// integrator preprocessing
		scene->sampler->SetFilm(scene->camera->film);
		scene->surfaceIntegrator->Preprocess(rng, *scene);
		scene->volumeIntegrator->Preprocess(rng, *scene);
		scene->camera->film->CreateBuffers();

		// Dade - to support autofocus for some camera model
		scene->camera->AutoFocus(*scene);

		sampPos = 0;

		// start the timer
		s_Timer.Start();

		// Dade - preprocessing done
		preprocessDone = true;
		scene->SetReady();

		// add a thread
		CreateRenderThread();
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

void SamplerRenderer::Pause() {
	boost::mutex::scoped_lock lock(classWideMutex);
	state = PAUSE;
	s_Timer.Stop();
}

void SamplerRenderer::Resume() {
	boost::mutex::scoped_lock lock(classWideMutex);
	state = RUN;
	s_Timer.Start();
}

void SamplerRenderer::Terminate() {
	boost::mutex::scoped_lock lock(classWideMutex);
	state = TERMINATE;
}

//------------------------------------------------------------------------------
// Statistic methods
//------------------------------------------------------------------------------

// Statistics Access
double SamplerRenderer::Statistics(const string &statName) {
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
		LOG( LUX_ERROR,LUX_BADTOKEN)<< "luxStatistics - requested an invalid data : "<< statName;
		return 0.;
	}
}

double SamplerRenderer::Statistics_GetNumberOfSamples() {
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

double SamplerRenderer::Statistics_SamplesPPx() {
	// divide by total pixels
	int xstart, xend, ystart, yend;
	scene->camera->film->GetSampleExtent(&xstart, &xend, &ystart, &yend);
	return Statistics_GetNumberOfSamples() / ((xend - xstart) * (yend - ystart));
}

double SamplerRenderer::Statistics_SamplesPSec() {
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

double SamplerRenderer::Statistics_SamplesPTotSec() {
	// Dade - s_Timer is inizialized only after the preprocess phase
	if (!preprocessDone)
		return 0.0;

	double samples = Statistics_GetNumberOfSamples();
	double time = s_Timer.Time();

	// return current samples / total elapsed secs
	return samples / time;
}

double SamplerRenderer::Statistics_Efficiency() {
	Statistics_GetNumberOfSamples(); // required before eff can be calculated.

	if (stat_Samples == 0.0)
		return 0.0;

	return (100.f * stat_blackSamples) / stat_Samples;
}

//------------------------------------------------------------------------------
// Private methods
//------------------------------------------------------------------------------

void SamplerRenderer::CreateRenderThread() {
	if (scene->IsFilmOnly())
		return;

	// Avoid to create the thread in case signal is EXIT. For instance, it
	// can happen when the rendering is done.
	if ((state == RUN) || (state == PAUSE)) {
		RenderThread *rt = new  RenderThread(renderThreads.size(), this);

		renderThreads.push_back(rt);
		rt->thread = new boost::thread(boost::bind(RenderThread::RenderImpl, rt));
	}
}

void SamplerRenderer::RemoveRenderThread() {
	if (renderThreads.size() == 0)
		return;

	renderThreads.back()->thread->interrupt();
	renderThreads.back()->thread->join();
	delete renderThreads.back();
	renderThreads.pop_back();
}

//------------------------------------------------------------------------------
// RenderThread methods
//------------------------------------------------------------------------------

SamplerRenderer::RenderThread::RenderThread(u_int index, SamplerRenderer *r) :
	n(index), renderer(r), thread(NULL), samples(0.), blackSamples(0.) {
}

SamplerRenderer::RenderThread::~RenderThread() {
}

void SamplerRenderer::RenderThread::RenderImpl(RenderThread *myThread) {
	SamplerRenderer *renderer = myThread->renderer;
	Scene &scene(*(renderer->scene));
	if (scene.IsFilmOnly())
		return;

	// To avoid interrupt excpetion
	boost::this_thread::disable_interruption di;

	Sampler *sampler = scene.sampler;
	Sample sample(scene.surfaceIntegrator, scene.volumeIntegrator, scene);
	sampler->InitSample(&sample);

	// Dade - wait the end of the preprocessing phase
	while (!renderer->preprocessDone) {
		boost::xtime xt;
		boost::xtime_get(&xt, boost::TIME_UTC);
		++xt.sec;
		boost::thread::sleep(xt);
	}

	// ContribBuffer has to wait until the end of the preprocessing
	// It depends on the fact that the film buffers have been created
	// This is done during the preprocessing phase
	sample.contribBuffer = new ContributionBuffer(scene.camera->film->contribPool);

	// initialize the thread's rangen
	u_long seed = scene.seedBase + myThread->n;
	LOG( LUX_INFO,LUX_NOERROR) << "Thread " << myThread->n << " uses seed: " << seed;

	RandomGenerator rng(seed);
	sample.camera = scene.camera->Clone();
	sample.realTime = 0.f;

	sample.rng = &rng;

	// Trace rays: The main loop
	while (true) {
		if (!sampler->GetNextSample(&sample)) {
			// Dade - we have done, check what we have to do now
			if (renderer->suspendThreadsWhenDone) {
				// Dade - wait for a resume rendering or exit
				renderer->Pause();
				while (renderer->state == PAUSE) {
					boost::xtime xt;
					boost::xtime_get(&xt, boost::TIME_UTC);
					xt.sec += 1;
					boost::thread::sleep(xt);
				}

				if (renderer->state == TERMINATE)
					break;
				else
					continue;
			} else {
				renderer->Terminate();
				break;
			}
		}

		// save ray time value
		sample.realTime = sample.camera->GetTime(sample.time);
		// sample camera transformation
		sample.camera->SampleMotion(sample.realTime);

		// Sample new SWC thread wavelengths
		sample.swl.Sample(sample.wavelengths);

		while (renderer->state == PAUSE && !boost::this_thread::interruption_requested()) {
			boost::xtime xt;
			boost::xtime_get(&xt, boost::TIME_UTC);
			xt.sec += 1;
			boost::thread::sleep(xt);
		}
		if ((renderer->state == TERMINATE) || boost::this_thread::interruption_requested())
			break;

		// Evaluate radiance along camera ray
		// Jeanphi - Hijack statistics until volume integrator revamp
		{
			const u_int nContribs = scene.surfaceIntegrator->Li(scene, sample);
			// update samples statistics
			fast_mutex::scoped_lock lockStats(myThread->statLock);
			myThread->blackSamples += nContribs;
			++(myThread->samples);
		}

		sampler->AddSample(sample);

		// Free BSDF memory from computing image sample value
		sample.arena.FreeAll();

#ifdef WIN32
		// Work around Windows bad scheduling -- Jeanphi
		myThread->thread->yield();
#endif
	}

	scene.camera->film->contribPool->End(sample.contribBuffer);
	sample.contribBuffer = NULL;

	//delete myThread->sample->camera; //FIXME deleting the camera clone would delete the film!
}

Renderer *SamplerRenderer::CreateRenderer(const ParamSet &params) {
	return new SamplerRenderer();
}

static DynamicLoader::RegisterRenderer<SamplerRenderer> r("sampler");
