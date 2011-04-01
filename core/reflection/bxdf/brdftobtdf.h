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

#ifndef LUX_BRDFTOBTDF_H
#define LUX_BRDFTOBTDF_H
// brdftobtdf.h*
#include "bxdf.h"

namespace lux
{

class  BRDFToBTDF : public BxDF {
public:
	// BRDFToBTDF Public Methods
	BRDFToBTDF(BxDF *b, float ei = 1.f, float et = 1.f, float c = 0.f)
		: BxDF(BxDFType(b->type ^
			(BSDF_REFLECTION | BSDF_TRANSMISSION))),
		etai(ei), etat(et), cb(c), brdf(b) {}
	virtual ~BRDFToBTDF() { }
	static Vector otherHemisphere(const Vector &w) {
		return Vector(w.x, w.y, -w.z);
	}
	virtual SWCSpectrum rho(const TsPack *tspack, const Vector &w, u_int nSamples,
			float *samples) const {
		return brdf->rho(tspack, otherHemisphere(w), nSamples, samples);
	}
	virtual SWCSpectrum rho(const TsPack *tspack, u_int nSamples, float *samples) const {
		return brdf->rho(tspack, nSamples, samples);
	}
	virtual void f(const TsPack *tspack, const Vector &wo, const Vector &wi, SWCSpectrum *const f) const;
	virtual bool Sample_f(const TsPack *tspack, const Vector &wo, Vector *wi,
		float u1, float u2, SWCSpectrum *const f, float *pdf, float *pdfBack = NULL,
		bool reverse = false) const;
	virtual float Weight(const TsPack *tspack, const Vector &wo) const {
		return brdf->Weight(tspack, wo);
	}
	virtual float Pdf(const TsPack *tspack, const Vector &wo, const Vector &wi) const;
private:
	float etai, etat, cb;
	BxDF *brdf;
};

}//namespace lux

#endif // LUX_BRDFTOBTDF_H
