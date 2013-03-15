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

#ifndef LUX_NULLTRANSMISSION_H
#define LUX_NULLTRANSMISSION_H
// nulltransmission.h*
#include "lux.h"
#include "bxdf.h"
#include "spectrum.h"
#include "luxrays/core/epsilon.h"
using luxrays::MachineEpsilon;

namespace lux
{

class  NullTransmission : public BxDF {
public:
	// NullTransmission Public Methods
	NullTransmission()
		: BxDF(BxDFType(BSDF_TRANSMISSION | BSDF_SPECULAR)) {}
	virtual ~NullTransmission() { }
	virtual void F(const SpectrumWavelengths &sw, const Vector &wo,
		const Vector &wi, SWCSpectrum *const f_) const {
		if (Dot(wo, wi) <= -1.f + MachineEpsilon::E(1.f))
			*f_ += SWCSpectrum(1.f);
	}
	virtual bool SampleF(const SpectrumWavelengths &sw, const Vector &wo,
		Vector *wi, float u1, float u2, SWCSpectrum *const f,
		float *pdf, float *pdfBack = NULL, bool reverse = false) const;
	virtual float Pdf(const SpectrumWavelengths &sw, const Vector &wo,
		const Vector &wi) const {
		return Dot(wo, wi) <= -1.f + MachineEpsilon::E(1.f) ? 1.f : 0.f;
	}
private:
	// NullTransmission Private Data
};

class  FilteredTransmission : public BxDF {
public:
	// FilteredTransmission Public Methods
	FilteredTransmission(const SWCSpectrum &r)
		: BxDF(BxDFType(BSDF_TRANSMISSION | BSDF_SPECULAR)), R(r) {}
	virtual ~FilteredTransmission() { }
	virtual void F(const SpectrumWavelengths &sw, const Vector &wo,
		const Vector &wi, SWCSpectrum *const f_) const {
		if (Dot(wo, wi) <= -1.f + MachineEpsilon::E(1.f))
			*f_ += R;
	}
	virtual bool SampleF(const SpectrumWavelengths &sw, const Vector &wo,
		Vector *wi, float u1, float u2, SWCSpectrum *const f,
		float *pdf, float *pdfBack = NULL, bool reverse = false) const;
	virtual float Pdf(const SpectrumWavelengths &sw, const Vector &wo,
		const Vector &wi) const {
		return Dot(wo, wi) <= -1.f + MachineEpsilon::E(1.f) ? 1.f : 0.f;
	}
private:
	// FilteredTransmission Private Data
	SWCSpectrum R;
};

}//namespace lux

#endif // LUX_NULLTRANSMISSION_H

