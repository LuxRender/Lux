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

// band.h*
#include "lux.h"
#include "spectrum.h"
#include "texture.h"
#include "color.h"
#include "fresnelgeneral.h"
#include "paramset.h"
#include "error.h"

#include <sstream>
using std::stringstream;

namespace lux
{

// MixTexture Declarations
template <class T>
class BandTexture : public Texture<T> {
public:
	// MixTexture Public Methods
	BandTexture(u_int n, const float *o,
		vector<boost::shared_ptr<Texture<T> > > &t,
		boost::shared_ptr<Texture<float> > &a) :
		Texture<T>("BandTexture-" + boost::lexical_cast<string>(this)),
		offsets(o, o + n),
		tex(t), amount(a) { }
	virtual ~BandTexture() { }
	virtual T Evaluate(const SpectrumWavelengths &sw,
		const DifferentialGeometry &dg) const {
		const float a = amount->Evaluate(sw, dg);
		if (a < offsets.front())
			return tex.front()->Evaluate(sw, dg);
		if (a >= offsets.back())
			return tex.back()->Evaluate(sw, dg);
		u_int p = upper_bound(offsets.begin(), offsets.end(), a) -
			offsets.begin();
		return Lerp((a - offsets[p - 1]) / (offsets[p] - offsets[p - 1]),
			tex[p - 1]->Evaluate(sw, dg), tex[p]->Evaluate(sw, dg));
	}
	virtual float Y() const {
		float ret = offsets[0] * tex[0]->Y();
		for (u_int i = 0; i < offsets.size() - 1; ++i)
			ret += .5f * (offsets[i + 1] - offsets[i]) *
				(tex[i + 1]->Y() + tex[i]->Y());
		return ret;
	}
	virtual float Filter() const {
		float ret = offsets[0] * tex[0]->Y();
		for (u_int i = 0; i < offsets.size() - 1; ++i)
			ret += .5f * (offsets[i + 1] - offsets[i]) *
				(tex[i + 1]->Filter() + tex[i]->Filter());
		return ret;
	}
	virtual void GetDuv(const SpectrumWavelengths &sw,
		const DifferentialGeometry &dg, float delta,
		float *du, float *dv) const {
		const float a = amount->Evaluate(sw, dg);
		if (a < offsets.front()) {
			tex.front()->GetDuv(sw, dg, delta, du, dv);
			return;
		}
		if (a >= offsets.back()) {
			tex.back()->GetDuv(sw, dg, delta, du, dv);
			return;
		}
		u_int p = upper_bound(offsets.begin(), offsets.end(), a) -
			offsets.begin();
		float dua, dva, du1, dv1, du2, dv2;
		amount->GetDuv(sw, dg, delta, &dua, &dva);
		tex[p - 1]->GetDuv(sw, dg, delta, &du1, &dv1);
		tex[p]->GetDuv(sw, dg, delta, &du2, &dv2);
		float d = tex[p]->EvalFloat(sw, dg) -
			tex[p - 1]->EvalFloat(sw, dg);
		*du = Lerp(a, du1, du2) + d * dua;
		*dv = Lerp(a, dv1, dv2) + d * dva;
	}
	virtual void GetMinMaxFloat(float *minValue, float *maxValue) const {
		tex.front()->GetMinMaxFloat(minValue, maxValue);
		for (u_int i = 1; i < offsets.size() - 1; ++i) {
			float minv, maxv;
			tex[i]->GetMinMaxFloat(&minv, &maxv);
			*minValue = min(*minValue, minv);
			*maxValue = max(*maxValue, maxv);
		}
	}
	virtual void SetIlluminant() {
		// Update sub-textures
		for (u_int i = 0; i < tex.size(); ++i)
			tex[i]->SetIlluminant();
	}
	static Texture<float> * CreateFloatTexture(const Transform &tex2world, const ParamSet &tp);
	static Texture<SWCSpectrum> * CreateSWCSpectrumTexture(const Transform &tex2world, const ParamSet &tp);
	static Texture<FresnelGeneral> * CreateFresnelTexture(const Transform &tex2world, const ParamSet &tp);
private:
	vector<float> offsets;
	vector<boost::shared_ptr<Texture<T> > > tex;
	boost::shared_ptr<Texture<float> > amount;
};

// MixTexture Method Definitions
template <class T> Texture<float> * BandTexture<T>::CreateFloatTexture(const Transform &tex2world,
	const ParamSet &tp) {
	u_int n;
	const float *o = tp.FindFloat("offsets", &n);
	for (u_int i = 0; i < n - 1; ++i)
		if (o[i] > o[i + 1])
			LOG(LUX_ERROR, LUX_LIMIT) << "Offsets in 'band' texture are not in ascending order";
	vector<boost::shared_ptr<Texture<float> > > tex;
	tex.reserve(n);
	for (u_int i = 0; i < n; ++i) {
		stringstream ss;
		ss << "tex" << (i + 1);
		tex.push_back(tp.GetFloatTexture(ss.str(), 0.f));
	}
	boost::shared_ptr<Texture<float> > a(tp.GetFloatTexture("amount", 0.f));
	return new BandTexture<float>(n, o, tex, a);
}

template <class T> Texture<SWCSpectrum> * BandTexture<T>::CreateSWCSpectrumTexture(const Transform &tex2world,
	const ParamSet &tp) {
	u_int n;
	const float *o = tp.FindFloat("offsets", &n);
	for (u_int i = 0; i < n - 1; ++i)
		if (o[i] > o[i + 1])
			LOG(LUX_ERROR, LUX_LIMIT) << "Offsets in 'band' texture are not in ascending order";
	vector<boost::shared_ptr<Texture<SWCSpectrum> > > tex;
	tex.reserve(n);
	for (u_int i = 0; i < n; ++i) {
		stringstream ss;
		ss << "tex" << (i + 1);
		tex.push_back(tp.GetSWCSpectrumTexture(ss.str(), 0.f));
	}
	boost::shared_ptr<Texture<float> > a(tp.GetFloatTexture("amount", 0.f));
	return new BandTexture<SWCSpectrum>(n, o, tex, a);
}

template <class T> Texture<FresnelGeneral> * BandTexture<T>::CreateFresnelTexture(const Transform &tex2world,
	const ParamSet &tp) {
	u_int n;
	const float *o = tp.FindFloat("offsets", &n);
	for (u_int i = 0; i < n - 1; ++i)
		if (o[i] > o[i + 1])
			LOG(LUX_ERROR, LUX_LIMIT) << "Offsets in 'band' texture are not in ascending order";
	vector<boost::shared_ptr<Texture<FresnelGeneral> > > tex;
	tex.reserve(n);
	for (u_int i = 0; i < n; ++i) {
		stringstream ss;
		ss << "tex" << (i + 1);
		tex.push_back(tp.GetFresnelTexture(ss.str(), 0.f));
	}
	boost::shared_ptr<Texture<float> > a(tp.GetFloatTexture("amount", 0.f));
	return new BandTexture<FresnelGeneral>(n, o, tex, a);
}

}//namespace lux

