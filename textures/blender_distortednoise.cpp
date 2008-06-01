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

// blender_distortednoise.cpp*
#include "blender_distortednoise.h"
#include "error.h"

#include <sstream>

using namespace lux;
using namespace blender;

/*
Name: SMISRA
File: blender_disortednoise.cpp*
Description: Implements the Blender disorted noise Texture
Modification Date: 
*/

Texture<float> *BlenderDistortedNoiseTexture3D::CreateFloatTexture(
        const Transform &tex2world,
        const TextureParams &tp) {
    // Initialize 3D texture mapping _map_ from _tp_
    TextureMapping3D *map = new IdentityMapping3D(tex2world);
	// Apply texture specified transformation option for 3D mapping
	IdentityMapping3D *imap = (IdentityMapping3D*) map;
	imap->Apply3DTextureMappingOptions(tp);

	// Decode the noise type
	short type = TEX_BLENDER;
	string stype = tp.FindString("type");
    if ((stype == "blender_original") || (stype == ""))
        type = TEX_BLENDER;
    else if (stype == "original_perlin")
        type = TEX_STDPERLIN;
    else if (stype == "improved_perlin")
        type = TEX_NEWPERLIN;
    else if (stype == "voronoi_f1")
        type = TEX_VORONOI_F1;
    else if (stype == "voronoi_f2")
        type = TEX_VORONOI_F2;
    else if (stype == "voronoi_f3")
        type = TEX_VORONOI_F3;
    else if (stype == "voronoi_f4")
        type = TEX_VORONOI_F4;
    else if (stype == "voronoi_f2f1")
        type = TEX_VORONOI_F2F1;
    else if (stype == "voronoi_crackle")
        type = TEX_VORONOI_CRACKLE;
    else if (stype == "cell_noise")
        type = TEX_CELLNOISE;
    else {
        std::stringstream ss;
        ss << "Unknown noise type '" << type << "'";
        luxError(LUX_BADTOKEN, LUX_ERROR, ss.str().c_str());
    }

	// Decode the noise basis
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

    return new BlenderDistortedNoiseTexture3D(
			type,
			basis,
			tp.FindFloat("noisesize", 0.250f),
			tp.FindFloat("distamount", 1.0f),
			tp.FindFloat("nabla", 0.025f),
            tp.FindFloat("bright", 1.0f),
            tp.FindFloat("contrast", 1.0f),
            map);
}
