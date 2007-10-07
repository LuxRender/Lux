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
#include "scene.h"
#include "camera.h"
#include "film.h"
#include "sampling.h"
#include "dynload.h"
#include "volume.h"

// Control Methods -------------------------------
extern Scene *luxCurrentScene;

// Engine Control (start/pause/restart) methods
void Scene::Start() {
	SignalThreads(THR_SIG_RUN);
	s_Timer.Start();
}
void Scene::Pause() {
	SignalThreads(THR_SIG_PAUSE);
	s_Timer.Stop();
}
void Scene::Exit() {
	SignalThreads(THR_SIG_EXIT);
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
	return camera->film->getldrDisplayInterval();
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

// Thread data --------------------------

// Thread data pack
class Thread_data
{
    public:
		int Sig, n;
		double stat_Samples;
		Integrator* Si;
		Integrator* Vi;
		Sample* Spl;
		Sampler* Splr;
		Camera* Cam;
		Scene* Scn;
		MemoryArena* arena;
};

// Thread pointers
#define MAX_THREADS 64
int CurThreadSignal;
int thr_nr;
Thread_data* thr_dat_ptrs[MAX_THREADS];
Fl_Thread* thr_ptrs[MAX_THREADS];

#if defined(WIN32)
#define SLEEP1S Sleep(1000)
#else
#define SLEEP1S sleep(1)
#endif


// Control Implementations in Scene::
double Scene::Statistics_SamplesPPx()
{
	// collect samples from all threads
	double samples = 0.;
	for(int i = 0; i < thr_nr; i++) {
		samples += thr_dat_ptrs[i]->stat_Samples;		// TODO add mutex
	}

	// divide by total pixels
	return samples / (double) (camera->film->xResolution * camera->film->yResolution);
}

double Scene::Statistics_SamplesPSec()
{
	// collect samples from all threads
	double samples = 0.;
	for(int i = 0; i < thr_nr; i++) {
		samples += thr_dat_ptrs[i]->stat_Samples;		// TODO add mutex
	}

	double time = s_Timer.Time();
	double dif_samples = samples - lastSamples;
	double elapsed = time - lastTime;
	lastSamples = samples;
	lastTime = time;

	// return current samples / sec total
	if(elapsed != 0.)
		return dif_samples / elapsed;
	else
		return 0.;
}

void Scene::SignalThreads(int signal)
{
	for(int i = 0; i < thr_nr; i++)
		thr_dat_ptrs[i]->Sig = signal;
	CurThreadSignal = signal;
}

// Scene Methods -----------------------
void* Render_Thread( void* p )
{
	// unpack thread data
    Thread_data* t_d = (Thread_data*) p;
	int n = t_d->n;
	printf("THR%i: Started.\n", n+1);

	SurfaceIntegrator* surfaceIntegrator = (SurfaceIntegrator*) t_d->Si;
	VolumeIntegrator* volumeIntegrator = (VolumeIntegrator*) t_d->Vi;
    Sample* sample = t_d->Spl;
	Sampler* sampler = t_d->Splr;
	Scene* scene = t_d->Scn;
	Camera* camera = t_d->Cam;
	MemoryArena* arena = t_d->arena;

	// Trace rays: The main loop
	while (sampler->GetNextSample(sample)) {
		while(t_d->Sig == THR_SIG_PAUSE) { SLEEP1S; }
		if(t_d->Sig == THR_SIG_EXIT)
			break;

		// Find camera ray for _sample_
		RayDifferential ray;
		float rayWeight = camera->GenerateRay(*sample, &ray);
		// Generate ray differentials for camera ray
		++(sample->imageX);
		float wt1 = camera->GenerateRay(*sample, &ray.rx);
		--(sample->imageX);
		++(sample->imageY);
		float wt2 = camera->GenerateRay(*sample, &ray.ry);
		if (wt1 > 0 && wt2 > 0) ray.hasDifferentials = true;
		--(sample->imageY);
		// Evaluate radiance along camera ray
		float alpha;
		Spectrum Ls = 0.f;
		if (rayWeight > 0.f) {
			//Ls = rayWeight * scene->Li(ray, sample, &alpha); don't use
			Spectrum Lo = surfaceIntegrator->Li(*arena, scene, ray, sample, &alpha);
			Spectrum T = volumeIntegrator->Transmittance(scene, ray, sample, &alpha);
			Spectrum Lv = volumeIntegrator->Li(*arena, scene, ray, sample, &alpha);
			Ls = rayWeight * ( T * Lo + Lv );
			//Ls = rayWeight * Lo;
		} 
		// Issue warning if unexpected radiance value returned
		if (Ls.IsNaN()) {
			Error("THR%i: Nan radiance value returned.\n", n+1);
			Ls = Spectrum(0.f);
		}
		else if (Ls.y() < -1e-5) {
			Error("THR%i: NegLum value, %g, returned.\n", n+1, Ls.y());
			Ls = Spectrum(0.f);
		}
		else if (isinf(Ls.y())) {
			Error("THR%i: InfinLum value returned.\n", n+1);
			Ls = Spectrum(0.f);
		} 
		// Add sample contribution to image
		if( Ls != Spectrum(0.f) )
		   camera->film->AddSample(*sample, ray, Ls, alpha);

		t_d->stat_Samples++;

		//if( t_d->stat_Samples >= 100 )	// TEMP CODE FOR CHECKING SAMPLERS - radiance
		//	while(true) {SLEEP1S;}

		// Free BSDF memory from computing image sample value
		arena->FreeAll();
	}

	printf("THR%i: Exiting.\n", n+1);
#ifdef WIN32
	_endthread();
#else
	pthread_exit(0);
#endif
    return 0;
}

int Scene::CreateRenderThread()
{
	if(thr_nr < MAX_THREADS) {
		printf("CTL: Adding thread...\n");
		Thread_data* thr_dat = new Thread_data();

		// Set thread signal
		thr_dat->Sig = CurThreadSignal;

		// Set data
		thr_dat->n = thr_nr;
		thr_dat->stat_Samples = 0.;
		thr_dat->Si	= surfaceIntegrator->clone();									// SurfaceIntegrator (uc)
		thr_dat->Vi = volumeIntegrator->clone();									// VolumeIntegrator (uc)
		thr_dat->Spl = new Sample( (SurfaceIntegrator*) thr_dat->Si, 				// Sample (u)
			(VolumeIntegrator*) thr_dat->Vi, this );
		thr_dat->Splr = sampler->clone();											// Sampler (uc)	

		/* TODO add different seeds and unique backend random generator for threads - radiance */
		//thr_dat->Splr->setSeed( RandomUInt() );	

		thr_dat->Cam = camera;														// Camera (1)
		thr_dat->Scn = this;														// Scene (this)
		thr_dat->arena = new MemoryArena();											// MemoryArena (u)

		Fl_Thread* thr_ptr = new Fl_Thread();
		fl_create_thread(*thr_ptr, Render_Thread, thr_dat );
		thr_dat_ptrs[thr_nr] = thr_dat;
		thr_ptrs[thr_nr] = thr_ptr;
		printf("CTL: Done.\n");
		thr_nr++;
		return 0;
	} else {
		printf("CTL: Cannot create thread. (MAX_THREADS reached)\n");
		return 1;
	}
}

void Scene::RemoveRenderThread()
{
	printf("CTL: Removing thread...\n");
	thr_dat_ptrs[thr_nr -1]->Sig = THR_SIG_EXIT;
	//delete thr_dat_ptrs[thr_nr -1]->Si;
	//delete thr_dat_ptrs[thr_nr -1]->Vi;
	//delete thr_dat_ptrs[thr_nr -1]->Spl;
	//delete thr_dat_ptrs[thr_nr -1]->Splr;
	//delete thr_dat_ptrs[thr_nr -1]->arena;
	//delete thr_dat_ptrs[thr_nr -1];
	//delete thr_ptrs[thr_nr -1];
	thr_nr--;
	printf("CTL: Done.\n");
}

void Scene::Render() {
	// integrator preprocessing
	printf("CTL: Preprocessing integrators...\n");
    surfaceIntegrator->Preprocess(this);
    volumeIntegrator->Preprocess(this);

	// number of threads
	thr_nr = 0;

	// initial thread signal is paused
	CurThreadSignal = THR_SIG_PAUSE;

    // set current scene pointer
	luxCurrentScene = (Scene*) this;

	while(true) { SLEEP1S; }	// TODO fix for non-progressive rendering

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
	if (lts.size() == 0)
		Warning("No light sources defined in scene; "
			"possibly rendering a black image.");
	// Scene Constructor Implementation
	bound = aggregate->WorldBound();
	if (volumeRegion) bound = Union(bound, volumeRegion->WorldBound());
}
const BBox &Scene::WorldBound() const {
	return bound;
}
Spectrum Scene::Li(const RayDifferential &ray,
		const Sample *sample, float *alpha) const {
//	Spectrum Lo = surfaceIntegrator->Li(this, ray, sample, alpha);
//	Spectrum T = volumeIntegrator->Transmittance(this, ray, sample, alpha);
//	Spectrum Lv = volumeIntegrator->Li(this, ray, sample, alpha);
//	return T * Lo + Lv;
	Spectrum Lo;
	return Lo;
}
Spectrum Scene::Transmittance(const Ray &ray) const {
	return volumeIntegrator->Transmittance(this, ray, NULL, NULL);
//	Spectrum Dummy;
//	return Dummy;
}
