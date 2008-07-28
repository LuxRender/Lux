/***************************************************************************
 *   Copyright (C) 1998-2008 by authors (see AUTHORS.txt )                 *
 *                                                                         *
 *   This file is part of Lux Renderer.                                    *
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
#include "scene.h"
#include "kdtree.h"
#include "mc.h"
#include "sampling.h"
#include "photonmap.h"

namespace lux
{

class ExPhotonIntegrator : public SurfaceIntegrator {
public:
	// ExPhotonIntegrator types
	enum RenderingMode { RM_DIRECTLIGHTING, RM_PATH };
	enum LightStrategy {
		SAMPLE_ALL_UNIFORM, SAMPLE_ONE_UNIFORM,
		SAMPLE_AUTOMATIC
	};

	// ExPhotonIntegrator Public Methods
	ExPhotonIntegrator(
			RenderingMode rm,
			LightStrategy st,
			int ncaus, int nindir,  int maxDirPhotons,
			int nLookup, int mdepth,
			float maxdist, bool finalGather, int gatherSamples, float ga,
			PhotonMapRRStrategy rrstrategy, float rrcontprob,
			string *mapsFileName,
			bool dbgEnableDirect, bool dbgEnableCaustic,
			bool dbgEnableIndirect, bool dbgEnableSpecular);
	~ExPhotonIntegrator();

	SWCSpectrum Li(const Scene *scene, const RayDifferential &ray,
			const Sample *sample, float *alpha) const;
	void RequestSamples(Sample *sample, const Scene *scene);
	void Preprocess(const Scene *);
	virtual ExPhotonIntegrator* clone() const; // Lux (copy) constructor for multithreading

	virtual bool IsSWCSupported() {
		return false;
	}

	IntegrationSampler* HasIntegrationSampler(IntegrationSampler *is) {
		return NULL;
	}; // Not implemented

	static SurfaceIntegrator *CreateSurfaceIntegrator(const ParamSet &params);

private:

    SWCSpectrum LiDirectLigthtingMode(const int specularDepth, const Scene *scene,
            const RayDifferential &ray, const Sample *sample,
            float *alpha) const;
    void LiPathMode(const Scene *scene,
            const RayDifferential &ray, const Sample *sample,
            float *alpha) const;

	// ExPhotonIntegrator Private Data
	RenderingMode renderingMode;
	LightStrategy lightStrategy;
	u_int nCausticPhotons, nIndirectPhotons, maxDirectPhotons;
	u_int nLookup;
	int maxDepth;
	float maxDistSquared;

	bool finalGather;
	float cosGatherAngle;
	int gatherSamples;
	PhotonMapRRStrategy rrStrategy;
	float rrContinueProbability;

	// Dade - != NULL if I have to read/write photon maps on file
	string *mapsFileName;

	// Dade - debug flags
	bool debugEnableDirect, debugEnableCaustic,
			debugEnableIndirect, debugEnableSpecular;

	// Declare sample parameters for light source sampling
	int sampleOffset;
	int sampleFinalGather1Offset;
	int sampleFinalGather2Offset;

	mutable LightPhotonMap *causticMap;
	mutable LightPhotonMap *indirectMap;
	mutable RadiancePhotonMap *radianceMap;
};

}//namespace lux

#endif
