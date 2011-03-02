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
#include "pixelsamplers/linear.h"

#include "lookupaccel.h"

namespace lux
{

//------------------------------------------------------------------------------
// Eye path hit points
//------------------------------------------------------------------------------

enum HitPointType {
	SURFACE, CONSTANT_COLOR
};

struct HitPointLightGroup {
	unsigned long long photonCount;
	XYZColor reflectedFlux;

	u_int accumPhotonCount;
	XYZColor accumReflectedFlux;
	XYZColor accumRadiance;

	u_int constantHitsCount;
	u_int surfaceHitsCount;

	XYZColor radiance;

	XYZColor eyeRadiance;
};


class HitPoint {
public:
	HitPointType type;
	Sample *sample;

	// Eye path data
	SWCSpectrum eyeThroughput; // Used only for SURFACE type
	float eyeAlpha;
	float eyeDistance;

	// Used for SURFACE type
	Point position;
	Vector wo;
	BSDF *bsdf;

	vector<HitPointLightGroup> lightGroupData;
	
	float accumPhotonRadius2;
};

class SPPMRenderer;

class HitPoints {
public:
	HitPoints(SPPMRenderer *engine);
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
		const SpectrumWavelengths &sw, const SWCSpectrum &photonFlux, const u_int light_group) {
		lookUpAccel->AddFlux(hitPoint, wi, sw, photonFlux, light_group);
	}

	void AccumulateFlux(const vector<unsigned long long> &photonTracedByLightGroup,
		const u_int index, const u_int count);
	void SetHitPoints(RandomGenerator *rng,
		const u_int index, const u_int count);

	void RefreshAccelMutex() {
		lookUpAccel->RefreshMutex();
	}

	void RefreshAccelParallel(const u_int index, const u_int count) {
		lookUpAccel->RefreshParallel(index, count);
	}

	void UpdateFilm();

private:
	void TraceEyePath(HitPoint *hp, const Sample &sample);

	SPPMRenderer *renderer;
	PixelSampler *pixelSampler;

	// Hit points information
	BBox bbox;
	float maxPhotonRaidus2;
	std::vector<HitPoint> *hitPoints;
	HitPointsLookUpAccel *lookUpAccel;

	u_int pass;
};

}//namespace lux

#endif	/* LUX_HITPOINTS_H */
