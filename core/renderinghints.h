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

//------------------------------------------------------------------------------
// Light Rendering Hints
//------------------------------------------------------------------------------

class LightRenderingHints {
public:
	LightRenderingHints() {
		importance = 1.f;
	}
	void Init(const ParamSet &params);

	float GetImportance() const { return importance; }

private:
	float importance;
};

//------------------------------------------------------------------------------
// Light Sampling Strategies
//------------------------------------------------------------------------------

class LightStrategy {
public:
	enum LightStrategyType {
		SAMPLE_ALL_UNIFORM, SAMPLE_ONE_UNIFORM,
		SAMPLE_AUTOMATIC, SAMPLE_ONE_IMPORTANCE,
		SAMPLE_ONE_POWER_IMPORTANCE
	};

	LightStrategy() { }

	virtual void RequestSamples(vector<u_int> &structure) const = 0;
	virtual u_int RequestSamplesCount() const = 0;

	// Note: results are added to L
	virtual u_int SampleLights(
		const TsPack *tspack, const Scene *scene,
		const Point &p, const Normal &n, const Vector &wo, BSDF *bsdf,
		const Sample *sample, const float *sampleData, const SWCSpectrum &scale,
		vector<SWCSpectrum> &L) const = 0;

};

class LightStrategyAllUniform : public LightStrategy {
public:
	LightStrategyAllUniform() { }

	virtual void RequestSamples(vector<u_int> &structure) const;
	virtual u_int RequestSamplesCount() const { return 6; }
	virtual u_int SampleLights(
		const TsPack *tspack, const Scene *scene,
		const Point &p, const Normal &n, const Vector &wo, BSDF *bsdf,
		const Sample *sample, const float *sampleData, const SWCSpectrum &scale,
		vector<SWCSpectrum> &L) const;

};

class LightStrategyOneUniform : public LightStrategy {
public:
	LightStrategyOneUniform() { }

	virtual void RequestSamples(vector<u_int> &structure) const;
	virtual u_int RequestSamplesCount() const { return 6; }
	virtual u_int SampleLights(
		const TsPack *tspack, const Scene *scene,
		const Point &p, const Normal &n, const Vector &wo, BSDF *bsdf,
		const Sample *sample, const float *sampleData, const SWCSpectrum &scale,
		vector<SWCSpectrum> &L) const;
};

class LightStrategyOneImportance : public LightStrategy {
public:
	LightStrategyOneImportance(const Scene *scene);
	~LightStrategyOneImportance() {
		delete lightImportance;
		delete lightCDF;
	}

	virtual void RequestSamples(vector<u_int> &structure) const;
	virtual u_int RequestSamplesCount() const { return 6; }
	virtual u_int SampleLights(
		const TsPack *tspack, const Scene *scene,
		const Point &p, const Normal &n, const Vector &wo, BSDF *bsdf,
		const Sample *sample, const float *sampleData, const SWCSpectrum &scale,
		vector<SWCSpectrum> &L) const;

private:
	float *lightImportance;
	float totalImportance;
	float *lightCDF;
};

class LightStrategyOnePowerImportance : public LightStrategy {
public:
	LightStrategyOnePowerImportance(const Scene *scene);
	~LightStrategyOnePowerImportance() {
		delete lightPower;
		delete lightCDF;
	}

	virtual void RequestSamples(vector<u_int> &structure) const;
	virtual u_int RequestSamplesCount() const { return 6; }
	// Note: results are added to L
	virtual u_int SampleLights(
		const TsPack *tspack, const Scene *scene,
		const Point &p, const Normal &n, const Vector &wo, BSDF *bsdf,
		const Sample *sample, const float *sampleData, const SWCSpectrum &scale,
		vector<SWCSpectrum> &L) const;

private:
	float *lightPower;
	float totalPower;
	float *lightCDF;
};

//------------------------------------------------------------------------------
// SurfaceIntegrator Rendering Hints
//------------------------------------------------------------------------------

class SurfaceIntegratorRenderingHints {
public:
	SurfaceIntegratorRenderingHints() {
		shadowRayCount = 1;
		lightStrategyType = LightStrategy::SAMPLE_AUTOMATIC;
		lightStrategy = NULL;
	};
	~SurfaceIntegratorRenderingHints() {
		if (lightStrategy)
			delete lightStrategy;
	}

	void Init(const ParamSet &params);

	float GetShadowRaysCount() const { return shadowRayCount; }
	LightStrategy::LightStrategyType GetLightStrategy() const { return lightStrategyType; }

	void CreateLightStrategy(const Scene *scene);
	void RequestLightSamples(vector<u_int> &structure);
	// Note: results overwrite L and optional parameter V content
	u_int SampleLights(
		const TsPack *tspack, const Scene *scene,
		const Point &p, const Normal &n, const Vector &wo, BSDF *bsdf,
		const Sample *sample, const float *sampleData, const SWCSpectrum &scale,
		vector<SWCSpectrum> &L, vector<float> *V = NULL) const {
		u_int nContribs = 0;
		const u_int sampleCount = lightStrategy->RequestSamplesCount();
		const SWCSpectrum newScale = scale / shadowRayCount;

		nContribs = lightStrategy->SampleLights(tspack, scene, p, n, wo, bsdf,
				sample, sampleData, newScale, L);
		for (int i = 1; i < shadowRayCount; ++i) {
			nContribs += lightStrategy->SampleLights(tspack, scene, p, n, wo, bsdf,
					sample, &sampleData[sampleCount * i], newScale, L);
		}

		if (V) {
			const float nLights = scene->lights.size();
			for (u_int i = 0; i < nLights; ++i)
				(*V)[i] += L[i].Filter(tspack);
		}

		return nContribs;
	}

private:
	int shadowRayCount;

	LightStrategy::LightStrategyType lightStrategyType;
	LightStrategy *lightStrategy;
};

}

#endif	/* _RENDERINGHINTS_H */