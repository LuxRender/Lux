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
#include "none.h"
#include "randomgen.h"
#include "light.h"
#include "bxdf.h"
#include "sampling.h"
#include "paramset.h"
#include "dynload.h"

using namespace lux;

// NoneScattering Method Definitions
void NoneScattering::RequestSamples(Sampler *sampler, const Scene &scene) {
}

void NoneScattering::Transmittance(const Scene &scene, const Ray &ray,
	const Sample &sample, float *alpha, SWCSpectrum *const L) const {
}

u_int NoneScattering::Li(const Scene &scene, const Ray &ray,
	const Sample &sample, SWCSpectrum *Lv, float *alpha) const {
	*Lv = 0.f;
	return 0;
}

bool NoneScattering::Intersect(const Scene &scene, const Sample &sample,
	const Volume *volume, bool scatteredStart, const Ray &ray, float u,
	Intersection *isect, BSDF **bsdf, float *pdf, float *pdfBack,
	SWCSpectrum *L) const {
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

	if (volume && L) {
		SWCSpectrum tau = -volume->Tau(sample.swl, ray);
		if (!tau.Black()) // Exp is _extremely_ slow
			*L *= Exp(tau);
	}
	if (pdf)
		*pdf = 1.f;
	if (pdfBack)
		*pdfBack = 1.f;

	if (hit) {
		if (bsdf)
			*bsdf = isect->GetBSDF(sample.arena, sample.swl, ray);
	}

	return hit;
}

bool NoneScattering::Intersect(const Scene &scene, const Sample &sample,
	const Volume *volume, bool scatteredStart, const Ray &ray,
	const luxrays::RayHit &rayHit, float u, Intersection *isect,
	BSDF **bsdf, float *pdf, float *pdfBack, SWCSpectrum *L) const {
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

	if (volume && L) {
		SWCSpectrum tau = -volume->Tau(sample.swl, ray);
		if (!tau.Black()) // Exp is _extremely_ slow
			*L *= Exp(tau);
	}
	if (pdf)
		*pdf = 1.f;
	if (pdfBack)
		*pdfBack = 1.f;

	if (hit) {
		if (bsdf)
			*bsdf = isect->GetBSDF(sample.arena, sample.swl, ray);
	}

	return hit;
}

VolumeIntegrator* NoneScattering::CreateVolumeIntegrator(const ParamSet &params) {
	return new NoneScattering();
}

static DynamicLoader::RegisterVolumeIntegrator<NoneScattering> r("none");
