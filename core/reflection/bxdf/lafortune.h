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

#ifndef LUX_LAFORTUNE_H
#define LUX_LAFORTUNE_H
// lafortune.h*
#include "lux.h"
#include "bxdf.h"
#include "luxrays/core/color/swcspectrum.h"

namespace lux
{

class  Lafortune : public BxDF {
public:
	// Lafortune Public Methods
	Lafortune(const SWCSpectrum &x, const SWCSpectrum &y,
		const SWCSpectrum &z, const SWCSpectrum &e);
	virtual ~Lafortune() { }
	virtual void F(const SpectrumWavelengths &sw, const Vector &wo,
		const Vector &wi, SWCSpectrum *const f) const;
	virtual bool SampleF(const SpectrumWavelengths &sw, const Vector &wi,
		Vector *sampled_f, float u1, float u2, SWCSpectrum *const f,
		float *pdf, float *pdfBack = NULL, bool reverse = false) const;
	virtual float Pdf(const SpectrumWavelengths &sw, const Vector &wi,
		const Vector &wo) const;
private:
	// Lafortune Private Data
	const SWCSpectrum x, y, z, exponent;
};

}//namespace lux

#endif // LUX_LAFORTUNE_H

