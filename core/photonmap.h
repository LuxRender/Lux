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

#ifndef LUX_PHOTONMAP_H
#define LUX_PHOTONMAP_H

#include "lux.h"
#include "spectrum.h"
#include "spectrumwavelengths.h"
#include "kdtree.h"
#include "bxdf.h"
#include "mc.h"

namespace lux
{

//------------------------------------------------------------------------------
// Dade - different kind of photon types. All of them must extend the base
// class BasicPhoton.
//------------------------------------------------------------------------------

class BasicPhoton {
public:
	BasicPhoton(const Point &pp)
			: p(pp) {
	}

	BasicPhoton() {
	}

	virtual ~BasicPhoton() {}

	virtual void save(bool isLittleEndian, std::basic_ostream<char> &stream) const = 0;
	virtual void load(bool isLittleEndian, std::basic_istream<char> &stream) = 0;

	Point p;
};

class BasicColorPhoton : public BasicPhoton {
public:
	BasicColorPhoton(const SpectrumWavelengths &sw, const Point &pp,
		const SWCSpectrum &wt)
		: BasicPhoton(pp), alpha(wt) {
		for (u_int i = 0; i < WAVELENGTH_SAMPLES; ++i)
			w[i] = sw.w[i];
		if (sw.single) {
			const float a = alpha.c[sw.single_w] * WAVELENGTH_SAMPLES;
			alpha = SWCSpectrum(0.f);
			alpha.c[sw.single_w] = a;
		}
	}

	BasicColorPhoton() : BasicPhoton() { }
	virtual ~BasicColorPhoton() { }

	SWCSpectrum GetSWCSpectrum(const SpectrumWavelengths &sw) const;

	virtual void save(bool isLittleEndian, std::basic_ostream<char> &stream) const;
	virtual void load(bool isLittleEndian, std::basic_istream<char> &stream);

	SWCSpectrum alpha;
	float w[WAVELENGTH_SAMPLES];
};

class LightPhoton : public BasicColorPhoton {
public:
	LightPhoton(const SpectrumWavelengths &sw, const Point &pp,
		const SWCSpectrum &wt, const Vector &wi_)
		: BasicColorPhoton(sw, pp, wt), wi(wi_) { }

	LightPhoton() : BasicColorPhoton() { }
	virtual ~LightPhoton() { }

	virtual void save(bool isLittleEndian, std::basic_ostream<char> &stream) const;
	virtual void load(bool isLittleEndian, std::basic_istream<char> &stream);

	Vector wi;
};

class RadiancePhoton : public BasicColorPhoton {
public:
	RadiancePhoton(const SpectrumWavelengths &sw, const Point &pp,
		const SWCSpectrum &wt, const Normal &nn)
		: BasicColorPhoton(sw, pp, wt), n(nn) { }
	RadiancePhoton(const SpectrumWavelengths &sw, const Point &pp, const Normal & nn)
		: BasicColorPhoton(sw, pp, SWCSpectrum(0.0f)), n(nn) { }

	RadiancePhoton() : BasicColorPhoton() { }
	virtual ~RadiancePhoton() { }

	virtual void save(bool isLittleEndian, std::basic_ostream<char> &stream) const;
	virtual void load(bool isLittleEndian, std::basic_istream<char> &stream);

	Normal n;
};

class PdfPhoton : public BasicPhoton {
public:
	// Declare direction class
	class Direction {
	public:
		Direction() { }
		Direction(float aWeight) : weight(aWeight) { }
		Direction(const Vector& aDir, float aRadius, float aWeight)
			: dir(aDir), cosRadius(cosf(aRadius)), weight(aWeight) { }

		bool operator < (const Direction& other) const {
			return weight < other.weight;
		}

		Vector dir;
		float cosRadius;
		float weight;
	};

