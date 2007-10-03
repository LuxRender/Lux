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

//#include "luxgui.h" << jromang : We cannot include FLTK Stuff here ! libcore needs to build without FLTK/OpenGL dependencies 

//here are the control methods
extern Scene *luxCurrentScene;

//Control methods
void Scene::Start() {}
void Scene::Pause() {}
void Scene::Exit() {}

//controlling number of threads
int Scene::AddThread() {}
void Scene::RemoveThread() {}

//framebuffer access
void Scene::UpdateFramebuffer() {}
unsigned char* Scene::GetFramebuffer() { return NULL; }
int Scene::DisplayInterval() { return 1; }

//statistics
double Scene::Statistics(char *statName) { return 0; }




extern Scene* GuiScenePtr;	// init in luxgui.cpp

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
Thread_data* thr_dat_ptrs[64];
Fl_Thread* thr_ptrs[64];

// Scene Methods
void* Render_Thread( void* p )
{
	// unpack thread data
    Thread_data* t_d = (Thread_data*) p;
	int n = t_d->n;
	printf("THR%i: thread started\n", n+1);

	SurfaceIntegrator* surfaceIntegrator = (SurfaceIntegrator*) t_d->Si;
	VolumeIntegrator* volumeIntegrator = (VolumeIntegrator*) t_d->Vi;
    Sample* sample = t_d->Spl;
	Sampler* sampler = t_d->Splr;
	Scene* scene = t_d->Scn;
	Camera* camera = t_d->Cam;
	MemoryArena* arena = t_d->arena;

	// Trace rays: The main loop
	//ProgressReporter progress(sampler->TotalSamples(), "Rendering");
	//while (t_d->Sig != THR_SIG_EXIT) // TODO add sleeps
		//while (t_d->Sig == THR_SIG_RUN && sampler->GetNextSample(sample)) {
	while (sampler->GetNextSample(sample)) {
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

			// Report rendering progress
			//static StatsCounter cameraRaysTraced("Camera", "Camera Rays Traced");
			//++cameraRaysTraced;
			//progress.Update();
		}

	_endthread();
    return 0;
}

void Scene::Render() {
	// set pointer for gui
	GuiScenePtr = (Scene*) this;
	luxCurrentScene=this;

	// integrator preprocessing
	printf("CTL: Preprocessing integrators...\n");
    surfaceIntegrator->Preprocess(this);
    volumeIntegrator->Preprocess(this);

	// init threads
	int thr_nr = 3;

	u_int seeds[4];
	seeds[0] = 536870912;
	seeds[1] = 1073741824;
	seeds[2] = 1610612736;
	seeds[3] = 2147483648;

	// lock gui TODO add check
	Fl::lock();    

	// create thread data structures and launch threads
	printf("CTL: Initializing %i render threads.\n", thr_nr);
	for( int i = 0; i < thr_nr; i++ ) {
		Thread_data* thr_dat = new Thread_data();
		// Set signal to pause
		thr_dat->Sig = THR_SIG_PAUSE;
		// Set data
		thr_dat->n = i;
		thr_dat->Si	= surfaceIntegrator->clone();									// SurfaceIntegrator (uc)
		thr_dat->Vi = volumeIntegrator->clone();									// VolumeIntegrator (uc)
		thr_dat->Spl = new Sample( (SurfaceIntegrator*) thr_dat->Si, 				// Sample (u)
			(VolumeIntegrator*) thr_dat->Vi, this );
		thr_dat->Splr = sampler->clone();											// Sampler (uc)		
		thr_dat->Splr->setSeed( seeds[i] );	
		thr_dat->Cam = camera;														// Camera (1)
		thr_dat->Scn = this;														// Scene (this)
		thr_dat->arena = new MemoryArena();											// MemoryArena (u)			// TODO delete sample * memoryarena

		Fl_Thread* thr_ptr = new Fl_Thread();
		fl_create_thread((Fl_Thread&)thr_ptr, Render_Thread, thr_dat );	// TODO make thr_ptr[i] ?
		thr_dat_ptrs[i] = thr_dat;
		thr_ptrs[i] = thr_ptr;
	}

	// Start Threads
	printf("CTL: Signaling threads to start...\n");
	for( int i = 0; i < thr_nr; i++ )
		thr_dat_ptrs[i]->Sig = THR_SIG_RUN;
   
	// unlock gui TODO add check
    Fl::unlock();

#if defined(WIN32)
	while(true) { Sleep(1000); } // win32 Sleep(milliseconds)
#else
	while(true) { sleep(1); }	// linux/gcc sleep(seconds)
#endif


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
