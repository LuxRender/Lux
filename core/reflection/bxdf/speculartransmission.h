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

#ifndef LUX_SPECULARTRANSMISSION_H
#define LUX_SPECULARTRANSMISSION_H
// speculartransmission.h*
#include "lux.h"
#include "bxdf.h"
#include "color.h"
#include "spectrum.h"
#include "fresneldielectric.h"

namespace lux
{

class  SpecularTransmission : public BxDF {
public:
	// SpecularTransmission Public Methods
	SpecularTransmission(const SWCSpectrum &t, float ei, float et, float cbf, bool archi = false)
		: BxDF(BxDFType(BSDF_TRANSMISSION | BSDF_SPECULAR)),
		  T(t), etai(ei), etat(et), cb(cbf), architectural(archi),
		  fresnel(ei, et) {}
	void f(const TsPack *tspack, const Vector &wo, const Vector &wi, SWCSpectrum *const f) const;
	virtual bool Sample_f(const TsPack *tspack, const Vector &wo, Vector *wi,
		float u1, float u2, SWCSpectrum *const f, float *pdf, float *pdfBack = NULL, bool reverse = false) const;
	float Weight(const TsPack *tspack, const Vector &wo, bool reverse) const;
	float Pdf(const TsPack *tspack, const Vector &wo, const Vector &wi) const {
		if (architectural && wi == -wo) return 1.f;
		else return 0.f;
	}
private:
	// SpecularTransmission Private Data
	SWCSpectrum T;
	float etai, etat, cb;
	bool architectural;
	FresnelDielectric fresnel;
};

}//namespace lux

#endif // LUX_SPECULARTRANSMISSION_H

