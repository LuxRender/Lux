/***************************************************************************
 *   Aldo Zang & Luiz Velho						   *
 *   Augmented Reality Project : http://w3.impa.br/~zang/arlux/ 	   *
 *   Visgraf-Impa Lab 2010       http://www.visgraf.impa.br/ 		   *
 *									   *
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
#include "ardirectlighting.h"
#include "bxdf.h"
#include "camera.h"
#include "film.h"
#include "sampling.h"
#include "color.h"
#include "paramset.h"
#include "dynload.h"

using namespace lux;

// DirectLightingIntegrator Method Definitions
ARDirectLightingIntegrator::ARDirectLightingIntegrator(u_int md) {
	maxDepth = md;
}

void ARDirectLightingIntegrator::RequestSamples(Sample *sample, const Scene *scene) {
	// Allocate and request samples for light sampling
	hints.RequestSamples(sample, scene, maxDepth + 1);
}

void ARDirectLightingIntegrator::Preprocess(const TsPack *tspack, const Scene *scene)
{
	// Prepare image buffers
	BufferType type = BUF_TYPE_PER_PIXEL;
	scene->sampler->GetBufferType(&type);
	bufferId = scene->camera->film->RequestBuffer(type, BUF_FRAMEBUFFER, "eye");

	hints.InitStrategies(scene);
}

u_int ARDirectLightingIntegrator::LiInternal(const TsPack *tspack,
	const Scene *scene, const Volume *volume, const RayDifferential &ray,
	const Sample *sample, vector<SWCSpectrum> &L, float *alpha,
	float &distance, u_int rayDepth, bool from_IsSup, bool path_type) const
{
	u_int nContribs = 0;
	Intersection isect;
	BSDF *bsdf;
	const float time = ray.time; // save time for motion blur
	const float nLights = scene->lights.size();
	SWCSpectrum Lt(1.f);
	bool surf_IsSup = false;
        bool path_t = path_type;
	if (scene->Intersect(tspack, volume, ray, &isect, &bsdf, &Lt)) {


		if (rayDepth == 0)
			distance = ray.maxt * ray.d.Length();

		// Evaluate BSDF at hit point
		Vector wo = -ray.d;
		const Point &p = bsdf->dgShading.p;
		const Normal &n = bsdf->dgShading.nn;

		// Compute emitted light if ray hit an area light source
		if (isect.arealight) {
			L[isect.arealight->group] += isect.Le(tspack, wo);
			++nContribs;
		}

		if (isect.primitive) { 

			surf_IsSup = (isect.primitive)->IsSupport();
			if ( !surf_IsSup ) path_t = true; 
			// Compute direct lighting for suport materials
			if (nLights > 0) {

				const u_int lightGroupCount = scene->lightGroups.size();
				vector<SWCSpectrum> Ld(lightGroupCount, 0.f);
				//if (from_IsSup && rayDepth > 0)
				nContribs += hints.SampleLights(tspack, scene, p, n, wo, bsdf,
						sample, rayDepth, 1.f, Ld, rayDepth, from_IsSup, surf_IsSup, path_t );

				for (u_int i = 0; i < lightGroupCount; ++i)
					L[i] += Ld[i];
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
				bsdf->ComputeReflectionDifferentials(ray, rd);
				vector<SWCSpectrum> Lr(scene->lightGroups.size(), SWCSpectrum(0.f));
				u_int nc = LiInternal(tspack, scene, bsdf->GetVolume(wi), rd, sample, Lr, alpha, distance, rayDepth + 1, surf_IsSup, path_t);
				if (nc > 0) {
					SWCSpectrum filter(f *
						(AbsDot(wi, n) / pdf));
					for (u_int i = 0; i < L.size(); ++i)
						L[i] += Lr[i] * filter;
					nContribs += nc;
				}
			}

			if (bsdf->Sample_f(tspack, wo, &wi, .5f, .5f, .5f, &f, &pdf, BxDFType(BSDF_TRANSMISSION | BSDF_SPECULAR), NULL, NULL, true)) {
				// Compute ray differential _rd_ for specular transmission
				RayDifferential rd(p, wi);
				rd.time = time;
				bsdf->ComputeTransmissionDifferentials(tspack, ray, rd);
				vector<SWCSpectrum> Lr(scene->lightGroups.size(), SWCSpectrum(0.f));
				u_int nc = LiInternal(tspack, scene, bsdf->GetVolume(wi), rd, sample, Lr, alpha, distance, rayDepth + 1, surf_IsSup, path_t);
				if (nc > 0) {
					SWCSpectrum filter(f *
						(AbsDot(wi, n) / pdf));
					for (u_int i = 0; i < L.size(); ++i)
						L[i] += Lr[i] * filter;
					nContribs += nc;
				}
			}
		}


	} else {
		if(path_type || rayDepth == 0){
			// Handle ray with no intersection
			for (u_int i = 0; i < nLights; ++i) {
				SWCSpectrum Le(scene->lights[i]->Le(tspack, ray));
				if (!Le.Black()) {
					L[scene->lights[i]->group] += Le;
					++nContribs;
				}
			}
		}
		if (rayDepth == 0) {
			*alpha = 0.f;
			distance = INFINITY;
		}
		
	}

	if (nContribs > 0) {
		scene->volumeIntegrator->Transmittance(tspack, scene, ray, sample, alpha, &Lt);
		for (u_int i = 0; i < L.size(); ++i)
			L[i] *= Lt;
	}
	SWCSpectrum VLi(0.f);
	u_int g = scene->volumeIntegrator->Li(tspack, scene, ray, sample, &VLi, alpha, from_IsSup, path_t); 
	if (!VLi.Black()) {
		L[g] += VLi;
		++nContribs;
	}

	return nContribs;
}

u_int ARDirectLightingIntegrator::Li(const TsPack *tspack, const Scene *scene,
	const Sample *sample) const
{
        RayDifferential ray;
        float rayWeight = tspack->camera->GenerateRay(tspack, scene, *sample,
		&ray);

	vector<SWCSpectrum> L(scene->lightGroups.size(), SWCSpectrum(0.f));
	float alpha = 1.f;
	float distance;
	u_int nContribs = LiInternal(tspack, scene, NULL, ray, sample, L, &alpha, distance, 0, false, false);

	for (u_int i = 0; i < scene->lightGroups.size(); ++i)
		sample->AddContribution(sample->imageX, sample->imageY,
			XYZColor(tspack, L[i]) * rayWeight, alpha, distance,
			0.f, bufferId, i);

	return nContribs;
}

SurfaceIntegrator* ARDirectLightingIntegrator::CreateSurfaceIntegrator(const ParamSet &params) {
	int maxDepth = params.FindOneInt("maxdepth", 5);

	ARDirectLightingIntegrator *dli = new ARDirectLightingIntegrator(max(maxDepth, 0));
	// Initialize the rendering hints
	dli->hints.InitParam(params);

	return dli;
}

static DynamicLoader::RegisterSurfaceIntegrator<ARDirectLightingIntegrator> r("ardirectlighting");
