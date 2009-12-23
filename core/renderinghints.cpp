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
#include "light.h"
#include "mcdistribution.h"
#include "spectrumwavelengths.h"

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

//******************************************************************************
// Light Sampling Strategies: LightStrategyAllUniform
//******************************************************************************

void LSSAllUniform::RequestSamples(const Scene *scene, vector<u_int> &structure) const {
	structure.push_back(2);	// light position sample
	structure.push_back(1);	// light number/portal sample
	structure.push_back(2);	// bsdf direction sample for light
	structure.push_back(1);	// bsdf component sample for light
}

u_int LSSAllUniform::SampleLights(
	const TsPack *tspack, const Scene *scene, const u_int shadowRayCount,
	const Point &p, const Normal &n, const Vector &wo, BSDF *bsdf,
	const Sample *sample, const float *sampleData, const SWCSpectrum &scale,
	vector<SWCSpectrum> &L) const {
	// Sample all lights in the scene
	const u_int nLights = scene->lights.size();
	const SWCSpectrum newScale = scale / shadowRayCount;
	const u_int sampleCount = RequestSamplesCount(scene);
	u_int nContribs = 0;

	// Using one sample for all lights
	SWCSpectrum Ll;

	for (u_int j = 0; j < nLights; ++j) {
		const Light *light = scene->lights[j];

		for (u_int i = 0; i < shadowRayCount; ++i) {
			const u_int offset = i * sampleCount;
			const float *lightSample = &sampleData[offset];
			const float *lightNum = &sampleData[offset + 2];
			const float *bsdfSample = &sampleData[offset + 3];
			const float *bsdfComponent =  &sampleData[offset + 5];

			Ll = EstimateDirect(tspack, scene, light,
				p, n, wo, bsdf, sample, lightSample[0], lightSample[1], *lightNum,
				bsdfSample[0], bsdfSample[1], *bsdfComponent);

			if (!Ll.Black()) {
				L[light->group] += Ll * newScale;
				++nContribs;
			}
		}
	}

	return nContribs;
}

//******************************************************************************
// Light Sampling Strategies: LightStrategyOneUniform
//******************************************************************************

void LSSOneUniform::RequestSamples(const Scene *scene, vector<u_int> &structure) const {
	structure.push_back(2);	// light position sample
	structure.push_back(1);	// light number/portal sample
	structure.push_back(2);	// bsdf direction sample for light
	structure.push_back(1);	// bsdf component sample for light
}

u_int LSSOneUniform::SampleLights(
	const TsPack *tspack, const Scene *scene, const u_int shadowRayCount,
	const Point &p, const Normal &n, const Vector &wo, BSDF *bsdf,
	const Sample *sample, const float *sampleData, const SWCSpectrum &scale,
	vector<SWCSpectrum> &L) const {
	// Randomly choose a single light to sample
	const u_int nLights = scene->lights.size();
	const SWCSpectrum newScale = static_cast<float>(nLights) * scale / shadowRayCount;
	const u_int sampleCount = this->RequestSamplesCount(scene);
	u_int nContribs = 0;
	for (u_int i = 0; i < shadowRayCount; ++i) {
		const u_int offset = i * sampleCount;
		const float *lightSample = &sampleData[offset];
		const float *lightNum = &sampleData[offset + 2];
		const float *bsdfSample = &sampleData[offset + 3];
		const float *bsdfComponent =  &sampleData[offset + 5];

		float ls3 = *lightNum * nLights;
		const u_int lightNumber = min(Floor2UInt(ls3), nLights - 1);
		ls3 -= lightNumber;
		const Light *light = scene->lights[lightNumber];
		SWCSpectrum Ll = EstimateDirect(tspack, scene, light,
				p, n, wo, bsdf, sample, lightSample[0], lightSample[1], ls3,
				bsdfSample[0], bsdfSample[1], *bsdfComponent);

		if (!Ll.Black()) {
			L[light->group] += Ll * newScale;
			++nContribs;
		}
	}

	return nContribs;
}

//******************************************************************************
// Light Sampling Strategies: LightStrategyOneImportance
//******************************************************************************

void LSSOneImportance::Init(const Scene *scene) {
	// Compute light importance CDF
	const u_int nLights = scene->lights.size();
	float *lightImportance = new float[nLights];

	for (u_int i = 0; i < nLights; ++i)
		lightImportance[i] = scene->lights[i]->GetRenderingHints()->GetImportance();

	lightDistribution = new Distribution1D(lightImportance, nLights);
	delete[] lightImportance;
}

void LSSOneImportance::RequestSamples(const Scene *scene, vector<u_int> &structure) const {
	structure.push_back(2);	// light position sample
	structure.push_back(1);	// light number/portal sample
	structure.push_back(2);	// bsdf direction sample for light
	structure.push_back(1);	// bsdf component sample for light
}

