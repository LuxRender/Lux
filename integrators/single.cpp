/***************************************************************************
 *   Copyright (C) 1998-2008 by authors (see AUTHORS.txt )                 *
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
#include "light.h"
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
		const Ray &ray, const Sample *sample, float *alpha, SWCSpectrum *const L) const {
	if (!scene->volumeRegion) 
		return;
	//float step = sample ? stepSize : 4.f * stepSize;
	float step = stepSize; // TODO - handle varying step size
	float offset = sample->oneD[tauSampleOffset][0];
	SWCSpectrum tau = SWCSpectrum(tspack, scene->volumeRegion->Tau(ray, step, offset));
	*L *= Exp(-tau);
}

int SingleScattering::Li(const TsPack *tspack, const Scene *scene,
		const RayDifferential &ray, const Sample *sample,
		SWCSpectrum *Lv, float *alpha) const {
	VolumeRegion *vr = scene->volumeRegion;
	float t0, t1;
	if (!vr || !vr->IntersectP(ray, &t0, &t1)) return 0.f;
	// Do single scattering volume integration in _vr_
	*Lv = 0.f;
	// Prepare for volume integration stepping
	int N = Ceil2Int((t1-t0) / stepSize);
	float step = (t1 - t0) / N;
	SWCSpectrum Tr(1.f);
	Point p = ray(t0), pPrev;
	Vector w = -ray.d;
	t0 += sample->oneD[scatterSampleOffset][0] * step;
	int nLights = scene->lights.size();
	int lightNum = min(Floor2Int(tspack->rng->floatValue() * nLights), nLights-1); //TODO - REFACT - remove and add random value from sample
	Light *light = scene->lights[lightNum];

	// Compute sample patterns for single scattering samples
	float *samp = (float *)alloca(3 * N * sizeof(float));
	LatinHypercube(tspack, samp, N, 3);
	int sampOffset = 0;
	for (int i = 0; i < N; ++i, t0 += step) {
		// Advance to sample at _t0_ and update _T_
		pPrev = p;
		p = ray(t0);

		SWCSpectrum stepTau = SWCSpectrum(tspack, vr->Tau(Ray(pPrev, p - pPrev, 0, 1),
			.5f * stepSize, tspack->rng->floatValue())); // TODO - REFACT - remove and add random value from sample
		Tr *= Exp(-stepTau);
		// Possibly terminate raymarching if transmittance is small
		if (Tr.filter(tspack) < 1e-3) {
			const float continueProb = .5f;
			if (tspack->rng->floatValue() > continueProb) break; // TODO - REFACT - remove and add random value from sample
			Tr /= continueProb;
		}

		// Compute single-scattering source term at _p_
		*Lv += Tr * SWCSpectrum(tspack, vr->Lve(p, w));

		SWCSpectrum ss = SWCSpectrum(tspack, vr->sigma_s(p, w));
		if (!ss.Black() && scene->lights.size() > 0) {
			// Add contribution of _light_ due to scattering at _p_
			float pdf;
			VisibilityTester vis;
			Vector wo;
			float u1 = samp[sampOffset], u2 = samp[sampOffset+1], u3 = samp[sampOffset+2];
			SWCSpectrum L = light->Sample_L(tspack, p, u1, u2, u3, &wo, &pdf, &vis);

			// Dade - use the new TestOcclusion() method
			SWCSpectrum occlusion;
			if ((!L.Black()) && (pdf > 0.0f) && vis.TestOcclusion(tspack, scene, &occlusion)) {	
				SWCSpectrum Ld = L * occlusion;
				vis.Transmittance(tspack, scene, sample, &Ld);
				*Lv += Tr * ss * vr->p(p, w, -wo) *
					  Ld * float(nLights) / pdf;
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
