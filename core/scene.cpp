/***************************************************************************
 *   Copyright (C) 1998-2008 by authors (see AUTHORS.txt )                 *
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

// scene.cpp*
#include <sstream>
#include <stdlib.h>

#include "scene.h"
#include "camera.h"
#include "film.h"
#include "sampling.h"
#include "volume.h"
#include "error.h"
#include "context.h"
#include "bxdf.h"
#include "light.h"
#include "spectrumwavelengths.h"

#include "randomgen.h"

#include <boost/thread/xtime.hpp>
#include <boost/bind.hpp>

using namespace lux;

// global sample pos/mutex
u_int sampPos;
boost::mutex sampPosMutex;

// Control Methods -------------------------------
//extern Scene *luxCurrentScene;

// Engine Control (start/pause/restart) methods
void Scene::Start() {
    SignalThreads(RUN);
    s_Timer.Start();
}

void Scene::Pause() {
    SignalThreads(PAUSE);
    s_Timer.Stop();
}

void Scene::Exit() {
    SignalThreads(EXIT);
}

// Engine Thread Control (adding/removing)
int Scene::AddThread() {
    return CreateRenderThread();
}

void Scene::RemoveThread() {
    RemoveRenderThread();
}

int Scene::getThreadsStatus(RenderingThreadInfo *info, int maxInfoCount) {
#if !defined(WIN32)
	boost::mutex::scoped_lock lock(renderThreadsMutex);
#endif

	for (int i = 0; i < min<int>(renderThreads.size(), maxInfoCount); i++) {
		info[i].threadIndex = renderThreads[i]->n;
		info[i].status = renderThreads[i]->signal;
	}

	return renderThreads.size();
}

// Framebuffer Access for GUI
void Scene::UpdateFramebuffer() {
    camera->film->updateFrameBuffer();
}

unsigned char* Scene::GetFramebuffer() {
    return camera->film->getFrameBuffer();
}

int Scene::DisplayInterval() {
    return (int)camera->film->getldrDisplayInterval();
}

int Scene::FilmXres() {
    return camera->film->xResolution;
}

int Scene::FilmYres() {
    return camera->film->yResolution;
}

// Statistics Access
double Scene::Statistics(const string &statName) {
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
        return FilmXres();
    else if(statName=="filmYres")
        return FilmYres();
    else if(statName=="displayInterval")
        return DisplayInterval();
    else {
        std::string eString("luxStatistics - requested an invalid data : ");
        eString+=statName;
        luxError(LUX_BADTOKEN, LUX_ERROR, eString.c_str());
        return 0.;
    }
}

// Control Implementations in Scene:
double Scene::GetNumberOfSamples() {
	boost::mutex::scoped_lock lock(renderThreadsMutex);

    // collect samples from all threads
    double samples = 0.;
    for(unsigned int i=0;i<renderThreads.size();i++)
        samples +=renderThreads[i]->stat_Samples;

    // Dade - add the samples received from network
    samples += numberOfSamplesFromNetwork;

    return samples;
}

double Scene::Statistics_SamplesPPx() {
    // divide by total pixels
    int xstart, xend, ystart, yend;
    camera->film->GetSampleExtent(&xstart, &xend, &ystart, &yend);
    return GetNumberOfSamples() / (double) ((xend-xstart)*(yend-ystart));
}

double Scene::Statistics_SamplesPSec() {
	// Dade - s_Timer is inizialized only after the preprocess phase
	if (!preprocessDone)
		return 0.0;

    double samples = GetNumberOfSamples();
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

double Scene::Statistics_SamplesPTotSec() {
	// Dade - s_Timer is inizialized only after the preprocess phase
	if (!preprocessDone)
		return 0.0;

    double samples = GetNumberOfSamples();
    double time = s_Timer.Time();

    // return current samples / total elapsed secs
    return samples / time;
}

double Scene::Statistics_Efficiency() {
	boost::mutex::scoped_lock lock(renderThreadsMutex);

	if(renderThreads.size() == 0)
		return 0.0;

    // collect samples from all threads
    double samples = 0.;
    double drops = 0.;
    for(unsigned int i=0;i<renderThreads.size();i++) {
        samples +=renderThreads[i]->stat_Samples;
        drops +=renderThreads[i]->stat_blackSamples;
    }

	if (samples == 0.0)
		return 0.0;

    // return efficiency percentage
    return 100. - (drops * (100/samples));
}

void Scene::SignalThreads(ThreadSignals signal) {
	boost::mutex::scoped_lock lock(renderThreadsMutex);

    for(unsigned int i=0;i<renderThreads.size();i++) {
        renderThreads[i]->signal=signal;
    }
    CurThreadSignal = signal;
}

// Scene Methods -----------------------
void RenderThread::render(RenderThread *myThread) {
    // Dade - wait the end of the preprocessing phase
    while(!myThread->scene->preprocessDone) {
        boost::xtime xt;
        boost::xtime_get(&xt, boost::TIME_UTC);
        xt.sec += 1;
        boost::thread::sleep(xt);
    }

    // initialize the thread's arena
    BSDF::arena.reset(new MemoryArena());
    myThread->stat_Samples = 0.;

    // initialize the thread's rangen
    int seed = myThread->scene->seedBase + myThread->n;
    std::stringstream ss;
    ss << "Thread " << myThread->n << " uses seed: " << seed;
    luxError(LUX_NOERROR, LUX_INFO, ss.str().c_str());

	// initialize the threads tspack
	myThread->tspack = new TsPack();									// TODO - radiance - remove

	myThread->tspack->swl = new SpectrumWavelengths();
	myThread->tspack->rng = new RandomGenerator();
	myThread->tspack->rng->init(seed);
	// TODO add bsdf arena

    myThread->sampler->SetTsPack(myThread->tspack);

    // allocate sample pos
    u_int *useSampPos = new u_int();
    *useSampPos = 0;
    u_int maxSampPos = myThread->sampler->GetTotalSamplePos();

    // Trace rays: The main loop
    while (true) {
		if(!myThread->sampler->GetNextSample(myThread->sample, useSampPos)) {
			// Dade - we have done, check what we have to do now
			if (myThread->scene->suspendThreadsWhenDone) {
				myThread->signal = PAUSE;

				// Dade - wait for a resume rendering or exit
				while(myThread->signal == PAUSE) {
					boost::xtime xt;
					boost::xtime_get(&xt, boost::TIME_UTC);
					xt.sec += 1;
					boost::thread::sleep(xt);
				}

				if(myThread->signal == EXIT)
					break;
				else
					continue;
			} else
				break;
		}

		// Dade - check if the integrator support SWC
		if (myThread->surfaceIntegrator->IsSWCSupported()) {						// COCACOCA -> remove ?
			// Sample new SWC thread wavelengths
			myThread->tspack->swl->Sample(myThread->sample->wavelengths,
					myThread->sample->singleWavelength);
		} else {
			myThread->sample->wavelengths = 0.5f;
			myThread->sample->singleWavelength = 0.5f;
			myThread->tspack->swl->Sample(0.5f, 0.5f);
		}

        while(myThread->signal == PAUSE) {
            boost::xtime xt;
            boost::xtime_get(&xt, boost::TIME_UTC);
            xt.sec += 1;
            boost::thread::sleep(xt);
        }
        if(myThread->signal== EXIT)
            break;

		// Find camera ray for _sample_
        RayDifferential ray;
        float rayWeight = myThread->camera->GenerateRay(*(myThread->sample), &ray);

        if (rayWeight > 0.f) {
            // Generate ray differentials for camera ray
            ++(myThread->sample->imageX);
            float wt1 = myThread->camera->GenerateRay(*(myThread->sample), &ray.rx);
            --(myThread->sample->imageX);
            ++(myThread->sample->imageY);
            float wt2 = myThread->camera->GenerateRay(*(myThread->sample), &ray.ry);
            ray.hasDifferentials = wt1 > 0.f && wt2 > 0.f;
            --(myThread->sample->imageY);

            // Evaluate radiance along camera ray
            float alpha;
            SWCSpectrum Lo = myThread->surfaceIntegrator->Li(myThread->tspack, myThread->scene, ray, myThread->sample, &alpha);
            /*			SWCSpectrum T = myThread->volumeIntegrator->Transmittance(myThread->tspack, myThread->scene, ray, myThread->sample, &alpha);
                        SWCSpectrum Lv = myThread->volumeIntegrator->Li(myThread->tspack, myThread->scene, ray, myThread->sample, &alpha);
                        SWCSpectrum Ls = rayWeight * ( T * Lo + Lv );*/

            if (Lo.Black())
                myThread->stat_blackSamples++;

            // TODO: what about rayWeight?
            myThread->sampler->AddSample(*(myThread->sample));

            // Free BSDF memory from computing image sample value
            BSDF::FreeAll();
        }

        // update samples statistics
        myThread->stat_Samples++;
        // increment (locked) global sample pos if necessary (eg maxSampPos != 0)
        if(*useSampPos == -1 && maxSampPos != 0) {
            boost::mutex::scoped_lock lock(sampPosMutex);
            sampPos++;
            if( sampPos == maxSampPos )
                sampPos = 0;
            *useSampPos = sampPos;
        }

