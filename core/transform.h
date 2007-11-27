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
#ifndef LUX_USE_SSE
inline Point Transform::operator()(const Point &pt) const {
	float x = pt.x, y = pt.y, z = pt.z;
	float xp = m->m[0][0]*x + m->m[0][1]*y + m->m[0][2]*z + m->m[0][3];
	float yp = m->m[1][0]*x + m->m[1][1]*y + m->m[1][2]*z + m->m[1][3];
	float zp = m->m[2][0]*x + m->m[2][1]*y + m->m[2][2]*z + m->m[2][3];
	float wp = m->m[3][0]*x + m->m[3][1]*y + m->m[3][2]*z + m->m[3][3];

	BOOST_ASSERT(wp != 0);
	if (wp == 1.) return Point(xp, yp, zp);
	else          return Point(xp, yp, zp)/wp;
}
#else
inline Point Transform::operator()(const Point &pt) const {
	__m128 ptvec=_mm_set_ps(0,pt.z,pt.y,pt.x);
	
	union{
		__m128 vec;
		float f[4];
	};

      vec = m->_L4;
      vec = _mm_add_ps(vec,_mm_mul_ps(_mm_shuffle_ps(ptvec,ptvec,0x00) , m->_L1));
      vec = _mm_add_ps(vec,_mm_mul_ps(_mm_shuffle_ps(ptvec,ptvec,0x55) , m->_L2));
      vec = _mm_add_ps(vec,_mm_mul_ps(_mm_shuffle_ps(ptvec,ptvec,0xAA) , m->_L3));

	BOOST_ASSERT(f[3] != 0);
      //BOOST_ASSERT(result.w != 0);
      
      //std::cout<<"Pt:"<<f[0]<<','<< f[1] <<','<< f[2]<< ',' << f[3] << std::endl;
      
      if (f[3] == 1.) return Point(f[0], f[1], f[2]);
	else          return Point(f[0], f[1], f[2])/f[3];
}
#endif

#ifndef LUX_USE_SSE
inline void Transform::operator()(const Point &pt, Point *ptrans) const {
	float x = pt.x, y = pt.y, z = pt.z;
	ptrans->x = m->m[0][0]*x + m->m[0][1]*y + m->m[0][2]*z + m->m[0][3];
	ptrans->y = m->m[1][0]*x + m->m[1][1]*y + m->m[1][2]*z + m->m[1][3];
	ptrans->z = m->m[2][0]*x + m->m[2][1]*y + m->m[2][2]*z + m->m[2][3];
	float w   = m->m[3][0]*x + m->m[3][1]*y + m->m[3][2]*z + m->m[3][3];
	if (w != 1.) *ptrans /= w;
}
#else
inline void Transform::operator()(const Point &pt, Point *ptrans) const {
	__m128 ptvec=_mm_set_ps(0,pt.z,pt.y,pt.x);
	
	union{
		__m128 vec;
		float f[4];
	};

      vec = m->_L4;
      vec = _mm_add_ps(vec,_mm_mul_ps(_mm_shuffle_ps(ptvec,ptvec,0x00) , m->_L1));
      vec = _mm_add_ps(vec,_mm_mul_ps(_mm_shuffle_ps(ptvec,ptvec,0x55) , m->_L2));
      vec = _mm_add_ps(vec,_mm_mul_ps(_mm_shuffle_ps(ptvec,ptvec,0xAA) , m->_L3));

	BOOST_ASSERT(f[3] != 0);
    ptrans->x=f[0];
    ptrans->y=f[1];
    ptrans->z=f[2];
    if (f[3] != 1.) *ptrans /= f[3];
}
#endif

