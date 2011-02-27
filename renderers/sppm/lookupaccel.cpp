/***************************************************************************
 *   Copyright (C) 1998-2010 by authors (see AUTHORS.txt )                 *
 *                                                                         *
 *   This file is part of LuxRays.                                         *
 *                                                                         *
 *   LuxRays is free software; you can redistribute it and/or modify       *
 *   it under the terms of the GNU General Public License as published by  *
 *   the Free Software Foundation; either version 3 of the License, or     *
 *   (at your option) any later version.                                   *
 *                                                                         *
 *   LuxRays is distributed in the hope that it will be useful,            *
 *   but WITHOUT ANY WARRANTY; without even the implied warranty of        *
 *   MERCHANTABILITY or FITNESS FOR A PARTICULAR PURPOSE.  See the         *
 *   GNU General Public License for more details.                          *
 *                                                                         *
 *   You should have received a copy of the GNU General Public License     *
 *   along with this program.  If not, see <http://www.gnu.org/licenses/>. *
 *                                                                         *
 *   LuxRays website: http://www.luxrender.net                             *
 ***************************************************************************/

#include "hitpoints.h"
#include "lookupaccel.h"
#include "bxdf.h"
#include "reflection/bxdf.h"

using namespace lux;

void HitPointsLookUpAccel::AddFluxToHitPoint(HitPoint *hp,
	const Point &hitPoint, const Vector &wi,
	const SpectrumWavelengths &sw, const SWCSpectrum &photonFlux, u_int light_group) {
		const float dist2 = DistanceSquared(hp->position, hitPoint);
		if ((dist2 >  hp->accumPhotonRadius2))
			return;

		SWCSpectrum f = hp->bsdf->F(sw, hp->wo, wi, false, BxDFType(BSDF_ALL | BSDF_DIFFUSE));
		if (f.Black())
			return;

		XYZColor flux = XYZColor(hp->sample->swl, photonFlux * f * hp->eyeThroughput);
		luxrays::AtomicInc(&hp->lightGroupData[light_group].accumPhotonCount);
		XYZColorAtomicAdd(hp->lightGroupData[light_group].accumReflectedFlux, flux);
}
