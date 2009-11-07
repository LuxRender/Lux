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
 *   Lux Renderer website : http://www.luxrender.org                       *
 ***************************************************************************/

// igi.cpp*
#include "igi.h"
#include "scene.h"
#include "light.h"
#include "camera.h"
#include "mc.h"
#include "reflection/bxdf.h"
#include "dynload.h"
#include "paramset.h"

using namespace lux;

SWCSpectrum VirtualLight::GetSWCSpectrum(const TsPack* tspack) const
{
	const float delta = (tspack->swl->w[0] - w[0]) * WAVELENGTH_SAMPLES /
		(WAVELENGTH_END - WAVELENGTH_START);
	SWCSpectrum result;
	if (delta < 0.f) {
		result.c[0] = Lerp(-delta, Le.c[0], 0.f);
		for (u_int i = 1; i < WAVELENGTH_SAMPLES; ++i)
			result.c[i] = Lerp(-delta, Le.c[i], Le.c[i - 1]);
	} else {
		for (u_int i = 0; i < WAVELENGTH_SAMPLES - 1; ++i)
			result.c[i] = Lerp(delta, Le.c[i], Le.c[i + 1]);
		result.c[WAVELENGTH_SAMPLES - 1] = Lerp(delta,
			Le.c[WAVELENGTH_SAMPLES - 1], 0.f);
	}
	return result;
}

