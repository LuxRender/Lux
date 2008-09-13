/***************************************************************************
 *   Copyright (C) 1998-2008 by authors (see AUTHORS.txt )                 *
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

#include "lux.h"
#include "texture.h"
#include "color.h"
#include "sampling.h"
#include "paramset.h"
#include "primitive.h"

namespace lux
{

// Dade - must be power of 2
#define HARLEQUIN_TEXTURE_PALETTE_SIZE 0x1f

// Harlequin Declarations
class HarlequinTexture : public Texture<RGBColor> {
public:
	// Harlequin Public Methods
	HarlequinTexture() {
		float c[3];
		for (int i = 0; i < HARLEQUIN_TEXTURE_PALETTE_SIZE; i++) {
			c[0] = RadicalInverse(i * COLOR_SAMPLES + 1, 2);
			c[1] = RadicalInverse(i * COLOR_SAMPLES + 1, 3);
			c[2] = RadicalInverse(i * COLOR_SAMPLES + 1, 5);

			ColorLookupTable[i] = RGBColor(c);
		}
	}
	~HarlequinTexture() { }

	RGBColor Evaluate(const DifferentialGeometry &dg) const {
		// Dade - I assume object are 8 bytes aligned
		u_long lookupIndex = (((u_long)dg.prim) &
				((HARLEQUIN_TEXTURE_PALETTE_SIZE-1) << 3)) >> 3;

		return ColorLookupTable[lookupIndex];
	}

	static Texture<float> *CreateFloatTexture(const Transform &tex2world, const TextureParams &tp);
	static Texture<RGBColor> *CreateRGBColorTexture(const Transform &tex2world, const TextureParams &tp);

private:
	static RGBColor ColorLookupTable[];
};

RGBColor HarlequinTexture::ColorLookupTable[HARLEQUIN_TEXTURE_PALETTE_SIZE];

// Harlequin Method Definitions
Texture<float> *HarlequinTexture::CreateFloatTexture(const Transform &tex2world,
		const TextureParams &tp) {
	return NULL;
}

Texture<RGBColor> *HarlequinTexture::CreateRGBColorTexture(const Transform &tex2world,
		const TextureParams &tp) {
	return new HarlequinTexture();
}

}//namespace lux
