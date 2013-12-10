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

// exponential.h*
#include "lux.h"
#include "luxrays/core/color/swcspectrum.h"
#include "texture.h"
#include "geometry/raydifferential.h"
#include "paramset.h"

namespace lux
{

// ExponentialTexture Declarations
class ExponentialTexture : public Texture<float> {
public:
	// ExponentialTexture Public Methods
	ExponentialTexture(const Point &o, const Vector &up, float d,
		TextureMapping3D *map) :
		Texture("ExponentialTexture-" + boost::lexical_cast<string>(this)),
		origin(o), upDir(up), decay(d), mapping(map) { }
	virtual ~ExponentialTexture() { delete mapping; }
	virtual float Evaluate(const SpectrumWavelengths &sw,
		const DifferentialGeometry &dg) const {
		const Point P(mapping->Map(dg));
		return luxrays::Clamp(expf(-decay * Dot(P - origin, upDir)), 0.f, 1.f);
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
	virtual void GetMinMaxFloat(float *minValue, float *maxValue) const {
		*minValue = 0.f;
		*maxValue = 1.f;
	}

	const TextureMapping3D *GetTextureMapping3D() const { return mapping; }

	static Texture<float> * CreateFloatTexture(const Transform &tex2world, const ParamSet &tp);
private:
	// ExponentialTexture Private Data
	Point origin;
	Vector upDir;
	float decay;
	TextureMapping3D *mapping;
};


// ExponentialTexture Method Definitions
inline Texture<float> * ExponentialTexture::CreateFloatTexture(const Transform &tex2world,
	const ParamSet &tp) {
	// Read mapping coordinates
	TextureMapping3D *imap = TextureMapping3D::Create(tex2world, tp);
	return new ExponentialTexture(tp.FindOnePoint("origin", Point(0, 0, 0)),
		tp.FindOneVector("updir", Vector(0, 0, 1)),
		tp.FindOneFloat("decay", 1.f), imap);
}

}//namespace lux

