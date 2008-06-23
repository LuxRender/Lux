/***************************************************************************
 *   Copyright (C) 1998-2008 by authors (see AUTHORS.txt )                 *
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

#ifndef LUX_SPECULARREFLECTION_H
#define LUX_SPECULARREFLECTION_H
// specularreflection.h*
#include "lux.h"
#include "bxdf.h"
#include "spectrum.h"

namespace lux
{

class  SpecularReflection : public BxDF {
public:
	// SpecularReflection Public Methods
	SpecularReflection(const SWCSpectrum &r, Fresnel *f)
		: BxDF(BxDFType(BSDF_REFLECTION | BSDF_SPECULAR)),
		  R(r), fresnel(f) {
	}
	SWCSpectrum f(const Vector &, const Vector &) const {
		return SWCSpectrum(0.);
	}
	virtual SWCSpectrum Sample_f(const Vector &wo, Vector *wi,
		float u1, float u2, float *pdf, float *pdfBack = NULL) const;
	float Pdf(const Vector &wo, const Vector &wi) const {
		return 0.;
	}
private:
	// SpecularReflection Private Data
	SWCSpectrum R;
	Fresnel *fresnel;
};

class ArchitecturalReflection : public SpecularReflection {
public:
	ArchitecturalReflection(const SWCSpectrum &r, Fresnel *f)
		: SpecularReflection(r, f) {}
	SWCSpectrum Sample_f(const Vector &wo, Vector *wi,
		float u1, float u2, float *pdf, float *pdfBack = NULL) const;
};

}//namespace lux

#endif // LUX_SPECULARREFLECTION_H

