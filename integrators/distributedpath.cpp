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

// distributedpath.cpp*
#include "distributedpath.h"
#include "bxdf.h"
#include "paramset.h"

using namespace lux;

// DistributedPath Method Definitions
DistributedPath::DistributedPath(LightStrategy st, int dd, int gd, int sd) {
	diffuseDepth = dd;
	glossyDepth = gd;
	specularDepth = sd;
	lightStrategy = st;
}

void DistributedPath::RequestSamples(Sample *sample, const Scene *scene) {
	if (lightStrategy == SAMPLE_AUTOMATIC) {
		if (scene->lights.size() > 7)
			lightStrategy = SAMPLE_ONE_UNIFORM;
		else
			lightStrategy = SAMPLE_ALL_UNIFORM;
	}

	vector<u_int> structure;

	structure.push_back(2); // diffuse reflection direction sample for bsdf
	structure.push_back(1); // diffuse reflection component sample for bsdf
	structure.push_back(2); // diffuse transmission direction sample for bsdf
	structure.push_back(1); // diffuse transmission component sample for bsdf
	structure.push_back(2); // glossy reflection direction sample for bsdf
	structure.push_back(1); // glossy reflection component sample for bsdf
	structure.push_back(2); // glossy transmission direction sample for bsdf
	structure.push_back(1); // glossy transmission component sample for bsdf

	// Dade - allocate and request samples for sampling one light
	structure.push_back(2);	// light position sample
	structure.push_back(1);	// light number/portal sample
	structure.push_back(2);	// bsdf direction sample for light
	structure.push_back(1);	// bsdf component sample for light

	int maxDepth = diffuseDepth;
	if(glossyDepth > diffuseDepth) maxDepth = glossyDepth;
	if(specularDepth > glossyDepth) maxDepth = specularDepth;
	sampleOffset = sample->AddxD(structure, maxDepth + 1);
}

