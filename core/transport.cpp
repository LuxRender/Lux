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
static SWCSpectrum EstimateDirect(const Scene *scene, const Light *light,
	const Point &p, const Normal &n, const Vector &wo, BSDF *bsdf,
	float ls1, float ls2, float ls3, float bs1, float bs2, float bcs);

SWCSpectrum UniformSampleAllLights(const Scene *scene,
	const Point &p, const Normal &n, const Vector &wo, BSDF *bsdf,
	const Sample *sample,
	int *lightSampleOffset, int *bsdfSampleOffset, int *bsdfComponentOffset)
{
	SWCSpectrum L(0.);
	for (u_int i = 0; i < scene->lights.size(); ++i) {
		Light *light = scene->lights[i];
		int nSamples = (sample && lightSampleOffset) ?
			sample->n2D[lightSampleOffset[i]] : 1;
		// Estimate direct lighting from _light_ samples
		SWCSpectrum Ld(0.);
		for (int j = 0; j < nSamples; ++j) {
			float ls1, ls2, ls3, bs1, bs2, bcs;
			if (sample && lightSampleOffset) {
				ls1 = sample->twoD[lightSampleOffset[i]][2 * j];
				ls2 = sample->twoD[lightSampleOffset[i]][2 * j + 1];
			} else {
				ls1 = lux::random::floatValue();
				ls2 = lux::random::floatValue();
			}
			ls3 = lux::random::floatValue(); //FIXME: use sample
			if (sample && bsdfSampleOffset) {
				bs1 = sample->twoD[bsdfSampleOffset[i]][2 * j];
				bs2 = sample->twoD[bsdfSampleOffset[i]][2 * j + 1];
			} else {
				bs1 = lux::random::floatValue();
				bs2 = lux::random::floatValue();
			}
			if (sample && bsdfComponentOffset)
				bcs = sample->twoD[bsdfComponentOffset[i]][j];
			else
				bcs = lux::random::floatValue();
			Ld += EstimateDirect(scene, light, p, n, wo, bsdf,
				ls1, ls2, ls3, bs1, bs2, bcs);
		}
		L += Ld / nSamples;
	}
	return L;
}
SWCSpectrum UniformSampleOneLight(const Scene *scene,
	const Point &p, const Normal &n, const Vector &wo, BSDF *bsdf,
	const Sample *sample,
	float *lightSample, float *lightNum,
	float *bsdfSample, float *bsdfComponent)
{
	// Randomly choose a single light to sample, _light_
	int nLights = int(scene->lights.size());
	// NOTE - lordcrc - Bugfix, pbrt tracker id 0000079: handling NULL parameters and 0 lights for light sampling
	if (nLights == 0) 
		return SWCSpectrum(0.f);
	int lightNumber;
	float ls1, ls2, ls3, bs1, bs2, bcs;
	if (lightNum)
		ls3 = *lightNum * nLights;
	else
		ls3 = lux::random::floatValue() * nLights;
	lightNumber = min(Floor2Int(ls3), nLights - 1);
	ls3 -= lightNumber;
	Light *light = scene->lights[lightNumber];
	if (lightSample) {
		ls1 = lightSample[0];
		ls2 = lightSample[1];
	} else {
		ls1 = lux::random::floatValue();
		ls2 = lux::random::floatValue();
	}
	if (bsdfSample) {
		bs1 = bsdfSample[0];
		bs2 = bsdfSample[1];
	} else {
		bs1 = lux::random::floatValue();
		bs2 = lux::random::floatValue();
	}
	if (bsdfComponent)
		bcs = *bsdfComponent;
	else
		bcs = lux::random::floatValue();
	return (float)nLights *
		EstimateDirect(scene, light, p, n, wo, bsdf,
			ls1, ls2, ls3, bs1, bs2, bcs);
}
SWCSpectrum WeightedSampleOneLight(const Scene *scene,
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
	if (sample && lightSampleOffset!= -1) {
		ls1 = sample->twoD[lightSampleOffset][0];
		ls2 = sample->twoD[lightSampleOffset][1];
	} else {
		ls1 = lux::random::floatValue();
		ls2 = lux::random::floatValue();
	}
	if (sample && lightNumOffset != -1)
		lightNum = sample->oneD[lightNumOffset][0];
	else
		lightNum = lux::random::floatValue();
	if (sample && bsdfSampleOffset != -1) {
		bs1 = sample->twoD[bsdfSampleOffset][0];
		bs2 = sample->twoD[bsdfSampleOffset][1];
	} else {
		bs1 = lux::random::floatValue();
		bs2 = lux::random::floatValue();
	}
	if (sample && bsdfComponentOffset != -1)
		bcs = sample->twoD[bsdfComponentOffset][0];
	else
		bcs = lux::random::floatValue();
	SWCSpectrum L(0.);
	if (overallAvgY == 0.) {
		int lightNumber = min(Float2Int(nLights * lightNum), nLights-1);
		ls3 = nLights * lightNum - lightNumber;
		Light *light = scene->lights[lightNumber];
		// Sample one light uniformly and initialize luminance arrays
		L = EstimateDirect(scene, light, p, n, wo, bsdf,
			ls1, ls2, ls3, bs1, bs2, bcs);
		float luminance = L.y();
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
		L = EstimateDirect(scene, light, p, n, wo, bsdf,
			ls1, ls2, ls3, bs1, bs2, bcs);
		// Update _avgY_ array with reflected radiance due to light
		float luminance = L.y();
		avgY[lightNumber] = Lerp(.99f, luminance, avgY[lightNumber]);
		overallAvgY = Lerp(.999f, luminance, overallAvgY);
		L /= lightSampleWeight;
	}
	return L;
}
static SWCSpectrum EstimateDirect(const Scene *scene, const Light *light,
	const Point &p, const Normal &n, const Vector &wo, BSDF *bsdf,
	float ls1, float ls2, float ls3, float bs1, float bs2, float bcs)
{
	SWCSpectrum Ld(0.);
	// Sample light source with multiple importance sampling
	Vector wi;
	float lightPdf, bsdfPdf;
	VisibilityTester visibility;
	SWCSpectrum Li = light->Sample_L(p, n,
		ls1, ls2, ls3, &wi, &lightPdf, &visibility);
	if (lightPdf > 0. && !Li.Black()) {
		SWCSpectrum f = bsdf->f(wo, wi);
		if (!f.Black() && visibility.Unoccluded(scene)) {
			// Add light's contribution to reflected radiance
			Li *= visibility.Transmittance(scene);
			if (light->IsDeltaLight())
				Ld += f * Li * AbsDot(wi, n) / lightPdf;
			else {
				bsdfPdf = bsdf->Pdf(wo, wi);
				float weight = PowerHeuristic(1, lightPdf, 1, bsdfPdf);
				Ld += f * Li * AbsDot(wi, n) * weight / lightPdf;
			}
		}
	}
	
	// Sample BSDF with multiple importance sampling
	if (!light->IsDeltaLight()) {
		BxDFType flags = BxDFType(BSDF_ALL & ~BSDF_SPECULAR);
		SWCSpectrum f = bsdf->Sample_f(wo, &wi,
			bs1, bs2, bcs, &bsdfPdf, flags);
		if (!f.Black() && bsdfPdf > 0.) {
			lightPdf = light->Pdf(p, n, wi);
			if (lightPdf > 0.) {
				// Add light contribution from BSDF sampling
				float weight = PowerHeuristic(1, bsdfPdf, 1, lightPdf);
				Intersection lightIsect;
				SWCSpectrum Li(0.f);
				RayDifferential ray(p, wi);
				if (scene->Intersect(ray, &lightIsect)) {
					if (lightIsect.primitive->GetAreaLight() == light)
						Li = lightIsect.Le(-wi);
				}
				else
					Li = light->Le(ray);
				if (!Li.Black()) {
					Li *= scene->Transmittance(ray);
					Ld += f * Li * AbsDot(wi, n) * weight / bsdfPdf;
				}
			}
		}
	}
	
	return Ld;
}

}//namespace lux

