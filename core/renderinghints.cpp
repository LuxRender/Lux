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

#include "renderinghints.h"
#include "error.h"
#include "transport.h"
#include "scene.h"
#include "light.h"
#include "mcdistribution.h"
#include "bxdf.h"
#include "sampling.h"
#include "paramset.h"

#include <boost/assert.hpp>

using namespace lux;

//------------------------------------------------------------------------------
// Light Rendering Hints
//------------------------------------------------------------------------------

void LightRenderingHints::InitParam(const ParamSet &params) {
	importance = max(params.FindOneFloat("importance", 1.f), 0.f);
}

//------------------------------------------------------------------------------
// Light Sampling Strategies
//------------------------------------------------------------------------------

LightsSamplingStrategy *LightsSamplingStrategy::Create(const string &name,
	const ParamSet &params)
{
	enum LightStrategyType lightStrategyType;
	LightsSamplingStrategy *lsStrategy = NULL;
	// For compatibility with past versions
	string st = params.FindOneString(name,
		params.FindOneString("strategy", "auto"));

	if (st == "one")
		lightStrategyType = LightsSamplingStrategy::SAMPLE_ONE_UNIFORM;
	else if (st == "all")
		lightStrategyType = LightsSamplingStrategy::SAMPLE_ALL_UNIFORM;
	else if (st == "auto")
		lightStrategyType = LightsSamplingStrategy::SAMPLE_AUTOMATIC;
	else if (st == "importance")
		lightStrategyType = LightsSamplingStrategy::SAMPLE_ONE_IMPORTANCE;
	else if (st == "powerimp")
		lightStrategyType = LightsSamplingStrategy::SAMPLE_ONE_POWER_IMPORTANCE;
	else if (st == "allpowerimp")
		lightStrategyType = LightsSamplingStrategy::SAMPLE_ALL_POWER_IMPORTANCE;
	else if (st == "logpowerimp")
		lightStrategyType = LightsSamplingStrategy::SAMPLE_ONE_LOG_POWER_IMPORTANCE;
	else {
		LOG( LUX_WARNING,LUX_BADTOKEN) << "Strategy  '" << st << "' unknown. Using \"auto\".";
		lightStrategyType = LightsSamplingStrategy::SAMPLE_AUTOMATIC;
	}

	// Create the light strategy
	switch (lightStrategyType) {
		case LightsSamplingStrategy::SAMPLE_ALL_UNIFORM:
			lsStrategy = new LSSAllUniform();
			break;
		case LightsSamplingStrategy::SAMPLE_ONE_UNIFORM:
			lsStrategy =  new LSSOneUniform();
			break;
		case LightsSamplingStrategy::SAMPLE_AUTOMATIC:
			lsStrategy =  new LSSAuto();
			break;
		case LightsSamplingStrategy::SAMPLE_ONE_IMPORTANCE:
			lsStrategy = new LSSOneImportance();
			break;
		case LightsSamplingStrategy::SAMPLE_ONE_POWER_IMPORTANCE:
			lsStrategy = new LSSOnePowerImportance();
			break;
		case LightsSamplingStrategy::SAMPLE_ALL_POWER_IMPORTANCE:
			lsStrategy = new LSSAllPowerImportance();
			break;
		case LightsSamplingStrategy::SAMPLE_ONE_LOG_POWER_IMPORTANCE:
			lsStrategy = new LSSOneLogPowerImportance();
			break;
		default:
			BOOST_ASSERT(false);
	}
	if (lsStrategy)
		lsStrategy->InitParam(params);
	return lsStrategy;
}

//******************************************************************************
// Light Sampling Strategies: LightStrategyAllUniform
//******************************************************************************

const Light *LSSAllUniform::SampleLight(const Scene &scene, u_int index,
	float *u, float *pdf) const
{
	if (index >= scene.lights.size())
		return NULL;
	*pdf = 1.f;
	return scene.lights[index];
}

float LSSAllUniform::Pdf(const Scene &scene, const Light *light) const
{
	return 1.f;
}

float LSSAllUniform::Pdf(const Scene &scene, u_int light) const
{
	return 1.f;
}

