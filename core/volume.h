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

#ifndef LUX_VOLUME_H
#define LUX_VOLUME_H
// volume.h*
#include "lux.h"
#include "color.h"
#include "geometry.h"

namespace lux
{

// Volume Scattering Declarations
float PhaseIsotropic(const Vector &w, const Vector &wp);

float PhaseRayleigh(const Vector &w, const Vector &wp);

float PhaseMieHazy(const Vector &w, const Vector &wp);

float PhaseMieMurky(const Vector &w, const Vector &wp);
float PhaseHG(const Vector &w, const Vector &wp, float g);
float PhaseSchlick(const Vector &w, const Vector &wp, float g);
class  VolumeRegion {
public:
	// VolumeRegion Interface
	virtual ~VolumeRegion() { }
	virtual BBox WorldBound() const = 0;
	virtual bool IntersectP(const Ray &ray, float *t0,
		float *t1) const = 0;
	virtual RGBColor sigma_a(const Point &,
		const Vector &) const = 0;
	virtual RGBColor sigma_s(const Point &,
		const Vector &) const = 0;
	virtual
		RGBColor Lve(const Point &, const Vector &) const = 0;
	virtual float p(const Point &, const Vector &,
		const Vector &) const = 0;
	virtual RGBColor sigma_t(const Point &, const Vector &) const;
	virtual RGBColor Tau(const Ray &ray,
		float step = 1.f, float offset = 0.5) const = 0;
};

class  DensityRegion : public VolumeRegion {
public:
	// DensityRegion Public Methods
	DensityRegion(const RGBColor &sig_a, const RGBColor &sig_s,
		float g, const RGBColor &Le, const Transform &VolumeToWorld);
	virtual float Density(const Point &Pobj) const = 0;
	RGBColor sigma_a(const Point &p, const Vector &) const {
		return Density(WorldToVolume(p)) * sig_a;
	}
	RGBColor sigma_s(const Point &p, const Vector &) const {
		return Density(WorldToVolume(p)) * sig_s;
	}
	RGBColor sigma_t(const Point &p, const Vector &) const {
		return Density(WorldToVolume(p)) * (sig_a + sig_s);
	}
	RGBColor Lve(const Point &p, const Vector &) const {
		return Density(WorldToVolume(p)) * le;
	}
	float p(const Point &p, const Vector &w,
			const Vector &wp) const {
		return PhaseHG(w, wp, g);
	}
	RGBColor Tau(const Ray &r, float stepSize, float offset) const;
protected:
	// DensityRegion Protected Data
	Transform WorldToVolume;
	RGBColor sig_a, sig_s, le;
	float g;
};

class  AggregateVolume : public VolumeRegion {
public:
	// AggregateVolume Public Methods
	AggregateVolume(const vector<VolumeRegion *> &r);
	~AggregateVolume();
	BBox WorldBound() const;
	bool IntersectP(const Ray &ray, float *t0, float *t1) const;
	RGBColor sigma_a(const Point &, const Vector &) const;
	RGBColor sigma_s(const Point &, const Vector &) const;
	RGBColor Lve(const Point &, const Vector &) const;
	float p(const Point &, const Vector &, const Vector &) const;
	RGBColor sigma_t(const Point &, const Vector &) const;
	RGBColor Tau(const Ray &ray, float, float) const;
private:
	// AggregateVolume Private Data
	vector<VolumeRegion *> regions;
	BBox bound;
};

}//namespace lux

#endif // LUX_VOLUME_H
