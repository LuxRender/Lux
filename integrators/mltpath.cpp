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

// MLT path tracing integrator (unfinished) by Radiance
// needs RR path termination and implicit illumination at end of path

// mltpath.cpp*
#include "mltpath.h"
#include "bxdf.h"
#include "light.h"
#include "paramset.h"

using namespace lux;

// MLTPathIntegrator Method Definitions
void MLTPathIntegrator::RequestSamples(Sample *sample, const Scene *scene)
{
	lightNumOffset = sample->Add1D(1);
	lightSampOffset = sample->Add2D(1);
	vector<u_int> offsets;
	offsets.push_back(2);	//outgoingDirectionOffset
	offsets.push_back(1);	//outgoingComponentOffset
	offsets.push_back(1);	//continueOffset
	sampleOffset = sample->AddxD(offsets, maxDepth);
}

SWCSpectrum MLTPathIntegrator::Li(const Scene *scene,
		const RayDifferential &r, const Sample *sample,
		float *alpha) const
{
	SampleGuard guard(sample->sampler, sample);
	RayDifferential ray(r);
	SWCSpectrum pathThroughput = 1., L = 0., Le;
	int lightNum;
	Light *light;
	Vector wi;
	float lightWeight, lightPdf, ls1, ls2;
	VisibilityTester visibility;

	// Setup light params for this path
	lightNum = Floor2Int(sample->oneD[lightNumOffset][0] * scene->lights.size());
	lightNum = min(lightNum, (int)scene->lights.size() - 1);
	light = scene->lights[lightNum];
	lightWeight = float(scene->lights.size());
	ls1 = sample->twoD[lightSampOffset][0];
	ls2 = sample->twoD[lightSampOffset][1];

	if (alpha) *alpha = 1.;
	// NOTE - Ratow - Removed recursion: looping gives me an ~8% speed up.
	for (int pathLength = 1; ; ++pathLength) {
		// Find next vertex of path
		Intersection isect;
		if (!scene->Intersect(ray, &isect)) {
			for (u_int i = 0; i < scene->lights.size(); ++i)
				L += pathThroughput * scene->lights[i]->Le(ray);
			break;
		}

		// Evaluate BSDF at hit point
		BSDF *bsdf = isect.GetBSDF(ray);
		const Point &p = bsdf->dgShading.p;
		const Normal &n = bsdf->dgShading.nn;
		Vector wo = -ray.d;
		SWCSpectrum emittance = isect.Le(wo);

		// Note - Ratow - first ray transmittance is accounted in scene.cpp
		if (pathLength > 1)
			pathThroughput *= scene->Transmittance(ray);

		if (emittance != 0.) {
			// Implicity light path
			L += pathThroughput * isect.Le(-ray.d);
		}	else {
			// Explicit light path: always connect to the same light
			Le = light->Sample_L(p, n,	ls1, ls2, &wi, &lightPdf, &visibility);
			SWCSpectrum f = bsdf->f(wo, wi);
			if(lightPdf > 0. && !Le.Black() && !f.Black() && visibility.Unoccluded(scene))
				L += pathThroughput * lightWeight * f * Le * AbsDot(wi, n) / lightPdf;
		}

		if (pathLength == maxDepth)
			break;

		// pick new direction for outgoing ray
		// using metropolis integration sampler
		Vector wi;
		float pdf;
		SWCSpectrum f;
		BxDFType flags;
		float *data = sample->sampler->GetLazyValues(const_cast<Sample *>(sample), sampleOffset, pathLength);

		// material, sample BSDF
		f = bsdf->Sample_f(wo, &wi, data[0]/*outgoingDirectionOffset[0]*/, data[1]/*outgoingDirectionOffset[1]*/,
					data[2]/*outgoingComponentOffset*/, &pdf, BSDF_ALL, &flags);
		if (f.Black() || pdf == 0.)
			break;

		// Possibly terminate the path
		if (pathLength > 3) {
			float q = min(1., f.y()*AbsDot(wi, n)/pdf);
			if (q < data[3]/*continueOffset[pathLength*/)
				break;
			pathThroughput /= q;
		}

		// trace reflected
		ray = RayDifferential(p, wi);

		// increase path contribution
		pathThroughput *= f * AbsDot(wi, n) / pdf;
	}
	L *= scene->volumeIntegrator->Transmittance(scene, ray, sample, alpha);
	L += scene->volumeIntegrator->Li(scene, ray, sample, alpha);
	sample->AddContribution(sample->imageX, sample->imageY,
		L.ToXYZ(), alpha ? *alpha : 1.f);
	return L;
}

SurfaceIntegrator* MLTPathIntegrator::CreateSurfaceIntegrator(const ParamSet &params)
{
	// general
	int maxDepth = params.FindOneInt("maxdepth", 32);
	return new MLTPathIntegrator(maxDepth);

}

