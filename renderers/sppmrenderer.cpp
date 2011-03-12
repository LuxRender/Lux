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
	return host->renderer->eyePassRenderThreads.size();
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

	if (eyePassRenderThreads.size() > 0)
		throw std::runtime_error("Internal error: called SPPMRenderer::~SPPMRenderer() while list of eyePassRenderThreads is not empty.");

	if (photonPassRenderThreads.size() > 0)
		throw std::runtime_error("Internal error: called SPPMRenderer::~SPPMRenderer() while list of photonPassRenderThreads is not empty.");

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
		scene->surfaceIntegrator->Preprocess(rng, *scene);
		scene->volumeIntegrator->Preprocess(rng, *scene);
		scene->camera->film->CreateBuffers();

		// Dade - to support autofocus for some camera model
		scene->camera->AutoFocus(*scene);

		sampPos = 0;

		size_t threadCount = boost::thread::hardware_concurrency();
		LOG(LUX_INFO, LUX_NOERROR) << "Hardware concurrency: " << threadCount;

		// Create synchronization barriers
		eyePassThreadBarrier = new boost::barrier(threadCount);
		photonPassThreadBarrier = new boost::barrier(threadCount);
		allThreadBarrier = new boost::barrier(2 * threadCount);
		exitBarrier = new boost::barrier(threadCount);

		// initialise
		photonTracedTotal.resize(scene->lightGroups.size(), 0);
		photonTracedPass.resize(scene->lightGroups.size(), 0);
		photonTracedPassNoLightGroup = 0;

		// start the timer
		s_Timer.Start();

		// Dade - preprocessing done
		preprocessDone = true;
		Context::GetActive()->SceneReady();

		// Start all threads
		for (size_t i = 0; i < threadCount; ++i) {
			// Start the eye pass thread
			EyePassRenderThread *eprt = new  EyePassRenderThread(i, this);

			eyePassRenderThreads.push_back(eprt);
			eprt->thread = new boost::thread(boost::bind(EyePassRenderThread::RenderImpl, eprt));

			// Start the eye pass thread
			PhotonPassRenderThread *pprt = new  PhotonPassRenderThread(i, this);

			photonPassRenderThreads.push_back(pprt);
			pprt->thread = new boost::thread(boost::bind(PhotonPassRenderThread::RenderImpl, pprt));
		}
	}

	if (eyePassRenderThreads.size() > 0) {
		// The first thread can not be removed
		// it will terminate when the rendering is finished
		eyePassRenderThreads[0]->thread->join();

		// rendering done, now I can remove all rendering threads
		{
			boost::mutex::scoped_lock lock(renderThreadsMutex);

			photonPassRenderThreads.clear();
			for (u_int i = 0; i < photonPassRenderThreads.size(); ++i) {
				photonPassRenderThreads[i]->thread->join();
				delete photonPassRenderThreads[i];
			}
			photonPassRenderThreads.clear();

			// wait for all threads to finish their job
			for (u_int i = 0; i < eyePassRenderThreads.size(); ++i) {
				eyePassRenderThreads[i]->thread->join();
				delete eyePassRenderThreads[i];
			}
			eyePassRenderThreads.clear();

			// I change the current signal to exit in order to disable the creation
			// of new threads after this point
			Terminate();
		}

		// Flush the contribution pool
		scene->camera->film->contribPool->Flush();
		scene->camera->film->contribPool->Delete();
	}

	delete allThreadBarrier;
	delete eyePassThreadBarrier;
	delete photonPassThreadBarrier;
	delete exitBarrier;
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
		return eyePassRenderThreads.size();
	else if (statName == "pass") {
		return (hitPoints) ? double(hitPoints->GetPhotonPassCount()) : 0.0;
	} else if (statName == "photonCount") {
		unsigned long long total = 0;
		for (size_t i = 0; i < photonTracedTotal.size(); ++i)
			total += photonTracedTotal[i] + photonTracedPass[i];
		return double(total);
	} else {
		LOG(LUX_ERROR,LUX_BADTOKEN)<< "luxStatistics - requested an invalid data : "<< statName;
		return 0.;
	}
}

//------------------------------------------------------------------------------
// Eye pass render thread
//------------------------------------------------------------------------------

SPPMRenderer::EyePassRenderThread::EyePassRenderThread(u_int index, SPPMRenderer *r) :
	n(index), renderer(r), thread(NULL) {
	threadRng = NULL;

	Scene &scene(*(renderer->scene));
	threadSample = new Sample(NULL, scene.volumeIntegrator, scene);
	// Initialized later
	threadSample->rng = NULL;
	threadSample->camera = scene.camera->Clone();
	threadSample->realTime = threadSample->camera->GetTime(.5f); //FIXME sample it
	threadSample->camera->SampleMotion(threadSample->realTime);
}

SPPMRenderer::EyePassRenderThread::~EyePassRenderThread() {
	delete threadSample;
	delete threadRng;
}

