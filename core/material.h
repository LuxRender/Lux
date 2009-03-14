/***************************************************************************
 *   Copyright (C) 1998-2008 by authors (see AUTHORS.txt )                 *
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

// Material Class Declarations
class Material  {
public:
	// Material Interface
	Material();
	virtual ~Material();

	void InitGeneralParams(const TextureParams &mp);

	virtual BSDF *GetBSDF(const TsPack *tspack, const DifferentialGeometry &dgGeom,
		const DifferentialGeometry &dgShading, float u) const = 0;
	void Bump(boost::shared_ptr<Texture<float> > d, const DifferentialGeometry &dgGeom,
		const DifferentialGeometry &dgShading, DifferentialGeometry *dgBump) const;
	void SetChild1(boost::shared_ptr<Material> x) { child1 = x; }
	void SetChild2(boost::shared_ptr<Material> x) { child2 = x; }

	boost::shared_ptr<Material> child1, child2;
	float bumpmapSampleDistance;
};

}//namespace lux

#endif // LUX_MATERIAL_H
