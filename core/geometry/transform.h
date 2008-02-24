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

#ifndef LUX_TRANSFORM_H
#define LUX_TRANSFORM_H
// transform.h*
#include "lux.h"
#include "geometry.h"

namespace lux
{

// Transform Declarations
class  Transform {
public:
	// Transform Public Methods
	Transform() {
		boost::shared_ptr<Matrix4x4> o (new Matrix4x4());
		m = mInv = o;
	}
	Transform(float mat[4][4]) {
		boost::shared_ptr<Matrix4x4> o (new Matrix4x4(mat[0][0],mat[0][1],mat[0][2],mat[0][3],
	                	mat[1][0],mat[1][1],mat[1][2],mat[1][3],
	                	mat[2][0],mat[2][1],mat[2][2],mat[2][3],
	                	mat[3][0],mat[3][1],mat[3][2],mat[3][3]));
		m = o;
		mInv = m->Inverse();
	}
	Transform(const boost::shared_ptr<Matrix4x4> &mat) {
		m = mat;
		mInv = m->Inverse();
	}
	Transform(const boost::shared_ptr<Matrix4x4> &mat,
	          const boost::shared_ptr<Matrix4x4> &minv) {
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
	boost::shared_ptr<Matrix4x4> m, mInv;
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

  Transform Translate(const Vector &delta);
  Transform Scale(float x, float y, float z);
  Transform RotateX(float angle);
  Transform RotateY(float angle);
  Transform RotateZ(float angle);
  Transform Rotate(float angle, const Vector &axis);
  Transform LookAt(const Point &pos, const Point &look, const Vector &up);
  Transform Orthographic(float znear, float zfar);
  Transform Perspective(float fov, float znear, float zfar);

}//namespace lux


#endif // LUX_TRANSFORM_H