// IGIIntegrator Implementation
IGIIntegrator::IGIIntegrator(u_int nl, u_int ns, u_int d, float md)
{
	nLightPaths = RoundUpPow2(nl);
	nLightSets = RoundUpPow2(ns);
	minDist2 = md * md;
	maxSpecularDepth = d;
	virtualLights.resize(nLightSets);
}
void IGIIntegrator::RequestSamples(Sample *sample, const Scene *scene)
{
	// Request samples for area light sampling
	u_int nLights = scene->lights.size();
	lightSampleOffset = new u_int[nLights];
	bsdfSampleOffset = new u_int[nLights];
	bsdfComponentOffset = new u_int[nLights];
	for (u_int i = 0; i < nLights; ++i) {
		u_int lightSamples = 1;
		lightSampleOffset[i] = sample->Add2D(lightSamples);
		bsdfSampleOffset[i] = sample->Add2D(lightSamples);
		bsdfComponentOffset[i] = sample->Add1D(lightSamples);
	}
	vlSetOffset = sample->Add1D(1);
}
void IGIIntegrator::Preprocess(const TsPack *tspack, const Scene *scene)
{
	// Prepare image buffers
	BufferOutputConfig config = BUF_FRAMEBUFFER;
	BufferType type = BUF_TYPE_PER_PIXEL;
	scene->sampler->GetBufferType(&type);
	bufferId = scene->camera->film->RequestBuffer(type, config, "eye");

	if (scene->lights.size() == 0)
		return;
	// Compute samples for emitted rays from lights
	float *lightNum = new float[nLightPaths * nLightSets];
	float *lightSamp0 = new float[2 * nLightPaths *	nLightSets];
	float *lightSamp1 = new float[2 * nLightPaths * nLightSets];
	LDShuffleScrambled1D(tspack, nLightPaths, nLightSets, lightNum);
	LDShuffleScrambled2D(tspack, nLightPaths, nLightSets, lightSamp0);
	LDShuffleScrambled2D(tspack, nLightPaths, nLightSets, lightSamp1);
	// Precompute information for light sampling densities
	tspack->swl->Sample(.5f);
	u_int nLights = scene->lights.size();
	float *lightPower = new float[nLights];
	float *lightCDF = new float[nLights + 1];
	for (u_int i = 0; i < nLights; ++i)
		lightPower[i] = scene->lights[i]->Power(tspack, scene).Y(tspack);
	float totalPower;
	ComputeStep1dCDF(lightPower, nLights, &totalPower, lightCDF);
	for (u_int s = 0; s < nLightSets; ++s) {
		for (u_int i = 0; i < nLightPaths; ++i) {
			tspack->swl->Sample(tspack->rng->floatValue());
			// Follow path _i_ from light to create virtual lights
			u_int sampOffset = s * nLightPaths + i;
			// Choose light source to trace path from
			float lightPdf;
			u_int lNum = Floor2UInt(SampleStep1d(lightPower, lightCDF,
				totalPower, nLights, lightNum[sampOffset],
				&lightPdf) * nLights);
			lNum = Clamp(lNum, 0U, nLights - 1U);
			Light *light = scene->lights[lNum];
			// Sample ray leaving light source
			RayDifferential ray;
			float pdf;
			SWCSpectrum alpha(light->Sample_L(tspack, scene,
				lightSamp0[2 * sampOffset],
				lightSamp0[2 * sampOffset + 1],
				lightSamp1[2 * sampOffset],
				lightSamp1[2 * sampOffset + 1],
				&ray, &pdf));
			if (pdf == 0.f || alpha.Black())
				continue;
			alpha /= pdf * lightPdf;
			Intersection isect;
			u_int nIntersections = 0;
			while (scene->Intersect(ray, &isect) &&
				!alpha.Black()) {
				++nIntersections;
//				alpha *= scene->Transmittance(ray);
				Vector wo = -ray.d;
				BSDF *bsdf = isect.GetBSDF(tspack, ray);
				// Create virtual light at ray intersection point
				SWCSpectrum Le = alpha * bsdf->rho(tspack, wo) / M_PI;
				virtualLights[s].push_back(
					VirtualLight(tspack, isect.dg.p,
					isect.dg.nn, Le));
				// Sample new ray direction and update weight
				Vector wi;
				float pdf;
				BxDFType flags;
				SWCSpectrum fr;
			       	if (!bsdf->Sample_f(tspack, wo, &wi,
					tspack->rng->floatValue(),
					tspack->rng->floatValue(),
					tspack->rng->floatValue(),
					&fr, &pdf, BSDF_ALL, &flags))
					break;
				SWCSpectrum anew = fr * AbsDot(wi, bsdf->dgShading.nn) / pdf;
				float r = min(1.f, anew.Filter(tspack));
				if (tspack->rng->floatValue() > r)
					break;
				alpha *= anew / r;
				ray = RayDifferential(isect.dg.p, wi, scene->machineEpsilon);
			}
		}
	}
	delete[] lightCDF;
	delete[] lightPower;
	delete[] lightNum; // NOBOOK
	delete[] lightSamp0; // NOBOOK
	delete[] lightSamp1; // NOBOOK
}
u_int IGIIntegrator::Li(const TsPack *tspack, const Scene *scene,
	const Sample *sample) const
{
	RayDifferential r;
	float rayWeight = tspack->camera->GenerateRay(*sample, &r);
	if (rayWeight > 0.f) {
		// Generate ray differentials for camera ray
		++(sample->imageX);
		float wt1 = tspack->camera->GenerateRay(*sample, &r.rx);
		--(sample->imageX);
		++(sample->imageY);
		float wt2 = tspack->camera->GenerateRay(*sample, &r.ry);
		r.hasDifferentials = (wt1 > 0.f) && (wt2 > 0.f);
		--(sample->imageY);
	}

	RayDifferential ray(r);
	SWCSpectrum L(0.f), pathThroughput(1.f);
	float alpha = 1.f;
	for (u_int depth = 0; ; ++depth) {
		Intersection isect;
		if (!scene->Intersect(r, &isect)) {
			// Handle ray with no intersection
			if (depth == 0)
				alpha = 0.f;
			for (u_int i = 0; i < scene->lights.size(); ++i)
				L += pathThroughput * scene->lights[i]->Le(tspack, r);
			break;
		}
		Vector wo = -r.d;
		// Compute emitted light if ray hit an area light source
		L += pathThroughput * isect.Le(tspack, wo);
		// Evaluate BSDF at hit point
		BSDF *bsdf = isect.GetBSDF(tspack, r);
		const Point &p = bsdf->dgShading.p;
		const Normal &n = bsdf->dgShading.nn;
		for (u_int i = 0; i < scene->lights.size(); ++i) {
			SWCSpectrum Ld(0.f);
			float ln = Clamp((i + tspack->rng->floatValue()) / scene->lights.size(), 0.f, 1.f);
			UniformSampleOneLight(tspack, scene, p, n, wo, bsdf,
				sample, sample->twoD[lightSampleOffset[i]],
				&ln,
				sample->twoD[bsdfSampleOffset[i]],
				sample->oneD[bsdfComponentOffset[i]], &Ld);
			L += pathThroughput * Ld / scene->lights.size();
		}
		// Compute indirect illumination with virtual lights
		size_t lSet = min<size_t>(Floor2UInt(sample->oneD[vlSetOffset][0] * nLightSets),
			max<size_t>(1U, virtualLights.size()) - 1U);
		for (size_t i = 0; i < virtualLights[lSet].size(); ++i) {
			const VirtualLight &vl = virtualLights[lSet][i];
			// Add contribution from _VirtualLight_ _vl_
			// Ignore light if it's too close
			float d2 = DistanceSquared(p, vl.p);
			if (d2 < .8f * minDist2)
				continue;
			float distScale = SmoothStep(.8f * minDist2,
				1.2f * minDist2, d2);
			// Compute virtual light's tentative contribution _Llight_
			Vector wi = Normalize(vl.p - p);
			SWCSpectrum f = distScale * bsdf->f(tspack, wi, wo, BxDFType(~BSDF_SPECULAR));
			if (f.Black())
				continue;
			float G = AbsDot(wi, n) * AbsDot(wi, vl.n) / d2;
			SWCSpectrum Llight = f * vl.GetSWCSpectrum(tspack) *
				(G / virtualLights[lSet].size());
			scene->Transmittance(tspack, Ray(p, vl.p - p, scene->machineEpsilon),
				sample, &Llight);
			if (!scene->IntersectP(Ray(p, vl.p - p, RAY_EPSILON,
				1.f - RAY_EPSILON)))
				L += pathThroughput * Llight;
		}
		// Trace rays for specular reflection and refraction
		if (depth >= maxSpecularDepth)
			break;
		Vector wi;
		// Trace rays for specular reflection and refraction
		SWCSpectrum f;
		float pdf;
		if (!bsdf->Sample_f(tspack, wo, &wi, .5f, .5f, tspack->rng->floatValue(), &f, &pdf, BxDFType(BSDF_SPECULAR | BSDF_REFLECTION | BSDF_TRANSMISSION)))
			break;
		// Compute ray differential _rd_ for specular reflection
		r = RayDifferential(p, wi, scene->machineEpsilon);
		r.time = ray.time;
		pathThroughput *= f * (AbsDot(wi, n) / pdf);
	}
	const XYZColor color(L.ToXYZ(tspack) * rayWeight);
	sample->AddContribution(sample->imageX, sample->imageY, color,
		alpha, bufferId, 0U);
	return L.Black() ? 0 : 1;
}
SurfaceIntegrator* IGIIntegrator::CreateSurfaceIntegrator(const ParamSet &params)
{
	int nLightSets = params.FindOneInt("nsets", 4);
	int nLightPaths = params.FindOneInt("nlights", 64);
	int maxDepth = params.FindOneInt("maxdepth", 5);
	float minDist = params.FindOneFloat("mindist", .1f);
	return new IGIIntegrator(max(nLightPaths, 0), max(nLightSets, 0), max(maxDepth, 0), minDist);
}

static DynamicLoader::RegisterSurfaceIntegrator<IGIIntegrator> r("igi");
