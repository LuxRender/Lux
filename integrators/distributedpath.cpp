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

// distributedpath.cpp*
#include "distributedpath.h"
#include "bxdf.h"
#include "material.h"
#include "camera.h"
#include "paramset.h"
#include "dynload.h"

using namespace lux;

// DistributedPath Method Definitions
DistributedPath::DistributedPath(bool da, bool dd, bool dg, bool ida, bool idd, bool idg,
	u_int drd, u_int drs, u_int dtd, u_int dts, u_int grd, u_int grs, u_int gtd, u_int gts, u_int srd, u_int std,
	bool drer, float drert, bool drfr, float drfrt,
	bool grer, float grert, bool grfr, float grfrt) {
	directAll = da;
	directDiffuse = dd;
	directGlossy = dg;
	indirectAll = ida;
	indirectDiffuse = idd;
	indirectGlossy = idg;
	diffusereflectDepth = drd;
	diffusereflectSamples = drs;
	diffuserefractDepth = dtd;
	diffuserefractSamples = dts;
	glossyreflectDepth = grd;
	glossyreflectSamples = grs;
	glossyrefractDepth = gtd;
	glossyrefractSamples = gts;
	specularreflectDepth = srd;
	specularrefractDepth = std;

	diffusereflectReject = drer;
	diffusereflectReject_thr = drert;
	diffuserefractReject = drfr;
	diffuserefractReject_thr = drfrt;
	glossyreflectReject = grer;
	glossyreflectReject_thr = grert;
	glossyrefractReject = grfr;
	glossyrefractReject_thr = grfrt;

	// Set the strategies supported by this integrator
	hints.GetSupportedStrategies().addLightSamplingStrategy(LightsSamplingStrategy::SAMPLE_ALL_UNIFORM);
	hints.GetSupportedStrategies().addLightSamplingStrategy(LightsSamplingStrategy::SAMPLE_ONE_UNIFORM);
	hints.GetSupportedStrategies().addLightSamplingStrategy(LightsSamplingStrategy::SAMPLE_AUTOMATIC);
	hints.GetSupportedStrategies().addLightSamplingStrategy(LightsSamplingStrategy::SAMPLE_ONE_IMPORTANCE);
	hints.GetSupportedStrategies().addLightSamplingStrategy(LightsSamplingStrategy::SAMPLE_ONE_POWER_IMPORTANCE);
	hints.GetSupportedStrategies().addLightSamplingStrategy(LightsSamplingStrategy::SAMPLE_ALL_POWER_IMPORTANCE);
	hints.GetSupportedStrategies().addLightSamplingStrategy(LightsSamplingStrategy::SAMPLE_ONE_LOG_POWER_IMPORTANCE);
	hints.GetSupportedStrategies().SetDefaultLightSamplingStrategy(LightsSamplingStrategy::SAMPLE_AUTOMATIC);

	// DistributedPath doesn't use RussianRouletteStrategies at all so none of them
	// is supported
	hints.GetSupportedStrategies().SetDefaultRussianRouletteStrategy(RussianRouletteStrategy::NOT_SUPPORTED);
}

