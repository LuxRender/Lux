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
template<int MAX_BxDFS>
class  MultiBSDF : public BSDF  {
public:
	// MultiBSDF Public Methods
	MultiBSDF(const DifferentialGeometry &dgs, const Normal &ngeom,
		const Volume *exterior, const Volume *interior);
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
	u_int nBxDFs;
	// MultiBSDF Private Data
	BxDF *bxdfs[MAX_BxDFS];
};

// BSDF Inline Method Definitions
template<int MAX_BxDFS>
inline void MultiBSDF<MAX_BxDFS>::Add(BxDF *b)
{
	BOOST_ASSERT(nBxDFs < MAX_BxDFS);
	bxdfs[nBxDFs++] = b;
}
template<int MAX_BxDFS>
inline u_int MultiBSDF<MAX_BxDFS>::NumComponents(BxDFType flags) const
{
	u_int num = 0;
	for (u_int i = 0; i < nBxDFs; ++i) {
		if (bxdfs[i]->MatchesFlags(flags))
			++num;
	}
	return num;
}

template<int MAX_BxDFS>
MultiBSDF<MAX_BxDFS>::MultiBSDF(const DifferentialGeometry &dg, const Normal &ngeom,
	const Volume *exterior, const Volume *interior) :
	BSDF(dg, ngeom, exterior, interior)
{
	nBxDFs = 0;
}
template<int MAX_BxDFS>
bool MultiBSDF<MAX_BxDFS>::SampleF(const SpectrumWavelengths &sw, const Vector &woW, Vector *wiW,
	float u1, float u2, float u3, SWCSpectrum *const f_, float *pdf,
	BxDFType flags, BxDFType *sampledType, float *pdfBack,
	bool reverse) const
{
	float weights[MAX_BxDFS];
	// Choose which _BxDF_ to sample
	Vector wo(WorldToLocal(woW));
	u_int matchingComps = 0;
	float totalWeight = 0.f;
	for (u_int i = 0; i < nBxDFs; ++i) {
		if (bxdfs[i]->MatchesFlags(flags)) {
			weights[i] = bxdfs[i]->Weight(sw, wo);
			totalWeight += weights[i];
			++matchingComps;
		} else
			weights[i] = 0.f;
	}
	if (matchingComps == 0 || !(totalWeight > 0.f)) {
		*pdf = 0.f;
		if (pdfBack)
			*pdfBack = 0.f;
		return false;
	}
	u3 *= totalWeight;
	u_int which = 0;
	for (u_int i = 0; i < nBxDFs; ++i) {
		if (weights[i] > 0.f) {
			which = i;
			u3 -= weights[i];
			if (u3 < 0.f) {
				break;
			}
		}
	}
	BxDF *bxdf = bxdfs[which];
	BOOST_ASSERT(bxdf); // NOBOOK
	// Sample chosen _BxDF_
	Vector wi;
	if (!bxdf->SampleF(sw, wo, &wi, u1, u2, f_, pdf, pdfBack, reverse))
		return false;
	if (sampledType)
		*sampledType = bxdf->type;
	*wiW = LocalToWorld(wi);
	// Compute overall PDF with all matching _BxDF_s
	// Compute value of BSDF for sampled direction
	const float sideTest = Dot(*wiW, ng) / Dot(woW, ng);
	BxDFType flags2;
	if (sideTest > 0.f)
		// ignore BTDFs
		flags2 = BxDFType(flags & ~BSDF_TRANSMISSION);
	else if (sideTest < 0.f)
		// ignore BRDFs
		flags2 = BxDFType(flags & ~BSDF_REFLECTION);
	else
		return false;
	if (!bxdf->MatchesFlags(flags2))
		return false;
	if (!(bxdf->type & BSDF_SPECULAR) && matchingComps > 1 && !isinf(*pdf)) {
		*f_ *= *pdf;
		*pdf *= weights[which];
		float totalWeightR = bxdfs[which]->Weight(sw, wi);
		if (pdfBack)
			*pdfBack *= totalWeightR;
		for (u_int i = 0; i < nBxDFs; ++i) {
			if (i== which || !bxdfs[i]->MatchesFlags(flags))
				continue;
			if (bxdfs[i]->MatchesFlags(flags2)) {
				if (reverse)
					bxdfs[i]->F(sw, wi, wo, f_);
				else
					bxdfs[i]->F(sw, wo, wi, f_);
			}
			*pdf += bxdfs[i]->Pdf(sw, wo, wi) * weights[i];
			if (pdfBack) {
				const float weightR = bxdfs[i]->Weight(sw, wi);
				*pdfBack += bxdfs[i]->Pdf(sw, wi, wo) * weightR;
				totalWeightR += weightR;
			}
		}
		*pdf /= totalWeight;
		*f_ /= *pdf;
		if (pdfBack)
			*pdfBack /= totalWeightR;
	} else {
		const float w = weights[which] / totalWeight;
		*pdf *= w;
		*f_ /= w;
		if (pdfBack && matchingComps > 1) {
			float totalWeightR = bxdfs[which]->Weight(sw, wi);
			*pdfBack *= totalWeightR;
			for (u_int i = 0; i < nBxDFs; ++i) {
				if (i== which || !bxdfs[i]->MatchesFlags(flags))
					continue;
				const float weightR = bxdfs[i]->Weight(sw, wi);
				if (!(bxdf->type & BSDF_SPECULAR))
					*pdfBack += bxdfs[i]->Pdf(sw, wi, wo) * weightR;
				totalWeightR += weightR;
			}
			*pdfBack /= totalWeightR;
		}
	}
	if (!reverse)
		*f_ *= fabsf(sideTest);
	return true;
}
template<int MAX_BxDFS>
float MultiBSDF<MAX_BxDFS>::Pdf(const SpectrumWavelengths &sw, const Vector &woW, const Vector &wiW,
	BxDFType flags) const
{
	Vector wo(WorldToLocal(woW)), wi(WorldToLocal(wiW));
	float pdf = 0.f;
	float totalWeight = 0.f;
	for (u_int i = 0; i < nBxDFs; ++i)
		if (bxdfs[i]->MatchesFlags(flags)) {
			float weight = bxdfs[i]->Weight(sw, wo);
			pdf += bxdfs[i]->Pdf(sw, wo, wi) * weight;
			totalWeight += weight;
		}
	return totalWeight > 0.f ? pdf / totalWeight : 0.f;
}

