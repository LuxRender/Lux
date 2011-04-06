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

// roughglass.cpp*
#include "lux.h"
#include "material.h"

namespace lux
{

// RoughGlassGlass Class Declarations
class RoughGlass : public Material {
public:
	// RoughGlass Public Methods
	RoughGlass(boost::shared_ptr<Texture<SWCSpectrum> > &r,
		boost::shared_ptr<Texture<SWCSpectrum> > &t, 
		boost::shared_ptr<Texture<float> > &urough,
		boost::shared_ptr<Texture<float> > &vrough,
		boost::shared_ptr<Texture<float> > &i,
		boost::shared_ptr<Texture<float> > &cbf,
		boost::shared_ptr<Texture<float> > &bump,
		const ParamSet &mp) : Material(mp), Kr(r), Kt(t), index(i),
		cauchyb(cbf), uroughness(urough), vroughness(vrough),
		bumpMap(bump) { }
	virtual ~RoughGlass() { }
	virtual void GetShadingGeometry(const SpectrumWavelengths &sw,
		const Normal &nGeom, DifferentialGeometry *dgBump) const {
		if (bumpMap)
			Bump(sw, bumpMap, nGeom, dgBump);
	}
	virtual BSDF *GetBSDF(MemoryArena &arena, const SpectrumWavelengths &sw,
		const Intersection &isect,
		const DifferentialGeometry &dgShading) const;

	static Material * CreateMaterial(const Transform &xform,
		const ParamSet &mp);
private:
	// RoughGlass Private Data
	boost::shared_ptr<Texture<SWCSpectrum> > Kr, Kt;
	boost::shared_ptr<Texture<float> > index;
	boost::shared_ptr<Texture<float> > cauchyb;
	boost::shared_ptr<Texture<float> > uroughness;
	boost::shared_ptr<Texture<float> > vroughness;
	boost::shared_ptr<Texture<float> > bumpMap;
};

}//namespace lux

