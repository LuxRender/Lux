/***************************************************************************
 *   Copyright (C) 1998-2009 by authors (see AUTHORS.txt )                 *
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
#include "camera.h"
#include "paramset.h"
#include "dynload.h"

using namespace lux;

// DirectLightingIntegrator Method Definitions
DirectLightingIntegrator::DirectLightingIntegrator(LightStrategy st, u_int md) {
	maxDepth = md;
	lightStrategy = st;
}

void DirectLightingIntegrator::RequestSamples(Sample *sample, const Scene *scene) {
	if (lightStrategy == SAMPLE_AUTOMATIC) {
		if (scene->lights.size() > 5)
			lightStrategy = SAMPLE_ONE_UNIFORM;
		else
			lightStrategy = SAMPLE_ALL_UNIFORM;
	}

	vector<u_int> structure;
	// Dade - allocate and request samples for light sampling
	structure.push_back(2);	// light position sample
	structure.push_back(1);	// light number sample
	structure.push_back(2);	// bsdf direction sample for light
	structure.push_back(1);	// bsdf component sample for light

	sampleOffset = sample->AddxD(structure, maxDepth + 1);
}

void DirectLightingIntegrator::Preprocess(const TsPack *tspack, const Scene *scene)
{
	// Prepare image buffers
	BufferType type = BUF_TYPE_PER_PIXEL;
	scene->sampler->GetBufferType(&type);
	bufferId = scene->camera->film->RequestBuffer(type, BUF_FRAMEBUFFER, "eye");
}

u_int DirectLightingIntegrator::LiInternal(const TsPack *tspack, const Scene *scene,
		const RayDifferential &ray, const Sample *sample,
		vector<SWCSpectrum> &L, float *alpha, u_int rayDepth) const {
	u_int nContribs = 0;
	Intersection isect;
	const float time = ray.time; // save time for motion blur

	if (scene->Intersect(ray, &isect)) {
		// Dade - collect samples
		float *sampleData = sample->sampler->GetLazyValues(const_cast<Sample *>(sample), sampleOffset, rayDepth);
		float *lightSample = &sampleData[0];
		float *lightNum = &sampleData[2];
		float *bsdfSample = &sampleData[3];
		float *bsdfComponent = &sampleData[5];

		// Evaluate BSDF at hit point
		BSDF *bsdf = isect.GetBSDF(tspack, ray);
		Vector wo = -ray.d;
		const Point &p = bsdf->dgShading.p;
		const Normal &n = bsdf->dgShading.nn;

		// Compute emitted light if ray hit an area light source
		if (isect.arealight) {
			L[isect.arealight->group] += isect.Le(tspack, wo);
			++nContribs;
		}

		// Compute direct lighting
		if (scene->lights.size() > 0) {
			// Apply direct lighting strategy
			SWCSpectrum Ll;
			switch (lightStrategy) {
				case SAMPLE_ALL_UNIFORM:
				{
					const u_int nLights = scene->lights.size();
					const float lIncrement = 1.f / nLights;
					float l = *lightNum * lIncrement;
					for (u_int i = 0; i < nLights; ++i, l += lIncrement) {
						u_int g = UniformSampleOneLight(tspack, scene, p, n,
							wo, bsdf, sample,
							lightSample, &l, bsdfSample, bsdfComponent, &Ll);
						if (!Ll.Black()) {
							Ll *= lIncrement;
							L[g] += Ll;
							++nContribs;
						}
					}
					break;
				}
				case SAMPLE_ONE_UNIFORM:
				{
					u_int g = UniformSampleOneLight(tspack, scene, p, n,
						wo, bsdf, sample,
						lightSample, lightNum, bsdfSample, bsdfComponent, &Ll);
					if (!Ll.Black()) {
						L[g] += Ll;
						++nContribs;
					}
					break;
				}
				default:
					break;
			}
		}

		if (rayDepth < maxDepth) {
			Vector wi;
			// Trace rays for specular reflection and refraction
			float pdf;
			SWCSpectrum f;
			if (bsdf->Sample_f(tspack, wo, &wi, .5f, .5f, .5f, &f, &pdf, BxDFType(BSDF_REFLECTION | BSDF_SPECULAR), NULL, NULL, true)) {
				// Compute ray differential _rd_ for specular reflection
				RayDifferential rd(p, wi);
				rd.time = time;
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
				rd.rx.d = wi - dwodx +
					2 * Vector(Dot(wo, n) * dndx + dDNdx * n);
				rd.ry.d = wi - dwody +
					2 * Vector(Dot(wo, n) * dndy + dDNdy * n);
				vector<SWCSpectrum> Lr(scene->lightGroups.size(), SWCSpectrum(0.f));
				u_int nc = LiInternal(tspack, scene, rd, sample, Lr, alpha, rayDepth + 1);
				if (nc > 0) {
					SWCSpectrum filter(f * AbsDot(wi, n));
					for (u_int i = 0; i < L.size(); ++i)
						L[i] += Lr[i] * filter;
					nContribs += nc;
				}
			}

			if (bsdf->Sample_f(tspack, wo, &wi, .5f, .5f, .5f, &f, &pdf, BxDFType(BSDF_TRANSMISSION | BSDF_SPECULAR), NULL, NULL, true)) {
				// Compute ray differential _rd_ for specular transmission
				RayDifferential rd(p, wi);
				rd.time = time;
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
				vector<SWCSpectrum> Lt(scene->lightGroups.size(), SWCSpectrum(0.f));
				u_int nc = LiInternal(tspack, scene, rd, sample, Lt, alpha, rayDepth + 1);
				if (nc > 0) {
					SWCSpectrum filter(f * AbsDot(wi, n));
					for (u_int i = 0; i < L.size(); ++i)
						L[i] += Lt[i] * filter;
					nContribs += nc;
				}
			}
		}
	} else {
		// Handle ray with no intersection
		for (u_int i = 0; i < scene->lights.size(); ++i) {
			SWCSpectrum Le(scene->lights[i]->Le(tspack, ray));
			if (!Le.Black()) {
				L[scene->lights[i]->group] += Le;
				++nContribs;
			}
		}
		if (rayDepth == 0)
			*alpha = 0.f;
	}

	if (nContribs > 0) {
		SWCSpectrum Lt(1.f);
		scene->volumeIntegrator->Transmittance(tspack, scene, ray, sample, alpha, &Lt);
		for (u_int i = 0; i < L.size(); ++i)
			L[i] *= Lt;
	}
	SWCSpectrum VLi(0.f);
	u_int g = scene->volumeIntegrator->Li(tspack, scene, ray, sample, &VLi, alpha);
	if (!VLi.Black()) {
		L[g] += VLi;
		++nContribs;
	}

	return nContribs;
}

u_int DirectLightingIntegrator::Li(const TsPack *tspack, const Scene *scene,
	const Sample *sample) const
{
        RayDifferential ray;
        float rayWeight = tspack->camera->GenerateRay(*sample, &ray);
	if (rayWeight > 0.f) {
		// Generate ray differentials for camera ray
		++(sample->imageX);
		float wt1 = tspack->camera->GenerateRay(*sample, &ray.rx);
		--(sample->imageX);
		++(sample->imageY);
		float wt2 = tspack->camera->GenerateRay(*sample, &ray.ry);
		ray.hasDifferentials = (wt1 > 0.f) && (wt2 > 0.f);
		--(sample->imageY);
	}

	vector<SWCSpectrum> L(scene->lightGroups.size(), SWCSpectrum(0.f));
	float alpha = 1.f;
	u_int nContribs = LiInternal(tspack, scene, ray,sample, L, &alpha, 0);
	for (u_int i = 0; i < L.size(); ++i)
		sample->AddContribution(sample->imageX, sample->imageY,
			L[i].ToXYZ(tspack) * rayWeight, alpha, 0.f, bufferId, i);

	return nContribs;
}

SurfaceIntegrator* DirectLightingIntegrator::CreateSurfaceIntegrator(const ParamSet &params) {
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

	return new DirectLightingIntegrator(estrategy, max(maxDepth, 0));
}

static DynamicLoader::RegisterSurfaceIntegrator<DirectLightingIntegrator> r("directlighting");
