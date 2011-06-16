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
	bool grer, float grert, bool grfr, float grfrt) : SurfaceIntegrator() {
	lightStrategy = st;

	directAll = da;
	directSamples = ds;
	directDiffuse = dd;
	directGlossy = dg;
	indirectAll = ida;
	indirectSamples = ids;
	indirectDiffuse = idd;
	indirectGlossy = idg;
	diffuseReflectDepth = drd;
	diffuseReflectSamples = drs;
	diffuseRefractDepth = dtd;
	diffuseRefractSamples = dts;
	glossyReflectDepth = grd;
	glossyReflectSamples = grs;
	glossyRefractDepth = gtd;
	glossyRefractSamples = gts;
	specularReflectDepth = srd;
	specularRefractDepth = std;

	diffuseReflectReject = drer;
	diffuseReflectRejectThreshold = drert;
	diffuseRefractReject = drfr;
	diffuseRefractRejectThreshold = drfrt;
	glossyReflectReject = grer;
	glossyReflectRejectThreshold = grert;
	glossyRefractReject = grfr;
	glossyRefractRejectThreshold = grfrt;

	AddStringConstant(*this, "name", "Name of current surface integrator", "distributedpath");
}

void DistributedPath::RequestSamples(Sample *sample, const Scene &scene) {
	if (lightStrategy == SAMPLE_AUTOMATIC) {
		if (scene.lights.size() > 7)
			lightStrategy = SAMPLE_ONE_UNIFORM;
		else
			lightStrategy = SAMPLE_ALL_UNIFORM;
	}

	// determine maximum depth for samples
	maxDepth = diffuseReflectDepth;
	maxDepth = max(maxDepth, diffuseRefractDepth);
	maxDepth = max(maxDepth, glossyReflectDepth);
	maxDepth = max(maxDepth, glossyRefractDepth);
	maxDepth = max(maxDepth, specularReflectDepth);
	maxDepth = max(maxDepth, specularRefractDepth);

	// Scattering
	scatterOffset = sample->Add1D(maxDepth);

	// Direct lighting
	// eye vertex
	lightSampleOffset = sample->Add2D(directSamples);
	lightNumOffset = sample->Add1D(directSamples);
	bsdfSampleOffset = sample->Add2D(directSamples);
	bsdfComponentOffset = sample->Add1D(directSamples);
	// remaining vertices
	indirectLightSampleOffset = sample->Add2D(indirectSamples * maxDepth);
	indirectLightNumOffset = sample->Add1D(indirectSamples * maxDepth);
	indirectBsdfSampleOffset = sample->Add2D(indirectSamples * maxDepth);
	indirectBsdfComponentOffset = sample->Add1D(indirectSamples * maxDepth);

	// Diffuse reflection
	// eye vertex
	diffuseReflectSampleOffset = sample->Add2D(diffuseReflectSamples);
	diffuseReflectComponentOffset = sample->Add1D(diffuseReflectSamples);
	// remaining vertices
	indirectDiffuseReflectSampleOffset = sample->Add2D(diffuseReflectDepth);
	indirectDiffuseReflectComponentOffset = sample->Add1D(diffuseReflectDepth);

	// Diffuse refraction
	// eye vertex
	diffuseRefractSampleOffset = sample->Add2D(diffuseRefractSamples);
	diffuseRefractComponentOffset = sample->Add1D(diffuseRefractSamples);
	// remaining vertices
	indirectDiffuseRefractSampleOffset = sample->Add2D(diffuseRefractDepth);
	indirectDiffuseRefractComponentOffset = sample->Add1D(diffuseRefractDepth);

	// Glossy reflection
	// eye vertex
	glossyReflectSampleOffset = sample->Add2D(glossyReflectSamples);
	glossyReflectComponentOffset = sample->Add1D(glossyReflectSamples);
	// remaining vertices
	indirectGlossyReflectSampleOffset = sample->Add2D(glossyReflectDepth);
	indirectGlossyReflectComponentOffset = sample->Add1D(glossyReflectDepth);

	// Glossy refraction
	// eye vertex
	glossyRefractSampleOffset = sample->Add2D(glossyRefractSamples);
	glossyRefractComponentOffset = sample->Add1D(glossyRefractSamples);
	// remaining vertices
	indirectGlossyRefractSampleOffset = sample->Add2D(glossyRefractDepth);
	indirectGlossyRefractComponentOffset = sample->Add1D(glossyRefractDepth);
}
void DistributedPath::Preprocess(const RandomGenerator &rng, const Scene &scene)
{
	// Prepare image buffers
	BufferType type = BUF_TYPE_PER_PIXEL;
	scene.sampler->GetBufferType(&type);
	bufferId = scene.camera->film->RequestBuffer(type, BUF_FRAMEBUFFER, "eye");
}

