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

#ifndef LUX_SCHLICKBSDF_H
#define LUX_SCHLICKBSDF_H
// schlickbsdf.h*
#include "bxdf.h"
#include "microfacetdistribution.h"
#include "geometry/raydifferential.h"
#include "luxrays/core/color/swcspectrum.h"

namespace lux
{

// SchlickBSDF BSDF declaration
class  SchlickBSDF : public BSDF  {
public:
	// SchlickBSDF Public Methods
	SchlickBSDF(const DifferentialGeometry &dgs, const Normal &ngeom,
		const Fresnel *coatingFresnel, const MicrofacetDistribution *coatingDistribution,
		bool multibounce, const SWCSpectrum &alpha, float depth, 
		BSDF *base, const Volume *exterior, const Volume *interior);
	virtual inline u_int NumComponents() const;
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
	virtual float ApplyTransform(const Transform &transform) {
		base->ApplyTransform(transform);
		return this->BSDF::ApplyTransform(transform);
	}
protected:
	// SchlickBSDF Private Methods
	virtual ~SchlickBSDF() { }
	// Helper function, used by SampleF() and Pdf()
	float CoatingWeight(const SpectrumWavelengths &sw, const Vector &wo) const;
	bool CoatingMatchesFlags(BxDFType flags) const;
	void CoatingF(const SpectrumWavelengths &sw, const Vector &wo,
		const Vector &wi, SWCSpectrum *const f) const;
	bool CoatingSampleF(const SpectrumWavelengths &sw, const Vector &wo,
		Vector *wi, float u1, float u2, SWCSpectrum *const f,
		float *pdf, float *pdfBack = NULL, bool reverse = false) const;
	float CoatingPdf(const SpectrumWavelengths &sw, const Vector &wi,
		const Vector &wo) const;
	SWCSpectrum CoatingRho(const SpectrumWavelengths &sw, const Vector &w, u_int nSamples = 16) const;
	SWCSpectrum CoatingRho(const SpectrumWavelengths &sw, u_int nSamples = 16) const;
	// SchlickBSDF Private Data
	BxDFType coatingType;
	const Fresnel *fresnel;
	const MicrofacetDistribution *distribution;
	bool multibounce;
	SWCSpectrum Alpha;
	float depth;
	BSDF *base;
};

// BSDF Inline Method Definitions
inline u_int SchlickBSDF::NumComponents() const
{
	return 1U + base->NumComponents();
}
inline u_int SchlickBSDF::NumComponents(BxDFType flags) const
{
	return (CoatingMatchesFlags(flags) ? 1U : 0U) +
		base->NumComponents(flags);
}
inline bool SchlickBSDF::CoatingMatchesFlags(BxDFType flags) const
{
	return (coatingType & flags) == coatingType;
}
}//namespace lux

#endif // LUX_BXDF_H
