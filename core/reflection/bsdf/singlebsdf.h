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

#ifndef LUX_SINGLEBSDF_H
#define LUX_SINGLEBSDF_H
// singlebsdf.h*
#include "bxdf.h"
#include "geometry/raydifferential.h"
#include "spectrum.h"

namespace lux
{

// Single BxDF BSDF declaration
class  SingleBSDF : public BSDF  {
public:
	// StackedBSDF Public Methods
	SingleBSDF(const DifferentialGeometry &dgs, const Normal &ngeom,
		const BxDF *b, const Volume *exterior, const Volume *interior, const SWCSpectrum bcolor = SWCSpectrum(0.f) ) :
		BSDF(dgs, ngeom, exterior, interior, bcolor), bxdf(b) { }
	virtual inline u_int NumComponents() const { return 1; }
	virtual inline u_int NumComponents(BxDFType flags) const {
		return bxdf->MatchesFlags(flags) ? 1U : 0U;
	}
	/**
	 * Samples the BSDF.
	 * Returns the result of the BSDF for the sampled direction in f.
	 */
	virtual bool SampleF(const SpectrumWavelengths &sw, const Vector &woW,
		Vector *wiW, float u1, float u2, float u3,
		SWCSpectrum *const f_, float *pdf, BxDFType flags = BSDF_ALL,
		BxDFType *sampledType = NULL, float *pdfBack = NULL,
		bool reverse = false) const;
	virtual float Pdf(const SpectrumWavelengths &sw, const Vector &woW,
		const Vector &wiW, BxDFType flags = BSDF_ALL) const;
	virtual SWCSpectrum F(const SpectrumWavelengths &sw, const Vector &woW,
		const Vector &wiW, bool reverse,
		BxDFType flags = BSDF_ALL) const;
	virtual SWCSpectrum rho(const SpectrumWavelengths &sw,
		BxDFType flags = BSDF_ALL) const;
	virtual SWCSpectrum rho(const SpectrumWavelengths &sw,
		const Vector &woW, BxDFType flags = BSDF_ALL) const;

protected:
	// SingleBSDF Private Methods
	virtual ~SingleBSDF() { }
	// SingleBSDF Private Data
	const BxDF *bxdf;
};

}//namespace lux

#endif // LUX_SINGLEBSDF_H