void DistributedPath::Reject(const SpectrumWavelengths &sw,
	vector< vector<SWCSpectrum> > &LL, vector<SWCSpectrum> &L,
	float rejectRange) const
{
	float totalLum = 0.f;
	const u_int samples = LL.size();
	vector<float> y(samples, 0.f);
	for (u_int i = 0; i < samples; ++i) {
		for (u_int j = 0; j < LL[i].size(); ++j)
			y[i] += LL[i][j].Y(sw);
		totalLum += y[i];
	}
	const float avgLum = totalLum / samples;

	if (avgLum > 0.f) {
		const float limit = avgLum * (1.f + rejectRange);

		// reject
		u_int accepted = 0;
		vector<SWCSpectrum> Lo(L.size(), SWCSpectrum(0.f));
		for (u_int i = 0; i < samples; ++i) {
			if (y[i] <= limit) {
				++accepted;
				for (u_int j = 0; j < LL[i].size(); ++j)
					Lo[j] += LL[i][j];
			}
		}

		const float weight = static_cast<float>(samples) / accepted;

		// Normalize
		for(u_int i = 0; i < L.size(); ++i)
			L[i] += Lo[i] * weight;
	}
}

void DistributedPath::ComputeEvent(const Scene &scene, const Sample &sample,
	BSDF *bsdf, const Vector &wo, u_int nSamples,
	u_int indirectSampleOffset, u_int indirectComponentOffset,
	u_int sampleOffset, u_int componentOffset, BxDFType type,
	bool reject, float threshold, vector<SWCSpectrum> &L, float *alpha,
	float *zdepth, u_int rayDepth, u_int &nrContribs) const
{
	const u_int samples = rayDepth > 0 ? 1 : nSamples;
	const float invsamples = 1.f / samples;

	vector< vector<SWCSpectrum> > LL;

	for (u_int i = 0; i < samples; ++i) {
		float direction[2], component;
		if (rayDepth > 0) {
			const u_int index = i * rayDepth;
			sample.sampler->GetTwoD(sample, indirectSampleOffset,
				index, direction);
			component = sample.sampler->GetOneD(sample,
				indirectComponentOffset, index);
		} else {
			sample.sampler->GetTwoD(sample, sampleOffset, i,
				direction);
			component = sample.sampler->GetOneD(sample,
				componentOffset, i);
		}

		Vector wi;
		SWCSpectrum f;
		float pdf;
		BxDFType flags;
		if (bsdf->SampleF(sample.swl, wo, &wi,
			direction[0], direction[1], component, &f, &pdf, type,
			&flags, NULL, true)) {
			f *= invsamples;
			Ray rd(bsdf->dgShading.p, wi);
			rd.time = bsdf->dgShading.time;
			vector<SWCSpectrum> Ll(L.size(), SWCSpectrum(0.f));
			LiInternal(scene, sample, bsdf->GetVolume(wi),
				bsdf->dgShading.scattered, rd, Ll, alpha,
				zdepth, rayDepth + 1, false, nrContribs);
			if (reject && samples > 1) {
				for (u_int j = 0; j < Ll.size(); ++j)
					Ll[j] *= f;
				LL.push_back(Ll);
			} else {
				for (u_int j = 0; j < Ll.size(); ++j)
					L[j] += f * Ll[j];
			}
		}
	}

	if (reject && samples > 1)
		Reject(sample.swl, LL, L, threshold);
}