#ifndef LUX_USE_SSE
inline Vector Transform::operator()(const Vector &v) const {
  float x = v.x, y = v.y, z = v.z;
  return Vector(m->m[0][0]*x + m->m[0][1]*y + m->m[0][2]*z,
			    m->m[1][0]*x + m->m[1][1]*y + m->m[1][2]*z,
			    m->m[2][0]*x + m->m[2][1]*y + m->m[2][2]*z);
}
#else
inline Vector Transform::operator()(const Vector &v) const {    
	__m128 vec=_mm_set_ps(0,v.z,v.y,v.x);
	union{
		__m128 res;
		float f[4];
	};
		    
    res = _mm_mul_ps(_mm_shuffle_ps(vec,vec,0x00) , m->_L1);
    res = _mm_add_ps(res,_mm_mul_ps(_mm_shuffle_ps(vec,vec,0x55) , m->_L2));
    res = _mm_add_ps(res,_mm_mul_ps(_mm_shuffle_ps(vec,vec,0xAA) , m->_L3));
    return Vector(f[0], f[1], f[2]);
}
#endif

#ifndef LUX_USE_SSE
inline void Transform::operator()(const Vector &v,
		Vector *vt) const {
  float x = v.x, y = v.y, z = v.z;
  vt->x = m->m[0][0] * x + m->m[0][1] * y + m->m[0][2] * z;
  vt->y = m->m[1][0] * x + m->m[1][1] * y + m->m[1][2] * z;
  vt->z = m->m[2][0] * x + m->m[2][1] * y + m->m[2][2] * z;
}
#else
inline void Transform::operator()(const Vector &v, Vector *vt) const
{
    __m128 vec=_mm_set_ps(0,v.z,v.y,v.x);
	union{
		__m128 res;
		float f[4];
	};
		    
    res = _mm_mul_ps(_mm_shuffle_ps(vec,vec,0x00) , m->_L1);
    res = _mm_add_ps(res,_mm_mul_ps(_mm_shuffle_ps(vec,vec,0x55) , m->_L2));
    res = _mm_add_ps(res,_mm_mul_ps(_mm_shuffle_ps(vec,vec,0xAA) , m->_L3));
    vt->x=f[0];
    vt->y=f[1];
    vt->z=f[2];
}
#endif

#ifndef LUX_USE_SSE
inline Normal Transform::operator()(const Normal &n) const {
	float x = n.x, y = n.y, z = n.z;
	return Normal(mInv->m[0][0]*x + mInv->m[1][0]*y + mInv->m[2][0]*z,
                  mInv->m[0][1]*x + mInv->m[1][1]*y + mInv->m[2][1]*z,
                  mInv->m[0][2]*x + mInv->m[1][2]*y + mInv->m[2][2]*z);
}
#else
inline Normal Transform::operator()(const Normal &n) const {
	__m128 vec=_mm_set_ps(0,n.z,n.y,n.x);
	union{
		__m128 res;
		float f[4];
	};
		    
    res = _mm_mul_ps(_mm_shuffle_ps(vec,vec,0x00) , m->_L1);
    res = _mm_add_ps(res,_mm_mul_ps(_mm_shuffle_ps(vec,vec,0x55) , m->_L2));
    res = _mm_add_ps(res,_mm_mul_ps(_mm_shuffle_ps(vec,vec,0xAA) , m->_L3));
    return Normal(f[0], f[1], f[2]);
}
#endif

#ifndef LUX_USE_SSE
inline void Transform::operator()(const Normal &n,
		Normal *nt) const {
	float x = n.x, y = n.y, z = n.z;
	nt->x = mInv->m[0][0]*x + mInv->m[1][0]*y + mInv->m[2][0]*z;
	nt->y = mInv->m[0][1]*x + mInv->m[1][1]*y + mInv->m[2][1]*z;
	nt->z = mInv->m[0][2]*x + mInv->m[1][2]*y + mInv->m[2][2]*z;
}
#else
inline void Transform::operator()(const Normal &n,
		Normal *nt) const {
	__m128 vec=_mm_set_ps(0,n.z,n.y,n.x);
	union{
		__m128 res;
		float f[4];
	};
		    
    res = _mm_mul_ps(_mm_shuffle_ps(vec,vec,0x00) , m->_L1);
    res = _mm_add_ps(res,_mm_mul_ps(_mm_shuffle_ps(vec,vec,0x55) , m->_L2));
    res = _mm_add_ps(res,_mm_mul_ps(_mm_shuffle_ps(vec,vec,0xAA) , m->_L3));
    nt->x=f[0];
    nt->y=f[1];
    nt->z=f[2];
}
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
