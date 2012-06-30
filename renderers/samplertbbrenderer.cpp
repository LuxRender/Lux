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
#include "samplertbbrenderer.h"
#include "randomgen.h"
#include "context.h"
#include "renderers/statistics/samplerstatistics.h"
#include "tbb/tbb.h"

using namespace lux;

//------------------------------------------------------------------------------
// SRTBBDeviceDescription
//------------------------------------------------------------------------------

unsigned int SRTBBDeviceDescription::GetUsedUnitsCount() const {
	return 0; // Not implemented with TBB
}

void SRTBBDeviceDescription::SetUsedUnitsCount(const unsigned int units) {
	// not implemented with TBB
}

//------------------------------------------------------------------------------
// SRTBBHostDescription
//------------------------------------------------------------------------------

SRTBBHostDescription::SRTBBHostDescription(SamplerTBBRenderer *r, const string &n) : renderer(r), name(n) {
	SRTBBDeviceDescription *desc = new SRTBBDeviceDescription(this, "CPUs");
	devs.push_back(desc);
}

SRTBBHostDescription::~SRTBBHostDescription() {
	for (size_t i = 0; i < devs.size(); ++i)
		delete devs[i];
}

//------------------------------------------------------------------------------
// SamplerTBBRenderer
//------------------------------------------------------------------------------

SamplerTBBRenderer::SamplerTBBRenderer() : Renderer() {
	state = INIT;

	SRTBBHostDescription *host = new SRTBBHostDescription(this, "Localhost");
	hosts.push_back(host);

	preprocessDone = false;
	suspendThreadsWhenDone = false;

	AddStringConstant(*this, "name", "Name of current renderer", "sampler");

	rendererStatistics = new SRStatistics((SamplerRenderer*)this); // TODO: UGLY cast to work with SRStatistcs... This sucks
}

SamplerTBBRenderer::~SamplerTBBRenderer() {
	boost::mutex::scoped_lock lock(classWideMutex);

	delete rendererStatistics;

	if ((state != TERMINATE) && (state != INIT))
		throw std::runtime_error("Internal error: called SamplerTBBRenderer::~SamplerTBBRenderer() while not in TERMINATE or INIT state.");

	if (renderThreads.size() > 0)
		throw std::runtime_error("Internal error: called SamplerTBBRenderer::~SamplerTBBRenderer() while list of renderThread sis not empty.");

	for (size_t i = 0; i < hosts.size(); ++i)
		delete hosts[i];
}

Renderer::RendererType SamplerTBBRenderer::GetType() const {
	return SAMPLER_TYPE;
}

Renderer::RendererState SamplerTBBRenderer::GetState() const {
	boost::mutex::scoped_lock lock(classWideMutex);

	return state;
}

vector<RendererHostDescription *> &SamplerTBBRenderer::GetHostDescs() {
	boost::mutex::scoped_lock lock(classWideMutex);

	return hosts;
}

void SamplerTBBRenderer::SuspendWhenDone(bool v) {
	boost::mutex::scoped_lock lock(classWideMutex);
	suspendThreadsWhenDone = v;
}

void SamplerTBBRenderer::Render(Scene *s) {
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
		rendererStatistics->reset();
	
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

		scene->surfaceIntegrator->RequestSamples(scene->sampler, *scene);
		scene->volumeIntegrator->RequestSamples(scene->sampler, *scene);

		// Dade - to support autofocus for some camera model
		scene->camera->AutoFocus(*scene);

		sampPos = 0;
		
		// start the timer
		rendererStatistics->timer.Start();

		// Dade - preprocessing done
		preprocessDone = true;
		scene->SetReady();
	}
	
	while(true)
	{
		tbb::parallel_for(tbb::blocked_range<u_int>(0, 1000), boost::bind(&SamplerTBBRenderer::RenderImpl, this, _1));
	}
	{
		boost::mutex::scoped_lock lock(renderThreadsMutex);

		// of new threads after this point
			Terminate();

		// Flush the contribution pool
		scene->camera->film->contribPool->Flush();
		scene->camera->film->contribPool->Delete();
	}
}

void SamplerTBBRenderer::Pause() {
	boost::mutex::scoped_lock lock(classWideMutex);
	state = PAUSE;
	rendererStatistics->timer.Stop();
}

void SamplerTBBRenderer::Resume() {
	boost::mutex::scoped_lock lock(classWideMutex);
	state = RUN;
	rendererStatistics->timer.Start();
}

void SamplerTBBRenderer::Terminate() {
	boost::mutex::scoped_lock lock(classWideMutex);
	state = TERMINATE;
}

//------------------------------------------------------------------------------
// Private methods
//------------------------------------------------------------------------------

