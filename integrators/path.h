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
	enum PathStateType {
		TO_INIT, EYE_VERTEX, NEXT_VERTEX, TERMINATE
	};

	PathState(const Scene &scene, ContributionBuffer *contribBuffer, RandomGenerator *rng);
	~PathState();

	bool Init(const Scene &scene);

	friend class PathIntegrator;

private:
	void Terminate(const Scene &scene, const u_int bufferId);

	Sample sample;

	// Path status information
	u_int pathLength;
	float alpha;
	float distance;
	float VContrib;
	SWCSpectrum pathThroughput;
	const Volume *volume;
	vector<SWCSpectrum> L;
	vector<float> V;

	// Next path vertex ray
	float eyeRayWeight;
	Ray pathRay;
	u_int currentPathRayIndex;

	// Direct lighting
	SWCSpectrum *Ld;
	float *Vd;
	u_int *LdGroup;
	// Direct light sampling rays
	u_int tracedShadowRayCount;
	Ray *shadowRay;
	u_int *currentShadowRayIndex;

	PathStateType state;
	bool specularBounce, specular;
};

// PathIntegrator Declarations
class PathIntegrator : public SurfaceIntegrator {
public:
	// PathIntegrator types
	enum RRStrategy { RR_EFFICIENCY, RR_PROBABILITY, RR_NONE };

	// PathIntegrator Public Methods
	PathIntegrator(RRStrategy rst, u_int md, float cp, bool ie)  :
		hints(), rrStrategy(rst), maxDepth(md), continueProbability(cp),
		sampleOffset(0), bufferId(0), includeEnvironment(ie) { }

	virtual u_int Li(const Scene &scene, const Sample &sample) const;
	virtual void RequestSamples(Sample *sample, const Scene &scene);
	virtual void Preprocess(const RandomGenerator &rng, const Scene &scene);

	// DataParallel interface
	virtual bool IsDataParallelSupported() const { return true; }
	//FIXME: just to check SurfaceIntegratorRenderingHints light strategy, to remove
	virtual bool CheckLightStrategy() const {
		if (hints.GetLightStrategy() != LightsSamplingStrategy::SAMPLE_ONE_UNIFORM) {
			luxError(LUX_ERROR, LUX_SEVERE, "The LightsSamplingStrategy must be ONE_UNIFORM.");
			return false;
		}

		if (hints.GetShadowRaysCount() != 1) {
			luxError(LUX_ERROR, LUX_SEVERE, "The shadow rays count of LightsSamplingStrategy must be 1.");
			return false;
		}

		return true;
	}
	virtual SurfaceIntegratorState *NewState(const Scene &,
		ContributionBuffer *contribBuffer, RandomGenerator *rng);
	virtual bool GenerateRays(const Scene &,
		SurfaceIntegratorState *state, luxrays::RayBuffer *rayBuffer);
	virtual bool NextState(const Scene &, SurfaceIntegratorState *state, luxrays::RayBuffer *rayBuffer, u_int *nrContribs);

	static SurfaceIntegrator *CreateSurfaceIntegrator(const ParamSet &params);

private:
	SurfaceIntegratorRenderingHints hints;

	// PathIntegrator Private Data
	RRStrategy rrStrategy;
	u_int maxDepth;
	float continueProbability;
	// Declare sample parameters for light source sampling
	u_int sampleOffset, bufferId;
	bool includeEnvironment;
};

}//namespace lux