void DistributedPath::RequestSamples(Sample *sample, const Scene *scene) {
	// determine maximum depth for samples
	maxDepth = diffusereflectDepth;
	maxDepth = max(maxDepth, diffuserefractDepth);
	maxDepth = max(maxDepth, glossyreflectDepth);
	maxDepth = max(maxDepth, glossyrefractDepth);
	maxDepth = max(maxDepth, specularreflectDepth);
	maxDepth = max(maxDepth, specularrefractDepth);

	// Diffuse reflection
	// eye vertex
	diffuse_reflectSampleOffset = sample->Add2D(diffusereflectSamples);
	diffuse_reflectComponentOffset = sample->Add1D(diffusereflectSamples);
	// remaining vertices
	indirectdiffuse_reflectSampleOffset = sample->Add2D(diffusereflectDepth);
	indirectdiffuse_reflectComponentOffset = sample->Add1D(diffusereflectDepth);

	// Diffuse refraction
	// eye vertex
	diffuse_refractSampleOffset = sample->Add2D(diffuserefractSamples);
	diffuse_refractComponentOffset = sample->Add1D(diffuserefractSamples);
	// remaining vertices
	indirectdiffuse_refractSampleOffset = sample->Add2D(diffuserefractDepth);
	indirectdiffuse_refractComponentOffset = sample->Add1D(diffuserefractDepth);

	// Glossy reflection
	// eye vertex
	glossy_reflectSampleOffset = sample->Add2D(glossyreflectSamples);
	glossy_reflectComponentOffset = sample->Add1D(glossyreflectSamples);
	// remaining vertices
	indirectglossy_reflectSampleOffset = sample->Add2D(glossyreflectDepth);
	indirectglossy_reflectComponentOffset = sample->Add1D(glossyreflectDepth);

	// Glossy refraction
	// eye vertex
	glossy_refractSampleOffset = sample->Add2D(glossyrefractSamples);
	glossy_refractComponentOffset = sample->Add1D(glossyrefractSamples);
	// remaining vertices
	indirectglossy_refractSampleOffset = sample->Add2D(glossyrefractDepth);
	indirectglossy_refractComponentOffset = sample->Add1D(glossyrefractDepth);

	vector<u_int> structure;
	// Allocate and request samples for light sampling, RR, etc.
	hints.RequestSamples(structure);
	sampleOffset = sample->AddxD(structure, maxDepth + 1);

}
void DistributedPath::Preprocess(const TsPack *tspack, const Scene *scene)
{
	// Prepare image buffers
	BufferType type = BUF_TYPE_PER_PIXEL;
	scene->sampler->GetBufferType(&type);
	bufferId = scene->camera->film->RequestBuffer(type, BUF_FRAMEBUFFER, "eye");

	hints.InitStrategies(scene);
}

void DistributedPath::Reject(const TsPack *tspack, vector< vector<SWCSpectrum> > &LL, 
							 vector<SWCSpectrum> &L, float rejectrange) const {
	float totallum = 0.f;
	float samples = LL.size();
	for (u_int i = 0; i < samples; ++i)
		for (u_int j = 0; j < LL[i].size(); ++j)
			totallum += LL[i][j].Y(tspack) * samples;
	float avglum = totallum / samples;

	float validlength;
	if (avglum > 0.f) {
		validlength = avglum * rejectrange;

		// reject
		u_int rejects = 0;
		vector<SWCSpectrum> Lo(L.size(), SWCSpectrum(0.f));
		for (u_int i = 0; i < samples; ++i) {
			float y = 0.f;
			for (u_int j = 0; j < LL[i].size(); ++j) {
				y += LL[i][j].Y(tspack) * samples;
			}
			if (y > avglum + validlength) {
				rejects++;
			} else {
				for (u_int j = 0; j < LL[i].size(); ++j)
					Lo[j] += LL[i][j];
			}
		}

//		float weight = samples / (samples-rejects);

		// Normalize
		for(u_int i = 0; i < L.size(); ++i)
			L[i] += Lo[i]; //* weight;
	} else {
		for(u_int i = 0; i < samples; ++i)
			for(u_int j = 0; j < LL[i].size(); ++j)
				L[j] += LL[i][j];
	}
}

