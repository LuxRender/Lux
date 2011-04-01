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

// glass2.h*
#include "lux.h"
#include "material.h"

namespace lux
{

// Glass Class Declarations
class Glass2 : public Material {
public:
	// Glass Public Methods
	Glass2(bool archi, bool disp, boost::shared_ptr<Texture<float> > &bump,
		const CompositingParams &cp, boost::shared_ptr<Texture<SWCSpectrum> > &sc) : bumpMap(bump),
		architectural(archi), dispersion(disp) {
		compParams = new CompositingParams(cp);
		Sc = sc;
	}
	virtual ~Glass2() { }
	virtual void GetShadingGeometry(const TsPack *tspack,
		const Normal &nGeom,
		DifferentialGeometry *dgBump) const {
		if (bumpMap)
			Bump(tspack, bumpMap, nGeom, dgBump);
	}
	virtual BSDF *GetBSDF(const TsPack *tspack,
		const DifferentialGeometry &dgGeom,
		const DifferentialGeometry &dgShading,
		const Volume *exterior, const Volume *interior) const;
	
	static Material * CreateMaterial(const Transform &xform,
		const ParamSet &mp);
private:
	// Glass Private Data
	boost::shared_ptr<Texture<float> > bumpMap;
	bool architectural, dispersion;
};

}//namespace lux

