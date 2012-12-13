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

class ChunkTimer
{
/* A container which stores a chunkSize for any tbb parallel_for
 *
 * the value is set in the constructor and read by the *chunkSize* attribute.
 *
 * call Start() / Stop() around your parallel_for using the *chunkSize* value.
 * if latency is != 0, the chunkSize value will be automatically adjusted to
 * ensure that the next execution of the parallel_for will run for approximatly
 * *latency* seconds.
 */
public:
	ChunkTimer(unsigned int initialChunkSize, float latency_):
		chunkSize(initialChunkSize),
		latency(latency_)
	{
	}

	void Start()
	{
		if(latency > 0)
		{
			timer.Reset();
			timer.Start();
		}
	}

	void Stop()
	{
		if(latency > 0)
		{
			timer.Stop();
			double ellapsed = timer.Time();

			if(ellapsed != 0)
			{
				double ratio_target = latency / ellapsed;
				chunkSize *= ratio_target;
			}
			else
			{
				// if the chunksize value is too small, elapsed time may be 0
				// depending on the clock resolution.
				//
				// in this case, higly increase the chunkSize. +1 here is to
				// ensure than chuckSize is not 0.
				//
				chunkSize = (chunkSize + 1) * 1000;
			}
			//std::cout << "Elapsed:" << ellapsed << " ChunkSize:" << chunkSize << std::endl;
		}
	}

	Timer timer;
	unsigned int chunkSize;
	float latency;
};

//------------------------------------------------------------------------------
// SamplerTBBRenderer
//------------------------------------------------------------------------------

SamplerTBBRenderer::SamplerTBBRenderer() : Renderer() {
	state = INIT;

	SRTBBHostDescription *host = new SRTBBHostDescription(this, "Localhost");
	hosts.push_back(host);

	suspendThreadsWhenDone = false;

	AddStringConstant(*this, "name", "Name of current renderer", "sampler");

	rendererStatistics = new SRStatistics<SamplerTBBRenderer>(this);
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

static void writeIntervalCheck(Film *film) {
	if (!film)
		return;

	while (!boost::this_thread::interruption_requested()) {
		try {
			boost::this_thread::sleep(boost::posix_time::seconds(1));

			film->CheckWriteOuputInterval();
		} catch(boost::thread_interrupted&) {
			break;
		}
	}
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
		LOG( LUX_DEBUG,LUX_NOERROR) << "Preprocess thread uses seed: " << seed;

		rng = new RandomGenerator(seed);

		// integrator preprocessing
		scene->sampler->SetFilm(scene->camera()->film);
		scene->surfaceIntegrator->Preprocess(*rng, *scene);
		scene->volumeIntegrator->Preprocess(*rng, *scene);
		scene->camera()->film->CreateBuffers();

		scene->surfaceIntegrator->RequestSamples(scene->sampler, *scene);
		scene->volumeIntegrator->RequestSamples(scene->sampler, *scene);

		// Dade - to support autofocus for some camera model
		scene->camera()->AutoFocus(*scene);

		// start the timer
		rendererStatistics->start();

		// Dade - preprocessing done
		scene->SetReady();
	}

	// thread for checking write interval
	boost::thread writeIntervalThread = boost::thread(boost::bind(writeIntervalCheck, scene->camera()->film));

	localStoragePool = new LocalStoragePool(boost::bind(&LocalStorageCreate, this));

	tbb::task_scheduler_init* tsi = new tbb::task_scheduler_init;

	Impl impl(this);
	ChunkTimer timer(chunkSize, pauseLatency);
	while(state != TERMINATE)
	{
		timer.Start();
		tbb::parallel_for(tbb::blocked_range<unsigned int>(0, timer.chunkSize), impl);
		timer.Stop();

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

	writeIntervalThread.interrupt();
	// possibly wait for writing to finish
	writeIntervalThread.join();
	{
		Terminate();

		// Flush the contribution pool
		scene->camera()->film->contribPool->Flush();
		scene->camera()->film->contribPool->Delete();
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
	storage.sample->contribBuffer = new ContributionBuffer(scene->camera()->film->contribPool);

	u_long seed;
	// initialize the thread's rangen
	{
		boost::mutex::scoped_lock lock(renderer->classWideMutex);
		seed = renderer->rng->uintValue();
		LOG( LUX_DEBUG,LUX_NOERROR) << "Thread uses seed: " << seed;
	}

	storage.sample->camera = scene->camera()->Clone();
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
	renderer->scene->camera()->film->contribPool->End(sample->contribBuffer);
	// don't delete contribBuffer as references are held in the pool
	sample->contribBuffer = NULL;

	renderer->scene->sampler->FreeSample(sample);
}


void SamplerTBBRenderer::Impl::operator()(const tbb::blocked_range<unsigned int> &r) const {
	Scene &scene(*(renderer->scene));
	if (scene.IsFilmOnly())
		return;

	Sampler *sampler = scene.sampler;

	LocalStoragePool::reference local = renderer->localStoragePool->local();

	Sample& sample = *local.sample;
	// Trace rays: The main loop
	for(unsigned int i = r.begin(); i < r.end(); ++i)
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
	SamplerTBBRenderer *renderer = new SamplerTBBRenderer();

	// chunksize is the number of ray launched between pauses. An high value
	// reduce threading overhead but increase pause latency.
	renderer->chunkSize = params.FindOneInt("chunksize", 10000);
	// Targeted pause latency, if different from 0, the chunk size will be
	// automatically adapted to target this latency.
	renderer->pauseLatency = params.FindOneFloat("pauselatency", 1.0);
	return renderer;
}

static DynamicLoader::RegisterRenderer<SamplerTBBRenderer> r("samplertbb");
