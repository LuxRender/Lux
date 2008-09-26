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
#include "dynload.h"

using namespace lux;

// PathIntegrator Method Definitions
void PathIntegrator::RequestSamples(Sample *sample, const Scene *scene)
{
	if (lightStrategy == SAMPLE_AUTOMATIC) {
		if (scene->lights.size() > 5)
			lightStrategy = SAMPLE_ONE_UNIFORM;
		else
			lightStrategy = SAMPLE_ALL_UNIFORM;
	}

	vector<u_int> structure;
	structure.push_back(2);	// light position sample
	structure.push_back(1);	// light number sample
	structure.push_back(2);	// bsdf direction sample for light
	structure.push_back(1);	// bsdf component sample for light
	structure.push_back(2);	// bsdf direction sample for path
	structure.push_back(1);	// bsdf component sample for path
	if (rrStrategy != RR_NONE)
		structure.push_back(1);	// continue sample
	sampleOffset = sample->AddxD(structure, maxDepth + 1);
}

SWCSpectrum PathIntegrator::Li(const TsPack *tspack, const Scene *scene,
		const RayDifferential &r, const Sample *sample,
		float *alpha) const
{
	SampleGuard guard(sample->sampler, sample);
	float nrContribs = 0.f;
	// Declare common path integration variables
	RayDifferential ray(r);
	SWCSpectrum pathThroughput(1.0f);
	SWCSpectrum L(0.0f);
	XYZColor color;
	float V = .1f;
	bool specularBounce = true, specular = true;
	if (alpha) *alpha = 1.;
	for (int pathLength = 0; ; ++pathLength) {
		// Find next vertex of path
		Intersection isect;
		if (!scene->Intersect(ray, &isect)) {
			if (pathLength == 0) {
				// Dade - now I know ray.maxt and I can call volumeIntegrator
				L = scene->volumeIntegrator->Li(tspack, scene, ray, sample, alpha);
				color = L.ToXYZ(tspack);
				if (!L.Black()) {
					sample->AddContribution(sample->imageX, sample->imageY,
						L.ToXYZ(tspack), alpha ? *alpha : 1.f, V);
					++nrContribs;
				}
				pathThroughput = 1.f;
				scene->volumeIntegrator->Transmittance(tspack, scene, ray, sample, alpha, &pathThroughput);
			}

			// Stop path sampling since no intersection was found
			// Possibly add emitted light
			// NOTE - Added by radiance - adds horizon in render & reflections
			if (specularBounce) {
				SWCSpectrum Le(0.f);
				for (u_int i = 0; i < scene->lights.size(); ++i)
					Le += scene->lights[i]->Le(tspack, ray);
				Le *= pathThroughput;
				L += Le;
				color = Le.ToXYZ(tspack);
			}
			// Set alpha channel
			if (pathLength == 0 && alpha && !(color.y() > 0.f))
				*alpha = 0.;
			if (color.y() > 0.f) {
				sample->AddContribution(sample->imageX, sample->imageY,
					color, alpha ? *alpha : 1.f, V);
				++nrContribs;
			}
			break;
		}
		if (pathLength == 0)
			r.maxt = ray.maxt;

		SWCSpectrum Lv(scene->volumeIntegrator->Li(tspack, scene, ray, sample, alpha));
		Lv *= pathThroughput;
		color = Lv.ToXYZ(tspack);
		if (color.y() > 0.f) {
			sample->AddContribution(sample->imageX, sample->imageY,
				color, alpha ? *alpha : 1.f, V);
			++nrContribs;
		}
		scene->volumeIntegrator->Transmittance(tspack, scene, ray, sample, alpha, &pathThroughput);

		// Possibly add emitted light at path vertex
		Vector wo(-ray.d);
		if (specularBounce) {
			SWCSpectrum Le(isect.Le(tspack, wo));
			Le *= pathThroughput;
			L += Le;
			color = Le.ToXYZ(tspack);
			if (color.y() > 0.f) {
				sample->AddContribution(sample->imageX, sample->imageY,
					color, alpha ? *alpha : 1.f, V);
				++nrContribs;
			}
		}
		if (pathLength == maxDepth)
			break;
		// Evaluate BSDF at hit point
		float *data = sample->sampler->GetLazyValues(const_cast<Sample *>(sample), sampleOffset, pathLength);
		BSDF *bsdf = isect.GetBSDF(tspack, ray, fabsf(2.f * data[5] - 1.f));
		// Sample illumination from lights to find path contribution
		const Point &p = bsdf->dgShading.p;
		const Normal &n = bsdf->dgShading.nn;

		SWCSpectrum Ll;
		switch (lightStrategy) {
			case SAMPLE_ALL_UNIFORM:
				Ll = UniformSampleAllLights(tspack, scene, p, n,
					wo, bsdf, sample,
					data, data + 2, data + 3, data + 5);
				break;
			case SAMPLE_ONE_UNIFORM:
				Ll = UniformSampleOneLight(tspack, scene, p, n,
					wo, bsdf, sample,
					data, data + 2, data + 3, data + 5);
				break;
			default:
				Ll = 0.f;
		}
		Ll *= pathThroughput;
		L += Ll;
		color = Ll.ToXYZ(tspack);
		if (color.y() > 0.f) {
			sample->AddContribution(sample->imageX, sample->imageY,
				color, alpha ? *alpha : 1.f, V);
			++nrContribs;
		}

		// Sample BSDF to get new path direction
		Vector wi;
		float pdf;
		BxDFType flags;
		SWCSpectrum f;
		if (!bsdf->Sample_f(tspack, wo, &wi, data[6], data[7], data[8], &f, &pdf, BSDF_ALL, &flags))
			break;

		const float dp = AbsDot(wi, n) / pdf;

		// Possibly terminate the path
		if (pathLength > 3) {
			if (rrStrategy == RR_EFFICIENCY) { // use efficiency optimized RR
				const float q = min<float>(1.f, f.filter(tspack) * dp);
				if (q < data[9])
					break;
				// increase path contribution
				pathThroughput /= q;
			} else if (rrStrategy == RR_PROBABILITY) { // use normal/probability RR
				if (continueProbability < data[9])
					break;
				// increase path contribution
				pathThroughput /= continueProbability;
			}
		}

		specularBounce = (flags & BSDF_SPECULAR) != 0;
		specular = specular && specularBounce;
		pathThroughput *= f;
		pathThroughput *= dp;
		if (!specular)
			V += dp;

		ray = RayDifferential(p, wi);
	}

	return nrContribs;
}
SurfaceIntegrator* PathIntegrator::CreateSurfaceIntegrator(const ParamSet &params)
{
	// general
	int maxDepth = params.FindOneInt("maxdepth", 16);
	float RRcontinueProb = params.FindOneFloat("rrcontinueprob", .65f);			// continueprobability for plain RR (0.0-1.0)
	LightStrategy estrategy;
	string st = params.FindOneString("strategy", "auto");
	if (st == "one") estrategy = SAMPLE_ONE_UNIFORM;
	else if (st == "all") estrategy = SAMPLE_ALL_UNIFORM;
	else if (st == "auto") estrategy = SAMPLE_AUTOMATIC;
	else {
		std::stringstream ss;
		ss<<"Strategy  '"<<st<<"' for direct lighting unknown. Using \"auto\".";
		luxError(LUX_BADTOKEN,LUX_WARNING,ss.str().c_str());
		estrategy = SAMPLE_AUTOMATIC;
	}
	RRStrategy rstrategy;
	string rst = params.FindOneString("rrstrategy", "efficiency");
	if (rst == "efficiency") rstrategy = RR_EFFICIENCY;
	else if (rst == "probability") rstrategy = RR_PROBABILITY;
	else if (rst == "none") rstrategy = RR_NONE;
	else {
		std::stringstream ss;
		ss<<"Strategy  '"<<st<<"' for russian roulette path termination unknown. Using \"efficiency\".";
		luxError(LUX_BADTOKEN,LUX_WARNING,ss.str().c_str());
		rstrategy = RR_EFFICIENCY;
	}

	return new PathIntegrator(estrategy, rstrategy, maxDepth, RRcontinueProb);
}

static DynamicLoader::RegisterSurfaceIntegrator<PathIntegrator> r("path");
