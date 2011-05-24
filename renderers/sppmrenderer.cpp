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
		return double(photonTracedTotal + photonTracedPass);
	} else if (statName == "hitPointsUpdateEfficiency") {
		return photonHitEfficiency;
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

		double passStartTime = 0.0;
		if (myThread->n == 0)
			passStartTime = osWallClockTime();

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

		if (myThread->n == 0) {
			const double photonPassTime = osWallClockTime() - passStartTime;
			LOG(LUX_INFO, LUX_NOERROR) << "Eye pass time: " << photonPassTime << "secs";
		}

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

	// Initialize the photon sampler
	PhotonSampler *sampler;
	switch (renderer->sppmi->photonSamplerType) {
		case HALTON:
			sampler = new HaltonPhotonSampler(scene, myThread->threadRng);
			break;
		case AMC:
			sampler = new AMCMCPhotonSampler(renderer->sppmi->maxPhotonPathDepth, scene, myThread->threadRng);
			break;
		default:
			throw std::runtime_error("Internal error: unknown photon sampler");
	}

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

		double passStartTime = 0.0;
		if (myThread->n == 0)
			passStartTime = osWallClockTime();

		//----------------------------------------------------------------------
		// Photon pass: trace photons
		//----------------------------------------------------------------------

		switch (renderer->sppmi->photonSamplerType) {
			case HALTON:
				myThread->TracePhotons((HaltonPhotonSampler *)sampler);
				break;
			case AMC:
				myThread->TracePhotons((AMCMCPhotonSampler *)sampler);
				break;
			default:
				throw std::runtime_error("Internal error: unknown photon sampler");
		}

		// Wait for other threads
		photonPassThreadBarrier->wait();

		// The first thread has to do some special task for the eye pass
		if (myThread->n == 0) {
			renderer->photonHitEfficiency = renderer->hitPoints->GetPhotonHitEfficency();

			if (renderer->sppmi->photonSamplerType == AMC) {
				u_int uniformCount = 0;
				for (u_int i = 0; i < renderer->photonPassRenderThreads.size(); ++i)
					uniformCount += renderer->photonPassRenderThreads[i]->amcUniformCount;

				renderer->accumulatedFluxScale = uniformCount / (float)renderer->photonTracedPass;
			} else
				renderer->accumulatedFluxScale = 1.f;

			// First thread only tasks
			renderer->photonTracedTotal += renderer->photonTracedPass;
			renderer->photonTracedPass = 0;
		}

		// Wait for other threads
		photonPassThreadBarrier->wait();

		hitPoints->AccumulateFlux(myThread->n, renderer->eyePassRenderThreads.size());

		// Wait for other threads
		photonPassThreadBarrier->wait();

		if (myThread->n == 0) {
			// Update the frame buffer
			hitPoints->UpdateFilm(renderer->accumulatedFluxScale, renderer->photonTracedTotal);
			// WARNING: Above UpdateFilm method use the current photon pass
			// index + 1. If you move the following line (IncPhotonPass),
			// please update UpdateFilm
			hitPoints->IncPhotonPass();

			const double photonPassTime = osWallClockTime() - passStartTime;
			LOG(LUX_INFO, LUX_NOERROR) << "Photon pass time: " << photonPassTime << "secs";
		}

		//----------------------------------------------------------------------
		// End of the pass
		//----------------------------------------------------------------------

		// Wait for other threads
		allThreadBarrier->wait();
	}

	delete sampler;

	// Wait for other threads
	barrierExit->wait();
}

//------------------------------------------------------------------------------
// Tracing photons for Halton Photon Sampler
//------------------------------------------------------------------------------

