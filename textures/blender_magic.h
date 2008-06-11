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
#include "blender_texlib.h"

namespace lux {
template <class T>
class BlenderMagicTexture3D : public Texture<T> {
public:
    // BlenderMagicTexture3D Public Methods

    ~BlenderMagicTexture3D() {
        delete mapping;
    }

    BlenderMagicTexture3D(
	        boost::shared_ptr<Texture<T> > c1,
			boost::shared_ptr<Texture<T> > c2,
            short noiseDepth,
			float turbul,
            float bright,
            float contrast,
            TextureMapping3D *map) : mapping(map) {
        tex.type = TEX_MAGIC;

		tex.noisedepth = noiseDepth;
        tex.turbul = turbul;
        tex.bright = bright;
        tex.contrast = contrast;
		tex.rfac = 1.0f;
		tex.gfac = 1.0f;
		tex.bfac = 1.0f;

		tex1 = c1;
		tex2 = c2;
    }

    T Evaluate(const DifferentialGeometry &dg) const {
        Vector dpdx, dpdy;
        Point P = mapping->Map(dg, &dpdx, &dpdy);

        blender::TexResult texres;
        multitex(&tex, &P.x, &texres);

		T t1 = tex1->Evaluate(dg), t2 = tex2->Evaluate(dg);
		return (1.f - texres.tin) * t1 + texres.tin * t2;
    }

    static Texture<float> *CreateFloatTexture(const Transform &tex2world, const TextureParams &tp);
	static Texture<Spectrum> *CreateSpectrumTexture(const Transform &tex2world, const TextureParams &tp);
private:
    // BlenderMagicTexture3D Private Data

    TextureMapping3D *mapping;
	boost::shared_ptr<Texture<T> > tex1, tex2;
    blender::Tex tex;
};

template <class T> Texture<float> *BlenderMagicTexture3D<T>::CreateFloatTexture(
        const Transform &tex2world,
        const TextureParams &tp) {
    // Initialize 3D texture mapping _map_ from _tp_
    TextureMapping3D *map = new IdentityMapping3D(tex2world);
	// Apply texture specified transformation option for 3D mapping
	IdentityMapping3D *imap = (IdentityMapping3D*) map;
	imap->Apply3DTextureMappingOptions(tp);

	boost::shared_ptr<Texture<float> > tex1 = tp.GetFloatTexture("tex1", 1.f);
	boost::shared_ptr<Texture<float> > tex2 = tp.GetFloatTexture("tex2", 0.f);

    return new BlenderMagicTexture3D<float>(
			tex1,
			tex2,
            (short)tp.FindInt("noisedepth", 2),
			tp.FindFloat("turbulance", 5.0f),
            tp.FindFloat("bright", 1.0f),
            tp.FindFloat("contrast", 1.0f),
            map);
}

template <class T> Texture<Spectrum> *BlenderMagicTexture3D<T>::CreateSpectrumTexture(
        const Transform &tex2world,
        const TextureParams &tp) {
    // Initialize 3D texture mapping _map_ from _tp_
    TextureMapping3D *map = new IdentityMapping3D(tex2world);
	// Apply texture specified transformation option for 3D mapping
	IdentityMapping3D *imap = (IdentityMapping3D*) map;
	imap->Apply3DTextureMappingOptions(tp);

	boost::shared_ptr<Texture<Spectrum> > tex1 = tp.GetSpectrumTexture("tex1", 1.f);
	boost::shared_ptr<Texture<Spectrum> > tex2 = tp.GetSpectrumTexture("tex2", 0.f);

    return new BlenderMagicTexture3D<Spectrum>(
			tex1,
			tex2,
            (short)tp.FindInt("noisedepth", 2),
			tp.FindFloat("turbulance", 5.0f),
            tp.FindFloat("bright", 1.0f),
            tp.FindFloat("contrast", 1.0f),
            map);
}

} // namespace lux