	PdfPhoton(const Point &pp, const vector<Direction>& aDirs)
		: BasicPhoton(pp), dirs(aDirs)
	{
		float totWeight = 0.f;
		for(u_int i = 0; i<dirs.size(); i++)
			totWeight += dirs[i].weight;
		for(u_int i = 0; i<dirs.size(); i++)
			dirs[i].weight /= totWeight;
	}

	PdfPhoton() : BasicPhoton(), dirs(0) { }
	virtual ~PdfPhoton() { }

	virtual void save(bool isLittleEndian, std::basic_ostream<char> &stream) const {
	}

	float Sample(Vector *wi, float u1, float u2, float u3) const {
		size_t dn = Clamp<size_t>(static_cast<size_t>(
			std::upper_bound(dirs.begin(), dirs.end(), Direction(u3)) - dirs.begin()),
			0U, dirs.size() - 1);

		Vector vx, vy;
		CoordinateSystem(dirs[dn].dir, &vx, &vy);
		*wi = UniformSampleCone(u1, u2, dirs[dn].cosRadius, vx, vy, dirs[dn].dir);

		return Pdf(*wi);
	}
	float Pdf(const Vector &wi) const {
		float pdf = 0.f;
		for (u_int i = 0; i < dirs.size(); ++i) {
			if (Dot(dirs[i].dir, wi) > dirs[i].cosRadius)
				pdf += UniformConePdf(dirs[i].cosRadius);
		}
		return pdf;
	}

private:
	vector<Direction> dirs;
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

	virtual ~PhotonMap() {
		if (photonmap)
			delete photonmap;
	}

	/**
	 * Performs a lookup in this photonmap.
	 *
	 * @param p              The lookup point.
	 * @param proc           The process that all photons near 
	 *                       the lookup point will be passed to.
	 * @param maxDistSquared The maximum squared between the lookup 
	 *                       point and the photons. This value can be
	 *                       update by the process during the photon
	 *                       lookup.
	 */
	void lookup(const Point &p, const PhotonProcess &proc,
			float &maxDistSquared) const {
		if (photonmap)
			photonmap->Lookup(p, proc, maxDistSquared);
	}

	u_int getPhotonCount() { return photonCount; }

protected:
	u_int photonCount;
	KdTree<PhotonType, PhotonProcess> *photonmap;
};

class RadiancePhotonMap : public PhotonMap<RadiancePhoton, NearPhotonProcess<RadiancePhoton> > {
public:
	RadiancePhotonMap(u_int nl, float md) :
		PhotonMap<RadiancePhoton, NearPhotonProcess<RadiancePhoton> >(),
		nLookup(nl), maxDistSquared(md), empty(true) { }
	virtual ~RadiancePhotonMap() { }

	void init(const vector<RadiancePhoton> &photons) {
		photonCount = photons.size();
		photonmap = new KdTree<RadiancePhoton, NearPhotonProcess<RadiancePhoton> >(photons);
		empty = false;
	}

	bool IsEmpty() const {
		return empty;
	}

	/**
	 * Estimates the outgoing radiance at a surface point in a single direction.
	 *
	 * @param sw       The current set of sampled wavelengths.
	 * @param isect    The surface point intersection.
	 * @param wo       The outgoing direction.
	 * @param bxdfType The bxdf types at the surface point to to take into account.
	 *
	 * @return A radiance estimate.
	 */
	SWCSpectrum LPhoton(const SpectrumWavelengths &sw, 
		const Intersection& isect, 
		const Vector& wo, 
		const BxDFType bxdfType) const;

	void save(std::basic_ostream<char> &stream) const;

	static void load(std::basic_istream<char> &stream, RadiancePhotonMap *map);

	// Dade - used only to build the map (lookup in the direct map) but not for lookup
	const u_int nLookup;
	const float maxDistSquared;
	bool empty;
};

class LightPhotonMap : public PhotonMap<LightPhoton, NearSetPhotonProcess<LightPhoton> > {
public:
	LightPhotonMap(u_int nl, float md) :
		PhotonMap<LightPhoton, NearSetPhotonProcess<LightPhoton> >(),
		nLookup(nl), maxDistSquared(md), nPaths(0) { }
	virtual ~LightPhotonMap() { }

