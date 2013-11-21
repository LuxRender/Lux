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

// uv.cpp*
#include "lux.h"
#include "spectrum.h"
#include "texture.h"
#include "color.h"
#include "paramset.h"

// TODO - radiance - add methods for Power and Illuminant propagation

namespace lux
{

// UVTexture Declarations
class UVTexture : public Texture<SWCSpectrum> {
public:
	// UVTexture Public Methods
	UVTexture(TextureMapping2D *m) : Texture("UVTexture-" + boost::lexical_cast<string>(this)) {
		mapping = m;
	}
	virtual ~UVTexture() {
		delete mapping;
	}
	virtual SWCSpectrum Evaluate(const SpectrumWavelengths &sw,
		const DifferentialGeometry &dg) const {
		float s, t;
		mapping->Map(dg, &s, &t);
		const float cs[COLOR_SAMPLES] = { s - luxrays::Floor2Int(s), t - luxrays::Floor2Int(t), 0.f };
		return SWCSpectrum(sw, RGBColor(cs));
	}
	virtual float Y() const {
		const float cs[COLOR_SAMPLES] = {.5f, .5f, 0.f};
		return RGBColor(cs).Y();
	}
	virtual float Filter() const { return 2.f / 3.f; }
	virtual void GetDuv(const SpectrumWavelengths &sw,
		const DifferentialGeometry &dg, float delta,
		float *du, float *dv) const {
		float s, t, dsdu, dtdu, dsdv, dtdv;
		mapping->MapDuv(dg, &s, &t, &dsdu, &dtdu, &dsdv, &dtdv);
		*du = dsdu + dtdu;
		*dv = dsdv + dtdv;
	}

	const TextureMapping2D *GetTextureMapping2D() const { return mapping; }

	static Texture<SWCSpectrum> * CreateSWCSpectrumTexture(const Transform &tex2world, const ParamSet &tp);
private:
	TextureMapping2D *mapping;
};

}//namespace lux

