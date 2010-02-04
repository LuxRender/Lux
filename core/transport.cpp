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

// transport.cpp*
#include "transport.h"
#include "scene.h"
#include "bxdf.h"
#include "light.h"
#include "mc.h"
#include "volume.h"

namespace lux
{

// Integrator Method Definitions
bool VolumeIntegrator::Intersect(const TsPack *tspack, const Scene *scene,
	const Volume *volume, const Ray &ray, Intersection *isect,
	SWCSpectrum *L) const
{
	const bool result = scene->Intersect(ray, isect);
	if (volume)
		*L *= Exp(-volume->Tau(tspack, ray));
	return result;
}

// Integrator Utility Functions
SWCSpectrum UniformSampleAllLights(const TsPack *tspack, const Scene *scene,
	const Point &p, const Normal &n, const Vector &wo, BSDF *bsdf,
	const Sample *sample,
	const float *lightSample, const float *lightNum,
	const float *bsdfSample, const float *bsdfComponent)
{
	SWCSpectrum L(0.f);
	for (u_int i = 0; i < scene->lights.size(); ++i) {
		L += EstimateDirect(tspack, scene, scene->lights[i], p, n, wo, bsdf,
			sample, lightSample[0], lightSample[1], *lightNum,
			bsdfSample[0], bsdfSample[1], *bsdfComponent);
	}
	return L;
}

u_int UniformSampleOneLight(const TsPack *tspack, const Scene *scene,
	const Point &p, const Normal &n, const Vector &wo, BSDF *bsdf,
	const Sample *sample,
	const float *lightSample, const float *lightNum,
	const float *bsdfSample, const float *bsdfComponent, SWCSpectrum *L)
{
	// Randomly choose a single light to sample, _light_
	u_int nLights = scene->lights.size();
	if (nLights == 0) {
		*L = 0.f;
		return 0;
	}
	float ls3 = *lightNum * nLights;
	const u_int lightNumber = min(Floor2UInt(ls3), nLights - 1);
	ls3 -= lightNumber;
	Light *light = scene->lights[lightNumber];
	*L = static_cast<float>(nLights) * EstimateDirect(tspack, scene, light,
		p, n, wo, bsdf, sample, lightSample[0], lightSample[1], ls3,
		bsdfSample[0], bsdfSample[1], *bsdfComponent);
	return scene->lights[lightNumber]->group;
}

SWCSpectrum EstimateDirect(const TsPack *tspack, const Scene *scene, const Light *light,
	const Point &p, const Normal &n, const Vector &wo, BSDF *bsdf, const Sample *sample, 
	float ls1, float ls2, float ls3, float bs1, float bs2, float bcs)
{
	SWCSpectrum Ld(0.f);

	// Dade - use MIS only if it is worth doing
	BxDFType noDiffuse = BxDFType(BSDF_ALL & ~(BSDF_DIFFUSE));
	if (light->IsDeltaLight() || (bsdf->NumComponents(noDiffuse) == 0)) {

		// Dade - trace only a single shadow ray
		Vector wi;
		float lightPdf;
		VisibilityTester visibility;
		SWCSpectrum Li = light->Sample_L(tspack, p, n,
			ls1, ls2, ls3, &wi, &lightPdf, &visibility);
		if (lightPdf > 0.f && !Li.Black()) {
			SWCSpectrum f = bsdf->f(tspack, wi, wo);
			SWCSpectrum fO(1.f);
			if (!f.Black() && visibility.TestOcclusion(tspack, scene, &fO)) {
				// Add light's contribution to reflected radiance
				visibility.Transmittance(tspack, scene, sample, &Li);
				Li *= fO;
				Ld += f * Li * (AbsDot(wi, n) / lightPdf);
			}
		}
	} else {
		// Dade - trace 2 shadow rays and use MIS
		// Sample light source with multiple importance sampling
		const BxDFType noSpecular = BxDFType(BSDF_ALL & ~BSDF_SPECULAR);
		Vector wi;
		float lightPdf, bsdfPdf;
		VisibilityTester visibility;
		SWCSpectrum Li = light->Sample_L(tspack, p, n,
			ls1, ls2, ls3, &wi, &lightPdf, &visibility);
		if (lightPdf > 0.f && !Li.Black()) {
			SWCSpectrum f = bsdf->f(tspack, wi, wo, noSpecular);
			SWCSpectrum fO(1.f);
			if (!f.Black() && visibility.TestOcclusion(tspack, scene, &fO)) {
				// Add light's contribution to reflected radiance
				visibility.Transmittance(tspack, scene, sample, &Li);
				Li *= fO;

				bsdfPdf = bsdf->Pdf(tspack, wi, wo, noSpecular);
				float weight = PowerHeuristic(1, lightPdf, 1, bsdfPdf);
				Ld += f * Li * (AbsDot(wi, n) * weight / lightPdf);
			}

			// Sample BSDF with multiple importance sampling
			SWCSpectrum fBSDF;
			if (bsdf->Sample_f(tspack, wo, &wi,	bs1, bs2, bcs, &fBSDF, &bsdfPdf, noSpecular, NULL, NULL, true)) {
				lightPdf = light->Pdf(tspack, p, n, wi);
				if (lightPdf > 0.) {
					// Add light contribution from BSDF sampling
					float weight = PowerHeuristic(1, bsdfPdf, 1, lightPdf);
					Intersection lightIsect;
					Li = SWCSpectrum(1.f);
					RayDifferential ray(p, wi);
					ray.time = tspack->time;
					const BxDFType flags(BxDFType(BSDF_SPECULAR | BSDF_TRANSMISSION));
					// The for loop prevents an infinite
					// loop when the ray is almost parallel
					// to the surface
					// It should much less frequent with
					// dynamic epsilon, but it's safer
					for (u_int i = 0; i < 10000; ++i) {
						if (!scene->Intersect(ray, &lightIsect)) {
							Li *= light->Le(tspack, ray);
							break;
						} else if (lightIsect.arealight == light) {
							Li *= lightIsect.Le(tspack, -wi);
							break;
						}
						BSDF *ibsdf = lightIsect.GetBSDF(tspack, ray);

						Li *= ibsdf->f(tspack, wi, -wi, flags);
						if (Li.Black())
							break;
						Li *= AbsDot(ibsdf->dgShading.nn, wi);

						ray.mint = ray.maxt + MachineEpsilon::E(ray.maxt);
						ray.maxt = INFINITY;
					}
					if (!Li.Black()) {
						scene->Transmittance(tspack, ray, sample, &Li);
						Ld += fBSDF * Li * (AbsDot(wi, n) * weight / bsdfPdf);
					}
				}
			}
		}
	}

	return Ld;
}

}//namespace lux
