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
#include "mcdistribution.h"
#include "reflection/bxdf.h"
#include "sampling.h"
#include "film.h"
#include "randomgen.h"
#include "dynload.h"
#include "paramset.h"

using namespace lux;

SWCSpectrum VirtualLight::GetSWCSpectrum(const SpectrumWavelengths &sw) const
{
	const float delta = (sw.w[0] - w[0]) * WAVELENGTH_SAMPLES /
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
IGIIntegrator::IGIIntegrator(u_int nl, u_int ns, u_int d, float gl)
{
	nLightPaths = RoundUpPow2(nl);
	nLightSets = RoundUpPow2(ns);
	gLimit = gl;
	maxSpecularDepth = d;
	virtualLights.resize(nLightSets);
}
void IGIIntegrator::RequestSamples(Sample *sample, const Scene &scene)
{
	// Request samples for area light sampling
	u_int nLights = scene.lights.size();
	lightSampleOffset = new u_int[nLights];
	lightSampleNumber = new u_int[nLights];
	bsdfSampleOffset = new u_int[nLights];
	bsdfComponentOffset = new u_int[nLights];
	for (u_int i = 0; i < nLights; ++i) {
		u_int lightSamples = 1;
		lightSampleOffset[i] = sample->Add2D(lightSamples);
		lightSampleNumber[i] = sample->Add1D(lightSamples);
		bsdfSampleOffset[i] = sample->Add2D(lightSamples);
		bsdfComponentOffset[i] = sample->Add1D(lightSamples);
	}
	vlSetOffset = sample->Add1D(1);

	vector<u_int> structure;
	structure.push_back(1);	// bsdf component
	sampleOffset = sample->AddxD(structure, maxSpecularDepth);
}
void IGIIntegrator::Preprocess(const RandomGenerator &rng, const Scene &scene)
{
	// Prepare image buffers
	BufferOutputConfig config = BUF_FRAMEBUFFER;
	BufferType type = BUF_TYPE_PER_PIXEL;
	scene.sampler->GetBufferType(&type);
	bufferId = scene.camera->film->RequestBuffer(type, config, "eye");

	if (scene.lights.size() == 0)
		return;
	// Compute samples for emitted rays from lights
	float *lightNum = new float[nLightPaths * nLightSets];
	float *lightSamp0 = new float[2 * nLightPaths * nLightSets];
	float *lightSamp0b = new float[nLightPaths * nLightSets];
	float *lightSamp1 = new float[2 * nLightPaths * nLightSets];
	float *lightSamp1b = new float[nLightPaths * nLightSets];
	LDShuffleScrambled1D(rng, nLightPaths, nLightSets, lightNum);
	LDShuffleScrambled2D(rng, nLightPaths, nLightSets, lightSamp0);
	LDShuffleScrambled1D(rng, nLightPaths, nLightSets, lightSamp0b);
	LDShuffleScrambled2D(rng, nLightPaths, nLightSets, lightSamp1);
	LDShuffleScrambled1D(rng, nLightPaths, nLightSets, lightSamp1b);
	// Precompute information for light sampling densities
	Sample sample(scene.surfaceIntegrator, scene.volumeIntegrator, scene);
	sample.rng = &rng;
	SpectrumWavelengths &sw(sample.swl);
	u_int nLights = scene.lights.size();
	float *lightPower = new float[nLights];
	for (u_int i = 0; i < nLights; ++i)
		lightPower[i] = scene.lights[i]->Power(scene);
	Distribution1D lightCDF(lightPower, nLights);
	delete[] lightPower;
	for (u_int s = 0; s < nLightSets; ++s) {
		for (u_int i = 0; i < nLightPaths; ++i) {
			sw.Sample(rng.floatValue());
			// Follow path _i_ from light to create virtual lights
			u_int sampOffset = s * nLightPaths + i;
			// Choose light source to trace path from
			float lightPdf;
			u_int lNum = lightCDF.SampleDiscrete(lightNum[sampOffset], &lightPdf);
			Light *light = scene.lights[lNum];
			// Sample ray leaving light source
			BSDF *bsdf;
			float pdf;
			SWCSpectrum alpha;
			if (!light->Sample_L(scene, sample,
				lightSamp0[2 * sampOffset ],
				lightSamp0[2 * sampOffset + 1],
				lightSamp0b[sampOffset], &bsdf, &pdf, &alpha))
				continue;
			Ray ray;
			ray.o = bsdf->dgShading.p;
			SWCSpectrum f;
			float pdf2;
			if (!bsdf->Sample_f(sw, Vector(bsdf->nn), &ray.d,
				lightSamp1[2 * sampOffset],
				lightSamp1[2 * sampOffset + 1],
				lightSamp1b[sampOffset], &f, &pdf2))
				continue;
			alpha *= f;
			alpha /= pdf2 * pdf * lightPdf;
			Intersection isect;
			const Volume *volume = NULL; //FIXME: get it from the light
			u_int nIntersections = 0;
			while (scene.Intersect(sample, volume, ray,
				&isect, &bsdf, &alpha) && !alpha.Black()) {
				++nIntersections;
				Vector wo = -ray.d;
				// Create virtual light at ray intersection point
				if (bsdf->NumComponents(BxDFType(~BSDF_SPECULAR))) {
					const SWCSpectrum Le(alpha *
						bsdf->rho(sw, wo) * INV_PI);
					virtualLights[s].push_back(VirtualLight(sw,
						isect.dg.p, isect.dg.nn, Le));
				}
				// Sample new ray direction and update weight
				Vector wi;
				float pdf;
				BxDFType flags;
				SWCSpectrum fr;
			       	if (!bsdf->Sample_f(sw, wo, &wi,
					rng.floatValue(),
					rng.floatValue(),
					rng.floatValue(),
					&fr, &pdf, BSDF_ALL, &flags))
					break;
				SWCSpectrum anew = fr * AbsDot(wi, bsdf->dgShading.nn) / pdf;
				float r = min(1.f, anew.Filter(sw));
				if (rng.floatValue() > r)
					break;
				alpha *= anew / r;
				ray = Ray(isect.dg.p, wi);
				volume = bsdf->GetVolume(wi);
			}
		}
	}
	delete[] lightNum; // NOBOOK
	delete[] lightSamp0; // NOBOOK
	delete[] lightSamp0b; // NOBOOK
	delete[] lightSamp1; // NOBOOK
	delete[] lightSamp1b; // NOBOOK
}
u_int IGIIntegrator::Li(const Scene &scene, const Sample &sample) const
{
	Ray ray;
	float rayWeight = sample.camera->GenerateRay(scene, sample, &ray);
	const SpectrumWavelengths &sw(sample.swl);
	SWCSpectrum L(0.f), pathThroughput(1.f);
	float alpha = 1.f;
	const Volume *volume = NULL;
	for (u_int depth = 0; ; ++depth) {
		Intersection isect;
		BSDF *bsdf;
		if (!scene.Intersect(sample, volume, ray, &isect, &bsdf,
			&pathThroughput)) {
			// Handle ray with no intersection
			if (depth == 0)
				alpha = 0.f;
			BSDF *ibsdf;
			for (u_int i = 0; i < scene.lights.size(); ++i) {
				SWCSpectrum Le(pathThroughput);
				if (scene.lights[i]->Le(scene, sample, ray,
					&ibsdf, NULL, NULL, &Le))
					L += Le;
			}
			break;
		}
		Vector wo = -ray.d;
		const Point &p = bsdf->dgShading.p;
		const Normal &n = bsdf->dgShading.nn;
		// Compute emitted light if ray hit an area light source
		if (isect.arealight) {
			BSDF *ibsdf;
			L += pathThroughput * isect.Le(sample, ray, &ibsdf,
				NULL, NULL);
		}
		for (u_int i = 0; i < scene.lights.size(); ++i) {
			SWCSpectrum Ld(0.f);
			const float ln = (i + sample.oneD[lightSampleNumber[i]][0]) / scene.lights.size();
			UniformSampleOneLight(scene, sample, p, n, wo, bsdf,
				sample.twoD[lightSampleOffset[i]], &ln,
				sample.twoD[bsdfSampleOffset[i]],
				sample.oneD[bsdfComponentOffset[i]], &Ld);
			L += pathThroughput * Ld / scene.lights.size();
		}
		// Compute indirect illumination with virtual lights
		size_t lSet = min<size_t>(Floor2UInt(sample.oneD[vlSetOffset][0] * nLightSets),
			max<size_t>(1U, virtualLights.size()) - 1U);
		for (u_int i = 0; i < virtualLights[lSet].size(); ++i) {
			const VirtualLight &vl = virtualLights[lSet][i];
			// Add contribution from _VirtualLight_ _vl_
			// Ignore light if it's too close
			float d2 = DistanceSquared(p, vl.p);
			Vector wi = Normalize(vl.p - p);
			float G = AbsDot(wi, n) * AbsDot(wi, vl.n) / d2;
			G = min(G, gLimit);
			// Compute virtual light's tentative contribution _Llight_
			SWCSpectrum f = bsdf->f(sw, wi, wo,
				BxDFType(~BSDF_SPECULAR));
			if (!(G > 0.f) || f.Black())
				continue;
			SWCSpectrum Llight = f * vl.GetSWCSpectrum(sw) *
				(G / nLightPaths);
			if (scene.Connect(sample, bsdf->GetVolume(wi), p, vl.p,
				false, &Llight, NULL, NULL)) {
				L += pathThroughput * Llight;
			}
		}
		if (depth >= maxSpecularDepth)
			break;
		Vector wi;
		SWCSpectrum f;
		float pdf;
		// Trace rays for specular reflection and refraction
		float *data = scene.sampler->GetLazyValues(sample, sampleOffset, depth);
		if (!bsdf->Sample_f(sw, wo, &wi, .5f, .5f, *data, &f, &pdf,
			BxDFType(BSDF_SPECULAR | BSDF_REFLECTION | BSDF_TRANSMISSION)))
			break;
		ray = Ray(p, wi);
		ray.time = sample.realTime;
		pathThroughput *= f * (AbsDot(wi, n) / pdf);
		volume = bsdf->GetVolume(wi);
	}
	const XYZColor color(sw, L);
	sample.AddContribution(sample.imageX, sample.imageY, color * rayWeight,
		alpha, bufferId, 0U);
	return L.Black() ? 0 : 1;
}
SurfaceIntegrator* IGIIntegrator::CreateSurfaceIntegrator(const ParamSet &params)
{
	int nLightSets = params.FindOneInt("nsets", 4);
	int nLightPaths = params.FindOneInt("nlights", 64);
	int maxDepth = params.FindOneInt("maxdepth", 5);
	float maxG = params.FindOneFloat("glimit",
		1.f / params.FindOneFloat("mindist", .1f));
	return new IGIIntegrator(max(nLightPaths, 0), max(nLightSets, 0), max(maxDepth, 0), maxG);
}

static DynamicLoader::RegisterSurfaceIntegrator<IGIIntegrator> r("igi");
