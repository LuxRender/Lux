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
#include "hybridrenderer.h"
#include "randomgen.h"
#include "context.h"

#include "luxrays/core/context.h"
#include "luxrays/core/virtualdevice.h"

using namespace lux;

void LuxRaysDebugHandler(const char *msg) {
	std::stringstream ss;
	ss << "[LuxRays] " << msg;
	luxError(LUX_NOERROR, LUX_DEBUG, ss.str().c_str());
}

//------------------------------------------------------------------------------
// HRDeviceDescription
//------------------------------------------------------------------------------

HRHardwareDeviceDescription::HRHardwareDeviceDescription(HRHostDescription *h, luxrays::DeviceDescription *desc) :
	HRDeviceDescription(h, desc->GetName()) {
	devDesc = desc;
	enabled = true;
}

void HRHardwareDeviceDescription::SetUsedUnitsCount(const unsigned int units) {
	// I assume there is only one single virtual device in use
	if ((units < 0) || (units > 1))
		throw std::runtime_error("A not valid amount of units used in HRDeviceDescription::SetUsedUnitsCount()");

	enabled = (units == 1);
}

HRVirtualDeviceDescription::HRVirtualDeviceDescription(HRHostDescription *h, const string &name) :
	HRDeviceDescription(h, name) {
}

unsigned int HRVirtualDeviceDescription::GetUsedUnitsCount() const {
	// I assume there is only one single virtual device in use
	boost::mutex::scoped_lock lock(host->renderer->classWideMutex);
	return host->renderer->renderThreads.size();
}

