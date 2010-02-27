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

// glass.cpp*
#include "lux.h"
#include "material.h"

namespace lux
{

// Glass Class Declarations
class Glass : public Material {
public:
	// Glass Public Methods
	Glass(boost::shared_ptr<Texture<SWCSpectrum> > &r,
		boost::shared_ptr<Texture<SWCSpectrum> > &t,
		boost::shared_ptr<Texture<float> > &i,
		boost::shared_ptr<Texture<float> > &cbf,
		boost::shared_ptr<Texture<float> > &flm,
		boost::shared_ptr<Texture<float> > &flmindex,
		bool archi, boost::shared_ptr<Texture<float> > &bump,
		const CompositingParams &cp) : Kr(r), Kt(t), index(i),
		cauchyb(cbf), film(flm), filmindex(flmindex), bumpMap(bump),
		architectural(archi) {
		compParams = new CompositingParams(cp);
	}
	virtual ~Glass() { }
	virtual void GetShadingGeometry(const DifferentialGeometry &dgGeom,
		DifferentialGeometry *dgBump) const {
		if (bumpMap)
			Bump(bumpMap, dgGeom, dgBump);
	}
	virtual BSDF *GetBSDF(const TsPack *tspack,
		const DifferentialGeometry &dgGeom,
		const DifferentialGeometry &dgShading,
		const Volume *exterior, const Volume *interior) const;
	
	static Material * CreateMaterial(const Transform &xform,
		const ParamSet &mp);
private:
	// Glass Private Data
	boost::shared_ptr<Texture<SWCSpectrum> > Kr, Kt;
	boost::shared_ptr<Texture<float> > index;
	boost::shared_ptr<Texture<float> > cauchyb;
	boost::shared_ptr<Texture<float> > film, filmindex;
	boost::shared_ptr<Texture<float> > bumpMap;
	bool architectural;
};

}//namespace lux

