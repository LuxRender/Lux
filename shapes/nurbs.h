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

// nurbs.cpp*
#include "lux.h"
#include "shape.h"
#include "dynload.h"
#include "texture.h"

namespace lux
{

// NURBS Declarations
class NURBS : public Shape {
public:
	// NURBS Methods
	NURBS(const Transform &o2w, bool reverseOrientation, const string &name, 
		u_int nu, u_int uorder, const float *uknot, float umin, float umax,
		u_int nv, u_int vorder, const float *vknot, float vmin, float vmax,
		const float *P, bool isHomogeneous);
	virtual ~NURBS();
	virtual BBox ObjectBound() const;
	virtual BBox WorldBound() const;
	virtual bool CanIntersect() const { return false; }
	virtual void Refine(vector<boost::shared_ptr<Shape> > &refined) const;
	
	static Shape* CreateShape(const Transform &o2w, bool reverseOrientation, const ParamSet &params);
private:
	// NURBS Data
	u_int nu, uorder, nv, vorder;
	float umin, umax, vmin, vmax;
	float *uknot, *vknot;
	bool isHomogeneous;
	float *P;
};

}//namespace lux
