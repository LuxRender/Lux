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

// dots.cpp*
#include "lux.h"
#include "texture.h"
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
	DotsTexture(TextureMapping2D *m, boost::shared_ptr<Texture<T> > c1,
			boost::shared_ptr<Texture<T> > c2) {
		mapping = m;
		outsideDot = c1;
		insideDot = c2;
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
	virtual void SetPower(float power, float area) {
		// Update sub-textures
		outsideDot->SetPower(power, area);
		insideDot->SetPower(power, area);
	}
	virtual void SetIlluminant() {
		// Update sub-textures
		outsideDot->SetIlluminant();
		insideDot->SetIlluminant();
	}
	static Texture<float> * CreateFloatTexture(const Transform &tex2world, const TextureParams &tp);
	static Texture<SWCSpectrum> * CreateSWCSpectrumTexture(const Transform &tex2world, const TextureParams &tp);
	
private:
	// DotsTexture Private Data
	boost::shared_ptr<Texture<T> > outsideDot, insideDot;
	TextureMapping2D *mapping;
};


// DotsTexture Method Definitions
template <class T> inline Texture<float> * DotsTexture<T>::CreateFloatTexture(const Transform &tex2world,
		const TextureParams &tp) {
	// Initialize 2D texture mapping _map_ from _tp_
	TextureMapping2D *map = NULL;
	string type = tp.FindString("mapping");
	if (type == "" || type == "uv") {
		float su = tp.FindFloat("uscale", 1.);
		float sv = tp.FindFloat("vscale", 1.);
		float du = tp.FindFloat("udelta", 0.);
		float dv = tp.FindFloat("vdelta", 0.);
		map = new UVMapping2D(su, sv, du, dv);
	}
	else if (type == "spherical") map = new SphericalMapping2D(tex2world.GetInverse());
	else if (type == "cylindrical") map = new CylindricalMapping2D(tex2world.GetInverse());
	else if (type == "planar")
		map = new PlanarMapping2D(tp.FindVector("v1", Vector(1,0,0)),
			tp.FindVector("v2", Vector(0,1,0)),
			tp.FindFloat("udelta", 0.f), tp.FindFloat("vdelta", 0.f));
	else {
		//Error("2D texture mapping \"%s\" unknown", type.c_str());
		std::stringstream ss;
		ss<<"2D texture mapping  '"<<type<<"' unknown";
		luxError(LUX_BADTOKEN,LUX_ERROR,ss.str().c_str());
		map = new UVMapping2D;
	}
	return new DotsTexture<float>(map,
		tp.GetFloatTexture("inside", 1.f),
		tp.GetFloatTexture("outside", 0.f));
}

template <class T> inline Texture<SWCSpectrum> * DotsTexture<T>::CreateSWCSpectrumTexture(const Transform &tex2world,
		const TextureParams &tp) {
	// Initialize 2D texture mapping _map_ from _tp_
	TextureMapping2D *map = NULL;
	string type = tp.FindString("mapping");
	if (type == "" || type == "uv") {
		float su = tp.FindFloat("uscale", 1.);
		float sv = tp.FindFloat("vscale", 1.);
		float du = tp.FindFloat("udelta", 0.);
		float dv = tp.FindFloat("vdelta", 0.);
		map = new UVMapping2D(su, sv, du, dv);
	}
	else if (type == "spherical") map = new SphericalMapping2D(tex2world.GetInverse());
	else if (type == "cylindrical") map = new CylindricalMapping2D(tex2world.GetInverse());
	else if (type == "planar")
		map = new PlanarMapping2D(tp.FindVector("v1", Vector(1,0,0)),
			tp.FindVector("v2", Vector(0,1,0)),
			tp.FindFloat("udelta", 0.f), tp.FindFloat("vdelta", 0.f));
	else {
		//Error("2D texture mapping \"%s\" unknown", type.c_str());
		std::stringstream ss;
		ss<<"2D texture mapping  '"<<type<<"' unknown";
		luxError(LUX_BADTOKEN,LUX_ERROR,ss.str().c_str());
		map = new UVMapping2D;
	}
	return new DotsTexture<SWCSpectrum>(map,
		tp.GetSWCSpectrumTexture("inside", 1.f),
		tp.GetSWCSpectrumTexture("outside", 0.f));
}

}//namespace lux

