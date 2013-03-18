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

// mixbxdf.cpp*
#include "mixbsdf.h"
#include "spectrum.h"

using namespace lux;

MixBSDF::MixBSDF(const DifferentialGeometry &dgs, const Normal &ngeom,
	const Volume *exterior, const Volume *interior) :
	BSDF(dgs, ngeom, exterior, interior), totalWeight(1.f)
{
	// totalWeight is initialized to 1 to avoid divisions by 0 when there
	// are no components in the mix
	nBSDFs = 0;
}
bool MixBSDF::SampleF(const SpectrumWavelengths &sw, const Vector &wo, Vector *wi,
	float u1, float u2, float u3, SWCSpectrum *const f_, float *pdf,
	BxDFType flags, BxDFType *sampledType, float *pdfBack,
	bool reverse) const
{
	if (nBSDFs == 0)
		return false;
	u3 *= totalWeight;
	u_int which = 0;
	for (u_int i = 0; i < nBSDFs; ++i) {
		if (u3 < weights[i]) {
			which = i;
			break;
		}
		u3 -= weights[i];
	}
	BxDFType sType;
	if (!bsdfs[which]->SampleF(sw,
		wo, wi, u1, u2, u3 / weights[which], f_, pdf, flags,
		&sType, pdfBack, reverse))
		return false;
	*pdf *= weights[which];
	*f_ *= *pdf;
	if (pdfBack)
		*pdfBack *= weights[which];
	if (sType & BSDF_SPECULAR)
		flags = sType;
	for (u_int i = 0; i < nBSDFs; ++i) {
		if (i == which)
			continue;
		BSDF *bsdf = bsdfs[i];
		if (bsdf->NumComponents(flags) == 0)
			continue;
		if (reverse)
			f_->AddWeighted(weights[i],
				bsdf->F(sw, *wi, wo, true, flags));
		else
			f_->AddWeighted(weights[i],
				bsdf->F(sw, wo, *wi, false, flags));
		*pdf += weights[i] * bsdf->Pdf(sw, wo, *wi, flags);
		if (pdfBack)
			*pdfBack += weights[i] * bsdf->Pdf(sw, *wi, wo, flags);
	}
	*pdf /= totalWeight;
	if (pdfBack)
		*pdfBack /= totalWeight;
	*f_ /= *pdf;
	if (sampledType)
		*sampledType = sType;

	// The conversion between RGB and SWCSpectrum is quite expansive so it is
	// better to skip the computation if it is useless.
	if ((dgShading.color.c[0] != 1.f) || (dgShading.color.c[1] != 1.f) || (dgShading.color.c[2] != 1.f))
		*f_ *= SWCSpectrum(sw, dgShading.color);

	return true;
}
float MixBSDF::Pdf(const SpectrumWavelengths &sw, const Vector &wo, const Vector &wi,
	BxDFType flags) const
{
	float pdf = 0.f;
	for (u_int i = 0; i < nBSDFs; ++i)
		pdf += weights[i] * bsdfs[i]->Pdf(sw, wo, wi, flags);
	return pdf / totalWeight;
}
SWCSpectrum MixBSDF::F(const SpectrumWavelengths &sw, const Vector &woW,
	const Vector &wiW, bool reverse, BxDFType flags) const
{
	SWCSpectrum ff(0.f);
	for (u_int i = 0; i < nBSDFs; ++i) {
		ff.AddWeighted(weights[i],
			bsdfs[i]->F(sw, woW, wiW, reverse, flags));
	}

	SWCSpectrum f = ff / totalWeight;
	
	// The conversion between RGB and SWCSpectrum is quite expansive so it is
	// better to skip the computation if it is useless.
	if ((dgShading.color.c[0] != 1.f) || (dgShading.color.c[1] != 1.f) || (dgShading.color.c[2] != 1.f))
		f *= SWCSpectrum(sw, dgShading.color);

	return f;
}
SWCSpectrum MixBSDF::rho(const SpectrumWavelengths &sw, BxDFType flags) const
{
	SWCSpectrum ret(0.f);
	for (u_int i = 0; i < nBSDFs; ++i)
		ret.AddWeighted(weights[i], bsdfs[i]->rho(sw, flags));
	ret /= totalWeight;

	// The conversion between RGB and SWCSpectrum is quite expansive so it is
	// better to skip the computation if it is useless.
	if ((dgShading.color.c[0] != 1.f) || (dgShading.color.c[1] != 1.f) || (dgShading.color.c[2] != 1.f))
		ret *= SWCSpectrum(sw, dgShading.color);

	return ret;
}
SWCSpectrum MixBSDF::rho(const SpectrumWavelengths &sw, const Vector &wo,
	BxDFType flags) const
{
	SWCSpectrum ret(0.f);
	for (u_int i = 0; i < nBSDFs; ++i)
		ret.AddWeighted(weights[i], bsdfs[i]->rho(sw, wo, flags));
	ret /= totalWeight;

	// The conversion between RGB and SWCSpectrum is quite expansive so it is
	// better to skip the computation if it is useless.
	if ((dgShading.color.c[0] != 1.f) || (dgShading.color.c[1] != 1.f) || (dgShading.color.c[2] != 1.f))
		ret *= SWCSpectrum(sw, dgShading.color);

	return ret;
}

float MixBSDF::ApplyTransform(const Transform &transform)
{
	for (u_int i = 0; i < nBSDFs; ++i)
		bsdfs[i]->ApplyTransform(transform);
	return this->BSDF::ApplyTransform(transform);
}

