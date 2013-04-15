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
#include "doublesidebsdf.h"

using namespace lux;

DoubleSideBSDF::DoubleSideBSDF(const DifferentialGeometry &dgs, const Normal &ngeom,
	const Volume *exterior, const Volume *interior) :
	BSDF(dgs, ngeom, exterior, interior) {
}

bool DoubleSideBSDF::SampleF(const SpectrumWavelengths &sw, const Vector &wo, Vector *wi,
	float u1, float u2, float u3, SWCSpectrum *const f_, float *pdf,
	BxDFType flags, BxDFType *sampledType, float *pdfBack,
	bool reverse) const {
	const bool into = (Dot(wo, ng) > 0.f);

	if (into)
		return frontBSDF->SampleF(sw, wo, wi, u1, u2, u3, f_, pdf, flags, sampledType, pdfBack, reverse);
	else
		return backBSDF->SampleF(sw, wo, wi, u1, u2, u3, f_, pdf, flags, sampledType, pdfBack, reverse);
}

float DoubleSideBSDF::Pdf(const SpectrumWavelengths &sw, const Vector &wo, const Vector &wi,
	BxDFType flags) const {
	// Transmission is not supported by DoubleSide
	const bool into = (Dot(wo, ng) > 0.f);

	if (into)
		return frontBSDF->Pdf(sw, wo, wi);
	else
		return backBSDF->Pdf(sw, wo, wi);
}

SWCSpectrum DoubleSideBSDF::F(const SpectrumWavelengths &sw, const Vector &woW,
	const Vector &wiW, bool reverse, BxDFType flags) const {
	// Transmission is not supported by DoubleSide
	const bool into = (Dot(woW, ng) > 0.f);

	if (into)
		return frontBSDF->F(sw, woW, wiW, reverse, flags);
	else
		return backBSDF->F(sw, woW, wiW, reverse, flags);
}

SWCSpectrum DoubleSideBSDF::rho(const SpectrumWavelengths &sw, BxDFType flags) const {
	return frontBSDF->rho(sw, flags);
}

SWCSpectrum DoubleSideBSDF::rho(const SpectrumWavelengths &sw, const Vector &wo,
	BxDFType flags) const {
	// Transmission is not supported by DoubleSide
	const bool into = (Dot(wo, ng) > 0.f);

	if (into)
		return frontBSDF->rho(sw, wo, flags);
	else
		return backBSDF->rho(sw, wo, flags);
}

float DoubleSideBSDF::ApplyTransform(const Transform &transform) {
	frontBSDF->ApplyTransform(transform);
	backBSDF->ApplyTransform(transform);

	return this->BSDF::ApplyTransform(transform);
}

