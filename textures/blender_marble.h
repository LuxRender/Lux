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

// Dade - BlenderMarbleTexture3D Declarations

class BlenderMarbleTexture3D : public Texture<float> {
public:
    // BlenderMarbleTexture3D Public Methods

    ~BlenderMarbleTexture3D() {
        delete mapping;
    }

    BlenderMarbleTexture3D(
            float noiseSize,
            short noiseType,
            short noiseDepth,
            float turbul,
            short sType,
            short noiseBasis2,
            short noiseBasis,
            float bright,
            float contrast,
            TextureMapping3D *map) : mapping(map) {
        tex.type = TEX_MARBLE;

        tex.noisesize = noiseSize;
        tex.noisetype = noiseType;
        tex.noisedepth = noiseDepth;
        tex.turbul = turbul;
        tex.stype = sType;
        tex.noisebasis = noiseBasis;
        tex.noisebasis2 = noiseBasis2;
        tex.bright = bright;
        tex.contrast = contrast;
    }

    float Evaluate(const DifferentialGeometry &dg) const {
        Vector dpdx, dpdy;
        Point P = mapping->Map(dg, &dpdx, &dpdy);

        blender::TexResult texres;
        multitex(&tex, &P.x, &texres);

        return texres.tin;
    }

    static Texture<float> *CreateFloatTexture(const Transform &tex2world, const TextureParams &tp);
private:
    // BlenderMarbleTexture3D Private Data

    TextureMapping3D *mapping;

    blender::Tex tex;
};

} // namespace lux
