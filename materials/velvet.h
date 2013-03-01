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

// velvet.cpp*
#include "lux.h"
#include "material.h"

namespace lux
{

// Velvet Class Declarations
class Velvet : public Material {
public:
	// Velvet Public Methods
	Velvet(boost::shared_ptr<Texture<SWCSpectrum> > &kd,
		boost::shared_ptr<Texture<float> > &p1,
		boost::shared_ptr<Texture<float> > &p2,
		boost::shared_ptr<Texture<float> > &p3,
		boost::shared_ptr<Texture<float> > &thickness,
		const ParamSet &mp) : Material("Velvet-" + boost::lexical_cast<string>(this), mp),
		Kd(kd), P1(p1), P2(p2), P3(p3), Thickness(thickness) { }
	virtual ~Velvet() { }
	virtual BSDF *GetBSDF(MemoryArena &arena, const SpectrumWavelengths &sw,
		const Intersection &isect,
		const DifferentialGeometry &dgShading) const;
	              
	static Material * CreateMaterial(const Transform &xform,
		const ParamSet &mp);
private:
	// Velvet Private Data
	boost::shared_ptr<Texture<SWCSpectrum> > Kd;
	boost::shared_ptr<Texture<float> > P1, P2, P3, Thickness;
};

}//namespace lux