	void init(u_int npaths, const vector<LightPhoton> &photons) {
		photonCount = photons.size();
		nPaths = npaths;
		photonmap = new KdTree<LightPhoton, NearSetPhotonProcess<LightPhoton> >(photons);
	}

	bool IsEmpty() const {
		return (nPaths == 0);
	}

	/**
	 * Estimates the incoming irradiance at a surface point.
	 *
	 * @param sw     The current set of sampled wavelengths.
	 * @param p      The position of the surface point.
	 * @param n      The orientation of the surface.
	 *
	 * @return An irradiance estimate.
	 */
	SWCSpectrum EPhoton(
		const SpectrumWavelengths &sw,
		const Point &p, 
		const Normal &n) const;

	/**
	 * Estimates the outgoing radiance at a surface point in a single direction.
	 *
	 * @param sw       The current set of sampled wavelengths.
	 * @param bsdf     The bsdf of the surface point.
	 * @param isect    The surface point intersection.
	 * @param wo       The outgoing direction.
	 * @param bxdfType The bxdf types at the surface point to to take into account.
	 *
	 * @return A radiance estimate.
	 */
	SWCSpectrum LPhoton(
		const SpectrumWavelengths &sw,
		const BSDF *bsdf,
		const Intersection &isect,
		const Vector &wo,
		const BxDFType bxdfType) const;

	/**
	 * Estimates the outgoing radiance at a surface point in a single direction
	 * using a diffuse surface approximation.
	 *
	 * @param sw       The current set of sampled wavelengths.
	 * @param bsdf     The bsdf of the surface point.
	 * @param isect    The surface point intersection.
	 * @param wo       The outgoing direction.
	 * @param bxdfType The bxdf types at the surface point to to take into account.
	 *
	 * @return A radiance estimate.
	 */
	SWCSpectrum LPhotonDiffuseApprox(
		const SpectrumWavelengths &sw,
		const BSDF *bsdf,
		const Intersection &isect,
		const Vector &wo,
		const BxDFType bxdfType) const;

	/**
	 * Estimates the outgoing radiance by diffuse reflection at a surface point 
	 * in a single direction.
	 *
	 * @param sw       The current set of sampled wavelengths.
	 * @param bsdf     The bsdf of the surface point.
	 * @param isect    The surface point intersection.
	 * @param wo       The outgoing direction.
	 *
	 * @return A radiance estimate.
	 */
	SWCSpectrum LDiffusePhoton(
		const SpectrumWavelengths &sw,
		const BSDF *bsdf,
		const Intersection &isect,
		const Vector &wo) const;

	void save(std::basic_ostream<char> &stream) const;

	static void load(std::basic_istream<char> &stream, LightPhotonMap *map);

