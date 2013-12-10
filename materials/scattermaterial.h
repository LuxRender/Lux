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

#ifndef LUX_MATERIALS_SCATTERMATERIAL_H
#define LUX_MATERIALS_SCATTERMATERIAL_H
// scattermaterial.h*
#include "lux.h"
#include "material.h"
#include "luxrays/core/color/color.h"
#include "paramset.h"

namespace lux
{

// ScatterMaterial Class Declarations
class ScatterMaterial : public Material {
public:
	// ScatterMaterial Public Methods
	ScatterMaterial(boost::shared_ptr<Texture<SWCSpectrum> > &kd,
		boost::shared_ptr<Texture<SWCSpectrum> > &g,
		const ParamSet &mp) : Material("ScatterMaterial-" + boost::lexical_cast<string>(this), mp, false),
		Kd(kd), G(g) { }
	virtual ~ScatterMaterial() { }
	virtual BSDF *GetBSDF(luxrays::MemoryArena &arena, const SpectrumWavelengths &sw,
		const Intersection &isect,
		const DifferentialGeometry &dgShading) const;

	static Material * CreateMaterial(const Transform &xform,
		const ParamSet &mp);
private:
	// ScatterMaterial Private Data
	boost::shared_ptr<Texture<SWCSpectrum> > Kd, G;
};

// UniformRGBScatterMaterial Class Declarations
class UniformRGBScatterMaterial : public Material {
public:
	// UniformRGBScatterMaterial Public Methods
	UniformRGBScatterMaterial(const RGBColor &ks, const RGBColor &ka,
		float &g_) : Material("UniformRGBScatterMaterial-" + boost::lexical_cast<string>(this), ParamSet(), false),
		kS(ks), kA(ka), g(g_) { }
	virtual ~UniformRGBScatterMaterial() { }
	virtual BSDF *GetBSDF(luxrays::MemoryArena &arena, const SpectrumWavelengths &sw,
		const Intersection &isect,
		const DifferentialGeometry &dgShading) const;
private:
	// UniformRGBScatterMaterial Private Data
	RGBColor kS, kA;
	float g;
};

// VolumeScatterMaterial Class Declarations
class VolumeScatterMaterial : public Material {
public:
	// VolumeScatterMaterial Public Methods
	VolumeScatterMaterial(const Volume *v,
		boost::shared_ptr<Texture<SWCSpectrum> > &g)
		: Material("VolumeScatterMaterial-" + boost::lexical_cast<string>(this), ParamSet(), false),
		volume(v), G(g) { }
	virtual ~VolumeScatterMaterial() { }
	virtual BSDF *GetBSDF(luxrays::MemoryArena &arena, const SpectrumWavelengths &sw,
		const Intersection &isect,
		const DifferentialGeometry &dgShading) const;
private:
	// VolumeScatterMaterial Private Data
	const Volume *volume;
	boost::shared_ptr<Texture<SWCSpectrum> > G;
};

}//namespace lux

#endif //LUX_MATERIALS_SCATTERMATERIAL_H
