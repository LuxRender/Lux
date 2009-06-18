/***************************************************************************
 *   Copyright (C) 1998-2008 by authors (see AUTHORS.txt )                 *
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

// material.cpp*
#include "material.h"
#include "shape.h"
#include "texture.h"
#include "paramset.h"

using namespace lux;

// Material Method Definitions
Material::Material() : bumpmapSampleDistance(.001f) {
	compParams = NULL;
}

Material::~Material() {
	if(compParams) delete compParams; 
}

void Material::InitGeneralParams(const TextureParams &mp) {
	bumpmapSampleDistance = mp.FindFloat("bumpmapsampledistance", .001f);
}

void Material::Bump(boost::shared_ptr<Texture<float> > d,
		const DifferentialGeometry &dgGeom,
		const DifferentialGeometry &dgs,
		DifferentialGeometry *dgBump) const {
	// Compute offset positions and evaluate displacement texture
	DifferentialGeometry dgEval = dgs;

	// Shift _dgEval_ _du_ in the $u$ direction and calculate bump map value
	float du = .5f * (fabsf(dgs.dudx) + fabsf(dgs.dudy));
	if (du == 0.f) du = bumpmapSampleDistance;
	dgEval.p += du * dgs.dpdu;
	dgEval.u += du;
	dgEval.nn = Normalize(dgs.nn + du * dgs.dndu);
	float uDisplace = d->Evaluate(NULL, dgEval);

	// Shift _dgEval_ _dv_ in the $v$ direction and calculate bump map value
	float dv = .5f * (fabsf(dgs.dvdx) + fabsf(dgs.dvdy));
	if (dv == 0.f) dv = bumpmapSampleDistance;
	dgEval.p = dgs.p + dv * dgs.dpdv;
	dgEval.u = dgs.u;
	dgEval.v += dv;
	dgEval.nn = Normalize(dgs.nn + dv * dgs.dndv);
	float vDisplace = d->Evaluate(NULL, dgEval);

	// Calculate bump map value at intersection point
	float displace = d->Evaluate(NULL, dgs);

	// Compute bump-mapped differential geometry
	*dgBump = dgs;
	dgBump->dpdu += (uDisplace - displace) / du * Vector(dgs.nn);   // different to book, as displace*dgs.dndu creates artefacts
	dgBump->dpdv += (vDisplace - displace) / dv * Vector(dgs.nn);   // different to book, as displace*dgs.dndv creates artefacts
	dgBump->nn = Normal(Normalize(Cross(dgBump->dpdu, dgBump->dpdv)));
	// INFO: We don't compute dgBump->dndu and dgBump->dndv as we need this
	//       only here.

	// Orient shading normal to match geometric normal
	if (Dot(dgGeom.nn, dgBump->nn) < 0.f)
		dgBump->nn *= -1.f;
}