void HRVirtualDeviceDescription::SetUsedUnitsCount(const unsigned int units) {
	// I assume there is only one single virtual device in use
	boost::mutex::scoped_lock lock(host->renderer->classWideMutex);

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
// HRHostDescription
//------------------------------------------------------------------------------

HRHostDescription::HRHostDescription(HybridRenderer *r, const string &n) : renderer(r), name(n) {
}

HRHostDescription::~HRHostDescription() {
	for (size_t i = 0; i < devs.size(); ++i)
		delete devs[i];
}

void HRHostDescription::AddDevice(HRDeviceDescription *devDesc) {
	devs.push_back(devDesc);
}

//------------------------------------------------------------------------------
// HybridRenderer
//------------------------------------------------------------------------------

HybridRenderer::HybridRenderer() {
	state = INIT;

	// Create the LuxRays context
	ctx = new luxrays::Context(LuxRaysDebugHandler);

	// Create the device descriptions
	HRHostDescription *host = new HRHostDescription(this, "Localhost");
	hosts.push_back(host);

	// Add one virtual device to feed all the OpenCL devices
	host->AddDevice(new HRVirtualDeviceDescription(host, "VirtualGPU"));

	// Get the list of devices available
	std::vector<luxrays::DeviceDescription *> deviceDescs = std::vector<luxrays::DeviceDescription *>(ctx->GetAvailableDeviceDescriptions());

	// Add all the OpenCL devices
	for (size_t i = 0; i < deviceDescs.size(); ++i)
		host->AddDevice(new HRHardwareDeviceDescription(host, deviceDescs[i]));

	// Create the virtual device to feed all hardware device
	std::vector<luxrays::DeviceDescription *> hwDeviceDescs = deviceDescs;
	luxrays::DeviceDescription::Filter(luxrays::DEVICE_TYPE_OPENCL, hwDeviceDescs);
	luxrays::OpenCLDeviceDescription::Filter(luxrays::OCL_DEVICE_TYPE_GPU, hwDeviceDescs);

	if (hwDeviceDescs.size() < 1)
		throw std::runtime_error("Unable to find an OpenCL GPU device.");
	hwDeviceDescs.resize(1);

	ctx->AddVirtualM2OIntersectionDevices(0, hwDeviceDescs);

	virtualIDevice = ctx->GetVirtualM2OIntersectionDevices()[0];

	preprocessDone = false;
	suspendThreadsWhenDone = false;
}

HybridRenderer::~HybridRenderer() {
	boost::mutex::scoped_lock lock(classWideMutex);

	if ((state != TERMINATE) && (state != INIT))
		throw std::runtime_error("Internal error: called HybridRenderer::~HybridRenderer() while not in TERMINATE or INIT state.");

	if (renderThreads.size() > 0)
		throw std::runtime_error("Internal error: called HybridRenderer::~HybridRenderer() while list of renderThread sis not empty.");

	delete ctx;

	for (size_t i = 0; i < hosts.size(); ++i)
		delete hosts[i];
}

Renderer::RendererType HybridRenderer::GetType() const {
	boost::mutex::scoped_lock lock(classWideMutex);

	return SAMPLER;
}

Renderer::RendererState HybridRenderer::GetState() const {
	boost::mutex::scoped_lock lock(classWideMutex);

	return state;
}

vector<RendererHostDescription *> &HybridRenderer::GetHostDescs() {
	boost::mutex::scoped_lock lock(classWideMutex);

	return hosts;
}

void HybridRenderer::SuspendWhenDone(bool v) {
	boost::mutex::scoped_lock lock(classWideMutex);
	suspendThreadsWhenDone = v;
}

void HybridRenderer::Render(Scene *s) {
	luxrays::DataSet *dataSet;

	{
		// Section under mutex
		boost::mutex::scoped_lock lock(classWideMutex);

		scene = s;

		if (scene->IsFilmOnly()) {
			state = TERMINATE;
			return;
		}

		if (scene->lights.size() == 0) {
			luxError(LUX_MISSINGDATA, LUX_SEVERE, "No light sources defined in scene; nothing to render.");
			state = TERMINATE;
			return;
		}

		if (scene->surfaceIntegrator->IsDataParallelSupported() == 0) {
			luxError(LUX_ERROR, LUX_SEVERE, "The SurfaceIntegrator doesn't support HybridRenderer.");
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

		// initialize the thread's RandomGenerator
		u_long seed = scene->seedBase - 1;
		std::stringstream ss;
		ss << "Preprocess thread uses seed: " << seed;
		luxError(LUX_NOERROR, LUX_INFO, ss.str().c_str());

		RandomGenerator rng(seed);

		// integrator preprocessing
		scene->sampler->SetFilm(scene->camera->film);
		scene->surfaceIntegrator->Preprocess(rng, *scene);
		scene->volumeIntegrator->Preprocess(rng, *scene);
		scene->camera->film->CreateBuffers();

		// Dade - to support autofocus for some camera model
		scene->camera->AutoFocus(*scene);

		//----------------------------------------------------------------------
		// Compile the scene geometries in a LuxRays compatible format
		//----------------------------------------------------------------------

		vector<luxrays::TriangleMesh *> meshList;
		primitiveList.clear();

		ss.str("");
		ss << "Tasselating " << scene->primitives.size() << " primitives";
		luxError(LUX_NOERROR, LUX_INFO, ss.str().c_str());

		for (size_t i = 0; i < scene->primitives.size(); ++i)
			scene->primitives[i]->Tesselate(&meshList, &primitiveList);

		// Create the DataSet
        dataSet = new luxrays::DataSet(ctx);

		// Add all mesh
        for (std::vector<luxrays::TriangleMesh *>::const_iterator obj = meshList.begin(); obj != meshList.end(); ++obj)
			dataSet->Add(*obj);

        dataSet->Preprocess();
		ctx->SetDataSet(dataSet);
        ctx->Start();

		// I can free temporary data
		for (std::vector<luxrays::TriangleMesh *>::const_iterator obj = meshList.begin(); obj != meshList.end(); ++obj)
			delete *obj;

		// start the timer
		s_Timer.Start();

		// Dade - preprocessing done
		preprocessDone = true;
		Context::GetActive()->SceneReady();

		// add a thread
		CreateRenderThread();
	}

	if (renderThreads.size() > 0) {
		// The first thread can not be removed
		// it will terminate when the rendering is finished
		renderThreads[0]->thread->join();

		// rendering done, now I can remove all rendering threads
		{
			boost::mutex::scoped_lock lock(classWideMutex);

			// wait for all threads to finish their job
			for (u_int i = 0; i < renderThreads.size(); ++i) {
				renderThreads[i]->thread->join();
				delete renderThreads[i];
			}
			renderThreads.clear();

			// I change the current signal to exit in order to disable the creation
			// of new threads after this point
			state = TERMINATE;
		}

		// Flush the contribution pool
		scene->camera->film->contribPool->Flush();
		scene->camera->film->contribPool->Delete();
	}

	luxError(LUX_NOERROR, LUX_INFO, "Work in progress: exit");
	exit(0);

	ctx->Stop();
	delete dataSet;
	primitiveList.clear();
}

void HybridRenderer::Pause() {
	boost::mutex::scoped_lock lock(classWideMutex);
	state = PAUSE;
}

void HybridRenderer::Resume() {
	boost::mutex::scoped_lock lock(classWideMutex);
	state = RUN;
}

void HybridRenderer::Terminate() {
	boost::mutex::scoped_lock lock(classWideMutex);
	state = TERMINATE;
}

//------------------------------------------------------------------------------
// Statistic methods
//------------------------------------------------------------------------------

// Statistics Access
double HybridRenderer::Statistics(const string &statName) {
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
	else if(statName=="filmXres")
		return scene->FilmXres();
	else if(statName=="filmYres")
		return scene->FilmYres();
	else if(statName=="displayInterval")
		return scene->DisplayInterval();
	else if(statName == "filmEV")
		return scene->camera->film->EV;
	else if (statName == "enoughSamples")
		return scene->camera->film->enoughSamplePerPixel;
	else {
		std::string eString("luxStatistics - requested an invalid data : ");
		eString+=statName;
		luxError(LUX_BADTOKEN, LUX_ERROR, eString.c_str());
		return 0.;
	}
}

double HybridRenderer::Statistics_GetNumberOfSamples() {
	if (s_Timer.Time() - lastTime > .5f) {
		boost::mutex::scoped_lock lock(classWideMutex);

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

double HybridRenderer::Statistics_SamplesPPx() {
	// divide by total pixels
	int xstart, xend, ystart, yend;
	scene->camera->film->GetSampleExtent(&xstart, &xend, &ystart, &yend);
	return Statistics_GetNumberOfSamples() / ((xend - xstart) * (yend - ystart));
}

double HybridRenderer::Statistics_SamplesPSec() {
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

double HybridRenderer::Statistics_SamplesPTotSec() {
	// Dade - s_Timer is inizialized only after the preprocess phase
	if (!preprocessDone)
		return 0.0;

	double samples = Statistics_GetNumberOfSamples();
	double time = s_Timer.Time();

	// return current samples / total elapsed secs
	return samples / time;
}

double HybridRenderer::Statistics_Efficiency() {
	if (stat_Samples == 0.0)
		return 0.0;

	return (100.f * stat_blackSamples) / stat_Samples;
}

//------------------------------------------------------------------------------
// Private methods
//------------------------------------------------------------------------------

void HybridRenderer::CreateRenderThread() {
	if (scene->IsFilmOnly())
		return;

	// Avoid to create the thread in case signal is EXIT. For instance, it
	// can happen when the rendering is done.
	if ((state != TERMINATE) || (state != INIT)) {
		// Add an instance to the LuxRays virtual device
		luxrays::IntersectionDevice * idev = virtualIDevice->AddVirtualDevice();

		RenderThread *rt = new  RenderThread(renderThreads.size(), this, idev);

		renderThreads.push_back(rt);
		rt->thread = new boost::thread(boost::bind(RenderThread::RenderImpl, rt));
	}
}

void HybridRenderer::RemoveRenderThread() {
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

HybridRenderer::RenderThread::RenderThread(u_int index, HybridRenderer *r, luxrays::IntersectionDevice * idev) :
	n(index), thread(NULL), renderer(r), iDevice(idev), samples(0.), blackSamples(0.) {
}

HybridRenderer::RenderThread::~RenderThread() {
}

void HybridRenderer::RenderThread::RenderImpl(RenderThread *renderThread) {
	HybridRenderer *renderer = renderThread->renderer;
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

	// ContribBuffer has to wait until the end of the preprocessing
	// It depends on the fact that the film buffers have been created
	// This is done during the preprocessing phase
	ContributionBuffer *contribBuffer = new ContributionBuffer(scene.camera->film->contribPool);

	// initialize the thread's rangen
	u_long seed = scene.seedBase + renderThread->n;
	std::stringstream ss;
	ss << "Thread " << renderThread->n << " uses seed: " << seed;
	luxError(LUX_NOERROR, LUX_INFO, ss.str().c_str());

	RandomGenerator rng(seed);

	luxrays::RayBuffer *rayBuffer = renderThread->iDevice->NewRayBuffer();

	// Init all PathState
	vector<SurfaceIntegratorState *> integratorState(rayBuffer->GetSize());
	for (size_t i = 0; i < integratorState.size(); ++i) {
		integratorState[i] = scene.surfaceIntegrator->NewState(scene, contribBuffer, &rng);
		integratorState[i]->Init(scene);
	}

	size_t currentGenerateIndex = 0;
	size_t currentNextIndex = 0;
	bool renderIsOver = false;
	while (!renderIsOver) {
		while (renderer->state == PAUSE) {
			boost::xtime xt;
			boost::xtime_get(&xt, boost::TIME_UTC);
			xt.sec += 1;
			boost::thread::sleep(xt);
		}
		if ((renderer->state == TERMINATE) || boost::this_thread::interruption_requested())
			break;

		while (rayBuffer->LeftSpace() > 0) {
			if (!scene.surfaceIntegrator->GenerateRays(scene, integratorState[currentGenerateIndex], rayBuffer)) {
				// The RayBuffer is full
				break;
			}

			currentGenerateIndex = (currentGenerateIndex + 1) % integratorState.size();
		}

		// Trace the RayBuffer
		renderThread->iDevice->PushRayBuffer(rayBuffer);
		rayBuffer = renderThread->iDevice->PopRayBuffer();

		// Advance the next step
		for (size_t i = 0; i < rayBuffer->GetRayCount(); ++i) {
			if (scene.surfaceIntegrator->NextState(scene, integratorState[currentNextIndex], rayBuffer)) {
				// The path is finished
				if (!integratorState[currentNextIndex]->Init(scene)) {
					renderer->Pause();

					// Dade - we have done, check what we have to do now
					if (renderer->suspendThreadsWhenDone) {
						// Dade - wait for a resume rendering or exit
						while (renderer->state == PAUSE) {
							boost::xtime xt;
							boost::xtime_get(&xt, boost::TIME_UTC);
							xt.sec += 1;
							boost::thread::sleep(xt);
						}

						if (renderer->state == TERMINATE) {
							renderIsOver = true;
							break;
						} else
							continue;
					} else {
						renderIsOver = true;
						break;
					}
				}
			}

			currentNextIndex = (currentNextIndex + 1) % integratorState.size();
		}

		rayBuffer->Reset();
	}

	scene.camera->film->contribPool->End(contribBuffer);

	// Free memory
	for (size_t i = 0; i < integratorState.size(); ++i)
		delete integratorState[i];
	delete rayBuffer;
}

Renderer *HybridRenderer::CreateRenderer(const ParamSet &params) {
	return new HybridRenderer();
}

static DynamicLoader::RegisterRenderer<HybridRenderer> r("hybrid");
