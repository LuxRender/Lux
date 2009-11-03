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

// marble.cpp*
#include "lux.h"
#include "texture.h"
#include "paramset.h"

// TODO - radiance - add methods for Power and Illuminant propagation

namespace lux
{

// MarbleTexture Declarations
class MarbleTexture : public Texture<SWCSpectrum> {
public:
	// MarbleTexture Public Methods
	virtual ~MarbleTexture() {
		delete mapping;
	}
	MarbleTexture(int oct, float roughness, float sc, float var,
			TextureMapping3D *map) {
		omega = roughness;
		octaves = oct;
		mapping = map;
		scale = sc;
		variation = var;
	}
	virtual SWCSpectrum Evaluate(const TsPack *tspack, const DifferentialGeometry &dg) const {
		Vector dpdx, dpdy;
		Point P = mapping->Map(dg, &dpdx, &dpdy);
		P *= scale;
		float marble = P.y + variation * FBm(P, scale * dpdx,
			scale * dpdy, omega, octaves);
		float t = .5f + .5f * sinf(marble);
		// Evaluate marble spline at _t_
		static float c[][3] = { { .58f, .58f, .6f }, { .58f, .58f, .6f }, { .58f, .58f, .6f },
			{ .5f, .5f, .5f }, { .6f, .59f, .58f }, { .58f, .58f, .6f },
			{ .58f, .58f, .6f }, {.2f, .2f, .33f }, { .58f, .58f, .6f }, };
		#define NC  sizeof(c) / sizeof(c[0])
		#define NSEG (NC-3)
		int first = Floor2Int(t * NSEG);
		t = (t * NSEG - first);
		RGBColor c0(c[first]), c1(c[first+1]), c2(c[first+2]), c3(c[first+3]);
		// Bezier spline evaluated with de Castilejau's algorithm
		RGBColor s0 = (1.f - t) * c0 + t * c1;
		RGBColor s1 = (1.f - t) * c1 + t * c2;
		RGBColor s2 = (1.f - t) * c2 + t * c3;
		s0 = (1.f - t) * s0 + t * s1;
		s1 = (1.f - t) * s1 + t * s2;
		// Extra scale of 1.5 to increase variation among colors
		return SWCSpectrum(tspack, 1.5f * ((1.f - t) * s0 + t * s1));
	}
	virtual float Y() const {
		static float c[][3] = { { .58f, .58f, .6f }, { .58f, .58f, .6f }, { .58f, .58f, .6f },
			{ .5f, .5f, .5f }, { .6f, .59f, .58f }, { .58f, .58f, .6f },
			{ .58f, .58f, .6f }, {.2f, .2f, .33f }, { .58f, .58f, .6f }, };
		RGBColor cs(0.f);
		for (u_int i = 0; i < NC; ++i)
			cs += RGBColor(c[i]);
		return cs.Y() / NC;
	}
	
	static Texture<float> * CreateFloatTexture(const Transform &tex2world, const TextureParams &tp);
	static Texture<SWCSpectrum> * CreateSWCSpectrumTexture(const Transform &tex2world, const TextureParams &tp);
private:
	// MarbleTexture Private Data
	int octaves;
	float omega, scale, variation;
	TextureMapping3D *mapping;
};

}//namespace lux

