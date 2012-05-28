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
#include "sampling.h"
#include "spectrum.h"

namespace lux
{

// ClearVolume Declarations
class ClearVolume : public Volume {
public:
	ClearVolume(const boost::shared_ptr<Texture<FresnelGeneral> > &fr,
		boost::shared_ptr<Texture<SWCSpectrum> > &a) :
		fresnel(fr), absorption(a) { }
	virtual ~ClearVolume() { }
	virtual SWCSpectrum SigmaA(const SpectrumWavelengths &sw,
		const DifferentialGeometry &dg) const {
		return fresnel->Evaluate(sw, dg).SigmaA(sw) +
			absorption->Evaluate(sw, dg).Clamp();
	}
	virtual SWCSpectrum SigmaS(const SpectrumWavelengths &sw,
		const DifferentialGeometry &dg) const {
		return SWCSpectrum(0.f);
	}
	virtual SWCSpectrum Lve(const SpectrumWavelengths &sw,
		const DifferentialGeometry &dg) const {
		return SWCSpectrum(0.f);
	}
	virtual float P(const SpectrumWavelengths &,
		const DifferentialGeometry &dg,
		const Vector &, const Vector &) const { return 0.f; }
	virtual SWCSpectrum SigmaT(const SpectrumWavelengths &sw,
		const DifferentialGeometry &dg) const {
		return SigmaA(sw, dg);
	}
	virtual SWCSpectrum Tau(const SpectrumWavelengths &sw, const Ray &ray,
		float step = 1.f, float offset = .5f) const {
		DifferentialGeometry dg;
		dg.p = ray.o;
		dg.nn = Normal(-ray.d);
		const SWCSpectrum sigma(SigmaT(sw, dg));
		if (sigma.Black())
			return SWCSpectrum(0.f);
		return sigma * ray.d.Length() * (ray.maxt - ray.mint);
	}
	virtual FresnelGeneral Fresnel(const SpectrumWavelengths &sw,
		const DifferentialGeometry &dg) const {
		return fresnel->Evaluate(sw, dg);
	}
	bool Scatter(const Sample &sample, bool scatteredStart, const Ray &ray,
		float u, Intersection *isect, float *pdf, float *pdfBack,
		SWCSpectrum *L) const {
		if (L)
			*L *= Exp(-Tau(sample.swl, ray));
		if (pdf)
			*pdf = 1.f;
		if (pdfBack)
			*pdfBack = 1.f;
		return false;
	}
	// ClearVolume Public Methods
	static Volume *CreateVolume(const Transform &volume2world, const ParamSet &params);
private:
	boost::shared_ptr<Texture<FresnelGeneral> > fresnel;
	boost::shared_ptr<Texture<SWCSpectrum> > absorption;
};

}//namespace lux
