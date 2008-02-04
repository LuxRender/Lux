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
	MLTPathIntegrator *path = new MLTPathIntegrator(*this);
	path->continueOffset = new int[maxDepth];
	path->outgoingDirectionOffset = new int[maxDepth];
	path->outgoingComponentOffset = new int[maxDepth];
	for (int i = 0; i < maxDepth; ++i) {
		path->continueOffset[i] = continueOffset[i];
		path->outgoingDirectionOffset[i] = outgoingDirectionOffset[i];
		path->outgoingComponentOffset[i] = outgoingComponentOffset[i];
	}
	return path;
 }
// MLTPathIntegrator Method Definitions
void MLTPathIntegrator::RequestSamples(Sample *sample, const Scene *scene)
{
	lightNumOffset = sample->Add1D(1);
	lightSampOffset = sample->Add2D(1);
	for (int i = 0; i < maxDepth; ++i) {
		continueOffset[i] = sample->Add1D(1);
		outgoingDirectionOffset[i] = sample->Add2D(1);
		outgoingComponentOffset[i] = sample->Add1D(1);
	}
}

Spectrum MLTPathIntegrator::Li(const Scene *scene,
		const RayDifferential &r, const Sample *sample,
		float *alpha) const
{
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
	for (int pathLength = 1; ; ++pathLength) {
		// Find next vertex of path
		Intersection isect;
		if (!scene->Intersect(ray, &isect)) {
			for (u_int i = 0; i < scene->lights.size(); ++i)
				L += pathThroughput * scene->lights[i]->Le(ray);
			break;
		}

		// Evaluate BSDF at hit point
		BSDF *bsdf = isect.GetBSDF(ray);
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

		// pick new direction for outgoing ray
		// using metropolis integration sampler
		float bs1, bs2, bcs;
		bs1 = sample->twoD[outgoingDirectionOffset[pathLength]][0];
		bs2 = sample->twoD[outgoingDirectionOffset[pathLength]][1];
		bcs = sample->oneD[outgoingComponentOffset[pathLength]][0];
		Vector wi;
		float pdf;
		Spectrum f;
		BxDFType flags;

		// material, sample BSDF
		f = bsdf->Sample_f(wo, &wi, bs1, bs2, bcs, &pdf, BSDF_ALL, &flags);
		if (f.Black() || pdf == 0.)
			break;

		// Possibly terminate the path
		if (pathLength > 3) {
			float q = min(1.0f, f.y()/pdf);
			if (q < sample->oneD[continueOffset[pathLength]][0])
				break;
			pathThroughput /= q;
		}

		// trace reflected
		ray = RayDifferential(p, wi);

		// increase path contribution
		pathThroughput *= f * AbsDot(wi, n) / pdf;
	}
	return L;
}

SurfaceIntegrator* MLTPathIntegrator::CreateSurfaceIntegrator(const ParamSet &params)
{
	// general
	int maxDepth = params.FindOneInt("maxdepth", 32);
	return new MLTPathIntegrator(maxDepth);

}

