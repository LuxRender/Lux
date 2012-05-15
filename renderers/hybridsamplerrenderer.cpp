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
#include "hybridsamplerrenderer.h"
#include "randomgen.h"
#include "context.h"
#include "integrators/path.h"
#include "renderers/statistics/hybridsamplerstatistics.h"

#include "luxrays/core/context.h"
#include "luxrays/core/device.h"
#include "luxrays/core/virtualdevice.h"

using namespace lux;

//------------------------------------------------------------------------------
// SurfaceIntegratorStateBuffer
//------------------------------------------------------------------------------

SurfaceIntegratorStateBuffer::SurfaceIntegratorStateBuffer(
		const Scene &scn, ContributionBuffer *contribBuf,
		RandomGenerator *rngGen, luxrays::RayBuffer *rayBuf) :
		scene(scn), integratorState(128) {
	contribBuffer = contribBuf;
	rng = rngGen;
	rayBuffer = rayBuf;

	// Inititialize the first set SurfaceIntegratorState
	for (size_t i = 0; i < integratorState.size(); ++i) {
		integratorState[i] = scene.surfaceIntegrator->NewState(scene, contribBuffer, rng);
		integratorState[i]->Init(scene);
	}

	firstStateIndex = 0;
}

SurfaceIntegratorStateBuffer::~SurfaceIntegratorStateBuffer() {
	// Free memory
	for (size_t i = 0; i < integratorState.size(); ++i) {
		integratorState[i]->Free(scene);
		delete integratorState[i];
	}
}

void SurfaceIntegratorStateBuffer::GenerateRays() {
	//--------------------------------------------------------------------------
	// File the RayBuffer with the generated rays
	//--------------------------------------------------------------------------

	bool usedAllStates = false;
	lastStateIndex = firstStateIndex;
	while (rayBuffer->LeftSpace() > 0) {
		if (!scene.surfaceIntegrator->GenerateRays(scene, integratorState[lastStateIndex], rayBuffer)) {
			// The RayBuffer is full
			break;
		}

		lastStateIndex = (lastStateIndex + 1) % integratorState.size();
		if (lastStateIndex == firstStateIndex) {
			usedAllStates = true;
			break;
		}
	}
	/*LOG(LUX_DEBUG, LUX_NOERROR) << "Used IntegratorStates: " <<
			(lastStateIndex > firstStateIndex ? (lastStateIndex - firstStateIndex) : (lastStateIndex + integratorState.size() - firstStateIndex)) <<
			"/" << integratorState.size();*/

	//--------------------------------------------------------------------------
	// Check if I need to add more SurfaceIntegratorState
	//--------------------------------------------------------------------------

	if (usedAllStates) {
		// Need to add more paths
		size_t newStateCount = 0;

		// To limit the number of new SurfaceIntegratorState generated at first run
		const size_t maxNewPaths = max<size_t>(64, rayBuffer->GetSize() >> 4);

		for (;;) {
			// Add more SurfaceIntegratorState
			SurfaceIntegratorState *s = scene.surfaceIntegrator->NewState(scene, contribBuffer, rng);
			s->Init(scene);
			integratorState.push_back(s);
			newStateCount++;

			if (!scene.surfaceIntegrator->GenerateRays(scene, s, rayBuffer)) {
				// The RayBuffer is full
				firstStateIndex = 0;
				// -2 because the addition of the last SurfaceIntegratorState failed
				lastStateIndex = integratorState.size() - 2;
				break;
			}

			if (newStateCount >= maxNewPaths) {
				firstStateIndex = 0;
				lastStateIndex = integratorState.size() - 1;
				break;
			}
		}

		integratorState.resize(integratorState.size());
		LOG(LUX_DEBUG, LUX_NOERROR) << "New allocated IntegratorStates: " << newStateCount << " => " <<
				integratorState.size() << " [RayBuffer size = " << rayBuffer->GetSize() << "]";
	}
}

bool SurfaceIntegratorStateBuffer::NextState(u_int &nrContribs, u_int &nrSamples) {
	//----------------------------------------------------------------------
	// Advance the next step
	//----------------------------------------------------------------------

	for (size_t i = firstStateIndex; i != lastStateIndex; i = (i + 1) % integratorState.size()) {
		u_int count;
		if (scene.surfaceIntegrator->NextState(scene, integratorState[i], rayBuffer, &count)) {
			// The sample is finished
			++(nrSamples);
			nrContribs += count;

			if (!integratorState[i]->Init(scene)) {
				// We have done
				firstStateIndex = (i + 1) % integratorState.size();
				return true;
			}
		}

		nrContribs += count;
	}

	firstStateIndex = (lastStateIndex + 1) % integratorState.size();

	return false;
}

