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

// constant.cpp*
#include "lux.h"
#include "texture.h"
#include "rgbrefl.h"
#include "rgbillum.h"
#include "fresnelgeneral.h"
#include "paramset.h"

namespace lux
{

// ConstantTexture Declarations
class ConstantFloatTexture : public Texture<float> {
public:
	// ConstantTexture Public Methods
	ConstantFloatTexture(float v) :
		Texture("ConstantFloatTexture-" + boost::lexical_cast<string>(this)), value(v) {
			AddFloatAttribute(*this, "value", "ConstantFloatTexture value", &ConstantFloatTexture::value);
		}
	virtual ~ConstantFloatTexture() { }
	virtual float Evaluate(const SpectrumWavelengths &sw,
		const DifferentialGeometry &) const {
		return value;
	}
	virtual float Y() const { return value; }
	virtual void GetDuv(const SpectrumWavelengths &sw,
		const DifferentialGeometry &dg, float delta,
		float *du, float *dv) const { *du = *dv = 0.f; }
	virtual void GetMinMaxFloat(float *minValue, float *maxValue) const {
		*minValue = value;
		*maxValue = value;
	}
private:
	float value;
};

class ConstantRGBColorTexture : public Texture<SWCSpectrum> {
public:
	// ConstantTexture Public Methods
	ConstantRGBColorTexture(const RGBColor &s) :
		Texture("ConstantRGBColorTexture-" + boost::lexical_cast<string>(this)),
		color(s) {
		RGBSPD = new RGBReflSPD(color);

		AddFloatAttribute(*this, "color.r", "ConstantRGBColorTexture color R", &ConstantRGBColorTexture::GetColorR);
		AddFloatAttribute(*this, "color.g", "ConstantRGBColorTexture color G", &ConstantRGBColorTexture::GetColorG);
		AddFloatAttribute(*this, "color.b", "ConstantRGBColorTexture color B", &ConstantRGBColorTexture::GetColorB);
	}
	virtual ~ConstantRGBColorTexture() { delete RGBSPD; }
	virtual SWCSpectrum Evaluate(const SpectrumWavelengths &sw,
		const DifferentialGeometry &) const {
		return SWCSpectrum(sw, *RGBSPD);
	}
	virtual float Y() const { return RGBSPD->Y(); }
	virtual float Filter() const { return RGBSPD->Filter(); }
	virtual void GetDuv(const SpectrumWavelengths &sw,
		const DifferentialGeometry &dg, float delta,
		float *du, float *dv) const { *du = *dv = 0.f; }
	virtual void SetIlluminant() {
		delete RGBSPD;
		RGBSPD = new RGBIllumSPD(color);
	}

private:
	// Used by Query interface
	float GetColorR() { return color.c[0]; }
	float GetColorG() { return color.c[1]; }
	float GetColorB() { return color.c[2]; }

	SPD* RGBSPD;
	RGBColor color;
};

class ConstantFresnelTexture : public Texture<FresnelGeneral> {
public:
	// ConstantTexture Public Methods
	ConstantFresnelTexture(float v) :
		Texture("ConstantFresnelTexture-" + boost::lexical_cast<string>(this)),
		value(DIELECTRIC_FRESNEL, SWCSpectrum(v), 0.f), val(v) {
		AddFloatAttribute(*this, "value", "ConstantFresnelTexture value", &ConstantFresnelTexture::val);
	}
	virtual ~ConstantFresnelTexture() { }
	virtual FresnelGeneral Evaluate(const SpectrumWavelengths &sw,
		const DifferentialGeometry &) const {
		return value;
	}
	virtual float Y() const { return val; }
	virtual void GetDuv(const SpectrumWavelengths &sw,
		const DifferentialGeometry &dg, float delta,
		float *du, float *dv) const { *du = *dv = 0.f; }
private:
	FresnelGeneral value;
	float val;
};

class Constant
{
public:
	static Texture<float> *CreateFloatTexture(const Transform &tex2world, const ParamSet &tp);
	static Texture<SWCSpectrum> *CreateSWCSpectrumTexture(const Transform &tex2world, const ParamSet &tp);
	static Texture<FresnelGeneral> *CreateFresnelTexture(const Transform &tex2world, const ParamSet &tp);
};

}//namespace lux