void SPPMRenderer::EyePassRenderThread::RenderImpl(EyePassRenderThread *myThread) {
	SPPMRenderer *renderer = myThread->renderer;
	boost::barrier *eyePassThreadBarrier = renderer->eyePassThreadBarrier;
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
	LOG(LUX_INFO, LUX_NOERROR) << "Eye pass thread " << myThread->n << " uses seed: " << seed;
	myThread->threadRng = new RandomGenerator(seed);
	myThread->threadSample->rng = myThread->threadRng;

	HitPoints *hitPoints = NULL;

	//--------------------------------------------------------------------------
	// First eye pass
	//--------------------------------------------------------------------------

	if (myThread->n == 0) {
		// One thread initialize the hit points
		renderer->hitPoints = new HitPoints(renderer, myThread->threadRng);
	}

	// Wait for other threads
	eyePassThreadBarrier->wait();

	hitPoints = renderer->hitPoints;
	hitPoints->SetHitPoints(myThread->threadRng,
			myThread->n, renderer->eyePassRenderThreads.size());

	// Wait for other threads
	eyePassThreadBarrier->wait();

	if (myThread->n == 0)
		hitPoints->Init();

	// Wait for other threads
	eyePassThreadBarrier->wait();

	// Last step of the initialization

	hitPoints->RefreshAccelParallel(myThread->n, renderer->eyePassRenderThreads.size());

	// Wait for other threads
	allThreadBarrier->wait();

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
		// Eye pass: find hit points
		//----------------------------------------------------------------------

		if (myThread->n == 0)
			hitPoints->IncEyePass();

		// Wait for other threads
		eyePassThreadBarrier->wait();

		hitPoints->SetHitPoints(myThread->threadRng,
				myThread->n, renderer->eyePassRenderThreads.size());

		// Wait for other threads
		eyePassThreadBarrier->wait();

		if (myThread->n == 0) {
			// Updating information of maxHitPointRadius2 is not thread safe
			// because HitPoint->accumPhotonRadius2 is updated by Photon Pass Threads.
			// However the only pratical result is that maxHitPointRadius2 is larger
			// than its real value. This is not a big problem when updating
			// the lookup accelerator.
			hitPoints->UpdatePointsInformation();
			hitPoints->RefreshAccelMutex();
		}

		// Wait for other threads
		eyePassThreadBarrier->wait();

		hitPoints->RefreshAccelParallel(myThread->n, renderer->eyePassRenderThreads.size());

		//----------------------------------------------------------------------
		// End of the pass
		//----------------------------------------------------------------------

		// Wait for other threads
		allThreadBarrier->wait();
	}

	// Wait for other threads
	barrierExit->wait();

	if (myThread->n == 0)
		delete hitPoints;
}

//------------------------------------------------------------------------------
// Photon pass render thread
//------------------------------------------------------------------------------

SPPMRenderer::PhotonPassRenderThread::PhotonPassRenderThread(u_int index, SPPMRenderer *r) :
	n(index), renderer(r), thread(NULL) {
	threadRng = NULL;

	Scene &scene(*(renderer->scene));
	threadSample = new Sample(NULL, scene.volumeIntegrator, scene);
	// Initialized later
	threadSample->rng = NULL;
	threadSample->camera = scene.camera->Clone();
	threadSample->realTime = threadSample->camera->GetTime(.5f); //FIXME sample it
	threadSample->camera->SampleMotion(threadSample->realTime);

	// Compute light power CDF for photon shooting
	u_int nLights = scene.lights.size();
	float *lightPower = new float[nLights];
	for (u_int i = 0; i < nLights; ++i)
		lightPower[i] = scene.lights[i]->Power(scene);
	lightCDF = new Distribution1D(lightPower, nLights);
	delete[] lightPower;
}

SPPMRenderer::PhotonPassRenderThread::~PhotonPassRenderThread() {
	delete lightCDF;
	delete threadSample;
	delete threadRng;
}

void SPPMRenderer::PhotonPassRenderThread::RenderImpl(PhotonPassRenderThread *myThread) {
	SPPMRenderer *renderer = myThread->renderer;
	boost::barrier *allThreadBarrier = renderer->allThreadBarrier;
	boost::barrier *photonPassThreadBarrier = renderer->photonPassThreadBarrier;
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
	u_long seed = scene.seedBase + myThread->n + renderer->eyePassRenderThreads.size();
	LOG(LUX_INFO, LUX_NOERROR) << "Photon pass thread " << myThread->n << " uses seed: " << seed;
	myThread->threadRng = new RandomGenerator(seed);
	myThread->threadSample->rng = myThread->threadRng;

	HitPoints *hitPoints = NULL;

	// Wait for other threads
	allThreadBarrier->wait();
	hitPoints = renderer->hitPoints;

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
		// Photon pass: trace photons
		//----------------------------------------------------------------------

		myThread->TracePhotons();

		// Wait for other threads
		photonPassThreadBarrier->wait();

		// The first thread has to do some special task for the eye pass
		if (myThread->n == 0) {
			// First thread only tasks
			for(u_int i = 0; i < renderer->photonTracedTotal.size(); ++i) {
				renderer->photonTracedTotal[i] += renderer->photonTracedPass[i];
				renderer->photonTracedPass[i] = 0;
			}
			renderer->photonTracedPassNoLightGroup = 0;
		}

		// Wait for other threads
		photonPassThreadBarrier->wait();

		hitPoints->AccumulateFlux(renderer->photonTracedTotal, myThread->n, renderer->eyePassRenderThreads.size());

		// Wait for other threads
		photonPassThreadBarrier->wait();

		if (myThread->n == 0) {
			// Update the frame buffer
			hitPoints->UpdateFilm();

			hitPoints->IncPhotonPass();
		}

		//----------------------------------------------------------------------
		// End of the pass
		//----------------------------------------------------------------------

		// Wait for other threads
		allThreadBarrier->wait();
	}

	// Wait for other threads
	barrierExit->wait();
}

