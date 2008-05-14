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

// blender_musgrave.cpp*
#include "lux.h"
#include "texture.h"
#include "paramset.h"
#include "blender_texlib.h"

namespace lux {

// Dade - BlenderMusgraveTexture3D Declarations
class BlenderMusgraveTexture3D : public Texture<float> {
public:
    // BlenderMusgraveTexture3D Public Methods

    ~BlenderMusgraveTexture3D() {
        delete mapping;
    }

    BlenderMusgraveTexture3D(
            float mg_H,
            float mg_lacunarity,
            float mg_octaves,
            float mg_gain,
            float mg_offset,
            float noiseSize,
            float ns_outscale,
            short sType,
            short noiseBasis,
            float bright,
            float contrast,
            TextureMapping3D *map) : mapping(map) {
        tex.type = TEX_MUSGRAVE;

        tex.mg_H = mg_H;
        tex.mg_lacunarity = mg_lacunarity;
        tex.mg_octaves = mg_octaves;
        tex.mg_gain = mg_gain;
        tex.mg_offset = mg_offset;

        tex.noisesize = noiseSize;
        tex.ns_outscale = ns_outscale;
        tex.stype = sType;
        tex.noisebasis = noiseBasis;

        tex.bright = bright;
        tex.contrast = contrast;
    }

    float Evaluate(const DifferentialGeometry &dg) const {
        Vector dpdx, dpdy;
        Point P = mapping->Map(dg, &dpdx, &dpdy);

        blender::TexResult texres;
        int resultType = multitex(&tex, &P.x, &texres);

        if(resultType & TEX_RGB)
            texres.tin = (0.35 * texres.tr + 0.45 * texres.tg
                    + 0.2 * texres.tb);
        else
            texres.tr = texres.tg = texres.tb = texres.tin;

        return texres.tin;
    }

    static Texture<float> *CreateFloatTexture(const Transform &tex2world, const TextureParams &tp);
    
private:
    // BlenderMusgraveTexture3D Private Data

    TextureMapping3D *mapping;

    blender::Tex tex;
};

} // namespace lux