u_int LSSOneImportance::SampleLights(const TsPack *tspack, const Scene *scene,
	const u_int shadowRayCount, const Point &p, const Normal &n,
	const Vector &wo, BSDF *bsdf, const Sample *sample,
	const float *sampleData, const SWCSpectrum &scale,
	vector<SWCSpectrum> &L) const {
	// Choose a single light to sample according the importance
	const u_int nLights = scene->lights.size();
	const SWCSpectrum newScale = scale / shadowRayCount;
	const u_int sampleCount = this->RequestSamplesCount(scene);
	u_int nContribs = 0;
	for (u_int i = 0; i < shadowRayCount; ++i) {
		const u_int offset = i * sampleCount;
		const float *lightSample = &sampleData[offset];
		const float *lightNum = &sampleData[offset + 2];
		const float *bsdfSample = &sampleData[offset + 3];
		const float *bsdfComponent =  &sampleData[offset + 5];

		float lightPdf, ls3;
		const u_int lightNumber = lightDistribution->SampleDiscrete(*lightNum, &lightPdf, &ls3);
		const Light *light = scene->lights[lightNumber];

		SWCSpectrum Ll = EstimateDirect(tspack, scene, light,
			p, n, wo, bsdf, sample, lightSample[0], lightSample[1],
			ls3, bsdfSample[0], bsdfSample[1], *bsdfComponent);

		if (!Ll.Black()) {
			Ll *=  newScale / lightPdf;
			L[light->group] += Ll;
			++nContribs;
		}
	}

	return nContribs;
}

//******************************************************************************
// Light Sampling Strategies: LightStrategyOnePowerImportance
//******************************************************************************

void LSSOnePowerImportance::Init(const Scene *scene) {
	// Compute light power CDF
	const u_int nLights = scene->lights.size();
	float *lightPower = new float[nLights];

	// Averge the light power
	for (u_int i = 0; i < nLights; ++i) {
		const Light *l = scene->lights[i];
		lightPower[i] = l->GetRenderingHints()->GetImportance() * l->Power(scene);
	}

	lightDistribution = new Distribution1D(lightPower, nLights);
	delete[] lightPower;
}

void LSSOnePowerImportance::RequestSamples(const Scene *scene, vector<u_int> &structure) const {
	structure.push_back(2);	// light position sample
	structure.push_back(1);	// light number/portal sample
	structure.push_back(2);	// bsdf direction sample for light
	structure.push_back(1);	// bsdf component sample for light
}

u_int LSSOnePowerImportance::SampleLights(
	const TsPack *tspack, const Scene *scene, const u_int shadowRayCount,
	const Point &p, const Normal &n, const Vector &wo, BSDF *bsdf,
	const Sample *sample, const float *sampleData, const SWCSpectrum &scale,
	vector<SWCSpectrum> &L) const {
	// Choose a single light to sample according the importance
	const u_int nLights = scene->lights.size();
	const SWCSpectrum newScale = scale / shadowRayCount;
	const u_int sampleCount = this->RequestSamplesCount(scene);
	u_int nContribs = 0;
	for (u_int i = 0; i < shadowRayCount; ++i) {
		const u_int offset = i * sampleCount;
		const float *lightSample = &sampleData[offset];
		const float *lightNum = &sampleData[offset + 2];
		const float *bsdfSample = &sampleData[offset + 3];
		const float *bsdfComponent =  &sampleData[offset + 5];

		float lightPdf, ls3;
		const u_int lightNumber = lightDistribution->SampleDiscrete(*lightNum, &lightPdf, &ls3);
		const Light *light = scene->lights[lightNumber];

		SWCSpectrum Ll = EstimateDirect(tspack, scene, light,
			p, n, wo, bsdf, sample, lightSample[0], lightSample[1],
			ls3, bsdfSample[0], bsdfSample[1], *bsdfComponent);

		if (!Ll.Black()) {
			Ll *=  newScale / lightPdf;
			L[light->group] += Ll;
			++nContribs;
		}
	}

	return nContribs;
}

//******************************************************************************
// Light Sampling Strategies: LightStrategyAllPowerImportance
//******************************************************************************

