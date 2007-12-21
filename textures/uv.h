/***************************************************************************
 *   Copyright (C) 1998-2007 by authors (see AUTHORS.txt )                 *
 *                                                                         *
 *   This file is part of Lux Renderer.                                    *
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
 *   Lux Renderer website : http://www.luxrender.org                       *
 ***************************************************************************/

// uv.cpp*
#include "lux.h"
#include "texture.h"
#include "paramset.h"

namespace lux
{

// UVTexture Declarations
class UVTexture : public Texture<Spectrum> {
public:
	// UVTexture Public Methods
	UVTexture(TextureMapping2D *m) {
		mapping = m;
	}
	~UVTexture() {
		delete mapping;
	}
	Spectrum Evaluate(const DifferentialGeometry &dg) const {
		float s, t, dsdx, dtdx, dsdy, dtdy;
		mapping->Map(dg, &s, &t, &dsdx, &dtdx, &dsdy, &dtdy);
		float cs[COLOR_SAMPLES];
		memset(cs, 0, COLOR_SAMPLES * sizeof(float));
		cs[0] = s - Floor2Int(s);
		cs[1] = t - Floor2Int(t);
		return Spectrum(cs);
	}
	
	static Texture<float> * CreateFloatTexture(const Transform &tex2world, const TextureParams &tp);
	static Texture<Spectrum> * CreateSpectrumTexture(const Transform &tex2world, const TextureParams &tp);
private:
	TextureMapping2D *mapping;
};

}//namespace lux

