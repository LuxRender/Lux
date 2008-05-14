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
#include "blender_musgrave.h"
#include "error.h"

#include <sstream>

using namespace lux;
using namespace blender;

// Dade - BlenderMusgraveTexture3D Method Definitions

Texture<float> *BlenderMusgraveTexture3D::CreateFloatTexture(
        const Transform &tex2world,
        const TextureParams &tp) {
    // Initialize 3D texture mapping _map_ from _tp_
    TextureMapping3D *map = new IdentityMapping3D(tex2world);
	// Apply texture specified transformation option for 3D mapping
	IdentityMapping3D *imap = (IdentityMapping3D*) map;
	imap->Apply3DTextureMappingOptions(tp);

    // Dade - decode the noise type
    short type = TEX_MFRACTAL;
    string noiseType = tp.FindString("type");
    if ((noiseType == "multifractal") || (noiseType == ""))
        type = TEX_MFRACTAL;
    else if (noiseType == "ridged_multifractal")
        type = TEX_RIDGEDMF;
    else if (noiseType == "hybrid_multifractal")
        type = TEX_HYBRIDMF;
    else if (noiseType == "hetero_terrain")
        type = TEX_HTERRAIN;
    else if (noiseType == "fbm")
        type = TEX_FBM;
    else {
        std::stringstream ss;
        ss << "Unknown noise type '" << noiseType << "'";
        luxError(LUX_BADTOKEN, LUX_ERROR, ss.str().c_str());
    }

    // Dade - decode the noise basis
    short basis = TEX_BLENDER;
    string noiseBasis = tp.FindString("noisebasis");
    if ((noiseBasis == "blender_original") || (noiseBasis == ""))
        basis = TEX_BLENDER;
    else if (noiseBasis == "original_perlin")
        basis = TEX_STDPERLIN;
    else if (noiseBasis == "improved_perlin")
        basis = TEX_NEWPERLIN;
    else if (noiseBasis == "voronoi_f1")
        basis = TEX_VORONOI_F1;
    else if (noiseBasis == "voronoi_f2")
        basis = TEX_VORONOI_F2;
    else if (noiseBasis == "voronoi_f3")
        basis = TEX_VORONOI_F3;
    else if (noiseBasis == "voronoi_f4")
        basis = TEX_VORONOI_F4;
    else if (noiseBasis == "voronoi_f2f1")
        basis = TEX_VORONOI_F2F1;
    else if (noiseBasis == "voronoi_crackle")
        basis = TEX_VORONOI_CRACKLE;
    else if (noiseBasis == "cell_noise")
        basis = TEX_CELLNOISE;
    else {
        std::stringstream ss;
        ss << "Unknown noise basis '" << noiseBasis << "'";
        luxError(LUX_BADTOKEN, LUX_ERROR, ss.str().c_str());
    }

    return new BlenderMusgraveTexture3D(
            tp.FindFloat("h", 1.0f),
            tp.FindFloat("lacu", 2.0f),
            tp.FindFloat("octs", 2.0f),
            tp.FindFloat("gain", 1.0f),
            tp.FindFloat("offset", 1.0f),
            tp.FindFloat("noisesize", 0.250f),
            tp.FindFloat("outscale", 1.0f),
            type,
            basis,
            tp.FindFloat("bright", 1.0f),
            tp.FindFloat("contrast", 1.0f),
            map);
}
