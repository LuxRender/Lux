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

// scattermaterial.h*
#include "lux.h"
#include "material.h"
#include "color.h"
#include "paramset.h"

namespace lux
{

// ScatterMaterial Class Declarations
class ScatterMaterial : public Material {
public:
	// ScatterMaterial Public Methods
	ScatterMaterial(boost::shared_ptr<Texture<SWCSpectrum> > &kd,
		boost::shared_ptr<Texture<float> > &g,
		const ParamSet &mp) : Material(mp), Kd(kd), G(g) { }
	virtual ~ScatterMaterial() { }
	virtual BSDF *GetBSDF(MemoryArena &arena, const SpectrumWavelengths &sw,
		const Intersection &isect,
		const DifferentialGeometry &dgShading) const;

	static Material * CreateMaterial(const Transform &xform,
		const ParamSet &mp);
private:
	// ScatterMaterial Private Data
	boost::shared_ptr<Texture<SWCSpectrum> > Kd;
	boost::shared_ptr<Texture<float> > G;
};

// UniformRGBScatterMaterial Class Declarations
class UniformRGBScatterMaterial : public Material {
public:
	// UniformRGBScatterMaterial Public Methods
	UniformRGBScatterMaterial(const RGBColor &kd, float &g)
		: Material(ParamSet()), Kd(kd), G(g) { }
	virtual ~UniformRGBScatterMaterial() { }
	virtual BSDF *GetBSDF(MemoryArena &arena, const SpectrumWavelengths &sw,
		const Intersection &isect,
		const DifferentialGeometry &dgShading) const;
private:
	// ScatterMaterial Private Data
	RGBColor Kd;
	float G;
};

}//namespace lux

