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

#include "lux.h"
#include "material.h"

namespace lux
{

// DoubleSideMaterial Class Declarations
class DoubleSideMaterial : public Material {
public:
	// DoubleSideMaterial Public Methods
	DoubleSideMaterial(
		boost::shared_ptr<Material> &m1,
		boost::shared_ptr<Material> &m2,
		const ParamSet &mp) : Material("DoubleSideMaterial-" + boost::lexical_cast<string>(this), mp, false),
		mat1(m1), mat2(m2) { }
	virtual ~DoubleSideMaterial() { }
	virtual BSDF *GetBSDF(MemoryArena &arena, const SpectrumWavelengths &sw,
		const Intersection &isect,
		const DifferentialGeometry &dgShading) const;

	Material *GetMaterial1() { return mat1.get(); }
	Material *GetMaterial2() { return mat2.get(); }

	static Material * CreateMaterial(const Transform &xform,
		const ParamSet &mp);

private:
	// DoubleSideMaterial Private Data
	boost::shared_ptr<Material> mat1, mat2;
};

}//namespace lux

