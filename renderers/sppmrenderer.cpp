/***************************************************************************
 *   Copyright (C) 1998-2013 by authors (see AUTHORS.txt)                  *
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
#include "renderers/statistics/sppmstatistics.h"
#include "samplers/random.h"

using namespace lux;

//------------------------------------------------------------------------------
// SPPMRDeviceDescription
//------------------------------------------------------------------------------

unsigned int SPPMRDeviceDescription::GetUsedUnitsCount() const {
	return host->renderer->scheduler->ThreadCount();
}

void SPPMRDeviceDescription::SetUsedUnitsCount(const unsigned int units) {
	unsigned int target = max(units, 1u);
	size_t current = host->renderer->scheduler->ThreadCount();

	if (current > target) {
		for (unsigned int i = 0; i < current - target; ++i)
			host->renderer->scheduler->DelThread();
	} else if (current < target) {
		for (unsigned int i = 0; i < target - current; ++i)
			host->renderer->scheduler->AddThread(new SPPMRenderer::RenderThread(host->renderer));
	}
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

	rendererStatistics = new SPPMRStatistics(this);

	scheduler = new scheduling::Scheduler(1000); // TODO: set blocksize as a parameter
}

SPPMRenderer::~SPPMRenderer() {
	boost::mutex::scoped_lock lock(classWideMutex);

	delete rendererStatistics;

	if ((state != TERMINATE) && (state != INIT))
		throw std::runtime_error("Internal error: called SPPMRenderer::~SPPMRenderer() while not in TERMINATE or INIT state.");

	for (size_t i = 0; i < hosts.size(); ++i)
		delete hosts[i];

	delete scheduler; // TODO: ask Done ?
}

Renderer::RendererType SPPMRenderer::GetType() const {
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

void SPPMRenderer::Render(Scene *s) {
	{
		// Section under mutex
		boost::mutex::scoped_lock lock(classWideMutex);

		scene = s;
		// Not yet available!
		//scene->arlux_setup();

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
		rendererStatistics->reset();
	
		// Dade - I have to do initiliaziation here for the current thread.
		// It can be used by the Preprocess() methods.

		// initialize the thread's rangen
		u_long seed = scene->seedBase - 1;
		LOG(LUX_DEBUG, LUX_NOERROR) << "Preprocess thread uses seed: " << seed;

		rng = new RandomGenerator(seed);

		// integrator preprocessing
		// sppm integrator will create film buffers
		scene->surfaceIntegrator->Preprocess(*rng, *scene);
		scene->volumeIntegrator->Preprocess(*rng, *scene);

		// Told each Buffer how to scale things
		for(u_int bg = 0; bg < scene->camera()->film->GetNumBufferGroups(); ++bg)
		{
			PerScreenNormalizedBufferScaled * buffer= dynamic_cast<PerScreenNormalizedBufferScaled *>(scene->camera()->film->GetBufferGroup(bg).getBuffer(sppmi->bufferPhotonId));
			buffer->scaleUpdate = new ScaleUpdaterSPPM(this);
		}

		// Dade - to support autofocus for some camera model
		scene->camera()->AutoFocus(*scene);

		size_t threadCount = boost::thread::hardware_concurrency();
		LOG(LUX_INFO, LUX_NOERROR) << "Hardware concurrency: " << threadCount;

		// initialise
		photonHitEfficiency = 0;

		// For AMCMC
		// TODO: check if it is really 1, or 0, or N-threads
		uniformCount = 1.f;

		// start the timer
		rendererStatistics->start();

		Context::GetActive()->SceneReady();
	}

	// Add the first thread // TODO: why
	scheduler->AddThread(new RenderThread(this));

	// thread for checking write interval
	boost::thread writeIntervalThread = boost::thread(boost::bind(writeIntervalCheck, scene->camera()->film));

	RenderMain(scene);

	scheduler->Done();

	// stop write interval checking
	writeIntervalThread.interrupt();
	// possibly wait for writing to finish
	writeIntervalThread.join();
}

void SPPMRenderer::Pause() {
	boost::mutex::scoped_lock lock(classWideMutex);
	state = PAUSE;
	scheduler->Pause();
	rendererStatistics->stop();
}

void SPPMRenderer::Resume() {
	boost::mutex::scoped_lock lock(classWideMutex);
	state = RUN;
	scheduler->Resume();
	rendererStatistics->start();
}

void SPPMRenderer::Terminate() {
	boost::mutex::scoped_lock lock(classWideMutex);
	state = TERMINATE;
}

//------------------------------------------------------------------------------
// Render thread
//------------------------------------------------------------------------------

SPPMRenderer::RenderThread::RenderThread(SPPMRenderer *r) :
	renderer(r) {

	// Initialize the thread's rangen
	u_long seed = renderer->rng->uintValue();
	threadRng = new RandomGenerator(seed);

	// TODO: this can be done in the sampler ?
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

void SPPMRenderer::RenderThread::Init() {
	// To avoid interrupt exception
	boost::this_thread::disable_interruption di;

	// Dade - wait the end of the preprocessing phase
	while (!renderer->preprocessDone) {
		boost::this_thread::sleep(boost::posix_time::seconds(1));
	}

	Scene &scene = *renderer->scene;

	// Initialize the photon sampler
	// TODO: there must be only one photon sampler instance instead of one by thread
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
	sample.contribBuffer = new ContributionBuffer(scene.camera()->film->contribPool);
	// The RNG might be used when initializing the sampler data below
	sample.rng = threadRng;
//	sample.camera = scene.camera->Clone(); // Unneeded for photons
	// sample.realTime and sample.swl are intialized later
	// Describe sampling data
	sampler->Add1D(1); // light sampling
	sampler->Add2D(1); // light position sampling
	sampler->Add1D(1); // light position portal sampling
	sampler->Add2D(1); // light direction sampling
	sampler->Add1D(1); // light direction portal sampling
	vector<u_int> structure;
	structure.push_back(2); // BSDF direction sampling
	structure.push_back(1); // BSDF component sampling
	structure.push_back(1); // RR sampling
	sampler->AddxD(structure, renderer->sppmi->maxPhotonPathDepth + 1);
	renderer->scene->volumeIntegrator->RequestSamples(sampler, *(renderer->scene));
	sampler->InitSample(&sample);

	// initialise the eye sample
	eyeSample.contribBuffer = new ContributionBuffer(scene.camera()->film->contribPool);
	eyeSample.camera = scene.camera()->Clone();
	eyeSample.realTime = 0.f;
	eyeSample.rng = threadRng;

	renderer->hitPoints->eyeSampler->InitSample(&eyeSample);
}

void SPPMRenderer::TracePhotons(scheduling::Range *range)
{
	RenderThread* thread = dynamic_cast<RenderThread*>(range->thread);
	Sample &sample = thread->sample;
	PhotonSampler *sampler = thread->sampler;

	sample.wavelengths = hitPoints->GetWavelengthSample();
	sample.time = hitPoints->GetTimeSample();
	sample.swl.Sample(sample.wavelengths);
	sample.realTime = scene->camera()->GetTime(sample.time);
//		sample.camera->SampleMotion(sample.realTime); // Unneeded for photons

	//--------------------------------------------------------------
	// Photon pass: trace photons
	//--------------------------------------------------------------
	sampler->TracePhotons(&sample, thread->lightCDF, range);
}

void SPPMRenderer::RenderMain(Scene *scene)
{
	if (scene->IsFilmOnly())
		return;

	//--------------------------------------------------------------------------
	// First eye pass
	//--------------------------------------------------------------------------

	// One thread initialize the hit points
	hitPoints = new HitPoints(this, rng);

	vector<u_int> structure;
	structure.push_back(1);	// volume scattering
	structure.push_back(2);	// bsdf sampling direction
	structure.push_back(1);	// bsdf sampling component
	structure.push_back(1);	// bsdf bouncing/storing component
	hitPoints->eyeSampler->AddxD(structure, sppmi->maxEyePathDepth + 1);

	scene->volumeIntegrator->RequestSamples(hitPoints->eyeSampler, *scene);
	sppmi->hints.RequestSamples(hitPoints->eyeSampler, *scene, sppmi->maxPhotonPathDepth + 1);

	preprocessDone = true;

	double eyePassStartTime = 0.0;
	eyePassStartTime = osWallClockTime();

	scheduler->Launch(boost::bind(&HitPoints::SetHitPoints, hitPoints, _1), 0, hitPoints->GetSize());

	hitPoints->Init();

	// Trace rays: The main loop
	while (!scene->camera()->film->enoughSamplesPerPixel &&
		(scene->camera()->film->haltSamplesPerPixel <= .0f || hitPoints->GetPassCount() < scene->camera()->film->haltSamplesPerPixel) &&
		state != TERMINATE) {
		hitPoints->UpdatePointsInformation();

		hitPoints->RefreshAccel(scheduler);

		const double eyePassTime = osWallClockTime() - eyePassStartTime;
		LOG(LUX_INFO, LUX_NOERROR) << "Eye pass time: " << eyePassTime << "secs";

		double photonPassStartTime = 0.0;
		photonPassStartTime = osWallClockTime();

		scheduler->Launch(boost::bind(&SPPMRenderer::TracePhotons, this, _1), 0, sppmi->photonPerPass);

		photonHitEfficiency = hitPoints->GetPhotonHitEfficency();

		scheduler->Launch(boost::bind(&HitPoints::AccumulateFlux, hitPoints, _1), 0, hitPoints->GetSize());

		hitPoints->IncPass();

		const double photonPassTime = osWallClockTime() - photonPassStartTime;
		LOG(LUX_INFO, LUX_NOERROR) << "Photon pass time: " << photonPassTime << "secs";

		eyePassStartTime = osWallClockTime();

		scheduler->Launch(boost::bind(&HitPoints::SetHitPoints, hitPoints, _1), 0, hitPoints->GetSize());
	}
}

void SPPMRenderer::RenderThread::End() {
	Scene &scene = *renderer->scene;

	scene.camera()->film->contribPool->End(sample.contribBuffer);
	scene.camera()->film->contribPool->End(eyeSample.contribBuffer);
	sample.contribBuffer = NULL;
	eyeSample.contribBuffer = NULL;

	sampler->FreeSample(&sample);
	renderer->hitPoints->eyeSampler->FreeSample(&eyeSample);

	delete sampler;
}

Renderer *SPPMRenderer::CreateRenderer(const ParamSet &params) {
	return new SPPMRenderer();
}

float SPPMRenderer::GetScaleFactor(const double scale) const
{
	if (sppmi->photonSamplerType == AMC) {
		return uniformCount / (sppmi->photonPerPass / scene->camera()->film->GetSamplePerPass() * scale * scale);
	} else
		return 1.0 / scale;
}

static DynamicLoader::RegisterRenderer<SPPMRenderer> r("sppm");
