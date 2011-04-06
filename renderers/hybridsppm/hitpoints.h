/***************************************************************************
 *   Copyright (C) 1998-2010 by authors (see AUTHORS.txt )                 *
 *                                                                         *
 *   This file is part of LuxRays.                                         *
 *                                                                         *
 *   LuxRays is free software; you can redistribute it and/or modify       *
 *   it under the terms of the GNU General Public License as published by  *
 *   the Free Software Foundation; either version 3 of the License, or     *
 *   (at your option) any later version.                                   *
 *                                                                         *
 *   LuxRays is distributed in the hope that it will be useful,            *
 *   but WITHOUT ANY WARRANTY; without even the implied warranty of        *
 *   MERCHANTABILITY or FITNESS FOR A PARTICULAR PURPOSE.  See the         *
 *   GNU General Public License for more details.                          *
 *                                                                         *
 *   You should have received a copy of the GNU General Public License     *
 *   along with this program.  If not, see <http://www.gnu.org/licenses/>. *
 *                                                                         *
 *   LuxRays website: http://www.luxrender.net                             *
 ***************************************************************************/

#ifndef LUX_HITPOINTS_H
#define	LUX_HITPOINTS_H

#include "lux.h"
#include "scene.h"
#include "sampling.h"

#include "luxrays/luxrays.h"
#include "luxrays/core/device.h"
#include "luxrays/core/intersectiondevice.h"

#include "lookupaccel.h"

namespace lux
{

enum EyePathStateType {
	NEXT_VERTEX, DIRECT_LIGHT_SAMPLING
};

class EyePath {
public:
	EyePath(const Scene &scene, RandomGenerator *rng, const u_int index);
	~EyePath();

	EyePathStateType state;

	Sample sample;

	// Screen information
	u_int pixelIndex;

	// Eye path information
	Ray ray;
	u_int depth;

	SWCSpectrum throughput;
};

class PhotonPath {
public:
	// The ray is stored in the RayBuffer and the index is implicitly stored
	// in the array of PhotonPath
	SWCSpectrum flux;
	u_int depth;
};

//------------------------------------------------------------------------------
// Eye path hit points
//------------------------------------------------------------------------------

enum HitPointType {
	SURFACE, CONSTANT_COLOR
};

class HitPoint {
public:
	HitPointType type;

	// Used for CONSTANT_COLOR and SURFACE type
	SWCSpectrum throughput;

	// Used for SURFACE type
	Point position;
	Vector wo;
	Normal normal;
	BSDF *bsdf;

	unsigned long long photonCount;
	SWCSpectrum reflectedFlux;

	float accumPhotonRadius2;
	u_int accumPhotonCount;
	SWCSpectrum accumReflectedFlux;
	// Used only for direct light sampling
	SWCSpectrum accumDirectLightRadiance;

	SWCSpectrum accumRadiance;

	u_int constantHitsCount;
	u_int surfaceHitsCount;

	SWCSpectrum radiance;
};

/*extern bool GetHitPointInformation(const Scene *scene, RandomGenerator *rndGen,
		Ray *ray, const RayHit *rayHit, Point &hitPoint,
		SWCSpectrum &surfaceColor, Normal &N, Normal &shadeN);*/

class HybridSPPMRenderer;

class HitPoints {
public:
	HitPoints(HybridSPPMRenderer *engine);
	~HitPoints();

	void Init();

	HitPoint *GetHitPoint(const u_int index) {
		return &(*hitPoints)[index];
	}

	const u_int GetSize() const {
		return hitPoints->size();
	}

	const BBox GetBBox() const {
		return bbox;
	}

	float GetMaxPhotonRaidus2() const { return maxPhotonRaidus2; }

	void UpdatePointsInformation();
	u_int GetPassCount() const { return pass; }
	void IncPass() { ++pass; }

	void AddFlux(const Point &hitPoint, const Vector &wi,
		const SpectrumWavelengths &sw, const SWCSpectrum &photonFlux) {
		lookUpAccel->AddFlux(hitPoint, wi, sw, photonFlux);
	}

	void AccumulateFlux(const unsigned long long photonTraced,
			const u_int index, const u_int count);
	void SetHitPoints(RandomGenerator *rndGen,
			luxrays::IntersectionDevice *device, luxrays::RayBuffer *rayBuffer,
			const u_int index, const u_int count);

	void RefreshAccelMutex() {
		lookUpAccel->RefreshMutex();
	}

	void RefreshAccelParallel(const u_int index, const u_int count) {
		lookUpAccel->RefreshParallel(index, count);
	}

private:
	HybridSPPMRenderer *renderer;

	// Hit points information
	BBox bbox;
	float maxPhotonRaidus2;
	std::vector<HitPoint> *hitPoints;
	HitPointsLookUpAccel *lookUpAccel;

	u_int pass;
};

}//namespace lux

#endif	/* LUX_HITPOINTS_H */