SWCSpectrum DistributedPath::LiInternal(const Scene *scene,
		const RayDifferential &ray, const Sample *sample,
		float *alpha, int rayDepth, bool includeEmit, bool caustic) const {
	Intersection isect;
	SWCSpectrum L(0.);
	if (alpha) *alpha = 1.;

	if (scene->Intersect(ray, &isect)) {
		// Dade - collect samples
		float *sampleData = sample->sampler->GetLazyValues(const_cast<Sample *>(sample), sampleOffset, rayDepth);
		float *lightSample, *lightNum, *bsdfSample, *bsdfComponent;
		lightSample = &sampleData[12];
		lightNum = &sampleData[14];
		bsdfSample = &sampleData[15];
		bsdfComponent = &sampleData[17];

		// Evaluate BSDF at hit point
		BSDF *bsdf = isect.GetBSDF(ray, fabsf(2.f * bsdfComponent[0] - 1.f));
		Vector wo = -ray.d;
		const Point &p = bsdf->dgShading.p;
		const Normal &n = bsdf->dgShading.nn;

		// Compute emitted light if ray hit an area light source
		if(includeEmit)
			L += isect.Le(wo);

		// Compute direct lighting for _DistributedPath_ integrator
		if (scene->lights.size() > 0) {
			// Apply direct lighting strategy
			switch (lightStrategy) {
				case SAMPLE_ALL_UNIFORM:
					L += UniformSampleAllLights(scene, p, n,
							wo, bsdf, sample,
							lightSample, lightNum, bsdfSample, bsdfComponent);
					break;
				case SAMPLE_ONE_UNIFORM:
					L += UniformSampleOneLight(scene, p, n,
							wo, bsdf, sample,
							lightSample, lightNum, bsdfSample, bsdfComponent);
					break;
				default:
					break;
			}
		}

		BxDFType flags;
		float pdf;
		Vector wi;
		SWCSpectrum f;

		// trace Diffuse reflection & transmission rays
		if (rayDepth < diffuseDepth) {
			f = bsdf->Sample_f(wo, &wi,
				sampleData[0], sampleData[1], sampleData[2],
				&pdf, BxDFType(BSDF_REFLECTION | BSDF_DIFFUSE), &flags);
			if (pdf != .0f && !f.Black()) {
				RayDifferential rd(p, wi);
				L += LiInternal(scene, rd, sample, alpha, rayDepth + 1, false, false) * f * AbsDot(wi, n) / pdf;
			}
			f = bsdf->Sample_f(wo, &wi,
				sampleData[3], sampleData[4], sampleData[5],
				&pdf, BxDFType(BSDF_TRANSMISSION | BSDF_DIFFUSE), &flags);
			if (pdf != .0f && !f.Black()) {
				RayDifferential rd(p, wi);
				L += LiInternal(scene, rd, sample, alpha, rayDepth + 1, false, false) * f * AbsDot(wi, n) / pdf;
			}
		}

		// trace Glossy reflection & transmission rays
		if (rayDepth < glossyDepth) {
			f = bsdf->Sample_f(wo, &wi,
				sampleData[6], sampleData[7], sampleData[8],
				&pdf, BxDFType(BSDF_REFLECTION | BSDF_GLOSSY), &flags);
			if (pdf != .0f && !f.Black()) {
				RayDifferential rd(p, wi);
				L += LiInternal(scene, rd, sample, alpha, rayDepth + 1, false, true) * f * AbsDot(wi, n) / pdf;
			}
			f = bsdf->Sample_f(wo, &wi,
				sampleData[9], sampleData[10], sampleData[11],
				&pdf, BxDFType(BSDF_TRANSMISSION | BSDF_GLOSSY), &flags);
			if (pdf != .0f && !f.Black()) {
				RayDifferential rd(p, wi);
				L += LiInternal(scene, rd, sample, alpha, rayDepth + 1, false, true) * f * AbsDot(wi, n) / pdf;
			}
		} 
		
		// trace specular reflection & transmission rays
		if (rayDepth < specularDepth) {
			f = bsdf->Sample_f(wo, &wi,
				BxDFType(BSDF_REFLECTION | BSDF_SPECULAR));
			if (!f.Black()) {
				RayDifferential rd(p, wi);
				L += LiInternal(scene, rd, sample, alpha, rayDepth + 1, true, true) * f * AbsDot(wi, n);
			}
			f = bsdf->Sample_f(wo, &wi,
				BxDFType(BSDF_TRANSMISSION | BSDF_SPECULAR));
			if (!f.Black()) {
				RayDifferential rd(p, wi);
				L += LiInternal(scene, rd, sample, alpha, rayDepth + 1, true, true) * f * AbsDot(wi, n);
			}
		} 

	} else {
		// Handle ray with no intersection
		for (u_int i = 0; i < scene->lights.size(); ++i)
			L += scene->lights[i]->Le(ray);
		if (alpha && L.Black()) *alpha = 0.;
	}

	return L * scene->volumeIntegrator->Transmittance(scene, ray, sample, alpha) + scene->volumeIntegrator->Li(scene, ray, sample, alpha);
}

SWCSpectrum DistributedPath::Li(const Scene *scene,
		const RayDifferential &ray, const Sample *sample,
		float *alpha) const {
	SampleGuard guard(sample->sampler, sample);

	sample->AddContribution(sample->imageX, sample->imageY,
		LiInternal(scene, ray, sample, alpha, 0, true, true).ToXYZ(),
		alpha ? *alpha : 1.f);

	return SWCSpectrum(-1.f);
}

SurfaceIntegrator* DistributedPath::CreateSurfaceIntegrator(const ParamSet &params) {
	int diffusedepth = params.FindOneInt("diffusedepth", 3);
	int glossydepth = params.FindOneInt("glossydepth", 2);
	int speculardepth = params.FindOneInt("speculardepth", 5);

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

	return new DistributedPath(estrategy, diffusedepth, glossydepth, speculardepth);
}