u_int LSSAllUniform::GetSamplingLimit(const Scene &scene) const
{
	return scene.lights.size();
}

//******************************************************************************
// Light Sampling Strategies: LightStrategyOneUniform
//******************************************************************************

const Light *LSSOneUniform::SampleLight(const Scene &scene, u_int index,
	float *u, float *pdf) const
{
	if (index > 0)
		return NULL;
	const u_int nLights = scene.lights.size();
	*u *= nLights;
	const u_int n = min(Floor2UInt(*u), nLights - 1);
	*u -= n;
	*pdf = 1.f / nLights;
	return scene.lights[n];
}

float LSSOneUniform::Pdf(const Scene &scene, const Light *light) const
{
	return 1.f / scene.lights.size();
}

float LSSOneUniform::Pdf(const Scene &scene, u_int light) const
{
	return 1.f / scene.lights.size();
}

//******************************************************************************
// Light Sampling Strategies: LightStrategyAuto
//******************************************************************************

void LSSAuto::Init(const Scene &scene)
{
	if (scene.lights.size() > 5)
		strategy = new LSSOneUniform();
	else
		strategy = new LSSAllUniform();

	strategy->Init(scene);
}

//******************************************************************************
// Light Sampling Strategies: LightStrategyOneImportance
//******************************************************************************

LSSOneImportance::~LSSOneImportance()
{
	delete lightDistribution;
}

void LSSOneImportance::Init(const Scene &scene) {
	// Compute light importance CDF
	const u_int nLights = scene.lights.size();
	float *lightImportance = new float[nLights];

	for (u_int i = 0; i < nLights; ++i)
		lightImportance[i] = scene.lights[i]->GetRenderingHints()->GetImportance();

	lightDistribution = new Distribution1D(lightImportance, nLights);
	delete[] lightImportance;
}

const Light *LSSOneImportance::SampleLight(const Scene &scene, u_int index,
	float *u, float *pdf) const
{
	if (index > 0)
		return NULL;
	return scene.lights[lightDistribution->SampleDiscrete(*u, pdf, u)];
}

float LSSOneImportance::Pdf(const Scene &scene, const Light *light) const
{
	for (u_int i = 0; i < scene.lights.size(); ++i) {
		if (scene.lights[i] == light)
			return lightDistribution->Pdf(i);
	}
	return 0.f;
}

float LSSOneImportance::Pdf(const Scene &scene, u_int light) const
{
	return light < scene.lights.size() ? lightDistribution->Pdf(light) : 0.f;
}

//******************************************************************************
// Light Sampling Strategies: LightStrategyOnePowerImportance
//******************************************************************************

void LSSOnePowerImportance::Init(const Scene &scene) {
	// Compute light power CDF
	const u_int nLights = scene.lights.size();
	float *lightPower = new float[nLights];

	// Averge the light power
	for (u_int i = 0; i < nLights; ++i) {
		const Light *l = scene.lights[i];
		lightPower[i] = l->GetRenderingHints()->GetImportance() * l->Power(scene);
	}

	lightDistribution = new Distribution1D(lightPower, nLights);
	delete[] lightPower;
}

//******************************************************************************
// Light Sampling Strategies: LightStrategyAllPowerImportance
//******************************************************************************

const Light *LSSAllPowerImportance::SampleLight(const Scene &scene, u_int index,
	float *u, float *pdf) const
{
	// Return as many samples as lights but distribute according to power
	// and importance
	if (index >= scene.lights.size())
		return NULL;
	const Light *light =  scene.lights[lightDistribution->SampleDiscrete((index + *u) / scene.lights.size(), pdf, u)];
	*pdf *= scene.lights.size();
	return light;
}

float LSSAllPowerImportance::Pdf(const Scene &scene, const Light *light) const
{
	for (u_int i = 0; i < scene.lights.size(); ++i) {
		if (scene.lights[i] == light)
			return lightDistribution->Pdf(i) * scene.lights.size();
	}
	return 0.f;
}

float LSSAllPowerImportance::Pdf(const Scene &scene, u_int light) const
{
	return light < scene.lights.size() ?
		lightDistribution->Pdf(light) * scene.lights.size() : 0.f;
}

