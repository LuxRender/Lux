/***************************************************************************
 *   Copyright (C) 1998-2013 by authors (see AUTHORS.txt)                  *
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
		bool archi, const ParamSet &mp) : Material("Glass-" + boost::lexical_cast<string>(this), mp),
		Kr(r), Kt(t), index(i),
		cauchyb(cbf), film(flm), filmindex(flmindex), architectural(archi) {
		AddBoolAttribute(*this, "architectural", "Glass architectural flag", &Glass::architectural);
	}
	virtual ~Glass() { }
	virtual BSDF *GetBSDF(MemoryArena &arena, const SpectrumWavelengths &sw,
		const Intersection &isect,
		const DifferentialGeometry &dgShading) const;

	Texture<SWCSpectrum> *GetKrTexture() { return Kr.get(); }
	Texture<SWCSpectrum> *GetKtTexture() { return Kt.get(); }
	Texture<float> *GetIndexTexture() { return index.get(); }

	static Material * CreateMaterial(const Transform &xform,
		const ParamSet &mp);

private:
	// Glass Private Data
	boost::shared_ptr<Texture<SWCSpectrum> > Kr, Kt;
	boost::shared_ptr<Texture<float> > index;
	boost::shared_ptr<Texture<float> > cauchyb;
	boost::shared_ptr<Texture<float> > film, filmindex;
	bool architectural;
};

}//namespace lux

