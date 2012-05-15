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

#include "epsilon.h"
#include "error.h"

using namespace lux;

float MachineEpsilon::minEpsilon = DEFAULT_EPSILON_MIN;
float MachineEpsilon::maxEpsilon = DEFAULT_EPSILON_MAX;

void MachineEpsilon::SetMin(const float min) {
	minEpsilon = min;
}

void MachineEpsilon::SetMax(const float max) {
	maxEpsilon = max;
}

void MachineEpsilon::Test() {
	MachineFloat mf;
	mf.f = DEFAULT_EPSILON_STATIC;
	LOG(LUX_DEBUG, LUX_NOERROR) << "Epsilon.DefaultEpsilonStatic: " << (mf.i & 0x7fffff);

	LOG(LUX_DEBUG, LUX_NOERROR) << "Epsilon.DefaultEpsilonStaticBitAdvance(0.f): " << MachineEpsilon::E(0.f);
	LOG(LUX_DEBUG, LUX_NOERROR) << "Epsilon.DefaultEpsilonStaticBitAdvance(1.f): " << MachineEpsilon::E(1.f);

	for (float v = 1e-5f; v < 1e5f; v *= 2.0f) {
		LOG(LUX_DEBUG, LUX_NOERROR) << "Epsilon.Test: " << v << " => " << E(v);
		LOG(LUX_DEBUG, LUX_NOERROR) << "Epsilon.Test: " << -v << " => " << E(-v);
	}
}
