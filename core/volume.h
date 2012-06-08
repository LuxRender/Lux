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
#include "primitive.h"
#include "spectrum.h"
#include "fresnelgeneral.h"
#include "color.h"
#include "materials/scattermaterial.h"

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
	virtual SWCSpectrum SigmaA(const SpectrumWavelengths &sw,
		const DifferentialGeometry &dg) const = 0;
	virtual SWCSpectrum SigmaS(const SpectrumWavelengths &sw,
		const DifferentialGeometry &dg) const = 0;
	virtual SWCSpectrum Lve(const SpectrumWavelengths &sw,
		const DifferentialGeometry &dg) const = 0;
	virtual float P(const SpectrumWavelengths &sw,
		const DifferentialGeometry &dg,
		const Vector &, const Vector &) const = 0;
	virtual SWCSpectrum SigmaT(const SpectrumWavelengths &sw,
		const DifferentialGeometry &dg) const {
		return SigmaA(sw, dg) + SigmaS(sw, dg);
	}
	virtual SWCSpectrum Tau(const SpectrumWavelengths &sw, const Ray &ray,
		float step = 1.f, float offset = 0.5f) const = 0;
	virtual FresnelGeneral Fresnel(const SpectrumWavelengths &sw,
		const DifferentialGeometry &dg) const = 0;
	virtual bool Scatter(const Sample &sample, bool scatteredStart,
		const Ray &ray, float u, Intersection *isect, float *pdf,
		float *pdfBack, SWCSpectrum *L) const = 0;
};

class RGBVolume : public Volume {
public:
	RGBVolume(const RGBColor &sA, const RGBColor &sS, const RGBColor &l,
		float gg) : sigA(sA), sigS(sS), le(l), g(gg),
		material(sS, sA, gg), primitive(&material, this, this) { }
	virtual ~RGBVolume() { }
	virtual SWCSpectrum SigmaA(const SpectrumWavelengths &sw,
		const DifferentialGeometry &dg) const {
		return SWCSpectrum(sw, sigA).Clamp();
	}
	virtual SWCSpectrum SigmaS(const SpectrumWavelengths &sw,
		const DifferentialGeometry &dg) const {
		return SWCSpectrum(sw, sigS);
	}
	virtual SWCSpectrum Lve(const SpectrumWavelengths &sw,
		const DifferentialGeometry &dg) const {
		//FIXME - use a D65 white point instead of the E white point
		return SWCSpectrum(sw, le);
	}
	virtual float P(const SpectrumWavelengths &sw,
		const DifferentialGeometry &dg,
		const Vector &w, const Vector &wp) const {
		return PhaseHG(w, wp, g);
	}
	virtual SWCSpectrum Tau(const SpectrumWavelengths &sw, const Ray &ray,
		float step = 1.f, float offset = 0.5f) const {
		DifferentialGeometry dg;
		dg.p = ray.o;
		dg.nn = Normal(-ray.d);
		const SWCSpectrum sigma(SigmaT(sw, dg));
		if (sigma.Black())
			return SWCSpectrum(0.f);
		const float rl = ray.d.Length() * (ray.maxt - ray.mint);
		SWCSpectrum tau;
		for (u_int i = 0; i < WAVELENGTH_SAMPLES; i++) {
			// avoid NaNs by defining zero absorption coefficient as no absorption
			tau.c[i] = (sigma.c[i] <= 0.f) ? 0.f : sigma.c[i] * rl;
		}
		return tau;
	}
	virtual FresnelGeneral Fresnel(const SpectrumWavelengths &sw,
		const DifferentialGeometry &dg) const {
		return FresnelGeneral(DIELECTRIC_FRESNEL,
			SWCSpectrum(1.f), SWCSpectrum(0.f));
	}
	virtual bool Scatter(const Sample &sample, bool scatteredStart,
		const Ray &ray, float u, Intersection *isect, float *pdf,
		float *pdfBack, SWCSpectrum *L) const;
private:
	RGBColor sigA, sigS, le;
	float g;
	UniformRGBScatterMaterial material;
	ScattererPrimitive primitive;
};

