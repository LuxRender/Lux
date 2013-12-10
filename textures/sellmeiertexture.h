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

// sellmeiertexture.h*
#include "lux.h"
#include "texture.h"
#include "fresnelgeneral.h"
#include "luxrays/core/color/spectrumwavelengths.h"
#include "memory.h"
#include "paramset.h"

namespace lux
{

// SellmeierTexture Declarations
class SellmeierTexture : public Texture<FresnelGeneral> {
public:
	// SellmeierTexture Public Methods
	SellmeierTexture(float a_, u_int n, const float *b_, const float *c_) :
		Texture("SellmeierTexture-" + boost::lexical_cast<string>(this)),
		b(b_, b_ + n), c(c_, c_ + n), a(a_) {
		// Sellmeier expects wavelength in µm but we have it in nm
		for (u_int i = 0; i < n; ++i)
			c[i] *= 1e6f;
		// Compute the mean value of the ior
		index = 0.f;
		for (u_int i = WAVELENGTH_START; i < WAVELENGTH_END; ++i) {
			float i2 = a;
			for (u_int j = 0; j < n; ++j)
				i2 += b[j] / (1.f - c[j] / (i * i));
			index += sqrtf(i2);
		}
		index /= (WAVELENGTH_END - WAVELENGTH_START);
	}
	virtual ~SellmeierTexture() { }
	virtual FresnelGeneral Evaluate(const SpectrumWavelengths &sw,
		const DifferentialGeometry &dg) const {
		const SWCSpectrum w(sw.w);
		const SWCSpectrum w2(w * w);
		SWCSpectrum ior2(a);
		for (u_int i = 0; i < b.size(); ++i)
			ior2 += b[i] * w2 / (w2 - SWCSpectrum(c[i]));
		return FresnelGeneral(DIELECTRIC_FRESNEL, Sqrt(ior2),
			SWCSpectrum(0.f));
	}
	virtual float Y() const { return index; }
	virtual void GetDuv(const SpectrumWavelengths &sw,
		const DifferentialGeometry &dg, float delta,
		float *du, float *dv) const { *du = *dv = 0.f; }

	static Texture<FresnelGeneral> *CreateFresnelTexture(const Transform &tex2world, const ParamSet &tp);
private:
	vector<float> b, c;
	float a, index;
};

}//namespace lux

