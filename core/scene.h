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
#include "camera.h"
#include "film.h"

#include <iostream>
#include <vector>
#include <boost/thread/thread.hpp>
#include <boost/thread/xtime.hpp>
#include <boost/noncopyable.hpp>
#include <boost/bind.hpp>
//#include <boost/thread/mutex.hpp>

class RenderThread : public boost::noncopyable
{
	public:
		RenderThread( int _n, int _signal, SurfaceIntegrator* _Si, VolumeIntegrator* _Vi, Sampler* _Splr, Camera* _Cam, Scene* _Scn)
			: n(_n), signal(_signal), surfaceIntegrator(_Si), volumeIntegrator(_Vi), sampler(_Splr), camera(_Cam), scene(_Scn)
		{
			stat_Samples=0;
			sample=new Sample( surfaceIntegrator, volumeIntegrator, scene);

			// Radiance - hand the sample struct to the integrationsampler if used by the surfaceintegrator
			integrationSampler = surfaceIntegrator->HasIntegrationSampler(NULL);
			if(integrationSampler)
				integrationSampler->SetFilmRes(_Cam->film->xResolution, _Cam->film->yResolution);

			arena=new MemoryArena();
			//std::cout<<"yepeee, creating thread"<<std::endl;
		}
	
		~RenderThread()
		{
			delete sample;
			delete arena;	
			delete thread;
		}
		
		static void render(RenderThread *r);
		
		int  n, signal;
		double stat_Samples;
		SurfaceIntegrator *surfaceIntegrator;
		VolumeIntegrator *volumeIntegrator;
		IntegrationSampler *integrationSampler;
		Sample *sample;
		Sampler *sampler;
		Camera *camera;
		Scene *scene;
		MemoryArena* arena;
		boost::thread *thread; //keep pointer the delete the thread object
		
		static const int SIG_RUN=1, SIG_PAUSE=2, SIG_EXIT=3;
};

// Scene Declarations
class  Scene {
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
	double lastSamples, lastTime;
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
	
	private:
		std::vector<RenderThread*> renderThreads;
		//boost::thread_group threadGroup;
		int CurThreadSignal;
};
#endif // LUX_SCENE_H
