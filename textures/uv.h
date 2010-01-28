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
	UVTexture(TextureMapping2D *m) {
		mapping = m;
	}
	virtual ~UVTexture() {
		delete mapping;
	}
	virtual SWCSpectrum Evaluate(const TsPack *tspack, const DifferentialGeometry &dg) const {
		float s, t, dsdx, dtdx, dsdy, dtdy;
		mapping->Map(dg, &s, &t, &dsdx, &dtdx, &dsdy, &dtdy);
		float cs[COLOR_SAMPLES];
		memset(cs, 0, COLOR_SAMPLES * sizeof(float));
		cs[0] = s - Floor2Int(s);
		cs[1] = t - Floor2Int(t);
		return SWCSpectrum(tspack, RGBColor(cs));
	}
	virtual float Y() const {
		const float cs[COLOR_SAMPLES] = {1.f, 1.f, 0.f};
		return RGBColor(cs).Y() / 2.f;
	}
	virtual float Filter() const { return 2.f / 3.f; }
	
	static Texture<SWCSpectrum> * CreateSWCSpectrumTexture(const Transform &tex2world, const ParamSet &tp);
private:
	TextureMapping2D *mapping;
};

}//namespace lux