#ifdef WIN32
		//Work around Windows bad scheduling -- Jeanphi
		myThread->thread->yield();
#endif
    }

    delete useSampPos;
    return;
}

int Scene::CreateRenderThread() {
#if !defined(WIN32)
	boost::mutex::scoped_lock lock(renderThreadsMutex);
#endif

    RenderThread *rt = new  RenderThread(renderThreads.size(),
            CurThreadSignal,
            surfaceIntegrator,
            volumeIntegrator,
            sampler->clone(), camera, this);

    renderThreads.push_back(rt);
    rt->thread = new boost::thread(boost::bind(RenderThread::render, rt));

    return 0;
}

void Scene::RemoveRenderThread() {
#if !defined(WIN32)
	boost::mutex::scoped_lock lock(renderThreadsMutex);
#endif

    renderThreads.back()->signal = EXIT;
    renderThreads.pop_back();
}

void Scene::Render() {
    // Dade - I have to do initiliaziation here for the current thread. It can
    // be used by the Preprocess() methods.

    // initialize the thread's arena
    BSDF::arena.reset(new MemoryArena());

    // initialize the thread's rangen
    int seed = seedBase - 1;
    std::stringstream ss;
    ss << "Preprocess thread uses seed: " << seed;
    luxError(LUX_NOERROR, LUX_INFO, ss.str().c_str());

	// initialize the preprocess thread's tspack
	tspack = new TsPack();
	tspack->swl = new SpectrumWavelengths();				// TODO - REFACT - check sample wavelengths
	tspack->rng = new RandomGenerator();
	tspack->rng->init(seed);
	// TODO add bsdf arena

    sampler->SetTsPack(tspack);

    // integrator preprocessing
    camera->film->SetScene(this);
    sampler->SetFilm(camera->film);
    surfaceIntegrator->Preprocess(tspack, this);
    volumeIntegrator->Preprocess(tspack, this);

	// Dade - to support autofocus for some camera model
	camera->AutoFocus(this);

    sampPos = 0;

    //start the timer
    s_Timer.Start();

    // Dade - preprocessing done
    preprocessDone = true;

    // initial thread signal is paused
    CurThreadSignal = RUN;

    // Dade - this code needs a fix, it must be removed in order to not create
    // 1 more thread than required
    //add a thread
    CreateRenderThread();

	// Dade - this code fragment is not thread safe
    //wait all threads to finish their job
    for(unsigned int i = 0; i < renderThreads.size(); i++)
        renderThreads[i]->thread->join();

    // Store final image
    camera->film->WriteImage((ImageType)(IMAGE_FILEOUTPUT|IMAGE_FRAMEBUFFER));
}