void SPPMRenderer::PhotonPassRenderThread::TracePhotons(HaltonPhotonSampler *sampler) {
	Scene &scene(*(renderer->scene));
	
	Sample *sample = sampler->StartNewPhotonPass(renderer->hitPoints->GetPhotonPassWavelengthSample());

	for (u_int photonCount = 0;; ++photonCount) {
		// Check if it is time to do an eye pass
		if (renderer->photonTracedPass > renderer->sppmi->photonPerPass) {
			// Ok, time to stop
			return;
		}

		sampler->StartNewPhotonPath();

		// I have to make a copy of SpectrumWavelengths because it can be modified
		// even if passed as a const argument !
		SpectrumWavelengths sw(sample->swl);

		// Trace a photon path and store contribution
		float u[7];
		sampler->GetLightData(photonCount, u);

		// Choose light to shoot photon from
		float lightPdf;
		u_int lightNum = lightCDF->SampleDiscrete(u[6], &lightPdf);
		const Light *light = scene.lights[lightNum];

		osAtomicInc(&renderer->photonTracedPass);

		// Generate _photonRay_ from light source and initialize _alpha_
		BSDF *bsdf;
		float pdf;
		SWCSpectrum alpha;
		if (!light->SampleL(scene, *sample, u[0], u[1], u[2],
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
			while (scene.Intersect(*sample, volume, false,
				photonRay, 1.f, &photonIsect, &photonBSDF,
				NULL, NULL, &alpha)) {
				++nIntersections;

				// Handle photon/surface intersection
				Vector wi = -photonRay.d;

				// Deposit Flux (only if we have hit a diffuse or glossy surface)
				if (photonBSDF->NumComponents(BxDFType(BSDF_REFLECTION | BSDF_TRANSMISSION | BSDF_GLOSSY | BSDF_DIFFUSE)) > 0)
					renderer->hitPoints->AddFlux(photonIsect.dg.p, *photonBSDF, wi, sw, alpha, light->group);

				// Sample new photon ray direction
				Vector wo;
				float pdfo;
				BxDFType flags;
				// Get random numbers for sampling outgoing photon direction
				float u1, u2, u3;
				sampler->GetPathVertexData(&u1, &u2, &u3);

				// Compute new photon weight and possibly terminate with RR
				SWCSpectrum fr;
				if (!photonBSDF->SampleF(sw, wi, &wo, u1, u2, u3, &fr, &pdfo, BSDF_ALL, &flags))
					break;

				// Russian Roulette
				SWCSpectrum anew = fr;
				const float continueProb = min(1.f, anew.Filter(sw));
				if ((sampler->GetPathVertexRRData() > continueProb) ||
						(nIntersections > renderer->sppmi->maxPhotonPathDepth))
					break;

				alpha *= anew / continueProb;
				photonRay = Ray(photonIsect.dg.p, wo);
				volume = photonBSDF->GetVolume(photonRay.d);
			}
		}

		sample->arena.FreeAll();
	}
}

//------------------------------------------------------------------------------
// Tracing photons Adaptative Markov Chain Sampler
//------------------------------------------------------------------------------

bool SPPMRenderer::PhotonPassRenderThread::IsVisible(Scene &scene, const Sample *sample, const float *u) {
	// I have to make a copy of SpectrumWavelengths because it can be modified
	// even if passed as a const argument !
	SpectrumWavelengths sw(sample->swl);

	// Choose light to shoot photon from
	float lightPdf;
	u_int lightNum = lightCDF->SampleDiscrete(u[6], &lightPdf);
	const Light *light = scene.lights[lightNum];

	// Generate _photonRay_ from light source and initialize _alpha_
	BSDF *bsdf;
	float pdf;
	SWCSpectrum alpha;
	if (!light->SampleL(scene, *sample, u[0], u[1], u[2],
			&bsdf, &pdf, &alpha)) {
		sample->arena.FreeAll();
		return false;
	}
	Ray photonRay;
	photonRay.o = bsdf->dgShading.p;
	float pdf2;
	SWCSpectrum alpha2;
	if (!bsdf->SampleF(sw, Vector(bsdf->nn), &photonRay.d,
			u[3], u[4], u[5], &alpha2, &pdf2)) {
		sample->arena.FreeAll();
		return false;
	}
	alpha *= alpha2;
	alpha /= lightPdf;

	if (!alpha.Black()) {
		// Follow photon path through scene and record intersections
		Intersection photonIsect;
		const Volume *volume = NULL; //FIXME: try to get volume from light
		BSDF *photonBSDF;
		u_int nIntersections = 0;
		size_t currentIndex = 7;
		while (scene.Intersect(*sample, volume, false,
			photonRay, 1.f, &photonIsect, &photonBSDF,
			NULL, NULL, &alpha)) {
			++nIntersections;

			// Handle photon/surface intersection
			Vector wi = -photonRay.d;

			// Deposit Flux (only if we have hit a diffuse or glossy surface)
			if (photonBSDF->NumComponents(BxDFType(BSDF_REFLECTION | BSDF_TRANSMISSION | BSDF_GLOSSY | BSDF_DIFFUSE)) > 0) {
				if (renderer->hitPoints->HitSomething(photonIsect.dg.p, *photonBSDF, wi, sw)) {
					sample->arena.FreeAll();
					return true;
				}
			}

			// Sample new photon ray direction
			Vector wo;
			float pdfo;
			BxDFType flags;
			// Get random numbers for sampling outgoing photon direction
			const float u1 = u[currentIndex++];
			const float u2 = u[currentIndex++];
			const float u3 = u[currentIndex++];

			// Compute new photon weight and possibly terminate with RR
			SWCSpectrum fr;
			if (!photonBSDF->SampleF(sw, wi, &wo, u1, u2, u3, &fr, &pdfo, BSDF_ALL, &flags)) {
				sample->arena.FreeAll();
				return false;
			}

			// Russian Roulette
			SWCSpectrum anew = fr;
			const float continueProb = min(1.f, anew.Filter(sw));
			const float u4 = u[currentIndex++];
			if ((u4 > continueProb) ||
					(nIntersections > renderer->sppmi->maxPhotonPathDepth)) {
				sample->arena.FreeAll();
				return false;
			}

			alpha *= anew / continueProb;
			photonRay = Ray(photonIsect.dg.p, wo);
			volume = photonBSDF->GetVolume(photonRay.d);
		}
	}

	sample->arena.FreeAll();
	return false;
}

bool SPPMRenderer::PhotonPassRenderThread::Splat(Scene &scene, const Sample *sample, const float *u) {
	// I have to make a copy of SpectrumWavelengths because it can be modified
	// even if passed as a const argument !
	SpectrumWavelengths sw(sample->swl);

	// Choose light to shoot photon from
	float lightPdf;
	u_int lightNum = lightCDF->SampleDiscrete(u[6], &lightPdf);
	const Light *light = scene.lights[lightNum];

	// Generate _photonRay_ from light source and initialize _alpha_
	BSDF *bsdf;
	float pdf;
	SWCSpectrum alpha;
	if (!light->SampleL(scene, *sample, u[0], u[1], u[2],
			&bsdf, &pdf, &alpha)) {
		sample->arena.FreeAll();
		return false;
	}
	Ray photonRay;
	photonRay.o = bsdf->dgShading.p;
	float pdf2;
	SWCSpectrum alpha2;
	if (!bsdf->SampleF(sw, Vector(bsdf->nn), &photonRay.d,
			u[3], u[4], u[5], &alpha2, &pdf2)) {
		sample->arena.FreeAll();
		return false;
	}
	alpha *= alpha2;
	alpha /= lightPdf;

	bool isVisible = false;
	if (!alpha.Black()) {
		// Follow photon path through scene and record intersections
		Intersection photonIsect;
		const Volume *volume = NULL; //FIXME: try to get volume from light
		BSDF *photonBSDF;
		u_int nIntersections = 0;
		size_t currentIndex = 7;
		while (scene.Intersect(*sample, volume, false,
			photonRay, 1.f, &photonIsect, &photonBSDF,
			NULL, NULL, &alpha)) {
			++nIntersections;

			// Handle photon/surface intersection
			Vector wi = -photonRay.d;

			// Deposit Flux (only if we have hit a diffuse or glossy surface)
			if (photonBSDF->NumComponents(BxDFType(BSDF_REFLECTION | BSDF_TRANSMISSION | BSDF_GLOSSY | BSDF_DIFFUSE)) > 0)
				isVisible |= renderer->hitPoints->AddFlux(photonIsect.dg.p, *photonBSDF, wi, sw, alpha, light->group);

			// Sample new photon ray direction
			Vector wo;
			float pdfo;
			BxDFType flags;
			// Get random numbers for sampling outgoing photon direction
			const float u1 = u[currentIndex++];
			const float u2 = u[currentIndex++];
			const float u3 = u[currentIndex++];

			// Compute new photon weight and possibly terminate with RR
			SWCSpectrum fr;
			if (!photonBSDF->SampleF(sw, wi, &wo, u1, u2, u3, &fr, &pdfo, BSDF_ALL, &flags)) {
				sample->arena.FreeAll();
				return isVisible;
			}

			// Russian Roulette
			SWCSpectrum anew = fr;
			const float continueProb = min(1.f, anew.Filter(sw));
			const float u4 = u[currentIndex++];
			if ((u4 > continueProb) ||
					(nIntersections > renderer->sppmi->maxPhotonPathDepth)) {
				sample->arena.FreeAll();
				return isVisible;
			}

			alpha *= anew / continueProb;
			photonRay = Ray(photonIsect.dg.p, wo);
			volume = photonBSDF->GetVolume(photonRay.d);
		}
	}

	sample->arena.FreeAll();
	return isVisible;
}

void SPPMRenderer::PhotonPassRenderThread::TracePhotons(AMCMCPhotonSampler *sampler) {
	Scene &scene(*(renderer->scene));

	sampler->StartNewPhotonPass(renderer->hitPoints->GetPhotonPassWavelengthSample());
	Sample *sample = sampler->GetSample();

	// Look for a visible photon path
	do {
		sampler->Uniform();
	} while (!IsVisible(scene, sample, sampler->GetCandidateData()));
	sampler->AcceptCandidate();

	float mutationSize = 1.f;
	u_int accepted = 1;
	u_int mutated = 0;
	u_int uniformCount = 1;

	for (;;) {
		// Check if it is time to do an eye pass
		if (renderer->photonTracedPass > renderer->sppmi->photonPerPass) {
			// Save the uniformCount to scale hit points flux later
			amcUniformCount = uniformCount;

			// Ok, time to stop
			return;
		}

		osAtomicInc(&renderer->photonTracedPass);

		sampler->Uniform();

		if (Splat(scene, sample, sampler->GetCandidateData())) {
			sampler->AcceptCandidate();
			++uniformCount;
		} else {
			sampler->Mutate(mutationSize);
			++mutated;
			if (Splat(scene, sample, sampler->GetCandidateData())) {
				sampler->AcceptCandidate();
				++accepted;
			} else
				Splat(scene, sample, sampler->GetCurrentData());

			const float R = accepted / (float)mutated;
			mutationSize += (R - 0.234) / mutated;
		}
	}
}

//------------------------------------------------------------------------------

Renderer *SPPMRenderer::CreateRenderer(const ParamSet &params) {
	return new SPPMRenderer();
}

static DynamicLoader::RegisterRenderer<SPPMRenderer> r("sppm");
