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
	else
	{
		std::string eString("luxStatistics - requested an invalid data : ");
		eString+=statName;
		luxError(LUX_BADTOKEN,LUX_ERROR,eString.c_str());
		return 0.;
	}
}

// Control Implementations in Scene:
double Scene::GetNumberOfSamples()
{
	// collect samples from all threads
	double samples = 0.;
	for(unsigned int i=0;i<renderThreads.size();i++)
		samples +=renderThreads[i]->stat_Samples;
	return samples;
}
double Scene::Statistics_SamplesPPx()
{
	// divide by total pixels
	int xstart,xend,ystart,yend;
	camera->film->GetSampleExtent(&xstart,&xend,&ystart,&yend);
	return GetNumberOfSamples() / (double) ((xend-xstart)*(yend-ystart));
}

double Scene::Statistics_SamplesPSec()
{
	double samples = GetNumberOfSamples();
	double time = s_Timer.Time();
	double dif_samples = samples - lastSamples;
	double elapsed = time - lastTime;
	lastSamples = samples;
	lastTime = time;

	// return current samples / sec total
	return dif_samples / elapsed;
}

double Scene::Statistics_Efficiency()
{
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

void Scene::SignalThreads(int signal)
{
	for(unsigned int i=0;i<renderThreads.size();i++)
	{
		renderThreads[i]->signal=signal;
	}	
	CurThreadSignal = signal;
}

// thread specific wavelengths
extern boost::thread_specific_ptr<SpectrumWavelengths> thread_wavelengths;

// Scene Methods -----------------------
void RenderThread::render(RenderThread *myThread)
{
	// initialize the thread's arena
	BSDF::arena.reset(new MemoryArena());
	myThread->stat_Samples = 0.;

	// initialize the thread's rangen
	lux::random::init(myThread->n);

	// initialize the thread's spectral wavelengths
	thread_wavelengths.reset(new SpectrumWavelengths());
	
	// allocate sample pos
	u_int *useSampPos = new u_int();
	*useSampPos = 0;
	u_int maxSampPos = myThread->sampler->GetTotalSamplePos();
	
	// Trace rays: The main loop
	while (true) {
		if(!myThread->sampler->GetNextSample(myThread->sample, useSampPos))
			break;

		// Sample new SWC thread wavelengths
		thread_wavelengths->Sample(myThread->sample->wavelengths,
			myThread->sample->singleWavelength);

		while(myThread->signal == RenderThread::SIG_PAUSE)
		{ 
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
	}

	delete useSampPos;
    return;
}

int Scene::CreateRenderThread()
{
	RenderThread *rt = new  RenderThread(renderThreads.size(),
		CurThreadSignal,
		surfaceIntegrator,
		volumeIntegrator,
		sampler->clone(), camera, this);

	renderThreads.push_back(rt);
	rt->thread = new boost::thread(boost::bind(RenderThread::render, rt));

	return 0;
}

void Scene::RemoveRenderThread()
{
	//printf("CTL: Removing thread...\n");
	//thr_dat_ptrs[thr_nr -1]->Sig = THR_SIG_EXIT;
	renderThreads.back()->signal = RenderThread::SIG_EXIT;
	renderThreads.pop_back();
	//delete thr_dat_ptrs[thr_nr -1]->Si;				// TODO deleting thread pack data deletes too much (shared_ptr?) - radiance
	//delete thr_dat_ptrs[thr_nr -1]->Vi;				// leave off for now. (creates slight memory leak when removing threads (~5kb))
	//delete thr_dat_ptrs[thr_nr -1]->Spl;
	//delete thr_dat_ptrs[thr_nr -1]->Splr;
	//delete thr_dat_ptrs[thr_nr -1]->arena;
	//delete thr_dat_ptrs[thr_nr -1];
	//delete thr_ptrs[thr_nr -1];
	//thr_nr--;
	//printf("CTL: Done.\n");
}

void Scene::Render() {
	// integrator preprocessing
	camera->film->SetScene(this);
	sampler->SetFilm(camera->film);
//	surfaceIntegrator->SetSampler(sampler);
    surfaceIntegrator->Preprocess(this);
    volumeIntegrator->Preprocess(this);

	sampPos = 0;

	//start the timer
	s_Timer.Start();

	// initial thread signal is paused
	CurThreadSignal = RenderThread::SIG_RUN;

    // set current scene pointer
	//luxCurrentScene = (Scene*) this;
	
	//add a thread
	CreateRenderThread();
	
	//wait all threads to finish their job
	for(unsigned int i = 0; i < renderThreads.size(); i++)
	{
		renderThreads[i]->thread->join();
	}

	// Store final image
	camera->film->WriteImage((ImageType)(IMAGE_HDR|IMAGE_FRAMEBUFFER));
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
		luxError(LUX_MISSINGDATA,LUX_SEVERE,"No light sources defined in scene; nothing to render. Exitting...");
		exit(1);
	}
	// Scene Constructor Implementation
	bound = aggregate->WorldBound();
	if (volumeRegion) bound = Union(bound, volumeRegion->WorldBound());
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
