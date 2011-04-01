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

// uvmask.cpp*
#include "lux.h"
#include "spectrum.h"
#include "texture.h"
#include "color.h"
#include "paramset.h"

namespace lux
{

// UVMaskFloatTexture Declarations
template <class T>
class UVMaskTexture : public Texture<T>
{
public:

	// UVMaskFloatTexture Public Methods
	UVMaskTexture(TextureMapping2D *m,
	              boost::shared_ptr<Texture<T> > &_innerTex,
	              boost::shared_ptr<Texture<T> > &_outerTex)
	: innerTex(_innerTex),
	  outerTex(_outerTex)
	{
		mapping = m;
	}

	virtual ~UVMaskTexture()
	{
		delete mapping;
	}

	virtual float Evaluate(const TsPack               *tspack,
	                       const DifferentialGeometry &dg) const
	{
		float s, t;
		mapping->Map(dg, &s, &t);
		if (s<0.f || s>1.f || t<0.f || t>1.f)
			return outerTex->Evaluate(tspack, dg);
		else
			return innerTex->Evaluate(tspack, dg);
	}

	virtual float Y() const
	{
		return (innerTex->Y()+outerTex->Y()) * .5f;
	}

	virtual void GetDuv(const TsPack               *tspack,
	                    const DifferentialGeometry &dg,
	                    float                      delta,
	                    float                      *du,
	                    float                      *dv) const
	{
		float s, t, dsdu, dtdu, dsdv, dtdv;
		mapping->MapDuv(dg, &s, &t, &dsdu, &dtdu, &dsdv, &dtdv);
		*du = dsdu + dtdu;
		*dv = dsdv + dtdv;
	}

	static Texture<float> * CreateFloatTexture(const Transform &tex2world,
	                                           const ParamSet  &tp);

private:

	TextureMapping2D               *mapping;
	boost::shared_ptr<Texture<T> > innerTex;
	boost::shared_ptr<Texture<T> > outerTex;
};


template <class T>
Texture<float> * UVMaskTexture<T>::CreateFloatTexture(const Transform &tex2world,
                                                      const ParamSet  &tp)
{
	// Initialize 2D texture mapping _map_ from _tp_
	TextureMapping2D *map = NULL;
	string type = tp.FindOneString("mapping", "uv");
	if (type == "uv") {
		float su = tp.FindOneFloat("uscale", 1.f);
		float sv = tp.FindOneFloat("vscale", 1.f);
		float du = tp.FindOneFloat("udelta", 0.f);
		float dv = tp.FindOneFloat("vdelta", 0.f);
		map = new UVMapping2D(su, sv, du, dv);
	} else if (type == "spherical") {
		float su = tp.FindOneFloat("uscale", 1.f);
		float sv = tp.FindOneFloat("vscale", 1.f);
		float du = tp.FindOneFloat("udelta", 0.f);
		float dv = tp.FindOneFloat("vdelta", 0.f);
		map = new SphericalMapping2D(tex2world.GetInverse(),
		                             su, sv, du, dv);
	} else if (type == "cylindrical") {
		float su = tp.FindOneFloat("uscale", 1.f);
		float du = tp.FindOneFloat("udelta", 0.f);
		map = new CylindricalMapping2D(tex2world.GetInverse(), su, du);
	} else if (type == "planar") {
		map = new PlanarMapping2D(tp.FindOneVector("v1", Vector(1,0,0)),
			tp.FindOneVector("v2", Vector(0,1,0)),
			tp.FindOneFloat("udelta", 0.f),
			tp.FindOneFloat("vdelta", 0.f));
	} else {
		std::stringstream ss;
		ss << "2D texture mapping '" << type << "' unknown";
		luxError(LUX_BADTOKEN, LUX_ERROR, ss.str().c_str());
		map = new UVMapping2D;
	}

	boost::shared_ptr<Texture<float> > innerTex(tp.GetFloatTexture("innertex", 1.f));
	boost::shared_ptr<Texture<float> > outerTex(tp.GetFloatTexture("outertex", 0.f));
  return new UVMaskTexture(map, innerTex, outerTex);
}



}//namespace lux

