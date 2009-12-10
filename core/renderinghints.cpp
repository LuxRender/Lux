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
#include "mc.h"
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
	if (scene->sampler->IsMutating()) {
		const u_int nLights = scene->lights.size();
		for (u_int j = 0; j < nLights; ++j) {
			structure.push_back(2);	// light position sample
			structure.push_back(1);	// light number/portal sample
			structure.push_back(2);	// bsdf direction sample for light
			structure.push_back(1);	// bsdf component sample for light
		}
	} else {
		structure.push_back(2);	// light position sample
		structure.push_back(1);	// light number/portal sample
		structure.push_back(2);	// bsdf direction sample for light
		structure.push_back(1);	// bsdf component sample for light
	}
}

u_int LSSAllUniform::SampleLights(
	const TsPack *tspack, const Scene *scene, const u_int shadowRayCount,
	const Point &p, const Normal &n, const Vector &wo, BSDF *bsdf,
	const Sample *sample, const float *sampleData, const SWCSpectrum &scale,
	vector<SWCSpectrum> &L) const {
	// Sample all lights in the scene
	const u_int nLights = scene->lights.size();
	if (nLights == 0)
		return 0;

	const SWCSpectrum newScale = scale / shadowRayCount;
	const u_int sampleCount = this->RequestSamplesCount(scene);
	u_int nContribs = 0;
	if (scene->sampler->IsMutating()) {
		// Using a different sample for each lights
		for (u_int i = 0; i < shadowRayCount; ++i) {
			SWCSpectrum Ll;
			for (u_int j = 0; j < nLights; ++j) {
				const u_int offset = i * sampleCount + j * 6;
				const float *lightSample = &sampleData[offset];
				const float *lightNum = &sampleData[offset + 2];
				const float *bsdfSample = &sampleData[offset + 3];
				const float *bsdfComponent =  &sampleData[offset + 5];

				const Light *light = scene->lights[j];
				Ll = EstimateDirect(tspack, scene, light,
					p, n, wo, bsdf, sample, lightSample[0], lightSample[1], *lightNum,
					bsdfSample[0], bsdfSample[1], *bsdfComponent);

				if (!Ll.Black()) {
					L[light->group] += Ll * newScale;
					++nContribs;
				}
			}
		}
	} else {
		// Using one sample for all lights
		for (u_int i = 0; i < shadowRayCount; ++i) {
			const u_int offset = i * sampleCount;
			const float *lightSample = &sampleData[offset];
			const float *lightNum = &sampleData[offset + 2];
			const float *bsdfSample = &sampleData[offset + 3];
			const float *bsdfComponent =  &sampleData[offset + 5];

			SWCSpectrum Ll;
			for (u_int j = 0; j < nLights; ++j) {
				const Light *light = scene->lights[j];
				Ll = EstimateDirect(tspack, scene, light,
					p, n, wo, bsdf, sample, lightSample[0], lightSample[1], *lightNum,
					bsdfSample[0], bsdfSample[1], *bsdfComponent);

				if (!Ll.Black()) {
					L[light->group] += Ll * newScale;
					++nContribs;
				}
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
	if (nLights == 0)
		return 0;

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
	lightImportance = new float[nLights];
	lightCDF = new float[nLights + 1];

	for (u_int i = 0; i < nLights; ++i)
		lightImportance[i] = scene->lights[i]->GetRenderingHints()->GetImportance();

	ComputeStep1dCDF(lightImportance, nLights, &totalImportance, lightCDF);
}

void LSSOneImportance::RequestSamples(const Scene *scene, vector<u_int> &structure) const {
	structure.push_back(2);	// light position sample
	structure.push_back(1);	// light number/portal sample
	structure.push_back(2);	// bsdf direction sample for light
	structure.push_back(1);	// bsdf component sample for light
}

u_int LSSOneImportance::SampleLights(
	const TsPack *tspack, const Scene *scene, const u_int shadowRayCount,
	const Point &p, const Normal &n, const Vector &wo, BSDF *bsdf,
	const Sample *sample, const float *sampleData, const SWCSpectrum &scale,
	vector<SWCSpectrum> &L) const {
	// Choose a single light to sample according the importance
	const u_int nLights = scene->lights.size();
	if (nLights == 0)
		return 0;

	const SWCSpectrum newScale = scale / shadowRayCount;
	const u_int sampleCount = this->RequestSamplesCount(scene);
	u_int nContribs = 0;
	for (u_int i = 0; i < shadowRayCount; ++i) {
		const u_int offset = i * sampleCount;
		const float *lightSample = &sampleData[offset];
		const float *lightNum = &sampleData[offset + 2];
		const float *bsdfSample = &sampleData[offset + 3];
		const float *bsdfComponent =  &sampleData[offset + 5];

		float ls3 = *lightNum * nLights;
		float lightPdf;
		u_int lightNumber = Floor2UInt(SampleStep1d(lightImportance, lightCDF,
			totalImportance, nLights, *lightNum, &lightPdf) * nLights);
		lightNumber = min(lightNumber, nLights - 1);
		ls3 -= lightNumber;
		const Light *light = scene->lights[lightNumber];

		SWCSpectrum Ll = EstimateDirect(tspack, scene, light,
				p, n, wo, bsdf, sample, lightSample[0], lightSample[1], ls3,
				bsdfSample[0], bsdfSample[1], *bsdfComponent);

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
	lightPower = new float[nLights];
	lightCDF = new float[nLights + 1];

	// Averge the light power
	for (u_int i = 0; i < nLights; ++i) {
		const Light *l = scene->lights[i];
		lightPower[i] = l->GetRenderingHints()->GetImportance() * l->Power(scene);
	}

	ComputeStep1dCDF(lightPower, nLights, &totalPower, lightCDF);
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
	if (nLights == 0)
		return 0;

	const SWCSpectrum newScale = scale / shadowRayCount;
	const u_int sampleCount = this->RequestSamplesCount(scene);
	u_int nContribs = 0;
	for (u_int i = 0; i < shadowRayCount; ++i) {
		const u_int offset = i * sampleCount;
		const float *lightSample = &sampleData[offset];
		const float *lightNum = &sampleData[offset + 2];
		const float *bsdfSample = &sampleData[offset + 3];
		const float *bsdfComponent =  &sampleData[offset + 5];

		float ls3 = *lightNum * nLights;
		float lightPdf;
		u_int lightNumber = Floor2UInt(SampleStep1d(lightPower, lightCDF,
			totalPower, nLights, *lightNum, &lightPdf) * nLights);
		lightNumber = min(lightNumber, nLights - 1);
		ls3 -= lightNumber;
		const Light *light = scene->lights[lightNumber];

		SWCSpectrum Ll = EstimateDirect(tspack, scene, light,
				p, n, wo, bsdf, sample, lightSample[0], lightSample[1], ls3,
				bsdfSample[0], bsdfSample[1], *bsdfComponent);

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

u_int LSSAllPowerImportance::SampleLights(
	const TsPack *tspack, const Scene *scene, const u_int shadowRayCount,
	const Point &p, const Normal &n, const Vector &wo, BSDF *bsdf,
	const Sample *sample, const float *sampleData, const SWCSpectrum &scale,
	vector<SWCSpectrum> &L) const {
	// Choose a single light to sample according the importance
	u_int nLights = scene->lights.size();
	if (nLights == 0)
		return 0;

	const u_int sampleCount = this->RequestSamplesCount(scene);
	u_int nContribs = 0;
	if (nLights > shadowRayCount) {
		// We don't have enough shadow rays, just work as LightStrategyOnePowerImportance
		const SWCSpectrum newScale = scale / shadowRayCount;
		for (u_int i = 0; i < shadowRayCount; ++i) {
			const u_int offset = i * sampleCount;
			const float *lightSample = &sampleData[offset];
			const float *lightNum = &sampleData[offset + 2];
			const float *bsdfSample = &sampleData[offset + 3];
			const float *bsdfComponent =  &sampleData[offset + 5];

			float ls3 = *lightNum * nLights;
			float lightPdf;
			u_int lightNumber = Floor2UInt(SampleStep1d(lightPower, lightCDF,
				totalPower, nLights, *lightNum, &lightPdf) * nLights);
			lightNumber = min(lightNumber, nLights - 1);
			ls3 -= lightNumber;
			const Light *light = scene->lights[lightNumber];

			SWCSpectrum Ll = EstimateDirect(tspack, scene, light,
					p, n, wo, bsdf, sample, lightSample[0], lightSample[1], ls3,
					bsdfSample[0], bsdfSample[1], *bsdfComponent);

			if (!Ll.Black()) {
				Ll *=  newScale / lightPdf;
				L[light->group] += Ll;
				++nContribs;
			}
		}
	} else {
		// We have enough shadow ray, trace at least one ray for each light source
		for (u_int i = 0; i < nLights; ++i) {
			const u_int offset = i * sampleCount;
			const float *lightSample = &sampleData[offset];
			const float *lightNum = &sampleData[offset + 2];
			const float *bsdfSample = &sampleData[offset + 3];
			const float *bsdfComponent =  &sampleData[offset + 5];

			const Light *light = scene->lights[i];
			SWCSpectrum Ll = EstimateDirect(tspack, scene, light,
					p, n, wo, bsdf, sample, lightSample[0], lightSample[1], *lightNum,
					bsdfSample[0], bsdfSample[1], *bsdfComponent);

			if (!Ll.Black()) {
				L[light->group] += Ll * scale;
				++nContribs;
			}
		}

		// Use lightCDF for any left shadow ray
		const SWCSpectrum newScale = scale / (shadowRayCount - nLights);
		for (u_int i = nLights; i < shadowRayCount; ++i) {
			const u_int offset = i * sampleCount;
			const float *lightSample = &sampleData[offset];
			const float *lightNum = &sampleData[offset + 2];
			const float *bsdfSample = &sampleData[offset + 3];
			const float *bsdfComponent =  &sampleData[offset + 5];

			float ls3 = *lightNum * nLights;
			float lightPdf;
			u_int lightNumber = Floor2UInt(SampleStep1d(lightPower, lightCDF,
				totalPower, nLights, *lightNum, &lightPdf) * nLights);
			lightNumber = min(lightNumber, nLights - 1);
			ls3 -= lightNumber;
			const Light *light = scene->lights[lightNumber];

			SWCSpectrum Ll = EstimateDirect(tspack, scene, light,
					p, n, wo, bsdf, sample, lightSample[0], lightSample[1], ls3,
					bsdfSample[0], bsdfSample[1], *bsdfComponent);

			if (!Ll.Black()) {
				Ll *=  newScale / lightPdf;
				L[light->group] += Ll;
				++nContribs;
			}
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
	lightPower = new float[nLights];
	lightCDF = new float[nLights + 1];

	// Averge the light power
	for (u_int i = 0; i < nLights; ++i) {
		const Light *l = scene->lights[i];
		lightPower[i] = logf(l->GetRenderingHints()->GetImportance() * l->Power(scene));
	}

	ComputeStep1dCDF(lightPower, nLights, &totalPower, lightCDF);
}

//------------------------------------------------------------------------------
// SurfaceIntegrator Rendering Hints
//------------------------------------------------------------------------------

SurfaceIntegratorSupportedStrategies::SurfaceIntegratorSupportedStrategies() {
	this->AddStrategy(LightsSamplingStrategy::SAMPLE_ALL_UNIFORM);
	this->AddStrategy(LightsSamplingStrategy::SAMPLE_ONE_UNIFORM);
	this->AddStrategy(LightsSamplingStrategy::SAMPLE_AUTOMATIC);
	this->AddStrategy(LightsSamplingStrategy::SAMPLE_ONE_IMPORTANCE);
	this->AddStrategy(LightsSamplingStrategy::SAMPLE_ONE_POWER_IMPORTANCE);
	this->AddStrategy(LightsSamplingStrategy::SAMPLE_ALL_POWER_IMPORTANCE);
	this->AddStrategy(LightsSamplingStrategy::SAMPLE_ONE_LOG_POWER_IMPORTANCE);
	// Set the defualt strategy supported
	this->SetDefaultStrategy(LightsSamplingStrategy::SAMPLE_AUTOMATIC);

	this->AddStrategy(RussianRouletteStrategy::NONE);
	this->AddStrategy(RussianRouletteStrategy::EFFICIENCY);
	this->AddStrategy(RussianRouletteStrategy::PROBABILITY);
	this->AddStrategy(RussianRouletteStrategy::IMPORTANCE);
	// Set the defualt strategy supported
	this->SetDefaultStrategy(RussianRouletteStrategy::EFFICIENCY);
}

void SurfaceIntegratorRenderingHints::InitParam(const ParamSet &params) {
	shadowRayCount = max(params.FindOneInt("shadowraycount", 1), 1);

	// Light Strategy
	if (supportedStrategies.GetDefaultLightSamplingStrategy() != LightsSamplingStrategy::NOT_SUPPORTED) {
		// For sompatibility with past versions
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

		// Check if it is one of the supported Light Sampling strategies
		if (!supportedStrategies.IsSupported(lightStrategyType)) {
			std::stringstream ss;
			ss << "Strategy  '" << st << "' for light sampling is unsupported by this integrator. Using the default.";
			luxError(LUX_BADTOKEN, LUX_WARNING, ss.str().c_str());

			lightStrategyType = supportedStrategies.GetDefaultLightSamplingStrategy();
		}

		// Create the light strategy
		switch (lightStrategyType) {
			case LightsSamplingStrategy::NOT_SUPPORTED:
				lsStrategy = NULL;
				break;
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
			default:
				BOOST_ASSERT(false);
		}
		if (lsStrategy)
			lsStrategy->InitParam(params);
	}

	// RR Strategy
	if (supportedStrategies.GetDefaultRussianRouletteStrategy() != RussianRouletteStrategy::NOT_SUPPORTED) {
		string st = params.FindOneString("rrstrategy", "efficiency");
		if (st == "none") rrStrategyType = RussianRouletteStrategy::NONE;
		else if (st == "efficiency") rrStrategyType = RussianRouletteStrategy::EFFICIENCY;
		else if (st == "probability") rrStrategyType = RussianRouletteStrategy::PROBABILITY;
		else if (st == "importance") rrStrategyType = RussianRouletteStrategy::IMPORTANCE;
		else {
			std::stringstream ss;
			ss << "Strategy  '" << st << "' for russian roulette unknown. Using \"efficiency\".";
			luxError(LUX_BADTOKEN, LUX_WARNING, ss.str().c_str());
			rrStrategyType = RussianRouletteStrategy::EFFICIENCY;
		}

		// Check if it is one of the supported RR strategies
		if (!supportedStrategies.IsSupported(rrStrategyType)) {
			std::stringstream ss;
			ss << "Strategy  '" << st << "' for russian roulette is unsupported by this integrator. Using the default.";
			luxError(LUX_BADTOKEN, LUX_WARNING, ss.str().c_str());

			rrStrategyType = supportedStrategies.GetDefaultRussianRouletteStrategy();
		}

		// Create the RR strategy
		switch (rrStrategyType) {
			case RussianRouletteStrategy::NOT_SUPPORTED:
				rrStrategy = NULL;
				break;
			case RussianRouletteStrategy::NONE:
				rrStrategy = new RRNoneStrategy();
				break;
			case RussianRouletteStrategy::EFFICIENCY:
				rrStrategy =  new RREfficiencyStrategy();
				break;
			case RussianRouletteStrategy::PROBABILITY:
				rrStrategy =  new RRProbabilityStrategy();
				break;
			case RussianRouletteStrategy::IMPORTANCE:
				rrStrategy =  new RRImportanceStrategy();
				break;
			default:
				BOOST_ASSERT(false);
		}
		if (rrStrategy != NULL)
			rrStrategy->InitParam(params);
	}
}

void SurfaceIntegratorRenderingHints::InitStrategies(const Scene *scene) {
	if (lsStrategy != NULL)
		lsStrategy->Init(scene);

	if (rrStrategy != NULL)
		rrStrategy->Init(scene);
}

void SurfaceIntegratorRenderingHints::RequestSamples(const Scene *scene, vector<u_int> &structure) {
	if (lsStrategy != NULL) {
		lightSampleOffset = structure.size();
		// Request samples for each shadow ray we have to trace
		for (u_int i = 0; i <  shadowRayCount; ++i)
			lsStrategy->RequestSamples(scene, structure);
	}

	if (rrStrategy != NULL) {
		rrSampleOffset = structure.size();
		rrStrategy->RequestSamples(scene, structure);
	}
}
