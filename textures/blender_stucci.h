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
#include "paramset.h"
#include "error.h"
#include "blender_texlib.h"

namespace lux {
template <class T>
// BlenderStucciTexture3D Declarations
class BlenderStucciTexture3D : public Texture<T> {
public:
    // BlenderStucciTexture3D Public Methods

    ~BlenderStucciTexture3D() {
        delete mapping;
    }

    BlenderStucciTexture3D(
	        boost::shared_ptr<Texture<T> > c1,
			boost::shared_ptr<Texture<T> > c2,
            float noiseSize,
            short noiseType,
			float turbul,
            short sType,
            short noiseBasis,
            float bright,
            float contrast,
            TextureMapping3D *map) : mapping(map) {
        tex.type = TEX_STUCCI;

        tex.noisesize = noiseSize;
        tex.noisetype = noiseType;
		tex.turbul = turbul;
        tex.stype = sType;
        tex.noisebasis = noiseBasis;
        tex.bright = bright;
        tex.contrast = contrast;
		tex1 = c1;
		tex2 = c2;
    }

    T Evaluate(const DifferentialGeometry &dg) const {
        Vector dpdx, dpdy;
        Point P = mapping->Map(dg, &dpdx, &dpdy);

        blender::TexResult texres;
        int resultType = multitex(&tex, &P.x, &texres);

        if(resultType & TEX_RGB)
            texres.tin = (0.35 * texres.tr + 0.45 * texres.tg
                    + 0.2 * texres.tb);
        else
            texres.tr = texres.tg = texres.tb = texres.tin;

		T t1 = tex1->Evaluate(dg), t2 = tex2->Evaluate(dg);
		return (1.f - texres.tin) * t1 + texres.tin * t2;
    }

    static Texture<float> *CreateFloatTexture(const Transform &tex2world, const TextureParams &tp);
	static Texture<RGBColor> *CreateRGBColorTexture(const Transform &tex2world, const TextureParams &tp);
private:
    // BlenderStucciTexture3D Private Data

    TextureMapping3D *mapping;
	boost::shared_ptr<Texture<T> > tex1, tex2;
    blender::Tex tex;
};

template <class T> Texture<float> *BlenderStucciTexture3D<T>::CreateFloatTexture(
        const Transform &tex2world,
        const TextureParams &tp) {
    // Initialize 3D texture mapping _map_ from _tp_
    TextureMapping3D *map = new IdentityMapping3D(tex2world);
	// Apply texture specified transformation option for 3D mapping
	IdentityMapping3D *imap = (IdentityMapping3D*) map;
	imap->Apply3DTextureMappingOptions(tp);

	boost::shared_ptr<Texture<float> > tex1 = tp.GetFloatTexture("tex1", 1.f);
	boost::shared_ptr<Texture<float> > tex2 = tp.GetFloatTexture("tex2", 0.f);

	// Decode the noise type
    short type = TEX_PLASTIC;
    string stype = tp.FindString("type");
    if ((stype == "Plastic") || (stype == ""))
        type = TEX_PLASTIC;
    else if (stype == "Wall In")
        type = TEX_WALLIN;
	else if (stype == "Wall Out")
        type = TEX_WALLOUT;
    else {
        std::stringstream ss;
        ss << "Unknown noise type '" << stype << "'";
        luxError(LUX_BADTOKEN, LUX_ERROR, ss.str().c_str());
    }

	// Decode the noise type
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

    return new BlenderStucciTexture3D<float>(
			tex1,
			tex2,
            tp.FindFloat("noisesize", 0.250f),
			ntype,
			tp.FindFloat("turbulance", 5.0f),
			type,
			basis,
            tp.FindFloat("bright", 1.0f),
            tp.FindFloat("contrast", 1.0f),
            map);
}

template <class T> Texture<RGBColor> *BlenderStucciTexture3D<T>::CreateRGBColorTexture(
        const Transform &tex2world,
        const TextureParams &tp) {
    // Initialize 3D texture mapping _map_ from _tp_
    TextureMapping3D *map = new IdentityMapping3D(tex2world);
	// Apply texture specified transformation option for 3D mapping
	IdentityMapping3D *imap = (IdentityMapping3D*) map;
	imap->Apply3DTextureMappingOptions(tp);

	boost::shared_ptr<Texture<RGBColor> > tex1 = tp.GetRGBColorTexture("tex1", 1.f);
	boost::shared_ptr<Texture<RGBColor> > tex2 = tp.GetRGBColorTexture("tex2", 0.f);

	// Decode the noise type
    short type = TEX_PLASTIC;
    string stype = tp.FindString("type");
    if ((stype == "Plastic") || (stype == ""))
        type = TEX_PLASTIC;
    else if (stype == "Wall In")
        type = TEX_WALLIN;
	else if (stype == "Wall Out")
        type = TEX_WALLOUT;
    else {
        std::stringstream ss;
        ss << "Unknown noise type '" << stype << "'";
        luxError(LUX_BADTOKEN, LUX_ERROR, ss.str().c_str());
    }

	// Decode the noise type
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

    return new BlenderStucciTexture3D<RGBColor>(
			tex1,
			tex2,
            tp.FindFloat("noisesize", 0.250f),
			ntype,
			tp.FindFloat("turbulance", 5.0f),
			type,
			basis,
            tp.FindFloat("bright", 1.0f),
            tp.FindFloat("contrast", 1.0f),
            map);
}

} // namespace lux
