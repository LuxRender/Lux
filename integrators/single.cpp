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

// single.cpp*
#include "single.h"
#include "randomgen.h"
#include "light.h"
#include "bxdf.h"
#include "sampling.h"
#include "paramset.h"
#include "dynload.h"

using namespace lux;

// SingleScattering Method Definitions
void SingleScattering::RequestSamples(Sample *sample, const Scene &scene)
{
	tauSampleOffset = sample->Add1D(1);
	scatterSampleOffset = sample->Add1D(1);
}

void SingleScattering::Transmittance(const Scene &scene, const Ray &ray,
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

u_int SingleScattering::Li(const Scene &scene, const Ray &ray,
	const Sample &sample, SWCSpectrum *Lv, float *alpha) const
{
	*Lv = 0.f;
	Region *vr = scene.volumeRegion;
	float t0, t1;
	if (!vr || !vr->IntersectP(ray, &t0, &t1))
		return 0;
	// Do single scattering volume integration in _vr_
	// Prepare for volume integration stepping
	const u_int N = Ceil2UInt((t1 - t0) / stepSize);
	const float step = (t1 - t0) / N;
	const SpectrumWavelengths &sw(sample.swl);
	SWCSpectrum Tr(1.f);
	Vector w = -ray.d;
	t0 += sample.oneD[tauSampleOffset][0] * step;
	Ray r(ray(t0), ray.d * (step / ray.d.Length()), 0.f, 1.f);
	const u_int nLights = scene.lights.size();
	const u_int lightNum = min(nLights - 1,
		Floor2UInt(sample.oneD[scatterSampleOffset][0] * nLights));
	Light *light = scene.lights[lightNum];

	// Compute sample patterns for single scattering samples
	// FIXME - use real samples
	float *samp = static_cast<float *>(alloca(3 * N * sizeof(float)));
	LatinHypercube(*(sample.rng), samp, N, 3);
	u_int sampOffset = 0;
	for (u_int i = 0; i < N; ++i, t0 += step) {
		// Advance to sample at _t0_ and update _T_
		r.o = ray(t0);

		// Ray is already offset above, no need to do it again
		const SWCSpectrum stepTau(vr->Tau(sw, r, .5f * stepSize, 0.f));
		Tr *= Exp(-stepTau);
		// Possibly terminate raymarching if transmittance is small
		if (Tr.Filter(sw) < 1e-3f) {
			const float continueProb = .5f;
			if (sample.rng->floatValue() > continueProb) break; // TODO - REFACT - remove and add random value from sample
			Tr /= continueProb;
		}

		// Compute single-scattering source term at _p_
		*Lv += Tr * vr->Lve(sw, r.o, w);

		if (scene.lights.size() == 0)
			continue;
		const SWCSpectrum ss(vr->SigmaS(sw, r.o, w));
		if (!ss.Black()) {
			// Add contribution of _light_ due to scattering at _p_
			float pdf;
			Vector wo;
			float u1 = samp[sampOffset], u2 = samp[sampOffset + 1],
				u3 = samp[sampOffset + 2];
			BSDF *ibsdf;
			SWCSpectrum L;
			if (light->Sample_L(scene, sample, r.o, u1, u2, u3,
				&ibsdf, NULL, &pdf, &L)) {
				if (Connect(scene, sample, NULL, true, false,
					r.o, ibsdf->dgShading.p, false, &L,
					NULL, NULL))
					*Lv += Tr * ss * L *
						(vr->P(sw, r.o, w, -wo) *
						 nLights / pdf);
			}
		}
		sampOffset += 3;
	}
	*Lv *= step;
	return light->group;
}

VolumeIntegrator* SingleScattering::CreateVolumeIntegrator(const ParamSet &params) {
	float stepSize  = params.FindOneFloat("stepsize", 1.f);
	return new SingleScattering(stepSize);
}

static DynamicLoader::RegisterVolumeIntegrator<SingleScattering> r("single");
