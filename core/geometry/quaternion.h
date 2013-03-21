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

#ifndef LUX_QUATERNION_H
#define LUX_QUATERNION_H
// quaternion.h*
#include "lux.h"
#include "luxrays/core/geometry/vector.h"
using luxrays::Vector;

namespace lux
{

class Quaternion {

public:
	float w;
	Vector v;

	// generate quaternion from 4x4 matrix
	Quaternion(const Matrix4x4 &m);
	Quaternion() : w(1.f), v(0.f) { }
	Quaternion(const Quaternion &q) : w(q.w), v(q.v) { }
	Quaternion(float _w, const Vector &_v) : w(_w), v(_v) { }

	Quaternion Invert() const {
		return Quaternion(w, -v);
	}

	Vector RotateVector(const Vector &v) const;

	// get the rotation matrix from quaternion
	void ToMatrix(float m[4][4]) const;
};

inline Quaternion operator +(const Quaternion& q1, const Quaternion& q2) {
	return Quaternion(q1.w + q2.w, q1.v + q2.v);
}

inline Quaternion operator -(const Quaternion& q1, const Quaternion& q2) {
	return Quaternion(q1.w - q2.w, q1.v - q2.v);
}

inline Quaternion operator *(float f, const Quaternion& q) {
	return Quaternion(q.w * f, q.v * f);
}

inline Quaternion operator *( const Quaternion& q1, const Quaternion& q2 ) {
	return Quaternion(
		q1.w*q2.w - Dot(q1.v, q2.v),
		q1.w*q2.v + q2.w*q1.v + Cross(q1.v, q2.v));
}

inline float Dot(const Quaternion &q1, const Quaternion &q2) {
	return q1.w * q2.w + Dot(q1.v, q2.v);
}

inline Quaternion Normalize(const Quaternion &q) {
	return (1.f / sqrtf(Dot(q, q))) * q;
}

Quaternion Slerp(float t, const Quaternion &q1, const Quaternion &q2);
Quaternion GetRotationBetween(const Vector &u, const Vector &v);

}//namespace lux


#endif // LUX_QUATERNION_H
