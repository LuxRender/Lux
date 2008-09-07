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

// homogeneous.cpp*
#include "volume.h"

namespace lux
{

// HomogeneousVolume Declarations
class HomogeneousVolume : public VolumeRegion {
public:
	// HomogeneousVolume Public Methods
	HomogeneousVolume(const RGBColor &sa, const RGBColor &ss, float gg,
		 	const RGBColor &emit, const BBox &e,
			const Transform &v2w) {
		WorldToVolume = v2w.GetInverse();
		sig_a = sa;
		sig_s = ss;
		g = gg;
		le = emit;
		extent = e;
	}
	BBox WorldBound() const {
		return WorldToVolume.GetInverse()(extent);
	}
	bool IntersectP(const Ray &r, float *t0, float *t1) const {
		Ray ray = WorldToVolume(r);
		return extent.IntersectP(ray, t0, t1);
	}
	RGBColor sigma_a(const Point &p, const Vector &) const {
		return extent.Inside(WorldToVolume(p)) ? sig_a : 0.;
	}
	RGBColor sigma_s(const Point &p, const Vector &) const {
		return extent.Inside(WorldToVolume(p)) ? sig_s : 0.;
	}
	RGBColor sigma_t(const Point &p, const Vector &) const {
		return extent.Inside(WorldToVolume(p)) ? (sig_a + sig_s) : RGBColor(0.f);
	}
	RGBColor Lve(const Point &p, const Vector &) const {
		return extent.Inside(WorldToVolume(p)) ? le : 0.;
	}
	float p(const Point &p, const Vector &wi, const Vector &wo) const {
		if (!extent.Inside(WorldToVolume(p))) return 0.;
		return PhaseHG(wi, wo, g);
	}
	RGBColor Tau(const Ray &ray, float, float) const {
		float t0, t1;
		if (!IntersectP(ray, &t0, &t1)) return 0.;

		return Distance(ray(t0), ray(t1)) * (sig_a + sig_s);
	}
	
	static VolumeRegion *CreateVolumeRegion(const Transform &volume2world, const ParamSet &params);
private:
	// HomogeneousVolume Private Data
	RGBColor sig_a, sig_s, le;
	float g;
	BBox extent;
	Transform WorldToVolume;
};

}//namespace lux