u_int LSSAllPowerImportance::GetSamplingLimit(const Scene &scene) const
{
	return scene.lights.size();
}

//******************************************************************************
// Light Sampling Strategies: LightStrategyOneLogPowerImportance
//******************************************************************************

void LSSOneLogPowerImportance::Init(const Scene &scene) {
	// Compute light power CDF
	const u_int nLights = scene.lights.size();
	float *lightPower = new float[nLights];

	// Averge the light power
	for (u_int i = 0; i < nLights; ++i) {
		const Light *l = scene.lights[i];
		lightPower[i] = logf(l->GetRenderingHints()->GetImportance() * l->Power(scene));
	}

	lightDistribution = new Distribution1D(lightPower, nLights);
	delete[] lightPower;
}

//------------------------------------------------------------------------------
// SurfaceIntegrator Rendering Hints
//------------------------------------------------------------------------------

void SurfaceIntegratorRenderingHints::InitParam(const ParamSet &params)
{
	shadowRayCount = max(params.FindOneInt("shadowraycount", 1), 1);

	// Light Strategy
	lsStrategy = LightsSamplingStrategy::Create("lightstrategy", params);
}

void SurfaceIntegratorRenderingHints::InitStrategies(const Scene &scene) {
	nLights = scene.lights.size();
	if (lsStrategy != NULL)
		lsStrategy->Init(scene);
}

void SurfaceIntegratorRenderingHints::RequestSamples(Sampler *sampler, const Scene &scene, u_int maxDepth)
{
	if (lsStrategy != NULL) {
		vector<u_int> structure(0);
		// Request samples for BSDF sampling
		structure.push_back(2); // BSDF direction sample
		structure.push_back(1); // BSDF component sample
		// Request samples for each shadow ray we have to trac
		const u_int samplingCount = lsStrategy->GetSamplingLimit(scene);
		for (u_int i = 0; i < samplingCount; ++i) {
			structure.push_back(1); // light source sample
			for (u_int j = 0; j < shadowRayCount; ++j) {
				structure.push_back(2); // light direction sample
				structure.push_back(1); // light portal sample
			}
		}
		lightSampleOffset = sampler->AddxD(structure, maxDepth);
	}
}

