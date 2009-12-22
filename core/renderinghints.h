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

#ifndef _RENDERINGHINTS_H
#define	_RENDERINGHINTS_H

#include "lux.h"
#include "paramset.h"
#include "scene.h"

namespace lux {

//******************************************************************************
// Strategies
//******************************************************************************

class Strategy {
public:
	virtual void InitParam(const ParamSet &params) = 0;
	virtual void Init(const Scene *scene) = 0;
};

//------------------------------------------------------------------------------
// Light Sampling Strategies
//------------------------------------------------------------------------------

class LightsSamplingStrategy : public Strategy {
public:
	enum LightStrategyType {
		NOT_SUPPORTED, // Used in the case LS strategies are not supported at all
		SAMPLE_ALL_UNIFORM, SAMPLE_ONE_UNIFORM,
		SAMPLE_AUTOMATIC, SAMPLE_ONE_IMPORTANCE,
		SAMPLE_ONE_POWER_IMPORTANCE, SAMPLE_ALL_POWER_IMPORTANCE,
		SAMPLE_ONE_LOG_POWER_IMPORTANCE
	};

	LightsSamplingStrategy() { }
	virtual void InitParam(const ParamSet &params) { }
	virtual void Init(const Scene *scene) { }

	virtual void RequestSamples(const Scene *scene, vector<u_int> &structure) const = 0;
	virtual u_int RequestSamplesCount(const Scene *scene) const = 0;

	// Note: results are added to L
	virtual u_int SampleLights(
		const TsPack *tspack, const Scene *scene, const u_int shadowRayCount,
		const Point &p, const Normal &n, const Vector &wo, BSDF *bsdf,
		const Sample *sample, const float *sampleData, const SWCSpectrum &scale,
		vector<SWCSpectrum> &L) const = 0;
};

class LSSAllUniform : public LightsSamplingStrategy {
public:
	virtual void RequestSamples(const Scene *scene, vector<u_int> &structure) const;
	virtual u_int RequestSamplesCount(const Scene *scene) const {
		return 6;
	}
	virtual u_int SampleLights(
		const TsPack *tspack, const Scene *scene, const u_int shadowRayCount,
		const Point &p, const Normal &n, const Vector &wo, BSDF *bsdf,
		const Sample *sample, const float *sampleData, const SWCSpectrum &scale,
		vector<SWCSpectrum> &L) const;

};

class LSSOneUniform : public LightsSamplingStrategy {
public:
	virtual void RequestSamples(const Scene *scene, vector<u_int> &structure) const;
	virtual u_int RequestSamplesCount(const Scene *scene) const { return 6; }
	virtual u_int SampleLights(
		const TsPack *tspack, const Scene *scene, const u_int shadowRayCount,
		const Point &p, const Normal &n, const Vector &wo, BSDF *bsdf,
		const Sample *sample, const float *sampleData, const SWCSpectrum &scale,
		vector<SWCSpectrum> &L) const;
};

class LSSAuto : public LightsSamplingStrategy {
public:
	LSSAuto() : strategy(NULL) { }
	virtual void Init(const Scene *scene) {
		// The check for mutating sampler is here for coherence with the
		// behavior of old code: otherwise LSSAllUniform would be used
		// instead of LSSOneUniform (i.e. the number of reported samples
		// per second would be a lot lower than in the past causing a lot
		// of complain for sure)
		if (scene->sampler->IsMutating() || (scene->lights.size() > 5))
			strategy = new LSSOneUniform();
		else
			strategy = new LSSAllUniform();

		strategy->Init(scene);
	}

	virtual void RequestSamples(const Scene *scene, vector<u_int> &structure) const {
		strategy->RequestSamples(scene, structure);
	}
	virtual u_int RequestSamplesCount(const Scene *scene) const {
		return strategy->RequestSamplesCount(scene);
	}

	virtual u_int SampleLights(
		const TsPack *tspack, const Scene *scene, const u_int shadowRayCount,
		const Point &p, const Normal &n, const Vector &wo, BSDF *bsdf,
		const Sample *sample, const float *sampleData, const SWCSpectrum &scale,
		vector<SWCSpectrum> &L) const {
		return strategy->SampleLights(tspack, scene, shadowRayCount,
				p, n, wo, bsdf, sample, sampleData, scale, L);
	}

private:
	LightsSamplingStrategy *strategy;
};

class LSSOneImportance : public LightsSamplingStrategy {
public:
	~LSSOneImportance() {
		delete lightImportance;
		delete lightCDF;
	}
	virtual void Init(const Scene *scene);

