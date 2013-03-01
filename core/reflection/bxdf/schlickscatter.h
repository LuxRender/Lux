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

#ifndef LUX_SCHLICKSCATTER_H
#define LUX_SCHLICKSCATTER_H
// schlickscatter.h*
#include "lux.h"
#include "bxdf.h"
#include "spectrum.h"

namespace lux
{

class  SchlickScatter : public BSDF
{
public:
	// SchlickScatter Public Methods
	SchlickScatter(const DifferentialGeometry &dgs, const Normal &ngeom,
		const Volume *exterior, const Volume *interior,
		const SWCSpectrum &R, const SWCSpectrum &g);
	virtual ~SchlickScatter() { }
	virtual u_int NumComponents() const { return 1; }
	virtual u_int NumComponents(BxDFType flags) const {
		return flags & BSDF_DIFFUSE ? 1 : 0;
	}
	virtual SWCSpectrum F(const SpectrumWavelengths &sw, const Vector &wo,
		const Vector &wi, bool reverse,
		BxDFType flags = BSDF_ALL) const;
	virtual bool SampleF(const SpectrumWavelengths &sw, const Vector &wo,
		Vector *wi, float u1, float u2, float u3, SWCSpectrum *const f,
		float *pdf, BxDFType flags = BSDF_ALL,
		BxDFType *sampled_type = NULL, float *pdfBack = NULL,
		bool reverse = false) const;
	virtual float Pdf(const SpectrumWavelengths &sw, const Vector &wo,
		const Vector &wi, BxDFType flags = BSDF_ALL) const;
	virtual SWCSpectrum rho(const SpectrumWavelengths &sw,
		BxDFType flags = BSDF_ALL) const {
		return flags & BSDF_DIFFUSE ? R : SWCSpectrum(0.f);
	}
	virtual SWCSpectrum rho(const SpectrumWavelengths &sw, const Vector &wo,
		BxDFType flags = BSDF_ALL) const {
		return flags & BSDF_DIFFUSE ? R : SWCSpectrum(0.f);
	}

protected:
	// SchlickScatter Private Data
	SWCSpectrum R, k;
};

}//namespace lux

#endif // LUX_SCHLICKSCATTER_H

