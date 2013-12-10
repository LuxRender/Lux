/***************************************************************************
 *   Copyright (C) 1998-2013 by authors (see AUTHORS.txt)                  *
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

#ifndef LUX_LAMBERTIAN_H
#define LUX_LAMBERTIAN_H
// lambertian.h*
#include "lux.h"
#include "bxdf.h"
#include "luxrays/core/color/swcspectrum.h"

namespace lux
{

class  Lambertian : public BxDF {
public:
	// Lambertian Public Methods
	Lambertian(const SWCSpectrum &reflectance)
		: BxDF(BxDFType(BSDF_REFLECTION | BSDF_DIFFUSE)),
		  R(reflectance) {
	}
	virtual ~Lambertian() { }
	virtual void F(const SpectrumWavelengths &sw, const Vector &wo,
		const Vector &wi, SWCSpectrum *const f) const;
	bool SampleF(const SpectrumWavelengths &sw, const Vector &wo,
		Vector *wi, float u1, float u2, SWCSpectrum *const f_,
		float *pdf, float *pdfBack, bool reverse) const;
	virtual SWCSpectrum rho(const SpectrumWavelengths &sw, const Vector &,
		u_int, float *) const { return R; }
	virtual SWCSpectrum rho(const SpectrumWavelengths &sw, u_int,
		float *) const { return R; }
private:
	// Lambertian Private Data
	SWCSpectrum R;
};

}//namespace lux

#endif // LUX_LAMBERTIAN_H

