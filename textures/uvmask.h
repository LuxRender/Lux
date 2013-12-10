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

// uvmask.cpp*
#include "lux.h"
#include "luxrays/core/color/swcspectrum.h"
#include "texture.h"
#include "luxrays/core/color/color.h"
#include "paramset.h"

namespace lux
{

// UVMaskFloatTexture Declarations
template <class T>
class UVMaskTexture : public Texture<T>
{
public:

	// UVMaskFloatTexture Public Methods
	UVMaskTexture(TextureMapping2D *m,
	              boost::shared_ptr<Texture<T> > &_innerTex,
	              boost::shared_ptr<Texture<T> > &_outerTex) :
		Texture<T>("UVMaskTexture-" + boost::lexical_cast<string>(this)),
		innerTex(_innerTex), outerTex(_outerTex) { mapping = m; }

	virtual ~UVMaskTexture() { delete mapping; }

	virtual float Evaluate(const SpectrumWavelengths &sw,
		const DifferentialGeometry &dg) const {
		float s, t;
		mapping->Map(dg, &s, &t);
		if (s < 0.f || s > 1.f || t < 0.f || t > 1.f)
			return outerTex->Evaluate(sw, dg);
		else
			return innerTex->Evaluate(sw, dg);
	}

	virtual float Y() const { return (innerTex->Y() + outerTex->Y()) * .5f; }

	virtual void GetDuv(const SpectrumWavelengths &sw,
		const DifferentialGeometry &dg,
		float delta, float *du, float *dv) const {
		float s, t, dsdu, dtdu, dsdv, dtdv;
		mapping->MapDuv(dg, &s, &t, &dsdu, &dtdu, &dsdv, &dtdv);
		const float ds = delta * (dsdu + dsdv);
		const float dt = delta * (dtdu + dtdv);
		*du = 0.f;
		*dv = 0.f;
		if (fabsf(s) < ds) {
			*du += dsdu;
			*dv += dsdv;
		} else if (fabsf(s - 1.f) < ds) {
			*du -= dsdu;
			*dv -= dsdv;
		}
		if (fabsf(t) < dt) {
			*du += dtdu;
			*dv += dtdv;
		} else if (fabsf(t - 1.f) < dt) {
			*du -= dtdu;
			*dv -= dtdv;
		}
		if (*du || *dv) {
			const float d = (innerTex->EvalFloat(sw, dg) -
				outerTex->EvalFloat(sw, dg)) / delta;
			*du *= d;
			*dv *= d;
		}
	}
	virtual void GetMinMaxFloat(float *minValue, float *maxValue) const {
		float min1, min2;
		float max1, max2;
		innerTex->GetMinMaxFloat(&min1, &max1);
		outerTex->GetMinMaxFloat(&min2, &max2);
		*minValue = min(min1, min2);
		*maxValue = max(max1, max2);
	}

	static Texture<float> * CreateFloatTexture(const Transform &tex2world,
	                                           const ParamSet  &tp);

private:

	TextureMapping2D               *mapping;
	boost::shared_ptr<Texture<T> > innerTex;
	boost::shared_ptr<Texture<T> > outerTex;
};


template <class T>
Texture<float> * UVMaskTexture<T>::CreateFloatTexture(const Transform &tex2world,
                                                      const ParamSet  &tp)
{
	// Initialize 2D texture mapping _map_ from _tp_
	boost::shared_ptr<Texture<float> > innerTex(tp.GetFloatTexture("innertex", 1.f));
	boost::shared_ptr<Texture<float> > outerTex(tp.GetFloatTexture("outertex", 0.f));
  return new UVMaskTexture(TextureMapping2D::Create(tex2world, tp), innerTex, outerTex);
}



}//namespace lux

