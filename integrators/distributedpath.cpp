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
#include "sampling.h"
#include "film.h"
#include "color.h"
#include "paramset.h"
#include "dynload.h"

using namespace lux;

// DistributedPath Method Definitions
DistributedPath::DistributedPath(LightStrategy st, bool da, u_int ds, bool dd, bool dg, bool ida, u_int ids, bool idd, bool idg,
	u_int drd, u_int drs, u_int dtd, u_int dts, u_int grd, u_int grs, u_int gtd, u_int gts, u_int srd, u_int std,
	bool drer, float drert, bool drfr, float drfrt,
	bool grer, float grert, bool grfr, float grfrt) {
	lightStrategy = st;

	directAll = da;
	directSamples = ds;
	directDiffuse = dd;
	directGlossy = dg;
	indirectAll = ida;
	indirectSamples = ids;
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
}

void DistributedPath::RequestSamples(Sample *sample, const Scene *scene) {
	if (lightStrategy == SAMPLE_AUTOMATIC) {
		if (scene->sampler->IsMutating() || scene->lights.size() > 7)
			lightStrategy = SAMPLE_ONE_UNIFORM;
		else
			lightStrategy = SAMPLE_ALL_UNIFORM;
	}

	// determine maximum depth for samples
	maxDepth = diffusereflectDepth;
	maxDepth = max(maxDepth, diffuserefractDepth);
	maxDepth = max(maxDepth, glossyreflectDepth);
	maxDepth = max(maxDepth, glossyrefractDepth);
	maxDepth = max(maxDepth, specularreflectDepth);
	maxDepth = max(maxDepth, specularrefractDepth);

	// Direct lighting
	// eye vertex
	lightSampleOffset = sample->Add2D(directSamples);
	lightNumOffset = sample->Add1D(directSamples);
	bsdfSampleOffset = sample->Add2D(directSamples);
	bsdfComponentOffset = sample->Add1D(directSamples);
	// remaining vertices
	indirectlightSampleOffset = sample->Add2D(indirectSamples * maxDepth);
	indirectlightNumOffset = sample->Add1D(indirectSamples * maxDepth);
	indirectbsdfSampleOffset = sample->Add2D(indirectSamples * maxDepth);
	indirectbsdfComponentOffset = sample->Add1D(indirectSamples * maxDepth);

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

}
void DistributedPath::Preprocess(const TsPack *tspack, const Scene *scene)
{
	// Prepare image buffers
	BufferType type = BUF_TYPE_PER_PIXEL;
	scene->sampler->GetBufferType(&type);
	bufferId = scene->camera->film->RequestBuffer(type, BUF_FRAMEBUFFER, "eye");
}

