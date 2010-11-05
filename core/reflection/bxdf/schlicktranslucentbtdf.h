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

#ifndef LUX_SCHLICKTRANSLUCENTBTDF_H
#define LUX_SCHLICKTRANSLUCENTBTDF_H

// schlicktranslucentbtdf.h*

#include "lux.h"
#include "bxdf.h"
#include "spectrum.h"

namespace lux
{

class  SchlickTranslucentBTDF : public BxDF
{
public:
	// SchlickBRDFTranslucent Public Methods
	SchlickTranslucentBTDF(const SWCSpectrum &Rd, const SWCSpectrum &Rt,
		const SWCSpectrum &Rs,const SWCSpectrum &Rs2,
		const SWCSpectrum &Alpha, const SWCSpectrum &Alpha2,
		float dep, float dep2);
	virtual ~SchlickTranslucentBTDF() { }
	virtual void f(const SpectrumWavelengths &sw, const Vector &wo,
		const Vector &wi, SWCSpectrum *const f) const;
	SWCSpectrum SchlickFresnel(float costheta, const SWCSpectrum &r) const {
		return r + powf(1.f - costheta, 5.f) * (SWCSpectrum(1.f) - r);
	}
	virtual bool Sample_f(const SpectrumWavelengths &sw, const Vector &wo,
		Vector *wi, float u1, float u2, SWCSpectrum *const f,
		float *pdf, float *pdfBack = NULL, bool reverse = false) const;
	virtual float Pdf(const SpectrumWavelengths &sw, const Vector &wi,
		const Vector &wo) const;
	
private:
	// SchlickTranslucentBTDF Private Data
	SWCSpectrum Rd, Rt, Rs, Rs_bf, Alpha, Alpha_bf;
	float depth, depth_bf;
};

}//namespace lux

#endif // LUX_SCHLICKTRANSLUCENTBTDF_H

