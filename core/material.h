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

#ifndef LUX_MATERIAL_H
#define LUX_MATERIAL_H
// material.h*
#include "lux.h"
#include "queryable.h"

namespace lux
{

// Per Material/BSDF CompositingParams for precise control
// when rendering objects in a compositing animation pipeline
struct CompositingParams {
	CompositingParams() : A(0.f), tVm(true), tVl(true), tiVm(true),
		tiVl(true), oA(false) { }
	float A;  // Overridden Alpha Value
	bool tVm; // Trace Visibility for material
	bool tVl; // Trace Visibility for emission
	bool tiVm; // Trace Indirect Visibility for material
	bool tiVl; // Trace Indirect Visibility for emission
	bool oA;  // Override Alpha
};

// Material Class Declarations
class Material : public Queryable {
public:
	// Material Interface
	Material(const string &name, const ParamSet &mp, const bool hasBumpMap = true);
	virtual ~Material() { }

	virtual BSDF *GetBSDF(luxrays::MemoryArena &arena, const SpectrumWavelengths &sw,
		const Intersection &isect,
		const DifferentialGeometry &dgShading) const = 0;
	void Bump(const SpectrumWavelengths &sw,
		const Normal &nGeom, DifferentialGeometry *dgBump) const;
	virtual void GetShadingGeometry(const SpectrumWavelengths &sw,
		const Normal &nGeom, DifferentialGeometry *dgBump) const { 
		if (bumpMap)
			Bump(sw, nGeom, dgBump);
	}

	boost::shared_ptr<Texture<float> > bumpMap;
	float bumpmapSampleDistance;
	CompositingParams compParams;
};

}//namespace lux

#endif // LUX_MATERIAL_H
