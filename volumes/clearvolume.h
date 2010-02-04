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
	ClearVolume(const boost::shared_ptr<Texture<const lux::Fresnel *> > &fr) :
		fresnel(fr) { }
	virtual ~ClearVolume() { }
	virtual SWCSpectrum SigmaA(const TsPack *tspack, const Point &,
		const Vector &) const { return SWCSpectrum(0.f); }
	virtual SWCSpectrum SigmaS(const TsPack *tspack, const Point &,
		const Vector &) const { return SWCSpectrum(0.f); }
	virtual SWCSpectrum Lve(const TsPack *tspack, const Point &,
		const Vector &) const { return SWCSpectrum(0.f); }
	virtual float P(const TsPack *, const Point &, const Vector &,
		const Vector &) const { return 0.f; }
	virtual SWCSpectrum SigmaT(const TsPack *tspack, const Point &,
		const Vector &) const { return SWCSpectrum(0.f); }
	virtual SWCSpectrum Tau(const TsPack *tspack, const Ray &ray,
		float step = 1.f, float offset = .5f) const {
		const lux::Fresnel *fr = Fresnel(tspack, ray.o, ray.d);
		return fr->SigmaA(tspack) * ray.d.Length() *
			(ray.maxt - ray.mint);
	}
	virtual const lux::Fresnel *Fresnel(const TsPack *tspack, const Point &P,
		const Vector &) const {
		DifferentialGeometry dg;
		dg.p = P;
		return fresnel->Evaluate(tspack, dg);
	}
	// ClearVolume Public Methods
	static Volume *CreateVolume(const Transform &volume2world, const ParamSet &params);
private:
	boost::shared_ptr<Texture<const lux::Fresnel *> > fresnel;
};

}//namespace lux
