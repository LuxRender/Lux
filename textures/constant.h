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
#include "paramset.h"

namespace lux
{

// ConstantTexture Declarations
template <class T>
class ConstantFloatTexture : public Texture<T> {
public:
	// ConstantTexture Public Methods
	ConstantFloatTexture(const T &v) { value = v; }
	virtual ~ConstantFloatTexture() { }
	virtual T Evaluate(const TsPack *tspack, const DifferentialGeometry &) const {
		return value;
	}
private:
	T value;
};

template <class T>
class ConstantRGBColorTexture : public Texture<T> {
public:
	// ConstantTexture Public Methods
	ConstantRGBColorTexture(const RGBColor &s) {
		color = s;
		RGBSPD = new RGBReflSPD(color);
	}
	virtual ~ConstantRGBColorTexture() { delete RGBSPD; }
	virtual T Evaluate(const TsPack *tspack, const DifferentialGeometry &) const {
		return SWCSpectrum(tspack, RGBSPD);
	}
	virtual void SetPower(float power, float area) {
		RGBSPD->Scale(power / (area * M_PI * RGBSPD->y()));
	}
	virtual void SetIlluminant() {
		delete RGBSPD;
		RGBSPD = new RGBIllumSPD(color);
	}
private:
	SPD* RGBSPD;
	RGBColor color;
};

class Constant
{
public:
	static Texture<float> * CreateFloatTexture(const Transform &tex2world, const TextureParams &tp);
	static Texture<SWCSpectrum> * CreateSWCSpectrumTexture(const Transform &tex2world, const TextureParams &tp);
};

}//namespace lux

