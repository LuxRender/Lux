/***************************************************************************
 *   Copyright (C) 1998-2007 by authors (see AUTHORS.txt )                 *
 *                                                                         *
 *   This file is part of Lux Renderer.                                    *
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

// transport.cpp*
#include "transport.h"
#include "scene.h"

namespace lux
{

// Integrator Method Definitions
Integrator::~Integrator() {
}
// Integrator Utility Functions
 SWCSpectrum EstimateDirect(const Scene *scene, const Light *light, const Point &p,
	const Normal &n, const Vector &wo, BSDF *bsdf,
	const Sample *sample, float *lightSamp, float *bsdfSamp,
	float *bsdfComponent, u_int sampleNum);
 SWCSpectrum UniformSampleAllLights(const Scene *scene,
		const Point &p, const Normal &n, const Vector &wo,
		BSDF *bsdf, const Sample *sample,
		int *lightSampleOffset, int *bsdfSampleOffset,
		int *bsdfComponentOffset) {
	SWCSpectrum L(0.);
	for (u_int i = 0; i < scene->lights.size(); ++i) {
		Light *light = scene->lights[i];
		int nSamples = (sample && lightSampleOffset) ?
			sample->n2D[lightSampleOffset[i]] : 1;
		// Estimate direct lighting from _light_ samples
		SWCSpectrum Ld(0.);
		// NOTE - lordcrc - Bugfix, pbrt tracker id 0000079: handling NULL parameters and 0 lights for light sampling
		float *curLightSampleOffset = (sample && lightSampleOffset) ? 
			sample->twoD[lightSampleOffset[i]] : NULL;
		float *curBsdfSampleOffset = (sample && lightSampleOffset) ? 
			sample->twoD[lightSampleOffset[i]] : NULL;
		float *curBsdfComponentOffset = (sample && bsdfComponentOffset) ? 
			sample->oneD[bsdfComponentOffset[i]] : NULL;
		for (int j = 0; j < nSamples; ++j)
			Ld += EstimateDirect(scene, light, p, n, wo, bsdf,
				sample, curLightSampleOffset,
				curBsdfSampleOffset, curBsdfComponentOffset, j);
		L += Ld / nSamples;
	}
	return L;
}
 SWCSpectrum UniformSampleOneLight(const Scene *scene,
		const Point &p, const Normal &n,
		const Vector &wo, BSDF *bsdf, const Sample *sample,
		int lightSampleOffset, int lightNumOffset,
		int bsdfSampleOffset, int bsdfComponentOffset) {
	float *lightSample = (sample && lightSampleOffset != -1) ?
		sample->twoD[lightSampleOffset] : NULL;
	float *lightNum = (sample && lightNumOffset != -1) ?
		sample->oneD[lightNumOffset] : NULL;
	float *bsdfSample = (sample && bsdfSampleOffset != -1) ?
		sample->twoD[bsdfSampleOffset] : NULL;
	float *bsdfComponent = (sample && bsdfComponentOffset != -1) ?
		sample->oneD[bsdfComponentOffset] : NULL;
	return UniformSampleOneLight(scene, p, n, wo, bsdf, sample,
		lightSample, lightNum, bsdfSample, bsdfComponent);
}
 SWCSpectrum UniformSampleOneLight(const Scene *scene,
		const Point &p, const Normal &n,
		const Vector &wo, BSDF *bsdf, const Sample *sample,
		float *lightSample, float *lightNum,
		float *bsdfSample, float *bsdfComponent) {
	// Randomly choose a single light to sample, _light_
	int nLights = int(scene->lights.size());
	// NOTE - lordcrc - Bugfix, pbrt tracker id 0000079: handling NULL parameters and 0 lights for light sampling
	if (nLights == 0) 
		return SWCSpectrum(0.f);
	int lightNumber;
	if (lightNum != NULL)
		lightNumber = Floor2Int(*lightNum * nLights);
	else
		lightNumber = Floor2Int(lux::random::floatValue() * nLights);
	lightNumber = min(lightNumber, nLights - 1);
	Light *light = scene->lights[lightNumber];
	return (float)nLights *
		EstimateDirect(scene, light, p, n, wo, bsdf, sample,
			lightSample, bsdfSample,
			bsdfComponent, 0);
}
 SWCSpectrum WeightedSampleOneLight(const Scene *scene,
		const Point &p, const Normal &n,
		const Vector &wo, BSDF *bsdf,
		const Sample *sample, int lightSampleOffset,
		int lightNumOffset, int bsdfSampleOffset,
		int bsdfComponentOffset, float *&avgY,
		float *&avgYsample, float *&cdf,
		float &overallAvgY) {
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
	SWCSpectrum L(0.);
	if (overallAvgY == 0.) {
		// Sample one light uniformly and initialize luminance arrays
		L = UniformSampleOneLight(scene, p, n,
		    wo, bsdf, sample, lightSampleOffset,
			lightNumOffset, bsdfSampleOffset,
			bsdfComponentOffset);
		float luminance = L.y();
		overallAvgY = luminance;
		for (int i = 0; i < nLights; ++i)
			avgY[i] = luminance;
	}
	else {
		float *lightSample = (sample && lightSampleOffset != -1) ?
			sample->twoD[lightSampleOffset] : NULL;
		float *lightNum = (sample && lightNumOffset != -1) ?
			sample->oneD[lightNumOffset] : NULL;
		float *bsdfSample = (sample && bsdfSampleOffset != -1) ?
			sample->twoD[bsdfSampleOffset] : NULL;
		float *bsdfComponent = (sample && bsdfComponentOffset != -1) ?
			sample->oneD[bsdfComponentOffset] : NULL;
		// Choose _light_ according to average reflected luminance
		float c, lightSampleWeight;
		for (int i = 0; i < nLights; ++i)
			avgYsample[i] = max(avgY[i], .1f * overallAvgY);
		ComputeStep1dCDF(avgYsample, nLights, &c, cdf);
		float t = SampleStep1d(avgYsample, cdf, c, nLights,
			lightNum ? *lightNum : lux::random::floatValue(), &lightSampleWeight);
		int lightNumber = min(Float2Int(nLights * t), nLights-1);
		Light *light = scene->lights[lightNumber];
		L = EstimateDirect(scene, light, p, n, wo, bsdf,
			sample, lightSample, bsdfSample,
			bsdfComponent, 0);
		// Update _avgY_ array with reflected radiance due to light
		float luminance = L.y();
		avgY[lightNumber] =
			ExponentialAverage(avgY[lightNumber], luminance, .99f);
		overallAvgY =
			ExponentialAverage(overallAvgY, luminance, .999f);
		L /= lightSampleWeight;
	}
	return L;
}
SWCSpectrum EstimateDirect(const Scene *scene,
        const Light *light, const Point &p,
		const Normal &n, const Vector &wo,
		BSDF *bsdf, const Sample *sample, float *lightSamp,
		float *bsdfSamp, float *bsdfComponent, u_int sampleNum) {
	SWCSpectrum Ld(0.);
	// Find light and BSDF sample values for direct lighting estimate
	float ls1, ls2, bs1, bs2, bcs;
	// NOTE - lordcrc - Bugfix, pbrt tracker id 0000079: handling NULL parameters and 0 lights for light sampling
	if (lightSamp != NULL && bsdfSamp != NULL && bsdfComponent != NULL /*&&
		sampleNum < sample->n2D[lightSamp] &&
		sampleNum < sample->n2D[bsdfSamp]*/) {
		ls1 = lightSamp/*sample->twoD[lightSamp]*/[2*sampleNum];
		ls2 = lightSamp/*sample->twoD[lightSamp]*/[2*sampleNum+1];
		bs1 = bsdfSamp/*sample->twoD[bsdfSamp]*/[2*sampleNum];
		bs2 = bsdfSamp/*sample->twoD[bsdfSamp]*/[2*sampleNum+1];
		bcs = bsdfComponent/*sample->oneD[bsdfComponent]*/[sampleNum];
	}
	else {
		ls1 = lux::random::floatValue();
		ls2 = lux::random::floatValue();
		bs1 = lux::random::floatValue();
		bs2 = lux::random::floatValue();
		bcs = lux::random::floatValue();
	} 
	// Sample light source with multiple importance sampling
	Vector wi;
	float lightPdf, bsdfPdf;
	VisibilityTester visibility;
	SWCSpectrum Li = light->Sample_L(p, n,
		ls1, ls2, &wi, &lightPdf, &visibility);
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

