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
 *   Lux Renderer website : http://www.luxrender.org                       *
 ***************************************************************************/

// bidirectional.cpp*
#include "lux.h"
#include "sampling.h"
#include "transport.h"
#include "reflection/bxdf.h"

namespace lux
{

class BidirPathState : public SurfaceIntegratorState {
public:
	enum PathState {
		TO_INIT, TRACE_SHADOWRAYS, TERMINATE
	};

	BidirPathState(const Scene &scene, ContributionBuffer *contribBuffer, RandomGenerator *rng);
	~BidirPathState() {	}

	bool Init(const Scene &scene);
	void Free(const Scene &scene);

	friend class BidirIntegrator;

private:
	struct BidirStateVertex {
		BidirStateVertex() : bsdf(NULL), flags(BxDFType(0)), throughputWi(1.f), throughputWo(1.f) {}

		BSDF *bsdf;
		BxDFType flags;

		// TOFIX: wi is available also inside the bsdf
		Vector wi, wo;
		SWCSpectrum throughputWi, throughputWo;
	};

	void Terminate(const Scene &scene, const u_int eyeBufferId, const u_int lightBufferId);

	// NOTE: the size of this class is extremely important for the total
	// amount of memory required for hybrid rendering.

	Sample sample;

	BidirStateVertex *eyePath;
	u_int eyePathLength;

	const Light *light;
	SWCSpectrum Le;
	BidirStateVertex *lightPath;
	u_int lightPathLength;

	// One for each eye path vertex
	SWCSpectrum *Ld;
	u_int *LdGroup;

	// One for each connection between eye path and light path
	SWCSpectrum *Lc;

	// One for each light path vertex (used for direct connection to the eye)
	SWCSpectrum *LlightPath;
	float *distanceLightPath;
	float *imageXYLightPath;

	u_int *raysIndex; // TODO: this can be replaced by just a single u_int (the first one)
	u_int raysCount;

	float distance, alpha;
	// One for each light group
	SWCSpectrum *L;
	float *V;
	u_int contribCount;

	PathState state;
};

// Bidirectional Local Declarations
class BidirIntegrator : public SurfaceIntegrator {
public:
	// PathIntegrator types
	enum LightStrategy { SAMPLE_ALL_UNIFORM, SAMPLE_ONE_UNIFORM,
		SAMPLE_AUTOMATIC
	};
//	enum RRStrategy { RR_EFFICIENCY, RR_PROBABILITY, RR_NONE };

	BidirIntegrator(u_int ed, u_int ld, float et, float lt, LightStrategy ls,
		bool d) : SurfaceIntegrator(),
		maxEyeDepth(ed), maxLightDepth(ld),
		eyeThreshold(et), lightThreshold(lt),
		lightStrategy(ls), debug(d) {
		eyeBufferId = 0;
		lightBufferId = 0;
		AddStringConstant(*this, "name", "Name of current surface integrator", "bidirectional");
	}
	virtual ~BidirIntegrator() { }
	// BidirIntegrator Public Methods
	virtual u_int Li(const Scene &scene, const Sample &sample) const;
	virtual void RequestSamples(Sample *sample, const Scene &scene);
	virtual void Preprocess(const RandomGenerator &rng, const Scene &scene);

	//--------------------------------------------------------------------------
	// DataParallel interface
	//--------------------------------------------------------------------------

	virtual bool IsDataParallelSupported() const { return true; }

	virtual bool CheckLightStrategy() const {
		if (lightStrategy != SAMPLE_ONE_UNIFORM) {
			LOG(LUX_ERROR, LUX_SEVERE)<< "The (light) strategy must be ONE_UNIFORM.";
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

	friend class BidirPathState;

	u_int maxEyeDepth, maxLightDepth;
	float eyeThreshold, lightThreshold;
	u_int sampleEyeOffset, sampleLightOffset;
	u_int eyeBufferId, lightBufferId;

private:
	// BidirIntegrator Data
	LightStrategy lightStrategy;
	u_int lightNumOffset, lightComponentOffset;
	u_int lightPosOffset, lightDirOffset, sampleDirectOffset;
	bool debug;
};

}//namespace lux
