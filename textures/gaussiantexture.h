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

// gaussian.cpp*
#include "lux.h"
#include "texture.h"
#include "gaussianspd.h"
#include "paramset.h"

namespace lux
{

// GaussianTexture Declarations
template <class T>
class GaussianFloatTexture : public Texture<T> {
public:
	// GaussianTexture Public Methods
	GaussianFloatTexture(const T &v) { value = v; }
	virtual ~GaussianFloatTexture() { }
	virtual T Evaluate(const TsPack *tspack, const DifferentialGeometry &) const {
		return value;
	}
private:
	T value;
};

template <class T>
class GaussianSpectrumTexture : public Texture<T> {
public:
	// GaussianTexture Public Methods
	GaussianSpectrumTexture(const float &m, const float &w, const float &r) {
		GSPD = new GaussianSPD(m, w, r);
	}
	virtual ~GaussianSpectrumTexture() { delete GSPD; }
	virtual T Evaluate(const TsPack *tspack, const DifferentialGeometry &) const {
		return SWCSpectrum(tspack, GSPD);
	}
	virtual void SetPower(float power, float area) {
		GSPD->Scale(power / (area * M_PI * GSPD->Y()));
	}
private:
	GaussianSPD* GSPD;
};

class GaussianTexture
{
public:
	static Texture<float> * CreateFloatTexture(const Transform &tex2world, const TextureParams &tp);
	static Texture<SWCSpectrum> * CreateSWCSpectrumTexture(const Transform &tex2world, const TextureParams &tp);
};

}//namespace lux