//------------------------------------------------------------------------------
// HybridSamplerRenderer
//------------------------------------------------------------------------------

HybridSamplerRenderer::HybridSamplerRenderer(const int oclPlatformIndex, bool useGPUs,
		const u_int forceGPUWorkGroupSize, const string &deviceSelection,
		const u_int rayBufSize, const u_int stateBufCount,
		const u_int qbvhStackSize) : HybridRenderer() {
	state = INIT;

	if (!IsPowerOf2(rayBufSize)) {
		LOG(LUX_WARNING, LUX_CONSISTENCY) << "HybridSampler ray buffer size being rounded up to power of 2";
		rayBufferSize = RoundUpPow2(rayBufSize);
	} else
		rayBufferSize = rayBufSize;

	stateBufferCount = stateBufCount;

	// Create the LuxRays context
	ctx = new luxrays::Context(LuxRaysDebugHandler, oclPlatformIndex);

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

	bool useNative = false;

#if !defined(LUXRAYS_DISABLE_OPENCL)
	// Create the virtual device to feed all hardware devices
	std::vector<luxrays::DeviceDescription *> hwDeviceDescs = deviceDescs;
	luxrays::DeviceDescription::Filter(luxrays::DEVICE_TYPE_OPENCL, hwDeviceDescs);
	luxrays::OpenCLDeviceDescription::Filter(luxrays::OCL_DEVICE_TYPE_GPU, hwDeviceDescs);

	if (useGPUs && (hwDeviceDescs.size() >= 1)) {
		if (hwDeviceDescs.size() == 1) {
			// Only one GPU available
			luxrays::OpenCLDeviceDescription *desc = (luxrays::OpenCLDeviceDescription *)hwDeviceDescs[0];
			if (forceGPUWorkGroupSize > 0)
				desc->SetForceWorkGroupSize(forceGPUWorkGroupSize);

			hardwareDevices = ctx->AddVirtualM2OIntersectionDevices(0, hwDeviceDescs);
			virtualIM2ODevice = ctx->GetVirtualM2OIntersectionDevices()[0];
			virtualIM2MDevice = NULL;
		} else {
			// Multiple GPUs available

			// Select the devices to use
			std::vector<luxrays::DeviceDescription *> selectedDescs;
			bool haveSelectionString = (deviceSelection.length() > 0);
			if (haveSelectionString) {
				if (deviceSelection.length() != hwDeviceDescs.size()) {
					LOG(LUX_WARNING, LUX_MISSINGDATA) << "OpenCL device selection string has the wrong length, must be " <<
							hwDeviceDescs.size() << " instead of " << deviceSelection.length() << ", ignored";

					selectedDescs = hwDeviceDescs;
				} else {
					for (size_t i = 0; i < hwDeviceDescs.size(); ++i) {
						if (deviceSelection.at(i) == '1')
							selectedDescs.push_back(hwDeviceDescs[i]);
					}
				}
			} else
				selectedDescs = hwDeviceDescs;

			if (forceGPUWorkGroupSize > 0) {
				for (size_t i = 0; i < selectedDescs.size(); ++i) {
					luxrays::OpenCLDeviceDescription *desc = (luxrays::OpenCLDeviceDescription *)selectedDescs[i];
					desc->SetForceWorkGroupSize(forceGPUWorkGroupSize);
				}
			}

			if (selectedDescs.size() == 1) {
				// Multiple GPUs are available but only one is selected
				hardwareDevices = ctx->AddVirtualM2OIntersectionDevices(0, selectedDescs);
				virtualIM2ODevice = ctx->GetVirtualM2OIntersectionDevices()[0];
				virtualIM2MDevice = NULL;
			} else {
				hardwareDevices = ctx->AddVirtualM2MIntersectionDevices(0, selectedDescs);
				virtualIM2ODevice = NULL;
				virtualIM2MDevice = ctx->GetVirtualM2MIntersectionDevices()[0];
			}
		}

		// The default QBVH stack size (i.e. 24) is too small for the average
		// LuxRender scene and the slight slow down caused by a bigger stack
		// should not be noticiable with hybrid rendering. The default is now 32.
		for (size_t i = 0; i < hardwareDevices.size(); ++i) {
			if (hardwareDevices[i]->GetType() == luxrays::DEVICE_TYPE_OPENCL)
				((luxrays::OpenCLIntersectionDevice *)hardwareDevices[i])->SetQBVHMaxStackSize(qbvhStackSize);
		}

		LOG(LUX_INFO, LUX_NOERROR) << "OpenCL Devices used:";
		for (size_t i = 0; i < hardwareDevices.size(); ++i)
			LOG(LUX_INFO, LUX_NOERROR) << " [" << hardwareDevices[i]->GetName() << "]";
	} else
		// don't want GPU or no hardware available, use native
		useNative = true;
#else
	// only native mode without OpenCL
	if (useGPUs)
		LOG(LUX_INFO, LUX_NOERROR) << "GPU assisted rendering requires an OpenCL enabled version of LuxRender, using CPU instead";

	useGPUs = false;
	useNative = true;
#endif

	if (useNative) {
		if (useGPUs)
			LOG(LUX_WARNING, LUX_SYSTEM) << "Unable to find an OpenCL GPU device, falling back to CPU";

		virtualIM2ODevice = NULL;
		virtualIM2MDevice = NULL;

		// allocate native threads
		std::vector<luxrays::DeviceDescription *> nativeDeviceDescs = deviceDescs;
		luxrays::DeviceDescription::Filter(luxrays::DEVICE_TYPE_NATIVE_THREAD, nativeDeviceDescs);

		nativeDevices = ctx->AddIntersectionDevices(nativeDeviceDescs);
	}


	preprocessDone = false;
	suspendThreadsWhenDone = false;

	AddStringConstant(*this, "name", "Name of current renderer", "hybridsampler");

	rendererStatistics = new HSRStatistics(this);
}

