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
 *   Lux Renderer website : http://www.luxrender.org                       *
 ***************************************************************************/

// exphotonmap.cpp*
#ifndef LUX_EXPHOTONMAP_H
#define LUX_EXPHOTONMAP_H


#include "lux.h"
#include "transport.h"
#include "photonmap.h"
#include "renderinghints.h"

namespace lux
{

class ExPhotonIntegrator : public SurfaceIntegrator {
public:
	// ExPhotonIntegrator types
	enum RenderingMode { RM_DIRECTLIGHTING, RM_PATH };

	// ExPhotonIntegrator Public Methods
	ExPhotonIntegrator( RenderingMode rm, u_int ndir, u_int ncaus,
		u_int nindir, u_int nrad, u_int nLookup, u_int mdepth,
		u_int mpdepth, float maxdist, bool finalGather,
		u_int gatherSamples, float ga, PhotonMapRRStrategy rrstrategy,
		float rrcontprob, float distThreshold, string *mapsFileName,
		bool dbgEnableDirect, bool dbgEnableDirectMap,
		bool dbgEnableCaustic, bool dbgEnableIndirect,
		bool dbgEnableSpecular);
	virtual ~ExPhotonIntegrator();

	virtual u_int Li(const Scene &scene, const Sample &sample) const;
	virtual void RequestSamples(Sampler *sampler, const Scene &scene);
	virtual void Preprocess(const RandomGenerator &rng, const Scene &scene);

	static SurfaceIntegrator *CreateSurfaceIntegrator(const ParamSet &params);
private:
	SWCSpectrum LiDirectLightingMode(const Scene &scene,
		const Sample &sample, const Volume *volume, bool scattered,
		const Ray &ray, float *alpha,
		const u_int reflectionDepth, const bool specularBounce) const;
	SWCSpectrum LiPathMode(const Scene &scene, const Sample &sample,
		const Ray &ray, float *alpha) const;

	// ExPhotonIntegrator Private Data
	SurfaceIntegratorRenderingHints hints;

	RenderingMode renderingMode;
	u_int nDirectPhotons, nCausticPhotons, nIndirectPhotons,
	      nRadiancePhotons;
	u_int nLookup;
	u_int maxDepth, maxPhotonDepth;
	float maxDistSquared;

	bool finalGather;
	float cosGatherAngle;
	u_int gatherSamples;
	PhotonMapRRStrategy rrStrategy;
	float rrContinueProbability;
	float distanceThreshold;

	// Dade - != NULL if I have to read/write photon maps on file
	string *mapsFileName;

	// Dade - debug flags
	bool debugEnableDirect, debugUseRadianceMap, debugEnableCaustic,
		debugEnableIndirect, debugEnableSpecular;

	u_int bufferId;

	// Declare sample parameters for light source sampling
	u_int sampleOffset;
	u_int sampleFinalGather1Offset;
	u_int sampleFinalGather2Offset;

	LightPhotonMap *causticMap;
	LightPhotonMap *indirectMap;
	RadiancePhotonMap *radianceMap;
};

}//namespace lux

#endif