void DistributedPath::LiInternal(const TsPack *tspack, const Scene *scene,
		const RayDifferential &ray, const Sample *sample,
		vector<SWCSpectrum> &L, float *alpha, float *zdepth, u_int rayDepth,
		bool includeEmit, u_int &nrContribs) const
{
	Intersection isect;
	const float time = ray.time; // save time for motion blur

	if (scene->Intersect(ray, &isect)) {
		// Evaluate BSDF at hit point
		BSDF *bsdf = isect.GetBSDF(tspack, ray);
		Vector wo = -ray.d;
		const Point &p = bsdf->dgShading.p;
		const Normal &n = bsdf->dgShading.nn;

		if (rayDepth == 0) {
			// Set Zbuf depth
			const Vector zv(p - ray.o);
			*zdepth = zv.Length();

			// Chroma Keying
//			if(bsdf->compParams->K) {
//				L[0] = SWCSpectrum(tspack, bsdf->compParams->Kc); // TODO write to separate channel
//				return;
//			}

			// Override alpha
			if(bsdf->compParams->oA)
				*alpha = bsdf->compParams->A;

			// Compute emitted light if ray hit an area light source with Visibility check
			if(bsdf->compParams->tVl && includeEmit) {
				const SWCSpectrum Le(isect.Le(tspack, wo));
				if (Le.Filter(tspack) > 0.f) {
					L[isect.arealight->group] += Le;
					++nrContribs;
				}
			}

			// Visibility check
			if(!bsdf->compParams->tVm) {
			       if (!bsdf->compParams->oA)
					*alpha = 0.f;
				return;
			}
		} else {

			// Compute emitted light if ray hit an area light source with Visibility check
			if(bsdf->compParams->tiVl && includeEmit) {
				const SWCSpectrum Le(isect.Le(tspack, wo));
				if (Le.Filter(tspack) > 0.f) {
					L[isect.arealight->group] += Le;
					++nrContribs;
				}
			}

			// Visibility check
			if(!bsdf->compParams->tiVm)
				return;
		}

		// Compute direct lighting for _DistributedPath_ integrator
		if (scene->lights.size() > 0) {
			const u_int lightGroupCount = scene->lightGroups.size();
			const float *sampleData = sample->sampler->GetLazyValues(const_cast<Sample *>(sample), sampleOffset, rayDepth);
			vector<SWCSpectrum> Ld(lightGroupCount, 0.f);
			nrContribs += hints.SampleLights(tspack, scene, p, n, wo, bsdf,
					sample, sampleData, 1.f, Ld);

			for (u_int i = 0; i < lightGroupCount; ++i)
				L[i] += Ld[i];
		}

		BxDFType flags;
		float pdf;
		Vector wi;
		SWCSpectrum f;
		u_int samples;
		float invsamples;

		// trace Diffuse reflection & transmission rays
		if (rayDepth < diffusereflectDepth) {
			if (rayDepth > 0)
				samples = 1;
			else
				samples = diffusereflectSamples;
			invsamples = 1.f / samples;

			vector< vector<SWCSpectrum> > LL;

			for (u_int i = 0; i < samples; ++i) {
				float u1, u2, u3;
				if (rayDepth > 0) {
					u1 = sample->twoD[indirectdiffuse_reflectSampleOffset][2 * i * rayDepth];
					u2 = sample->twoD[indirectdiffuse_reflectSampleOffset][(2 * i * rayDepth) + 1];
					u3 = sample->oneD[indirectdiffuse_reflectComponentOffset][i * rayDepth];
				} else {
					u1 = sample->twoD[diffuse_reflectSampleOffset][2 * i];
					u2 = sample->twoD[diffuse_reflectSampleOffset][2 * i + 1];
					u3 = sample->oneD[diffuse_reflectComponentOffset][i];
				}

				if (bsdf->Sample_f(tspack, wo, &wi, u1, u2, u3, &f, 
					 &pdf, BxDFType(BSDF_REFLECTION | BSDF_DIFFUSE), &flags, NULL, true)) {
					RayDifferential rd(p, wi);
					rd.time = time;
					vector<SWCSpectrum> Ll(L.size(), SWCSpectrum(0.f));
					LiInternal(tspack, scene, rd, sample, Ll, alpha, zdepth, rayDepth + 1, false, nrContribs);
					f *= invsamples * AbsDot(wi, n) / pdf;
					if (diffusereflectReject && (rayDepth == 0 || includeEmit)) {
						for (u_int j = 0; j < Ll.size(); ++j)
							Ll[j] *= f;
						LL.push_back(Ll);
					} else {
						for (u_int j = 0; j < L.size(); ++j)
							L[j] += f * Ll[j];
					}
				}
			}

			if (rayDepth == 0)
				Reject(tspack, LL, L, diffusereflectReject_thr);
		}
		if (rayDepth < diffuserefractDepth) {
			if (rayDepth > 0)
				samples = 1;
			else
				samples = diffuserefractSamples;
			invsamples = 1.f / samples;

			vector< vector<SWCSpectrum> > LL;

			for (u_int i = 0; i < samples; ++i) {
				float u1, u2, u3;
				if (rayDepth > 0) {
					u1 = sample->twoD[indirectdiffuse_refractSampleOffset][2 * i * rayDepth];
					u2 = sample->twoD[indirectdiffuse_refractSampleOffset][(2 * i * rayDepth) + 1];
					u3 = sample->oneD[indirectdiffuse_refractComponentOffset][i * rayDepth];
				} else {
					u1 = sample->twoD[diffuse_refractSampleOffset][2 * i];
					u2 = sample->twoD[diffuse_refractSampleOffset][2 * i + 1];
					u3 = sample->oneD[diffuse_refractComponentOffset][i];
				}

				if (bsdf->Sample_f(tspack, wo, &wi, u1, u2, u3, &f, 
					 &pdf, BxDFType(BSDF_TRANSMISSION | BSDF_DIFFUSE), &flags, NULL, true)) {
					RayDifferential rd(p, wi);
					rd.time = time;
					vector<SWCSpectrum> Ll(L.size(), SWCSpectrum(0.f));
					LiInternal(tspack, scene, rd, sample, Ll, alpha, zdepth, rayDepth + 1, false, nrContribs);
					f *= invsamples * AbsDot(wi, n) / pdf;
					if (diffuserefractReject && (rayDepth == 0 || includeEmit)) {
						for (u_int j = 0; j < Ll.size(); ++j)
							Ll[j] *= f;
						LL.push_back(Ll);
					} else {
						for (u_int j = 0; j < L.size(); ++j)
							L[j] += f * Ll[j];
					}
				}
			}

			if (rayDepth == 0)
				Reject(tspack, LL, L, diffuserefractReject_thr);
		}

		// trace Glossy reflection & transmission rays
		if (rayDepth < glossyreflectDepth) {
			if (rayDepth > 0)
				samples = 1;
			else
				samples = glossyreflectSamples;
			invsamples = 1.f / samples;

			vector< vector<SWCSpectrum> > LL;

			for (u_int i = 0; i < samples; ++i) {
				float u1, u2, u3;
				if (rayDepth > 0) {
					u1 = sample->twoD[indirectglossy_reflectSampleOffset][2 * i * rayDepth];
					u2 = sample->twoD[indirectglossy_reflectSampleOffset][(2 * i * rayDepth) + 1];
					u3 = sample->oneD[indirectglossy_reflectComponentOffset][i * rayDepth];
				} else {
					u1 = sample->twoD[glossy_reflectSampleOffset][2 * i];
					u2 = sample->twoD[glossy_reflectSampleOffset][2 * i + 1];
					u3 = sample->oneD[glossy_reflectComponentOffset][i];
				}

				if (bsdf->Sample_f(tspack, wo, &wi, u1, u2, u3, &f, 
					 &pdf, BxDFType(BSDF_REFLECTION | BSDF_GLOSSY), &flags, NULL, true)) {
					RayDifferential rd(p, wi);
					rd.time = time;
					vector<SWCSpectrum> Ll(L.size(), SWCSpectrum(0.f));
					LiInternal(tspack, scene, rd, sample, Ll, alpha, zdepth, rayDepth + 1, false, nrContribs);
					f *= invsamples * AbsDot(wi, n) / pdf;
					if (glossyreflectReject && (rayDepth == 0 || includeEmit)) {
						for (u_int j = 0; j < Ll.size(); ++j)
							Ll[j] *= f;
						LL.push_back(Ll);
					} else {
						for (u_int j = 0; j < L.size(); ++j)
							L[j] += f * Ll[j];
					}
				}
			}

			if (rayDepth == 0)
				Reject(tspack, LL, L, glossyreflectReject_thr);
		}
		if (rayDepth < glossyrefractDepth) {
			if (rayDepth > 0)
				samples = 1;
			else
				samples = glossyrefractSamples;
			invsamples = 1.f / samples;

			vector< vector<SWCSpectrum> > LL;

			for (u_int i = 0; i < samples; ++i) {
				float u1, u2, u3;
				if (rayDepth > 0) {
					u1 = sample->twoD[indirectglossy_refractSampleOffset][2 * i * rayDepth];
					u2 = sample->twoD[indirectglossy_refractSampleOffset][(2 * i * rayDepth) + 1];
					u3 = sample->oneD[indirectglossy_refractComponentOffset][i * rayDepth];
				} else {
					u1 = sample->twoD[glossy_refractSampleOffset][2 * i];
					u2 = sample->twoD[glossy_refractSampleOffset][2 * i + 1];
					u3 = sample->oneD[glossy_refractComponentOffset][i];
				}

				if (bsdf->Sample_f(tspack, wo, &wi, u1, u2, u3, &f, 
					&pdf, BxDFType(BSDF_TRANSMISSION | BSDF_GLOSSY), &flags, NULL, true)) {
					RayDifferential rd(p, wi);
					rd.time = time;
					vector<SWCSpectrum> Ll(L.size(), SWCSpectrum(0.f));
					LiInternal(tspack, scene, rd, sample, Ll, alpha, zdepth, rayDepth + 1, false, nrContribs);
					f *= invsamples * AbsDot(wi, n) / pdf;
					if (glossyrefractReject && (rayDepth == 0 || includeEmit)) {
						for (u_int j = 0; j < Ll.size(); ++j)
							Ll[j] *= f;
						LL.push_back(Ll);
					} else {
						for (u_int j = 0; j < L.size(); ++j)
							L[j] += f * Ll[j];
					}
				}
			}

			if (rayDepth == 0)
				Reject(tspack, LL, L, glossyrefractReject_thr);
		} 
		
		// trace specular reflection & transmission rays
		if (rayDepth < specularreflectDepth) {
			if (bsdf->Sample_f(tspack, wo, &wi, 1.f, 1.f, 1.f, &f, 
				 &pdf, BxDFType(BSDF_REFLECTION | BSDF_SPECULAR), NULL, NULL, true)) {
				RayDifferential rd(p, wi);
				rd.time = time;
				vector<SWCSpectrum> Ll(L.size(), SWCSpectrum(0.f));
				LiInternal(tspack, scene, rd, sample, Ll, alpha, zdepth, rayDepth + 1, true, nrContribs);
				f *= AbsDot(wi, n);
				for (u_int j = 0; j < L.size(); ++j)
					L[j] += f * Ll[j];
			}
		}
		if (rayDepth < specularrefractDepth) {
			if (bsdf->Sample_f(tspack, wo, &wi, 1.f, 1.f, 1.f, &f, 
				 &pdf, BxDFType(BSDF_TRANSMISSION | BSDF_SPECULAR), NULL, NULL, true)) {
				RayDifferential rd(p, wi);
				rd.time = time;
				vector<SWCSpectrum> Ll(L.size(), SWCSpectrum(0.f));
				LiInternal(tspack, scene, rd, sample, Ll, alpha, zdepth, rayDepth + 1, true, nrContribs);
				f *= AbsDot(wi, n);
				for (u_int j = 0; j < L.size(); ++j)
					L[j] += f * Ll[j];
			}
		} 

	} else {
		// Handle ray with no intersection
		for (u_int i = 0; i < scene->lights.size(); ++i) {
			const SWCSpectrum Le(scene->lights[i]->Le(tspack, ray));
			if (Le.Filter(tspack) > 0.f) {
				L[scene->lights[i]->group] += Le;
				++nrContribs;
			}
		}
		if (rayDepth == 0)
			*alpha = 0.f;
	}

	SWCSpectrum Lt(1.f);
	scene->volumeIntegrator->Transmittance(tspack, scene, ray, sample, alpha, &Lt);
	for (u_int i = 0; i < L.size(); ++i)
		L[i] *= Lt;
	SWCSpectrum Lv(0.f);
	u_int g = scene->volumeIntegrator->Li(tspack, scene, ray, sample, &Lv, alpha);
	L[g] += Lv;
}

