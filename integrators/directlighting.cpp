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
DirectLightingIntegrator::DirectLightingIntegrator(u_int md) {
	maxDepth = md;

	// DirectLighting doesn't use RussianRouletteStrategies at all so none of them
	// is supported
	hints.GetSupportedStrategies().RemoveAllRussianRouletteStrategy();
}

void DirectLightingIntegrator::RequestSamples(Sample *sample, const Scene *scene) {
	vector<u_int> structure;
	// Allocate and request samples for light sampling
	hints.RequestSamples(structure);

	sampleOffset = sample->AddxD(structure, maxDepth + 1);
}

void DirectLightingIntegrator::Preprocess(const TsPack *tspack, const Scene *scene)
{
	// Prepare image buffers
	BufferType type = BUF_TYPE_PER_PIXEL;
	scene->sampler->GetBufferType(&type);
	bufferId = scene->camera->film->RequestBuffer(type, BUF_FRAMEBUFFER, "eye");

	hints.InitStrategies(scene);
}

u_int DirectLightingIntegrator::LiInternal(const TsPack *tspack, const Scene *scene,
		const RayDifferential &ray, const Sample *sample,
		vector<SWCSpectrum> &L, float *alpha, float &distance, u_int rayDepth) const {
	u_int nContribs = 0;
	Intersection isect;
	const float time = ray.time; // save time for motion blur
	const float nLights = scene->lights.size();

	if (scene->Intersect(ray, &isect)) {
		if (rayDepth == 0)
			distance = ray.maxt * ray.d.Length();

		// Dade - collect samples
		const float *sampleData = sample->sampler->GetLazyValues(const_cast<Sample *>(sample), sampleOffset, rayDepth);

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
		if (nLights > 0) {
			const u_int lightGroupCount = scene->lightGroups.size();
			vector<SWCSpectrum> Ld(lightGroupCount, 0.f);
			nContribs += hints.SampleLights(tspack, scene, p, n, wo, bsdf,
					sample, sampleData, 1.f, Ld);

			for (u_int i = 0; i < lightGroupCount; ++i)
				L[i] += Ld[i];
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
				u_int nc = LiInternal(tspack, scene, rd, sample, Lr, alpha, distance, rayDepth + 1);
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
				u_int nc = LiInternal(tspack, scene, rd, sample, Lt, alpha, distance, rayDepth + 1);
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
		for (u_int i = 0; i < nLights; ++i) {
			SWCSpectrum Le(scene->lights[i]->Le(tspack, ray));
			if (!Le.Black()) {
				L[scene->lights[i]->group] += Le;
				++nContribs;
			}
		}
		if (rayDepth == 0) {
			*alpha = 0.f;
			distance = INFINITY;
		}
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
	float distance;
	u_int nContribs = LiInternal(tspack, scene, ray,sample, L, &alpha, distance, 0);

	for (u_int i = 0; i < scene->lightGroups.size(); ++i)
		sample->AddContribution(sample->imageX, sample->imageY,
			XYZColor(tspack, L[i]) * rayWeight, alpha, distance,
			0.f, bufferId, i);

	return nContribs;
}

SurfaceIntegrator* DirectLightingIntegrator::CreateSurfaceIntegrator(const ParamSet &params) {
	int maxDepth = params.FindOneInt("maxdepth", 5);

	DirectLightingIntegrator *dli = new DirectLightingIntegrator(max(maxDepth, 0));
	// Initialize the rendering hints
	dli->hints.InitParam(params);

	return dli;
}

static DynamicLoader::RegisterSurfaceIntegrator<DirectLightingIntegrator> r("directlighting");
