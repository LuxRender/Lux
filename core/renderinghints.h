/***************************************************************************
 *   Copyright (C) 1998-2013 by authors (see AUTHORS.txt)                  *
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

#include "luxrays/utils/mcdistribution.h"

namespace lux {

//******************************************************************************
// Strategies
//******************************************************************************

class Strategy {
public:
	Strategy() { }
	virtual ~Strategy() { }
	virtual void InitParam(const ParamSet &params) = 0;
	virtual void Init(const Scene &scene) = 0;
};

//------------------------------------------------------------------------------
// Light Sampling Strategies
//------------------------------------------------------------------------------

class LightsSamplingStrategy : public Strategy {
public:
	enum LightStrategyType {
		SAMPLE_ALL_UNIFORM, SAMPLE_ONE_UNIFORM,
		SAMPLE_AUTOMATIC, SAMPLE_ONE_IMPORTANCE,
		SAMPLE_ONE_POWER_IMPORTANCE, SAMPLE_ALL_POWER_IMPORTANCE, SAMPLE_AUTOMATIC_POWER_IMPORTANCE,
		SAMPLE_ONE_LOG_POWER_IMPORTANCE
	};

	LightsSamplingStrategy() : Strategy() { }
	virtual ~LightsSamplingStrategy() { }
	virtual void InitParam(const ParamSet &params) { }
	virtual void Init(const Scene &scene) { }
	/**
	 * Samples a light according to the defined strategy.
	 * The method should be called in a loop until it returns NULL.
	 * @param scene The current scene
	 * @param index The current sampling iteration
	 * @param u A pointer to a random variable in the [0,1) range,
	 * the value might be adjusted if needed so that it can be used
	 * to sample the light component
	 * @param pdf The probability of having sampled that light taking
	 * the looping process into account
	 * @return A pointer to the sampled Light or NULL if the looping is over
	 * in which case u and pdf are left untouched
	 */
	virtual const Light *SampleLight(const Scene &scene, u_int index,
		float *u, float *pdf) const = 0;
	/**
	 * The probability of sampling a given light according to the strategy
	 * @param scene The current scene
	 * @param light A pointer to the light being queried
	 * @return The requested probability
	 */
	virtual float Pdf(const Scene &scene, const Light *light) const = 0;
	/**
	 * The probability of sampling a given light according to the strategy
	 * @param scene The current scene
	 * @param light The index of the light being queried in scene.lights
	 * @return The requested probability
	 */
	virtual float Pdf(const Scene &scene, u_int light) const = 0;
	/**
	 * The maximum number of light samples in one go
	 * The looping over SampleLight will never exceed he returned value
	 * @param scene The current scene
	 * @return The maximum number of sampling events in one go
	 */
	virtual u_int GetSamplingLimit(const Scene &scene) const = 0;

	/**
	 * Static function to create a light sampling strategy from a param set
	 * It will parse the parameters, check the requested strategy and
	 * return a pointer to the new LightSamplingStrategy.
	 * @param name The parameter name to check, "strategy" will also be checked for
	 * @param params The paramset to parse
	 * @return a pointer to the new light sampling strategy
	 */
	static LightsSamplingStrategy *Create(const string &name,
		const ParamSet &params);
};

class LSSAllUniform : public LightsSamplingStrategy {
public:
	LSSAllUniform() : LightsSamplingStrategy() { }
	virtual ~LSSAllUniform() { }
	virtual const Light *SampleLight(const Scene &scene, u_int index,
		float *u, float *pdf) const;
	virtual float Pdf(const Scene &scene, const Light *light) const;
	virtual float Pdf(const Scene &scene, u_int light) const;
	virtual u_int GetSamplingLimit(const Scene &scene) const;
};

class LSSOneUniform : public LightsSamplingStrategy {
public:
	LSSOneUniform() : LightsSamplingStrategy() { }
	virtual ~LSSOneUniform() { }
	virtual const Light *SampleLight(const Scene &scene, u_int index,
		float *u, float *pdf) const;
	virtual float Pdf(const Scene &scene, const Light *light) const;
	virtual float Pdf(const Scene &scene, u_int light) const;
	virtual u_int GetSamplingLimit(const Scene &scene) const { return 1; }
};

class LSSAuto : public LightsSamplingStrategy {
public:
	LSSAuto() : LightsSamplingStrategy(), strategy(NULL) { }
	virtual ~LSSAuto() { delete strategy; }
	virtual void Init(const Scene &scene);

	virtual const Light *SampleLight(const Scene &scene, u_int index,
		float *u, float *pdf) const {
		return strategy->SampleLight(scene, index, u, pdf);
	}
	virtual float Pdf(const Scene &scene, const Light *light) const {
		return strategy->Pdf(scene, light);
	}
	virtual float Pdf(const Scene &scene, u_int light) const {
		return strategy->Pdf(scene, light);
	}
	virtual u_int GetSamplingLimit(const Scene &scene) const {
		return strategy->GetSamplingLimit(scene);
	}

private:
	LightsSamplingStrategy *strategy;
};

class LSSOneImportance : public LightsSamplingStrategy {
public:
	LSSOneImportance() :
		LightsSamplingStrategy(), lightDistribution(NULL) { }
	virtual ~LSSOneImportance();
	virtual void Init(const Scene &scene);