	const u_int nLookup;
	const float maxDistSquared;
private:
	u_int nPaths;
};

inline float Ekernel(const BasicPhoton *photon, const Point &p, float md2) {
	float s = (1.f - DistanceSquared(photon->p, p) / md2);

	return 3.f / (md2 * M_PI) * s * s;
}

enum PhotonMapRRStrategy { RR_EFFICIENCY, RR_PROBABILITY, RR_NONE };

/**
 * Creates a number of photonmaps. This function should be called during 
 * the preprocess step of an integrator.
 *
 * @param rng              The random generator to use
 * @param scene            The scene to build the photon maps for.
 * @param mapFileName      The file to load photonmaps from and store them to.
 * @param photonBxdfType   The bxdf types where photons should be stored.
 * @param radianceBxdfType The bxdf types that the radiance photons should take
 *                         into account.
 * @param nDirectPhotons   The number of direct lighting photons to use. These
 *                         are only used to create the radiance map.
 * @param nRadiancePhotons The number of radiance photons to create.
 * @param radianceMap      The target map for radiance photons.
 * @param nIndirectPhotons The number of indrect photons to create.
 * @param indirectMap      The target map for the indirect photons.
 * @param nCausticPhotons  The number of caustic photons to create.
 * @param causticMap       The target map for the caustic photons.
 */
extern void PhotonMapPreprocess(
	const RandomGenerator &rng,
	const Scene &scene, 
	const string *mapFileName,
	const BxDFType photonBxdfType,
	const BxDFType radianceBxdfType,
	u_int nDirectPhotons,
	u_int nRadiancePhotons, RadiancePhotonMap *radianceMap,
	u_int nIndirectPhotons, LightPhotonMap *indirectMap,
	u_int nCausticPhotons, LightPhotonMap *causticMap,
	u_int maxDepth);

/**
 * Estimates the outgoing radiance from a surface point in a single direction 
 * by performing a final gather using sampling with nearby photons and 
 * sampling of the bsdf.
 *
 * @param scene                    The scene.
 * @param sample                   The sample containing all necessary samples.
 * @param sampleFinalGather1Offset The offset for sampling the bsdf. For each 
 *                                 gather sample a 2D and a 1D sample are required.
 * @param sampleFinalGather2Offset The offset for sampling with photons. For each 
 *                                 gather sample a 2D and a 1D sample are required.
 * @param gatherSamples            The number of gather samples to use.
 * @param cosGatherAngle           The cosine of the angle around a photon to sample.
 * @param rrStrategy               The russian roulette strategy to use.
 * @param rrContinueProbability    The russian roulette continue probability.
 * @param indirectMap              The indirect photon map.
 * @param radianceMap              The radiance photon map.
 * @param wo                       The outgoing direction to estimate the radiance in.
 * @param bsdf                     The bsdf at the surface point.
 * @param bxdfType                 The bxdf types at the surface point to to take 
 *                                 into account.
 *
 * @return A radiance estimate.
 */
extern SWCSpectrum PhotonMapFinalGatherWithImportaceSampling(
	const Scene &scene,
	const Sample &sample,
	u_int sampleFinalGather1Offset,
	u_int sampleFinalGather2Offset,
	u_int gatherSamples,
	float cosGatherAngle,
	PhotonMapRRStrategy rrStrategy,
	float rrContinueProbability,
	const LightPhotonMap *indirectMap,
	const RadiancePhotonMap *radianceMap,
	const Vector &wo,
	const BSDF *bsdf,
	const BxDFType bxdfType);

/**
 * Estimates the outgoing radiance from a surface point in a single direction 
 * by performing a final gather using only sampling of the bsdf.
 *
 * @param scene                   The scene.
 * @param sample                  The sample containing all necessary samples.
 * @param sampleFinalGatherOffset The offset for sampling the bsdf. For each 
 *                                gather sample a 2D and a 1D sample are required.
 * @param gatherSamples           The number of gather samples to use.
 * @param rrStrategy              The russian roulette strategy to use.
 * @param rrContinueProbability   The russian roulette continue probability.
 * @param indirectMap             The indirect photon map.
 * @param radianceMap             The radiance photon map.
 * @param wo                      The outgoing direction to estimate the radiance in.
 * @param bxdfType                The bxdf types at the surface point to to take 
 *                                into account.
 *
 * @return A radiance estimate.
 */
extern SWCSpectrum PhotonMapFinalGather(
	const Scene &scene,
	const Sample &sample,
	u_int sampleFinalGatherOffset,
	u_int gatherSamples,
	PhotonMapRRStrategy rrStrategy,
	float rrContinueProbability,
	const LightPhotonMap *indirectMap,
	const RadiancePhotonMap *radianceMap,
	const Vector &wo,
	const BSDF *bsdf,
	const BxDFType bxdfType);

}//namespace lux

#endif // LUX_KDTREE_H

