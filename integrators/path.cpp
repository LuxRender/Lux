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

// path.cpp*
#include "path.h"

// PathIntegrator Method Definitions
void PathIntegrator::RequestSamples(Sample *sample,
		const Scene *scene) {
	for (int i = 0; i < SAMPLE_DEPTH; ++i) {
		lightPositionOffset[i] = sample->Add2D(1);
		lightNumOffset[i] = sample->Add1D(1);
		bsdfDirectionOffset[i] = sample->Add2D(1);
		bsdfComponentOffset[i] = sample->Add1D(1);
		outgoingDirectionOffset[i] = sample->Add2D(1);
		outgoingComponentOffset[i] = sample->Add1D(1);
	}
}
Spectrum PathIntegrator::Li(const Scene *scene,
		const RayDifferential &r, const Sample *sample,
		float *alpha) const {
	// Declare common path integration variables
	Spectrum pathThroughput = 1., L = 0.;
	RayDifferential ray(r);
	bool specularBounce = false;
	for (int pathLength = 0; ; ++pathLength) {
		// Find next vertex of path
		Intersection isect;
		if (!scene->Intersect(ray, &isect)) {
			// Stop path sampling since no intersection was found
			if (pathLength == 0 && alpha) {
				if (L != 0.) *alpha = 1.;
				else *alpha = 0.;
			}
			break;
		}
		if (pathLength == 0) {
			r.maxt = ray.maxt;
			if (alpha) *alpha = 1.;
		}
		else
			pathThroughput *= scene->Transmittance(ray);
		// Possibly add emitted light at path vertex
		if (pathLength == 0 || specularBounce)
			L += pathThroughput * isect.Le(-ray.d);
		// Evaluate BSDF at hit point
		BSDF *bsdf = isect.GetBSDF(ray);
		// Sample illumination from lights to find path contribution
		const Point &p = bsdf->dgShading.p;
		const Normal &n = bsdf->dgShading.nn;
		Vector wo = -ray.d;
		if (pathLength < SAMPLE_DEPTH)
			L += pathThroughput *
				UniformSampleOneLight(scene, p, n,
					wo, bsdf, sample,
					lightPositionOffset[pathLength],
					lightNumOffset[pathLength],
					bsdfDirectionOffset[pathLength],
					bsdfComponentOffset[pathLength]);
		else
			L += pathThroughput *
				UniformSampleOneLight(scene, p, n,
					wo, bsdf, sample);
		// Sample BSDF to get new path direction
		// Get random numbers for sampling new direction, _bs1_, _bs2_, and _bcs_
		float bs1, bs2, bcs;
		if (pathLength < SAMPLE_DEPTH) {
			bs1 = sample->twoD[outgoingDirectionOffset[pathLength]][0];
			bs2 = sample->twoD[outgoingDirectionOffset[pathLength]][1];
			bcs = sample->oneD[outgoingComponentOffset[pathLength]][0];
		}
		else {
			bs1 = RandomFloat();
			bs2 = RandomFloat();
			bcs = RandomFloat();
		}
		Vector wi;
		float pdf;
		BxDFType flags;
		Spectrum f = bsdf->Sample_f(wo, &wi, bs1, bs2, bcs,
			&pdf, BSDF_ALL, &flags);
		if (f.Black() || pdf == 0.)
			break;
		specularBounce = (flags & BSDF_SPECULAR) != 0;
		pathThroughput *= f * AbsDot(wi, n) / pdf;
		ray = RayDifferential(p, wi);
		// Possibly terminate the path
		if (pathLength > 3) {
			float continueProbability = .5f;
			if (RandomFloat() > continueProbability)
				break;
			pathThroughput /= continueProbability;
		}
		if (pathLength == maxDepth)
			break;
	}
	return L;
}
SurfaceIntegrator* PathIntegrator::CreateSurfaceIntegrator(const ParamSet &params) {
	int maxDepth = params.FindOneInt("maxdepth", 5);
	return new PathIntegrator(maxDepth);
}
