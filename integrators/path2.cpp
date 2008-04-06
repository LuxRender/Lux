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

// path2.cpp*
#include "path2.h"
#include "bxdf.h"
#include "light.h"
#include "camera.h"
#include "paramset.h"

using namespace lux;

// Path2Integrator Method Definitions
void Path2Integrator::RequestSamples(Sample *sample, const Scene *scene)
{
	vector<u_int> structure;
	structure.push_back(2);
	structure.push_back(1);
	structure.push_back(2);
	structure.push_back(1);
	structure.push_back(1);
	structure.push_back(2);
	structure.push_back(1);
	sampleOffset = sample->AddxD(structure, maxDepth + 1);
}

SWCSpectrum Path2Integrator::Li(const Scene *scene,
		const RayDifferential &r, const Sample *sample,
		float *alpha) const
{
	SampleGuard guard(sample->sampler, sample);
	// Declare common path integration variables
	RayDifferential ray(r);
	Point lenP;
	float lenPdf;
//	SWCSpectrum pathThroughput = 1., L = 0.;
	vector<SWCSpectrum> pathThroughput(maxDepth + 1), L(maxDepth + 1);
	vector<float> imageX(maxDepth + 1), imageY(maxDepth  + 1);
	vector<float> weight(maxDepth + 1);
	pathThroughput[0] = scene->volumeIntegrator->Transmittance(scene, ray, sample, alpha);
	L[0] = scene->volumeIntegrator->Li(scene, ray, sample, alpha);
	imageX[0] = sample->imageX;
	imageY[0] = sample->imageY;
	scene->camera->SamplePosition(sample->lensU, sample->lensV, &lenP, &lenPdf);
	RayDifferential ray_gen;
	Sample &sample_gen = const_cast<Sample &>(*sample);
	bool specularBounce = true, specular0 = false;
	if (alpha) *alpha = 1.;
	int pathLength;
	float totalWeight = 1.f;
	for (pathLength = 0; ; ++pathLength) {
		// Find next vertex of path
		Intersection isect;
		if (!scene->Intersect(ray, &isect)) {
			// Stop path sampling since no intersection was found
			// Possibly add emitted light
			// NOTE - Added by radiance - adds horizon in render & reflections
			if (specularBounce) {
				SWCSpectrum Le(0.f);
				for (u_int i = 0; i < scene->lights.size(); ++i)
					Le += scene->lights[i]->Le(ray);
				Le *= pathThroughput[0];
				L[0] += Le;
			}
			// Set alpha channel
			if (pathLength == 0 && alpha && L[0].Black())
				*alpha = 0.;
			break;
		}
		if (pathLength == 0) {
			r.maxt = ray.maxt;
		} else {
//			pathThroughput *= scene->Transmittance(ray);
			SWCSpectrum T(scene->Transmittance(ray));
			for (int i = 0; i < pathLength; ++i) {
				pathThroughput[i] *= T;
			}
		}
		// Possibly add emitted light at path vertex
		Vector wo = -ray.d;
		if (specularBounce)
			L[0] += pathThroughput[0] * isect.Le(wo);
		if (pathLength == maxDepth)
			break;
		// Evaluate BSDF at hit point
		BSDF *bsdf = isect.GetBSDF(ray);
		// Sample illumination from lights to find path contribution
		const Point &p = bsdf->dgShading.p;
		const Normal &n = bsdf->dgShading.nn;
		float *data = sample->sampler->GetLazyValues(const_cast<Sample *>(sample), sampleOffset, pathLength);
		SWCSpectrum Ll = UniformSampleOneLight(scene, p, n,
			wo, bsdf, sample,
			data, data + 2, data + 3, data + 5);
		L[0] += pathThroughput[0] * Ll;
		for (int i = 1; i < pathLength; ++i)
			L[i] += pathThroughput[i] * Ll;

		// Possibly terminate the path
		if (pathLength > 3) {
			if (data[6] > continueProbability)
				break;

			// increase path contribution
			for (int i = 0; i < pathLength; ++i)
				pathThroughput[i] /= continueProbability;
		}
		// Sample BSDF to get new path direction
		Vector wi;
		float pdf;
		BxDFType flags;
		SWCSpectrum f = bsdf->Sample_f(wo, &wi, data[7], data[8], data[9],
			&pdf, BSDF_ALL, &flags);
		if (pdf == .0f || f.Black())
			break;
		specularBounce = (flags & BSDF_SPECULAR) != 0;
		float cos = AbsDot(wi, n);
		float factor = cos / pdf;
		pathThroughput[0] *= f;
		pathThroughput[0] *= factor;
		for (int i = 1; i < pathLength; ++i) {
			pathThroughput[i] *= f;
			pathThroughput[i] *= factor;
		}
		if (pathLength == 0) {
			weight[0] = scene->camera->GetConnectingFactor(lenP, p, wo, n);
			specular0 = specularBounce;
			if (specular0) {
				totalWeight = 0.f;
				weight[0] = 1.f;
			}
		}
		if (pathLength > 0) {
			if (scene->camera->IsVisibleFromEyes(scene, lenP, p, &sample_gen, &ray_gen)) {
				wo = -ray_gen.d;
				bsdf = isect.GetBSDF(ray_gen);
				weight[pathLength] = scene->camera->GetConnectingFactor(lenP, bsdf->dgShading.p, wo, bsdf->dgShading.nn);
				pathThroughput[pathLength] = scene->Transmittance(ray_gen) * (weight[pathLength] / weight[0]);
				imageX[pathLength] = sample_gen.imageX;
				imageY[pathLength] = sample_gen.imageY;
//				L[pathLength] = pathThroughput[pathLength] * isect.Le(wo);
				L[pathLength] += pathThroughput[pathLength] *
					UniformSampleOneLight(scene, bsdf->dgShading.p, bsdf->dgShading.nn,
						wo, bsdf, sample,
						data, data + 2, data + 3, data + 5);
				pdf = bsdf->Pdf(wo, wi);
				if (pdf > 0.f) {
					f = bsdf->f(wo, wi);
					if (!f.Black())
						pathThroughput[pathLength] *= f * (cos / pdf);
					else {
						pathThroughput[pathLength] = 0.f;
						weight[pathLength] = 0.f;
					}
				} else {
					pathThroughput[pathLength] = 0.f;
					weight[pathLength] = 0.f;
				}
			}
			totalWeight += weight[pathLength]  / weight[0];
		}
/*		for (int i = 1; i <= pathLength; ++i)
			pathThroughput[i] *= scene->camera->film->xResolution * scene->camera->film->yResolution;*/

		ray = RayDifferential(p, wi);
		sample_gen.imageX = imageX[0];
		sample_gen.imageY = imageY[0];
	}
	for (int i = 0; i <= pathLength; ++i) {
		XYZColor color = L[i].ToXYZ();
		if (i || !specular0)
			color /= totalWeight;
		if (isinf(color.y()))
			continue;
		if ((i == 0 || weight[i] > 0.f) && color.y() >= -1e-5f)
			sample->AddContribution(imageX[i], imageY[i],
				color, alpha ? *alpha : 1.f);
	}
	return totalWeight;
}
SurfaceIntegrator* Path2Integrator::CreateSurfaceIntegrator(const ParamSet &params)
{
	// general
	int maxDepth = params.FindOneInt("maxdepth", 16);
	float RRcontinueProb = params.FindOneFloat("rrcontinueprob", .65f);			// continueprobability for RR (0.0-1.0)

	return new Path2Integrator(maxDepth, RRcontinueProb);

}

