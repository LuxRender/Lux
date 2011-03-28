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

#include <iomanip>

#include "api.h"
#include "scene.h"
#include "camera.h"
#include "film.h"
#include "sampling.h"
#include "hybridsppmrenderer.h"
#include "randomgen.h"
#include "context.h"

#include "luxrays/core/context.h"
#include "luxrays/core/virtualdevice.h"

using namespace lux;

#if !defined(LUXRAYS_DISABLE_OPENCL)

//------------------------------------------------------------------------------
// HybridSPPM
//------------------------------------------------------------------------------

HybridSPPMRenderer::HybridSPPMRenderer() : HybridRenderer() {
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

	barrier = NULL;
	barrierExit = NULL;
	requestedRenderThreadsCount = 0;

	hitPoints = NULL;

	AddStringConstant(*this, "name", "Name of current renderer", "hybridsppm");
}

HybridSPPMRenderer::~HybridSPPMRenderer() {
	boost::mutex::scoped_lock lock(classWideMutex);

	if ((state != TERMINATE) && (state != INIT))
		throw std::runtime_error("Internal error: called HybridSPPM::~HybridSPPM() while not in TERMINATE or INIT state.");

	if (renderThreads.size() > 0)
		throw std::runtime_error("Internal error: called HybridSPPM::~HybridSPPM() while list of renderThread is not empty.");

	delete ctx;

	for (size_t i = 0; i < hosts.size(); ++i)
		delete hosts[i];
}

Renderer::RendererType HybridSPPMRenderer::GetType() const {
	boost::mutex::scoped_lock lock(classWideMutex);

	return HYBRIDSPPM;
}

Renderer::RendererState HybridSPPMRenderer::GetState() const {
	boost::mutex::scoped_lock lock(classWideMutex);

	return state;
}

vector<RendererHostDescription *> &HybridSPPMRenderer::GetHostDescs() {
	boost::mutex::scoped_lock lock(classWideMutex);

	return hosts;
}

void HybridSPPMRenderer::SuspendWhenDone(bool v) {
	boost::mutex::scoped_lock lock(classWideMutex);
	suspendThreadsWhenDone = v;
}

void HybridSPPMRenderer::Render(Scene *s) {
	luxrays::DataSet *dataSet;

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

		state = RUN;

		// Initialize the stats
		lastSamples = 0.;
		lastTime = 0.;
		stat_Samples = 0.;
		stat_blackSamples = 0.;
		s_Timer.Reset();
	
		// Initialize the thread's RandomGenerator seed
		lastUsedSeed = scene->seedBase - 1;
		LOG(LUX_INFO, LUX_NOERROR) << "Preprocess thread uses seed: " << lastUsedSeed;

		RandomGenerator rng(lastUsedSeed);

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

		dataSet = HybridRenderer::PreprocessGeometry(ctx, scene);
        ctx->Start();

		// start the timer
		s_Timer.Start();

		// Dade - preprocessing done
		preprocessDone = true;
		Context::GetActive()->SceneReady();

		// add a thread
		PrivateCreateRenderThread();
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

	ctx->Stop();
	delete dataSet;
	scene->dataSet = NULL;
}

void HybridSPPMRenderer::Pause() {
	boost::mutex::scoped_lock lock(classWideMutex);
	state = PAUSE;
	s_Timer.Stop();
}

void HybridSPPMRenderer::Resume() {
	boost::mutex::scoped_lock lock(classWideMutex);
	state = RUN;
	s_Timer.Start();
}

void HybridSPPMRenderer::Terminate() {
	boost::mutex::scoped_lock lock(classWideMutex);
	state = TERMINATE;
}

//------------------------------------------------------------------------------
// Statistic methods
//------------------------------------------------------------------------------

// Statistics Access
double HybridSPPMRenderer::Statistics(const string &statName) {
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
		return requestedRenderThreadsCount;
	else {
		LOG( LUX_ERROR,LUX_BADTOKEN)<< "luxStatistics - requested an invalid data : " << statName;
		return 0.;
	}
}

double HybridSPPMRenderer::Statistics_GetNumberOfSamples() {
	if (s_Timer.Time() - lastTime > .5f) {
		boost::mutex::scoped_lock lock(classWideMutex);

		for (u_int i = 0; i < requestedRenderThreadsCount; ++i) {
			if (i < renderThreads.size()) {
				fast_mutex::scoped_lock lockStats(renderThreads[i]->statLock);
				stat_Samples += renderThreads[i]->samples;
				stat_blackSamples += renderThreads[i]->blackSamples;
				renderThreads[i]->samples = 0.;
				renderThreads[i]->blackSamples = 0.;
			}
		}
	}

	return stat_Samples + scene->camera->film->numberOfSamplesFromNetwork;
}

double HybridSPPMRenderer::Statistics_SamplesPPx() {
	// divide by total pixels
	int xstart, xend, ystart, yend;
	scene->camera->film->GetSampleExtent(&xstart, &xend, &ystart, &yend);
	return Statistics_GetNumberOfSamples() / ((xend - xstart) * (yend - ystart));
}

