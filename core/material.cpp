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

void Material::InitGeneralParams(const ParamSet &mp) {
	bumpmapSampleDistance = mp.FindOneFloat("bumpmapsampledistance", .001f);
}

void Material::FindCompositingParams(const ParamSet &mp, CompositingParams *cp)
{
	cp->tVm = mp.FindOneBool("compo_visible_material", true);
	cp->tVl = mp.FindOneBool("compo_visible_emission", true);
	cp->tiVm = mp.FindOneBool("compo_visible_indirect_material", true);
	cp->tiVl = mp.FindOneBool("compo_visible_indirect_emission", true);
	cp->oA = mp.FindOneBool("compo_override_alpha", false);
	cp->A = mp.FindOneFloat("compo_override_alpha_value", 0.f);
}

void Material::Bump(const boost::shared_ptr<Texture<float> > &d,
		const DifferentialGeometry &dgGeom,
		DifferentialGeometry *dgBump) const {
	// Calculate bump map value at intersection point
	float displace = d->Evaluate(NULL, *dgBump);

	// Compute offset positions and evaluate displacement texture
	const Point origP(dgBump->p);
	const Normal origN(dgBump->nn);
	const float origU = dgBump->u, origV = dgBump->v;

	// Shift _dgEval_ _du_ in the $u$ direction and calculate bump map value
	float du = .5f * (fabsf(dgBump->dudx) + fabsf(dgBump->dudy));
	if (du == 0.f)
		du = bumpmapSampleDistance;
	dgBump->p += du * dgBump->dpdu;
	dgBump->u += du;
	dgBump->nn = Normalize(origN + du * dgBump->dndu);
	float uDisplace = d->Evaluate(NULL, *dgBump);

	// Shift _dgEval_ _dv_ in the $v$ direction and calculate bump map value
	float dv = .5f * (fabsf(dgBump->dvdx) + fabsf(dgBump->dvdy));
	if (dv == 0.f)
		dv = bumpmapSampleDistance;
	dgBump->p = origP + dv * dgBump->dpdv;
	dgBump->u = origU;
	dgBump->v += dv;
	dgBump->nn = Normalize(origN + dv * dgBump->dndv);
	float vDisplace = d->Evaluate(NULL, *dgBump);

	// Compute bump-mapped differential geometry
	dgBump->p = origP;
	dgBump->v = origV;
	dgBump->dpdu += (uDisplace - displace) / du * Vector(origN);   // different to book, as displace*dgs.dndu creates artefacts
	dgBump->dpdv += (vDisplace - displace) / dv * Vector(origN);   // different to book, as displace*dgs.dndv creates artefacts
	dgBump->nn = Normal(Normalize(Cross(dgBump->dpdu, dgBump->dpdv)));
	// INFO: We don't compute dgBump->dndu and dgBump->dndv as we need this
	//       only here.

	// Orient shading normal to match geometric normal
	if (Dot(dgGeom.nn, dgBump->nn) < 0.f)
		dgBump->nn *= -1.f;
}
