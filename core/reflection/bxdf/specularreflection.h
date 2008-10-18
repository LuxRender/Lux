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
	SpecularReflection(const SWCSpectrum &r, Fresnel *f, float flm, float flmindex)
		: BxDF(BxDFType(BSDF_REFLECTION | BSDF_SPECULAR)),
		  R(r), fresnel(f), film(flm), filmindex(flmindex) {
	}
	void f(const TsPack *tspack, const Vector &wo, const Vector &wi, SWCSpectrum *const f) const {
		
	}
	bool Sample_f(const TsPack *tspack, const Vector &wo, Vector *wi,
		float u1, float u2, SWCSpectrum *const f, float *pdf, float *pdfBack = NULL, bool reverse = false) const;
	virtual float Weight(const TsPack *tspack, const Vector &wo, bool reverse) const;
	float Pdf(const TsPack *tspack, const Vector &wo, const Vector &wi) const {
		return 0.;
	}
private:
	// SpecularReflection Private Data
	SWCSpectrum R;
	Fresnel *fresnel;
	float film, filmindex;
};

class ArchitecturalReflection : public SpecularReflection {
public:
	ArchitecturalReflection(const SWCSpectrum &r, Fresnel *f, float flm, float flmindex)
		: SpecularReflection(r, f, flm, flmindex) {}
	bool Sample_f(const TsPack *tspack, const Vector &wo, Vector *wi,
		float u1, float u2, SWCSpectrum *const f, float *pdf, float *pdfBack = NULL, bool reverse = false) const;
	float Weight(const TsPack *tspack, const Vector &wo, bool reverse) const;
};

}//namespace lux

#endif // LUX_SPECULARREFLECTION_H

