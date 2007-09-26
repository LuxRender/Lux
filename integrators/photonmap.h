/***************************************************************************
 *   Copyright (C) 1998-2007 by authors (see AUTHORS.txt )                 *
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

#ifndef LUX_PHOTONMAP_H
#define LUX_PHOTONMAP_H

// photonmap.cpp*

#include "lux.h"
#include "kdtree.h"
#include "transport.h"
#include "scene.h"
#include "mc.h"
#include "sampling.h"
// Photonmap Local Declarations
struct Photon;
struct ClosePhoton;
struct PhotonProcess;



class PhotonIntegrator : public SurfaceIntegrator {
public:
	// PhotonIntegrator Public Methods
	PhotonIntegrator(int ncaus, int ndir, int nindir, int nLookup, int mdepth,
		float maxdist, bool finalGather, int gatherSamples,
		bool directWithPhotons);
	~PhotonIntegrator();
	Spectrum Li(const Scene *scene, const RayDifferential &ray,
		const Sample *sample, float *alpha) const;
	void RequestSamples(Sample *sample, const Scene *scene);
	void Preprocess(const Scene *);
	
	static SurfaceIntegrator *CreateSurfaceIntegrator(const ParamSet &params);
private:
	// PhotonIntegrator Private Methods
	static inline bool unsuccessful(int needed, int found, int shot) {
		return (found < needed &&
		        (found == 0 || found < shot / 1024));
	}
	static Spectrum LPhoton(KdTree<Photon, PhotonProcess> *map,
		int nPaths, int nLookup, BSDF *bsdf, const Intersection &isect,
		const Vector &w, float maxDistSquared);
	// PhotonIntegrator Private Data
	u_int nCausticPhotons, nIndirectPhotons, nDirectPhotons;
	u_int nLookup;
	mutable int specularDepth;
	int maxSpecularDepth;
	float maxDistSquared;
	bool directWithPhotons, finalGather;
	int gatherSamples;
	// Declare sample parameters for light source sampling
	int *lightSampleOffset, lightNumOffset;
	int *bsdfSampleOffset, *bsdfComponentOffset;
	int gatherSampleOffset, gatherComponentOffset;
	int nCausticPaths, nDirectPaths, nIndirectPaths;
	mutable KdTree<Photon, PhotonProcess> *causticMap;
	mutable KdTree<Photon, PhotonProcess> *directMap;
	mutable KdTree<Photon, PhotonProcess> *indirectMap;
};
struct Photon {
	// Photon Constructor
	Photon(const Point &pp, const Spectrum &wt, const Vector &w)
		: p(pp), alpha(wt), wi(w) {
	}
	Photon() { }
	Point p;
	Spectrum alpha;
	Vector wi;
};
struct PhotonProcess {
	// PhotonProcess Public Methods
	PhotonProcess(u_int mp, const Point &p);
	void operator()(const Photon &photon, float dist2, float &maxDistSquared) const;
	const Point &p;
	ClosePhoton *photons;
	u_int nLookup;
	mutable u_int foundPhotons;
};
struct ClosePhoton {
	ClosePhoton(const Photon *p = NULL,
	            float md2 = INFINITY) {
		photon = p;
		distanceSquared = md2;
	}
	bool operator<(const ClosePhoton &p2) const {
		return distanceSquared == p2.distanceSquared ? (photon < p2.photon) :
			distanceSquared < p2.distanceSquared;
	}
	const Photon *photon;
	float distanceSquared;
};

#endif
