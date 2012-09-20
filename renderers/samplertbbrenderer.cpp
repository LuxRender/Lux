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
#include "renderers/statistics/samplertbbstatistics.h"
#include "tbb/tbb.h"

using namespace lux;

//------------------------------------------------------------------------------
// SRTBBDeviceDescription
//------------------------------------------------------------------------------

unsigned int SRTBBDeviceDescription::GetUsedUnitsCount() const {
	boost::mutex::scoped_lock lock(host->renderer->classWideMutex);
	return host->renderer->numberOfThreads;
}

void SRTBBDeviceDescription::SetUsedUnitsCount(const unsigned int units) {
	boost::mutex::scoped_lock lock(host->renderer->classWideMutex);
	host->renderer->numberOfThreads = max(units, 1u);
	host->renderer->mustChangeNumberOfThreads = true;
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

	suspendThreadsWhenDone = false;

	AddStringConstant(*this, "name", "Name of current renderer", "sampler");

	rendererStatistics = new SRTBBStatistics(this);
	numberOfThreads = 1;
}

SamplerTBBRenderer::~SamplerTBBRenderer() {
	boost::mutex::scoped_lock lock(classWideMutex);

	delete rendererStatistics;

	if ((state != TERMINATE) && (state != INIT))
		throw std::runtime_error("Internal error: called SamplerTBBRenderer::~SamplerTBBRenderer() while not in TERMINATE or INIT state.");

	for (size_t i = 0; i < hosts.size(); ++i)
		delete hosts[i];

	delete localStoragePool;
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
	

		// initialize the thread's rangen
		u_long seed = scene->seedBase - 1;
		LOG( LUX_INFO,LUX_NOERROR) << "Preprocess thread uses seed: " << seed;

		rng = new RandomGenerator(seed);

		// integrator preprocessing
		scene->sampler->SetFilm(scene->camera->film);
		scene->surfaceIntegrator->Preprocess(*rng, *scene);
		scene->volumeIntegrator->Preprocess(*rng, *scene);
		scene->camera->film->CreateBuffers();

		scene->surfaceIntegrator->RequestSamples(scene->sampler, *scene);
		scene->volumeIntegrator->RequestSamples(scene->sampler, *scene);

		// Dade - to support autofocus for some camera model
		scene->camera->AutoFocus(*scene);

		// start the timer
		rendererStatistics->start();

		// Dade - preprocessing done
		scene->SetReady();
	}

	localStoragePool = new LocalStoragePool(boost::bind(&LocalStorageCreate, this));

	tbb::task_scheduler_init* tsi = new tbb::task_scheduler_init;

	Impl impl(this);
	while(state != TERMINATE)
	{
		// Parallel do needs an iterable to iterate over, value will do the work with only one item
		// each task will submit more items
		unsigned int value;
		tbb::parallel_do(&value, &value + 1, impl);

		// we exited because we must be in pause
		while (state == PAUSE) {
			boost::this_thread::sleep(boost::posix_time::seconds(1));
		}

		// perhaps we must change the number of threads
		// TBB TODO: put this in the main program
		if(mustChangeNumberOfThreads)
		{
			mustChangeNumberOfThreads = false;
			tsi->terminate();
			delete tsi;
			tsi= new tbb::task_scheduler_init(numberOfThreads);
		}
	}
	tsi->terminate();
	delete tsi;;

	localStoragePool->clear(); // Ends the local storage and ends the Contribution stored inside

	{
		Terminate();

		// Flush the contribution pool
		scene->camera->film->contribPool->Flush();
		scene->camera->film->contribPool->Delete();
	}
}

void SamplerTBBRenderer::Pause() {
	boost::mutex::scoped_lock lock(classWideMutex);
	state = PAUSE;
	rendererStatistics->stop();
}

void SamplerTBBRenderer::Resume() {
	boost::mutex::scoped_lock lock(classWideMutex);
	state = RUN;
	rendererStatistics->start();
}

void SamplerTBBRenderer::Terminate() {
	boost::mutex::scoped_lock lock(classWideMutex);
	state = TERMINATE;
}

