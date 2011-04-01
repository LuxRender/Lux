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

// matte.cpp*
#include "lux.h"
#include "material.h"
#include "spectrum.h"

namespace lux
{

// Matte Class Declarations
class Matte : public Material {
public:
	// Matte Public Methods
	Matte(boost::shared_ptr<Texture<SWCSpectrum> > &kd,
		boost::shared_ptr<Texture<float> > &sig,
		boost::shared_ptr<Texture<float> > &bump,
		const CompositingParams &cp, boost::shared_ptr<Texture<SWCSpectrum> > &sc) : Kd(kd), sigma(sig),
		bumpMap(bump) {
		compParams = new CompositingParams(cp);
		Sc = sc; 
	}
	virtual ~Matte() { }
	virtual void GetShadingGeometry(const TsPack *tspack,
		const Normal &nGeom, DifferentialGeometry *dgBump) const {
		if (bumpMap)
			Bump(tspack, bumpMap, nGeom, dgBump);
	}


	virtual BSDF *GetBSDF(const TsPack *tspack,
		const DifferentialGeometry &dgGeom,
		const DifferentialGeometry &dgShading,
		const Volume *exterior, const Volume *interior) const;

	virtual SWCSpectrum GetKd(const TsPack *tspack,	const DifferentialGeometry &dgs) const;
              
	static Material * CreateMaterial(const Transform &xform,
		const ParamSet &mp);
private:
	// Matte Private Data
	boost::shared_ptr<Texture<SWCSpectrum> > Kd;
	boost::shared_ptr<Texture<float> > sigma, bumpMap;
};

}//namespace lux

