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

#ifndef LUX_FRESNELBLEND_H
#define LUX_FRESNELBLEND_H
// fresnelblend.h*
#include "lux.h"
#include "bxdf.h"
#include "spectrum.h"

namespace lux
{

class  FresnelBlend : public BxDF
{
public:
	// FresnelBlend Public Methods
	FresnelBlend(const SWCSpectrum &Rd, const SWCSpectrum &Rs,
		const SWCSpectrum &Alpha, float d,
		MicrofacetDistribution *dist);
	virtual ~FresnelBlend() { }
	virtual void f(const SpectrumWavelengths &sw, const Vector &wo,
		const Vector &wi, SWCSpectrum *const f) const;
	SWCSpectrum SchlickFresnel(float costheta) const {
		return Rs + powf(1.f - costheta, 5.f) * (SWCSpectrum(1.f) - Rs);
	}
	virtual bool Sample_f(const SpectrumWavelengths &sw, const Vector &wo,
		Vector *wi, float u1, float u2, SWCSpectrum *const f,
		float *pdf, float *pdfBack = NULL, bool reverse = false) const;
	virtual float Pdf(const SpectrumWavelengths &sw, const Vector &wi,
		const Vector &wo) const;

private:
	// FresnelBlend Private Data
	SWCSpectrum Rd, Rs, Alpha;
	float depth;
	MicrofacetDistribution *distribution;
};

}//namespace lux

#endif // LUX_FRESNELBLEND_H

