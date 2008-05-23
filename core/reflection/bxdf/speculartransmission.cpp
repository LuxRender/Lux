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

// speculartransmission.cpp*
#include "speculartransmission.h"
#include "color.h"
#include "spectrum.h"
#include "spectrumwavelengths.h"
#include "mc.h"
#include "sampling.h"
#include <stdarg.h>

#include <boost/thread/tss.hpp>

using namespace lux;

extern boost::thread_specific_ptr<SpectrumWavelengths> thread_wavelengths;

SWCSpectrum SpecularTransmission::Sample_f(const Vector &wo,
	Vector *wi, float u1, float u2, float *pdf, float *pdfBack) const {
	// Figure out which $\eta$ is incident and which is transmitted
	bool entering = CosTheta(wo) > 0.;
	float ei = etai, et = etat;

	if(cb != 0.) {
		// Handle dispersion using cauchy formula
		float w = thread_wavelengths->SampleSingle();
		et = etat + (cb * 1000000) / (w*w);
	}

	if (!entering)
		swap(ei, et);
	// Compute transmitted ray direction
	float sini2 = SinTheta2(wo);
	float eta = ei / et;
	float sint2 = eta * eta * sini2;
	// Handle total internal reflection for transmission
	if (sint2 > 1.) {
		*pdf = 0.f;
		if (pdfBack)
			*pdfBack = 0.f;
		return 0.;
	}
	float cost = sqrtf(max(0.f, 1.f - sint2));
	if (entering) cost = -cost;
	float sintOverSini = eta;
	*wi = Vector(sintOverSini * -wo.x,
	             sintOverSini * -wo.y,
				 cost);
	*pdf = 1.f;
	if (pdfBack)
		*pdfBack = 1.f;
	SWCSpectrum F = fresnel.Evaluate(CosTheta(wo));
	return (et*et)/(ei*ei) * (SWCSpectrum(1.)-F) * T /
		fabsf(CosTheta(*wi));
}

