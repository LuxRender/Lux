/***************************************************************************
 *   Copyright (C) 1998-2011 by authors (see AUTHORS.txt )                 *
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

// multi.cpp*
#include "multi.h"
#include "randomgen.h"
#include "light.h"
#include "bxdf.h"
#include "sampling.h"
#include "paramset.h"
#include "dynload.h"

using namespace lux;

// MultiScattering Method Definitions
void MultiScattering::RequestSamples(Sample *sample, const Scene &scene)
{
	tauSampleOffset = sample->Add1D(1);
	scatterSampleOffset = sample->Add1D(1);
}

void MultiScattering::Transmittance(const Scene &scene, const Ray &ray,
	const Sample &sample, float *alpha, SWCSpectrum *const L) const
{
	if (!scene.volumeRegion) 
		return;
	const float step = stepSize; // TODO - handle varying step size
	const float offset = sample.oneD[tauSampleOffset][0];
	const SWCSpectrum tau(scene.volumeRegion->Tau(sample.swl, ray, step,
		offset));
	*L *= Exp(-tau);
}

u_int MultiScattering::Li(const Scene &scene, const Ray &ray,
	const Sample &sample, SWCSpectrum *Lv, float *alpha) const
{
	*Lv = 0.f;
	Region *vr = scene.volumeRegion;
	float t0, t1;
	if (!vr || !vr->IntersectP(ray, &t0, &t1))
		return 0;
	// Do single scattering volume integration in _vr_
	// Multiple scattering in this context is almost impossible to do
	// Prepare for volume integration stepping
	const float length = ray.d.Length();
	const u_int N = Ceil2UInt((t1 - t0) * length / stepSize);
	const float step = (t1 - t0) / N;
	const SpectrumWavelengths &sw(sample.swl);
	SWCSpectrum Tr(1.f);
	const Vector w(-ray.d / length);
	t0 += sample.oneD[tauSampleOffset][0] * step;
	Ray r(ray(t0), ray.d, 0.f, step, ray.time);
	const u_int nLights = scene.lights.size();
	const u_int lightNum = min(nLights - 1,
		Floor2UInt(sample.oneD[scatterSampleOffset][0] * nLights));
	Light *light = scene.lights[lightNum];

	// Compute sample patterns for single scattering samples
	// FIXME - use real samples
	float *samp = static_cast<float *>(alloca(3 * N * sizeof(float)));
	LatinHypercube(*(sample.rng), samp, N, 3);
	u_int sampOffset = 0;
	DifferentialGeometry dg;
	dg.p = r.o;
	dg.nn = Normal(w);
	for (u_int i = 0; i < N; ++i) {
		// Ray is already offset above, no need to do it again
		const SWCSpectrum stepTau(vr->Tau(sw, r, .5f * stepSize, 0.f));
		Tr *= Exp(-stepTau);
		// Possibly terminate raymarching if transmittance is small
		if (Tr.Filter(sw) < 1e-3f) {
			const float continueProb = .5f;
			if (sample.rng->floatValue() > continueProb)
				break; // TODO - REFACT - remove and add random value from sample
			Tr /= continueProb;
		}

		// Compute single-scattering source term at _p_
		*Lv += Tr * vr->Lve(sw, dg);

		if (nLights > 0) {
			const SWCSpectrum ss(vr->SigmaS(sw, dg));
			if (!ss.Black()) {
				// Add contribution of _light_ due to scattering at _p_
				float pdf;
				float u1 = samp[sampOffset], u2 = samp[sampOffset + 1],
					u3 = samp[sampOffset + 2];
				BSDF *ibsdf;
				SWCSpectrum L;
				if (light->SampleL(scene, sample, r.o, u1, u2, u3,
					&ibsdf, NULL, &pdf, &L)) {
					if (Connect(scene, sample, vr, true, false,
						r.o, ibsdf->dgShading.p, false, &L,
						NULL, NULL)) {
						const Vector wo(Normalize(r.o - ibsdf->dgShading.p));
						*Lv += Tr * ss * L *
							ibsdf->F(sw, Vector(ibsdf->nn), wo, false) *
							(vr->P(sw, dg, w, wo) *
							nLights);
					}
				}
			}
		}
		sampOffset += 3;

		// Advance to sample at _t0_ and update _T_
		t0 += step;
		r.o = ray(t0);
		dg.p = r.o;
	}
	*Lv *= step * length;
	return light->group;
}

bool MultiScattering::Intersect(const Scene &scene, const Sample &sample,
	const Volume *volume, bool scatteredStart, const Ray &ray, float u,
	Intersection *isect, BSDF **bsdf, float *pdf, float *pdfBack,
	SWCSpectrum *L) const
{
	bool hit = scene.Intersect(ray, isect);
	if (hit) {
		if (Dot(ray.d, isect->dg.nn) > 0.f) {
			if (!volume)
				volume = isect->interior;
			else if (!isect->interior)
				isect->interior = volume;
		} else {
			if (!volume)
				volume = isect->exterior;
			else if (!isect->exterior)
				isect->exterior = volume;
		}
	}
	if (volume)
		hit |= volume->Scatter(sample, scatteredStart, ray, u, isect,
			pdf, pdfBack, L);
	else {
		if (pdf)
			*pdf = 1.f;
		if (pdfBack)
			*pdfBack = 1.f;
	}
	if (hit) {
		if (bsdf)
			*bsdf = isect->GetBSDF(sample.arena, sample.swl, ray);
	}
	if (L)
		Transmittance(scene, ray, sample, NULL, L);
	return hit;
}

bool MultiScattering::Intersect(const Scene &scene, const Sample &sample,
	const Volume *volume, bool scatteredStart, const Ray &ray,
	const luxrays::RayHit &rayHit, float u, Intersection *isect,
	BSDF **bsdf, float *pdf, float *pdfBack, SWCSpectrum *L) const
{
	bool hit = scene.Intersect(rayHit, isect);
	if (hit) {
		ray.maxt = rayHit.t;
		if (Dot(ray.d, isect->dg.nn) > 0.f) {
			if (!volume)
				volume = isect->interior;
			else if (!isect->interior)
				isect->interior = volume;
		} else {
			if (!volume)
				volume = isect->exterior;
			else if (!isect->exterior)
				isect->exterior = volume;
		}
	}
	if (volume)
		hit |= volume->Scatter(sample, scatteredStart, ray, u, isect,
			pdf, pdfBack, L);
	else {
		if (pdf)
			*pdf = 1.f;
		if (pdfBack)
			*pdfBack = 1.f;
	}
	if (hit) {
		if (bsdf)
			*bsdf = isect->GetBSDF(sample.arena, sample.swl, ray);
	}
	if (L)
		Transmittance(scene, ray, sample, NULL, L);
	return hit;
}

VolumeIntegrator* MultiScattering::CreateVolumeIntegrator(const ParamSet &params) {
	float stepSize  = params.FindOneFloat("stepsize", 1.f);
	return new MultiScattering(stepSize);
}

static DynamicLoader::RegisterVolumeIntegrator<MultiScattering> r("multi");