// Note: results are added to L and optional parameter V content
u_int SurfaceIntegratorRenderingHints::SampleLights(const Scene &scene,
	const Sample &sample, const Point &p, const Normal &n, const Vector &wo,
	BSDF *bsdf, u_int depth, const SWCSpectrum &scale,
	vector<SWCSpectrum> &L, vector<float> *V) const
{
	if (nLights == 0)
		return 0;

	const float *data = sample.sampler->GetLazyValues(sample,
		lightSampleOffset, depth);
	u_int nContribs = 0;
	// Use multiple importance sampling if the surface is not diffuse
	const BxDFType noDiffuse = BxDFType(BSDF_ALL & ~(BSDF_DIFFUSE));
	const bool mis = bsdf->NumComponents(noDiffuse) > 0;
	if (mis) {
		// Trace a second shadow ray by sampling the BSDF
		Vector wi;
		float bsdfPdf;
		BxDFType sampledType;
		SWCSpectrum Li, Lt(scale);
		if (bsdf->SampleF(sample.swl, wo, &wi, data[0], data[1], data[2],
			&Li, &bsdfPdf, BSDF_ALL, &sampledType, NULL, true) &&
			!(sampledType & BSDF_SPECULAR)) {
			Lt *= Li;
			// Add light contribution from BSDF sampling
			Intersection lightIsect;
			Ray ray(p, wi);
			ray.time = sample.realTime;
			BSDF *ibsdf;
			const Volume *volume = bsdf->GetVolume(wi);
			for (u_int n = 0; n < 1000; ++n) {
				if (!scene.Intersect(sample, volume,
					bsdf->dgShading.scattered, ray, 1.f,
					&lightIsect, &ibsdf, NULL, NULL, &Lt)) {
					BSDF *lightBsdf;
					float lightPdf;
					for (u_int i = 0; i < scene.lights.size(); ++i) {
						const Light *light = scene.lights[i];
						if (!light->IsEnvironmental())
							continue;
						Li = Lt;
						if (!light->Le(scene, sample, ray,
							&lightBsdf, NULL, &lightPdf,
							&Li))
							continue;
						const float d2 = DistanceSquared(p,
							lightBsdf->dgShading.p);
						const float lsPdf = lsStrategy->Pdf(scene, light);
						const float lightPdf2 = lightPdf *
							lsPdf * shadowRayCount * d2 /
							AbsDot(wi, lightBsdf->dgShading.nn);
						const float weight = PowerHeuristic(1,
							bsdfPdf, 1, lightPdf2);
						L[light->group] += Li * weight;
						++nContribs;
					}
					break;
				} else {
					Li = Lt;
					BSDF *lightBsdf;
					float lightPdf;
					if (lightIsect.Le(sample, ray,
						&lightBsdf, NULL, &lightPdf,
						&Li)) {
						const float d2 = DistanceSquared(p,
							lightBsdf->dgShading.p);
						const float lsPdf = lsStrategy->Pdf(scene, lightIsect.arealight) * shadowRayCount;
						const float lightPdf2 = lightPdf *
							lsPdf * d2 /
							AbsDot(wi, lightBsdf->dgShading.nn);
						const float weight = PowerHeuristic(1,
							bsdfPdf, 1, lightPdf2);
						L[lightIsect.arealight->group] += Li *
							weight;
						++nContribs;
					}
					bsdfPdf *= ibsdf->Pdf(sample.swl, -wi, wi,
						BxDFType(BSDF_TRANSMISSION | BSDF_SPECULAR));
					if (!(bsdfPdf > 0.f))
						break;
					ray = Ray(ibsdf->dgShading.p, wi);
					ray.time = sample.realTime;
					Lt *= ibsdf->F(sample.swl, -wi, wi, true, BxDFType(BSDF_TRANSMISSION | BSDF_SPECULAR));
					volume = ibsdf->GetVolume(wi);
				}
			}
		}
	}

	// Do the next event estimation (direct lighting)
	const u_int sampleCount = lsStrategy->GetSamplingLimit(scene);
	for (u_int i = 0; i < sampleCount; ++i) {
		const u_int offset = i * (1 + shadowRayCount * 3) + 3;
		float lc = data[offset];
		float lsPdf;
		const Light *light = lsStrategy->SampleLight(scene, i, &lc,
			&lsPdf);
		if (!light)
			break;
		lsPdf *= shadowRayCount;
		for (u_int j = 0; j < shadowRayCount; ++j) {
			const u_int offset2 = offset + j * 3 + 1;
			// Trace a shadow ray by sampling the light source
			float lightPdf;
			SWCSpectrum Li;
			BSDF *lightBsdf;
			if (!light->SampleL(scene, sample, p, data[offset2],
				data[offset2 + 1], data[offset2 + 2],
				&lightBsdf, NULL, &lightPdf, &Li))
				continue;
			const Point &pL(lightBsdf->dgShading.p);
			const Vector wi0(pL - p);
			const Volume *volume = bsdf->GetVolume(wi0);
			if (!volume)
				volume = lightBsdf->GetVolume(-wi0);
			if (!scene.Connect(sample, volume,
				bsdf->dgShading.scattered,
				false, p, pL, false, &Li, NULL, NULL))
				continue;
			const float d2 = wi0.LengthSquared();
			const Vector wi(wi0 / sqrtf(d2));
			Li *= lightBsdf->F(sample.swl, Vector(lightBsdf->dgShading.nn),
				-wi, false) / (d2 * lsPdf);
			Li *= bsdf->F(sample.swl, wi, wo, true) * scale;
			if (Li.Black())
				continue;
			if (mis) {
				const float bsdfPdf = bsdf->Pdf(sample.swl,
					wo, wi);
				Li *= PowerHeuristic(1, lightPdf * lsPdf * d2 /
					AbsDot(wi, lightBsdf->dgShading.nn), 1, bsdfPdf);
			}
			// Add light's contribution
			L[light->group] += Li;
			++nContribs;
		}
	}

	if (V) {
		for (u_int i = 0; i < scene.lightGroups.size(); ++i)
			(*V)[i] += L[i].Filter(sample.swl);
	}

	return nContribs;
}
