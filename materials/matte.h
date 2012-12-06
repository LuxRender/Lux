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
#include "queryable.h"

namespace lux
{

// Matte Class Declarations
class Matte : public Material, public Queryable {
public:
	// Matte Public Methods
	Matte(boost::shared_ptr<Texture<SWCSpectrum> > &kd,
		boost::shared_ptr<Texture<float> > &sig,
		const ParamSet &mp) : Material(mp),
			Queryable("Matte-" + boost::lexical_cast<string>(this)),
			Kd(kd), sigma(sig) { }
	virtual ~Matte() { }
	virtual BSDF *GetBSDF(MemoryArena &arena, const SpectrumWavelengths &sw,
		const Intersection &isect,
		const DifferentialGeometry &dgShading) const;

	Texture<SWCSpectrum> *GetTexture() { return Kd.get(); }

	static Material * CreateMaterial(const Transform &xform,
		const ParamSet &mp);
private:
	// Matte Private Data
	boost::shared_ptr<Texture<SWCSpectrum> > Kd;
	boost::shared_ptr<Texture<float> > sigma;
};

}//namespace lux

