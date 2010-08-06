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

// tabulatedfresnel.h*
#include "lux.h"
#include "memory.h"
#include "texture.h"
#include "irregular.h"
#include "fresnelgeneral.h"
#include "paramset.h"

namespace lux
{

// TabulatedFresnel Declarations
class TabulatedFresnel : public Texture<FresnelGeneral> {
public:
	// TabulatedFresnel Public Methods
	TabulatedFresnel(const vector<float> &wl, const vector<float> &n,
		const vector<float> &k) :
		N(&wl[0], &n[0], wl.size()), K(&wl[0], &k[0], wl.size()),
		index(N.Filter()) { }
	virtual ~TabulatedFresnel() { }
	virtual FresnelGeneral Evaluate(const SpectrumWavelengths &sw,
		const DifferentialGeometry &) const {
		// FIXME - Try to detect the best model to use
		// FIXME - FresnelGeneral should take a float index for accurate
		// non dispersive behaviour
		return FresnelGeneral(AUTO_FRESNEL, SWCSpectrum(sw, N), SWCSpectrum(sw, K));
	}
	virtual float Y() const { return index; }
	virtual void GetDuv(const SpectrumWavelengths &sw,
		const DifferentialGeometry &dg, float delta,
		float *du, float *dv) const { *du = *dv = 0.f; }

private:
	IrregularSPD N, K;
	float index;
};

// SopraTexture Declarations
class SopraTexture {
public:
	// SopraTexture Public Methods
	static Texture<FresnelGeneral> *CreateFresnelTexture(const Transform &tex2world, const ParamSet &tp);
};

// LuxpopTexture Declarations
class LuxpopTexture {
public:
	// LuxpopTexture Public Methods
	static Texture<FresnelGeneral> *CreateFresnelTexture(const Transform &tex2world, const ParamSet &tp);
};

// FresnelPreset Declarations
class FresnelPreset {
public:
	// FresnelPreset Public Methods
	static Texture<FresnelGeneral> *CreateFresnelTexture(const Transform &tex2world, const ParamSet &tp);
};

}//namespace lux

