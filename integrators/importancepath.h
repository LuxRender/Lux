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
#ifndef LUX_IMPORTANCEPATH_H
#define LUX_IMPORTANCEPATH_H


#include "lux.h"
#include "transport.h"
#include "scene.h"
#include "kdtree.h"
#include "mc.h"
#include "sampling.h"

namespace lux
{

struct EClosePhoton;

// Local Declarations

struct IPhoton {
	IPhoton(const Point &pp, const Spectrum &wt, const Vector & w)
			: p(pp), alpha(wt), wi(w) { }
	IPhoton() { }

	Point p;
	Spectrum alpha;
	Vector wi;
};

struct IClosePhoton {
	IClosePhoton(const IPhoton *p = NULL,
			float md2 = INFINITY) {
		photon = p;
		distanceSquared = md2;
	}

	bool operator<(const IClosePhoton & p2) const {
		return distanceSquared == p2.distanceSquared ? (photon < p2.photon) :
				distanceSquared < p2.distanceSquared;
	}

	const IPhoton *photon;
	float distanceSquared;
};

struct IPhotonProcess {
	// PhotonProcess Public Methods
	IPhotonProcess(u_int mp, const Point & p);

	void operator()(const IPhoton &photon, float dist2, float &maxDistSquared) const;

	const Point &p;
	IClosePhoton *photons;
	u_int nLookup;
	mutable u_int foundPhotons;
};

class ImportancePathIntegrator : public SurfaceIntegrator {
public:
	// ImportancePathIntegrator types
	enum LightStrategy {
		SAMPLE_ALL_UNIFORM, SAMPLE_ONE_UNIFORM,
		SAMPLE_AUTOMATIC
	};
	enum RRStrategy { RR_EFFICIENCY, RR_PROBABILITY, RR_NONE };

	// ImportancePathIntegrator Public Methods
	ImportancePathIntegrator(
			LightStrategy st,
			int nphoton,
			float mdist,
			int impdepth,
			int impwidth,
			int mdepth,
			RRStrategy rrStrategy,
			float continueProbability);
	~ImportancePathIntegrator();

	SWCSpectrum Li(const Scene *scene, const RayDifferential &ray,
			const Sample *sample, float *alpha) const;
	void RequestSamples(Sample *sample, const Scene *scene);
	void Preprocess(const Scene *);
	virtual ImportancePathIntegrator* clone() const; // Lux (copy) constructor for multithreading

	IntegrationSampler *HasIntegrationSampler(IntegrationSampler *is) {
		return NULL;
	}; // Not implemented

	static SurfaceIntegrator *CreateSurfaceIntegrator(const ParamSet &params);

private:

	static inline bool unsuccessful(int needed, int found, int shot) {
		return (found < needed &&
				(found == 0 || found < shot / 1024));
	}

	SWCSpectrum LiInternal(const int specularDepth, const Scene *scene,
            const RayDifferential &ray, const Sample *sample,
            float *alpha) const;

	// ImportancePathIntegrator Private Data
	LightStrategy lightStrategy;
	u_int nIndirectPhotons;
	float maxDistSquared;
	int impDepth, impTableWidth, impTableHeight, impTableSize;
	int maxDepth;
	RRStrategy rrStrategy;
	float continueProbability;

	// Declare sample parameters for light source sampling
	int sampleOffset;

	int nImportancePaths;
	mutable KdTree<IPhoton, IPhotonProcess> *importanceMap;
};

}//namespace lux

#endif
