/***************************************************************************
 *   Copyright (C) 1998-2008 by authors (see AUTHORS.txt )                 *
 *                                                                         *
 *   This file is part of LuxRender.                                       *
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
 *   Lux Renderer website : http://www.luxrender.net                       *
 ***************************************************************************/

// path.cpp*
#include "path.h"
#include "bxdf.h"
#include "light.h"
#include "paramset.h"

using namespace lux;

// Lux (copy) constructor
PathIntegrator* PathIntegrator::clone() const
{
	PathIntegrator *path = new PathIntegrator(*this);
	return path;
}
// PathIntegrator Method Definitions
void PathIntegrator::RequestSamples(Sample *sample, const Scene *scene)
{
	vector<u_int> structure;
	structure.push_back(2);
	structure.push_back(1);
	structure.push_back(2);
	structure.push_back(1);
	structure.push_back(1);
	structure.push_back(2);
	structure.push_back(1);
	sampleOffset = sample->AddxD(structure, maxDepth + 1);
}

SWCSpectrum PathIntegrator::Li(const Scene *scene,
		const RayDifferential &r, const Sample *sample,
		float *alpha) const
{
	// Declare common path integration variables
	SWCSpectrum pathThroughput = 1., L = 0.;
	RayDifferential ray(r);
	bool specularBounce = true;
	if (alpha) *alpha = 1.;
	for (int pathLength = 0; ; ++pathLength) {
		// Find next vertex of path
		Intersection isect;
		if (!scene->Intersect(ray, &isect)) {
			// Stop path sampling since no intersection was found
			// Possibly add emitted light
			// NOTE - Added by radiance - adds horizon in render & reflections
			if (specularBounce) {
				SWCSpectrum Le(0.f);
				for (u_int i = 0; i < scene->lights.size(); ++i)
					Le += scene->lights[i]->Le(ray);
				Le *= pathThroughput;
				L += Le;
			}
			// Set alpha channel
			if (pathLength == 0 && alpha && L.Black())
				*alpha = 0.;
			break;
		}
		if (pathLength == 0) {
			r.maxt = ray.maxt;
		}
		else
			pathThroughput *= scene->Transmittance(ray);
		// Possibly add emitted light at path vertex
		if (specularBounce)
			L += pathThroughput * isect.Le(-ray.d);
		if (pathLength == maxDepth)
			break;
		// Evaluate BSDF at hit point
		BSDF *bsdf = isect.GetBSDF(ray);
		// Sample illumination from lights to find path contribution
		const Point &p = bsdf->dgShading.p;
		const Normal &n = bsdf->dgShading.nn;
		Vector wo = -ray.d;
		float *data = sample->sampler->GetLazyValues(const_cast<Sample *>(sample), sampleOffset, pathLength);
		L += pathThroughput *
			UniformSampleOneLight(scene, p, n,
				wo, bsdf, sample,
				data, data + 2, data + 3, data + 5);

		// Possibly terminate the path
		if (pathLength > 3) {
			if (data[6] > continueProbability)
				break;

			// increase path contribution
			pathThroughput /= continueProbability;
		}
		// Sample BSDF to get new path direction
		Vector wi;
		float pdf;
		BxDFType flags;
		SWCSpectrum f = bsdf->Sample_f(wo, &wi, data[7], data[8], data[9],
			&pdf, BSDF_ALL, &flags);
		if (pdf == .0f || f.Black())
			break;
		specularBounce = (flags & BSDF_SPECULAR) != 0;
		pathThroughput *= f;
		pathThroughput *= AbsDot(wi, n) / pdf;

		ray = RayDifferential(p, wi);
	}
	return L;
}
SurfaceIntegrator* PathIntegrator::CreateSurfaceIntegrator(const ParamSet &params)
{
	// general
	int maxDepth = params.FindOneInt("maxdepth", 16);
	float RRcontinueProb = params.FindOneFloat("rrcontinueprob", .65f);			// continueprobability for RR (0.0-1.0)

	return new PathIntegrator(maxDepth, RRcontinueProb);

}

