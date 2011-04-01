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
	char buf[256];
	MachineFloat mf;
	mf.f = DEFAULT_EPSILON_STATIC;
	sprintf(buf,"Epsilon.DefaultEpsilonStatic: %x", mf.i & 0x7fffff);
	luxError(LUX_NOERROR, LUX_DEBUG, buf);

	sprintf(buf,"Epsilon.DefaultEpsilonStaticBitAdvance(0.f): %e", MachineEpsilon::E(0.f));
	luxError(LUX_NOERROR, LUX_DEBUG, buf);
	sprintf(buf,"Epsilon.DefaultEpsilonStaticBitAdvance(1.f): %e", MachineEpsilon::E(1.f));
	luxError(LUX_NOERROR, LUX_DEBUG, buf);

	for (float v = 1e-5f; v < 1e5f; v *= 2.0f) {
		sprintf(buf,"Epsilon.Test: %f => %e", v, E(v));
		luxError(LUX_NOERROR, LUX_DEBUG, buf);
		sprintf(buf,"Epsilon.Test: %f => %e", -v, E(-v));
		luxError(LUX_NOERROR, LUX_DEBUG, buf);
	}
}
