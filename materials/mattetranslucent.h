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

// mattetranslucent.cpp*
#include "lux.h"
#include "material.h"

namespace lux
{

// MatteTranslucent Class Declarations
class MatteTranslucent : public Material {
public:
	// MatteTranslucent Public Methods
	MatteTranslucent(boost::shared_ptr<Texture<SWCSpectrum> > &kr,
		boost::shared_ptr<Texture<SWCSpectrum> > &kt,
		boost::shared_ptr<Texture<float> > &sig,
		boost::shared_ptr<Texture<float> > &bump,
		const CompositingParams &cp) : Kr(kr), Kt(kt), sigma(sig),
		bumpMap(bump) {
		compParams = new CompositingParams(cp);
	}
	virtual ~MatteTranslucent() { }
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
	// MatteTranslucent Private Data
	boost::shared_ptr<Texture<SWCSpectrum> > Kr, Kt;
	boost::shared_ptr<Texture<float> > sigma, bumpMap;
};

}//namespace lux

