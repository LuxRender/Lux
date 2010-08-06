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

// wrinkled.cpp*
#include "lux.h"
#include "spectrum.h"
#include "texture.h"
#include "geometry/raydifferential.h"
#include "paramset.h"

// TODO - radiance - add methods for Power and Illuminant propagation

namespace lux
{

// WrinkledTexture Declarations
class WrinkledTexture : public Texture<float> {
public:
	// WrinkledTexture Public Methods
	WrinkledTexture(int oct, float roughness, TextureMapping3D *map) {
		omega = roughness;
		octaves = oct;
		mapping = map;
	}
	virtual ~WrinkledTexture() { delete mapping; }
	virtual float Evaluate(const SpectrumWavelengths &sw,
		const DifferentialGeometry &dg) const {
		Vector dpdx, dpdy;
		Point P = mapping->MapDxy(dg, &dpdx, &dpdy);
		return Turbulence(P, dpdx, dpdy, omega, octaves);
	}
	virtual float Y() const { return .5f; }
	virtual void GetDuv(const SpectrumWavelengths &sw,
		const DifferentialGeometry &dg, float delta,
		float *du, float *dv) const {
		//FIXME: Generic derivative computation, replace with exact
		DifferentialGeometry dgTemp = dg;
		// Calculate bump map value at intersection point
		const float base = Evaluate(sw, dg);

		// Compute offset positions and evaluate displacement texture
		const Point origP(dgTemp.p);
		const Normal origN(dgTemp.nn);
		const float origU = dgTemp.u;

		// Shift _dgTemp_ _du_ in the $u$ direction and calculate value
		const float uu = delta / dgTemp.dpdu.Length();
		dgTemp.p += uu * dgTemp.dpdu;
		dgTemp.u += uu;
		dgTemp.nn = Normalize(origN + uu * dgTemp.dndu);
		*du = (Evaluate(sw, dgTemp) - base) / uu;

		// Shift _dgTemp_ _dv_ in the $v$ direction and calculate value
		const float vv = delta / dgTemp.dpdv.Length();
		dgTemp.p = origP + vv * dgTemp.dpdv;
		dgTemp.u = origU;
		dgTemp.v += vv;
		dgTemp.nn = Normalize(origN + vv * dgTemp.dndv);
		*dv = (Evaluate(sw, dgTemp) - base) / vv;
	}
	
	static Texture<float> * CreateFloatTexture(const Transform &tex2world, const ParamSet &tp);
private:
	// WrinkledTexture Private Data
	int octaves;
	float omega;
	TextureMapping3D *mapping;
};


// WrinkledTexture Method Definitions
inline Texture<float> * WrinkledTexture::CreateFloatTexture(const Transform &tex2world,
	const ParamSet &tp) {
	// Apply texture specified transformation option for 3D mapping
	IdentityMapping3D *imap = new IdentityMapping3D(tex2world);
	imap->Apply3DTextureMappingOptions(tp);
	return new WrinkledTexture(tp.FindOneInt("octaves", 8),
		tp.FindOneFloat("roughness", .5f), imap);
}

}//namespace lux

