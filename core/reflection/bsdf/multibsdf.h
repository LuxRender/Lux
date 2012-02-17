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

#ifndef LUX_MULTIBSDF_H
#define LUX_MULTIBSDF_H
// multibsdf.h*
#include "bxdf.h"
#include "geometry/raydifferential.h"
#include "spectrum.h"

namespace lux
{

// Multiple BxDF BSDF declaration
class  MultiBSDF : public BSDF  {
public:
	// MultiBSDF Public Methods
	MultiBSDF(const DifferentialGeometry &dgs, const Normal &ngeom,
		const Volume *exterior, const Volume *interior, const SWCSpectrum bcolor = SWCSpectrum(0.f));
	inline void Add(BxDF *bxdf);
	virtual inline u_int NumComponents() const { return nBxDFs; }
	virtual inline u_int NumComponents(BxDFType flags) const;
	/**
	 * Samples the BSDF.
	 * Returns the result of the BSDF for the sampled direction in f.
	 */
	virtual bool SampleF(const SpectrumWavelengths &sw, const Vector &wo,
		Vector *wi, float u1, float u2, float u3, SWCSpectrum *const f,
		float *pdf, BxDFType flags = BSDF_ALL,
		BxDFType *sampledType = NULL, float *pdfBack = NULL,
		bool reverse = false) const;
	virtual float Pdf(const SpectrumWavelengths &sw, const Vector &wo,
		const Vector &wi, BxDFType flags = BSDF_ALL) const;
	virtual SWCSpectrum F(const SpectrumWavelengths &sw, const Vector &woW,
		const Vector &wiW, bool reverse,
		BxDFType flags = BSDF_ALL) const;
	virtual SWCSpectrum rho(const SpectrumWavelengths &sw,
		BxDFType flags = BSDF_ALL) const;
	virtual SWCSpectrum rho(const SpectrumWavelengths &sw, const Vector &wo,
		BxDFType flags = BSDF_ALL) const;

protected:
	// MultiBSDF Private Methods
	virtual ~MultiBSDF() { }
	// MultiBSDF Private Data
	u_int nBxDFs;
	#define MAX_BxDFS 8
	BxDF *bxdfs[MAX_BxDFS];
};

// BSDF Inline Method Definitions
inline void MultiBSDF::Add(BxDF *b)
{
	BOOST_ASSERT(nBxDFs < MAX_BxDFS);
	bxdfs[nBxDFs++] = b;
}
inline u_int MultiBSDF::NumComponents(BxDFType flags) const
{
	u_int num = 0;
	for (u_int i = 0; i < nBxDFs; ++i) {
		if (bxdfs[i]->MatchesFlags(flags))
			++num;
	}
	return num;
}

}//namespace lux

#endif // LUX_MULTIBSDF_H
