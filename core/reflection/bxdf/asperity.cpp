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

// asperity.cpp*
#include "asperity.h"
#include "spectrum.h"
#include "mc.h"

// Asperity scattering BRDF as described in "The Secret of Velvety Skin. J. Koendering and S. Pont, 2002"
// Phase function limited to four coefficients of Legendre polynomials (first assumed to be 1.0 by the paper)

using namespace lux;

void Asperity::f(const TsPack *tspack, const Vector &wo, 
	const Vector &wi, SWCSpectrum *const f_) const
{
	float costheta = Dot(-wo, wi);

	// Compute phase function

	float B = 3.0f * costheta;

	float p = 1.0f + A1 * costheta + A2 * 0.5f * (B * costheta - 1.0f) + A3 * 0.5 * (5.0f * costheta * costheta * costheta - B);
	p = p / (4.0f * M_PI);
 
	p = (p * delta) / (CosTheta(wo) * CosTheta(wi));

	// Clamp the BRDF (page 7)
	if (p > 1.0f)
		p = 1.0f;
	else if (p < 0.0f)
		p = 0.0f;
	
	f_->AddWeighted(p, R);
}