double HybridSPPMRenderer::Statistics_SamplesPSec() {
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

double HybridSPPMRenderer::Statistics_SamplesPTotSec() {
	// Dade - s_Timer is inizialized only after the preprocess phase
	if (!preprocessDone)
		return 0.0;

	double samples = Statistics_GetNumberOfSamples();
	double time = s_Timer.Time();

	// return current samples / total elapsed secs
	return samples / time;
}

double HybridSPPMRenderer::Statistics_Efficiency() {
	Statistics_GetNumberOfSamples();	// required before eff can be calculated.

	if (stat_Samples == 0.0)
		return 0.0;

	return (100.f * stat_blackSamples) / stat_Samples;
}

//------------------------------------------------------------------------------
// Private methods
//------------------------------------------------------------------------------

void HybridSPPMRenderer::CreateRenderThread() {
	if (scene->IsFilmOnly())
		return;

	// Avoid to create the thread in case signal is EXIT. For instance, it
	// can happen when the rendering is done.
	if ((state == RUN) || (state == PAUSE)) {
	}
}

void HybridSPPMRenderer::PrivateCreateRenderThread() {
	if (scene->IsFilmOnly())
		return;

	// Avoid to create the thread in case signal is EXIT. For instance, it
	// can happen when the rendering is done.
	if ((state == RUN) || (state == PAUSE)) {
		// Add an instance to the LuxRays virtual device
		luxrays::IntersectionDevice *idev = virtualIDevice->AddVirtualDevice();

		RenderThread *rt = new  RenderThread(renderThreads.size(), this, idev);

		renderThreads.push_back(rt);
		rt->thread = new boost::thread(boost::bind(RenderThread::RenderImpl, rt));

		// Create synchronization barriers
		barrier = new boost::barrier(renderThreads.size());
		barrierExit = new boost::barrier(renderThreads.size());

		requestedRenderThreadsCount++;
	}
}

void HybridSPPMRenderer::RemoveRenderThread() {
	if (requestedRenderThreadsCount == 0)
		return;

	requestedRenderThreadsCount--;
}

void HybridSPPMRenderer::PrivateRemoveRenderThread() {
	if (renderThreads.size() == 0)
		return;

	renderThreads.back()->thread->interrupt();
	renderThreads.back()->thread->join();
	delete renderThreads.back();
	renderThreads.pop_back();

	requestedRenderThreadsCount--;
}

//------------------------------------------------------------------------------
// RenderThread methods
//------------------------------------------------------------------------------

HybridSPPMRenderer::RenderThread::RenderThread(u_int index, HybridSPPMRenderer *r, luxrays::IntersectionDevice *idev) :
	n(index), thread(NULL), renderer(r), iDevice(idev), samples(0.), blackSamples(0.) {
	const u_long bufferSize = 8192;

	for (unsigned int i = 0; i < SPPM_DEVICE_RENDER_BUFFER_COUNT; ++i) {
		rayBuffersList.push_back(new luxrays::RayBuffer(bufferSize));
		rayBuffersList[i]->PushUserData(i);
		photonPathsList.push_back(new std::vector<PhotonPath>(rayBuffersList[i]->GetSize()));
	}
	rayBufferHitPoints = new luxrays::RayBuffer(bufferSize);
}

HybridSPPMRenderer::RenderThread::~RenderThread() {
	for (unsigned int i = 0; i < SPPM_DEVICE_RENDER_BUFFER_COUNT; ++i) {
		delete rayBuffersList[i];
		delete photonPathsList[i];
	}
	delete rayBufferHitPoints;
}

void HybridSPPMRenderer::RenderThread::UpdateFilm() {
	/*Film *film = renderer->scene->camera->film;
	const u_int width = film->GetXPixelCount();
	const u_int height = film->GetYPixelCount();

	HitPoints *hitPoints = renderer->hitPoints;

	Contribution contrib;
	for (u_int i = 0; i < hitPoints->GetSize(); ++i) {
		//HitPoint *hp = hitPoints->GetHitPoint(i);

		const u_int x = i % width;
		const u_int y = i / width;
		const float r = x / (float)width;
		const float g = y / (float)height;
		contrib = Contribution(
				x + 0.5f, y + 0.5f,
				colorSpace.ToXYZ(RGBColor(r, 0.f, 0.f)), renderer->bufferId);
		film->SetSample(&contrib);
	}*/
}

void HybridSPPMRenderer::RenderThread::RenderImpl(RenderThread *renderThread) {
	HybridSPPMRenderer *renderer = renderThread->renderer;
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

	// initialize the thread's rangen
	u_long seed;
	{
		boost::mutex::scoped_lock lock(renderer->classWideMutex);
		renderer->lastUsedSeed++;
		seed = renderer->lastUsedSeed;
	}
	LOG(LUX_INFO, LUX_NOERROR) << "Thread " << renderThread->n << " uses seed: " << seed;

	RandomGenerator rndGen(seed);

	luxrays::IntersectionDevice *device = renderThread->iDevice;
	luxrays::RayBuffer *rayBufferHitPoints = renderThread->rayBufferHitPoints;
	HitPoints *hitPoints = NULL;
	try {
		// First eye pass
		if (renderThread->n == 0) {
			// One thread initialize the EyePaths list
			renderer->hitPoints = new HitPoints(renderer);
		}

		// Wait for other threads
		renderer->barrier->wait();

		hitPoints = renderer->hitPoints;
		// Multi-threads calculate hit points
		hitPoints->SetHitPoints(&rndGen, device, rayBufferHitPoints, renderThread->n, renderer->renderThreads.size());

		// Wait for other threads
		renderer->barrier->wait();

		double passStartTime = luxrays::WallClockTime();
		while (!boost::this_thread::interruption_requested()) {
			/*for (;;) {
				// Trace the rays
				while (todoBuffers.size() > 0) {
					RayBuffer *rayBuffer = todoBuffers.front();
					todoBuffers.pop_front();

					device->PushRayBuffer(rayBuffer);
				}

				RayBuffer *rayBuffer = device->PopRayBuffer();
				std::vector<PhotonPath> &photonPaths = *(renderThread->photonPathsList[rayBuffer->GetUserData()]);
				AdvancePhotonPaths(renderEngine, hitPoints, scene, rndGen, rayBuffer, photonPaths);
				todoBuffers.push_back(rayBuffer);

				// Check if it is time to do an eye pass
				if ((renderEngine->photonTracedPass > renderEngine->stochasticInterval) ||
					boost::this_thread::interruption_requested()) {
					// Ok, time to stop, finish current work
					const unsigned int pendingBuffers = SPPM_DEVICE_RENDER_BUFFER_COUNT - todoBuffers.size();
					for(unsigned int i = 0; i < pendingBuffers; i++) {
						RayBuffer *rayBuffer = device->PopRayBuffer();
						std::vector<PhotonPath> &photonPaths = *(renderThread->photonPathsList[rayBuffer->GetUserData()]);
						AdvancePhotonPaths(renderEngine, hitPoints, scene, rndGen, rayBuffer, photonPaths);
						todoBuffers.push_back(rayBuffer);
					}
					break;
				}
			}*/

			// Wait for other threads
			renderer->barrier->wait();

			const double t1 = luxrays::WallClockTime();

			/*const long long count = renderEngine->photonTracedTotal + renderEngine->photonTracedPass;
			hitPoints->AccumulateFlux(count, renderThread->threadIndex, renderEngine->renderThreads.size());
			hitPoints->SetHitPoints(rndGen, device, rayBufferHitPoints, renderThread->threadIndex, renderEngine->renderThreads.size());*/

			// Wait for other threads
			renderer->barrier->wait();

			// The first thread has to do some special task for the eye pass
			if (renderThread->n == 0) {
				// First thread only tasks
				//hitPoints->UpdatePointsInformation();
				//hitPoints->IncPass();
				//hitPoints->RefreshAccelMutex();

				// Update the frame buffer
				// TODO transform in static method
				renderThread->UpdateFilm();

				//renderEngine->photonTracedTotal = count;
				//renderEngine->photonTracedPass = 0;
			}

			// Wait for other threads
			renderer->barrier->wait();

			//hitPoints->RefreshAccelParallel(renderThread->threadIndex, renderEngine->renderThreads.size());

			if (renderThread->n == 0) {
				const double photonPassTime = t1 - passStartTime;
				LOG(LUX_INFO, LUX_NOERROR) << "Photon pass time: " << photonPassTime << "secs";
				const double eyePassTime = luxrays::WallClockTime() - t1;
				LOG(LUX_INFO, LUX_NOERROR) << "Eye pass time: " << eyePassTime << "secs (" << 100.0 * eyePassTime / (eyePassTime + photonPassTime) << "%)";

				passStartTime = luxrays::WallClockTime();
			}

			if (boost::this_thread::interruption_requested())
				break;
		}
	} catch (boost::thread_interrupted) {
		LOG(LUX_INFO, LUX_NOERROR) << "Rendering thread " << renderThread->n << " halted";
	}
#if !defined(LUXRAYS_DISABLE_OPENCL)
	catch (cl::Error err) {
		LOG(LUX_INFO, LUX_ERROR) << "[SPPMDeviceRenderThread::" << renderThread->n << "] Rendering thread ERROR: " << err.what() << "(" << err.err() << ")";
	}
#endif
}

Renderer *HybridSPPMRenderer::CreateRenderer(const ParamSet &params) {
	return new HybridSPPMRenderer();
}

static DynamicLoader::RegisterRenderer<HybridSPPMRenderer> r("hybridsppm");

#endif // !defined(LUXRAYS_DISABLE_OPENCL)
