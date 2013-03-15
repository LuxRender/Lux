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

// bilerp.cpp*
#include "lux.h"
#include "spectrum.h"
#include "texture.h"
#include "color.h"
#include "spds/rgbillum.h"
#include "fresnelgeneral.h"
#include "paramset.h"
#include "error.h"

namespace lux
{

// BilerpTexture Declarations
class BilerpFloatTexture : public Texture<float> {
public:
	// BilerpTexture Public Methods
	BilerpFloatTexture(TextureMapping2D *m,
		float t00, float t01, float t10, float t11) :
		Texture("BilerpFloatTexture-" + boost::lexical_cast<string>(this)) {
		mapping = m;
		v00 = t00;
		v01 = t01;
		v10 = t10;
		v11 = t11;
	}
	virtual ~BilerpFloatTexture() { delete mapping; }
	virtual float Evaluate(const SpectrumWavelengths &sw,
		const DifferentialGeometry &dg) const {
		float s, t;
		mapping->Map(dg, &s, &t);
		s -= Floor2Int(s);
		t -= Floor2Int(t);
		return (1.f - s) * (1.f - t) * v00 + (1.f - s) * t * v01 +
			s * (1.f - t) * v10 + s * t * v11;
	}
	virtual float Y() const { return (v00 + v01 + v10 + v11) / 4.f; }
	virtual void GetDuv(const SpectrumWavelengths &sw,
		const DifferentialGeometry &dg,
		float delta, float *du, float *dv) const {
		float s, t, dsdu, dtdu, dsdv, dtdv;
		mapping->MapDuv(dg, &s, &t, &dsdu, &dtdu, &dsdv, &dtdv);
		s -= Floor2Int(s);
		t -= Floor2Int(t);
		const float d = v00 + v11 - v01 - v10;
		*du = dsdu * (v10 - v00 + t * d) + dtdu * (v01 - v00 + s * d);
		*dv = dsdv * (v10 - v00 + t * d) + dtdv * (v01 - v00 + s * d);
	}
	virtual void GetMinMaxFloat(float *minValue, float *maxValue) const {
		*minValue = min(min(v00, v01), min(v10, v11));
		*maxValue = max(max(v00, v01), max(v10, v11));
	}	

	static Texture<float> * CreateFloatTexture(const Transform &tex2world, const ParamSet &tp);

private:
	// BilerpTexture Private Data
	TextureMapping2D *mapping;
	float v00, v01, v10, v11;
};

class BilerpSpectrumTexture : public Texture<SWCSpectrum> {
public:
	// BilerpTexture Public Methods
	BilerpSpectrumTexture(TextureMapping2D *m,
		const RGBColor &t00, const RGBColor &t01,
		const RGBColor &t10, const RGBColor &t11) :
		Texture("BilerpSpectrumTexture-" + boost::lexical_cast<string>(this)) {
		mapping = m;
		v00 = t00;
		v01 = t01;
		v10 = t10;
		v11 = t11;
		isIlluminant = false;
	}
	virtual ~BilerpSpectrumTexture() { delete mapping; }
	virtual SWCSpectrum Evaluate(const SpectrumWavelengths &sw,
		const DifferentialGeometry &dg) const {
		float s, t;
		mapping->Map(dg, &s, &t);
		s -= Floor2Int(s);
		t -= Floor2Int(t);
		return SWCSpectrum(sw, (1.f - s) * (1.f - t) * v00 +
			(1.f - s) * t * v01 + s * (1.f - t) * v10 +
			s * t * v11);
	}
	virtual float Y() const {
		return RGBColor(v00 + v01 + v10 + v11).Y() / 4.f;
	}
	virtual float Filter() const {
		return RGBColor(v00 + v01 + v10 + v11).Filter() / 4.f;
	}
	virtual void GetDuv(const SpectrumWavelengths &sw,
		const DifferentialGeometry &dg,
		float delta, float *du, float *dv) const {
		float s, t, dsdu, dtdu, dsdv, dtdv;
		mapping->MapDuv(dg, &s, &t, &dsdu, &dtdu, &dsdv, &dtdv);
		s -= Floor2Int(s);
		t -= Floor2Int(t);
		const float d = RGBColor(v00 + v11 - v01 - v10).Filter();
		const float d1 = RGBColor(v10 - v00).Filter();
		const float d2 = RGBColor(v01 - v00).Filter();
		*du = dsdu * (d1 + t * d) + dtdu * (d2 + s * d);
		*dv = dsdv * (d1 + t * d) + dtdv * (d2 + s * d);
	}
	virtual void SetIlluminant() { isIlluminant = true; }
	
	static Texture<SWCSpectrum> * CreateSWCSpectrumTexture(const Transform &tex2world, const ParamSet &tp);

private:
	// BilerpTexture Private Data
	TextureMapping2D *mapping;
	RGBColor v00, v01, v10, v11;
	static RGBIllumSPD whiteRGBIllum;
	bool isIlluminant;
};

class BilerpFresnelTexture : public Texture<FresnelGeneral> {
public:
	// BilerpTexture Public Methods
	BilerpFresnelTexture(TextureMapping2D *m,
		const boost::shared_ptr<Texture<FresnelGeneral> > &t00,
		const boost::shared_ptr<Texture<FresnelGeneral> > &t01,
		const boost::shared_ptr<Texture<FresnelGeneral> > &t10,
		const boost::shared_ptr<Texture<FresnelGeneral> > &t11) :
		Texture("BilerpFresnelTexture-" + boost::lexical_cast<string>(this)) {
		mapping = m;
		v00 = t00;
		v01 = t01;
		v10 = t10;
		v11 = t11;
	}
	virtual ~BilerpFresnelTexture() { delete mapping; }
	virtual FresnelGeneral Evaluate(const SpectrumWavelengths &sw,
		const DifferentialGeometry &dg) const {
		float s, t;
		mapping->Map(dg, &s, &t);
		s -= Floor2Int(s);
		t -= Floor2Int(t);
		return (1.f - s) * (1.f - t) * v00->Evaluate(sw, dg) +
			(1.f - s) * t * v01->Evaluate(sw, dg) +
			s * (1.f - t) * v10->Evaluate(sw, dg) +
			s * t * v11->Evaluate(sw, dg);
	}
	virtual float Y() const {
		return (v00->Y() + v01->Y() + v10->Y() + v11->Y()) / 4.f;
	}
	virtual float Filter() const {
		return (v00->Filter() + v01->Filter() + v10->Filter() + v11->Filter()) / 4.f;
	}
	virtual void GetDuv(const SpectrumWavelengths &sw,
		const DifferentialGeometry &dg,
		float delta, float *du, float *dv) const {
		float s, t, dsdu, dtdu, dsdv, dtdv;
		mapping->MapDuv(dg, &s, &t, &dsdu, &dtdu, &dsdv, &dtdv);
		s -= Floor2Int(s);
		t -= Floor2Int(t);
		const float d = v00->Filter() + v11->Filter() - v01->Filter() - v10->Filter();
		const float d1 = v10->Filter() - v00->Filter();
		const float d2 = v01->Filter() - v00->Filter();
		*du = dsdu * (d1 + t * d) + dtdu * (d2 + s * d);
		*dv = dsdv * (d1 + t * d) + dtdv * (d2 + s * d);
	}

	static Texture<FresnelGeneral> * CreateFresnelTexture(const Transform &tex2world, const ParamSet &tp);

private:
	// BilerpTexture Private Data
	TextureMapping2D *mapping;
	boost::shared_ptr<Texture<FresnelGeneral> > v00, v01, v10, v11;
};

}//namespace lux

