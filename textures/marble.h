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
#include "spectrum.h"
#include "texture.h"
#include "color.h"
#include "geometry/raydifferential.h"
#include "paramset.h"

// TODO - radiance - add methods for Power and Illuminant propagation

namespace lux
{

// MarbleTexture Declarations
class MarbleTexture : public Texture<SWCSpectrum> {
public:
	// MarbleTexture Public Methods
	virtual ~MarbleTexture() { delete mapping; }
	MarbleTexture(int oct, float roughness, float sc, float var,
		TextureMapping3D *map) : Texture("MarbleTexture-" + boost::lexical_cast<string>(this)) {
		omega = roughness;
		octaves = oct;
		mapping = map;
		scale = sc;
		variation = var;
	}
	virtual SWCSpectrum Evaluate(const SpectrumWavelengths &sw,
		const DifferentialGeometry &dg) const {
		Point P(mapping->Map(dg));
		P *= scale;
		float marble = P.y + variation * FBm(P, 0.f, 0.f, omega,
			octaves);
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
		RGBColor s0(Lerp(t, c0, c1));
		RGBColor s1(Lerp(t, c1, c2));
		RGBColor s2(Lerp(t, c2, c3));
		s0 = Lerp(t, s0, s1);
		s1 = Lerp(t, s1, s2);
		// Extra scale of 1.5 to increase variation among colors
		return SWCSpectrum(sw, 1.5f * Lerp(t, s0, s1));
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
	virtual float Filter() const {
		static float c[][3] = { { .58f, .58f, .6f }, { .58f, .58f, .6f }, { .58f, .58f, .6f },
			{ .5f, .5f, .5f }, { .6f, .59f, .58f }, { .58f, .58f, .6f },
			{ .58f, .58f, .6f }, {.2f, .2f, .33f }, { .58f, .58f, .6f }, };
		RGBColor cs(0.f);
		for (u_int i = 0; i < NC; ++i)
			cs += RGBColor(c[i]);
		return cs.Filter() / NC;
	}
	virtual void GetDuv(const SpectrumWavelengths &sw,
		const DifferentialGeometry &dg, float delta,
		float *du, float *dv) const {
		//FIXME: Generic derivative computation, replace with exact
		DifferentialGeometry dgTemp = dg;
		// Calculate bump map value at intersection point
		const float base = EvalFloat(sw, dg);

		// Compute offset positions and evaluate displacement texture
		const Point origP(dgTemp.p);
		const Normal origN(dgTemp.nn);
		const float origU = dgTemp.u;

		// Shift _dgTemp_ _du_ in the $u$ direction and calculate value
		const float uu = delta / dgTemp.dpdu.Length();
		dgTemp.p += uu * dgTemp.dpdu;
		dgTemp.u += uu;
		dgTemp.nn = Normalize(origN + uu * dgTemp.dndu);
		*du = (EvalFloat(sw, dgTemp) - base) / uu;

		// Shift _dgTemp_ _dv_ in the $v$ direction and calculate value
		const float vv = delta / dgTemp.dpdv.Length();
		dgTemp.p = origP + vv * dgTemp.dpdv;
		dgTemp.u = origU;
		dgTemp.v += vv;
		dgTemp.nn = Normalize(origN + vv * dgTemp.dndv);
		*dv = (EvalFloat(sw, dgTemp) - base) / vv;
	}
	
	static Texture<SWCSpectrum> * CreateSWCSpectrumTexture(const Transform &tex2world, const ParamSet &tp);
private:
	// MarbleTexture Private Data
	int octaves;
	float omega, scale, variation;
	TextureMapping3D *mapping;
};

}//namespace lux

