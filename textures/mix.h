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

// mix.cpp*
#include "lux.h"
#include "spectrum.h"
#include "texture.h"
#include "color.h"
#include "fresnelgeneral.h"
#include "paramset.h"

namespace lux
{

// MixTexture Declarations
template <class T>
class MixTexture : public Texture<T> {
public:
	// MixTexture Public Methods
	MixTexture(boost::shared_ptr<Texture<T> > &t1,
		boost::shared_ptr<Texture<T> > &t2,
		boost::shared_ptr<Texture<float> > &amt) :
		Texture<T>("MixTexture-" + boost::lexical_cast<string>(this)),
		tex1(t1), tex2(t2),	amount(amt) { }
	virtual ~MixTexture() { }
	virtual T Evaluate(const SpectrumWavelengths &sw,
		const DifferentialGeometry &dg) const {
		T t1 = tex1->Evaluate(sw, dg), t2 = tex2->Evaluate(sw, dg);
		float amt = amount->Evaluate(sw, dg);
		return Lerp(amt, t1, t2);
	}
	virtual float Y() const { return Lerp(amount->Y(), tex1->Y(),
		tex2->Y()); }
	virtual float Filter() const { return Lerp(amount->Y(), tex1->Filter(),
		tex2->Filter()); }
	virtual void GetDuv(const SpectrumWavelengths &sw,
		const DifferentialGeometry &dg, float delta,
		float *du, float *dv) const {
		float dua, dva, du1, dv1, du2, dv2;
		amount->GetDuv(sw, dg, delta, &dua, &dva);
		tex1->GetDuv(sw, dg, delta, &du1, &dv1);
		tex2->GetDuv(sw, dg, delta, &du2, &dv2);
		float a = amount->Evaluate(sw, dg);
		float d = tex2->EvalFloat(sw, dg) -
			tex1->EvalFloat(sw, dg);
		*du = Lerp(a, du1, du2) + d * dua;
		*dv = Lerp(a, dv1, dv2) + d * dva;
	}
	virtual void GetMinMaxFloat(float *minValue, float *maxValue) const {
		float mina, min1, min2;
		float maxa, max1, max2;
		amount->GetMinMaxFloat(&mina, &maxa);
		tex1->GetMinMaxFloat(&min1, &max1);
		tex2->GetMinMaxFloat(&min2, &max2);
		*minValue = min(Lerp(mina, min1, min2), Lerp(maxa, min1, min2));
		*maxValue = max(Lerp(mina, max1, max2), Lerp(maxa, max1, max2));
	}
	virtual void SetIlluminant() {
		// Update sub-textures
		tex1->SetIlluminant();
		tex2->SetIlluminant();
	}

	const Texture<float> *GetAmountTex() const { return amount.get(); }
	const Texture<T> *GetTex1() const { return tex1.get(); }
	const Texture<T> *GetTex2() const { return tex2.get(); }

	static Texture<float> * CreateFloatTexture(const Transform &tex2world, const ParamSet &tp);
	static Texture<SWCSpectrum> * CreateSWCSpectrumTexture(const Transform &tex2world, const ParamSet &tp);
	static Texture<FresnelGeneral> * CreateFresnelTexture(const Transform &tex2world, const ParamSet &tp);
private:
	boost::shared_ptr<Texture<T> > tex1, tex2;
	boost::shared_ptr<Texture<float> > amount;
};

// MixTexture Method Definitions
template <class T> Texture<float> * MixTexture<T>::CreateFloatTexture(const Transform &tex2world,
	const ParamSet &tp) {
	boost::shared_ptr<Texture<float> > tex1(tp.GetFloatTexture("tex1", 0.f)),
		tex2(tp.GetFloatTexture("tex2", 1.f));
	boost::shared_ptr<Texture<float> > amount(tp.GetFloatTexture("amount", .5f));
	return new MixTexture<float>(tex1, tex2, amount);
}

template <class T> Texture<SWCSpectrum> * MixTexture<T>::CreateSWCSpectrumTexture(const Transform &tex2world,
	const ParamSet &tp) {
	boost::shared_ptr<Texture<SWCSpectrum> > tex1(tp.GetSWCSpectrumTexture("tex1", RGBColor(0.f))),
		tex2(tp.GetSWCSpectrumTexture("tex2", RGBColor(1.f)));
	boost::shared_ptr<Texture<float> > amount(tp.GetFloatTexture("amount", .5f));
	return new MixTexture<SWCSpectrum>(tex1, tex2, amount);
}

template <class T> Texture<FresnelGeneral> * MixTexture<T>::CreateFresnelTexture(const Transform &tex2world,
	const ParamSet &tp) {
	boost::shared_ptr<Texture<FresnelGeneral> > tex1(tp.GetFresnelTexture("tex1", 1.f)),
		tex2(tp.GetFresnelTexture("tex2", 1.5f));
	boost::shared_ptr<Texture<float> > amount(tp.GetFloatTexture("amount", .5f));
	return new MixTexture<FresnelGeneral>(tex1, tex2, amount);
}

}//namespace lux