void SPPMRenderer::PhotonPassRenderThread::TracePhotons() {
	Scene &scene(*(renderer->scene));
	Sample &sample(*threadSample);

	// Sample the wavelengths
	sample.swl.Sample(renderer->hitPoints->GetPhotonPassWavelengthSample());

	// Build the sample sequence
	PermutedHalton halton(7, *threadRng);
	const float haltonOffset = threadRng->floatValue();

	for (u_int photonCount = 0;; ++photonCount) {
		// Check if it is time to do an eye pass
		if (renderer->photonTracedPassNoLightGroup > renderer->sppmi->photonPerPass) {
			// Ok, time to stop
			return;
		}

		// I have to make a copy of SpectrumWavelengths because it can be modified
		// even if passed as a const argument !
		SpectrumWavelengths sw(threadSample->swl);

		// Trace a photon path and store contribution
		float u[7];
		halton.Sample(photonCount, u);
		// Add an offset to the samples to avoid to start with 0.f values
		for (int j = 0; j < 7; ++j) {
			float v = u[j] + haltonOffset;
			u[j] = (v >= 1.f) ? (v - 1.f) : v;
		}

		// This may be required by the volume integrator
		for (u_int j = 0; j < sample.n1D.size(); ++j)
			for (u_int k = 0; k < sample.n1D[j]; ++k)
				sample.oneD[j][k] = threadRng->floatValue();

		// Choose light to shoot photon from
		float lightPdf;
		u_int lightNum = lightCDF->SampleDiscrete(u[6], &lightPdf);
		const Light *light = scene.lights[lightNum];

		osAtomicInc(&(renderer->photonTracedPass[light->group]));
		osAtomicInc(&(renderer->photonTracedPassNoLightGroup));

		// Generate _photonRay_ from light source and initialize _alpha_
		BSDF *bsdf;
		float pdf;
		SWCSpectrum alpha;
		if (!light->SampleL(scene, sample, u[0], u[1], u[2],
				&bsdf, &pdf, &alpha))
			continue;
		Ray photonRay;
		photonRay.o = bsdf->dgShading.p;
		float pdf2;
		SWCSpectrum alpha2;
		if (!bsdf->SampleF(sw, Vector(bsdf->nn), &photonRay.d,
				u[3], u[4], u[5], &alpha2, &pdf2))
			continue;
		alpha *= alpha2;
		alpha /= lightPdf;

		if (!alpha.Black()) {
			// Follow photon path through scene and record intersections
			Intersection photonIsect;
			const Volume *volume = NULL; //FIXME: try to get volume from light
			BSDF *photonBSDF;
			u_int nIntersections = 0;
			while (scene.Intersect(sample, volume, false,
				photonRay, 1.f, &photonIsect, &photonBSDF,
				NULL, NULL, &alpha)) {
				++nIntersections;

				// Handle photon/surface intersection
				Vector wo = -photonRay.d;

				// Deposit Flux
				renderer->hitPoints->AddFlux(photonIsect.dg.p, wo, sw, alpha, light->group);

				// Sample new photon ray direction
				Vector wi;
				float pdfo;
				BxDFType flags;
				// Get random numbers for sampling outgoing photon direction
				const float u1 = threadRng->floatValue();
				const float u2 = threadRng->floatValue();
				const float u3 = threadRng->floatValue();

				// Compute new photon weight and possibly terminate with RR
				SWCSpectrum fr;
				if (!photonBSDF->SampleF(sw, wo, &wi, u1, u2, u3, &fr, &pdfo, BSDF_ALL, &flags))
					break;

				// Russian Roulette
				SWCSpectrum anew = fr;
				float continueProb = min(1.f, anew.Filter(sw));
				if ((threadRng->floatValue() > continueProb) ||
						(nIntersections > renderer->sppmi->maxPhotonPathDepth))
					break;

				alpha *= anew / continueProb;
				photonRay = Ray(photonIsect.dg.p, wi);
				volume = photonBSDF->GetVolume(photonRay.d);
			}
		}

		sample.arena.FreeAll();
	}
}

//------------------------------------------------------------------------------

Renderer *SPPMRenderer::CreateRenderer(const ParamSet &params) {
	return new SPPMRenderer();
}

static DynamicLoader::RegisterRenderer<SPPMRenderer> r("sppm");
