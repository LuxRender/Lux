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

void MLTPathIntegrator::RequestSamples(Sample *sample, const Scene *scene) {
	lightNumOffset = sample->Add1D(1);
	lightSampOffset = sample->Add2D(1);
	/* Other offsets handled by mlt getNext */
}

Spectrum MLTPathIntegrator::Li(MemoryArena &arena, const Scene *scene,
		const RayDifferential &r, const Sample *sample,
		float *alpha) const {
	RayDifferential ray(r);
	Spectrum pathThroughput = 1., L = 0., Le;
	int lightNum;
	Light *light;
	Vector wi;
	float lightWeight, lightPdf, ls1, ls2;
	VisibilityTester visibility;

	// Setup light params for this path
	lightNum = Floor2Int(sample->oneD[lightNumOffset][0] * scene->lights.size());
	lightNum = min(lightNum, (int)scene->lights.size() - 1);
	light = scene->lights[lightNum];
	lightWeight = float(scene->lights.size());
	ls1 = sample->twoD[lightSampOffset][0];
	ls2 = sample->twoD[lightSampOffset][1];

	if (alpha) *alpha = 1.;
	// NOTE - Ratow - Removed recursion: looping gives me an ~8% speed up.
	for (int pathLength = 0; ; ++pathLength) {
		// Find next vertex of path
		Intersection isect;
		if (!scene->Intersect(ray, &isect)) {
			for (u_int i = 0; i < scene->lights.size(); ++i)
				L += pathThroughput * scene->lights[i]->Le(ray);
			break;
		}

		// Evaluate BSDF at hit point
		BSDF *bsdf = isect.GetBSDF(arena, ray);
		const Point &p = bsdf->dgShading.p;
		const Normal &n = bsdf->dgShading.nn;
		Vector wo = -ray.d;
		Spectrum emittance = isect.Le(wo);

		pathThroughput *= scene->Transmittance(ray);

		if (emittance != 0.) {
			// Implicity light path
			L += pathThroughput * isect.Le(-ray.d);
		}	else {
			// Explicit light path: always connect to the same light
			Le = light->Sample_L(p, n,	ls1, ls2, &wi, &lightPdf, &visibility);
			Spectrum f = bsdf->f(wo, wi);
			if(lightPdf > 0. && !Le.Black() && !f.Black() && visibility.Unoccluded(scene))
				L += pathThroughput * lightWeight * f * Le * AbsDot(wi, n) / lightPdf;
		}

		if (pathLength == maxDepth)
			break;

		// Possibly terminate the path
		if (pathLength > 3) {
			// NOTE - Ratow - should the path length be the same after a small mutation?
			if (lux::random::floatValue() > continueProbability)
				break;
			// increase path contribution
			pathThroughput /= continueProbability;
		}

		// pick new direction for outgoing ray
		// using metropolis integration sampler
		float bs1, bs2, bcs;
		bs1 = bs2 = bcs = -1.;
		mltIntegrationSampler->GetNext(bs1, bs2, bcs, pathLength);

		Vector wi;
		float pdf;
		Spectrum f;
		BxDFType flags;

		// material, sample BSDF
		f = bsdf->Sample_f(wo, &wi, bs1, bs2, bcs, &pdf, BSDF_ALL, &flags);
		if (f.Black() || pdf == 0.)
			break;

		// trace reflected
		ray = RayDifferential(p, wi);

		pathThroughput *= f * AbsDot(wi, n) / pdf;
	}
	return L;
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
