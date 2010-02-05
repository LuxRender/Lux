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

#ifndef LUX_SPECULARREFLECTION_H
#define LUX_SPECULARREFLECTION_H
// specularreflection.h*
#include "lux.h"
#include "bxdf.h"
#include "spectrum.h"

namespace lux
{

class SimpleSpecularReflection : public BxDF {
public:
	SimpleSpecularReflection(const Fresnel *fr) :
		BxDF(BxDFType(BSDF_REFLECTION | BSDF_SPECULAR)),
		fresnel(fr) { }
	virtual ~SimpleSpecularReflection() { }
	virtual void f(const TsPack *tspack, const Vector &wo, const Vector &wi,
		SWCSpectrum *const f_) const { }
	virtual bool Sample_f(const TsPack *tspack, const Vector &wo,
		Vector *wi, float u1, float u2, SWCSpectrum *const f,
		float *pdf, float *pdfBack = NULL, bool reverse = false) const;
	virtual float Weight(const TsPack *tspack, const Vector &wo) const;
	virtual float Pdf(const TsPack *tspack, const Vector &wo,
		const Vector &wi) const { return 0.f; }
protected:
	const Fresnel *fresnel;
};

class SimpleArchitecturalReflection : public SimpleSpecularReflection {
public:
	SimpleArchitecturalReflection(const Fresnel *fr) :
		SimpleSpecularReflection(fr) { }
	virtual ~SimpleArchitecturalReflection() { }
	virtual bool Sample_f(const TsPack *tspack, const Vector &wo,
		Vector *wi, float u1, float u2, SWCSpectrum *const f,
		float *pdf, float *pdfBack = NULL, bool reverse = false) const;
	virtual float Weight(const TsPack *tspack, const Vector &wo) const;
};

class  SpecularReflection : public SimpleSpecularReflection {
public:
	// SpecularReflection Public Methods
	SpecularReflection(const SWCSpectrum &r, const Fresnel *fr, float flm,
		float flmindex) : SimpleSpecularReflection(fr), R(r),
		film(flm), filmindex(flmindex) { }
	virtual ~SpecularReflection() { }
	virtual bool Sample_f(const TsPack *tspack, const Vector &wo, Vector *wi,
		float u1, float u2, SWCSpectrum *const f, float *pdf, float *pdfBack = NULL, bool reverse = false) const;
protected:
	// SpecularReflection Private Data
	SWCSpectrum R;
	float film, filmindex;
};

class ArchitecturalReflection : public SpecularReflection {
public:
	ArchitecturalReflection(const SWCSpectrum &r, const Fresnel *fr, float flm, float flmindex)
		: SpecularReflection(r, fr, flm, flmindex) {}
	virtual ~ArchitecturalReflection() { }
	virtual bool Sample_f(const TsPack *tspack, const Vector &wo, Vector *wi,
		float u1, float u2, SWCSpectrum *const f, float *pdf, float *pdfBack = NULL, bool reverse = false) const;
	virtual float Weight(const TsPack *tspack, const Vector &wo) const;
};

}//namespace lux

#endif // LUX_SPECULARREFLECTION_H

