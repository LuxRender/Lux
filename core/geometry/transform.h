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

#ifndef LUX_TRANSFORM_H
#define LUX_TRANSFORM_H
// transform.h*
#include "luxrays/core/geometry/transform.h"
#include "raydifferential.h"

namespace lux
{

	using luxrays::Transform;
	using luxrays::InvTransform;
	using luxrays::LookAt;
	using luxrays::Orthographic;
	using luxrays::Perspective;
	using luxrays::Rotate;
	using luxrays::RotateX;
	using luxrays::RotateY;
	using luxrays::RotateZ;
	using luxrays::Scale;
	using luxrays::Translate;

inline DifferentialGeometry operator*(const Transform &t, const DifferentialGeometry &dg)
{
	DifferentialGeometry dgt(t * dg.p, Normalize(t * dg.nn),
		t * dg.dpdu, t * dg.dpdv, t * dg.dndu, t * dg.dndv,
		t * dg.tangent, t * dg.bitangent, dg.btsign, dg.u, dg.v,
		dg.handle);
	dgt.ihandle = dg.ihandle;
	dgt.time = dg.time;
	dgt.scattered = dg.scattered;
	dgt.iData = dg.iData;
	return dgt;
}

inline DifferentialGeometry &operator*=(DifferentialGeometry &dg, const Transform &t)
{
	dg.p *= t;
	dg.nn = Normalize(t * dg.nn);
	dg.dpdu *= t;
	dg.dpdv *= t;
	dg.dndu *= t;
	dg.dndv *= t;
	dg.tangent *= t;
	dg.bitangent *= t;
	return dg;
}

inline DifferentialGeometry operator*(const InvTransform &t, const DifferentialGeometry &dg)
{
	DifferentialGeometry dgt(t * dg.p, Normalize(t * dg.nn),
		t * dg.dpdu, t * dg.dpdv, t * dg.dndu, t * dg.dndv,
		t * dg.tangent, t * dg.bitangent, dg.btsign, dg.u, dg.v,
		dg.handle);
	dgt.ihandle = dg.ihandle;
	dgt.time = dg.time;
	dgt.scattered = dg.scattered;
	dgt.iData = dg.iData;
	return dgt;
}

inline PartialDifferentialGeometry operator*(const Transform &t, const PartialDifferentialGeometry &dg)
{
	PartialDifferentialGeometry dgt(t * dg.p, Normalize(t * dg.nn),
		t * dg.dpdu, t * dg.dpdv);
	dgt.time = dg.time;
	dgt.scattered = dg.scattered;
	return dgt;
}

inline PartialDifferentialGeometry &operator*=(PartialDifferentialGeometry &dg, const Transform &t)
{
	dg.p *= t;
	dg.nn = Normalize(t * dg.nn);
	dg.dpdu *= t;
	dg.dpdv *= t;
	return dg;
}

inline PartialDifferentialGeometry operator*(const InvTransform &t, const PartialDifferentialGeometry &dg)
{
	PartialDifferentialGeometry dgt(t * dg.p, Normalize(t * dg.nn),
		t * dg.dpdu, t * dg.dpdv);
	dgt.time = dg.time;
	dgt.scattered = dg.scattered;
	return dgt;
}

}//namespace lux


#endif // LUX_TRANSFORM_H
