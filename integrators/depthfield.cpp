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

// depthfield.cpp*
#include "depthfield.h"
#include "bxdf.h"
#include "camera.h"
#include "film.h"
#include "sampling.h"
#include "color.h"
#include "paramset.h"
#include "dynload.h"

using namespace lux;

// DirectLightingIntegrator Method Definitions
DepthfieldIntegrator::DepthfieldIntegrator(u_int md) : SurfaceIntegrator() {
	maxDepth = md;
	AddStringConstant(*this, "name", "Name of current surface integrator", "depthfield");	
}

void DepthfieldIntegrator::RequestSamples(Sampler *sampler, const Scene &scene) {
	// Allocate and request samples for light sampling
	hints.RequestSamples(sampler, scene, maxDepth + 1);
	vector<u_int> structure;
	structure.push_back(1);	//scattering
	scatterOffset = sampler->AddxD(structure, maxDepth + 1);
}

void DepthfieldIntegrator::Preprocess(const RandomGenerator &rng,
	const Scene &scene)
{
	// Prepare image buffers
	BufferType type = BUF_TYPE_PER_PIXEL;
	scene.sampler->GetBufferType(&type);
	bufferId = scene.camera->film->RequestBuffer(type, BUF_FRAMEBUFFER, "eye");

	hints.InitStrategies(scene);
}

u_int DepthfieldIntegrator::LiInternal(const Scene &scene,
	const Sample &sample, const Volume *volume, bool scattered,
	const Ray &ray, vector<SWCSpectrum> &L, float *alpha, float &distance,
	u_int rayDepth, bool from_IsSup, bool path_type) const
{
	u_int nContribs = 0;
	Intersection isect;
	BSDF *bsdf;
	const float time = ray.time; // save time for motion blur
	const float nLights = scene.lights.size();
	const SpectrumWavelengths &sw(sample.swl);
	SWCSpectrum Lt(1.f);
	bool surf_IsSup = false;
	bool path_t = path_type;

	const float *data = sample.sampler->GetLazyValues(sample,scatterOffset,
							  rayDepth);
	float spdf;
	if (scene.Intersect(sample, volume, scattered, ray, data[0], &isect,
			    &bsdf, &spdf, NULL, &Lt)) {

		if (rayDepth == 0)
			distance = ray.maxt * ray.d.Length();

		// Evaluate BSDF at hit point
		Vector wo = -ray.d;

		// Compute direct lighting
		const Point &p = bsdf->dgShading.p;
		const Normal &n = bsdf->dgShading.nn;
		if (isect.primitive) {

			//surf_IsSup = (isect.primitive)->IsSupport();
			//if ( !surf_IsSup ) path_t = true;
			// Compute direct lighting for suport materials
			if (nLights > 0 ) {
				const u_int lightGroupCount = scene.lightGroups.size();
				vector<SWCSpectrum> Ld(lightGroupCount, 0.f);
				nContribs += hints.EnvSampleLights(scene, sample, p, n, wo,
								     bsdf, rayDepth, 1.f, Ld, rayDepth, from_IsSup, surf_IsSup, path_t );

				for (u_int i = 0; i < lightGroupCount; ++i)
					L[i] += Ld[i];
			}
		}
		//		if (rayDepth < maxDepth) {
		//			Vector wi;
		//			// Trace rays for specular reflection and refraction
		//			float pdf;
		//			SWCSpectrum f;
		//			if (bsdf->SampleF(sw, wo, &wi, .5f, .5f, .5f, &f, &pdf,
		//				BxDFType(BSDF_REFLECTION | BSDF_SPECULAR), NULL,
		//				NULL, true)) {
		//				// Compute ray differential _rd_ for specular reflection
		//				Ray rd(p, wi);
		//				rd.time = time;
		//				vector<SWCSpectrum> Lr(scene.lightGroups.size(),
		//					SWCSpectrum(0.f));
		//				u_int nc = LiInternal(scene, sample,
		//					bsdf->GetVolume(wi),
		//					bsdf->dgShading.scattered,
		//					rd, Lr, alpha, distance, rayDepth + 1, surf_IsSup, path_t);
		//				if (nc > 0) {
		//					for (u_int i = 0; i < L.size(); ++i)
		//						L[i] += Lr[i] * f;
		//					nContribs += nc;
		//				}
		//			}

		//			if (bsdf->SampleF(sw, wo, &wi, .5f, .5f, .5f, &f, &pdf,
		//				BxDFType(BSDF_TRANSMISSION | BSDF_SPECULAR),
		//				NULL, NULL, true)) {
		//				// Compute ray differential _rd_ for specular transmission
		//				Ray rd(p, wi);
		//				rd.time = time;
		//				vector<SWCSpectrum> Lr(scene.lightGroups.size(),
		//					SWCSpectrum(0.f));
		//				u_int nc = LiInternal(scene, sample,
		//					bsdf->GetVolume(wi),
		//					bsdf->dgShading.scattered, rd, Lr,
		//					alpha, distance, rayDepth + 1, surf_IsSup, path_t);
		//				if (nc > 0) {
		//					for (u_int i = 0; i < L.size(); ++i)
		//						L[i] += Lr[i] * f;
		//					nContribs += nc;
		//				}
		//			}
		//		}
	}
//	if(!surf_IsSup) {
//		BSDF *ibsdf;
//		for (u_int i = 0; i < nLights; ++i) {
//			SWCSpectrum Le(1.f);
//			if (scene.lights[i]->Le(scene, sample, ray, &ibsdf,
//						NULL, NULL, &Le)) {
//				L[scene.lights[i]->group] += Le;// SWCSpectrum(*alpha);//Le
//				++nContribs;
//			}
//		}

//	}
	Lt /= spdf;

	if (nContribs > 0) {
		for (u_int i = 0; i < L.size(); ++i)
			L[i] *= Lt;
	}
	SWCSpectrum VLi(0.f);
	u_int g = scene.volumeIntegrator->Li(scene, ray, sample, &VLi, alpha, from_IsSup, path_t);
	if (!VLi.Black()) {
		L[g] += VLi;
		//L[g] += SWCSpectrum(*alpha);
		++nContribs;
	}

	return nContribs;
}

u_int DepthfieldIntegrator::Li(const Scene &scene,
	const Sample &sample) const
{
	Ray ray;
	float xi, yi;
	float rayWeight = sample.camera->GenerateRay(scene, sample, &ray, &xi, &yi);

	vector<SWCSpectrum> L(scene.lightGroups.size(), SWCSpectrum(0.f));
	float alpha = 1.f;
	float distance;
	u_int nContribs = LiInternal(scene, sample, NULL, false, ray, L, &alpha,
		distance, 0, false, false);

	for (u_int i = 0; i < scene.lightGroups.size(); ++i)
	sample.AddContribution(xi, yi,
			XYZColor(sample.swl, L[i]) , XYZColor(sample.swl, L[i]).Y(),
			distance, 0.f, bufferId, i);

	return nContribs;
}

SurfaceIntegrator* DepthfieldIntegrator::CreateSurfaceIntegrator(const ParamSet &params) {
	int maxDepth = params.FindOneInt("maxdepth", 1);

	DepthfieldIntegrator *dli = new DepthfieldIntegrator(max(maxDepth, 0));
	// Initialize the rendering hints
	dli->hints.InitParam(params);

	return dli;
}

static DynamicLoader::RegisterSurfaceIntegrator<DepthfieldIntegrator> r("depthfield");
