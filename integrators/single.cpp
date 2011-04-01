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
#include "sampling.h"
#include "paramset.h"
#include "dynload.h"

using namespace lux;

// SingleScattering Method Definitions
void SingleScattering::RequestSamples(Sample *sample,
		const Scene *scene) {
	tauSampleOffset = sample->Add1D(1);
	scatterSampleOffset = sample->Add1D(1);
}

void SingleScattering::Transmittance(const TsPack *tspack, const Scene *scene,
	const Ray &ray, const Sample *sample,
	float *alpha, SWCSpectrum *const L) const
{
	if (!scene->volumeRegion) 
		return;
	const float step = stepSize; // TODO - handle varying step size
	const float offset = sample->oneD[tauSampleOffset][0];
	const SWCSpectrum tau(scene->volumeRegion->Tau(tspack, ray, step,
		offset));
	*L *= Exp(-tau);
}

u_int SingleScattering::Li(const TsPack *tspack, const Scene *scene,
	const RayDifferential &ray, const Sample *sample,
	SWCSpectrum *Lv, float *alpha) const
{
	*Lv = 0.f;
	Region *vr = scene->volumeRegion;
	float t0, t1;
	if (!vr || !vr->IntersectP(ray, &t0, &t1))
		return 0;
	// Do single scattering volume integration in _vr_
	// Prepare for volume integration stepping
	const u_int N = Ceil2UInt((t1 - t0) / stepSize);
	const float step = (t1 - t0) / N;
	SWCSpectrum Tr(1.f);
	Vector w = -ray.d;
	t0 += sample->oneD[tauSampleOffset][0] * step;
	Ray r(ray(t0), ray.d * (step / ray.d.Length()), 0.f, 1.f);
	const u_int nLights = scene->lights.size();
	const u_int lightNum = min(nLights - 1,
		Floor2UInt(sample->oneD[scatterSampleOffset][0] * nLights));
	Light *light = scene->lights[lightNum];

	// Compute sample patterns for single scattering samples
	// FIXME - use real samples
	float *samp = static_cast<float *>(alloca(3 * N * sizeof(float)));
	LatinHypercube(tspack, samp, N, 3);
	u_int sampOffset = 0;
	for (u_int i = 0; i < N; ++i, t0 += step) {
		// Advance to sample at _t0_ and update _T_
		r.o = ray(t0);

		// Ray is already offset above, no need to do it again
		const SWCSpectrum stepTau(vr->Tau(tspack, r,
			.5f * stepSize, 0.f));
		Tr *= Exp(-stepTau);
		// Possibly terminate raymarching if transmittance is small
		if (Tr.Filter(tspack) < 1e-3f) {
			const float continueProb = .5f;
			if (tspack->rng->floatValue() > continueProb) break; // TODO - REFACT - remove and add random value from sample
			Tr /= continueProb;
		}

		// Compute single-scattering source term at _p_
		*Lv += Tr * vr->Lve(tspack, r.o, w);

		if (scene->lights.size() == 0)
			continue;
		const SWCSpectrum ss(vr->SigmaS(tspack, r.o, w));
		if (!ss.Black()) {
			// Add contribution of _light_ due to scattering at _p_
			float pdf;
			VisibilityTester vis;
			Vector wo;
			float u1 = samp[sampOffset], u2 = samp[sampOffset + 1],
				u3 = samp[sampOffset + 2];
			const SWCSpectrum L(light->Sample_L(tspack, r.o, u1, u2,
				u3, &wo, &pdf, &vis));

			// Dade - use the new TestOcclusion() method
			SWCSpectrum occlusion(1.f);
			if ((!L.Black()) && (pdf > 0.0f) &&
				vis.TestOcclusion(tspack, scene, &occlusion)) {	
				SWCSpectrum Ld = L * occlusion;
				vis.Transmittance(tspack, scene, sample, &Ld);
				*Lv += Tr * ss * Ld *
					(vr->P(tspack, r.o, w, -wo) * nLights /
					pdf);
			}
		}
		sampOffset += 3;
	}
	*Lv *= step;
	return light->group;
}

u_int SingleScattering::Li(const TsPack *tspack, const Scene *scene, 
	const RayDifferential &ray, const Sample *sample, 
	SWCSpectrum *Lv, float *alpha, bool from_IsSup, bool path_type) const
{
	if ( !from_IsSup ) 
		return Li(tspack, scene, ray, sample, Lv, alpha);
	else {	*Lv = 0.f; return false; }

}

VolumeIntegrator* SingleScattering::CreateVolumeIntegrator(const ParamSet &params) {
	float stepSize  = params.FindOneFloat("stepsize", 1.f);
	return new SingleScattering(stepSize);
}

static DynamicLoader::RegisterVolumeIntegrator<SingleScattering> r("single");
