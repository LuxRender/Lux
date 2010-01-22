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

// mix.cpp*
#include "lux.h"
#include "texture.h"
#include "color.h"
#include "paramset.h"

namespace lux
{

// MixTexture Declarations
template <class T>
class MixTexture : public Texture<T> {
public:
	// MixTexture Public Methods
	MixTexture(boost::shared_ptr<Texture<T> > t1,
			   boost::shared_ptr<Texture<T> > t2,
			   boost::shared_ptr<Texture<float> > amt) {
		tex1 = t1;
		tex2 = t2;
		amount = amt;
	}
	virtual ~MixTexture() { }
	virtual T Evaluate(const TsPack *tspack, const DifferentialGeometry &dg) const {
		T t1 = tex1->Evaluate(tspack, dg), t2 = tex2->Evaluate(tspack, dg);
		float amt = amount->Evaluate(tspack, dg);
		return Lerp(amt, t1, t2);
	}
	virtual float Y() const { return Lerp(amount->Y(), tex1->Y(), tex2->Y()); }
	virtual float Filter() const { return Lerp(amount->Y(), tex1->Filter(), tex2->Filter()); }
	virtual void SetIlluminant() {
		// Update sub-textures
		tex1->SetIlluminant();
		tex2->SetIlluminant();
	}
	static Texture<float> * CreateFloatTexture(const Transform &tex2world, const ParamSet &tp);
	static Texture<SWCSpectrum> * CreateSWCSpectrumTexture(const Transform &tex2world, const ParamSet &tp);
private:
	boost::shared_ptr<Texture<T> > tex1, tex2;
	boost::shared_ptr<Texture<float> > amount;
};

// MixTexture Method Definitions
template <class T> Texture<float> * MixTexture<T>::CreateFloatTexture(const Transform &tex2world,
	const ParamSet &tp) {
	return new MixTexture<float>(
		tp.GetFloatTexture("tex1", 0.f),
		tp.GetFloatTexture("tex2", 1.f),
		tp.GetFloatTexture("amount", 0.5f));
}

template <class T> Texture<SWCSpectrum> * MixTexture<T>::CreateSWCSpectrumTexture(const Transform &tex2world,
	const ParamSet &tp) {
	return new MixTexture<SWCSpectrum>(
		tp.GetSWCSpectrumTexture("tex1", RGBColor(0.f)),
		tp.GetSWCSpectrumTexture("tex2", RGBColor(1.f)),
		tp.GetFloatTexture("amount", 0.5f));
}

}//namespace lux

