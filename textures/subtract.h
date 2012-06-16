/***************************************************************************
 *	 Copyright (C) 1998-2009 by authors (see AUTHORS.txt )								 *
 *																																				 *
 *	 This file is part of LuxRender.																			 *
 *																																				 *
 *	 Lux Renderer is free software; you can redistribute it and/or modify	*
 *	 it under the terms of the GNU General Public License as published by	*
 *	 the Free Software Foundation; either version 3 of the License, or		 *
 *	 (at your option) any later version.																	 *
 *																																				 *
 *	 Lux Renderer is distributed in the hope that it will be useful,			 *
 *	 but WITHOUT ANY WARRANTY; without even the implied warranty of				*
 *	 MERCHANTABILITY or FITNESS FOR A PARTICULAR PURPOSE.	See the				 *
 *	 GNU General Public License for more details.													*
 *																																				 *
 *	 You should have received a copy of the GNU General Public License		 *
 *	 along with this program.	If not, see <http://www.gnu.org/licenses/>. *
 *																																				 *
 *	 This project is based on PBRT ; see http://www.pbrt.org							 *
 *	 Lux Renderer website : http://www.luxrender.net											 *
 ***************************************************************************/

/**
 * This texture provides texture subtraction. This can be used, for example, to remove
 * a certain amount of red to a texture and make it shift color tone.
 */

#include "lux.h"
#include "context.h"
#include "spectrum.h"
#include "texture.h"
#include "color.h"
#include "paramset.h"

#include <map>
using std::map;

namespace lux
{

// SubtractTexture Declarations
template <class T1, class T2>
class SubtractTexture : public Texture<T2> {
public:
	// SubtractTexture Public Methods
	SubtractTexture(boost::shared_ptr<Texture<T1> > &t1, boost::shared_ptr<Texture<T2> > &t2) : tex1(t1), tex2(t2) { 
		// Nothing
	}
	virtual ~SubtractTexture() { 
		// Nothing
	}

	virtual T2 Evaluate(const SpectrumWavelengths &sw, const DifferentialGeometry &dg) const {
		return tex1->Evaluate(sw, dg) - tex2->Evaluate(sw, dg);
	}
	
	// In Y() one of the textures must use Filter to avoid double W->lm conv
	virtual float Y() const { 
		return tex1->Filter() - tex2->Y(); 
	}
	
	virtual float Filter() const { 
		return tex1->Filter() - tex2->Filter(); 
	}
	
	virtual void GetDuv(const SpectrumWavelengths &sw,
		const DifferentialGeometry &dg, float delta,
		float *du, float *dv) const {
		float du1, dv1, du2, dv2;
		tex1->GetDuv(sw, dg, delta, &du1, &dv1);
		tex2->GetDuv(sw, dg, delta, &du2, &dv2);
		float t1 = tex1->EvalFloat(sw, dg);
		float t2 = tex2->EvalFloat(sw, dg);
		*du = t1 - du2 + t2 - du1;
		*dv = t1 - dv2 + t2 - dv1;
	}
	
	virtual void GetMinMaxFloat(float *minValue, float *maxValue) const {
		float min1, min2;
		float max1, max2;
		tex1->GetMinMaxFloat(&min1, &max1);
		tex2->GetMinMaxFloat(&min2, &max2);
		const float minmin = min1 - min2;
		const float minmax = min1 - max2;
		const float maxmin = max1 - min2;
		const float maxmax = max1 - max2;
		*minValue = min(min(minmin, minmax), min(maxmin, maxmax));
		*maxValue = max(max(minmin, minmax), max(maxmin, maxmax));
	}
	
	virtual void SetIlluminant() {
		// Update sub-textures
		tex1->SetIlluminant();
		tex2->SetIlluminant();
	}
	
	static Texture<float> * CreateFloatTexture(const Transform &tex2world, const ParamSet &tp);
	
	static Texture<SWCSpectrum> * CreateSWCSpectrumTexture(const Transform &tex2world, const ParamSet &tp);

private:
	boost::shared_ptr<Texture<T1> > tex1;
	boost::shared_ptr<Texture<T2> > tex2;

};

// SubtractTexture Method Definitions
template <class T, class U> inline Texture<float> * SubtractTexture<T,U>::CreateFloatTexture(const Transform &tex2world,
	const ParamSet &tp)
{
	boost::shared_ptr<Texture<float> > tex1(tp.GetFloatTexture("tex1", 1.f)),
		tex2(tp.GetFloatTexture("tex2", 1.f));
	return new SubtractTexture<float, float>(tex1,tex2);
}

template <class T,class U> inline Texture<SWCSpectrum> * SubtractTexture<T,U>::CreateSWCSpectrumTexture(const Transform &tex2world,
	const ParamSet &tp)
{
	boost::shared_ptr<Texture<SWCSpectrum> > tex2(tp.GetSWCSpectrumTexture("tex2", RGBColor(1.f)));
	map<string, boost::shared_ptr<Texture<float> > > *ft = Context::GetActiveFloatTextures();
	if (ft->find(string("tex1")) == ft->end()) {
		boost::shared_ptr<Texture<SWCSpectrum> > tex1(tp.GetSWCSpectrumTexture("tex1", RGBColor(1.f)));
		return new SubtractTexture<SWCSpectrum, SWCSpectrum>(tex1, tex2);
	} else {
		boost::shared_ptr<Texture<float> > ftex1(tp.GetFloatTexture("tex1", 1.f));
		return new SubtractTexture<float, SWCSpectrum>(ftex1, tex2);
	}
}

}//namespace lux

