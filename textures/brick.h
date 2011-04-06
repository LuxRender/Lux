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
#include "spectrum.h"
#include "texture.h"
#include "color.h"
#include "paramset.h"
#include "error.h"
#include "geometry/raydifferential.h"

namespace lux {

typedef enum { FLEMISH, RUNNING, ENGLISH, HERRINGBONE, BASKET, KETTING } MasonryBond;

// BrickTexture3D Declarations
template <class T> class BrickTexture3D : public Texture<T> {
public:
	// BrickTexture3D Public Methods
	BrickTexture3D(boost::shared_ptr<Texture<T> > &c1,
		boost::shared_ptr<Texture<T> > &c2,
		boost::shared_ptr<Texture<T> > &c3,
		float brickw, float brickh, float brickd, float mortar,
		float r, float bev, const string &b,
		TextureMapping3D *map) : brickwidth(brickw),
		brickheight(brickh), brickdepth(brickd), mortarsize(mortar),
		run(r), mapping(map), tex1(c1), tex2(c2), tex3(c3) {
		if (b == "stacked") {
			bond = RUNNING;
			run = 0.f;
		} else if (b == "flemish")
			bond = FLEMISH;
		else if (b == "english") {
			bond = ENGLISH;
			run = 0.25f;
		} else if (b == "herringbone")
			bond = HERRINGBONE;
		else if (b == "basket")
			bond = BASKET;
		else if (b == "chain link") {
			bond = KETTING;
			run = 1.25f;
			offset = Point(.25f,-1.f,0);
		} else {
			bond = RUNNING;
			offset = Point(0,-0.5f,0);
		}
		if (bond == HERRINGBONE || bond == BASKET) {
			proportion = floorf(brickwidth / brickheight);
			brickdepth = brickheight = brickwidth;
			invproportion = 1.f / proportion;
		}
		mortarwidth = mortarsize / brickwidth;
		mortarheight = mortarsize / brickheight;
		mortardepth = mortarsize / brickdepth;
		bevelwidth = bev / brickwidth;
		bevelheight = bev / brickheight;
		beveldepth = bev / brickdepth;
		usebevel = bev > 0.f;
	}
	virtual ~BrickTexture3D() { delete mapping; }

#define BRICK_EPSILON 1e-3f

	virtual T Evaluate(const SpectrumWavelengths &sw,
		const DifferentialGeometry &dg) const {
		const Point P(mapping->Map(dg));

		const float offs = BRICK_EPSILON + mortarsize;
		Point bP(P + Point(offs, offs, offs));

		// Normalize coordinates according brick dimensions
		bP.x /= brickwidth;
		bP.y /= brickdepth;
		bP.z /= brickheight;

		bP += offset;

		Point brickIndex;
		Point bevel;
		bool b;
		switch (bond) {
			case FLEMISH:
				b = RunningAlternate(bP, brickIndex, bevel, 1);
				break;
			case RUNNING:
				b = Running(bP, brickIndex, bevel);
				break;
			case ENGLISH:
				b = English(bP, brickIndex, bevel);
				break;
			case HERRINGBONE:
				b = Herringbone(bP, brickIndex);
				break;
			case BASKET:
				b = Basket(bP, brickIndex);
				break;
			case KETTING:
				b = RunningAlternate(bP, brickIndex, bevel, 2);
				break; 
			default:
				b = true;
				break;
		}
		// Bevels, lets skip that for now
/*		if (usebevel) {
			const float bevelx = 1.f - 2.f / bevelwidth *
				min(bevel.x, 1.f - bevel.x);
			const float bevely = 1.f - 2.f / beveldepth *
				min(bevel.y, 1.f - bevel.y);
			const float bevelz = 1.f - 2.f / bevelheight *
				min(bevel.z, 1.f - bevel.z);
			const float amount = max(bevelz, max(bevelx, bevely));
			
			return Lerp(amount, tex1->Evaluate(tspack, dg),
				tex2->Evaluate(tspack, dg);
		} else */
		if (b) {
			// DifferentialGeometry for brick
			DifferentialGeometry dgb = dg;
			dgb.p = brickIndex;
			// Create seams between brick texture, lets skip that for now
/*			const float o = tex4->Evaluate(tspack,dgb);
			DifferentialGeometry dg2 = dg;
			dg2.p += Vector(o, o, o);*/
			// Brick texture * Modulation texture
			return tex1->Evaluate(sw, dg) *
				tex3->Evaluate(sw, dgb);
		} else {
			// Mortar texture
			return tex2->Evaluate(sw, dg);
		}
	}
	virtual float Y() const {
		const float m = powf(Clamp(1.f - mortarsize, 0.f, 1.f), 3);
		return Lerp(m, tex2->Y(), tex1->Y());
	}
	virtual float Filter() const {
		const float m = powf(Clamp(1.f - mortarsize, 0.f, 1.f), 3);
		return Lerp(m, tex2->Filter(), tex1->Filter());
	}
	virtual void GetDuv(const SpectrumWavelengths &sw,
		const DifferentialGeometry &dg,
		float delta, float *du, float *dv) const {
		//FIXME: Generic derivative computation, replace with exact
		DifferentialGeometry dgTemp = dg;
		// Calculate bump map value at intersection point
		const float base = Texture<T>::EvalFloat(sw, dg);

		// Compute offset positions and evaluate displacement texture
		const Point origP(dgTemp.p);
		const Normal origN(dgTemp.nn);
		const float origU = dgTemp.u;

		// Shift _dgTemp_ _du_ in the $u$ direction and calculate value
		const float uu = delta / dgTemp.dpdu.Length();
		dgTemp.p += uu * dgTemp.dpdu;
		dgTemp.u += uu;
		dgTemp.nn = Normalize(origN + uu * dgTemp.dndu);
		*du = (Texture<T>::EvalFloat(sw, dgTemp) - base) / uu;

		// Shift _dgTemp_ _dv_ in the $v$ direction and calculate value
		const float vv = delta / dgTemp.dpdv.Length();
		dgTemp.p = origP + vv * dgTemp.dpdv;
		dgTemp.u = origU;
		dgTemp.v += vv;
		dgTemp.nn = Normalize(origN + vv * dgTemp.dndv);
		*dv = (Texture<T>::EvalFloat(sw, dgTemp) - base) / vv;
	}
	virtual void GetMinMaxFloat(float *minValue, float *maxValue) const {
		float min1, min2, min3;
		float max1, max2, max3;
		tex1->GetMinMaxFloat(&min1, &max1);
		tex2->GetMinMaxFloat(&min2, &max2);
		tex3->GetMinMaxFloat(&min3, &max3);
		const float minmin13 = min1 * min3;
		const float minmax13 = min1 * max3;
		const float maxmin13 = max1 * min3;
		const float maxmax13 = max1 * max3;
		*minValue = min(min(min(minmin13, minmax13), min(maxmin13, maxmax13)), min2);
		*maxValue = max(max(max(minmin13, minmax13), max(maxmin13, maxmax13)), max2);
	}
	virtual void SetIlluminant() {
		// Update sub-textures
		// Don't update tex3 as it's a filtering texture
		tex1->SetIlluminant();
		tex2->SetIlluminant();
	}
	static Texture<float> *CreateFloatTexture(const Transform &tex2world, const ParamSet &tp);
	static Texture<SWCSpectrum> *CreateSWCSpectrumTexture(const Transform &tex2world, const ParamSet &tp);
private:
	bool RunningAlternate(const Point &p, Point &i, Point &b,
		int nWhole) const {
		const float sub = nWhole + 0.5f;
		const float rsub = ceilf(sub);
		i.z = floorf(p.z);
		b.x = (p.x + i.z * run) / sub;
		b.y = (p.y + i.z * run) / sub;
		i.x = floorf(b.x);
		i.y = floorf(b.y);
		b.x = (b.x - i.x) * sub;
		b.y = (b.y - i.y) * sub;
		b.z = (p.z - i.z) * sub;
		i.x += floor(b.x) / rsub;
		i.y += floor(b.y) / rsub;
		b.x -= floor(b.x);
		b.y -= floor(b.y);
		return b.z > mortarheight && b.y > mortardepth &&
			b.x > mortarwidth;
	}

	bool Basket(const Point &p, Point &i) const {
		i.x = floorf(p.x);
		i.y = floorf(p.y);
		float bx = p.x - i.x;
		float by = p.y - i.y;
		i.x += i.y - 2.f * floorf(0.5f * i.y);
		const bool split = (i.x - 2.f * floor(0.5f * i.x)) < 1.f;
		if (split) {
			bx = fmodf(bx, invproportion);
			i.x = floorf(proportion * p.x) * invproportion;
		} else {
			by = fmodf(by, invproportion);
			i.y = floorf(proportion * p.y) * invproportion;
		}
		return by > mortardepth && bx > mortarwidth;
	}

	bool Herringbone(const Point &p, Point &i) const {
		i.y = floorf(proportion * p.y);
		const float px = p.x + i.y * invproportion;
		i.x = floorf(px);
		float bx = 0.5f * px - floorf(px * 0.5f);
		bx *= 2.f;
		float by = proportion * p.y - floorf(proportion * p.y);
		by *= invproportion;
		if (bx > 1.f + invproportion) {
			bx = proportion * (bx - 1.f);
			i.y -= floorf(bx - 1.f);
			bx -= floorf(bx);
			bx *= invproportion;
			by = 1.f;
		} else if (bx > 1.f) {
			bx = proportion * (bx - 1.f);
			i.y -= floorf(bx - 1.f);
			bx -= floorf(bx);
			bx *= invproportion;
		}
		return by > mortarheight && bx > mortarwidth;
	}

	bool Running(const Point &p, Point &i, Point &b) const {
		i.z = floorf(p.z);
		b.x = p.x + i.z * run;
		b.y = p.y - i.z * run;
		i.x = floorf(b.x);
		i.y = floorf(b.y);
		b.z = p.z - i.z;
		b.x -= i.x;
		b.y -= i.y;
		return b.z > mortarheight && b.y > mortardepth &&
			b.x > mortarwidth;
	}

	bool English(const Point &p, Point &i, Point &b) const {
		i.z = floorf(p.z);
		b.x = p.x + i.z * run;
		b.y = p.y - i.z * run;
		i.x = floorf(b.x);
		i.y = floorf(b.y);
		b.z = p.z - i.z;
		const float divider = floorf(fmodf(fabsf(i.z), 2.f)) + 1.f;
		b.x = (divider * b.x - floorf(divider * b.x)) / divider;
		b.y = (divider * b.y - floorf(divider * b.y)) / divider;
		return b.z > mortarheight && b.y > mortardepth &&
			b.x > mortarwidth;
	}

	// BrickTexture3D Private Data
	MasonryBond bond;
	Point offset;
	float brickwidth, brickheight, brickdepth, mortarsize;
	float proportion, invproportion, run;
	float mortarwidth, mortarheight, mortardepth;
	float bevelwidth, bevelheight, beveldepth;
	bool usebevel;
	TextureMapping3D *mapping;
	boost::shared_ptr<Texture<T> > tex1, tex2, tex3;
};

template <class T> Texture<float> *BrickTexture3D<T>::CreateFloatTexture(
	const Transform &tex2world, const ParamSet &tp) {
	TextureMapping3D *imap;
	// Read mapping coordinates
	string coords = tp.FindOneString("coordinates", "global");
	if (coords == "global")
		imap = new GlobalMapping3D(tex2world);
	else if (coords == "local")
		imap = new LocalMapping3D(tex2world);
	else if (coords == "uv")
		imap = new UVMapping3D(tex2world);
	else if (coords == "globalnormal")
		imap = new GlobalNormalMapping3D(tex2world);
	else if (coords == "localnormal")
		imap = new LocalNormalMapping3D(tex2world);
	else
		imap = new GlobalMapping3D(tex2world);
	// Apply texture specified transformation option for 3D mapping
	imap->Apply3DTextureMappingOptions(tp);

	boost::shared_ptr<Texture<float> > tex1(tp.GetFloatTexture("bricktex", 1.f));
	boost::shared_ptr<Texture<float> > tex2(tp.GetFloatTexture("mortartex", 0.2f));
	boost::shared_ptr<Texture<float> > tex3(tp.GetFloatTexture("brickmodtex", 1.f));

	float bw = tp.FindOneFloat("brickwidth", 0.3f);
	float bh = tp.FindOneFloat("brickheight", 0.1f);
	float bd = tp.FindOneFloat("brickdepth", 0.15f);
	float m = tp.FindOneFloat("mortarsize", 0.01f);
	float r = tp.FindOneFloat("brickrun",0.5f);
	float b = tp.FindOneFloat("brickbevel",0.f);
	string t = tp.FindOneString("brickbond","running");

	return new BrickTexture3D<float>(tex1, tex2, tex3, bw, bh, bd, m, r, b,
		t, imap);
}

template <class T> Texture<SWCSpectrum> *BrickTexture3D<T>::CreateSWCSpectrumTexture(
	const Transform &tex2world, const ParamSet &tp) {
	TextureMapping3D *imap;
	// Read mapping coordinates
	string coords = tp.FindOneString("coordinates", "global");
	if (coords == "global")
		imap = new GlobalMapping3D(tex2world);
	else if (coords == "local")
		imap = new LocalMapping3D(tex2world);
	else if (coords == "uv")
		imap = new UVMapping3D(tex2world);
	else if (coords == "globalnormal")
		imap = new GlobalNormalMapping3D(tex2world);
	else if (coords == "localnormal")
		imap = new LocalNormalMapping3D(tex2world);
	else
		imap = new GlobalMapping3D(tex2world);
	// Apply texture specified transformation option for 3D mapping
	imap->Apply3DTextureMappingOptions(tp);

	boost::shared_ptr<Texture<SWCSpectrum> > tex1(tp.GetSWCSpectrumTexture("bricktex", RGBColor(1.f)));
	boost::shared_ptr<Texture<SWCSpectrum> > tex2(tp.GetSWCSpectrumTexture("mortartex", RGBColor(0.2f)));
	boost::shared_ptr<Texture<SWCSpectrum> > tex3(tp.GetSWCSpectrumTexture("brickmodtex", RGBColor(1.f)));
	
	float bw = tp.FindOneFloat("brickwidth", 0.3f);
	float bh = tp.FindOneFloat("brickheight", 0.1f);
	float bd = tp.FindOneFloat("brickdepth", 0.15f);
	float m = tp.FindOneFloat("mortarsize", 0.01f);
	float r = tp.FindOneFloat("brickrun",0.5f);
	float b = tp.FindOneFloat("brickbevel",0.f);
	string t = tp.FindOneString("brickbond","running");
	
	return new BrickTexture3D<SWCSpectrum>(tex1, tex2, tex3, bw, bh, bd, m,
		r, b, t, imap);
}

} // namespace lux
