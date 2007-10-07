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

#ifndef LUX_SCENE_H
#define LUX_SCENE_H
// scene.h*
#include "lux.h"
#include "primitive.h"
#include "transport.h"
#include "timer.h"

//#include <boost/thread/thread.hpp>
//#include <boost/thread/mutex.hpp>

#  if HAVE_PTHREAD_H
// Use POSIX threading...

#    include <pthread.h>

typedef pthread_t Fl_Thread;

inline int fl_create_thread(Fl_Thread& t, void *(*f) (void *), void* p) {
  return pthread_create((pthread_t*)&t, 0, f, p);
}

#  elif defined(WIN32) && !defined(__WATCOMC__) // Use Windows threading...

#    include <windows.h>
#    include <process.h>

typedef unsigned long Fl_Thread;

static int fl_create_thread(Fl_Thread& t, void *(*f) (void *), void* p) {
  return t = (Fl_Thread)_beginthread((void( __cdecl * )( void * ))f, 0, p);
}

#  elif defined(__WATCOMC__)
#    include <process.h>

typedef unsigned long Fl_Thread;

static int fl_create_thread(Fl_Thread& t, void *(*f) (void *), void* p) {
  return t = (Fl_Thread)_beginthread((void(* )( void * ))f, 32000, p);
}
#  endif // !HAVE_PTHREAD_H

#define THR_SIG_RUN 1
#define THR_SIG_PAUSE 2
#define THR_SIG_EXIT 3

// Scene Declarations
class COREDLL Scene {
public:
	// Scene Public Methods
	void Render();
	Scene(Camera *c, SurfaceIntegrator *in,
		VolumeIntegrator *vi, Sampler *s,
		Primitive *accel, const vector<Light *> &lts,
		VolumeRegion *vr);
	~Scene();
	bool Intersect(const Ray &ray, Intersection *isect) const {
		return aggregate->Intersect(ray, isect);
	}
	bool IntersectP(const Ray &ray) const {
		return aggregate->IntersectP(ray);
	}
	const BBox &WorldBound() const;
	Spectrum Li(const RayDifferential &ray, const Sample *sample,
		float *alpha = NULL) const;
	Spectrum Transmittance(const Ray &ray) const;
	
	//Control methods
	void Start();
	void Pause();
	void Exit();
	//controlling number of threads
	int AddThread(); //returns the thread ID
	void RemoveThread();
	double Statistics_SamplesPSec();
	double Statistics_SamplesPPx();
	//framebuffer access
	void UpdateFramebuffer();
	unsigned char* GetFramebuffer();
	int DisplayInterval();
	int FilmXres();
	int FilmYres();
	Timer s_Timer;
	//statistics
	double Statistics(char *statName);

	void SignalThreads(int signal);
	int CreateRenderThread();
	void RemoveRenderThread();
	
	
	// Scene Data
	Primitive *aggregate;
	vector<Light *> lights;
	Camera *camera;
	VolumeRegion *volumeRegion;
	SurfaceIntegrator *surfaceIntegrator;
	VolumeIntegrator *volumeIntegrator;
	Sampler *sampler;
	BBox bound;
};
#endif // LUX_SCENE_H
