/***************************************************************************
 *   Copyright (C) 1998-2007 by authors (see AUTHORS.txt )                 *
 *                                                                         *
 *   This file is part of Lux Renderer.                                    *
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
 *   Lux Renderer website : http://www.luxrender.org                       *
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

// global sample pos/mutex
u_int sampPos;
boost::mutex sampPosMutex;

// Control Methods -------------------------------
extern Scene *luxCurrentScene;

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
double Scene::Statistics(char *statName) {
	if(std::string(statName)=="secElapsed")
		return s_Timer.Time();
	if(std::string(statName)=="samplesSec")
		return Statistics_SamplesPSec(); 
	if(std::string(statName)=="samplesPx")
		return Statistics_SamplesPPx(); 
	
	return 0.;
}

// Control Implementations in Scene::
double Scene::Statistics_SamplesPPx()
{
	// collect samples from all threads
	double samples = 0.;
	for(unsigned int i=0;i<renderThreads.size();i++)
		samples +=renderThreads[i]->stat_Samples;

	// divide by total pixels
	return samples / (double) (camera->film->xResolution * camera->film->yResolution);
}

double Scene::Statistics_SamplesPSec()
{
	// collect samples from all threads
	double samples = 0.;
	for(unsigned int i=0;i<renderThreads.size();i++)
		samples +=renderThreads[i]->stat_Samples; 

	double time = s_Timer.Time();
	double dif_samples = samples - lastSamples;
	double elapsed = time - lastTime;
	lastSamples = samples;
	lastTime = time;

	// return current samples / sec total
	return dif_samples / elapsed;
}

void Scene::SignalThreads(int signal)
{
	for(unsigned int i=0;i<renderThreads.size();i++)
	{
		//std::cout<<"signaling thread "<<i<<" message:"<<signal<<std::endl;
		renderThreads[i]->signal=signal;
	}	
	CurThreadSignal = signal;
}

// Scene Methods -----------------------
void RenderThread::render(RenderThread *myThread)
{
	myThread->stat_Samples = 0.;
	
	// allocate sample pos
	u_int *useSampPos = new u_int();
	*useSampPos = 1;

	// Trace rays: The main loop
	while (true) {
		if(myThread->integrationSampler) {
			// use integration sampler, it might want to mutate them etc...
			if(!myThread->integrationSampler->GetNextSample(myThread->sampler, myThread->sample, useSampPos))
				break;
		} else {
			// use traditional sampler
			if(!myThread->sampler->GetNextSample(myThread->sample, useSampPos))
				break;
		}

		while(myThread->signal == RenderThread::SIG_PAUSE)
		{ 
			boost::xtime xt;
			boost::xtime_get(&xt, boost::TIME_UTC);
			xt.sec += 1;
			boost::thread::sleep(xt);
			
			//std::cout<<"thread "<<myThread->n<<" paused"<<std::endl;
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
			if (wt1 > 0 && wt2 > 0) ray.hasDifferentials = true;
			--(myThread->sample->imageY);

			// Evaluate radiance along camera ray
			float alpha;
			Spectrum Ls = 0.f;

			//Ls = rayWeight * scene->Li(ray, sample, &alpha); don't use
			Spectrum Lo = myThread->surfaceIntegrator->Li(*(myThread->arena), myThread->scene, ray, myThread->sample, &alpha);
			Spectrum T = myThread->volumeIntegrator->Transmittance(myThread->scene, ray, myThread->sample, &alpha);
			Spectrum Lv = myThread->volumeIntegrator->Li(*(myThread->arena), myThread->scene, ray, myThread->sample, &alpha);
			Ls = rayWeight * ( T * Lo + Lv );

			// Radiance - Add sample contribution to image using integrationsampler if necessary
			// the integration sampler might want to add the sample in a different way.
			if(myThread->integrationSampler)
				myThread->integrationSampler->AddSample(*(myThread->sample), 
					ray, Ls, alpha, myThread->camera->film);
			else
				if( Ls != Spectrum(0.f) ) {
					float sX = myThread->sample->imageX;
					float sY = myThread->sample->imageY;
					myThread->camera->film->AddSample(sX, sY, Ls, alpha);
				}

			// Free BSDF memory from computing image sample value
			myThread->arena->FreeAll();
		}

		// update samples statistics
		myThread->stat_Samples++;

		// increment (locked) global sample pos if necessary
		if(*useSampPos == 0) {
			boost::mutex::scoped_lock lock(sampPosMutex);
			if( sampPos >= 4294967295 ) // u_int size -1
				sampPos = 0;
			sampPos++;
			*useSampPos = sampPos;
		}
	}

	delete useSampPos;
	//printf("THR%i: Exiting.\n", myThread->n+1);
    return;
}

int Scene::CreateRenderThread()
{
		//printf("CTL: Adding thread...\n");	
		//std::cout<<CurThreadSignal<<std::endl;
		RenderThread *rt=new  RenderThread (renderThreads.size(),
										CurThreadSignal,
										(SurfaceIntegrator*)surfaceIntegrator->clone(), 
										(VolumeIntegrator*)volumeIntegrator->clone(),
										sampler->clone(),
										camera,
										this
										);
		
		renderThreads.push_back(rt);									
		rt->thread=new boost::thread(boost::bind(RenderThread::render,rt));
		//threadGroup.add_thread(rt->thread);
		
		//printf("CTL: Done.\n");
		return 0;
}

void Scene::RemoveRenderThread()
{
	//printf("CTL: Removing thread...\n");
	//thr_dat_ptrs[thr_nr -1]->Sig = THR_SIG_EXIT;
	renderThreads.back()->signal=RenderThread::SIG_EXIT;
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
	//printf("CTL: Preprocessing integrators...\n");
    surfaceIntegrator->Preprocess(this);
    volumeIntegrator->Preprocess(this);

	sampPos = 1;

	//start the timer
	s_Timer.Start();

	// initial thread signal is paused
	CurThreadSignal = RenderThread::SIG_RUN;

    // set current scene pointer
	luxCurrentScene = (Scene*) this;
	
	//add a thread
	CreateRenderThread();
	
	//launch the render
	
/*
	//while(true)
	while(renderThreads.size()==0) //wait for at least a thread to start
	{
		boost::xtime xt;
		boost::xtime_get(&xt, boost::TIME_UTC);
		xt.sec += 1;
		boost::thread::sleep(xt);
	}
	
	std::cout<<"waiting fro threads to join..."<<std::endl;*/
	//threadGroup.join_all();
	
	//wait all threads to finish their job
	for(unsigned int i=0;i<renderThreads.size();i++)
	{
		renderThreads[i]->thread->join();
	}
	//std::cout<<"all threads joined"<<std::endl;


	// Store final image
	camera->film->WriteImage();
	return; // everything worked fine! Have a great day :) 
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
Spectrum Scene::Li(const RayDifferential &ray,
		const Sample *sample, float *alpha) const {
//  NOTE - radiance - leave these off for now, should'nt be used
//  TODO - radiance - cleanup / reimplement into integrators
//	Spectrum Lo = surfaceIntegrator->Li(this, ray, sample, alpha);
//	Spectrum T = volumeIntegrator->Transmittance(this, ray, sample, alpha);
//	Spectrum Lv = volumeIntegrator->Li(this, ray, sample, alpha);
//	return T * Lo + Lv;
	return 0.;
}
Spectrum Scene::Transmittance(const Ray &ray) const {
	return volumeIntegrator->Transmittance(this, ray, NULL, NULL);
}
