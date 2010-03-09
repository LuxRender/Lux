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
template <class T>
class BrickTexture3D : public Texture<T> {
public:
	// BrickTexture3D Public Methods

	virtual ~BrickTexture3D() { delete mapping; }

	BrickTexture3D(boost::shared_ptr<Texture<T> > &c1,
		boost::shared_ptr<Texture<T> > &c2,
		boost::shared_ptr<Texture<T> > &c3,
		float brickw, float brickh, float brickd, float mortar, float r, float bev, const string &b, 
		TextureMapping3D *map) : brickwidth(brickw),
		brickheight(brickh), brickdepth(brickd), mortarsize(mortar), run(r),
		mapping(map), tex1(c1), tex2(c2), tex3(c3) { 
			if ( b == "stacked" ) { bond = RUNNING; run = 0.f; }
			else if ( b == "flemish" ) bond = FLEMISH;
			else if ( b == "english" ) { bond = ENGLISH; run = 0.25f; }
			else if ( b == "herringbone" ) bond = HERRINGBONE;
			else if ( b == "basket" ) bond = BASKET;
			else if ( b == "chain link" ) { bond = KETTING; run = 1.25f; offset = Point(.25f,-1.f,0); }
			else { bond = RUNNING; offset = Point(0,-0.5f,0); }
			if ( bond == HERRINGBONE || bond == BASKET ) {
				proportion = floor(brickwidth/brickheight);
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

#define BRICK_EPSILON 1e-3f

	virtual bool running_alternate(Point p, Point &i, Point &b, int nWhole) const {
		const float sub = nWhole + 0.5f; 
		const float rsub = ceil(sub);
		i.z = floor(p.z);
		i.x = floor((p.x+i.z*run)/sub);
		i.y = floor((p.y+i.z*run)/sub);
		b.x = (p.x+i.z*run)/sub;
		b.y = (p.y+i.z*run)/sub;
		b.x = (b.x - floor(b.x))*sub;
		b.y = (b.y - floor(b.y))*sub;
		b.z = (p.z - floor(p.z))*sub;
		i.x += floor(b.x) / rsub;
		i.y += floor(b.y) / rsub;
		b.x -= floor(b.x);
		b.y -= floor(b.y);
		return ( b.z > mortarheight ) &&
		       ( b.y > mortardepth  ) &&
		       ( b.x > mortarwidth  );
	}
	
	virtual bool basket(Point p, Point &i) const {
		i.y = floor(p.y);
		i.x = floor(p.x);
		float bx = p.x - floor(p.x);
		float by = p.y - floor(p.y);
		i.x += i.y - 2.0f * floor(0.5f * i.y);
		const bool split = (i.x - 2.0f * floor(0.5f * i.x)) < 1.f;
		if ( split ) {
			bx = fmod(bx,invproportion);
			i.x = floor(proportion*p.x)/proportion;
		} else {
			by = fmod(by,invproportion);
			i.y = floor(proportion*p.y)/proportion;
		}
		return ( by > mortardepth ) &&
		       ( bx > mortarwidth  );
	}
	
	virtual bool herringbone(Point p, Point &i) const {
		i.y = floor(proportion * p.y);
		i.x = floor(p.x+i.y/proportion);
		p.x += i.y * invproportion;
		float bx = 0.5f * p.x - floor(p.x * 0.5f);
		bx *= 2;
		float by = proportion * p.y - floor(proportion * p.y);
		by *= invproportion;
		if ( bx > 1.0f + invproportion ) {
			bx = proportion * bx - proportion;
			i.y -= floor(bx-1.f);
			bx -= floor(bx);
			bx /= proportion;
			by = 1.f;
		} else if ( bx > 1.0f ) {
			bx = proportion * bx - proportion;
			i.y -= floor(bx-1.f);
			bx -= floor(bx);
			bx /= proportion;
		}
		return ( by > mortarheight ) &&
		       ( bx > mortarwidth  );
	} 
	
	virtual bool running(Point p, Point &i, Point &b) const {
		i.z = floor(p.z);
		i.x = floor(p.x+i.z*run);
		i.y = floor(p.y-i.z*run);
		b.x = p.x + i.z * run;
		b.y = p.y - i.z * run;
		b.z = p.z - floor(p.z);
		b.x -= floor(b.x);
		b.y -= floor(b.y);
		const bool ret = ( b.z > mortarheight ) &&
				 ( b.y > mortardepth  ) &&
				 ( b.x > mortarwidth  );
		return ret;
	}
	
	virtual bool english(Point p, Point &i, Point &b) const {
		i.z = floor(p.z);
		const float divider = floor(fmod(abs(i.z),2.f)) + 1.f;
		i.x = floor((p.x+i.z*run));
		i.y = floor((p.y-i.z*run));
		b.x = p.x + i.z * run;
		b.y = p.y - i.z * run;
		b.z = p.z - floor(p.z);
		b.x = ( divider * b.x - floor(divider * b.x) ) / divider;
		b.y = ( divider * b.y - floor(divider * b.y) ) / divider;
		return ( b.z > mortarheight ) &&
		       ( b.y > mortardepth  ) &&
		       ( b.x > mortarwidth  );
	}
	
	virtual T Evaluate(const TsPack *tspack,
		const DifferentialGeometry &dg) const {
		Vector dpdx, dpdy;
		Point P = mapping->Map(dg, &dpdx, &dpdy);
		
		const float offs = BRICK_EPSILON + mortarsize;
		Point bP = P + Point(offs, offs, offs);
		
		// Normalize coordinates according brick dimensions
		bP.x /= brickwidth;
		bP.y /= brickdepth;
		bP.z /= brickheight;
		
		bP += offset;
		
		Point brickIndex;
		Point bevel;
		bool b;
		switch( bond ) {
			case FLEMISH: b = running_alternate(bP,brickIndex,bevel,1); break;
			case RUNNING: b = running(bP,brickIndex,bevel); break;
			case ENGLISH: b = english(bP,brickIndex,bevel); break;
			case HERRINGBONE: b = herringbone(bP,brickIndex); break;
			case BASKET: b = basket(bP,brickIndex); break;
			case KETTING: b = running_alternate(bP,brickIndex,bevel,2); break; 
			default: b = true; break;
		}
		/* Bevels, lets skip that for now
		if ( usebevel ) {
			const float bevelx = ( 2.f * max(bevel.x,1.f-bevel.x) - 2.f + bevelwidth ) / bevelwidth;
			const float bevely = ( 2.f * max(bevel.y,1.f-bevel.y) - 2.f + beveldepth ) / beveldepth;
			const float bevelz = ( 2.f * max(bevel.z,1.f-bevel.z) - 2.f + bevelheight ) / bevelheight;
			const float amount = max(bevelz,max(bevelx,bevely));
			
			return amount * tex2->Evaluate(tspack, dg) + (1.f - amount) * tex1->Evaluate(tspack, dg);
		} else 
		*/
		if ( b ) {
			// DifferentialGeometry for brick
			DifferentialGeometry dgb = DifferentialGeometry(brickIndex,dg.nn,dg.dpdu,dg.dpdv,dg.dndu,dg.dndv,dg.u,dg.v,dg.handle);
			/* Create seams between brick texture, lets skip that for now
			Point offset = Point(dg.p);
			const float o = tex4->Evaluate(tspack,dgb);
			offset.x += o;offset.y += o;offset.z += o;
			DifferentialGeometry dg2 = DifferentialGeometry(offset,dg.nn,dg.dpdu,dg.dpdv,dg.dndu,dg.dndv,dg.u,dg.v,dg.handle);
			*/
			// Brick texture * Modulation texture
			return tex1->Evaluate(tspack, dg) * tex3->Evaluate(tspack,dgb);
		} else {
			// Mortar texture
			return tex2->Evaluate(tspack, dg);
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
	virtual void SetIlluminant() {
		// Update sub-textures
		tex1->SetIlluminant();
		tex2->SetIlluminant();
	}
	static Texture<float> *CreateFloatTexture(const Transform &tex2world, const ParamSet &tp);
	static Texture<SWCSpectrum> *CreateSWCSpectrumTexture(const Transform &tex2world, const ParamSet &tp);
private:
	// BrickTexture3D Private Data
	MasonryBond bond;
	Point offset;
	float brickwidth, brickheight, brickdepth, mortarsize, proportion, invproportion, run, mortarwidth, mortarheight, mortardepth, bevelwidth, bevelheight, beveldepth;
	bool usebevel;
	TextureMapping3D *mapping;
	boost::shared_ptr<Texture<T> > tex1, tex2, tex3;
};

template <class T> Texture<float> *BrickTexture3D<T>::CreateFloatTexture(
	const Transform &tex2world,
	const ParamSet &tp) {
	// Initialize 3D texture mapping _map_ from _tp_
	TextureMapping3D *map = new IdentityMapping3D(tex2world);
	// Apply texture specified transformation option for 3D mapping
	IdentityMapping3D *imap = (IdentityMapping3D*) map;
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

	return new BrickTexture3D<float>(tex1, tex2, tex3, bw, bh, bd, m, r, b, t, map);
}

template <class T> Texture<SWCSpectrum> *BrickTexture3D<T>::CreateSWCSpectrumTexture(
	const Transform &tex2world,
	const ParamSet &tp) {
	// Initialize 3D texture mapping _map_ from _tp_
	TextureMapping3D *map = new IdentityMapping3D(tex2world);
	// Apply texture specified transformation option for 3D mapping
	IdentityMapping3D *imap = (IdentityMapping3D*) map;
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
	
	return new BrickTexture3D<SWCSpectrum>(tex1, tex2, tex3, bw, bh, bd, m, r, b, t, map);
}

} // namespace lux
