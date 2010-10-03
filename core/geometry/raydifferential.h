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


#ifndef LUX_RAYDIFFERENTIAL_H
#define LUX_RAYDIFFERENTIAL_H

#include "vector.h"
#include "point.h"
#include "normal.h"

namespace lux
{

// DifferentialGeometry Declarations
class DifferentialGeometry {
public:
	typedef union {
		struct {
			float coords[3];
		} baryTriangle;
		struct {
			float coords[3];
			u_int triIndex;
		} mesh;
	} IntersectionData;

	DifferentialGeometry() { u = v = 0.; handle = NULL; }
	// DifferentialGeometry Public Methods
	DifferentialGeometry(
			const Point &P,
			const Vector &DPDU,	const Vector &DPDV,
			const Normal &DNDU, const Normal &DNDV,
			float uu, float vv,
			const void *pr);
	DifferentialGeometry(
			const Point &P, const Normal &NN,
			const Vector &DPDU,	const Vector &DPDV,
			const Normal &DNDU, const Normal &DNDV,
			float uu, float vv,
			const void *pr);
	void AdjustNormal(bool ro, bool swapsHandedness) {
		// Adjust normal based on orientation and handedness
		if (ro ^ swapsHandedness) {
			nn.x = -nn.x;
			nn.y = -nn.y;
			nn.z = -nn.z;
		}
	}
	// DifferentialGeometry Public Data
	Point p;
	Normal nn;
	Vector dpdu, dpdv;
	Normal dndu, dndv;
	float u, v;
	const void* handle;
	float time;

	// Dade - shape specific data, useful to "transport" informatin between
	// shape intersection method and GetShadingGeometry()
	IntersectionData iData;
};

}//namespace lux

#endif //LUX_RAYDIFFERENTIAL_H
