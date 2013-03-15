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


#ifndef LUX_RAYDIFFERENTIAL_H
#define LUX_RAYDIFFERENTIAL_H

#include "luxrays/core/geometry/vector.h"
using luxrays::Vector;
#include "luxrays/core/geometry/point.h"
using luxrays::Point;
#include "luxrays/core/geometry/normal.h"
using luxrays::Normal;

namespace lux
{

class Primitive;

class PartialDifferentialGeometry
{
public:
	Point p;
	Normal nn;
	Vector dpdu, dpdv;
	float time;
	bool scattered;
	Point wuv;

	PartialDifferentialGeometry() { scattered = false; }

	PartialDifferentialGeometry(
			const Point &P,
			const Vector &DPDU,
			const Vector &DPDV);

	PartialDifferentialGeometry(
			const Point &P,
			const Vector &DPDU,
			const Vector &DPDV, const Point &WUV);

	PartialDifferentialGeometry(
		const Point &P,
		const Normal &NN,
		const Vector &DPDU,
		const Vector &DPDV);

	PartialDifferentialGeometry(
		const Point &P,
		const Normal &NN,
		const Vector &DPDU,
		const Vector &DPDV, const Point &WUV);
	/**
	 * Returns the volume defined by dpdu, dpdv and nn
	 * @return The volume defined by dpdu, dpdv and nn
	 */
	float Volume() const { return fabsf(Dot(Cross(dpdu, dpdv), Vector(nn))); }
};

// DifferentialGeometry Declarations
class DifferentialGeometry : public PartialDifferentialGeometry {
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

	DifferentialGeometry() { u = v = 0.; handle = ihandle = NULL; scattered = false; }
	// DifferentialGeometry Public Methods
	DifferentialGeometry(
			const Point &P,
			const Vector &DPDU,	const Vector &DPDV,
			const Normal &DNDU, const Normal &DNDV,
			float uu, float vv,
			const Primitive *pr);
	DifferentialGeometry(
			const Point &P,
			const Vector &DPDU,	const Vector &DPDV,
			const Normal &DNDU, const Normal &DNDV,
			float uu, float vv,
			const Primitive *pr, float scale, const Point &WUV);
	DifferentialGeometry(
			const Point &P, const Normal &NN,
			const Vector &DPDU,	const Vector &DPDV,
			const Normal &DNDU, const Normal &DNDV,
			float uu, float vv,
			const Primitive *pr);
	DifferentialGeometry(
			const Point &P, const Normal &NN,
			const Vector &DPDU,	const Vector &DPDV,
			const Normal &DNDU, const Normal &DNDV,
			float uu, float vv,
			const Primitive *pr, float scale, const Point &WUV);
	DifferentialGeometry(
			const Point &P, const Normal &NN,
			const Vector &DPDU,	const Vector &DPDV,
			const Normal &DNDU, const Normal &DNDV,
			const Vector &T, const Vector &BiT, float BiTsign,
			float uu, float vv,
			const Primitive *pr);
	DifferentialGeometry(
			const Point &P, const Normal &NN,
			const Vector &DPDU,	const Vector &DPDV,
			const Normal &DNDU, const Normal &DNDV,
			const Vector &T, const Vector &BiT, float BiTsign,
			float uu, float vv,
			const Primitive *pr, float scale, const Point &WUV);
	void AdjustNormal(bool ro, bool swapsHandedness) {
		// Adjust normal based on orientation and handedness
		if (ro ^ swapsHandedness)
			nn = -nn;
	}
	// DifferentialGeometry Public Data
	Normal dndu, dndv;
	Vector tangent, bitangent; // surface tangents, may be different to dpdu,dpdv but in same plane, not normalized
	float btsign; // sign of the bitangent, actual bitangent is "bitangent * (btsign > 0.f ? 1.f : -1.f)"
	float u, v;
	mutable float Scale;

	const Primitive *handle;
	const Primitive *ihandle; // handle to intersected primitive, used with instances

	// Dade - shape specific data, useful to "transport" informatin between
	// shape intersection method and GetShadingGeometry()
	IntersectionData iData;
};

}//namespace lux

#endif //LUX_RAYDIFFERENTIAL_H
