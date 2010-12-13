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
Material::Material(const ParamSet &mp) {
	bumpmapSampleDistance = mp.FindOneFloat("bumpmapsampledistance", .001f);
	compParams.tVm = mp.FindOneBool("compo_visible_material", true);
	compParams.tVl = mp.FindOneBool("compo_visible_emission", true);
	compParams.tiVm = mp.FindOneBool("compo_visible_indirect_material", true);
	compParams.tiVl = mp.FindOneBool("compo_visible_indirect_emission", true);
	compParams.oA = mp.FindOneBool("compo_override_alpha", false);
	compParams.A = mp.FindOneFloat("compo_override_alpha_value", 0.f);
}

void Material::Bump(const SpectrumWavelengths &sw,
	const boost::shared_ptr<Texture<float> > &d, const Normal &nGeom,
	DifferentialGeometry *dgBump) const
{
	float du, dv;
	d->GetDuv(sw, *dgBump, bumpmapSampleDistance, &du, &dv);
	dgBump->dpdu += du * Vector(dgBump->nn);   // different to book, as displace*dgs.dndu creates artefacts
	dgBump->dpdv += dv * Vector(dgBump->nn);   // different to book, as displace*dgs.dndv creates artefacts
	const Normal nn(dgBump->nn);
	dgBump->nn = Normal(Normalize(Cross(dgBump->dpdu, dgBump->dpdv)));
	// INFO: We don't compute dgBump->dndu and dgBump->dndv as we need this
	//       only here.
	// The above transform keeps the normal in the original normal
	// hemisphere. If they are opposed, it means UVN was indirect and
	// the normal needs to be reversed
	if (Dot(nn, dgBump->nn) < 0.f)
		dgBump->nn = -dgBump->nn;
}
