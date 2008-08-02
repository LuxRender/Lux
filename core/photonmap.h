/***************************************************************************
 *   Copyright (C) 1998-2008 by authors (see AUTHORS.txt )                 *
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

#ifndef LUX_PHOTONMAP_H
#define LUX_PHOTONMAP_H

#include "lux.h"
#include "scene.h"
#include "geometry.h"
#include "spectrum.h"
#include "kdtree.h"
#include "bxdf.h"
#include "primitive.h"

namespace lux
{

//------------------------------------------------------------------------------
// Dade - different kind of photon types. All of them must extend the base
// class BasicPhoton.
//------------------------------------------------------------------------------

class BasicPhoton {
public:
	BasicPhoton(const Point &pp, const SWCSpectrum &wt)
			: p(pp), alpha(wt) {
	}

	BasicPhoton() {
	}

	virtual void save(bool isLittleEndian, std::basic_ostream<char> &stream) = 0;

	Point p;
	SWCSpectrum alpha;
};

class LightPhoton : public BasicPhoton {
public:
	LightPhoton(const Point &pp, const SWCSpectrum &wt, const Vector & w)
			: BasicPhoton(pp, wt), wi(w) { }

	LightPhoton() : BasicPhoton() { }

	void save(bool isLittleEndian, std::basic_ostream<char> &stream);

	Vector wi;
};

class RadiancePhoton : public BasicPhoton {
public:
	RadiancePhoton(const Point &pp, const SWCSpectrum &wt, const Normal & nn)
			: BasicPhoton(pp, wt), n(nn) { }
	RadiancePhoton(const Point &pp, const Normal & nn)
			: BasicPhoton(pp, 0.0f), n(nn) { }

	RadiancePhoton() : BasicPhoton() { }

	void save(bool isLittleEndian, std::basic_ostream<char> &stream);

	Normal n;
};

//------------------------------------------------------------------------------
// Dade - different kind of photon process types. All of them must extend
// the base class BasicPhotonProcess. This class is used by the KDTree
// code to process photons.
//------------------------------------------------------------------------------

template <class PhotonType> class BasicPhotonProcess {
public:
	BasicPhotonProcess() {
	}
};

//------------------------------------------------------------------------------
// Dade - construct a set of the nearer photons
//------------------------------------------------------------------------------

template <class PhotonType> class ClosePhoton {
public:
	ClosePhoton(const PhotonType *p = NULL,
			float md2 = INFINITY) {
		photon = p;
		distanceSquared = md2;
	}

	bool operator<(const ClosePhoton & p2) const {
		return distanceSquared == p2.distanceSquared ? (photon < p2.photon) :
				distanceSquared < p2.distanceSquared;
	}

	const PhotonType *photon;
	float distanceSquared;
};

template <class PhotonType> class NearSetPhotonProcess : BasicPhotonProcess<PhotonType> {
public:
	NearSetPhotonProcess(u_int mp, const Point &P): p(P) {
	    photons = NULL;
		nLookup = mp;
		foundPhotons = 0;
	}

	void operator()(const PhotonType &photon, float distSquared, float &maxDistSquared) const {
		// Do usual photon heap management
		if (foundPhotons < nLookup) {
			// Add photon to unordered array of photons
			photons[foundPhotons++] = ClosePhoton<PhotonType>(&photon, distSquared);
			if (foundPhotons == nLookup) {
				std::make_heap(&photons[0], &photons[nLookup]);
				maxDistSquared = photons[0].distanceSquared;
			}
		} else {
			// Remove most distant photon from heap and add new photon
			std::pop_heap(&photons[0], &photons[nLookup]);
			photons[nLookup - 1] = ClosePhoton<PhotonType>(&photon, distSquared);
			std::push_heap(&photons[0], &photons[nLookup]);
			maxDistSquared = photons[0].distanceSquared;
		}
	}

	const Point &p;
	ClosePhoton<PhotonType> *photons;
	u_int nLookup;
	mutable u_int foundPhotons;
};

//------------------------------------------------------------------------------
// Dade - find the nearest photon
//------------------------------------------------------------------------------

template <class PhotonType> class NearPhotonProcess : public BasicPhotonProcess<PhotonType> {
public:
	NearPhotonProcess(const Point &pp, const Normal & nn)
			: p(pp), n(nn) {
		photon = NULL;
	}

	void operator()(const PhotonType &rp,
			float distSquared, float &maxDistSquared) const {
		if (Dot(rp.n, n) > 0) {
			photon = &rp;
			maxDistSquared = distSquared;
		}
	}

	const Point &p;
	const Normal &n;
	mutable const PhotonType *photon;
};

//------------------------------------------------------------------------------
// Dade - photonmap type
//------------------------------------------------------------------------------

template <class PhotonType, class PhotonProcess> class PhotonMap {
public:
	PhotonMap() : photonCount(0), photonmap(NULL) { }

	~PhotonMap() {
		if (photonmap)
			delete photonmap;
	}

	void lookup(const Point &p, const PhotonProcess &proc,
			float &maxDistSquared) const {
		if (photonmap)
			photonmap->Lookup(p, proc, maxDistSquared);
	}

	int getPhotonCount() { return photonCount; }

protected:
	int photonCount;
	KdTree<PhotonType, PhotonProcess> *photonmap;
};

class RadiancePhotonMap : public PhotonMap<RadiancePhoton, NearPhotonProcess<RadiancePhoton> > {
public:
	RadiancePhotonMap(u_int nl, float md) :
		PhotonMap<RadiancePhoton, NearPhotonProcess<RadiancePhoton> >(),
		nLookup(nl), maxDistSquared(md) { }

	void init(const vector<RadiancePhoton> photons) {
		photonCount = photons.size();
		photonmap = new KdTree<RadiancePhoton, NearPhotonProcess<RadiancePhoton> >(photons);
	}

	void save(std::basic_ostream<char> &stream) const;

	static void load(std::basic_istream<char> &stream, RadiancePhotonMap *map);

	// Dade - used only to build the map (lookup in the direct map) but not for lookup
	u_int nLookup;
	float maxDistSquared;
};

class LightPhotonMap : public PhotonMap<LightPhoton, NearSetPhotonProcess<LightPhoton> > {
public:
	LightPhotonMap(u_int nl, float md) :
		PhotonMap<LightPhoton, NearSetPhotonProcess<LightPhoton> >(),
		nLookup(nl), maxDistSquared(md), nPaths(0) { }

	void init(int npaths, const vector<LightPhoton> photons) {
		photonCount = photons.size();
		nPaths = npaths;
		photonmap = new KdTree<LightPhoton, NearSetPhotonProcess<LightPhoton> >(photons);
	}

	bool isEmpty() {
		return (nPaths == 0);
	}

	SWCSpectrum estimateE(const Point &p, const Normal &n) const;

	SWCSpectrum LPhoton(
			BSDF *bsdf,
			const Intersection &isect,
			const Vector &wo) const;

	void save(std::basic_ostream<char> &stream) const;

	static void load(std::basic_istream<char> &stream, LightPhotonMap *map);

	u_int nLookup;
	float maxDistSquared;

private:
	u_int nPaths;
};

inline float Ekernel(const BasicPhoton *photon, const Point &p, float md2) {
	float s = (1.f - DistanceSquared(photon->p, p) / md2);

	return 3.f / (md2 * M_PI) * s * s;
}

enum PhotonMapRRStrategy { RR_EFFICIENCY, RR_PROBABILITY, RR_NONE };

extern void PhotonMapPreprocess(
		const Scene *scene, string *mapFileName,
		u_int nDirectPhotons,
		u_int nRadiancePhotons, RadiancePhotonMap *radianceMap,
		u_int nIndirectPhotons, LightPhotonMap *indirectMap,
		u_int nCausticPhotons, LightPhotonMap *causticMap);

extern SWCSpectrum PhotonMapFinalGatherWithImportaceSampling(
		const Scene *scene,
		const Sample *sample,
		int sampleFinalGather1Offset,
		int sampleFinalGather2Offset,
		int gatherSamples,
		float cosGatherAngle,
		PhotonMapRRStrategy rrStrategy,
		float rrContinueProbability,
		const LightPhotonMap *indirectMap,
		const RadiancePhotonMap *radianceMap,
		const Vector &wo,
		const BSDF *bsdf);

extern SWCSpectrum PhotonMapFinalGather(
		const Scene *scene,
		const Sample *sample,
		int sampleFinalGatherOffset,
		int gatherSamples,
		PhotonMapRRStrategy rrStrategy,
		float rrContinueProbability,
		const LightPhotonMap *indirectMap,
		const RadiancePhotonMap *radianceMap,
		const Vector &wo,
		const BSDF *bsdf);

}//namespace lux

#endif // LUX_KDTREE_H

