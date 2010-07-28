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

// path.cpp*
#include "path.h"
#include "sampling.h"
#include "scene.h"
#include "bxdf.h"
#include "light.h"
#include "camera.h"
#include "paramset.h"
#include "dynload.h"

using namespace lux;

static const u_int passThroughLimit = 10000;

// PathIntegrator Method Definitions
void PathIntegrator::RequestSamples(Sample *sample, const Scene *scene)
{
	vector<u_int> structure;
	structure.push_back(2);	// bsdf direction sample for path
	structure.push_back(1);	// bsdf component sample for path
	if (rrStrategy != RR_NONE)
		structure.push_back(1);	// continue sample

	sampleOffset = sample->AddxD(structure, maxDepth + 1);

	// Allocate and request samples for light sampling, RR, etc.
	hints.RequestSamples(sample, scene, maxDepth + 1);
}

void PathIntegrator::Preprocess(const TsPack *tspack, const Scene *scene)
{
	// Prepare image buffers
	BufferType type = BUF_TYPE_PER_PIXEL;
	scene->sampler->GetBufferType(&type);
	bufferId = scene->camera->film->RequestBuffer(type, BUF_FRAMEBUFFER, "eye");

	hints.InitStrategies(scene);
}

u_int PathIntegrator::Li(const TsPack *tspack, const Scene *scene,
	const Sample *sample) const
{
	u_int nrContribs = 0;
	// Declare common path integration variables
	RayDifferential r;
	float rayWeight = tspack->camera->GenerateRay(tspack, scene, *sample,
		&r);

	const float nLights = scene->lights.size();
	const u_int lightGroupCount = scene->lightGroups.size();
	// Direct lighting
	vector<SWCSpectrum> Ld(lightGroupCount, 0.f);
	// Direct lighting samples variance
	vector<float> Vd(lightGroupCount, 0.f);
	RayDifferential ray(r);
	SWCSpectrum pathThroughput(1.0f);
	vector<SWCSpectrum> L(lightGroupCount, 0.f);
	vector<float> V(lightGroupCount, 0.f);
	float VContrib = .1f;
	bool specularBounce = true, specular = true;
	float alpha = 1.f;
	float distance = INFINITY;
	u_int vertexIndex = 0;
	const Volume *volume = NULL;

	for (u_int pathLength = 0; ; ++pathLength) {
		// Find next vertex of path
		Intersection isect;
		BSDF *bsdf;
		if (!scene->Intersect(tspack, volume, ray, &isect, &bsdf,
			&pathThroughput)) {
			// Dade - now I know ray.maxt and I can call volumeIntegrator
			SWCSpectrum Lv;
			u_int g = scene->volumeIntegrator->Li(tspack, scene,
				ray, sample, &Lv, &alpha);
			if (!Lv.Black()) {
				L[g] = Lv;
				V[g] += Lv.Filter(tspack) * VContrib;
				++nrContribs;
			}
			scene->volumeIntegrator->Transmittance(tspack, scene,
				ray, sample, &alpha, &pathThroughput);

			// Stop path sampling since no intersection was found
			// Possibly add horizon in render & reflections
			if ((includeEnvironment || vertexIndex > 0) &&
				specularBounce) {
				BSDF *ibsdf;
				for (u_int i = 0; i < nLights; ++i) {
					SWCSpectrum Le(pathThroughput);
					if (scene->lights[i]->Le(tspack, scene,
						ray, &ibsdf, NULL, NULL, &Le)) {
						L[scene->lights[i]->group] += Le;
						V[scene->lights[i]->group] += Le.Filter(tspack) * VContrib;
						++nrContribs;
					}
				}
			}

			// Set alpha channel
			if (vertexIndex == 0)
				alpha = 0.f;
			break;
		}
		if (vertexIndex == 0) {
			r.maxt = ray.maxt;
			distance = ray.maxt * ray.d.Length();
		}

		SWCSpectrum Lv;
		const u_int g = scene->volumeIntegrator->Li(tspack, scene, ray, sample, &Lv, &alpha);
		if (!Lv.Black()) {
			Lv *= pathThroughput;
			L[g] += Lv;
			V[g] += Lv.Filter(tspack) * VContrib;
			++nrContribs;
		}

		// Possibly add emitted light at path vertex
		Vector wo(-ray.d);
		if (specularBounce && isect.arealight) {
			BSDF *ibsdf;
			SWCSpectrum Le(isect.Le(tspack, ray, &ibsdf, NULL, 
				NULL));
			if (!Le.Black()) {
				Le *= pathThroughput;
				L[isect.arealight->group] += Le;
				V[isect.arealight->group] += Le.Filter(tspack) * VContrib;
				++nrContribs;
			}
		}
		if (pathLength == maxDepth)
			break;
		// Evaluate BSDF at hit point
		const float *data = sample->sampler->GetLazyValues(const_cast<Sample *>(sample), sampleOffset, pathLength);

		// Estimate direct lighting
		const Point &p = bsdf->dgShading.p;
		const Normal &n = bsdf->dgShading.nn;
		if (nLights > 0) {
			for (u_int i = 0; i < lightGroupCount; ++i) {
				Ld[i] = 0.f;
				Vd[i] = 0.f;
			}

			nrContribs += hints.SampleLights(tspack, scene, p, n, wo, bsdf,
				sample, pathLength, pathThroughput, Ld, &Vd);

			for (u_int i = 0; i < lightGroupCount; ++i) {
				L[i] += Ld[i];
				V[i] += Vd[i] * VContrib;
			}
		}

		// Sample BSDF to get new path direction
		Vector wi;
		float pdf;
		BxDFType flags;
		SWCSpectrum f;
		if (!bsdf->Sample_f(tspack, wo, &wi, data[0], data[1], data[2], &f, &pdf, BSDF_ALL, &flags, NULL, true))
			break;

		const float dp = AbsDot(wi, n) / pdf;

		if (flags != (BSDF_TRANSMISSION | BSDF_SPECULAR) || !(bsdf->Pdf(tspack, wi, wo, BxDFType(BSDF_TRANSMISSION | BSDF_SPECULAR)) > 0.f)) {
			// Possibly terminate the path
			if (vertexIndex > 3) {
				if (rrStrategy == RR_EFFICIENCY) { // use efficiency optimized RR
					const float q = min<float>(1.f, f.Filter(tspack) * dp);
					if (q < data[3])
						break;
					// increase path contribution
					pathThroughput /= q;
				} else if (rrStrategy == RR_PROBABILITY) { // use normal/probability RR
					if (continueProbability < data[3])
						break;
					// increase path contribution
					pathThroughput /= continueProbability;
				}
			}
			++vertexIndex;

			specularBounce = (flags & BSDF_SPECULAR) != 0;
			specular = specular && specularBounce;
		}
		pathThroughput *= f;
		pathThroughput *= dp;
		if (!specular)
			VContrib += dp;

		ray = RayDifferential(p, wi);
		ray.time = r.time;
		volume = bsdf->GetVolume(wi);
	}
	for (u_int i = 0; i < lightGroupCount; ++i) {
		if (!L[i].Black())
			V[i] /= L[i].Filter(tspack);
		sample->AddContribution(sample->imageX, sample->imageY,
			XYZColor(tspack, L[i]) * rayWeight, alpha, distance,
			V[i], bufferId, i);
	}

	return nrContribs;
}
SurfaceIntegrator* PathIntegrator::CreateSurfaceIntegrator(const ParamSet &params)
{
	// general
	int maxDepth = params.FindOneInt("maxdepth", 16);

	float RRcontinueProb = params.FindOneFloat("rrcontinueprob", .65f);			// continueprobability for plain RR (0.0-1.0)
	RRStrategy rstrategy;
	string rst = params.FindOneString("rrstrategy", "efficiency");
	if (rst == "efficiency") rstrategy = RR_EFFICIENCY;
	else if (rst == "probability") rstrategy = RR_PROBABILITY;
	else if (rst == "none") rstrategy = RR_NONE;
	else {
		std::stringstream ss;
		ss<<"Strategy  '" << rst <<"' for russian roulette path termination unknown. Using \"efficiency\".";
		luxError(LUX_BADTOKEN,LUX_WARNING,ss.str().c_str());
		rstrategy = RR_EFFICIENCY;
	}
	bool include_environment = params.FindOneBool("includeenvironment", true);

	PathIntegrator *pi = new PathIntegrator(rstrategy, max(maxDepth, 0), RRcontinueProb, include_environment);
	// Initialize the rendering hints
	pi->hints.InitParam(params);

	return pi;
}

static DynamicLoader::RegisterSurfaceIntegrator<PathIntegrator> r("path");