HybridSamplerRenderer::~HybridSamplerRenderer() {
	boost::mutex::scoped_lock lock(classWideMutex);

	delete rendererStatistics;

	if ((state != TERMINATE) && (state != INIT))
		throw std::runtime_error("Internal error: called HybridSamplerRenderer::~HybridSamplerRenderer() while not in TERMINATE or INIT state.");

	if (renderThreads.size() > 0)
		throw std::runtime_error("Internal error: called HybridSamplerRenderer::~HybridSamplerRenderer() while list of renderThread sis not empty.");

	delete ctx;

	for (size_t i = 0; i < hosts.size(); ++i)
		delete hosts[i];
}

Renderer::RendererType HybridSamplerRenderer::GetType() const {
	boost::mutex::scoped_lock lock(classWideMutex);

	return HYBRIDSAMPLER_TYPE;
}

Renderer::RendererState HybridSamplerRenderer::GetState() const {
	boost::mutex::scoped_lock lock(classWideMutex);

	return state;
}

vector<RendererHostDescription *> &HybridSamplerRenderer::GetHostDescs() {
	boost::mutex::scoped_lock lock(classWideMutex);

	return hosts;
}

void HybridSamplerRenderer::SuspendWhenDone(bool v) {
	boost::mutex::scoped_lock lock(classWideMutex);
	suspendThreadsWhenDone = v;
}

