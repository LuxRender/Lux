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

namespace lux
{

// Integrator Method Definitions
Integrator::~Integrator() {
}
// Integrator Utility Functions
SWCSpectrum UniformSampleAllLights(const TsPack *tspack, const Scene *scene,
	const Point &p, const Normal &n, const Vector &wo, BSDF *bsdf,
	const Sample *sample,
	float *lightSample, float *lightNum,
	float *bsdfSample, float *bsdfComponent)
{
	SWCSpectrum L(0.f);
	for (u_int i = 0; i < scene->lights.size(); ++i) {
		L += EstimateDirect(tspack, scene, scene->lights[i], p, n, wo, bsdf,
			sample, lightSample[0], lightSample[1], *lightNum,
			bsdfSample[0], bsdfSample[1], *bsdfComponent);
	}
	return L;
}

int UniformSampleOneLight(const TsPack *tspack, const Scene *scene,
	const Point &p, const Normal &n, const Vector &wo, BSDF *bsdf,
	const Sample *sample,
	float *lightSample, float *lightNum,
	float *bsdfSample, float *bsdfComponent, SWCSpectrum *L)
{
	// Randomly choose a single light to sample, _light_
	int nLights = scene->lights.size();
	if (nLights == 0) {
		*L = 0.f;
		return 0;
	}
	int lightNumber;
	float ls3 = *lightNum * nLights;
	lightNumber = min(Floor2Int(ls3), nLights - 1);
	ls3 -= lightNumber;
	Light *light = scene->lights[lightNumber];
	*L = (float)nLights *
		EstimateDirect(tspack, scene, light, p, n, wo, bsdf,
			sample, lightSample[0], lightSample[1], ls3,
			bsdfSample[0], bsdfSample[1], *bsdfComponent);
	return scene->lights[lightNumber]->group;
}

// Note - Radiance - disabled as this code is broken. (not threadsafe)
/*
SWCSpectrum WeightedSampleOneLight(const TsPack *tspack, const Scene *scene,
	const Point &p, const Normal &n, const Vector &wo, BSDF *bsdf,
	const Sample *sample,
	int lightSampleOffset, int lightNumOffset,
	int bsdfSampleOffset, int bsdfComponentOffset,
	float *&avgY, float *&avgYsample, float *&cdf, float &overallAvgY)
{
	int nLights = int(scene->lights.size());
	// NOTE - lordcrc - Bugfix, pbrt tracker id 0000079: handling NULL parameters and 0 lights for light sampling
	if (nLights == 0)
		return SWCSpectrum(0.f);
	// Initialize _avgY_ array if necessary
	if (!avgY) {
		avgY = new float[nLights];
		avgYsample = new float[nLights];
		cdf = new float[nLights+1];
		for (int i = 0; i < nLights; ++i)
			avgY[i] = avgYsample[i] = 0.;
	}
	float ls1, ls2, ls3, bs1, bs2, bcs, lightNum;
	ls1 = sample->twoD[lightSampleOffset][0];
	ls2 = sample->twoD[lightSampleOffset][1];
	lightNum = sample->oneD[lightNumOffset][0];
	bs1 = sample->twoD[bsdfSampleOffset][0];
	bs2 = sample->twoD[bsdfSampleOffset][1];
	bcs = sample->twoD[bsdfComponentOffset][0];
	SWCSpectrum L(0.);
	if (overallAvgY == 0.) {
		int lightNumber = min(Float2Int(nLights * lightNum), nLights-1);
		ls3 = nLights * lightNum - lightNumber;
		Light *light = scene->lights[lightNumber];
		// Sample one light uniformly and initialize luminance arrays
		L = EstimateDirect(tspack, scene, light, p, n, wo, bsdf,
			sample, ls1, ls2, ls3, bs1, bs2, bcs);
		float luminance = L.y(tspack);
		overallAvgY = luminance;
		for (int i = 0; i < nLights; ++i)
			avgY[i] = luminance;
	}
	else {
		// Choose _light_ according to average reflected luminance
		float c, lightSampleWeight;
		for (int i = 0; i < nLights; ++i)
			avgYsample[i] = max(avgY[i], .1f * overallAvgY);
		ComputeStep1dCDF(avgYsample, nLights, &c, cdf);
		float t = SampleStep1d(avgYsample, cdf, c, nLights,
			lightNum, &lightSampleWeight);
		int lightNumber = min(Float2Int(nLights * t), nLights-1);
		ls3 = nLights * t - lightNumber;
		Light *light = scene->lights[lightNumber];
		L = EstimateDirect(tspack, scene, light, p, n, wo, bsdf,
			sample, ls1, ls2, ls3, bs1, bs2, bcs);
		// Update _avgY_ array with reflected radiance due to light
		float luminance = L.y(tspack);
		avgY[lightNumber] = Lerp(.99f, luminance, avgY[lightNumber]);
		overallAvgY = Lerp(.999f, luminance, overallAvgY);
		L /= lightSampleWeight;
	}
	return L;
}
*/

SWCSpectrum EstimateDirect(const TsPack *tspack, const Scene *scene, const Light *light,
	const Point &p, const Normal &n, const Vector &wo, BSDF *bsdf, const Sample *sample, 
	float &ls1, float &ls2, float &ls3, float &bs1, float &bs2, float &bcs)
{
	SWCSpectrum Ld(0.);

	// Dade - use MIS only if it is worth doing
	BxDFType noDiffuse = BxDFType(BSDF_ALL & ~(BSDF_DIFFUSE));
	if (light->IsDeltaLight() || (bsdf->NumComponents(noDiffuse) == 0)) {

		// Dade - trace only a single shadow ray
		Vector wi;
		float lightPdf;
		VisibilityTester visibility;
		SWCSpectrum Li = light->Sample_L(tspack, p, n,
			ls1, ls2, ls3, &wi, &lightPdf, &visibility);
		if (lightPdf > 0. && !Li.Black()) {
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
		if (lightPdf > 0. && !Li.Black()) {
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
				lightPdf = light->Pdf(p, n, wi);
				if (lightPdf > 0.) {
					// Add light contribution from BSDF sampling
					float weight = PowerHeuristic(1, bsdfPdf, 1, lightPdf);
					Intersection lightIsect;
					Li = SWCSpectrum(1.f);
					RayDifferential ray(p, wi);
					ray.time = tspack->time;
					const BxDFType flags(BxDFType(BSDF_SPECULAR | BSDF_TRANSMISSION));
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
