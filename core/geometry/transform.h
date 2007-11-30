/***************************************************************************
 *   Copyright (C) 1998-2007 by authors (see AUTHORS.txt )                 *
 *                                                                         *
 *   This file is part of Lux Renderer.                                    *
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
 *   Lux Renderer website : http://www.luxrender.org                       *
 ***************************************************************************/

#ifndef LUX_TRANSFORM_H
#define LUX_TRANSFORM_H
// transform.h*
#include "lux.h"
#include "geometry.h"


// Transform Declarations
class  Transform {
public:
	// Transform Public Methods
	Transform() {
		Matrix4x4Ptr o (new Matrix4x4());
		m = mInv = o;
	}
	Transform(float mat[4][4]) {
		Matrix4x4Ptr o (new Matrix4x4(mat[0][0],mat[0][1],mat[0][2],mat[0][3],
	                	mat[1][0],mat[1][1],mat[1][2],mat[1][3],
	                	mat[2][0],mat[2][1],mat[2][2],mat[2][3],
	                	mat[3][0],mat[3][1],mat[3][2],mat[3][3]));
		m = o;
		mInv = m->Inverse();
	}
	Transform(const Matrix4x4Ptr &mat) {
		m = mat;
		mInv = m->Inverse();
	}
	Transform(const Matrix4x4Ptr &mat,
	          const Matrix4x4Ptr &minv) {
		m = mat;
		mInv = minv;
	}
	friend ostream &operator<<(ostream &, const Transform &);
	Transform GetInverse() const {
		return Transform(mInv, m);
	}
	bool HasScale() const;
	inline Point operator()(const Point &pt) const;
	inline void operator()(const Point &pt,Point *ptrans) const;
	inline Vector operator()(const Vector &v) const;
	inline void operator()(const Vector &v, Vector *vt) const;
	inline Normal operator()(const Normal &) const;
	inline void operator()(const Normal &, Normal *nt) const;
	inline Ray operator()(const Ray &r) const;
	inline void operator()(const Ray &r, Ray *rt) const;
	BBox operator()(const BBox &b) const;
	Transform operator*(const Transform &t2) const;
	bool SwapsHandedness() const;
private:
	// Transform Private Data
	Matrix4x4Ptr m, mInv;
};
// Transform Inline Functions
#ifdef LUX_USE_SSE
#include "transform-sse.inl"
#else
#include "transform.inl"
#endif

inline Ray Transform::operator()(const Ray &r) const {
	Ray ret;
	(*this)(r.o, &ret.o);
	(*this)(r.d, &ret.d);
	ret.mint = r.mint;
	ret.maxt = r.maxt;
	ret.time = r.time;
	return ret;
}
inline void Transform::operator()(const Ray &r,
                                  Ray *rt) const {
	(*this)(r.o, &rt->o);
	(*this)(r.d, &rt->d);
	rt->mint = r.mint;
	rt->maxt = r.maxt;
	rt->time = r.time;
}



#endif // LUX_TRANSFORM_H