u_int LSSAllPowerImportance::SampleLights(const TsPack *tspack,
	const Scene *scene, const u_int shadowRayCount,
	const Point &p, const Normal &n, const Vector &wo, BSDF *bsdf,
	const Sample *sample, const float *sampleData, const SWCSpectrum &scale,
	vector<SWCSpectrum> &L) const {
	// Choose a single light to sample according the importance
	u_int nLights = scene->lights.size();
	const u_int sampleCount = this->RequestSamplesCount(scene);
	u_int nContribs = 0;
	u_int shadowBase = 0;
	const SWCSpectrum newScale = scale / shadowRayCount;
	if (shadowRayCount >= nLights) {
		// We have enough shadow ray, trace at least one ray for each light source
		for (u_int i = 0; i < nLights; ++i) {
			const u_int offset = i * sampleCount;
			const float *lightSample = &sampleData[offset];
			const float *lightNum = &sampleData[offset + 2];
			const float *bsdfSample = &sampleData[offset + 3];
			const float *bsdfComponent =  &sampleData[offset + 5];

			const Light *light = scene->lights[i];
			SWCSpectrum Ll = EstimateDirect(tspack, scene, light,
				p, n, wo, bsdf, sample, lightSample[0],
				lightSample[1], *lightNum, bsdfSample[0],
				bsdfSample[1], *bsdfComponent);

			if (!Ll.Black()) {
				L[light->group] += Ll * newScale * nLights;
				++nContribs;
			}
		}
		shadowBase = nLights;
	}
	// Use lightCDF for any left shadow ray
	for (u_int i = shadowBase; i < shadowRayCount; ++i) {
		const u_int offset = i * sampleCount;
		const float *lightSample = &sampleData[offset];
		const float *lightNum = &sampleData[offset + 2];
		const float *bsdfSample = &sampleData[offset + 3];
		const float *bsdfComponent =  &sampleData[offset + 5];

		float lightPdf, ls3;
		const u_int lightNumber = lightDistribution->SampleDiscrete(*lightNum, &lightPdf, &ls3);
		const Light *light = scene->lights[lightNumber];

		SWCSpectrum Ll = EstimateDirect(tspack, scene, light,
			p, n, wo, bsdf, sample, lightSample[0], lightSample[1],
			ls3, bsdfSample[0], bsdfSample[1], *bsdfComponent);

		if (!Ll.Black()) {
			Ll *=  newScale / lightPdf;
			L[light->group] += Ll;
			++nContribs;
		}
	}

	return nContribs;
}

//******************************************************************************
// Light Sampling Strategies: LightStrategyOneLogPowerImportance
//******************************************************************************

void LSSOneLogPowerImportance::Init(const Scene *scene) {
	// Compute light power CDF
	const u_int nLights = scene->lights.size();
	float *lightPower = new float[nLights];

	// Averge the light power
	for (u_int i = 0; i < nLights; ++i) {
		const Light *l = scene->lights[i];
		lightPower[i] = logf(l->GetRenderingHints()->GetImportance() * l->Power(scene));
	}

	lightDistribution = new Distribution1D(lightPower, nLights);
	delete[] lightPower;
}

//------------------------------------------------------------------------------
// SurfaceIntegrator Rendering Hints
//------------------------------------------------------------------------------

void SurfaceIntegratorRenderingHints::InitParam(const ParamSet &params) {
	shadowRayCount = max(params.FindOneInt("shadowraycount", 1), 1);

	// Light Strategy

	// For compatibility with past versions
	string oldst = params.FindOneString("strategy", "auto");
	string newst = params.FindOneString("lightstrategy", "auto");
	string st;
	if ((oldst == newst) || (oldst == "auto"))
		st = newst;
	else
		st = oldst;

	if (st == "one") lightStrategyType = LightsSamplingStrategy::SAMPLE_ONE_UNIFORM;
	else if (st == "all") lightStrategyType = LightsSamplingStrategy::SAMPLE_ALL_UNIFORM;
	else if (st == "auto") lightStrategyType = LightsSamplingStrategy::SAMPLE_AUTOMATIC;
	else if (st == "importance") lightStrategyType = LightsSamplingStrategy::SAMPLE_ONE_IMPORTANCE;
	else if (st == "powerimp") lightStrategyType = LightsSamplingStrategy::SAMPLE_ONE_POWER_IMPORTANCE;
	else if (st == "allpowerimp") lightStrategyType = LightsSamplingStrategy::SAMPLE_ALL_POWER_IMPORTANCE;
	else if (st == "logpowerimp") lightStrategyType = LightsSamplingStrategy::SAMPLE_ONE_LOG_POWER_IMPORTANCE;
	else {
		std::stringstream ss;
		ss << "Strategy  '" << st << "' unknown. Using \"auto\".";
		luxError(LUX_BADTOKEN, LUX_WARNING, ss.str().c_str());
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
}

void SurfaceIntegratorRenderingHints::InitStrategies(const Scene *scene) {
	if (lsStrategy != NULL)
		lsStrategy->Init(scene);
}

void SurfaceIntegratorRenderingHints::RequestSamples(const Scene *scene, vector<u_int> &structure) {
	if (lsStrategy != NULL) {
		lightSampleOffset = structure.size();
		// Request samples for each shadow ray we have to trace
		for (u_int i = 0; i <  shadowRayCount; ++i)
			lsStrategy->RequestSamples(scene, structure);
	}
}
