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


#ifndef _EPSILON_H
#define	_EPSILON_H

#include "lux.h"
#include "error.h"
#include "geometry/vector.h"
#include "geometry/point.h"
#include "geometry/bbox.h"

#include <sstream>
//#include <boost/math/special_functions/trunc.hpp>
//#include <boost/math/special_functions/next.hpp>

//#define MACHINE_EPSILON_DEBUG 1

#if defined(MACHINE_EPSILON_DEBUG)
#define DEBUG(TYPE, VALUE) DebugPrint(TYPE, VALUE)
#else
#define DEBUG(TYPE, VALUE)
#endif

namespace lux {

// This class assume IEEE 754-2008 binary32 format for floating point numbers
// check http://en.wikipedia.org/wiki/Single_precision_floating-point_format for
// reference. Most method of this class should be thread-safe.

#define DEFAULT_EPSILON_MIN 1e-9f
#define DEFAULT_EPSILON_MAX 1e-1f
#define DEFAULT_EPSILON_STATIC 1e-5f

class MachineEpsilon {
public:
	MachineEpsilon() { MachineEpsilon(DEFAULT_EPSILON_MIN, DEFAULT_EPSILON_MAX); }
	MachineEpsilon(const float minValue, const float maxValue);

	// Not thread-safe method
	static void SetMin(const float min) { minEpsilon = min; UpdateAvarageEpsilon(); }
	// Not thread-safe method
	static void SetMax(const float max) { maxEpsilon = max; UpdateAvarageEpsilon(); }

	// Thread-safe method
	static float E(const float value) {
		DEBUG("E(float).value", value);
		const float epsilon = addE(value) - value;
		DEBUG("E(float).epsilon", epsilon);

		return Clamp(epsilon, minEpsilon, maxEpsilon);
	}

	static float addE(const float value) {
		DEBUG("addE(float).value", value);
		const float valuePlusEpsilon = FloatAdvance(value);
		DEBUG("addE(float).epsilon", valuePlusEpsilon);

		return valuePlusEpsilon;
	}

	static float E(const Vector &v) {
		return max(E(v.x), max(E(v.y), E(v.z)));
	}

	static float E(const Point &p) {
		return max(E(p.x), max(E(p.y), E(p.z)));
	}

	static float E(const BBox &bb) {
		return max(E(bb.pMin), E(bb.pMax));
	}

private:
	union MachineFloat {
		float f;
		u_int i;
	};

	static float minEpsilon;
	static float maxEpsilon;
	static float averageEpsilon;

	static float FloatAdvance(const float value) {
		// TODO - optimize this method
		int exp;
		float s = frexpf(value, &exp);
		s += averageEpsilon;
		if (s >= 1.f) s = 1.f;

		return ldexpf(s, exp);
	}

	static void UpdateAvarageEpsilon() {
		const int minExp = FloatExponent(minEpsilon);
		DEBUG("minExp", minExp);

		const int maxExp = FloatExponent(maxEpsilon);
		DEBUG("maxExp", maxExp);

		const int avgExp = (minExp + maxExp) / 2;
		averageEpsilon = Exp2Float(avgExp);
		DEBUG("avarageEpsilon", averageEpsilon);
	}

	static float Exp2Float(const int exp) {
		return ldexpf(1.f, exp);
	}

	static int FloatSign(const float value) {
		return (value >= 0.f) ? 1 : -1;
	}

	static int FloatExponent(const float value) {
		int exp;
		frexpf(value, &exp);

		return exp;
	}

	static float FloatSignificandPrecision(const float value) {
		int exp;
		const float s = frexpf(value, &exp);

		// s is between 0.5 and 1.0: [0.5, 1.0)
		return s;
	}

#if defined(MACHINE_EPSILON_DEBUG)
	static Test() const;

	static void DebugPrint(const string type, const float e) {
		MachineFloat mf;
		mf.f = e;

		char buf[256];
		sprintf(buf,"Epsilon.DebugPrint: %s=%e %x [%d * 2^%d * %f]",
			type.c_str(), mf.f, mf.i,
			FloatSign(e), FloatExponent(e), FloatSignificandPrecision(e));
		luxError(LUX_NOERROR, LUX_DEBUG, buf);
	}

	static void DebugPrint(const string type, const int i) {
		char buf[256];
		sprintf(buf,"Epsilon.DebugPrint: %s=%d", type.c_str(), i);
		luxError(LUX_NOERROR, LUX_DEBUG, buf);
	}
#endif
};

}

#endif	/* _EPSILON_H */

