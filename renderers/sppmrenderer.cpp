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

/*
 * A scene, to be rendered with SPPM, must have:
 *
 * Renderer "sppm"
 * SurfaceIntegrator "sppm"
 *
 */

#include "api.h"
#include "scene.h"
#include "camera.h"
#include "film.h"
#include "sampling.h"
#include "randomgen.h"
#include "context.h"
#include "light.h"
#include "mc.h"
#include "mcdistribution.h"
#include "spectrumwavelengths.h"
#include "reflection/bxdf.h"
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

	hitPoints = NULL;

	AddStringConstant(*this, "name", "Name of current renderer", "sppm");
}

SPPMRenderer::~SPPMRenderer() {
	boost::mutex::scoped_lock lock(classWideMutex);

	if ((state != TERMINATE) && (state != INIT))
		throw std::runtime_error("Internal error: called SPPMRenderer::~SPPMRenderer() while not in TERMINATE or INIT state.");

	if (renderThreads.size() > 0)
		throw std::runtime_error("Internal error: called SPPMRenderer::~SPPMRenderer() while list of RenderThreads is not empty.");

	for (size_t i = 0; i < hosts.size(); ++i)
		delete hosts[i];
}

Renderer::RendererType SPPMRenderer::GetType() const {
	boost::mutex::scoped_lock lock(classWideMutex);

	return SPPM_TYPE;
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

		sppmi = dynamic_cast<SPPMIntegrator*>(scene->surfaceIntegrator);
		if (!sppmi) {
			LOG(LUX_SEVERE,LUX_CONSISTENCY)<< "SPPM renderer requires the SPPM integrator.";
			return;
		}

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
		s_Timer.Reset();
	
		// Dade - I have to do initiliaziation here for the current thread.
		// It can be used by the Preprocess() methods.

		// initialize the thread's rangen
		u_long seed = scene->seedBase - 1;
		LOG(LUX_INFO, LUX_NOERROR) << "Preprocess thread uses seed: " << seed;

		RandomGenerator rng(seed);

		// integrator preprocessing
		// sppm integrator will create film buffers
		scene->surfaceIntegrator->Preprocess(rng, *scene);
		scene->volumeIntegrator->Preprocess(rng, *scene);

		// Told each Buffer how to scale things
		for(u_int bg = 0; bg < scene->camera->film->GetNumBufferGroups(); ++bg)
		{
			PerScreenNormalizedBufferScaled * buffer= dynamic_cast<PerScreenNormalizedBufferScaled *>(scene->camera->film->GetBufferGroup(bg).getBuffer(sppmi->bufferPhotonId));
			buffer->scaleUpdate = new ScaleUpdaterSPPM(this);
		}

		// Dade - to support autofocus for some camera model
		scene->camera->AutoFocus(*scene);

		sampPos = 0;

		const size_t threadCount = boost::thread::hardware_concurrency();
		LOG(LUX_INFO, LUX_NOERROR) << "Hardware concurrency: " << threadCount;

		// Create synchronization barriers
		allThreadBarrier = new boost::barrier(threadCount);
		exitBarrier = new boost::barrier(threadCount);

		// initialise
		photonTracedTotal = 0;
		photonTracedPass = 0;
		photonHitEfficiency = 0;

		// start the timer
		s_Timer.Start();

		// Dade - preprocessing done
		preprocessDone = true;
		Context::GetActive()->SceneReady();

		// Start all threads
		for (size_t i = 0; i < threadCount; ++i) {
			// Start the threads
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

	delete allThreadBarrier;
	delete exitBarrier;
}

void SPPMRenderer::Pause() {
	boost::mutex::scoped_lock lock(classWideMutex);
	state = PAUSE;
	s_Timer.Stop();
}

void SPPMRenderer::Resume() {
	boost::mutex::scoped_lock lock(classWideMutex);
	state = RUN;
	s_Timer.Start();
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
		return 0.0;
	else if(statName=="samplesTotSec")
		return 0.0;
	else if(statName=="samplesPx")
		return 0.0;
	else if(statName=="efficiency")
		return 0.0;
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
	else if (statName == "pass") {
		return (hitPoints) ? double(hitPoints->GetPassCount()) : 0.0;
	} else if (statName == "photonCount") {
		return double(photonTracedTotal + photonTracedPass);
	} else if (statName == "photonPerSecond")
	{
		int delta = photonTracedPass - last_pps_photons;
		double new_time = osWallClockTime();
		if(delta < 0)
			return 0.0;

		double res =  delta / (new_time - last_pps_time);
		last_pps_photons = photonTracedPass;
		last_pps_time = osWallClockTime();

		return res;
	} else if (statName == "hitPointsUpdateEfficiency") {
		return photonHitEfficiency;
	} else {
		LOG(LUX_ERROR,LUX_BADTOKEN)<< "luxStatistics - requested an invalid data : "<< statName;
		return 0.;
	}
}

//------------------------------------------------------------------------------
// Render thread
//------------------------------------------------------------------------------

SPPMRenderer::RenderThread::RenderThread(u_int index, SPPMRenderer *r) :
	n(index), renderer(r), thread(NULL) {
	threadRng = NULL;
	
	// Compute light power CDF for photon shooting
	u_int nLights = renderer->scene->lights.size();
	float *lightPower = new float[nLights];
	for (u_int i = 0; i < nLights; ++i)
		lightPower[i] = renderer->scene->lights[i]->Power(*renderer->scene);
	lightCDF = new Distribution1D(lightPower, nLights);
	delete[] lightPower;
}

SPPMRenderer::RenderThread::~RenderThread() {
	delete threadRng;
	delete lightCDF;
}

void SPPMRenderer::RenderThread::RenderImpl(RenderThread *myThread) {
	SPPMRenderer *renderer = myThread->renderer;
	boost::barrier *allThreadBarrier = renderer->allThreadBarrier;
	boost::barrier *barrierExit = renderer->exitBarrier;

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

	// Wait for other threads
	allThreadBarrier->wait();

	// Initialize the thread's rangen
	u_long seed = scene.seedBase + myThread->n;
	LOG(LUX_INFO, LUX_NOERROR) << "Render thread " << myThread->n << " uses seed: " << seed;
	myThread->threadRng = new RandomGenerator(seed);
	
	// Initialize the photon sampler
	PhotonSampler * &sampler = myThread->sampler;
	switch (renderer->sppmi->photonSamplerType) {
		case HALTON:
			sampler = new HaltonPhotonSampler(renderer);
			break;
		case AMC:
			sampler = new AMCMCPhotonSampler(renderer);
			break;
		default:
			throw std::runtime_error("Internal error: unknown photon sampler");
	}

	// Initialize the photon sample
	Sample sample;
	sample.contribBuffer = new ContributionBuffer(scene.camera->film->contribPool);
	// The RNG might be used when initializing the sampler data below
	sample.rng = myThread->threadRng;
//	sample.camera = scene.camera->Clone(); // Unneeded for photons
	// sample.realTime and sample.swl are intialized later
	// Describe sampling data
	sample.Add1D(1); // light sampling
	sample.Add2D(1); // light position sampling
	sample.Add1D(1); // light position portal sampling
	sample.Add2D(1); // light direction sampling
	sample.Add1D(1); // light direction portal sampling
	vector<u_int> structure;
	structure.push_back(2); // BSDF direction sampling
	structure.push_back(1); // BSDF component sampling
	structure.push_back(1); // RR sampling
	sample.AddxD(structure, renderer->sppmi->maxPhotonPathDepth + 1);
	renderer->scene->volumeIntegrator->RequestSamples(&sample, *(renderer->scene));
	sampler->InitSample(&sample);

	// initialise the eye sample
	Sample eyeSample;
	eyeSample.contribBuffer = new ContributionBuffer(scene.camera->film->contribPool);
	eyeSample.camera = scene.camera->Clone();
	eyeSample.realTime = 0.f;
	eyeSample.rng = myThread->threadRng;

	structure.clear();
	structure.push_back(1);	// volume scattering
	structure.push_back(2);	// bsdf sampling direction
	structure.push_back(1);	// bsdf sampling component
	structure.push_back(1);	// bsdf bouncing/storing component
	eyeSample.AddxD(structure, renderer->sppmi->maxEyePathDepth + 1);
	renderer->scene->volumeIntegrator->RequestSamples(&eyeSample, *(renderer->scene));

	renderer->sppmi->hints.RequestSamples(&eyeSample, scene, renderer->sppmi->maxPhotonPathDepth + 1);

	//--------------------------------------------------------------------------
	// First eye pass
	//--------------------------------------------------------------------------

	if (myThread->n == 0) {
		// One thread initialize the hit points
		renderer->hitPoints = new HitPoints(renderer, myThread->threadRng);
	}

	// Wait for other threads
	allThreadBarrier->wait();

	HitPoints *hitPoints = renderer->hitPoints;

	hitPoints->eyeSampler->InitSample(&eyeSample);

	double eyePassStartTime = 0.0;
	if (myThread->n == 0)
		eyePassStartTime = osWallClockTime();

	// Set hitpoints
	hitPoints->SetHitPoints(eyeSample, myThread->threadRng,
			myThread->n, renderer->renderThreads.size(), myThread->eyePassMemoryArena);

	allThreadBarrier->wait();

	renderer->paused(); // no return because we are not in the loop

	if (myThread->n == 0)
		hitPoints->Init();

	// Trace rays: The main loop
	while (true) {
		allThreadBarrier->wait();

		if(renderer->paused())
			break;

		if (myThread->n == 0) {
			// Updating information of maxHitPointRadius2 is not thread safe
			// because HitPoint->accumPhotonRadius2 is updated by Photon Pass Threads.
			// However the only pratical result is that maxHitPointRadius2 is larger
			// than its real value. This is not a big problem when updating
			// the lookup accelerator.
			hitPoints->UpdatePointsInformation();
			hitPoints->RefreshAccelMutex();
		}
		// Wait for photon pass
		allThreadBarrier->wait();

		if(renderer->paused())
			break;

		hitPoints->RefreshAccelParallel(myThread->n, renderer->renderThreads.size());
		// Wait for photon pass
		allThreadBarrier->wait();

		if (myThread->n == 0) {
			const double eyePassTime = osWallClockTime() - eyePassStartTime;
			LOG(LUX_INFO, LUX_NOERROR) << "Eye pass time: " << eyePassTime << "secs";
		}

		if(renderer->paused())
			break;

		double photonPassStartTime = 0.0;
		if (myThread->n == 0)
		{
			photonPassStartTime = osWallClockTime();
			renderer->last_pps_photons = renderer->photonTracedPass;
			renderer->last_pps_time = osWallClockTime();
		}
		
		// Initialize new wavelengths and time
		sample.wavelengths = hitPoints->GetWavelengthSample();
		sample.time = hitPoints->GetTimeSample();
		sample.swl.Sample(sample.wavelengths);
		sample.realTime = scene.camera->GetTime(sample.time);
//		sample.camera->SampleMotion(sample.realTime); // Unneeded for photons

		//--------------------------------------------------------------
		// Photon pass: trace photons
		//--------------------------------------------------------------
		sampler->TracePhotons(&sample, myThread->lightCDF);

		// Wait for other threads
		allThreadBarrier->wait();

		// The first thread has to do some special task for the eye pass
		if (myThread->n == 0) {
			renderer->photonHitEfficiency = renderer->hitPoints->GetPhotonHitEfficency();

			// First thread only tasks
			renderer->photonTracedTotal += renderer->photonTracedPass;
			renderer->photonTracedPass = 0;
		}

		// Wait for other threads
		allThreadBarrier->wait();

		hitPoints->AccumulateFlux(myThread->n, renderer->renderThreads.size());

		// Wait for other threads
		allThreadBarrier->wait();

		if(renderer->paused())
			break;

		if (myThread->n == 0) {
			hitPoints->IncPass();

			// Check for termination
			int passCount = luxStatistics("pass");
			int hltSpp = scene.camera->film->haltSamplesPerPixel;
			if(hltSpp > 0){
				if(passCount == hltSpp){
					renderer->Terminate();
				}
			}

			int secsElapsed = luxStatistics("secElapsed");
			int hltTime = scene.camera->film->haltTime;
			if(hltTime > 0){
				if(secsElapsed > hltTime){
					renderer->Terminate();
				}
			}
			const double photonPassTime = osWallClockTime() - photonPassStartTime;
			LOG(LUX_INFO, LUX_NOERROR) << "Photon pass time: " << photonPassTime << "secs";
		}

		if(renderer->paused())
			break;

		// Wait for other threads
		allThreadBarrier->wait();

		if (myThread->n == 0)
			eyePassStartTime = osWallClockTime();

		hitPoints->SetHitPoints(eyeSample, myThread->threadRng,
				myThread->n, renderer->renderThreads.size(), myThread->eyePassMemoryArena);
	}

	scene.camera->film->contribPool->End(sample.contribBuffer);
	scene.camera->film->contribPool->End(eyeSample.contribBuffer);
	sample.contribBuffer = NULL;
	eyeSample.contribBuffer = NULL;

	sampler->FreeSample(&sample);
	hitPoints->eyeSampler->FreeSample(&eyeSample);

	//delete sample.camera; //FIXME deleting the camera clone would delete the film!
	//delete eyeSample.camera; //FIXME deleting the camera clone would delete the film!
	delete sampler;

	// Wait for other threads
	barrierExit->wait();

	if (myThread->n == 0)
		delete hitPoints;
}

Renderer *SPPMRenderer::CreateRenderer(const ParamSet &params) {
	return new SPPMRenderer();
}

float SPPMRenderer::GetScaleFactor() const
{
	if (sppmi->photonSamplerType == AMC) {
		u_int uniformCount = 0;
		for (u_int i = 0; i < renderThreads.size(); ++i)
			uniformCount += dynamic_cast<AMCMCPhotonSampler*>(renderThreads[i]->sampler)->uniformCount;

		return uniformCount / ((float)photonTracedTotal + (float)photonTracedPass);
	} else
		return 1.f;
}

static DynamicLoader::RegisterRenderer<SPPMRenderer> r("sppm");
