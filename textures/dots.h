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

// dots.cpp*
#include "lux.h"
#include "spectrum.h"
#include "texture.h"
#include "color.h"
#include "paramset.h"
#include "error.h"

namespace lux
{

// DotsTexture Declarations
template <class T> class DotsTexture : public Texture<T> {
public:
	// DotsTexture Public Methods
	virtual ~DotsTexture() {
		delete mapping;
	}
	DotsTexture(TextureMapping2D *m, boost::shared_ptr<Texture<T> > &c1,
		boost::shared_ptr<Texture<T> > &c2) : outsideDot(c1),
		insideDot(c2) {
		mapping = m;
	}
	virtual T Evaluate(const TsPack *tspack, const DifferentialGeometry &dg) const {
		// Compute cell indices for dots
		float s, t, dsdx, dtdx, dsdy, dtdy;
		mapping->Map(dg, &s, &t, &dsdx, &dtdx, &dsdy, &dtdy);
		int sCell = Floor2Int(s + .5f), tCell = Floor2Int(t + .5f);
		// Return _insideDot_ result if point is inside dot
		if (Noise(sCell+.5f, tCell+.5f) > 0) {
			float radius = .35f;
			float maxShift = 0.5f - radius;
			float sCenter = sCell + maxShift *
				Noise(sCell + 1.5f, tCell + 2.8f);
			float tCenter = tCell + maxShift *
				Noise(sCell + 4.5f, tCell + 9.8f);
			float ds = s - sCenter, dt = t - tCenter;
			if (ds*ds + dt*dt < radius*radius)
				return insideDot->Evaluate(tspack, dg);
		}
		return outsideDot->Evaluate(tspack, dg);
	}
	virtual float Y() const { return (insideDot->Y() + outsideDot->Y()) / 2.f; }
	virtual float Filter() const { return (insideDot->Filter() + outsideDot->Filter()) / 2.f; }
	virtual void SetIlluminant() {
		// Update sub-textures
		outsideDot->SetIlluminant();
		insideDot->SetIlluminant();
	}
	static Texture<float> * CreateFloatTexture(const Transform &tex2world, const ParamSet &tp);
	static Texture<SWCSpectrum> * CreateSWCSpectrumTexture(const Transform &tex2world, const ParamSet &tp);
	
private:
	// DotsTexture Private Data
	boost::shared_ptr<Texture<T> > outsideDot, insideDot;
	TextureMapping2D *mapping;
};


// DotsTexture Method Definitions
template <class T> inline Texture<float> * DotsTexture<T>::CreateFloatTexture(const Transform &tex2world,
		const ParamSet &tp) {
	// Initialize 2D texture mapping _map_ from _tp_
	TextureMapping2D *map = NULL;
	string type = tp.FindOneString("mapping", "uv");
	if (type == "uv") {
		float su = tp.FindOneFloat("uscale", 1.f);
		float sv = tp.FindOneFloat("vscale", 1.f);
		float du = tp.FindOneFloat("udelta", 0.f);
		float dv = tp.FindOneFloat("vdelta", 0.f);
		map = new UVMapping2D(su, sv, du, dv);
	} else if (type == "spherical")
		map = new SphericalMapping2D(tex2world.GetInverse());
	else if (type == "cylindrical")
		map = new CylindricalMapping2D(tex2world.GetInverse());
	else if (type == "planar")
		map = new PlanarMapping2D(tp.FindOneVector("v1", Vector(1,0,0)),
			tp.FindOneVector("v2", Vector(0,1,0)),
			tp.FindOneFloat("udelta", 0.f),
			tp.FindOneFloat("vdelta", 0.f));
	else {
		std::stringstream ss;
		ss << "2D texture mapping  '" << type << "' unknown";
		luxError(LUX_BADTOKEN, LUX_ERROR, ss.str().c_str());
		map = new UVMapping2D;
	}
	boost::shared_ptr<Texture<float> > in(tp.GetFloatTexture("inside", 1.f)),
		out(tp.GetFloatTexture("outside", 0.f));
	return new DotsTexture<float>(map, in, out);
}

template <class T> inline Texture<SWCSpectrum> * DotsTexture<T>::CreateSWCSpectrumTexture(const Transform &tex2world,
		const ParamSet &tp) {
	// Initialize 2D texture mapping _map_ from _tp_
	TextureMapping2D *map = NULL;
	string type = tp.FindOneString("mapping", "uv");
	if (type == "uv") {
		float su = tp.FindOneFloat("uscale", 1.f);
		float sv = tp.FindOneFloat("vscale", 1.f);
		float du = tp.FindOneFloat("udelta", 0.f);
		float dv = tp.FindOneFloat("vdelta", 0.f);
		map = new UVMapping2D(su, sv, du, dv);
	} else if (type == "spherical")
		map = new SphericalMapping2D(tex2world.GetInverse());
	else if (type == "cylindrical")
		map = new CylindricalMapping2D(tex2world.GetInverse());
	else if (type == "planar")
		map = new PlanarMapping2D(tp.FindOneVector("v1", Vector(1,0,0)),
			tp.FindOneVector("v2", Vector(0,1,0)),
			tp.FindOneFloat("udelta", 0.f),
			tp.FindOneFloat("vdelta", 0.f));
	else {
		std::stringstream ss;
		ss << "2D texture mapping  '" << type << "' unknown";
		luxError(LUX_BADTOKEN, LUX_ERROR, ss.str().c_str());
		map = new UVMapping2D;
	}
	boost::shared_ptr<Texture<SWCSpectrum> >
		in(tp.GetSWCSpectrumTexture("inside", RGBColor(1.f))),
		out(tp.GetSWCSpectrumTexture("outside", RGBColor(0.f)));
	return new DotsTexture<SWCSpectrum>(map, in, out);
}

}//namespace lux

