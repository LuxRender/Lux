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
	ConstantFloatTexture(float v) : value(v) { }
	virtual ~ConstantFloatTexture() { }
	virtual float Evaluate(const SpectrumWavelengths &sw,
		const DifferentialGeometry &) const {
		return value;
	}
	virtual float Y() const { return value; }
	virtual void GetDuv(const SpectrumWavelengths &sw,
		const DifferentialGeometry &dg, float delta,
		float *du, float *dv) const { *du = *dv = 0.f; }
private:
	float value;
};

class ConstantRGBColorTexture : public Texture<SWCSpectrum> {
public:
	// ConstantTexture Public Methods
	ConstantRGBColorTexture(const RGBColor &s) : color(s) {
		RGBSPD = new RGBReflSPD(color);
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
	SPD* RGBSPD;
	RGBColor color;
};

class ConstantFresnelTexture : public Texture<FresnelGeneral> {
public:
	// ConstantTexture Public Methods
	ConstantFresnelTexture(float v) :
		value(DIELECTRIC_FRESNEL, SWCSpectrum(v), 0.f), val(v) { }
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

