/***************************************************************************
 *   Aldo Zang & Luiz Velho						   *
 *   Augmented Reality Project : http://w3.impa.br/~zang/arlux/ 	   *
 *   Visgraf-Impa Lab 2010       http://www.visgraf.impa.br/ 		   *
 *									   *
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
#include "lux.h"
#include "transport.h"
#include "renderinghints.h"

namespace lux
{

class ARPathState : public SurfaceIntegratorState {
public:
	enum ARPathStateType {
		TO_INIT, EYE_VERTEX, NEXT_VERTEX, CONTINUE_SHADOWRAY, TERMINATE
	};

	ARPathState(const Scene &scene, ContributionBuffer *contribBuffer, RandomGenerator *rng);
	~ARPathState() { }

	bool Init(const Scene &scene);
	void Free(const Scene &scene);

	friend class ARPathIntegrator;

private:
	void Terminate(const Scene &scene, const u_int bufferId,
		const float alpha = 1.f);

	ARPathStateType GetState() const {
		return (ARPathStateType)pathState;
	}

	void SetState(const ARPathStateType s) {
		pathState = s;
	}

#define ARPathState_FLAGS_SPECULARBOUNCE (1<<0)
#define ARPathState_FLAGS_SPECULAR (1<<1)
#define ARPathState_FLAGS_SCATTERED (1<<2)

	bool GetSpecularBounce() const {
		return (flags & ARPathState_FLAGS_SPECULARBOUNCE) != 0;
	}

	void SetSpecularBounce(const bool v) {
		flags = v ? (flags | ARPathState_FLAGS_SPECULARBOUNCE) : (flags & ~ARPathState_FLAGS_SPECULARBOUNCE);
	}

	bool GetSpecular() const {
		return (flags & ARPathState_FLAGS_SPECULAR) != 0;
	}

	void SetSpecular(const bool v) {
		flags = v ? (flags | ARPathState_FLAGS_SPECULAR) : (flags & ~ARPathState_FLAGS_SPECULAR);
	}

	bool GetScattered() const {
		return (flags & ARPathState_FLAGS_SCATTERED) != 0;
	}

	void SetScattered(const bool v) {
		flags = v ? (flags | ARPathState_FLAGS_SCATTERED) : (flags & ~ARPathState_FLAGS_SCATTERED);
	}

	// NOTE: the size of this class is extremely important for the total
	// amount of memory required for hybrid rendering.

	Sample sample;

	// Path status information
	float distance;
	float VContrib;
	SWCSpectrum pathThroughput;
	const Volume *volume;
	SWCSpectrum *L;
	float *V;

	// Next path vertex ray
	Ray pathRay;
	luxrays::RayHit pathRayHit; // Used when in  CONTINUE_SHADOWRAY state
	u_int currentPathRayIndex;

	// Direct lighting
	SWCSpectrum *Ld;
	float *Vd;
	u_int *LdGroup;

	// Direct light sampling rays
	Ray *shadowRay;
	u_int *currentShadowRayIndex;
	const Volume **shadowVolume;

	float bouncePdf;
	Point lastBounce;

	u_short pathLength;
	// Use Get/SetState to access this
	u_short pathState;
	u_short tracedShadowRayCount;
	// Used to save memory and store:
	//  specularBounce (1bit)
	//  specular (1bit)
	//  scattered (1bit)
	// Use Get/SetState to access this
	u_short flags;
};

// ARPathIntegrator Declarations
class ARPathIntegrator : public SurfaceIntegrator {
public:
	// ARPathIntegrator types
	enum RRStrategy { RR_EFFICIENCY, RR_PROBABILITY, RR_NONE };

	// PathIntegrator Public Methods
	ARPathIntegrator(RRStrategy rst, u_int md, float cp, bool ie, bool dls) : SurfaceIntegrator(),
		hints(), rrStrategy(rst), maxDepth(md), continueProbability(cp),
		sampleOffset(0), bufferId(0), includeEnvironment(ie), enableDirectLightSampling(dls) {
		AddStringConstant(*this, "name", "Name of current surface integrator", "arpath");
	}

	virtual u_int Li(const Scene &scene, const Sample &sample) const;
	virtual void RequestSamples(Sample *sample, const Scene &scene);
	virtual void Preprocess(const RandomGenerator &rng, const Scene &scene);

	// DataParallel interface
	virtual bool IsDataParallelSupported() const { return true; }
	//FIXME: just to check SurfaceIntegratorRenderingHints light strategy, to remove
	virtual bool CheckLightStrategy() const {
		if ((hints.GetLightStrategy() != LightsSamplingStrategy::SAMPLE_ONE_UNIFORM) &&
			(hints.GetLightStrategy() != LightsSamplingStrategy::SAMPLE_ALL_UNIFORM) &&
			(hints.GetLightStrategy() != LightsSamplingStrategy::SAMPLE_AUTOMATIC)) {
			LOG(LUX_ERROR, LUX_SEVERE)<< "The LightsSamplingStrategy must be ONE_UNIFORM or ALL_UNIFORM or AUTO.";
			return false;
		}

		return true;
	}
	virtual SurfaceIntegratorState *NewState(const Scene &scene,
		ContributionBuffer *contribBuffer, RandomGenerator *rng);
	virtual bool GenerateRays(const Scene &scene,
		SurfaceIntegratorState *state, luxrays::RayBuffer *rayBuffer);
	virtual bool NextState(const Scene &scene, SurfaceIntegratorState *state,
		luxrays::RayBuffer *rayBuffer, u_int *nrContribs);

	static SurfaceIntegrator *CreateSurfaceIntegrator(const ParamSet &params);

	friend class ARPathState;

private:
	// Used by DataParallel methods
	void BuildShadowRays(const Scene &scene, ARPathState *ARPathState, BSDF *bsdf);

	SurfaceIntegratorRenderingHints hints;

	// PathIntegrator Private Data
	RRStrategy rrStrategy;
	u_int maxDepth;
	float continueProbability;
	// Declare sample parameters for light source sampling
	u_int sampleOffset, bufferId;

	// Used only for HybridSampler
	u_int hybridRendererLightSampleOffset;
	LightsSamplingStrategy::LightStrategyType hybridRendererLightStrategy;

	bool includeEnvironment, enableDirectLightSampling;
};

}//namespace lux
