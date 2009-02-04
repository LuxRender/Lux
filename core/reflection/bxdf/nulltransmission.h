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

#ifndef LUX_NULLTRANSMISSION_H
#define LUX_NULLTRANSMISSION_H
// nulltransmission.h*
#include "lux.h"
#include "bxdf.h"
#include "spectrum.h"

namespace lux
{

class  NullTransmission : public BxDF {
public:
	// NullTransmission Public Methods
	NullTransmission()
		: BxDF(BxDFType(BSDF_TRANSMISSION | BSDF_SPECULAR)) {}
	void f(const TsPack *tspack, const Vector &wo, const Vector &wi, SWCSpectrum *const f) const {
		if (fabsf(1.f + Dot(wo,wi)) < SHADOW_RAY_EPSILON)
			*f += SWCSpectrum(1.f / fabsf(CosTheta(wi)));
	}
	bool Sample_f(const TsPack *tspack, const Vector &wo, Vector *wi,
		float u1, float u2, SWCSpectrum *const f, float *pdf, float *pdfBack = NULL,
		bool reverse = false) const;
	float Pdf(const TsPack *tspack, const Vector &wo, const Vector &wi) const {
		return fabsf(1.f + Dot(wo,wi)) < SHADOW_RAY_EPSILON ? 1.f : 0.f;
	}
private:
	// NullTransmission Private Data
};

}//namespace lux

#endif // LUX_NULLTRANSMISSION_H

