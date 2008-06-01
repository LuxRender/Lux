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

#include "blender_blend.h"
#include "error.h"

#include <sstream>

using namespace lux;
using namespace blender;

Texture<float> *BlenderBlendTexture3D::CreateFloatTexture(
        const Transform &tex2world,
        const TextureParams &tp) {
    // Initialize 3D texture mapping _map_ from _tp_
    TextureMapping3D *map = new IdentityMapping3D(tex2world);
	// Apply texture specified transformation option for 3D mapping
	IdentityMapping3D *imap = (IdentityMapping3D*) map;
	imap->Apply3DTextureMappingOptions(tp);

    // Decode the noise type
	short type = TEX_LIN;
    string stype = tp.FindString("type");
    if ((stype == "lin") || (stype == ""))
        type = TEX_LIN;
    else if (stype == "quad")
        type = TEX_QUAD;
    else if (stype == "ease")
        type = TEX_EASE;
    else if (stype == "diag")
        type = TEX_DIAG;
	else if (stype == "sphere")
        type = TEX_SPHERE;
	else if (stype == "halo")
        type = TEX_HALO;
	else if (stype == "radial")
        type = TEX_RAD;
    else {
        std::stringstream ss;
        ss << "Unknown noise type '" << type << "'";
        luxError(LUX_BADTOKEN, LUX_ERROR, ss.str().c_str());
    }
	
	//SMISRA - flag parameter
	short flag = !TEX_FLIPBLEND;
	string sflag = tp.FindString("flag");
//	if(sflag == "")
//		;//No action needed so empty
	if (sflag == "flip xy")
        flag = TEX_FLIPBLEND;
	else
	{ 
		std::stringstream ss;
        ss << "Unknown flag type '" << flag << "'";
        luxError(LUX_BADTOKEN, LUX_ERROR, ss.str().c_str());
    }

    return new BlenderBlendTexture3D(
            type,
            flag,
            tp.FindFloat("bright", 1.0f),
            tp.FindFloat("contrast", 1.0f),
            map);
}
