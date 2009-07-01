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

// shinymetal.cpp*
#include "lux.h"
#include "material.h"

namespace lux
{

// ShinyMetal Class Declarations
class ShinyMetal : public Material {
public:
	// ShinyMetal Public Methods
	ShinyMetal(boost::shared_ptr<Texture<SWCSpectrum> > ks,
		boost::shared_ptr<Texture<float> > u, boost::shared_ptr<Texture<float> > v,
		boost::shared_ptr<Texture<float> > flm, boost::shared_ptr<Texture<float> > flmindex, 
			boost::shared_ptr<Texture<SWCSpectrum> > kr, boost::shared_ptr<Texture<float> > bump,
			CompositingParams cp) {
		Ks = ks;
		Kr = kr;
		nu = u;
		nv = v;
		film = flm;
		filmindex = flmindex;
		bumpMap = bump;
		compParams = new CompositingParams(cp);
	}
	virtual ~ShinyMetal() { }
	virtual BSDF *GetBSDF(const TsPack *tspack, const DifferentialGeometry &dgGeom, const DifferentialGeometry &dgShading, float u) const;
	
	static Material * CreateMaterial(const Transform &xform, const TextureParams &mp);
private:
	// ShinyMetal Private Data
	boost::shared_ptr<Texture<SWCSpectrum> > Ks, Kr;
	boost::shared_ptr<Texture<float> > nu, nv;
	boost::shared_ptr<Texture<float> > film, filmindex;
	boost::shared_ptr<Texture<float> > bumpMap;
};

}//namespace lux

