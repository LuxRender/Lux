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

// clearvolume.h*
#include "volume.h"
#include "texture.h"
#include "geometry/raydifferential.h"
#include "fresnel.h"

namespace lux
{

// ClearVolume Declarations
class ClearVolume : public Volume {
public:
	ClearVolume(const boost::shared_ptr<Texture<const lux::Fresnel *> > &fr,
		boost::shared_ptr<Texture<SWCSpectrum> > &a) :
		fresnel(fr), absorption(a) { }
	virtual ~ClearVolume() { }
	virtual SWCSpectrum SigmaA(const TsPack *tspack, const Point &p,
		const Vector &) const {
		DifferentialGeometry dg; //FIXME give it as argument
		dg.p = p;
		return fresnel->Evaluate(tspack, dg)->SigmaA(tspack) +
			absorption->Evaluate(tspack, dg);
	}
	virtual SWCSpectrum SigmaS(const TsPack *tspack, const Point &,
		const Vector &) const { return SWCSpectrum(0.f); }
	virtual SWCSpectrum Lve(const TsPack *tspack, const Point &,
		const Vector &) const { return SWCSpectrum(0.f); }
	virtual float P(const TsPack *, const Point &, const Vector &,
		const Vector &) const { return 0.f; }
	virtual SWCSpectrum SigmaT(const TsPack *tspack, const Point &p,
		const Vector &w) const { return SigmaA(tspack, p, w); }
	virtual SWCSpectrum Tau(const TsPack *tspack, const Ray &ray,
		float step = 1.f, float offset = .5f) const {
		return SigmaT(tspack, ray.o, ray.d) * ray.d.Length() *
			(ray.maxt - ray.mint);
	}
	virtual const lux::Fresnel *Fresnel(const TsPack *tspack, const Point &p,
		const Vector &) const {
		DifferentialGeometry dg; //FIXME give it as argument
		dg.p = p;
		return fresnel->Evaluate(tspack, dg);
	}
	// ClearVolume Public Methods
	static Volume *CreateVolume(const Transform &volume2world, const ParamSet &params);
private:
	boost::shared_ptr<Texture<const lux::Fresnel *> > fresnel;
	boost::shared_ptr<Texture<SWCSpectrum> > absorption;
};

}//namespace lux
