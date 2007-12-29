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

// MLT path tracing integrator (unfinished) by Radiance
// needs RR path termination and implicit illumination at end of path

// mltpath.cpp*
#include "mltpath.h"

using namespace lux;

// Lux (copy) constructor
MLTPathIntegrator* MLTPathIntegrator::clone() const
 {
   return new MLTPathIntegrator(*this);
 }
// MLTPathIntegrator Method Definitions
IntegrationSampler* MLTPathIntegrator::HasIntegrationSampler(IntegrationSampler *is) {
	IntegrationSampler *isa = NULL;
	isa = new Metropolis();	// TODO - radiance - delete afterwards in renderthread
	isa->SetParams(maxReject, pLarge);
	mltIntegrationSampler = isa;
    return isa;
}

Spectrum MLTPathIntegrator::Next(MemoryArena &arena, RayDifferential ray, 
		const Scene *scene, int pathLength) const {
	if (pathLength == maxDepth)
		return 0.;

	// TODO add russian roulette path termination and weighting

	// Find next vertex of path
	Intersection isect;
	if (!scene->Intersect(ray, &isect)) {
		Spectrum L = 0.;
		for (u_int i = 0; i < scene->lights.size(); ++i)
			L += scene->lights[i]->Le(ray); 
		return L;
	}

	// Evaluate BSDF at hit point
	BSDF *bsdf = isect.GetBSDF(arena, ray);
	const Point &p = bsdf->dgShading.p;
	const Normal &n = bsdf->dgShading.nn;
	Vector wo = -ray.d;
	Spectrum emittance = isect.Le(-ray.d);

	// pick new direction for outgoing ray 
	// using metropolis integration sampler
	float bs1, bs2, bcs;
	bs1 = bs2 = bcs = -1.;
	mltIntegrationSampler->GetNext(bs1, bs2, bcs, pathLength);

	Vector wi;
	float pdf;
	Spectrum f;
	BxDFType flags;

	if(emittance != 0.) {
		// white BSDF for area light
		f = 1.; pdf = 1.;
	} else {
		// material, sample BSDF
		f = bsdf->Sample_f(wo, &wi, bs1, bs2, bcs,
			&pdf, BSDF_ALL, &flags);
		if (f.Black() || pdf == 0.)
			return 0.;
	}

	// trace reflected
	ray = RayDifferential(p, wi);
	Spectrum reflected = Next(arena, ray, scene, pathLength+1);

	// return total
	return emittance + (f * AbsDot(wi, n) / pdf) * reflected;
}

Spectrum MLTPathIntegrator::Li(MemoryArena &arena, const Scene *scene,
		const RayDifferential &r, const Sample *sample,
		float *alpha) const {
	RayDifferential ray(r);
	if (alpha) *alpha = 1.;
	return Next(arena, ray, scene, 0);
}

SurfaceIntegrator* MLTPathIntegrator::CreateSurfaceIntegrator(const ParamSet &params) {
	// general
	int maxDepth = params.FindOneInt("maxdepth", 32);
	float RRcontinueProb = params.FindOneFloat("rrcontinueprob", .65f);			// continueprobability for RR (0.0-1.0)
	// MLT
	int MaxConsecRejects = params.FindOneInt("maxconsecrejects", 512);          // number of consecutive rejects before a new mutation is forced
	float LargeMutationProb = params.FindOneFloat("largemutationprob", .4f);	// probability of generation a large sample (mutation)

	return new MLTPathIntegrator(maxDepth, RRcontinueProb, MaxConsecRejects, LargeMutationProb);

}
