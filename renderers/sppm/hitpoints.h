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

class HitPointLightGroup {
public:
	unsigned long long photonCount;
	XYZColor reflectedFlux;

	u_int accumPhotonCount;
	XYZColor accumReflectedFlux;
	XYZColor accumRadiance;

	u_int constantHitsCount;
	u_int surfaceHitsCount;

	XYZColor radiance;
	// Debug code
	// Radiance Sum Square Error, used to compute Mean Square Error
	//float radianceSSE;
};

class HitPointEyePass {
public:
	HitPointType type;

	// Eye path data
	SWCSpectrum pathThroughput; // Used only for SURFACE type
	float alpha;
	float distance;

	// Used for SURFACE type
	Point position;
	Vector wo;
	Normal bsdfNG;

	vector<XYZColor> emittedRadiance;
};


class HitPoint {
public:
	PermutedHalton *halton;
	float haltonOffset;

	// Used to render eye pass n+1 while doing photon pass n
	HitPointEyePass eyePass[2];

	vector<HitPointLightGroup> lightGroupData;
	
	float accumPhotonRadius2;
};

class SPPMRenderer;

class HitPoints {
public:
	HitPoints(SPPMRenderer *engine, RandomGenerator *rng);
	~HitPoints();

	void Init();

	const double GetPhotonHitEfficency();

	HitPoint *GetHitPoint(const u_int index) {
		return &(*hitPoints)[index];
	}

	const u_int GetSize() const {
		return hitPoints->size();
	}

	const BBox &GetBBox(const u_int passIndex) const {
		return hitPointBBox[passIndex];
	}

	float GetMaxPhotonRaidus2(const u_int passIndex) const { return maxHitPointRadius2[passIndex]; }

	void UpdatePointsInformation();
	const u_int GetEyePassCount() const { return currentEyePass; }
	const u_int GetPhotonPassCount() const { return currentPhotonPass; }
	void IncEyePass() {
		++currentEyePass;
		eyePassWavelengthSample = Halton(currentEyePass, wavelengthSampleScramble);
	}
	void IncPhotonPass() {
		++currentPhotonPass;
		photonPassWavelengthSample = Halton(currentPhotonPass, wavelengthSampleScramble);
	}

	const float GetPassWavelengthSample() { return eyePassWavelengthSample; }
	const float GetPhotonPassWavelengthSample() { return photonPassWavelengthSample; }

	void AddFlux(const Point &hitPoint, const BSDF &bsdf, const Vector &wi,
		const SpectrumWavelengths &sw, const SWCSpectrum &photonFlux, const u_int light_group) {
		const u_int passIndex = currentPhotonPass % 2;
		lookUpAccel[passIndex]->AddFlux(hitPoint, passIndex, bsdf, wi, sw, photonFlux, light_group);
	}

	bool HitSomething(const Point &hitPoint, const BSDF &bsdf, const Vector &wi,
		const SpectrumWavelengths &sw) {
		const u_int passIndex = currentPhotonPass % 2;
		return lookUpAccel[passIndex]->HitSomething(hitPoint, passIndex, bsdf, wi, sw);
	}

	void AccumulateFlux(unsigned long long const total_photons,
		const u_int index, const u_int count);
	void SetHitPoints(RandomGenerator *rng,	const u_int index, const u_int count);

	void RefreshAccelMutex() {
		const u_int passIndex = currentEyePass % 2;
		lookUpAccel[passIndex]->RefreshMutex(passIndex);
	}

	void RefreshAccelParallel(const u_int index, const u_int count) {
		const u_int passIndex = currentEyePass % 2;
		lookUpAccel[passIndex]->RefreshParallel(passIndex, index, count);
	}

	void UpdateFilm();

private:
	bool TraceEyePath(HitPoint *hp, const Sample &sample, const float *u);

	SPPMRenderer *renderer;
	PixelSampler *pixelSampler;

	// Hit points information
	float initialPhotonRaidus;
	// Used to render eye pass n+1 while doing photon pass n
	BBox hitPointBBox[2];
	float maxHitPointRadius2[2];
	std::vector<HitPoint> *hitPoints;
	HitPointsLookUpAccel *lookUpAccel[2];

	u_int currentEyePass;
	u_int currentPhotonPass;

	// Only a single set of wavelengths is sampled for each pass
	float eyePassWavelengthSample, photonPassWavelengthSample;
	u_int wavelengthSampleScramble;
};

}//namespace lux

#endif	/* LUX_HITPOINTS_H */
