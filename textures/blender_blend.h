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

#include "lux.h"
#include "texture.h"
#include "color.h"
#include "paramset.h"
#include "error.h"
#include "blender_texlib.h"

namespace lux {

template <class T>
class BlenderBlendTexture3D : public Texture<T> {
public:
    // BlenderBlendTexture3D Public Methods

    virtual ~BlenderBlendTexture3D() {
        delete mapping;
    }

    BlenderBlendTexture3D(
	        boost::shared_ptr<Texture<T> > c1,
			boost::shared_ptr<Texture<T> > c2,
            short sType,
            short flag,
            float bright,
            float contrast,
            TextureMapping3D *map) : mapping(map) {
        tex.type = TEX_BLEND;

        tex.stype = sType;
		tex.flag = flag;
        tex.bright = bright;
        tex.contrast = contrast;
		tex1 = c1;
		tex2 = c2;
    }

    virtual T Evaluate(const TsPack *tspack, const DifferentialGeometry &dg) const {
        Vector dpdx, dpdy;
        Point P = mapping->Map(dg, &dpdx, &dpdy);

        blender::TexResult texres;
        int resultType = multitex(&tex, &P.x, &texres);

        if(resultType & TEX_RGB)
            texres.tin = (0.35f * texres.tr + 0.45f * texres.tg
                    + 0.2f * texres.tb);
        else
            texres.tr = texres.tg = texres.tb = texres.tin;

		T t1 = tex1->Evaluate(tspack, dg), t2 = tex2->Evaluate(tspack, dg);
		return (1.f - texres.tin) * t1 + texres.tin * t2;
    }
	virtual float Y() const { return (tex1->Y() + tex2->Y()) / 2.f; }
	virtual float Filter() const { return (tex1->Filter() + tex2->Filter()) / 2.f; }
	virtual void SetIlluminant() {
		// Update sub-textures
		tex1->SetIlluminant();
		tex2->SetIlluminant();
	}
    static Texture<float> *CreateFloatTexture(const Transform &tex2world, const TextureParams &tp);
	static Texture<SWCSpectrum> *CreateSWCSpectrumTexture(const Transform &tex2world, const TextureParams &tp);
private:
    // BlenderBlendTexture3D Private Data

    TextureMapping3D *mapping;
	boost::shared_ptr<Texture<T> > tex1, tex2;
    blender::Tex tex;
};

template <class T> Texture<float> *BlenderBlendTexture3D<T>::CreateFloatTexture(
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

	short flag = !TEX_FLIPBLEND;
	bool sflag = tp.FindBool("flipxy", false);
	if(sflag == true)
		flag = TEX_FLIPBLEND;

    return new BlenderBlendTexture3D<float>(
			tex1,
			tex2,
            type,
            flag,
            tp.FindFloat("bright", 1.0f),
            tp.FindFloat("contrast", 1.0f),
            map);
}

template <class T> Texture<SWCSpectrum> *BlenderBlendTexture3D<T>::CreateSWCSpectrumTexture(
        const Transform &tex2world,
        const TextureParams &tp) {
    // Initialize 3D texture mapping _map_ from _tp_
    TextureMapping3D *map = new IdentityMapping3D(tex2world);
	// Apply texture specified transformation option for 3D mapping
	IdentityMapping3D *imap = (IdentityMapping3D*) map;
	imap->Apply3DTextureMappingOptions(tp);

	boost::shared_ptr<Texture<SWCSpectrum> > tex1 = tp.GetSWCSpectrumTexture("tex1", RGBColor(1.f));
	boost::shared_ptr<Texture<SWCSpectrum> > tex2 = tp.GetSWCSpectrumTexture("tex2", RGBColor(0.f));

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

	short flag = !TEX_FLIPBLEND;
	bool sflag = tp.FindBool("flipxy", false);
	if(sflag == true)
		flag = TEX_FLIPBLEND;

    return new BlenderBlendTexture3D<SWCSpectrum>(
			tex1,
			tex2,
            type,
            flag,
            tp.FindFloat("bright", 1.0f),
            tp.FindFloat("contrast", 1.0f),
            map);
}

} // namespace lux
