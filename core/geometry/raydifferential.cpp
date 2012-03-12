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

#include "lux.h"
#include "raydifferential.h"

using namespace lux;

PartialDifferentialGeometry::PartialDifferentialGeometry(
		const Point &P,
		const Vector &DPDU,
		const Vector &DPDV)
	: p(P), dpdu(DPDU), dpdv(DPDV) {
	nn = Normal(Normalize(Cross(dpdu, dpdv)));
	scattered = false;
}

PartialDifferentialGeometry::PartialDifferentialGeometry(
		const Point &P,
		const Normal &NN,
		const Vector &DPDU,
		const Vector &DPDV)
	: p(P), nn(NN), dpdu(DPDU), dpdv(DPDV) {
	scattered = false;
}

DifferentialGeometry::DifferentialGeometry(const Point &P,
		const Vector &DPDU, const Vector &DPDV,
		const Normal &DNDU, const Normal &DNDV,
		float uu, float vv, const void *pr)
	: PartialDifferentialGeometry(P, DPDU, DPDV), dndu(DNDU), dndv(DNDV),
	  tangent(DPDU), bitangent(DPDV), btsign(1.f) {
	// Initialize _DifferentialGeometry_ from parameters
	u = uu;
	v = vv;
	handle = pr;
	ihandle = NULL;
}

DifferentialGeometry::DifferentialGeometry(const Point &P,
		const Vector &DPDU, const Vector &DPDV,
		const Normal &DNDU, const Normal &DNDV,
		float uu, float vv, const void *pr, float scale)
	: PartialDifferentialGeometry(P, DPDU, DPDV), dndu(DNDU), dndv(DNDV),
	  tangent(DPDU), bitangent(DPDV), btsign(1.f), Scale(scale) {
	// Initialize _DifferentialGeometry_ from parameters
	u = uu;
	v = vv;
	handle = pr;
	ihandle = NULL;
}
// Dade - added this costructor as a little optimization if the
// normalized normal is already available
DifferentialGeometry::DifferentialGeometry(const Point &P,
		const Normal &NN,
		const Vector &DPDU, const Vector &DPDV,
		const Normal &DNDU, const Normal &DNDV,
		float uu, float vv, const void *pr)
	: PartialDifferentialGeometry(P, NN, DPDU, DPDV), dndu(DNDU), dndv(DNDV),
	  tangent(DPDU), bitangent(DPDV), btsign(1.f) {
	// Initialize _DifferentialGeometry_ from parameters
	u = uu;
	v = vv;
	handle = pr;
	ihandle = NULL;
}
DifferentialGeometry::DifferentialGeometry(const Point &P,
		const Normal &NN,
		const Vector &DPDU, const Vector &DPDV,
		const Normal &DNDU, const Normal &DNDV,
		float uu, float vv, const void *pr, float scale)
	: PartialDifferentialGeometry(P, NN, DPDU, DPDV), dndu(DNDU), dndv(DNDV),
	  tangent(DPDU), bitangent(DPDV), btsign(1.f), Scale(scale) {
	// Initialize _DifferentialGeometry_ from parameters
	u = uu;
	v = vv;
	handle = pr;
	ihandle = NULL;
}
DifferentialGeometry::DifferentialGeometry(const Point &P,
		const Normal &NN,
		const Vector &DPDU, const Vector &DPDV,
		const Normal &DNDU, const Normal &DNDV,
		const Vector &T, const Vector &BiT, float BiTsign,
		float uu, float vv, const void *pr)
	: PartialDifferentialGeometry(P, NN, DPDU, DPDV), dndu(DNDU), dndv(DNDV),
	  tangent(T), bitangent(BiT), btsign(BiTsign) {
	// Initialize _DifferentialGeometry_ from parameters
	u = uu;
	v = vv;
	handle = pr;
	ihandle = NULL;
}
DifferentialGeometry::DifferentialGeometry(const Point &P,
		const Normal &NN,
		const Vector &DPDU, const Vector &DPDV,
		const Normal &DNDU, const Normal &DNDV,
		const Vector &T, const Vector &BiT, float BiTsign,
		float uu, float vv, const void *pr, float scale)
	: PartialDifferentialGeometry(P, NN, DPDU, DPDV), dndu(DNDU), dndv(DNDV),
	  tangent(T), bitangent(BiT), btsign(BiTsign), Scale(scale) {
	// Initialize _DifferentialGeometry_ from parameters
	u = uu;
	v = vv;
	handle = pr;
	ihandle = NULL;
}
