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
	SWCSpectrum f(const Vector &wo, const Vector &wi) const {
		return wo == -wi ? SWCSpectrum(1.f / fabsf(CosTheta(wi))) : SWCSpectrum(0.);
	}
	SWCSpectrum Sample_f(const Vector &wo, Vector *wi, float u1, float u2, float *pdf, float *pdfBack = NULL) const;
	float Pdf(const Vector &wo, const Vector &wi) const {
		return 0.;
	}
private:
	// NullTransmission Private Data
};

}//namespace lux

#endif // LUX_NULLTRANSMISSION_H