u_int DistributedPath::Li(const TsPack *tspack, const Scene *scene,
		const Sample *sample) const
{
	u_int nrContribs = 0;
	float zdepth = 0.f;
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
	LiInternal(tspack, scene, ray, sample, L, &alpha, &zdepth, 0, true, nrContribs);

	for (u_int i = 0; i < L.size(); ++i)
		sample->AddContribution(sample->imageX, sample->imageY,
		XYZColor(tspack, L[i]) * rayWeight, alpha, zdepth, bufferId, i);

	return nrContribs;
}

SurfaceIntegrator* DistributedPath::CreateSurfaceIntegrator(const ParamSet &params) {

	// DirectLight Sampling
	bool directall = params.FindOneBool("directsampleall", true);
	bool directdiffuse = params.FindOneBool("directdiffuse", true);
	bool directglossy = params.FindOneBool("directglossy", true);
	// Indirect DirectLight Sampling
	bool indirectall = params.FindOneBool("indirectsampleall", false);
	bool indirectdiffuse = params.FindOneBool("indirectdiffuse", true);
	bool indirectglossy = params.FindOneBool("indirectglossy", true);

	// Diffuse
	int diffusereflectdepth = params.FindOneInt("diffusereflectdepth", 3);
	int diffusereflectsamples = params.FindOneInt("diffusereflectsamples", 1);
	int diffuserefractdepth = params.FindOneInt("diffuserefractdepth", 5);
	int diffuserefractsamples = params.FindOneInt("diffuserefractsamples", 1);
	// Glossy
	int glossyreflectdepth = params.FindOneInt("glossyreflectdepth", 2);
	int glossyreflectsamples = params.FindOneInt("glossyreflectsamples", 1);
	int glossyrefractdepth = params.FindOneInt("glossyrefractdepth", 5);
	int glossyrefractsamples = params.FindOneInt("glossyrefractsamples", 1);
	// Specular
	int specularreflectdepth = params.FindOneInt("specularreflectdepth", 2);
	int specularrefractdepth = params.FindOneInt("specularrefractdepth", 5);

	// Rejection System
	bool diffusereflectreject = params.FindOneBool("diffusereflectreject", false);
	float diffusereflectreject_thr = params.FindOneFloat("diffusereflectreject_threshold", 10.0f);
	bool diffuserefractreject = params.FindOneBool("diffuserefractreject", false);;
	float diffuserefractreject_thr = params.FindOneFloat("diffuserefractreject_threshold", 10.0f);
	bool glossyreflectreject = params.FindOneBool("glossyreflectreject", false);;
	float glossyreflectreject_thr = params.FindOneFloat("glossyreflectreject_threshold", 10.0f);
	bool glossyrefractreject = params.FindOneBool("glossyrefractreject", false);;
	float glossyrefractreject_thr = params.FindOneFloat("glossyrefractreject_threshold", 10.0f);

	DistributedPath *di = new DistributedPath(directall,
		directdiffuse, directglossy, indirectall, indirectdiffuse, indirectglossy,
		max(diffusereflectdepth, 0), max(diffusereflectsamples, 0), max(diffuserefractdepth, 0), max(diffuserefractsamples, 0), max(glossyreflectdepth, 0), max(glossyreflectsamples, 0), 
		max(glossyrefractdepth, 0), max(glossyrefractsamples, 0), max(specularreflectdepth, 0), max(specularrefractdepth, 0),
		diffusereflectreject, diffusereflectreject_thr, diffuserefractreject, diffuserefractreject_thr,
		glossyreflectreject, glossyreflectreject_thr, glossyrefractreject, glossyrefractreject_thr);
		// Initialize the rendering hints
	di->hints.InitParam(params);

	return di;
}

static DynamicLoader::RegisterSurfaceIntegrator<DistributedPath> r("distributedpath");
