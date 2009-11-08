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

MachineEpsilon::MachineEpsilon(const float minValue, const float maxValue)
{
	minEpsilon = minValue;
	DEBUG("minEpsilon", minEpsilon);

	maxEpsilon = maxValue;
	DEBUG("maxEpsilon", maxEpsilon);

	UpdateAvarageEpsilon();

#if defined(MACHINE_EPSILON_DEBUG)
	char buf[256];
	for (float v = 1e-5f; v < 1e5f; v *= 2.0f) {
		sprintf(buf,"Epsilon.Test: %f => %e", v, this->E(v));
		luxError(LUX_NOERROR, LUX_DEBUG, buf);
	}
#endif
}
