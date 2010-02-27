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

#ifndef LUX_MATERIAL_H
#define LUX_MATERIAL_H
// material.h*
#include "lux.h"

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
class Material  {
public:
	// Material Interface
	Material();
	virtual ~Material();

	void InitGeneralParams(const ParamSet &mp);

	virtual BSDF *GetBSDF(const TsPack *tspack,
		const DifferentialGeometry &dgGeom,
		const DifferentialGeometry &dgShading,
		const Volume *exterior, const Volume *interior) const = 0;
	void Bump(const boost::shared_ptr<Texture<float> > &d,
		const DifferentialGeometry &dgGeom,
		DifferentialGeometry *dgBump) const;
	virtual void GetShadingGeometry(const DifferentialGeometry &dgGeom,
		DifferentialGeometry *dgBump) const { }

	static void FindCompositingParams(const ParamSet &mp, CompositingParams *cp);

	float bumpmapSampleDistance;
	CompositingParams *compParams;
};

}//namespace lux

#endif // LUX_MATERIAL_H