	virtual const Light *SampleLight(const Scene &scene, u_int index,
		float *u, float *pdf) const;
	virtual float Pdf(const Scene &scene, const Light *light) const;
	virtual float Pdf(const Scene &scene, u_int light) const;
	virtual u_int GetSamplingLimit(const Scene &scene) const { return 1; }

protected:
	luxrays::Distribution1D *lightDistribution;
};

class LSSOnePowerImportance : public LSSOneImportance {
public:
	LSSOnePowerImportance() : LSSOneImportance() { }
	virtual ~LSSOnePowerImportance() {}
	virtual void Init(const Scene &scene);
};

class LSSAllPowerImportance : public LSSOnePowerImportance {
public:
	LSSAllPowerImportance() : LSSOnePowerImportance() { }
	virtual ~LSSAllPowerImportance() { }
	// Note: results are added to L
	virtual const Light *SampleLight(const Scene &scene, u_int index,
		float *u, float *pdf) const;
	virtual float Pdf(const Scene &scene, const Light *light) const;
	virtual float Pdf(const Scene &scene, u_int light) const;
	virtual u_int GetSamplingLimit(const Scene &scene) const;
};
	
class LSSAutoPowerImportance : public LightsSamplingStrategy {
public:
	LSSAutoPowerImportance() : LightsSamplingStrategy(), strategy(NULL) { }
	virtual ~LSSAutoPowerImportance() { delete strategy; }
	virtual void Init(const Scene &scene);

	virtual const Light *SampleLight(const Scene &scene, u_int index,
		float *u, float *pdf) const {
		return strategy->SampleLight(scene, index, u, pdf);
	}
	virtual float Pdf(const Scene &scene, const Light *light) const {
		return strategy->Pdf(scene, light);
	}
	virtual float Pdf(const Scene &scene, u_int light) const {
		return strategy->Pdf(scene, light);
	}
	virtual u_int GetSamplingLimit(const Scene &scene) const {
		return strategy->GetSamplingLimit(scene);
	}

private:
	LightsSamplingStrategy *strategy;
};

class LSSOneLogPowerImportance : public LSSOnePowerImportance {
public:
	LSSOneLogPowerImportance() : LSSOnePowerImportance() { }
	virtual ~LSSOneLogPowerImportance() { }
	virtual void Init(const Scene &scene);
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
		nLights = 0;
		lsStrategy = NULL;
	}
	~SurfaceIntegratorRenderingHints() {
		delete lsStrategy;
	}

	void InitParam(const ParamSet &params);

	u_int GetShadowRaysCount() const { return shadowRayCount; }
	/**
	 * Samples a light according to the defined strategy.
	 * The method should be called in a loop until it returns NULL.
	 * @param scene The current scene
	 * @param index The current sampling iteration
	 * @param u A pointer to a random variable in the [0,1) range,
	 * the value might be adjusted if needed so that it can be used
	 * to sample the light component
	 * @param pdf The probability of having sampled that light taking
	 * the looping process into account
	 * @return A pointer to the sampled Light or NULL if the looping is over
	 * in which case u and pdf are left untouched
	 */
	const Light *SampleLight(const Scene &scene, u_int index,
		float *u, float *pdf) const {
		return lsStrategy->SampleLight(scene, index, u, pdf);
	}
	/**
	 * The probability of sampling a given light according to the strategy
	 * @param scene The current scene
	 * @param light A pointer to the light being queried
	 * @return The requested probability
	 */
	float Pdf(const Scene &scene, const Light *light) const {
		return lsStrategy->Pdf(scene, light);
	}
	/**
	 * The probability of sampling a given light according to the strategy
	 * @param scene The current scene
	 * @param light The index of the light being queried in scene.lights
	 * @return The requested probability
	 */
	float Pdf(const Scene &scene, u_int light) const {
		return lsStrategy->Pdf(scene, light);
	}
	/**
	 * The maximum number of light samples in one go
	 * The looping over SampleLight will never exceed he returned value
	 * @param scene The current scene
	 * @return The maximum number of sampling events in one go
	 */
	u_int GetSamplingLimit(const Scene &scene) const {
		return lsStrategy->GetSamplingLimit(scene);
	}
	
	const LightsSamplingStrategy *GetLightsSamplingStrategy() const { return lsStrategy; };

	void InitStrategies(const Scene &scene);
	void RequestSamples(Sampler *sampler, const Scene &scene, u_int maxDepth);

	// Note: results are added to L and optional parameter V content
	u_int SampleLights(const Scene &scene, const Sample &sample,
		const Point &p, const Normal &n, const Vector &wo, BSDF *bsdf,
		u_int depth, const SWCSpectrum &scale,
		vector<SWCSpectrum> &L, vector<float> *V = NULL) const;

	//FIXME: temporary until the implementation of DataParallel interface
	friend class PathIntegrator;
private:
	// Light Strategies
	u_int shadowRayCount, nLights;
	LightsSamplingStrategy *lsStrategy;
	u_int lightSampleOffset;
};

}

#endif	/* _RENDERINGHINTS_H */