void HybridSamplerRenderer::Render(Scene *s) {
	luxrays::DataSet *dataSet;

	{
		// Section under mutex
		boost::mutex::scoped_lock lock(classWideMutex);

		scene = s;
		// Not yet available!
		//scene->arlux_setup();

		if (scene->IsFilmOnly()) {
			state = TERMINATE;
			return;
		}

		if (scene->lights.size() == 0) {
			LOG( LUX_SEVERE,LUX_MISSINGDATA)<< "No light sources defined in scene; nothing to render.";
			state = TERMINATE;
			return;
		}

		if (!scene->surfaceIntegrator->IsDataParallelSupported()) {
			LOG( LUX_SEVERE,LUX_ERROR)<< "The SurfaceIntegrator doesn't support HybridSamplerRenderer.";
			state = TERMINATE;
			return;
		}

		// scene->surfaceIntegrator->CheckLightStrategy() can not been used before
		// preprocess anymore.

		state = RUN;

		// Initialize the stats
		rendererStatistics->reset();
	
		// Dade - I have to do initiliaziation here for the current thread.
		// It can be used by the Preprocess() methods.

		// initialize the thread's RandomGenerator
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
		rendererStatistics->timer.Start();

		// Dade - preprocessing done
		preprocessDone = true;
		scene->SetReady();

		if (scene->surfaceIntegrator->CheckLightStrategy(*scene)) {
			// add a thread
			CreateRenderThread();
		}
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

void HybridSamplerRenderer::Pause() {
	boost::mutex::scoped_lock lock(classWideMutex);
	state = PAUSE;
	rendererStatistics->timer.Stop();
}

void HybridSamplerRenderer::Resume() {
	boost::mutex::scoped_lock lock(classWideMutex);
	state = RUN;
	rendererStatistics->timer.Start();
}

void HybridSamplerRenderer::Terminate() {
	boost::mutex::scoped_lock lock(classWideMutex);
	state = TERMINATE;
}

//------------------------------------------------------------------------------
// Private methods
//------------------------------------------------------------------------------

void HybridSamplerRenderer::CreateRenderThread() {
	if (scene->IsFilmOnly())
		return;

	// Avoid to create the thread in case signal is EXIT. For instance, it
	// can happen when the rendering is done.
	if ((state == RUN) || (state == PAUSE)) {
		luxrays::IntersectionDevice *idev;

		if (virtualIM2ODevice) {
			// Add an instance to the LuxRays virtual device
			idev = virtualIM2ODevice->AddVirtualDevice();
		} else if (virtualIM2MDevice) {
			// Add an instance to the LuxRays virtual device
			idev = virtualIM2MDevice->AddVirtualDevice();
		} else {
			// Add a nativethread device
			idev = nativeDevices[renderThreads.size() % nativeDevices.size()];
		}

		RenderThread *rt = new  RenderThread(renderThreads.size(), this, idev);

		renderThreads.push_back(rt);
		rt->thread = new boost::thread(boost::bind(RenderThread::RenderImpl, rt));
	}
}

void HybridSamplerRenderer::RemoveRenderThread() {
	if (renderThreads.size() == 0)
		return;

	RenderThread *renderThread = renderThreads.back();
	renderThread->thread->interrupt();
	renderThread->thread->join();

	if (virtualIM2ODevice) {
		// Add an instance to the LuxRays virtual device
		virtualIM2ODevice->RemoveVirtualDevice(renderThread->iDevice);
	} else if (virtualIM2MDevice) {
		// Add an instance to the LuxRays virtual device
		virtualIM2MDevice->RemoveVirtualDevice(renderThread->iDevice);
	}

	delete renderThread;
	renderThreads.pop_back();

}

//------------------------------------------------------------------------------
// RenderThread methods
//------------------------------------------------------------------------------

HybridSamplerRenderer::RenderThread::RenderThread(u_int index, HybridSamplerRenderer *r, luxrays::IntersectionDevice * idev) :
	n(index), thread(NULL), renderer(r), iDevice(idev), samples(0.), blackSamples(0.) {
}

HybridSamplerRenderer::RenderThread::~RenderThread() {
}

void HybridSamplerRenderer::RenderThread::RenderImpl(RenderThread *renderThread) {
	HybridSamplerRenderer *renderer = renderThread->renderer;
	Scene &scene(*(renderer->scene));
	if (scene.IsFilmOnly())
		return;

	// To avoid interrupt exception
	boost::this_thread::disable_interruption di;

	// Dade - wait the end of the preprocessing phase
	while (!renderer->preprocessDone) {
		boost::this_thread::sleep(boost::posix_time::seconds(1));
	}

	// ContribBuffer has to wait until the end of the preprocessing
	// It depends on the fact that the film buffers have been created
	// This is done during the preprocessing phase
	ContributionBuffer *contribBuffer = new ContributionBuffer(scene.camera->film->contribPool);

	// initialize the thread's rangen
	u_long seed;
	{
		boost::mutex::scoped_lock lock(renderer->classWideMutex);
		renderer->lastUsedSeed++;
		seed = renderer->lastUsedSeed;
	}
	LOG(LUX_INFO, LUX_NOERROR) << "Thread " << renderThread->n << " uses seed: " << seed;

	RandomGenerator rng(seed);

	// Inititialize the first set SurfaceIntegratorState
	const double t0 = luxrays::WallClockTime();

	vector<SurfaceIntegratorStateBuffer *> stateBuffers(renderer->stateBufferCount);
	for (size_t i = 0; i < stateBuffers.size(); ++i) {
		luxrays::RayBuffer *rayBuffer = renderThread->iDevice->NewRayBuffer(renderer->rayBufferSize);
		rayBuffer->PushUserData(i);

		stateBuffers[i] = new SurfaceIntegratorStateBuffer(scene, contribBuffer, &rng, rayBuffer);
		stateBuffers[i]->GenerateRays();
		renderThread->iDevice->PushRayBuffer(rayBuffer);
	}

	LOG(LUX_DEBUG, LUX_NOERROR) << "Thread " << renderThread->n << " initialization time: " <<
			std::setiosflags(std::ios::fixed) << std::setprecision(2) <<
			luxrays::WallClockTime() - t0 << " secs";

	for(;;) {
		while (renderer->state == PAUSE) {
			boost::this_thread::sleep(boost::posix_time::seconds(1));
		}
		if ((renderer->state == TERMINATE) || boost::this_thread::interruption_requested()) {
			// Pop left rayBuffers
			for (size_t i = 0; i < stateBuffers.size(); ++i)
				renderThread->iDevice->PopRayBuffer();
			break;
		}

		luxrays::RayBuffer *rayBuffer = renderThread->iDevice->PopRayBuffer();
		SurfaceIntegratorStateBuffer *stateBuffer = stateBuffers[rayBuffer->GetUserData()];

		//----------------------------------------------------------------------
		// Advance the next step
		//----------------------------------------------------------------------

		//const double t1 = luxrays::WallClockTime();

		bool renderIsOver = false;
		u_int nrContribs = 0;
		u_int nrSamples = 0;
		// stateBuffer.NextState() returns true when the rendering is
		// finished, false otherwise
		while (stateBuffer->NextState(nrContribs, nrSamples)) {
			// Dade - we have done, check what we have to do now
			if (renderer->suspendThreadsWhenDone) {
				renderer->Pause();
				// Dade - wait for a resume rendering or exit
				while (renderer->state == PAUSE) {
					boost::this_thread::sleep(boost::posix_time::seconds(1));
				}

				if (renderer->state == TERMINATE) {
					renderIsOver = true;
					break;
				} else
					continue;
			} else {
				renderer->Terminate();
				renderIsOver = true;
				break;
			}
		}

		// Jeanphi - Hijack statistics until volume integrator revamp
		{
			// update samples statistics
			fast_mutex::scoped_lock lockStats(renderThread->statLock);
			renderThread->blackSamples += nrContribs;
			renderThread->samples += nrSamples;
		}

		if (renderIsOver) {
			// Pop left rayBuffers (one has already been pop)
			for (size_t i = 0; i < stateBuffers.size()- 1; ++i)
				renderThread->iDevice->PopRayBuffer();
			break;
		}

		//LOG(LUX_DEBUG, LUX_NOERROR) << "NextState() time: " << int((luxrays::WallClockTime() - t1) * 1000.0) << "ms";

		//----------------------------------------------------------------------
		// File the RayBuffer with the generated rays
		//----------------------------------------------------------------------

		//const double t2 = luxrays::WallClockTime();

		rayBuffer->Reset();
		stateBuffer->GenerateRays();

		//LOG(LUX_DEBUG, LUX_NOERROR) << "GenerateRays() time: " << (luxrays::WallClockTime() - t2) * 1000.0 << "ms";

		//----------------------------------------------------------------------
		// Trace the RayBuffer
		//----------------------------------------------------------------------

		renderThread->iDevice->PushRayBuffer(rayBuffer);
	}

	scene.camera->film->contribPool->End(contribBuffer);

	// Free memory
	for (size_t i = 0; i < stateBuffers.size(); ++i) {
		delete stateBuffers[i]->GetRayBuffer();
		delete stateBuffers[i];
	}
}

Renderer *HybridSamplerRenderer::CreateRenderer(const ParamSet &params) {

	ParamSet configParams(params);

	const  string configFile = params.FindOneString("configfile", "");
	if (configFile != "")
		HybridRenderer::LoadCfgParams(configFile, &configParams);

	const u_int rayBufferSize = params.FindOneInt("raybuffersize", 8192);
	const u_int stateBufferCount = max(1, params.FindOneInt("statebuffercount", 1));

	string deviceSelection = configParams.FindOneString("opencl.devices.select", "");
	int platformIndex = configParams.FindOneInt("opencl.platform.index", -1);

	bool useGPUs = configParams.FindOneBool("opencl.gpu.use", true);

	// With a value of 0 we would end to use the default OpenCL driver for
	// the compiled kernel. However, 99% of times, the default OpenCL driver is
	// not a valid value for the kernel (and local GPU memory) we are going to use.
	// Better to use 64 as default value (a valid and good values in for
	// about all OpenCL devices).
	const u_int forceGPUWorkGroupSize = max(0, configParams.FindOneInt("opencl.gpu.workgroup.size", 64));

	const u_int qbvhStackSize = max(16, configParams.FindOneInt("accelerator.qbvh.stacksize.max", 32));

	params.MarkUsed(configParams);
	return new HybridSamplerRenderer(platformIndex, useGPUs,
			forceGPUWorkGroupSize, deviceSelection, rayBufferSize,
			stateBufferCount, qbvhStackSize);
}

static DynamicLoader::RegisterRenderer<HybridSamplerRenderer> r("hybrid");
static DynamicLoader::RegisterRenderer<HybridSamplerRenderer> r2("hybridsampler");
