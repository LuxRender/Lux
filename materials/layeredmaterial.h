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

// layeredmaterial.cpp*
#include "lux.h"
#include "material.h"

namespace lux
{

// LayeredMaterial Class Declarations
class LayeredMaterial : public Material {
public:
	// LayeredMaterial Public Methods
	LayeredMaterial(const ParamSet &mp, boost::shared_ptr<Material> &m1,
		 boost::shared_ptr<Material> &m2,boost::shared_ptr<Material> &m3,
		  boost::shared_ptr<Material> &m4,
		  boost::shared_ptr<Texture<float>> &op1, boost::shared_ptr<Texture<float>> &op2,
		  boost::shared_ptr<Texture<float>> &op3, boost::shared_ptr<Texture<float>> &op4
		  ) : Material(mp), mat1(m1),mat2(m2),
		  mat3(m3),mat4(m4), opacity1(op1),opacity2(op2),opacity3(op3),opacity4(op4) {}

	//void LayeredMaterial::setMaterial(int slot, boost::shared_ptr<Material> &mat);
	
	virtual ~LayeredMaterial() { }
	virtual BSDF *GetBSDF(MemoryArena &arena, const SpectrumWavelengths &sw,
		const Intersection &isect,
		const DifferentialGeometry &dgShading) const;

	static Material * CreateMaterial(const Transform &xform,
		const ParamSet &mp);

private:
	// LayeredMaterial Private Data
	boost::shared_ptr<Material> mat1,mat2,mat3,mat4;
	boost::shared_ptr<Texture<float>> opacity1,opacity2,opacity3,opacity4;
};

}//namespace lux

