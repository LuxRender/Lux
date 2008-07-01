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
#include "blackbody.h"

namespace lux
{

struct EClosePhoton;

// Local Declarations

struct EPhoton {
	EPhoton(const Point &pp, const Spectrum &wt, const Vector & w)
			: p(pp), alpha(wt), wi(w) {
	}

	EPhoton() {
	}

	Point p;
	Spectrum alpha;
	Vector wi;
};

struct ERadiancePhoton {
	ERadiancePhoton(const Point &pp, const Normal & nn)
			: p(pp), n(nn), Lo(0.f) {
	}

	ERadiancePhoton() {
	}

	Point p;
	Normal n;
	Spectrum Lo;
};

struct ERadiancePhotonProcess {
	// RadiancePhotonProcess Methods

	ERadiancePhotonProcess(const Point &pp, const Normal & nn)
			: p(pp), n(nn) {
		photon = NULL;
	}

	void operator()(const ERadiancePhoton &rp,
			float distSquared, float &maxDistSquared) const {
		if (Dot(rp.n, n) > 0) {
			photon = &rp;
			maxDistSquared = distSquared;
		}
	}
	const Point &p;
	const Normal &n;
	mutable const ERadiancePhoton *photon;
};

inline float Ekernel(const EPhoton *photon, const Point &p,
		float md2) {
	float s = (1.f - DistanceSquared(photon->p, p) / md2);

	return 3.f / (md2 * M_PI) * s * s;
}

struct EPhotonProcess {
	// PhotonProcess Public Methods
	EPhotonProcess(u_int mp, const Point & p);
	void operator()(const EPhoton &photon, float dist2, float &maxDistSquared) const;
	const Point &p;
	EClosePhoton *photons;
	u_int nLookup;
	mutable u_int foundPhotons;
};

struct EClosePhoton {

	EClosePhoton(const EPhoton *p = NULL,
			float md2 = INFINITY) {
		photon = p;
		distanceSquared = md2;
	}

	bool operator<(const EClosePhoton & p2) const {
		return distanceSquared == p2.distanceSquared ? (photon < p2.photon) :
				distanceSquared < p2.distanceSquared;
	}
	const EPhoton *photon;
	float distanceSquared;
};

class ExPhotonIntegrator : public SurfaceIntegrator {
public:
	// ExPhotonIntegrator types
	enum RenderingMode { RM_DIRECTLIGHTING, RM_PATH };
	enum LightStrategy {
		SAMPLE_ALL_UNIFORM, SAMPLE_ONE_UNIFORM,
		SAMPLE_AUTOMATIC
	};
	enum RRStrategy { RR_EFFICIENCY, RR_PROBABILITY, RR_NONE };

	// ExPhotonIntegrator Public Methods
	ExPhotonIntegrator(
			RenderingMode rm,
			LightStrategy st,
			int ncaus, int nindir,  int maxDirPhotons,
			int nLookup, int mdepth,
			float maxdist, bool finalGather, int gatherSamples, float ga,
			RRStrategy rrstrategy, float rrcontprob,
			bool dbgEnableDirect, bool dbgEnableCaustic,
			bool dbgEnableIndirect, bool dbgEnableSpecular);
	~ExPhotonIntegrator();

	SWCSpectrum Li(const Scene *scene, const RayDifferential &ray,
			const Sample *sample, float *alpha) const;
	void RequestSamples(Sample *sample, const Scene *scene);
	void Preprocess(const Scene *);
	virtual ExPhotonIntegrator* clone() const; // Lux (copy) constructor for multithreading

	IntegrationSampler* HasIntegrationSampler(IntegrationSampler *is) {
		return NULL;
	}; // Not implemented

	static SurfaceIntegrator *CreateSurfaceIntegrator(const ParamSet &params);

private:

	static inline bool unsuccessful(int needed, int found, int shot) {
		return (found < needed &&
				(found == 0 || found < shot / 1024));
	}

	SWCSpectrum LPhoton(KdTree<EPhoton, EPhotonProcess> *map,
			int nPaths, int nLookup, BSDF *bsdf, const Intersection &isect,
			const Vector &w, float maxDistSquared) const;
	SWCSpectrum estimateE(KdTree<EPhoton, EPhotonProcess> *map, int count,
			const Point &p, const Normal &n) const;
    SWCSpectrum LiDirectLigthtingMode(const int specularDepth, const Scene *scene,
            const RayDifferential &ray, const Sample *sample,
            float *alpha) const;
    SWCSpectrum LiPathMode(const Scene *scene,
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
	RRStrategy rrStrategy;
	float rrContinueProbability;

	// Dade - debug flags
	bool debugEnableDirect, debugEnableCaustic,
			debugEnableIndirect, debugEnableSpecular;

	// Dade - used to translate Spectrum in SWCSpectrum
	BlackbodySPD blackBodySPD;

	// Declare sample parameters for light source sampling
	int sampleOffset;
	int sampleFinalGather1Offset;
	int sampleFinalGather2Offset;

	int nCausticPaths, nIndirectPaths;
	mutable KdTree<EPhoton, EPhotonProcess> *causticMap;
	mutable KdTree<EPhoton, EPhotonProcess> *indirectMap;
	mutable KdTree<ERadiancePhoton, ERadiancePhotonProcess> *radianceMap;
};

}//namespace lux

#endif
