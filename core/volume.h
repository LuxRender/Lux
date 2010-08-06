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

#ifndef LUX_VOLUME_H
#define LUX_VOLUME_H
// volume.h*
#include "lux.h"
#include "geometry/transform.h"
#include "geometry/bbox.h"
#include "spectrum.h"
#include "fresnelgeneral.h"
#include "color.h"

namespace lux
{

// Volume Scattering Declarations
float PhaseIsotropic(const Vector &w, const Vector &wp);
float PhaseRayleigh(const Vector &w, const Vector &wp);
float PhaseMieHazy(const Vector &w, const Vector &wp);
float PhaseMieMurky(const Vector &w, const Vector &wp);
float PhaseHG(const Vector &w, const Vector &wp, float g);
float PhaseSchlick(const Vector &w, const Vector &wp, float g);

class Volume {
public:
	// Volume Interface
	virtual ~Volume() { }
	virtual SWCSpectrum SigmaA(const SpectrumWavelengths &sw, const Point &,
		const Vector &) const = 0;
	virtual SWCSpectrum SigmaS(const SpectrumWavelengths &sw, const Point &,
		const Vector &) const = 0;
	virtual SWCSpectrum Lve(const SpectrumWavelengths &sw, const Point &,
		const Vector &) const = 0;
	virtual float P(const SpectrumWavelengths &sw, const Point &,
		const Vector &, const Vector &) const = 0;
	virtual SWCSpectrum SigmaT(const SpectrumWavelengths &sw,
		const Point &p, const Vector &w) const {
		return SigmaA(sw, p, w) + SigmaS(sw, p, w);
	}
	virtual SWCSpectrum Tau(const SpectrumWavelengths &sw, const Ray &ray,
		float step = 1.f, float offset = 0.5f) const = 0;
	virtual FresnelGeneral Fresnel(const SpectrumWavelengths &sw,
		const Point &, const Vector &) const = 0;
};

class RGBVolume : public Volume {
public:
	RGBVolume(const RGBColor &sA, const RGBColor &sS, const RGBColor &l,
		float gg) : sigA(sA), sigS(sS), le(l), g(gg) { }
	virtual ~RGBVolume() { }
	virtual SWCSpectrum SigmaA(const SpectrumWavelengths &sw,
		const Point &p, const Vector &) const {
		return SWCSpectrum(sw, sigA);
	}
	virtual SWCSpectrum SigmaS(const SpectrumWavelengths &sw,
		const Point &p, const Vector &) const {
		return SWCSpectrum(sw, sigS);
	}
	virtual SWCSpectrum Lve(const SpectrumWavelengths &sw, const Point &p,
		const Vector &) const {
		//FIXME - use a D65 white point instead of the E white point
		return SWCSpectrum(sw, le);
	}
	virtual float P(const SpectrumWavelengths &sw, const Point &p,
		const Vector &w, const Vector &wp) const {
		return PhaseHG(w, wp, g);
	}
	virtual SWCSpectrum Tau(const SpectrumWavelengths &sw, const Ray &ray,
		float step = 1.f, float offset = 0.5f) const {
		return SigmaT(sw, ray.o, -ray.d) *
			(ray.d.Length() * (ray.maxt - ray.mint));
	}
	virtual FresnelGeneral Fresnel(const SpectrumWavelengths &sw,
		const Point &, const Vector &) const {
		return FresnelGeneral(DIELECTRIC_FRESNEL,
			SWCSpectrum(1.f), SWCSpectrum(0.f));
	}
private:
	RGBColor sigA, sigS, le;
	float g;
};

class  Region : public Volume {
public:
	// VolumeRegion Interface
	Region() { }
	Region(const BBox &b) : bound(b) { }
	virtual ~Region() { }
	virtual BBox WorldBound() const { return bound; };
	virtual bool IntersectP(const Ray &ray, float *t0, float *t1) const {
		return bound.IntersectP(ray, t0, t1);
	}
protected:
	BBox bound;
};

template<class T> class VolumeRegion : public Region {
public:
	VolumeRegion(const Transform &v2w, const BBox &b, const T &v) :
		Region(v2w(b)), WorldToVolume(v2w.GetInverse()), region(b),
		volume(v) { }
	virtual ~VolumeRegion() { }
	virtual bool IntersectP(const Ray &ray, float *t0, float *t1) const {
		return region.IntersectP(WorldToVolume(ray), t0, t1);
	}
	virtual SWCSpectrum SigmaA(const SpectrumWavelengths &sw,
		const Point &p, const Vector &w) const {
		return region.Inside(WorldToVolume(p)) ?
			volume.SigmaA(sw, p, w) : SWCSpectrum(0.f);
	}
	virtual SWCSpectrum SigmaS(const SpectrumWavelengths &sw,
		const Point &p, const Vector &w) const {
		return region.Inside(WorldToVolume(p)) ?
			volume.SigmaA(sw, p, w) : SWCSpectrum(0.f);
	}
	virtual SWCSpectrum SigmaT(const SpectrumWavelengths &sw,
		const Point &p, const Vector &w) const {
		return region.Inside(WorldToVolume(p)) ?
			volume.SigmaT(sw, p, w) : SWCSpectrum(0.f);
	}
	virtual SWCSpectrum Lve(const SpectrumWavelengths &sw, const Point &p,
		const Vector &w) const {
		return region.Inside(WorldToVolume(p)) ?
			volume.Lve(sw, p, w) : SWCSpectrum(0.f);
	}
	virtual float P(const SpectrumWavelengths &sw, const Point &p,
		const Vector &w, const Vector &wp) const {
		return volume.P(sw, p, w, wp);
	}
	virtual SWCSpectrum Tau(const SpectrumWavelengths &sw, const Ray &r,
		float stepSize, float offset) const {
		float t0, t1;
		Ray rn(r);
		if (!IntersectP(rn, &t0, &t1))
			return SWCSpectrum(0.f);
		rn.mint = t0;
		rn.maxt = t1;
		return volume.Tau(sw, rn, stepSize, offset);
	}
	virtual FresnelGeneral Fresnel(const SpectrumWavelengths &sw,
		const Point &p, const Vector &w) const {
		return volume.Fresnel(sw, p, w);
	}
protected:
	Transform WorldToVolume;
	BBox region;
	T volume;
};

template<class T> class  DensityVolume : public Volume {
public:
	// DensityVolume Public Methods
	DensityVolume(const T &v) : volume(v) { }
	virtual ~DensityVolume() { }
	virtual float Density(const Point &Pobj) const = 0;
	virtual SWCSpectrum SigmaA(const SpectrumWavelengths &sw,
		const Point &p, const Vector &w) const {
		return Density(p) * volume.SigmaA(sw, p, w);
	}
	virtual SWCSpectrum SigmaS(const SpectrumWavelengths &sw,
		const Point &p, const Vector &w) const {
		return Density(p) * volume.SigmaA(sw, p, w);
	}
	virtual SWCSpectrum SigmaT(const SpectrumWavelengths &sw,
		const Point &p, const Vector &w) const {
		return Density(p) * volume.SigmaT(sw, p, w);
	}
	virtual SWCSpectrum Lve(const SpectrumWavelengths &sw, const Point &p,
		const Vector &w) const {
		return Density(p) * volume.Lve(sw, p, w);
	}
	virtual float P(const SpectrumWavelengths &sw, const Point &p,
		const Vector &w, const Vector &wp) const {
		return volume.P(sw, p, w, wp);
	}
	virtual SWCSpectrum Tau(const SpectrumWavelengths &sw, const Ray &r,
		float stepSize, float offset) const {
		if (!(r.d.Length() > 0.f))
			return SWCSpectrum(0.f);
		const float step = stepSize / r.d.Length();
		SWCSpectrum tau(0.f);
		for (float t0 = r.mint + offset * step; t0 < r.maxt; t0 += step)
			tau += SigmaT(sw, r(t0), -r.d);
		return tau * stepSize;
	}
	virtual FresnelGeneral Fresnel(const SpectrumWavelengths &sw,
		const Point &p, const Vector &w) const {
		return volume.Fresnel(sw, p, w);
	}
protected:
	// DensityVolume Protected Data
	T volume;
};

class  AggregateRegion : public Region {
public:
	// AggregateRegion Public Methods
	AggregateRegion(const vector<Region *> &r);
	virtual ~AggregateRegion();
	virtual bool IntersectP(const Ray &ray, float *t0, float *t1) const;
	virtual SWCSpectrum SigmaA(const SpectrumWavelengths &sw, const Point &,
		const Vector &) const;
	virtual SWCSpectrum SigmaS(const SpectrumWavelengths &sw, const Point &,
		const Vector &) const;
	virtual SWCSpectrum Lve(const SpectrumWavelengths &sw, const Point &,
		const Vector &) const;
	virtual float P(const SpectrumWavelengths &sw, const Point &,
		const Vector &, const Vector &) const;
	virtual SWCSpectrum SigmaT(const SpectrumWavelengths &sw, const Point &,
		const Vector &) const;
	virtual SWCSpectrum Tau(const SpectrumWavelengths &sw, const Ray &ray,
		float stepSize, float u) const;
	virtual FresnelGeneral Fresnel(const SpectrumWavelengths &sw,
		const Point &, const Vector &) const {
		return FresnelGeneral(DIELECTRIC_FRESNEL,
			SWCSpectrum(1.f), SWCSpectrum(0.f));
	}
private:
	// AggregateRegion Private Data
	vector<Region *> regions;
};

}//namespace lux

#endif // LUX_VOLUME_H
