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

// brick.cpp*
#include "lux.h"
#include "texture.h"
#include "paramset.h"
#include "error.h"

namespace lux {

// BrickTexture3D Declarations
template <class T>
class BrickTexture3D : public Texture<T> {
public:
    // BrickTexture3D Public Methods

    virtual ~BrickTexture3D() {
        delete mapping;
    }

    BrickTexture3D(
	        boost::shared_ptr<Texture<T> > c1,
			boost::shared_ptr<Texture<T> > c2,
            float brickw,
			float brickh,
			float brickd,
			float mortar,
            TextureMapping3D *map) : mapping(map) {
		brickwidth = brickw;
		brickheight = brickh;
		brickdepth = brickd;
		mortarsize = mortar;
		tex1 = c1;
		tex2 = c2;
    }

#define BRICK_EPSILON 1e-3f

    virtual T Evaluate(const TsPack *tspack, const DifferentialGeometry &dg) const {
        Vector dpdx, dpdy;
        Point P = mapping->Map(dg, &dpdx, &dpdy);

		const float offs = BRICK_EPSILON + mortarsize;
		Point bP = P + Point(offs, offs, offs);

		// Check top
		float bz = bP.z / brickheight;
		int ibz = (int) bz; bz -= ibz;
		if (bz < .0f) bz += 1.f;
		const float mortarheight = mortarsize / brickheight;
		if (bz <= mortarheight)
			return tex2->Evaluate(tspack, dg);
		bz = (bP.z / brickheight) * .5f;
		ibz = (int) bz; bz -= ibz;
		if (bz < .0f) bz += 1.f;

		// Check odd sides
		float bx = bP.x / brickwidth;
		int ibx = (int) bx; bx -= ibx;
		if (bx < .0f) bx += 1.f;
		const float mortarwidth  = mortarsize / brickwidth;
		if ((bx <= mortarwidth) && (bz <= .5f))
			return tex2->Evaluate(tspack, dg);

		// Check even sides
		bx = (bP.x / brickwidth) + .5f;
		ibx = (int) bx; bx -= ibx;
		if (bx < .0f) bx += 1.f;
		if ((bx <= mortarwidth) && (bz > .5f))
			return tex2->Evaluate(tspack, dg);

		// Check odd fronts
		float by = bP.y / brickdepth;
		int iby = (int) by; by -= iby;
		if (by < .0f) by += 1.f;
		const float mortardepth  = mortarsize / brickdepth;
		if ((by <= mortardepth) && (bz > .5f))
			return tex2->Evaluate(tspack, dg);

		// Check even fronts
		by = (bP.y / brickdepth) + .5f;
		iby = (int) by; by -= iby;
		if (by < .0f) by += 1.f;
		if ((by <= mortardepth) && (bz <= .5f))
			return tex2->Evaluate(tspack, dg);

		// Inside brick
		return tex1->Evaluate(tspack, dg);
    }
	virtual float Y() const {
		const float m = powf(Clamp(1.f - mortarsize, 0.f, 1.f), 3);
		return Lerp(m, tex2->Y(), tex1->Y());
	}
	virtual float Filter() const {
		const float m = powf(Clamp(1.f - mortarsize, 0.f, 1.f), 3);
		return Lerp(m, tex2->Filter(), tex1->Filter());
	}
	virtual void SetIlluminant() {
		// Update sub-textures
		tex1->SetIlluminant();
		tex2->SetIlluminant();
	}
    static Texture<float> *CreateFloatTexture(const Transform &tex2world, const TextureParams &tp);
	static Texture<SWCSpectrum> *CreateSWCSpectrumTexture(const Transform &tex2world, const TextureParams &tp);
private:
    // BrickTexture3D Private Data
	float brickwidth, brickheight, brickdepth, mortarsize;
    TextureMapping3D *mapping;
	boost::shared_ptr<Texture<T> > tex1, tex2;
};

template <class T> Texture<float> *BrickTexture3D<T>::CreateFloatTexture(
        const Transform &tex2world,
        const TextureParams &tp) {
    // Initialize 3D texture mapping _map_ from _tp_
    TextureMapping3D *map = new IdentityMapping3D(tex2world);
	// Apply texture specified transformation option for 3D mapping
	IdentityMapping3D *imap = (IdentityMapping3D*) map;
	imap->Apply3DTextureMappingOptions(tp);

	boost::shared_ptr<Texture<float> > tex1 = tp.GetFloatTexture("bricktex", 1.f);
	boost::shared_ptr<Texture<float> > tex2 = tp.GetFloatTexture("mortartex", 0.2f);

	float bw = tp.FindFloat("brickwidth", 0.3f);
	float bh = tp.FindFloat("brickheight", 0.1f);
	float bd = tp.FindFloat("brickdepth", 0.15f);

	float m = tp.FindFloat("mortarsize", 0.01f);

    return new BrickTexture3D<float>(tex1, tex2, bw, bh, bd, m, map);
}

template <class T> Texture<SWCSpectrum> *BrickTexture3D<T>::CreateSWCSpectrumTexture(
        const Transform &tex2world,
        const TextureParams &tp) {
    // Initialize 3D texture mapping _map_ from _tp_
    TextureMapping3D *map = new IdentityMapping3D(tex2world);
	// Apply texture specified transformation option for 3D mapping
	IdentityMapping3D *imap = (IdentityMapping3D*) map;
	imap->Apply3DTextureMappingOptions(tp);

	boost::shared_ptr<Texture<SWCSpectrum> > tex1 = tp.GetSWCSpectrumTexture("bricktex", 1.f);
	boost::shared_ptr<Texture<SWCSpectrum> > tex2 = tp.GetSWCSpectrumTexture("mortartex", 0.2f);

	float bw = tp.FindFloat("brickwidth", 0.3f);
	float bh = tp.FindFloat("brickheight", 0.1f);
	float bd = tp.FindFloat("brickdepth", 0.15f);

	float m = tp.FindFloat("mortarsize", 0.01f);

    return new BrickTexture3D<SWCSpectrum>(tex1, tex2, bw, bh, bd, m, map);
}

} // namespace lux