class  Region : public Volume {
public:
	// VolumeRegion Interface
	Region() { }
	Region(const BBox &b) : bound(b) { }
	virtual ~Region() { }
	virtual BBox WorldBound() const { return bound; };
	virtual bool IntersectP(const Ray &ray, float *t0, float *t1, bool null_shp_isect = false) const {
		return bound.IntersectP(ray, t0, t1, null_shp_isect);
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
	virtual bool IntersectP(const Ray &ray, float *t0, float *t1, bool null_shp_isect = false) const {
		return region.IntersectP(WorldToVolume(ray), t0, t1, null_shp_isect);
	}
	virtual SWCSpectrum SigmaA(const SpectrumWavelengths &sw,
		const DifferentialGeometry &dg) const {
		return region.Inside(WorldToVolume(dg.p)) ?
			volume.SigmaA(sw, dg) : SWCSpectrum(0.f);
	}
	virtual SWCSpectrum SigmaS(const SpectrumWavelengths &sw,
		const DifferentialGeometry &dg) const {
		return region.Inside(WorldToVolume(dg.p)) ?
			volume.SigmaS(sw, dg) : SWCSpectrum(0.f);
	}
	virtual SWCSpectrum SigmaT(const SpectrumWavelengths &sw,
		const DifferentialGeometry &dg) const {
		return region.Inside(WorldToVolume(dg.p)) ?
			volume.SigmaT(sw, dg) : SWCSpectrum(0.f);
	}
	virtual SWCSpectrum Lve(const SpectrumWavelengths &sw,
		const DifferentialGeometry &dg) const {
		return region.Inside(WorldToVolume(dg.p)) ?
			volume.Lve(sw, dg) : SWCSpectrum(0.f);
	}
	virtual float P(const SpectrumWavelengths &sw,
		const DifferentialGeometry &dg,
		const Vector &w, const Vector &wp) const {
		return volume.P(sw, dg, w, wp);
	}
	virtual SWCSpectrum Tau(const SpectrumWavelengths &sw, const Ray &r,
		float stepSize, float offset) const {
		Ray rn(r);
		if (!IntersectP(rn, &rn.mint, &rn.maxt))
			return SWCSpectrum(0.f);
		return volume.Tau(sw, rn, stepSize, offset);
	}
	virtual FresnelGeneral Fresnel(const SpectrumWavelengths &sw,
		const DifferentialGeometry &dg) const {
		return volume.Fresnel(sw, dg);
	}
	virtual bool Scatter(const Sample &sample, bool scatteredStart,
		const Ray &ray, float u, Intersection *isect, float *pdf,
		float *pdfBack, SWCSpectrum *L) const {
		Ray r(WorldToVolume(ray));
		if (!region.IntersectP(r, &r.mint, &r.maxt))
			return false;
		if (r.maxt <= r.mint)
			return false;
		if (!volume.Scatter(sample, scatteredStart, r, u, isect, pdf,
			pdfBack, L))
			return false;
		ray.maxt = r.maxt;
		Transform VolumeToWorld(WorldToVolume.GetInverse());
		isect->dg.p = VolumeToWorld(isect->dg.p);
		isect->dg.nn = VolumeToWorld(isect->dg.nn);
		isect->dg.dpdu = VolumeToWorld(isect->dg.dpdu);
		isect->dg.dpdv = VolumeToWorld(isect->dg.dpdv);
		isect->dg.dndu = VolumeToWorld(isect->dg.dndu);
		isect->dg.dndv = VolumeToWorld(isect->dg.dndv);
		return true;
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
		const DifferentialGeometry &dg) const {
		return Density(dg.p) * volume.SigmaA(sw, dg);
	}
	virtual SWCSpectrum SigmaS(const SpectrumWavelengths &sw,
		const DifferentialGeometry &dg) const {
		return Density(dg.p) * volume.SigmaS(sw, dg);
	}
	virtual SWCSpectrum SigmaT(const SpectrumWavelengths &sw,
		const DifferentialGeometry &dg) const {
		return Density(dg.p) * volume.SigmaT(sw, dg);
	}
	virtual SWCSpectrum Lve(const SpectrumWavelengths &sw,
		const DifferentialGeometry &dg) const {
		return Density(dg.p) * volume.Lve(sw, dg);
	}
	virtual float P(const SpectrumWavelengths &sw,
		const DifferentialGeometry &dg,
		const Vector &w, const Vector &wp) const {
		return volume.P(sw, dg, w, wp);
	}
	virtual SWCSpectrum Tau(const SpectrumWavelengths &sw, const Ray &r,
		float stepSize, float offset) const {
		const float length = r.d.Length();
		if (!(length > 0.f))
			return SWCSpectrum(0.f);
		const u_int N = Ceil2UInt((r.maxt - r.mint) * length / stepSize);
		const float step = (r.maxt - r.mint) / N;
		DifferentialGeometry dg;
		dg.nn = Normal(-r.d);
		SWCSpectrum tau(0.f);
		float t0 = r.mint + offset * step;
		for (u_int i = 0; i < N; ++i) {
			dg.p = r(t0);
			tau += SigmaT(sw, dg);
			t0 += step;
		}
		return tau * (step * length);
	}
	virtual FresnelGeneral Fresnel(const SpectrumWavelengths &sw,
		const DifferentialGeometry &dg) const {
		return volume.Fresnel(sw, dg);
	}
	virtual bool Scatter(const Sample &sample, bool scatteredStart,
		const Ray &ray, float u, Intersection *isect, float *pdf,
		float *pdfBack, SWCSpectrum *L) const {
		if (pdf)
			*pdf = 1.f;
		if (pdfBack)
			*pdfBack = 1.f;
		const float mint = ray.mint;
		while (true) {
			float lpdf, lpdfBack;
			bool scatter = volume.Scatter(sample, scatteredStart,
				ray, u, isect, &lpdf, &lpdfBack, L);
			if (!scatter) {
				ray.mint = mint;
				//TODO compute correct probabilities
				return false;
			}
			//TODO update probabilities
			const float d = Density(ray(ray.maxt));
			if (d >= u) //FIXME: need to decouple random values
				break;
			//*pdf *= 1.f - d;
			ray.mint = ray.maxt;
		}
		ray.mint = mint;
		return false; //FIXME scattering disabled for now
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
	virtual bool IntersectP(const Ray &ray, float *t0, float *t1, bool null_shp_isect = false) const;
	virtual SWCSpectrum SigmaA(const SpectrumWavelengths &sw,
		const DifferentialGeometry &dg) const;
	virtual SWCSpectrum SigmaS(const SpectrumWavelengths &sw,
		const DifferentialGeometry &dg) const;
	virtual SWCSpectrum Lve(const SpectrumWavelengths &sw,
		const DifferentialGeometry &dg) const;
	virtual float P(const SpectrumWavelengths &sw,
		const DifferentialGeometry &dg,
		const Vector &, const Vector &) const;
	virtual SWCSpectrum SigmaT(const SpectrumWavelengths &sw,
		const DifferentialGeometry &dg) const;
	virtual SWCSpectrum Tau(const SpectrumWavelengths &sw, const Ray &ray,
		float stepSize, float u) const;
	virtual FresnelGeneral Fresnel(const SpectrumWavelengths &sw,
		const DifferentialGeometry &dg) const {
		return FresnelGeneral(DIELECTRIC_FRESNEL,
			SWCSpectrum(1.f), SWCSpectrum(0.f));
	}
	virtual bool Scatter(const Sample &sample, bool scatteredStart,
		const Ray &ray, float u, Intersection *isect, float *pdf,
		float *pdfBack, SWCSpectrum *L) const;
private:
	// AggregateRegion Private Data
	vector<Region *> regions;
};

}//namespace lux

#endif // LUX_VOLUME_H
