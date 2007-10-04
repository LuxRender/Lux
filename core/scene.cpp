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

//here are the control methods
extern Scene *luxCurrentScene;

//Control methods
void Scene::Start() { SignalThreads(THR_SIG_RUN); }
void Scene::Pause() { SignalThreads(THR_SIG_PAUSE); }
void Scene::Exit() { SignalThreads(THR_SIG_EXIT); }

//controlling number of threads
int Scene::AddThread() { return CreateRenderThread(); }
void Scene::RemoveThread() { RemoveRenderThread(); }

//framebuffer access
void Scene::UpdateFramebuffer() { camera->film->updateFrameBuffer(); }
unsigned char* Scene::GetFramebuffer() { return camera->film->getFrameBuffer(); }
int Scene::DisplayInterval() { return camera->film->getldrDisplayInterval(); }
int Scene::FilmXres() { return camera->film->xResolution; }
int Scene::FilmYres() { return camera->film->yResolution; }

//statistics
double Scene::Statistics(char *statName) { return 0; }

// thread data pack class
class Thread_data
{
    public:
		int Sig, n;
		Integrator* Si;
		Integrator* Vi;
		Sample* Spl;
		Sampler* Splr;
		Camera* Cam;
		Scene* Scn;
		MemoryArena* arena;
};

// thread pointers
#define MAX_THREADS 64
int CurThreadSignal;
int thr_nr;
Thread_data* thr_dat_ptrs[64];
Fl_Thread* thr_ptrs[64];

#if defined(WIN32)
#define SLEEP1S Sleep(1000)
#else
#define SLEEP1S sleep(1)
#endif

// Scene Methods
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

		// Free BSDF memory from computing image sample value
		arena->FreeAll();
	}

	printf("THR%i: Exiting.\n", n+1);
	_endthread();
    return 0;
}

void Scene::SignalThreads(int signal)
{
	for(int i = 0; i < thr_nr; i++)
		thr_dat_ptrs[i]->Sig = signal;
	CurThreadSignal = signal;
}

int Scene::CreateRenderThread()
{
	if(thr_nr < MAX_THREADS) {
		printf("CTL: Adding thread...\n");
		Thread_data* thr_dat = new Thread_data();

		// Set signal to pause
		thr_dat->Sig = CurThreadSignal;

		// Set data
		thr_dat->n = thr_nr;
		thr_dat->Si	= surfaceIntegrator->clone();									// SurfaceIntegrator (uc)
		thr_dat->Vi = volumeIntegrator->clone();									// VolumeIntegrator (uc)
		thr_dat->Spl = new Sample( (SurfaceIntegrator*) thr_dat->Si, 				// Sample (u)
			(VolumeIntegrator*) thr_dat->Vi, this );
		thr_dat->Splr = sampler->clone();											// Sampler (uc)		
		thr_dat->Splr->setSeed( RandomUInt() );	
		thr_dat->Cam = camera;														// Camera (1)
		thr_dat->Scn = this;														// Scene (this)
		thr_dat->arena = new MemoryArena();											// MemoryArena (u)	// TODO delete sample * memoryarena

		Fl_Thread* thr_ptr = new Fl_Thread();
		fl_create_thread((Fl_Thread&)thr_ptr, Render_Thread, thr_dat );
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
	thr_nr--;
	printf("CTL: Done.\n");
}

void Scene::Render() {
	// integrator preprocessing
	printf("CTL: Preprocessing integrators...\n");
    surfaceIntegrator->Preprocess(this);
    volumeIntegrator->Preprocess(this);

	thr_nr = 0;

	CurThreadSignal = THR_SIG_RUN;

    // set current scene pointer
	luxCurrentScene = (Scene*) this;

	while(true) { SLEEP1S; }	// TODO fix for non-progressive rendering

	return; // everything worked fine! Have a great day :) */
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