template<int MAX_BxDFS>
SWCSpectrum MultiBSDF<MAX_BxDFS>::F(const SpectrumWavelengths &sw, const Vector &woW,
		const Vector &wiW, bool reverse, BxDFType flags) const
{
	const float sideTest = Dot(wiW, ng) / Dot(woW, ng);
	if (sideTest > 0.f)
		// ignore BTDFs
		flags = BxDFType(flags & ~BSDF_TRANSMISSION);
	else if (sideTest < 0.f)
		// ignore BRDFs
		flags = BxDFType(flags & ~BSDF_REFLECTION);
	else
		flags = static_cast<BxDFType>(0);
	Vector wi(WorldToLocal(wiW)), wo(WorldToLocal(woW));
	SWCSpectrum f_(0.f);
	for (u_int i = 0; i < nBxDFs; ++i)
		if (bxdfs[i]->MatchesFlags(flags))
			bxdfs[i]->F(sw, wo, wi, &f_);
	if (!reverse)
		f_ *= fabsf(sideTest);
	return f_;
}
template<int MAX_BxDFS>
SWCSpectrum MultiBSDF<MAX_BxDFS>::rho(const SpectrumWavelengths &sw, BxDFType flags) const
{
	SWCSpectrum ret(0.f);
	for (u_int i = 0; i < nBxDFs; ++i)
		if (bxdfs[i]->MatchesFlags(flags))
			ret += bxdfs[i]->rho(sw);
	return ret;
}
template<int MAX_BxDFS>
SWCSpectrum MultiBSDF<MAX_BxDFS>::rho(const SpectrumWavelengths &sw, const Vector &woW,
	BxDFType flags) const
{
	Vector wo(WorldToLocal(woW));
	SWCSpectrum ret(0.f);
	for (u_int i = 0; i < nBxDFs; ++i)
		if (bxdfs[i]->MatchesFlags(flags))
			ret += bxdfs[i]->rho(sw, wo);
	return ret;
}

}//namespace lux

#endif // LUX_MULTIBSDF_H