void DistributedPath::LiInternal(const Scene &scene, const Sample &sample,
		const Volume *volume, bool scattered, const Ray &ray,
		vector<SWCSpectrum> &L, float *alpha, float *zdepth,
		u_int rayDepth, bool includeEmit, u_int &nrContribs) const
{
	Intersection isect;
	BSDF *bsdf;
	const float time = ray.time; // save time for motion blur
	const SpectrumWavelengths &sw(sample.swl);
	SWCSpectrum Lt(1.f);
	float spdf;

	if (scene.Intersect(sample, volume, scattered, ray,
		sample.sampler->GetOneD(sample, scatterOffset, rayDepth),
		&isect, &bsdf, &spdf, NULL, &Lt)) {
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
				const SWCSpectrum Le(isect.Le(sample, ray,
					&ibsdf, NULL, NULL));
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
				const SWCSpectrum Le(isect.Le(sample, ray,
					&ibsdf, NULL, NULL));
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
		if (scene.lights.size() > 0) {
			const u_int samples = rayDepth > 0 ? indirectSamples :
				directSamples;
			const float invsamples = 1.f / samples;
			float lightSample[2], lightNum;
			float bsdfSample[2], bsdfComponent;
			for (u_int i = 0; i < samples; ++i) {
				// get samples
				if (rayDepth > 0) {
					const u_int index = i * rayDepth;
					sample.sampler->GetTwoD(sample,
						indirectLightSampleOffset,
						index, lightSample);
					lightNum = sample.sampler->GetOneD(sample, indirectLightNumOffset, index);
					sample.sampler->GetTwoD(sample,
						indirectBsdfSampleOffset,
						index, bsdfSample);
					bsdfComponent = sample.sampler->GetOneD(sample, indirectBsdfComponentOffset, index);
				} else {
					sample.sampler->GetTwoD(sample,
						lightSampleOffset, i,
						lightSample);
					lightNum = sample.sampler->GetOneD(sample, lightNumOffset, i);
					sample.sampler->GetTwoD(sample,
						bsdfSampleOffset, i,
						bsdfSample);
					bsdfComponent = sample.sampler->GetOneD(sample, bsdfComponentOffset, i);
				}

				// Apply direct lighting strategy
				switch (lightStrategy) {
					case SAMPLE_ALL_UNIFORM:
						for (u_int i = 0; i < scene.lights.size(); ++i) {
							const SWCSpectrum Ld(EstimateDirect(scene, *(scene.lights[i]), sample, p, n, wo, bsdf,
								lightSample[0], lightSample[1], lightNum, bsdfSample[0], bsdfSample[1], bsdfComponent));
							if (!Ld.Black()) {
								L[scene.lights[i]->group] += invsamples * Ld;
								++nrContribs;
							}
							// TODO add bsdf selection flags
						}
						break;
					case SAMPLE_ONE_UNIFORM:
					{
						SWCSpectrum Ld;
						u_int g = UniformSampleOneLight(scene, sample, p, n, wo, bsdf,
							lightSample, &lightNum, bsdfSample, &bsdfComponent, &Ld);
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

		// trace Diffuse reflection & transmission rays
		if (rayDepth < diffuseReflectDepth) {
			ComputeEvent(scene, sample, bsdf, wo, diffuseReflectSamples,
				indirectDiffuseReflectSampleOffset, indirectDiffuseReflectComponentOffset,
				diffuseReflectSampleOffset, diffuseReflectComponentOffset,
				BxDFType(BSDF_REFLECTION | BSDF_DIFFUSE),
				diffuseReflectReject, diffuseReflectRejectThreshold,
				L, alpha, zdepth, rayDepth, nrContribs);
		}
		if (rayDepth < diffuseRefractDepth) {
			ComputeEvent(scene, sample, bsdf, wo, diffuseRefractSamples,
				indirectDiffuseRefractSampleOffset, indirectDiffuseRefractComponentOffset,
				diffuseRefractSampleOffset, diffuseRefractComponentOffset,
				BxDFType(BSDF_TRANSMISSION | BSDF_DIFFUSE),
				diffuseRefractReject, diffuseRefractRejectThreshold,
				L, alpha, zdepth, rayDepth, nrContribs);
		}

		// trace Glossy reflection & transmission rays
		if (rayDepth < glossyReflectDepth) {
			ComputeEvent(scene, sample, bsdf, wo, glossyReflectSamples,
				indirectGlossyReflectSampleOffset, indirectGlossyReflectComponentOffset,
				glossyReflectSampleOffset, glossyReflectComponentOffset,
				BxDFType(BSDF_REFLECTION | BSDF_GLOSSY),
				glossyReflectReject, glossyReflectRejectThreshold,
				L, alpha, zdepth, rayDepth, nrContribs);
		}
		if (rayDepth < glossyRefractDepth) {
			ComputeEvent(scene, sample, bsdf, wo, glossyRefractSamples,
				indirectGlossyRefractSampleOffset, indirectGlossyRefractComponentOffset,
				glossyRefractSampleOffset, glossyRefractComponentOffset,
				BxDFType(BSDF_TRANSMISSION | BSDF_GLOSSY),
				glossyRefractReject, glossyRefractRejectThreshold,
				L, alpha, zdepth, rayDepth, nrContribs);
		}
		
		// trace specular reflection & transmission rays
		if (rayDepth < specularReflectDepth) {
			float pdf;
			Vector wi;
			SWCSpectrum f;
			if (bsdf->SampleF(sw, wo, &wi, .5f, .5f, .5f, &f, &pdf,
				BxDFType(BSDF_REFLECTION | BSDF_SPECULAR),
				NULL, NULL, true)) {
				Ray rd(p, wi);
				rd.time = time;
				vector<SWCSpectrum> Ll(L.size(),
					SWCSpectrum(0.f));
				LiInternal(scene, sample, bsdf->GetVolume(wi),
					bsdf->dgShading.scattered, rd, Ll,
					alpha, zdepth, rayDepth + 1, true,
					nrContribs);
				for (u_int j = 0; j < L.size(); ++j)
					L[j] += f * Ll[j];
			}
		}
		if (rayDepth < specularRefractDepth) {
			float pdf;
			Vector wi;
			SWCSpectrum f;
			if (bsdf->SampleF(sw, wo, &wi, .5f, .5f, .5f, &f, &pdf,
				BxDFType(BSDF_TRANSMISSION | BSDF_SPECULAR),
				NULL, NULL, true)) {
				Ray rd(p, wi);
				rd.time = time;
				vector<SWCSpectrum> Ll(L.size(),
					SWCSpectrum(0.f));
				LiInternal(scene, sample, bsdf->GetVolume(wi),
					bsdf->dgShading.scattered, rd, Ll,
					alpha, zdepth, rayDepth + 1, true,
					nrContribs);
				for (u_int j = 0; j < L.size(); ++j)
					L[j] += f * Ll[j];
			}
		}
	} else {
		// Handle ray with no intersection
		BSDF *ibsdf;
		for (u_int i = 0; i < scene.lights.size(); ++i) {
			SWCSpectrum Le(1.f);
			if (scene.lights[i]->Le(scene, sample, ray, &ibsdf,
				NULL, NULL, &Le)) {
				L[scene.lights[i]->group] += Le;
				++nrContribs;
			}
		}
		if (rayDepth == 0)
			*alpha = 0.f;
	}
	Lt /= spdf;

	for (u_int i = 0; i < L.size(); ++i)
		L[i] *= Lt;
	SWCSpectrum Lv(0.f);
	u_int g = scene.volumeIntegrator->Li(scene, ray, sample, &Lv, alpha);
	L[g] += Lv;
}

u_int DistributedPath::Li(const Scene &scene, const Sample &sample) const
{
	u_int nrContribs = 0;
	float zdepth = 0.f;
	Ray ray;
	float rayWeight = sample.camera->GenerateRay(scene, sample, &ray);

	vector<SWCSpectrum> L(scene.lightGroups.size(), SWCSpectrum(0.f));
	float alpha = 1.f;
	LiInternal(scene, sample, NULL, false, ray, L, &alpha, &zdepth, 0, true,
		nrContribs);

	for (u_int i = 0; i < L.size(); ++i)
		sample.AddContribution(sample.imageX, sample.imageY,
			XYZColor(sample.swl, L[i]) * rayWeight, alpha, zdepth,
			bufferId, i);

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
		LOG(LUX_WARNING,LUX_BADTOKEN)<<"Strategy  '"<<st<<"' for direct lighting unknown. Using \"auto\".";
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
