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

// directlighting.cpp*
#include "directlighting.h"
#include "bxdf.h"
#include "paramset.h"

using namespace lux;

// DirectLighting Method Definitions
DirectLighting::DirectLighting(LightStrategy st, int md) {
	maxDepth = md;
	lightStrategy = st;
}

void DirectLighting::RequestSamples(Sample *sample, const Scene *scene) {
	if (lightStrategy == SAMPLE_AUTOMATIC) {
		if (scene->lights.size() > 5)
			lightStrategy = SAMPLE_ONE_UNIFORM;
		else
			lightStrategy = SAMPLE_ALL_UNIFORM;
	}

	vector<u_int> structure;
	if (lightStrategy == SAMPLE_ALL_UNIFORM) {
		// Dade - allocate and request samples for sampling all lights
		structure.push_back(2);	// light position sample
		structure.push_back(2);	// bsdf direction sample for light
		structure.push_back(1);	// bsdf component sample for light
	} else {
		// Dade - allocate and request samples for sampling one light
		structure.push_back(2);	// light position sample
		structure.push_back(1);	// light number sample
		structure.push_back(2);	// bsdf direction sample for light
		structure.push_back(1);	// bsdf component sample for light
	}

	sampleOffset = sample->AddxD(structure, maxDepth + 1);
}

SWCSpectrum DirectLighting::LiInternal(const Scene *scene,
		const RayDifferential &ray, const Sample *sample,
		float *alpha, int rayDepth) const {
	Intersection isect;
	SWCSpectrum L(0.);
	if (alpha) *alpha = 1.;

	if (scene->Intersect(ray, &isect)) {
		// Dade - collect samples
		float *sampleData = sample->sampler->GetLazyValues(const_cast<Sample *>(sample), sampleOffset, rayDepth);
		float *lightSample, *lightNum, *bsdfSample, *bsdfComponent;
		if (lightStrategy == SAMPLE_ALL_UNIFORM) {
			lightSample = &sampleData[0];
			lightNum = NULL;
			bsdfSample = &sampleData[2];
			bsdfComponent = &sampleData[4];
		} else {
			lightSample = &sampleData[0];
			lightNum = &sampleData[2];
			bsdfSample = &sampleData[3];
			bsdfComponent = &sampleData[5];
		}

		// Evaluate BSDF at hit point
		BSDF *bsdf = isect.GetBSDF(ray, fabsf(2.f * bsdfComponent[0] - 1.f));
		Vector wo = -ray.d;
		const Point &p = bsdf->dgShading.p;
		const Normal &n = bsdf->dgShading.nn;

		// Compute emitted light if ray hit an area light source
		L += isect.Le(wo);

		// Compute direct lighting for _DirectLighting_ integrator
		if (scene->lights.size() > 0) {
			// Apply direct lighting strategy
			switch (lightStrategy) {
				case SAMPLE_ALL_UNIFORM:
					L += UniformSampleAllLights(scene, p, n,
							wo, bsdf, sample,
							lightSample, bsdfSample, bsdfComponent);
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

		if (rayDepth < maxDepth) {
			Vector wi;
			// Trace rays for specular reflection and refraction
			SWCSpectrum f = bsdf->Sample_f(wo, &wi,
				BxDFType(BSDF_REFLECTION | BSDF_SPECULAR));
			if (!f.Black()) {
				// Compute ray differential _rd_ for specular reflection
				RayDifferential rd(p, wi);
				rd.hasDifferentials = true;
				rd.rx.o = p + isect.dg.dpdx;
				rd.ry.o = p + isect.dg.dpdy;
				// Compute differential reflected directions
				Normal dndx = bsdf->dgShading.dndu * bsdf->dgShading.dudx +
					bsdf->dgShading.dndv * bsdf->dgShading.dvdx;
				Normal dndy = bsdf->dgShading.dndu * bsdf->dgShading.dudy +
					bsdf->dgShading.dndv * bsdf->dgShading.dvdy;
				Vector dwodx = -ray.rx.d - wo, dwody = -ray.ry.d - wo;
				float dDNdx = Dot(dwodx, n) + Dot(wo, dndx);
				float dDNdy = Dot(dwody, n) + Dot(wo, dndy);
				rd.rx.d = wi -
				          dwodx + 2 * Vector(Dot(wo, n) * dndx +
						  dDNdx * n);
				rd.ry.d = wi -
				          dwody + 2 * Vector(Dot(wo, n) * dndy +
						  dDNdy * n);
				//L += scene->Li(rd, sample) * f * AbsDot(wi, n);
				L += LiInternal(scene, rd, sample, alpha, rayDepth + 1) * f * AbsDot(wi, n);
			}

			f = bsdf->Sample_f(wo, &wi,
				BxDFType(BSDF_TRANSMISSION | BSDF_SPECULAR));
			if (!f.Black()) {
				// Compute ray differential _rd_ for specular transmission
				RayDifferential rd(p, wi);
				rd.hasDifferentials = true;
				rd.rx.o = p + isect.dg.dpdx;
				rd.ry.o = p + isect.dg.dpdy;
				
				float eta = bsdf->eta;
				Vector w = -wo;
				if (Dot(wo, n) < 0) eta = 1.f / eta;
				
				Normal dndx = bsdf->dgShading.dndu * bsdf->dgShading.dudx + bsdf->dgShading.dndv * bsdf->dgShading.dvdx;
				Normal dndy = bsdf->dgShading.dndu * bsdf->dgShading.dudy + bsdf->dgShading.dndv * bsdf->dgShading.dvdy;
				
				Vector dwodx = -ray.rx.d - wo, dwody = -ray.ry.d - wo;
				float dDNdx = Dot(dwodx, n) + Dot(wo, dndx);
				float dDNdy = Dot(dwody, n) + Dot(wo, dndy);
				
				float mu = eta * Dot(w, n) - Dot(wi, n);
				float dmudx = (eta - (eta*eta*Dot(w,n))/Dot(wi, n)) * dDNdx;
				float dmudy = (eta - (eta*eta*Dot(w,n))/Dot(wi, n)) * dDNdy;
				
				rd.rx.d = wi + eta * dwodx - Vector(mu * dndx + dmudx * n);
				rd.ry.d = wi + eta * dwody - Vector(mu * dndy + dmudy * n);
				//L += scene->Li(rd, sample) * f * AbsDot(wi, n);
				L += LiInternal(scene, rd, sample, alpha, rayDepth + 1) * f * AbsDot(wi, n);
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

SWCSpectrum DirectLighting::Li(const Scene *scene,
		const RayDifferential &ray, const Sample *sample,
		float *alpha) const {
	SampleGuard guard(sample->sampler, sample);

	sample->AddContribution(sample->imageX, sample->imageY,
		LiInternal(scene, ray, sample, alpha, 0).ToXYZ(),
		alpha ? *alpha : 1.f);

	return SWCSpectrum(-1.f);
}

SurfaceIntegrator* DirectLighting::CreateSurfaceIntegrator(const ParamSet &params) {
	int maxDepth = params.FindOneInt("maxdepth", 5);

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

	return new DirectLighting(estrategy, maxDepth);
}