	virtual void RequestSamples(const Scene *scene, vector<u_int> &structure) const;
	virtual u_int RequestSamplesCount(const Scene *scene) const { return 6; }
	virtual u_int SampleLights(
		const TsPack *tspack, const Scene *scene, const u_int shadowRayCount,
		const Point &p, const Normal &n, const Vector &wo, BSDF *bsdf,
		const Sample *sample, const float *sampleData, const SWCSpectrum &scale,
		vector<SWCSpectrum> &L) const;

private:
	float *lightImportance;
	float totalImportance;
	float *lightCDF;
};

class LSSOnePowerImportance : public LightsSamplingStrategy {
public:
	~LSSOnePowerImportance() {
		delete lightPower;
		delete lightCDF;
	}
	virtual void Init(const Scene *scene);

	virtual void RequestSamples(const Scene *scene, vector<u_int> &structure) const;
	virtual u_int RequestSamplesCount(const Scene *scene) const { return 6; }
	// Note: results are added to L
	virtual u_int SampleLights(
		const TsPack *tspack, const Scene *scene, const u_int shadowRayCount,
		const Point &p, const Normal &n, const Vector &wo, BSDF *bsdf,
		const Sample *sample, const float *sampleData, const SWCSpectrum &scale,
		vector<SWCSpectrum> &L) const;

protected:
	float *lightPower;
	float totalPower;
	float *lightCDF;
};

class LSSAllPowerImportance : public LSSOnePowerImportance {
public:
	// Note: results are added to L
	virtual u_int SampleLights(
		const TsPack *tspack, const Scene *scene, const u_int shadowRayCount,
		const Point &p, const Normal &n, const Vector &wo, BSDF *bsdf,
		const Sample *sample, const float *sampleData, const SWCSpectrum &scale,
		vector<SWCSpectrum> &L) const;
};

class LSSOneLogPowerImportance : public LSSOnePowerImportance {
public:
	virtual void Init(const Scene *scene);
};

//******************************************************************************
// Rendering Hints
//******************************************************************************

//------------------------------------------------------------------------------
// Light Rendering Hints
//------------------------------------------------------------------------------

class LightRenderingHints {
public:
	LightRenderingHints() {
		importance = 1.f;
	}
	void InitParam(const ParamSet &params);

	float GetImportance() const { return importance; }

private:
	float importance;
};

class SurfaceIntegratorRenderingHints {
public:
	SurfaceIntegratorRenderingHints() {
		shadowRayCount = 1;
		lightStrategyType = LightsSamplingStrategy::SAMPLE_AUTOMATIC;
		lsStrategy = NULL;
	}
	~SurfaceIntegratorRenderingHints() {
		if (lsStrategy)
			delete lsStrategy;
	}

	void InitParam(const ParamSet &params);

	u_int GetShadowRaysCount() const { return shadowRayCount; }
	LightsSamplingStrategy::LightStrategyType GetLightStrategy() const { return lightStrategyType; }

	void InitStrategies(const Scene *scene);
	void RequestSamples(const Scene *scene, vector<u_int> &structure);

	// Note: results are added to L and optional parameter V content
	u_int SampleLights(
		const TsPack *tspack, const Scene *scene,
		const Point &p, const Normal &n, const Vector &wo, BSDF *bsdf,
		const Sample *sample, const float *sampleData, const SWCSpectrum &scale,
		vector<SWCSpectrum> &L, vector<float> *V = NULL) const {
		const u_int nLights = scene->lights.size();
		if (nLights == 0)
			return 0;

		const u_int nContribs = lsStrategy->SampleLights(tspack, scene,
				shadowRayCount, p, n, wo, bsdf, sample, &sampleData[lightSampleOffset], scale, L);

		if (V) {
			const float nLights = scene->lights.size();
			for (u_int i = 0; i < nLights; ++i)
				(*V)[i] += L[i].Filter(tspack);
		}

		return nContribs;
	}

private:
	// Light Strategies
	u_int shadowRayCount;
	LightsSamplingStrategy::LightStrategyType lightStrategyType;
	LightsSamplingStrategy *lsStrategy;
	u_int lightSampleOffset;
};

}

#endif	/* _RENDERINGHINTS_H */
