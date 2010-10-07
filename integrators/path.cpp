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
#include "sampling.h"
#include "scene.h"
#include "bxdf.h"
#include "light.h"
#include "camera.h"
#include "paramset.h"
#include "dynload.h"
#include "path.h"

#include "luxrays/core/dataset.h"

using namespace lux;

static const u_int passThroughLimit = 10000;

// PathIntegrator Method Definitions
void PathIntegrator::RequestSamples(Sample *sample, const Scene &scene)
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

void PathIntegrator::Preprocess(const RandomGenerator &rng, const Scene &scene)
{
	// Prepare image buffers
	BufferType type = BUF_TYPE_PER_PIXEL;
	scene.sampler->GetBufferType(&type);
	bufferId = scene.camera->film->RequestBuffer(type, BUF_FRAMEBUFFER, "eye");

	hints.InitStrategies(scene);
}

//------------------------------------------------------------------------------
// SamplerRenderer integrator code
//------------------------------------------------------------------------------

u_int PathIntegrator::Li(const Scene &scene, const Sample &sample) const
{
	u_int nrContribs = 0;
	// Declare common path integration variables
	const SpectrumWavelengths &sw(sample.swl);
	Ray ray;
	float rayWeight = sample.camera->GenerateRay(scene, sample, &ray);

	const float nLights = scene.lights.size();
	const u_int lightGroupCount = scene.lightGroups.size();
	// Direct lighting
	vector<SWCSpectrum> Ld(lightGroupCount, 0.f);
	// Direct lighting samples variance
	vector<float> Vd(lightGroupCount, 0.f);
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
		if (!scene.Intersect(sample, volume, ray, &isect, &bsdf,
			&pathThroughput)) {
			// Dade - now I know ray.maxt and I can call volumeIntegrator
			SWCSpectrum Lv;
			u_int g = scene.volumeIntegrator->Li(scene, ray, sample,
				&Lv, &alpha);
			if (!Lv.Black()) {
				L[g] = Lv;
				V[g] += Lv.Filter(sw) * VContrib;
				++nrContribs;
			}

			// Stop path sampling since no intersection was found
			// Possibly add horizon in render & reflections
			if ((includeEnvironment || vertexIndex > 0) &&
				specularBounce) {
				BSDF *ibsdf;
				for (u_int i = 0; i < nLights; ++i) {
					SWCSpectrum Le(pathThroughput);
					if (scene.lights[i]->Le(scene, sample,
						ray, &ibsdf, NULL, NULL, &Le)) {
						L[scene.lights[i]->group] += Le;
						V[scene.lights[i]->group] += Le.Filter(sw) * VContrib;
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
			distance = ray.maxt * ray.d.Length();
		}

		SWCSpectrum Lv;
		const u_int g = scene.volumeIntegrator->Li(scene, ray, sample,
			&Lv, &alpha);
		if (!Lv.Black()) {
			Lv *= pathThroughput;
			L[g] += Lv;
			V[g] += Lv.Filter(sw) * VContrib;
			++nrContribs;
		}

		// Possibly add emitted light at path vertex
		Vector wo(-ray.d);
		if (specularBounce && isect.arealight) {
			BSDF *ibsdf;
			SWCSpectrum Le(isect.Le(sample, ray, &ibsdf, NULL,
				NULL));
			if (!Le.Black()) {
				Le *= pathThroughput;
				L[isect.arealight->group] += Le;
				V[isect.arealight->group] += Le.Filter(sw) * VContrib;
				++nrContribs;
			}
		}
		if (pathLength == maxDepth)
			break;
		// Evaluate BSDF at hit point
		const float *data = scene.sampler->GetLazyValues(sample,
			sampleOffset, pathLength);

		// Estimate direct lighting
		const Point &p = bsdf->dgShading.p;
		const Normal &n = bsdf->dgShading.nn;
		if (nLights > 0) {
			for (u_int i = 0; i < lightGroupCount; ++i) {
				Ld[i] = 0.f;
				Vd[i] = 0.f;
			}

			nrContribs += hints.SampleLights(scene, sample, p, n,
				wo, bsdf, pathLength, pathThroughput, Ld, &Vd);

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
		if (!bsdf->Sample_f(sw, wo, &wi, data[0], data[1], data[2], &f,
			&pdf, BSDF_ALL, &flags, NULL, true))
			break;

		const float dp = AbsDot(wi, n) / pdf;

		if (flags != (BSDF_TRANSMISSION | BSDF_SPECULAR) ||
			!(bsdf->Pdf(sw, wi, wo, BxDFType(BSDF_TRANSMISSION | BSDF_SPECULAR)) > 0.f)) {
			// Possibly terminate the path
			if (vertexIndex > 3) {
				if (rrStrategy == RR_EFFICIENCY) { // use efficiency optimized RR
					const float q = min<float>(1.f, f.Filter(sw) * dp);
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

		ray = Ray(p, wi);
		ray.time = sample.realTime;
		volume = bsdf->GetVolume(wi);
	}
	for (u_int i = 0; i < lightGroupCount; ++i) {
		if (!L[i].Black())
			V[i] /= L[i].Filter(sw);
		sample.AddContribution(sample.imageX, sample.imageY,
			XYZColor(sw, L[i]) * rayWeight, alpha, distance,
			V[i], bufferId, i);
	}

	return nrContribs;
}

//------------------------------------------------------------------------------
// DataParallel integrator PathState code
//------------------------------------------------------------------------------

PathState::PathState(const Scene &scene, ContributionBuffer *contribBuffer, RandomGenerator *rng) :
	sample(scene.surfaceIntegrator, scene.volumeIntegrator, scene), state(TO_INIT) {
	scene.sampler->InitSample(&sample);
	sample.contribBuffer = contribBuffer;
	sample.camera = scene.camera->Clone();
	sample.realTime = 0.f;
	sample.rng = rng;

	const u_int lightGroupCount = scene.lightGroups.size();
	L.resize(lightGroupCount, 0.f);
	V.resize(lightGroupCount, 0.f);

	Ld = new SWCSpectrum[1];
	Vd = new float[1];
	LdGroup = new u_int[1];
	shadowRay = new Ray[1];
	currentShadowRayIndex = new u_int[1];
}

PathState::~PathState() {
	delete[] Ld;
	delete[] Vd;
	delete[] LdGroup;
	delete[] shadowRay;
	delete[] currentShadowRayIndex;
}

bool PathState::Init(const Scene &scene) {
	// Free BSDF memory from computing image sample value
	sample.arena.FreeAll();

	const bool result = scene.sampler->GetNextSample(&sample);

	// save ray time value
	sample.realTime = sample.camera->GetTime(sample.time);
	// sample camera transformation
	sample.camera->SampleMotion(sample.realTime);

	// Sample new SWC thread wavelengths
	sample.swl.Sample(sample.wavelengths);

	pathLength = 0;
	alpha = 1.f;
	distance = INFINITY;
	VContrib = .1f;
	pathThroughput = 1.f;
	volume = NULL;
	specularBounce = true;
	specular = true;

	const u_int lightGroupCount = scene.lightGroups.size();
	for (u_int i = 0; i < lightGroupCount; ++i) {
		L[i] = 0.f;
		V[i] = 0.f;
	}

	eyeRayWeight = sample.camera->GenerateRay(scene, sample, &pathRay);

	state = EYE_VERTEX;

	return result;
}

void PathState::Terminate(const Scene &scene, const u_int bufferId) {
	const u_int lightGroupCount = scene.lightGroups.size();
	for (u_int i = 0; i < lightGroupCount; ++i) {
		if (!L[i].Black())
			V[i] /= L[i].Filter(sample.swl);

		sample.AddContribution(sample.imageX, sample.imageY,
			XYZColor(sample.swl, L[i]) * eyeRayWeight, alpha, distance,
			V[i], bufferId, i);
	}
	scene.sampler->AddSample(sample);
	state = PathState::TERMINATE;
}

//------------------------------------------------------------------------------
// DataParallel integrator PathIntegrator code
//------------------------------------------------------------------------------

SurfaceIntegratorState *PathIntegrator::NewState(const Scene &scene,
		ContributionBuffer *contribBuffer, RandomGenerator *rng) {
	return new PathState(scene, contribBuffer, rng);
}

bool PathIntegrator::GenerateRays(const Scene &,
		SurfaceIntegratorState *s, luxrays::RayBuffer *rayBuffer) {
	PathState *state = (PathState *)s;
	const u_int leftSpace = rayBuffer->LeftSpace();

	switch (state->state) {
		case PathState::EYE_VERTEX: {
			if (1 > leftSpace)
				return false;

			// A pointer trick
			luxrays::Ray *ray = (luxrays::Ray *)&state->pathRay;
			state->currentPathRayIndex = rayBuffer->AddRay(*ray);
			break;
		}
		case PathState::NEXT_VERTEX: {
			if (1 + state->tracedShadowRayCount > leftSpace)
				return false;

			// A pointer trick
			luxrays::Ray *ray = (luxrays::Ray *)&state->pathRay;
			state->currentPathRayIndex = rayBuffer->AddRay(*ray);

			for (u_int i = 0; i < state->tracedShadowRayCount; ++i) {
				// A pointer trick
				luxrays::Ray *ray = (luxrays::Ray *)&state->shadowRay[i];
				state->currentShadowRayIndex[i] = rayBuffer->AddRay(*ray);
			}
			break;
		}
		case PathState::CONTINUE_SHADOWRAY: {
			if (state->tracedShadowRayCount > leftSpace)
				return false;

			for (u_int i = 0; i < state->tracedShadowRayCount; ++i) {
				// A pointer trick
				luxrays::Ray *ray = (luxrays::Ray *)&state->shadowRay[i];
				state->currentShadowRayIndex[i] = rayBuffer->AddRay(*ray);
			}
			break;
		}
		default:
			throw std::runtime_error("Internal error in PathIntegrator::GenerateRays(): unknown path state.");
	}

	return true;
}

bool PathIntegrator::NextState(const Scene &scene, SurfaceIntegratorState *s, luxrays::RayBuffer *rayBuffer, u_int *nrContribs) {
	PathState *state = (PathState *)s;

	const luxrays::RayHit *rayHit = rayBuffer->GetRayHit(state->currentPathRayIndex);
	const u_int nLights = scene.lights.size();
	const SpectrumWavelengths &sw(state->sample.swl);

	*nrContribs = 0;

	//--------------------------------------------------------------------------
	// Finish direct light sampling
	//--------------------------------------------------------------------------

	if (((state->state == PathState::NEXT_VERTEX) || (state->state == PathState::CONTINUE_SHADOWRAY)) &&
			(state->tracedShadowRayCount > 0)) {
		u_int leftShadowRaysToTrace = 0;

		for (u_int i = 0; i < state->tracedShadowRayCount; ++i) {
			int result = scene.Connect(state->sample, state->volume, state->shadowRay[i],
					*(rayBuffer->GetRayHit(state->currentShadowRayIndex[i])), &state->Ld[i], NULL, NULL);
			if (result == 1) {
				const u_int group = state->LdGroup[i];
				state->L[group] += state->Ld[i];
				state->V[group] += state->Vd[i];
			} else if (result == 0) {
				// I have to continue to trace the ray
				state->shadowRay[leftShadowRaysToTrace] = state->shadowRay[i];
				++leftShadowRaysToTrace;
			}
		}

		if (leftShadowRaysToTrace > 0) {
			// I have to continue to trace shadow rays
			state->state = PathState::CONTINUE_SHADOWRAY;
			state->tracedShadowRayCount = leftShadowRaysToTrace;

			// Save the path ray hit
			state->pathRayHit = *rayHit;

			return false;
		}
	}

	//--------------------------------------------------------------------------
	// Calculate next step
	//--------------------------------------------------------------------------

	BSDF *bsdf;
	Intersection isect;
	if (!scene.Intersect(state->sample, state->volume, state->pathRay, *rayHit, &isect, &bsdf, &state->pathThroughput)) {
		// Stop path sampling since no intersection was found
		// Possibly add horizon in render & reflections
		if ((includeEnvironment || state->pathLength > 0) && state->specularBounce) {
			BSDF *ibsdf;
			for (u_int i = 0; i < nLights; ++i) {
				SWCSpectrum Le(state->pathThroughput);
				if (scene.lights[i]->Le(scene, state->sample,
					state->pathRay, &ibsdf, NULL, NULL, &Le)) {
					state->L[scene.lights[i]->group] += Le;
					state->V[scene.lights[i]->group] += Le.Filter(sw) * state->VContrib;
					++(*nrContribs);
				}
			}
		}

		// Set alpha channel
		if (state->pathLength == 0)
			state->alpha = 0.f;

		// The path is finished
		state->Terminate(scene, bufferId);

		return true;
	} else {
		if (state->pathLength == 0)
			state->distance = rayHit->t * state->pathRay.d.Length();

		// Possibly add emitted light at path vertex
		Vector wo(-state->pathRay.d);

		if (state->specularBounce && isect.arealight) {
			BSDF *ibsdf;
			SWCSpectrum Le(isect.Le(state->sample, state->pathRay, &ibsdf, NULL, NULL));

			if (!Le.Black()) {
				Le *= state->pathThroughput;
				state->L[isect.arealight->group] += Le;
				state->V[isect.arealight->group] += Le.Filter(sw) * state->VContrib;
				++(*nrContribs);
			}
		}

		// Check if we have reached the max. path depth
		if (state->pathLength == maxDepth) {
			state->Terminate(scene, bufferId);
			return true;
		}

		const Point &p = bsdf->dgShading.p;
		const Normal &n = bsdf->dgShading.nn;

		// Direct light sampling
		state->tracedShadowRayCount = 0;
		if (nLights > 0) {
			const float *sampleData = scene.sampler->GetLazyValues(state->sample,
					hints.lightSampleOffset, state->pathLength);

			const float lightNum = sampleData[0];

			// Select a light source to sample
			const u_int lightNumber = min(Floor2UInt(lightNum * nLights), nLights - 1);
			const Light &light(*(scene.lights[lightNumber]));

			// Check if I need direct light sampling
			if (bsdf->NumComponents(BxDFType(BSDF_REFLECTION | BSDF_TRANSMISSION | BSDF_DIFFUSE | BSDF_GLOSSY)) > 0) {
				const float lightSample0 = sampleData[1];
				const float lightSample1 = sampleData[2];
				const float lightSample2 = sampleData[3];

				// Trace a shadow ray by sampling the light source
				float lightPdf;
				SWCSpectrum Li;
				BSDF *lightBsdf;
				if (light.Sample_L(scene, state->sample, p, lightSample0, lightSample1, lightSample2,
					&lightBsdf, NULL, &lightPdf, &Li)) {
					const Point &pL(lightBsdf->dgShading.p);
					const Vector wi0(pL - p);
					const float d2 = wi0.LengthSquared();
					const float length = sqrtf(d2);
					const Vector wi(wi0 / length);

					Li *= lightBsdf->f(sw, Vector(lightBsdf->nn), -wi);
					Li *= bsdf->f(sw, wi, wo);

					if (!Li.Black()) {
						const float shadowRayEpsilon = max(MachineEpsilon::E(pL),
								MachineEpsilon::E(length));

						if (shadowRayEpsilon < length * .5f) {
							const float lightPdf2 = lightPdf * d2 /	AbsDot(wi, lightBsdf->nn);

							// Store light's contribution
							state->Ld[0] = state->pathThroughput * Li * (AbsDot(wi, n) / lightPdf2);
							state->Vd[0] = state->Ld[0].Filter(sw) * state->VContrib;
							state->LdGroup[0] = light.group;

							state->shadowRay[0] = Ray(p, wi, shadowRayEpsilon, length, state->sample.time);
							state->tracedShadowRayCount = 1;
						}
					}
				}
			}
		}

		// Evaluate BSDF at hit point
		const float *data = scene.sampler->GetLazyValues(state->sample,
				sampleOffset, state->pathLength);

		// Sample BSDF to get new path direction
		Vector wi;
		float pdf;
		BxDFType flags;
		SWCSpectrum f;
		if (!bsdf->Sample_f(sw, wo, &wi, data[0], data[1], data[2], &f,
			&pdf, BSDF_ALL, &flags, NULL, true)) {
			state->Terminate(scene, bufferId);
			return true;
		}

		const float dp = AbsDot(wi, n) / pdf;

		if (flags != (BSDF_TRANSMISSION | BSDF_SPECULAR) ||
			!(bsdf->Pdf(sw, wi, wo, BxDFType(BSDF_TRANSMISSION | BSDF_SPECULAR)) > 0.f)) {
			// Possibly terminate the path
			if (state->pathLength > 3) {
				if (rrStrategy == RR_EFFICIENCY) { // use efficiency optimized RR
					const float q = min<float>(1.f, f.Filter(sw) * dp);
					if (q < data[3]) {
						state->Terminate(scene, bufferId);
						return true;
					}
					// increase path contribution
					state->pathThroughput /= q;
				} else if (rrStrategy == RR_PROBABILITY) { // use normal/probability RR
					if (continueProbability < data[3]) {
						state->Terminate(scene, bufferId);
						return true;
					}
					// increase path contribution
					state->pathThroughput /= continueProbability;
				}
			}
			++(state->pathLength);

			state->specularBounce = (flags & BSDF_SPECULAR) != 0;
			state->specular = state->specular && state->specularBounce;
		}
		state->pathThroughput *= f;
		state->pathThroughput *= dp;
		if (!state->specular)
			state->VContrib += dp;

		state->pathRay = Ray(p, wi);
		state->pathRay.time = state->sample.realTime;
		state->volume = bsdf->GetVolume(wi);
		state->state = PathState::NEXT_VERTEX;

		return false;
	}
}

//------------------------------------------------------------------------------
// Integrator parsing code
//------------------------------------------------------------------------------

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
		LOG(LUX_WARNING,LUX_BADTOKEN)<<"Strategy  '" << rst <<"' for russian roulette path termination unknown. Using \"efficiency\".";
		rstrategy = RR_EFFICIENCY;
	}
	bool include_environment = params.FindOneBool("includeenvironment", true);

	PathIntegrator *pi = new PathIntegrator(rstrategy, max(maxDepth, 0), RRcontinueProb, include_environment);
	// Initialize the rendering hints
	pi->hints.InitParam(params);

	return pi;
}

static DynamicLoader::RegisterSurfaceIntegrator<PathIntegrator> r("path");