Scene::~Scene() {
    delete camera;
    delete sampler;
    delete surfaceIntegrator;
    delete volumeIntegrator;
    //delete aggregate;
    delete volumeRegion;
    for (u_int i = 0; i < lights.size(); ++i)
        delete lights[i];
}

Scene::Scene(Camera *cam, SurfaceIntegrator *si,
        VolumeIntegrator *vi, Sampler *s,
        boost::shared_ptr<Primitive> accel, const vector<Light *> &lts,
        VolumeRegion *vr) {
    lights = lts;
    aggregate = accel;
    camera = cam;
    sampler = s;
    surfaceIntegrator = si;
    volumeIntegrator = vi;
    volumeRegion = vr;
    s_Timer.Reset();
    lastSamples = 0.;
	numberOfSamplesFromNetwork = 0.; // NOTE - radiance - added initialization
    lastTime = 0.;
    if (lts.size() == 0) {
        luxError(LUX_MISSINGDATA, LUX_SEVERE, "No light sources defined in scene; nothing to render. Exitting...");
        exit(1);
    }
    // Scene Constructor Implementation
    bound = aggregate->WorldBound();
    if (volumeRegion) bound = Union(bound, volumeRegion->WorldBound());

    // Dade - Initialize the base seed with the standard C lib random number generator
    seedBase = rand();

    preprocessDone = false;
	suspendThreadsWhenDone = false;
}

const BBox &Scene::WorldBound() const {
    return bound;
}

SWCSpectrum Scene::Li(const RayDifferential &ray,
		const Sample *sample, float *alpha) const {
//  NOTE - radiance - leave these off for now, should'nt be used (broken with multithreading)
//  TODO - radiance - cleanup / reimplement into integrators
//	SWCSpectrum Lo = surfaceIntegrator->Li(this, ray, sample, alpha);
//	SWCSpectrum T = volumeIntegrator->Transmittance(this, ray, sample, alpha);
//	SWCSpectrum Lv = volumeIntegrator->Li(this, ray, sample, alpha);
//	return T * Lo + Lv;
	return 0.;
}

SWCSpectrum Scene::Transmittance(const TsPack *tspack, const Ray &ray) const {
    return volumeIntegrator->Transmittance(tspack, this, ray, NULL, NULL);
}
