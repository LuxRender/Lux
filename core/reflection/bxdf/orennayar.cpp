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

// orennayar.cpp*
#include "orennayar.h"
#include "color.h"
#include "spectrum.h"
#include "mc.h"
#include "sampling.h"
#include <stdarg.h>

using namespace lux;

SWCSpectrum OrenNayar::f(const TsPack *tspack, const Vector &wo,
		const Vector &wi) const {
	float sinthetai = SinTheta(wi);
	float sinthetao = SinTheta(wo);
	// Compute cosine term of Oren--Nayar model
	float maxcos = 0.f;
	if (sinthetai > 1e-4 && sinthetao > 1e-4) {
		float sinphii = SinPhi(wi), cosphii = CosPhi(wi);
		float sinphio = SinPhi(wo), cosphio = CosPhi(wo);
		float dcos = cosphii * cosphio + sinphii * sinphio;
		maxcos = max(0.f, dcos);
	}
	// Compute sine and tangent terms of Oren--Nayar model
	float sinalpha, tanbeta;
	if (fabsf(CosTheta(wi)) > fabsf(CosTheta(wo))) {
		sinalpha = sinthetao;
		tanbeta = sinthetai / fabsf(CosTheta(wi));
	}
	else {
		sinalpha = sinthetai;
		tanbeta = sinthetao / fabsf(CosTheta(wo));
	}
	return R * INV_PI *
	       (A + B * maxcos * sinalpha * tanbeta);
}