void SamplerTBBRenderer::CreateRenderThread() {
/*
	if (scene->IsFilmOnly())
		return;

	// Avoid to create the thread in case signal is EXIT. For instance, it
	// can happen when the rendering is done.
	if ((state == RUN) || (state == PAUSE)) {
		RenderThread *rt = new  RenderThread(renderThreads.size(), this);

		renderThreads.push_back(rt);
		rt->thread = new boost::thread(boost::bind(RenderThread::RenderImpl, rt));
	}
*/
}

void SamplerTBBRenderer::RemoveRenderThread() {
/*
	if (renderThreads.size() == 0)
		return;

	renderThreads.back()->thread->interrupt();
	renderThreads.back()->thread->join();
	delete renderThreads.back();
	renderThreads.pop_back();
*/
}

//------------------------------------------------------------------------------
// RenderThread methods
//------------------------------------------------------------------------------

/*
SamplerTBBRenderer::RenderThread::RenderThread(u_int index, SamplerTBBRenderer *r) :
	n(index), renderer(r), thread(NULL), samples(0.), blackSamples(0.) {
}

SamplerTBBRenderer::RenderThread::~RenderThread() {
}

SamplerTBBRenderer::RenderThread::RenderOneRay()
{


}
*/

void SamplerTBBRenderer::RenderImpl(tbb::blocked_range<u_int> const &range) {
	SamplerTBBRenderer *renderer = this;
	Scene &scene(*(renderer->scene));
	if (scene.IsFilmOnly())
		return;

	Sampler *sampler = scene.sampler;

	Sample *ss = samplePool.Get();

	if(ss == NULL)
	{
		std::cout << "alloca" << std::endl;
		ss = new Sample;
		// init a sample

		sampler->InitSample(ss);

		// ContribBuffer has to wait until the end of the preprocessing
		// It depends on the fact that the film buffers have been created
		// This is done during the preprocessing phase
		ss->contribBuffer = new ContributionBuffer(scene.camera->film->contribPool);

		// initialize the thread's rangen
		//u_long seed = scene.seedBase + myThread->n; // TODO: broken
		//LOG( LUX_INFO,LUX_NOERROR) << "Thread " << myThread->n << " uses seed: " << seed;

		u_long seed = 0;
		ss->camera = scene.camera->Clone();
		ss->realTime = 0.f;

		ss->rng = new RandomGenerator(seed); // TODO
	}
	Sample& sample = *ss;
	// Trace rays: The main loop
	for(u_int i = range.begin(); i != range.end(); ++i)
	{
		#if 1
		if (!sampler->GetNextSample(&sample)) {
			break;
			/*
			// Dade - we have done, check what we have to do now
			if (renderer->suspendThreadsWhenDone) {
				// Dade - wait for a resume rendering or exit
				renderer->Pause();
				while (renderer->state == PAUSE) {
					boost::this_thread::sleep(boost::posix_time::seconds(1));
				}

				if (renderer->state == TERMINATE)
					break;
				else
					continue;
			} else {
				renderer->Terminate();
				break;
			}
			*/

		}

		// save ray time value
		sample.realTime = sample.camera->GetTime(sample.time);
		// sample camera transformation
		sample.camera->SampleMotion(sample.realTime);

		// Sample new SWC thread wavelengths
		sample.swl.Sample(sample.wavelengths);

		while (renderer->state == PAUSE && !boost::this_thread::interruption_requested()) {
			boost::this_thread::sleep(boost::posix_time::seconds(1));
		}
		if ((renderer->state == TERMINATE) || boost::this_thread::interruption_requested())
			break;

		// Evaluate radiance along camera ray
		// Jeanphi - Hijack statistics until volume integrator revamp
		{
			const u_int nContribs = scene.surfaceIntegrator->Li(scene, sample);
			// update samples statistics TODO
			//fast_mutex::scoped_lock lockStats(myThread->statLock);
			//myThread->blackSamples += nContribs;
			//++(myThread->samples);
		}

		sampler->AddSample(sample);

		// Free BSDF memory from computing image sample value
		sample.arena.FreeAll();

/*
#ifdef WIN32
		// Work around Windows bad scheduling -- Jeanphi
		myThread->thread->yield();
#endif
*/
	#endif
	}

	samplePool.Release(&sample);
	#if 0
	scene.camera->film->contribPool->End(sample.contribBuffer);
	delete sample.contribBuffer; // TODO: added for memory sack
	//sample.contribBuffer = NULL;

	//delete myThread->sample->camera; //FIXME deleting the camera clone would delete the film!
	sampler->FreeSample(&sample);
	#endif
}

Renderer *SamplerTBBRenderer::CreateRenderer(const ParamSet &params) {
	return new SamplerTBBRenderer();
}

static DynamicLoader::RegisterRenderer<SamplerTBBRenderer> r("samplertbb");