// This create a sample for each thread on runtime
SamplerTBBRenderer::LocalStorage SamplerTBBRenderer::LocalStorageCreate(SamplerTBBRenderer *renderer)
{
	SamplerTBBRenderer::LocalStorage storage;
	storage.sample = new Sample;

	Scene* scene = renderer->scene;
	storage.renderer = renderer;
	scene->sampler->InitSample(storage.sample);

	// ContribBuffer has to wait until the end of the preprocessing
	// It depends on the fact that the film buffers have been created
	// This is done during the preprocessing phase
	storage.sample->contribBuffer = new ContributionBuffer(scene->camera->film->contribPool);

	u_long seed;
	// initialize the thread's rangen
	{
		boost::mutex::scoped_lock lock(renderer->classWideMutex);
		seed = renderer->rng->uintValue();
		LOG( LUX_INFO,LUX_NOERROR) << "Thread uses seed: " << seed;
	}

	storage.sample->camera = scene->camera->Clone();
	storage.sample->realTime = 0.f;

	storage.sample->rng = new RandomGenerator(seed);

	storage.blackSamples = 0.;
	storage.samples = 0.;
	storage.blackSamplePaths = 0.;

	return storage;
}

// Called when the thread local storage is destroy
// TBB TODO: what happen if someone is limiting the number of threads ? The
// local storage keeps the Sample and never contribute them until the end of
// the rendering
SamplerTBBRenderer::LocalStorage::~LocalStorage()
{
	renderer->scene->camera->film->contribPool->End(sample->contribBuffer);
	sample->contribBuffer = NULL;

	//delete myThread->sample->camera; //FIXME deleting the camera clone would delete the film!
	renderer->scene->sampler->FreeSample(sample);
}


void SamplerTBBRenderer::Impl::operator()(unsigned int i, tbb::parallel_do_feeder<unsigned int>& feeder) const {
	// ask the scheduler to launch a new ray, only if we are not paused
	if(renderer->state == RUN && !renderer->mustChangeNumberOfThreads)
		feeder.add(0);
	else
		return;

	Scene &scene(*(renderer->scene));
	if (scene.IsFilmOnly())
		return;

	Sampler *sampler = scene.sampler;

	LocalStoragePool::reference local = renderer->localStoragePool->local();

	Sample& sample = *local.sample;
	// Trace rays: The main loop
	// TODO: perhaps here we can make an infinite loop too, this may remove some load on TBB
	{
		if (!sampler->GetNextSample(&sample)) {
			// Dade - we have done, check what we have to do now
			if (renderer->suspendThreadsWhenDone)
				renderer->Pause(); // This will pause all threads
			else
				renderer->Terminate();

			return;
		}

		// save ray time value
		sample.realTime = sample.camera->GetTime(sample.time);
		// sample camera transformation
		sample.camera->SampleMotion(sample.realTime);

		// Sample new SWC thread wavelengths
		sample.swl.Sample(sample.wavelengths);

		// Evaluate radiance along camera ray
		// Jeanphi - Hijack statistics until volume integrator revamp
		{
			const u_int nContribs = scene.surfaceIntegrator->Li(scene, sample);
			// update samples statistics
			//fast_mutex::scoped_lock lockStats(local.statLock); // TODO: tbb need this lock ?
			local.blackSamples += nContribs;
			if(nContribs > 0)
				++local.blackSamplePaths;
			++local.samples;
		}

		sampler->AddSample(sample);

		// Free BSDF memory from computing image sample value
		sample.arena.FreeAll();

/*
 * TBB TODO: is this needed ?
#ifdef WIN32
		// Work around Windows bad scheduling -- Jeanphi
		myThread->thread->yield();
#endif
*/
	}
}

Renderer *SamplerTBBRenderer::CreateRenderer(const ParamSet &params) {
	return new SamplerTBBRenderer();
}

static DynamicLoader::RegisterRenderer<SamplerTBBRenderer> r("samplertbb");
