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

// blender_wood.cpp*
#include "blender_wood.h"
#include "error.h"

#include <sstream>

using namespace lux;
using namespace blender;

// Dade - BlenderWoodTexture3D Method Definitions

Texture<float> *BlenderWoodTexture3D::CreateFloatTexture(
        const Transform &tex2world,
        const TextureParams &tp) {
    // Initialize 3D texture mapping _map_ from _tp_
    TextureMapping3D *map = new IdentityMapping3D(tex2world);
	// Apply texture specified transformation option for 3D mapping
	IdentityMapping3D *imap = (IdentityMapping3D*) map;
	imap->Apply3DTextureMappingOptions(tp);

    // Dade - decode the noise type
    short type = TEX_BAND;
    string stype = tp.FindString("type");
    if ((stype == "bands") || (stype == ""))
        type = TEX_BAND;
    else if (stype == "rings")
        type = TEX_RING;
    else if (stype == "bandnoise")
        type = TEX_BANDNOISE;
    else if (stype == "ringnoise")
        type = TEX_RINGNOISE;
    else {
        std::stringstream ss;
        ss << "Unknown noise type '" << stype << "'";
        luxError(LUX_BADTOKEN, LUX_ERROR, ss.str().c_str());
    }

    // Dade - decode the noise type
    short ntype = TEX_NOISEPERL;
    string noiseType = tp.FindString("noisetype");
    if ((noiseType == "soft_noise") || (noiseType == ""))
        ntype = TEX_NOISESOFT;
    else if (noiseType == "hard_noise")
        ntype = TEX_NOISEPERL;
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

    // Dade - decode the noise basis
    short basis2 = TEX_SIN;
    string noiseBasis2 = tp.FindString("noisebasis2");
    if ((noiseBasis2 == "sin") || (noiseBasis2 == ""))
        basis2 = TEX_SIN;
    else if (noiseBasis2 == "saw")
        basis2 = TEX_SAW;
    else if (noiseBasis2 == "tri")
        basis2 = TEX_TRI;
    else {
        std::stringstream ss;
        ss << "Unknown noise basis2 '" << noiseBasis2 << "'";
        luxError(LUX_BADTOKEN, LUX_ERROR, ss.str().c_str());
    }

    return new BlenderWoodTexture3D(
            tp.FindFloat("noisesize", 0.250f),
            ntype,
            tp.FindFloat("turbulance", 5.0f),
            type,
            basis2,
            basis,
            tp.FindFloat("bright", 1.0f),
            tp.FindFloat("contrast", 1.0f),
            map);
}
