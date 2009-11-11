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

// fresnelgeneral.cpp*
#include "fresnelgeneral.h"
#include "spectrum.h"

using namespace lux;

void FresnelGeneral::Evaluate(const TsPack *tspack, float cosi, SWCSpectrum *const f) const {
	SWCSpectrum cost(sqrtf(max(0.f, 1.f - cosi * cosi)));
	if (cosi > 0.f)
		cost = cost / eta;
	else
		cost = cost * eta;
	cost.Clamp(0.f, 1.f);
	cost = (SWCSpectrum(1.f) - cost * cost).Sqrt();
	FrFull(fabsf(cosi), cost, eta, k, f);
}
