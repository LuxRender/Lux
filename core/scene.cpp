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
#include "dynload.h"
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
    SignalThreads(RenderThread::SIG_RUN);
    s_Timer.Start();
}

void Scene::Pause() {
    SignalThreads(RenderThread::SIG_PAUSE);
    s_Timer.Stop();
}

void Scene::Exit() {
    SignalThreads(RenderThread::SIG_EXIT);
}

// Engine Thread Control (adding/removing)
int Scene::AddThread() {
    return CreateRenderThread();
}

void Scene::RemoveThread() {
    RemoveRenderThread();
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
    if(statName=="secElapsed")
        return s_Timer.Time();
    else if(statName=="samplesSec")
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
    double samples = GetNumberOfSamples();
    double time = s_Timer.Time();
    double dif_samples = samples - lastSamples;
    double elapsed = time - lastTime;
    lastSamples = samples;
    lastTime = time;
    
    // return current samples / sec total
    return dif_samples / elapsed;
}

double Scene::Statistics_SamplesPTotSec() {
    double samples = GetNumberOfSamples();
    double time = s_Timer.Time();
    
    // return current samples / total elapsed secs
    return samples / time;
}

double Scene::Statistics_Efficiency() {
	boost::mutex::scoped_lock lock(renderThreadsMutex);

    // collect samples from all threads
    double samples = 0.;
    double drops = 0.;
    for(unsigned int i=0;i<renderThreads.size();i++) {
        samples +=renderThreads[i]->stat_Samples;
        drops +=renderThreads[i]->stat_blackSamples;
    }
    
    // return efficiency percentage
    return 100. - (drops * (100/samples));
}

void Scene::SignalThreads(int signal) {
	boost::mutex::scoped_lock lock(renderThreadsMutex);

    for(unsigned int i=0;i<renderThreads.size();i++) {
        renderThreads[i]->signal=signal;
    }
    CurThreadSignal = signal;
}

// thread specific wavelengths
extern boost::thread_specific_ptr<SpectrumWavelengths> thread_wavelengths;

static RGBColor sumRGB = 0.f;
static int tot = 0;

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
    lux::random::init(seed);

    // initialize the thread's spectral wavelengths
    thread_wavelengths.reset(new SpectrumWavelengths());
    SpectrumWavelengths *thr_wl = thread_wavelengths.get();
    
    // allocate sample pos
    u_int *useSampPos = new u_int();
    *useSampPos = 0;
    u_int maxSampPos = myThread->sampler->GetTotalSamplePos();
    
    // Trace rays: The main loop
    while (true) {
        if(!myThread->sampler->GetNextSample(myThread->sample, useSampPos))
            break;
        
        // Sample new SWC thread wavelengths
        thr_wl->Sample(myThread->sample->wavelengths,
                myThread->sample->singleWavelength);
        
        while(myThread->signal == RenderThread::SIG_PAUSE) {
            boost::xtime xt;
            boost::xtime_get(&xt, boost::TIME_UTC);
            xt.sec += 1;
            boost::thread::sleep(xt);
        }
        if(myThread->signal== RenderThread::SIG_EXIT)
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
            SWCSpectrum Lo = myThread->surfaceIntegrator->Li(myThread->scene, ray, myThread->sample, &alpha);
            /*			SWCSpectrum T = myThread->volumeIntegrator->Transmittance(myThread->scene, ray, myThread->sample, &alpha);
                        SWCSpectrum Lv = myThread->volumeIntegrator->Li(myThread->scene, ray, myThread->sample, &alpha);
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
        
        
        // Temporary colour scaling code by radiance - SHOULD NOT BE IN CVS !
        
        /*	// RGB -> XYZ -> RGB
         * printf("\n\n\nRGB -> XYZ -> RGB --------------\n");
         * Spectrum RGB = 1.f;
         * printf("RGB: %f %f %f\n\n", RGB.c[0], RGB.c[1], RGB.c[2]);
         *
         * XYZColor XYZ = RGB.ToXYZ();
         * printf("XYZ: %f %f %f\n", XYZ.c[0], XYZ.c[1], XYZ.c[2]);
         * printf("XYZ.y(): %f\n\n", XYZ.y());
         *
         * RGBColor oRGB = XYZ.ToRGB();
         * printf("out RGB: %f %f %f\n", oRGB.c[0], oRGB.c[1], oRGB.c[2]);
         * printf("--------------------------------\n");
         *
         * // RGB -> SWCSpectrum -> XYZ -> RGB
         * printf("\n\n\nRGB -> SWCSpect -> XYZ -> RGB --\n");
         * printf("RGB: %f %f %f\n\n", RGB.c[0], RGB.c[1], RGB.c[2]);
         *
         * SWCSpectrum spect = SWCSpectrum(RGB);
         * printf("SWCSpect.y(): %f\n\n", spect.y());
         *
         * XYZ = spect.ToXYZ();
         * printf("XYZ: %f %f %f\n", XYZ.c[0], XYZ.c[1], XYZ.c[2]);
         * printf("XYZ.y(): %f\n\n", XYZ.y());
         *
         * oRGB = XYZ.ToRGB();
         * printf("out RGB: %f %f %f\n", oRGB.c[0], oRGB.c[1], oRGB.c[2]);
         *
         * sumRGB += oRGB;
         * tot++;
         * printf("avg RGB: %f %f %f\n", sumRGB.c[0] / tot, sumRGB.c[1] / tot, sumRGB.c[2] / tot);
         *
         * for(int i=0; i<8; i++)
         * printf("wl: %f, dat: %f\n", thread_wavelengths->w[i], spect.c[i] * 8);
         *
         * printf("\n\n");
         *
         *
         * printf("--------------------------------\n"); */
        
        
        
        
        
        
        // sleep 1 sec
        //		boost::xtime xt;
        //		boost::xtime_get(&xt, boost::TIME_UTC);
        //		xt.sec += 1;
        //boost::thread::sleep(xt);
        
    }
    
    delete useSampPos;
    return;
}

int Scene::CreateRenderThread() {
	boost::mutex::scoped_lock lock(renderThreadsMutex);

    // Dade - this code is not thread safe
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
	boost::mutex::scoped_lock lock(renderThreadsMutex);

    renderThreads.back()->signal = RenderThread::SIG_EXIT;
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
    lux::random::init(seed);

    // initialize the thread's spectral wavelengths
    thread_wavelengths.reset(new SpectrumWavelengths());

    // integrator preprocessing
    camera->film->SetScene(this);
    sampler->SetFilm(camera->film);
    surfaceIntegrator->Preprocess(this);
    volumeIntegrator->Preprocess(this);
    
    sampPos = 0;
    
    //start the timer
    s_Timer.Start();

    // Dade - preprocessing done
    preprocessDone = true;
    
    // initial thread signal is paused
    CurThreadSignal = RenderThread::SIG_RUN;
    
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
    delete aggregate;
    delete volumeRegion;
    for (u_int i = 0; i < lights.size(); ++i)
        delete lights[i];
}

Scene::Scene(Camera *cam, SurfaceIntegrator *si,
        VolumeIntegrator *vi, Sampler *s,
        Primitive *accel, const vector<Light *> &lts,
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

SWCSpectrum Scene::Transmittance(const Ray &ray) const {
    return volumeIntegrator->Transmittance(this, ray, NULL, NULL);
}