void DistributedPath::Reject(const SpectrumWavelengths &sw,
	vector< vector<SWCSpectrum> > &LL, vector<SWCSpectrum> &L,
	float rejectrange) const
{
	float totallum = 0.f;
	float samples = LL.size();
	for (u_int i = 0; i < samples; ++i)
		for (u_int j = 0; j < LL[i].size(); ++j)
			totallum += LL[i][j].Y(sw) * samples;
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
				y += LL[i][j].Y(sw) * samples;
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
		const Volume *volume, const RayDifferential &ray,
		const Sample *sample, vector<SWCSpectrum> &L, float *alpha,
		float *zdepth, u_int rayDepth, bool includeEmit,
		u_int &nrContribs) const
{
	Intersection isect;
	BSDF *bsdf;
	const float time = ray.time; // save time for motion blur
	const SpectrumWavelengths &sw(sample->swl);
	SWCSpectrum Lt(1.f);

	if (scene->Intersect(tspack, sample, volume, ray, &isect, &bsdf, &Lt)) {
		// Evaluate BSDF at hit point
		Vector wo = -ray.d;
		const Point &p = bsdf->dgShading.p;
		const Normal &n = bsdf->dgShading.nn;

		if (rayDepth == 0) {
			// Set Zbuf depth
			const Vector zv(p - ray.o);
			*zdepth = zv.Length();

			// Override alpha
			if(bsdf->compParams->oA)
				*alpha = bsdf->compParams->A;

			// Compute emitted light if ray hit an area light source with Visibility check
			if(bsdf->compParams->tVl && includeEmit &&
				isect.arealight) {
				BSDF *ibsdf;
				const SWCSpectrum Le(isect.Le(tspack->arena,
					sample, ray, &ibsdf, NULL, NULL));
				if (!Le.Black()) {
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
			if(bsdf->compParams->tiVl && includeEmit &&
				isect.arealight) {
				BSDF *ibsdf;
				const SWCSpectrum Le(isect.Le(tspack->arena,
					sample, ray, &ibsdf, NULL, NULL));
				if (!Le.Black()) {
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
			const u_int samples = rayDepth > 0 ? indirectSamples :
				directSamples;
			const float invsamples = 1.f / samples;
			const float *lightSample, *lightNum, *bsdfSample, *bsdfComponent;
			for (u_int i = 0; i < samples; ++i) {
				// get samples
				if (rayDepth > 0) {
					lightSample = &sample->twoD[indirectlightSampleOffset][2 * i * rayDepth];
					lightNum = &sample->oneD[indirectlightNumOffset][i * rayDepth];
					bsdfSample = &sample->twoD[indirectbsdfSampleOffset][2 * i * rayDepth];
					bsdfComponent = &sample->oneD[indirectbsdfComponentOffset][i * rayDepth];
				} else {
					lightSample = &sample->twoD[lightSampleOffset][2 * i];
					lightNum = &sample->oneD[lightNumOffset][i];
					bsdfSample = &sample->twoD[bsdfSampleOffset][2 * i];
					bsdfComponent = &sample->oneD[bsdfComponentOffset][i];
				}

				// Apply direct lighting strategy
				switch (lightStrategy) {
					case SAMPLE_ALL_UNIFORM:
						for (u_int i = 0; i < scene->lights.size(); ++i) {
							const SWCSpectrum Ld(EstimateDirect(tspack, scene, scene->lights[i], p, n, wo, bsdf,
								sample, lightSample[0], lightSample[1], *lightNum, bsdfSample[0], bsdfSample[1], *bsdfComponent));
							if (!Ld.Black()) {
								L[scene->lights[i]->group] += invsamples * Ld;
								++nrContribs;
							}
							// TODO add bsdf selection flags
						}
						break;
					case SAMPLE_ONE_UNIFORM:
					{
						SWCSpectrum Ld;
						u_int g = UniformSampleOneLight(tspack, scene, p, n,
							wo, bsdf, sample,
							lightSample, lightNum, bsdfSample, bsdfComponent, &Ld);
						if (!Ld.Black()) {
							L[g] += invsamples * Ld;
							++nrContribs;
						}
						break;
					}
					default:
						break;
				}
			}
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

				if (bsdf->Sample_f(sw, wo, &wi, u1, u2, u3, &f, 
					 &pdf, BxDFType(BSDF_REFLECTION | BSDF_DIFFUSE), &flags, NULL, true)) {
					RayDifferential rd(p, wi);
					rd.time = time;
					vector<SWCSpectrum> Ll(L.size(), SWCSpectrum(0.f));
					LiInternal(tspack, scene, bsdf->GetVolume(wi), rd, sample, Ll, alpha, zdepth, rayDepth + 1, false, nrContribs);
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
				Reject(sw, LL, L, diffusereflectReject_thr);
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

				if (bsdf->Sample_f(sw, wo, &wi, u1, u2, u3, &f, 
					 &pdf, BxDFType(BSDF_TRANSMISSION | BSDF_DIFFUSE), &flags, NULL, true)) {
					RayDifferential rd(p, wi);
					rd.time = time;
					vector<SWCSpectrum> Ll(L.size(), SWCSpectrum(0.f));
					LiInternal(tspack, scene, bsdf->GetVolume(wi), rd, sample, Ll, alpha, zdepth, rayDepth + 1, false, nrContribs);
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
				Reject(sw, LL, L, diffuserefractReject_thr);
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

				if (bsdf->Sample_f(sw, wo, &wi, u1, u2, u3, &f, 
					 &pdf, BxDFType(BSDF_REFLECTION | BSDF_GLOSSY), &flags, NULL, true)) {
					RayDifferential rd(p, wi);
					rd.time = time;
					vector<SWCSpectrum> Ll(L.size(), SWCSpectrum(0.f));
					LiInternal(tspack, scene, bsdf->GetVolume(wi), rd, sample, Ll, alpha, zdepth, rayDepth + 1, false, nrContribs);
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
				Reject(sw, LL, L, glossyreflectReject_thr);
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

				if (bsdf->Sample_f(sw, wo, &wi, u1, u2, u3, &f, 
					&pdf, BxDFType(BSDF_TRANSMISSION | BSDF_GLOSSY), &flags, NULL, true)) {
					RayDifferential rd(p, wi);
					rd.time = time;
					vector<SWCSpectrum> Ll(L.size(), SWCSpectrum(0.f));
					LiInternal(tspack, scene, bsdf->GetVolume(wi), rd, sample, Ll, alpha, zdepth, rayDepth + 1, false, nrContribs);
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
				Reject(sw, LL, L, glossyrefractReject_thr);
		} 
		
		// trace specular reflection & transmission rays
		if (rayDepth < specularreflectDepth) {
			if (bsdf->Sample_f(sw, wo, &wi, 1.f, 1.f, 1.f, &f, 
				 &pdf, BxDFType(BSDF_REFLECTION | BSDF_SPECULAR), NULL, NULL, true)) {
				RayDifferential rd(p, wi);
				rd.time = time;
				vector<SWCSpectrum> Ll(L.size(), SWCSpectrum(0.f));
				LiInternal(tspack, scene, bsdf->GetVolume(wi), rd, sample, Ll, alpha, zdepth, rayDepth + 1, true, nrContribs);
				f *= AbsDot(wi, n);
				for (u_int j = 0; j < L.size(); ++j)
					L[j] += f * Ll[j];
			}
		}
		if (rayDepth < specularrefractDepth) {
			if (bsdf->Sample_f(sw, wo, &wi, 1.f, 1.f, 1.f, &f, 
				 &pdf, BxDFType(BSDF_TRANSMISSION | BSDF_SPECULAR), NULL, NULL, true)) {
				RayDifferential rd(p, wi);
				rd.time = time;
				vector<SWCSpectrum> Ll(L.size(), SWCSpectrum(0.f));
				LiInternal(tspack, scene, bsdf->GetVolume(wi), rd, sample, Ll, alpha, zdepth, rayDepth + 1, true, nrContribs);
				f *= AbsDot(wi, n);
				for (u_int j = 0; j < L.size(); ++j)
					L[j] += f * Ll[j];
			}
		} 

	} else {
		// Handle ray with no intersection
		BSDF *ibsdf;
		for (u_int i = 0; i < scene->lights.size(); ++i) {
			SWCSpectrum Le(1.f);
			if (scene->lights[i]->Le(tspack->arena, scene, sample,
				ray, &ibsdf, NULL, NULL, &Le)) {
				L[scene->lights[i]->group] += Le;
				++nrContribs;
			}
		}
		if (rayDepth == 0)
			*alpha = 0.f;
	}

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
	float rayWeight = sample->camera->GenerateRay(tspack->arena, scene,
		*sample, &ray);

	vector<SWCSpectrum> L(scene->lightGroups.size(), SWCSpectrum(0.f));
	float alpha = 1.f;
	LiInternal(tspack, scene, NULL, ray, sample, L, &alpha, &zdepth, 0, true, nrContribs);

	for (u_int i = 0; i < L.size(); ++i)
		sample->AddContribution(sample->imageX, sample->imageY,
		XYZColor(sample->swl, L[i]) * rayWeight, alpha, zdepth, bufferId, i);

	return nrContribs;
}

SurfaceIntegrator* DistributedPath::CreateSurfaceIntegrator(const ParamSet &params) {

	// DirectLight Sampling
	bool directall = params.FindOneBool("directsampleall", true);
	int directsamples = params.FindOneInt("directsamples", 1);
	bool directdiffuse = params.FindOneBool("directdiffuse", true);
	bool directglossy = params.FindOneBool("directglossy", true);
	// Indirect DirectLight Sampling
	bool indirectall = params.FindOneBool("indirectsampleall", false);
	int indirectsamples = params.FindOneInt("indirectsamples", 1);
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

	// Rejection System
	bool diffusereflectreject = params.FindOneBool("diffusereflectreject", false);
	float diffusereflectreject_thr = params.FindOneFloat("diffusereflectreject_threshold", 10.0f);
	bool diffuserefractreject = params.FindOneBool("diffuserefractreject", false);;
	float diffuserefractreject_thr = params.FindOneFloat("diffuserefractreject_threshold", 10.0f);
	bool glossyreflectreject = params.FindOneBool("glossyreflectreject", false);;
	float glossyreflectreject_thr = params.FindOneFloat("glossyreflectreject_threshold", 10.0f);
	bool glossyrefractreject = params.FindOneBool("glossyrefractreject", false);;
	float glossyrefractreject_thr = params.FindOneFloat("glossyrefractreject_threshold", 10.0f);

	return new DistributedPath(estrategy, directall, max(directsamples, 0),
		directdiffuse, directglossy, indirectall, max(indirectsamples, 0), indirectdiffuse, indirectglossy,
		max(diffusereflectdepth, 0), max(diffusereflectsamples, 0), max(diffuserefractdepth, 0), max(diffuserefractsamples, 0), max(glossyreflectdepth, 0), max(glossyreflectsamples, 0), 
		max(glossyrefractdepth, 0), max(glossyrefractsamples, 0), max(specularreflectdepth, 0), max(specularrefractdepth, 0),
		diffusereflectreject, diffusereflectreject_thr, diffuserefractreject, diffuserefractreject_thr,
		glossyreflectreject, glossyreflectreject_thr, glossyrefractreject, glossyrefractreject_thr);
}

static DynamicLoader::RegisterSurfaceIntegrator<DistributedPath> r("distributedpath");
