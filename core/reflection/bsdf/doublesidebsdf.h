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

#ifndef LUX_DOUBLESIDEBSDF_H
#define LUX_DOUBLESIDEBSDF_H

#include "bxdf.h"
#include "geometry/raydifferential.h"
#include "spectrum.h"

namespace lux
{

// DoubleSide BSDF Declarations
class DoubleSideBSDF : public BSDF {
public:
	// MixBSDF Public Methods
	DoubleSideBSDF(const DifferentialGeometry &dgs, const Normal &ngeom,
		const Volume *exterior, const Volume *interior);

	inline void SetFrontMaterial(BSDF *bsdf);
	inline void SetBackMaterial(BSDF *bsdf);

	virtual inline u_int NumComponents() const;
	virtual inline u_int NumComponents(BxDFType flags) const;
	virtual inline void SetCompositingParams(const CompositingParams *cp) {
		compParams = cp;
		frontBSDF->SetCompositingParams(cp);
		backBSDF->SetCompositingParams(cp);
	}

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
	virtual float ApplyTransform(const Transform &transform);

private:
	// DoubleSideBSDF Private Methods
	virtual ~DoubleSideBSDF() { }
	// DoubleSideBSDF Private Data

	BSDF *frontBSDF, *backBSDF;
};

inline void DoubleSideBSDF::SetFrontMaterial(BSDF *bsdf) {
	frontBSDF = bsdf;
}

inline void DoubleSideBSDF::SetBackMaterial(BSDF *bsdf) {
	backBSDF = bsdf;
}

inline u_int DoubleSideBSDF::NumComponents() const {
	u_int num = 0;

	num += frontBSDF->NumComponents();
	num += backBSDF->NumComponents();

	return num;
}

inline u_int DoubleSideBSDF::NumComponents(BxDFType flags) const {
	u_int num = 0;
	
	num += frontBSDF->NumComponents(flags);
	num += backBSDF->NumComponents(flags);

	return num;
}

}//namespace lux

#endif // LUX_DOUBLESIDEBSDF_H
