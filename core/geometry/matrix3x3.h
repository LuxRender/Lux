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

#ifndef LUX_MATRIX3X3_H
#define LUX_MATRIX3X3_H

#include "geometry/vector.h"

namespace lux
{

static inline float Determinant3x3(const float matrix[3][3])
{
	float temp;
	temp  = matrix[0][0] * matrix[1][1] * matrix[2][2];
	temp -= matrix[0][0] * matrix[1][2] * matrix[2][1];
	temp -= matrix[1][0] * matrix[0][1] * matrix[2][2];
	temp += matrix[1][0] * matrix[0][2] * matrix[2][1];
	temp += matrix[2][0] * matrix[0][1] * matrix[1][2];
	temp -= matrix[2][0] * matrix[0][2] * matrix[1][1];
	return temp;
}

static inline bool Invert3x3(const float matrix[3][3], float result[3][3])
{
	const float det = Determinant3x3(matrix);
	if (fabsf(det) < 1e-9f)
		return false;
	const float a = matrix[0][0], b = matrix[0][1], c = matrix[0][2],
		d = matrix[1][0], e = matrix[1][1], f = matrix[1][2],
		g = matrix[2][0], h = matrix[2][1], i = matrix[2][2];

	const float idet = 1.f / det;

	result[0][0] = (e * i - f * h) * idet;
	result[0][1] = (c * h - b * i) * idet;
	result[0][2] = (b * f - c * e) * idet;
	result[1][0] = (f * g - d * i) * idet;
	result[1][1] = (a * i - c * g) * idet;
	result[1][2] = (c * d - a * f) * idet;
	result[2][0] = (d * h - e * g) * idet;
	result[2][1] = (b * g - a * h) * idet;
	result[2][2] = (a * e - b * d) * idet;

	return true;
}

static inline void Transform3x3(const float matrix[3][3], const float vector[3], float result[3])
{
	result[0] = matrix[0][0] * vector[0] + matrix[0][1] * vector[1] + matrix[0][2] * vector[2];
	result[1] = matrix[1][0] * vector[0] + matrix[1][1] * vector[1] + matrix[1][2] * vector[2];
	result[2] = matrix[2][0] * vector[0] + matrix[2][1] * vector[1] + matrix[2][2] * vector[2];
}

static inline Vector Transform3x3(const float matrix[3][3], const Vector &v)
{
	return Vector(
		matrix[0][0] * v.x + matrix[0][1] * v.y + matrix[0][2] * v.z,
		matrix[1][0] * v.x + matrix[1][1] * v.y + matrix[1][2] * v.z,
		matrix[2][0] * v.x + matrix[2][1] * v.y + matrix[2][2] * v.z);
}

static inline void Multiply3x3(const float a[3][3], const float b[3][3], float result[3][3])
{
	result[0][0] = a[0][0] * b[0][0] + a[0][1] * b[1][0] + a[0][2] * b[2][0];
	result[0][1] = a[0][0] * b[0][1] + a[0][1] * b[1][1] + a[0][2] * b[2][1];
	result[0][2] = a[0][0] * b[0][2] + a[0][1] * b[1][2] + a[0][2] * b[2][2];
	result[1][0] = a[1][0] * b[0][0] + a[1][1] * b[1][0] + a[1][2] * b[2][0];
	result[1][1] = a[1][0] * b[0][1] + a[1][1] * b[1][1] + a[1][2] * b[2][1];
	result[1][2] = a[1][0] * b[0][2] + a[1][1] * b[1][2] + a[1][2] * b[2][2];
	result[2][0] = a[2][0] * b[0][0] + a[2][1] * b[1][0] + a[2][2] * b[2][0];
	result[2][1] = a[2][0] * b[0][1] + a[2][1] * b[1][1] + a[2][2] * b[2][1];
	result[2][2] = a[2][0] * b[0][2] + a[2][1] * b[1][2] + a[2][2] * b[2][2];
}

static inline bool SolveLinearSystem3x3(const float A[3][3], const float B[3], float x[3])
{
	float invA[3][3];

	if (!Invert3x3(A, invA))
		return false;

	Transform3x3(invA, B, x);

	return true;
}

}//namespace lux

#endif //LUX_MATRIX3X3_H
