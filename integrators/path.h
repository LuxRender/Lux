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
#include "lux.h"
#include "transport.h"
#include "renderinghints.h"

namespace lux
{

class PathState : public SurfaceIntegratorState {
public:
	enum PathStateType {
		TO_INIT, EYE_VERTEX, NEXT_VERTEX, CONTINUE_SHADOWRAY, TERMINATE
	};

	PathState(const Scene &scene, ContributionBuffer *contribBuffer, RandomGenerator *rng);
	~PathState() { }

	bool Init(const Scene &scene);
	void Free(const Scene &scene);

	friend class PathIntegrator;

private:
	void Terminate(const Scene &scene, const u_int bufferId,
			const float alpha = 1.f);

	PathStateType GetState() const {
		return (PathStateType)pathState;
	}

	void SetState(const PathStateType s) {
		pathState = s;
	}

#define PATHSTATE_FLAGS_SPECULARBOUNCE (1<<0)
#define PATHSTATE_FLAGS_SPECULAR (1<<1)
#define PATHSTATE_FLAGS_SCATTERED (1<<2)

	bool GetSpecularBounce() const {
		return (flags & PATHSTATE_FLAGS_SPECULARBOUNCE) != 0;
	}

	void SetSpecularBounce(const bool v) {
		flags = v ? (flags | PATHSTATE_FLAGS_SPECULARBOUNCE) : (flags & ~PATHSTATE_FLAGS_SPECULARBOUNCE);
	}

	bool GetSpecular() const {
		return (flags & PATHSTATE_FLAGS_SPECULAR) != 0;
	}

	void SetSpecular(const bool v) {
		flags = v ? (flags | PATHSTATE_FLAGS_SPECULAR) : (flags & ~PATHSTATE_FLAGS_SPECULAR);
	}

	bool GetScattered() const {
		return (flags & PATHSTATE_FLAGS_SCATTERED) != 0;
	}

	void SetScattered(const bool v) {
		flags = v ? (flags | PATHSTATE_FLAGS_SCATTERED) : (flags & ~PATHSTATE_FLAGS_SCATTERED);
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
	float xi, yi; // Hold the image coordinates of the sample
};

// PathIntegrator Declarations
class PathIntegrator : public SurfaceIntegrator {
public:
	// PathIntegrator types
	enum RRStrategy { RR_EFFICIENCY, RR_PROBABILITY, RR_NONE };

	// PathIntegrator Public Methods
	PathIntegrator(RRStrategy rst, u_int md, float cp, bool ie, bool dls) : SurfaceIntegrator(),
		bufferId(0), hints(), rrStrategy(rst), maxDepth(md), continueProbability(cp),
		sampleOffset(0), includeEnvironment(ie), enableDirectLightSampling(dls) {
		AddStringConstant(*this, "name", "Name of current surface integrator", "path");
		AddIntAttribute(*this, "maxDepth", "Path max. depth", &PathIntegrator::GetMaxDepth);
	}

	virtual u_int Li(const Scene &scene, const Sample &sample) const;
	virtual void RequestSamples(Sampler *sampler, const Scene &scene);
	virtual void Preprocess(const RandomGenerator &rng, const Scene &scene);

	// DataParallel interface
	virtual bool IsDataParallelSupported() const { return true; }
	//FIXME: just to check SurfaceIntegratorRenderingHints light strategy, to remove
	virtual bool CheckLightStrategy(const Scene &scene) const {
		return true;
	}
	virtual SurfaceIntegratorState *NewState(const Scene &scene,
		ContributionBuffer *contribBuffer, RandomGenerator *rng);
	virtual bool GenerateRays(const Scene &scene,
		SurfaceIntegratorState *state, luxrays::RayBuffer *rayBuffer);
	virtual bool NextState(const Scene &scene, SurfaceIntegratorState *state,
		luxrays::RayBuffer *rayBuffer, u_int *nrContribs);

	static SurfaceIntegrator *CreateSurfaceIntegrator(const ParamSet &params);

	friend class PathState;

	u_int bufferId;

private:
	// Used by Queryable interface
	u_int GetMaxDepth() { return maxDepth; }

	// Used by DataParallel methods
	void BuildShadowRays(const Scene &scene, PathState *pathState, BSDF *bsdf);

	SurfaceIntegratorRenderingHints hints;

	// PathIntegrator Private Data
	RRStrategy rrStrategy;
	u_int maxDepth;
	float continueProbability;
	// Declare sample parameters for light source sampling
	u_int sampleOffset;

	// Used only for HybridSampler
	u_int hybridRendererLightSampleOffset;
	u_int samplingCount;

	bool includeEnvironment, enableDirectLightSampling;
};

}//namespace lux

