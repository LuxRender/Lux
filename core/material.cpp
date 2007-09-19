/***************************************************************************
 *   Copyright (C) 1998-2007 by authors (see AUTHORS.txt )                 *
 *                                                                         *
 *   This file is part of Lux Renderer.                                    *
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
 *   Lux Renderer website : http://www.luxrender.org                       *
 ***************************************************************************/

// material.cpp*
#include "material.h"
// Material Method Definitions
Material::~Material() {
}
void Material::Bump(Reference<Texture<float> > d,
		const DifferentialGeometry &dgGeom,
		const DifferentialGeometry &dgs,
		DifferentialGeometry *dgBump) {
	// Compute offset positions and evaluate displacement texture
	DifferentialGeometry dgEval = dgs;
	// Shift _dgEval_ _du_ in the $u$ direction
	float du = .5f * (fabsf(dgs.dudx) + fabsf(dgs.dudy));
	if (du == 0.f) du = .01f;
	dgEval.p = dgs.p + du * dgs.dpdu;
	dgEval.u = dgs.u + du;
	dgEval.nn =
		Normalize((Normal)Cross(dgs.dpdu, dgs.dpdv) +
		                 du * dgs.dndu);
	float uDisplace = d->Evaluate(dgEval);
	// Shift _dgEval_ _dv_ in the $v$ direction
	float dv = .5f * (fabsf(dgs.dvdx) + fabsf(dgs.dvdy));
	if (dv == 0.f) dv = .01f;
	dgEval.p = dgs.p + dv * dgs.dpdv;
	dgEval.u = dgs.u;
	dgEval.v = dgs.v + dv;
	dgEval.nn =
		Normalize((Normal)Cross(dgs.dpdu, dgs.dpdv) +
		                 dv * dgs.dndv);
	float vDisplace = d->Evaluate(dgEval);
	float displace = d->Evaluate(dgs);
	// Compute bump-mapped differential geometry
	*dgBump = dgs;
	dgBump->dpdu = dgs.dpdu +
		(uDisplace - displace) / du * Vector(dgs.nn) +
		displace * Vector(dgs.dndu);
	dgBump->dpdv = dgs.dpdv +
		(vDisplace - displace) / dv * Vector(dgs.nn) +
		displace * Vector(dgs.dndv);
	dgBump->nn =
		Normal(Normalize(Cross(dgBump->dpdu, dgBump->dpdv)));
	if (dgs.shape->reverseOrientation ^
		dgs.shape->transformSwapsHandedness)
		dgBump->nn *= -1.f;
	// Orient shading normal to match geometric normal
	if (Dot(dgGeom.nn, dgBump->nn) < 0.f)
		dgBump->nn *= -1.f;
}
