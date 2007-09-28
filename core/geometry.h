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

#ifndef LUX_GEOMETRY_H
#define LUX_GEOMETRY_H
// geometry.h*
#include "lux.h"
#include <float.h>

#ifdef LUX_USE_SSE
#include <xmmintrin.h>
#endif

// Geometry Declarations
#ifndef LUX_USE_SSE
class COREDLL Vector {
public:
	// Vector Public Methods
	Vector(float _x=0, float _y=0, float _z=0)
		: x(_x), y(_y), z(_z) {
	}
	explicit Vector(const Point &p);
	Vector operator+(const Vector &v) const {
		return Vector(x + v.x, y + v.y, z + v.z);
	}
	
	Vector& operator+=(const Vector &v) {
		x += v.x; y += v.y; z += v.z;
		return *this;
	}
	Vector operator-(const Vector &v) const {
		return Vector(x - v.x, y - v.y, z - v.z);
	}
	
	Vector& operator-=(const Vector &v) {
		x -= v.x; y -= v.y; z -= v.z;
		return *this;
	}
	bool operator==(const Vector &v) const {
		return x == v.x && y == v.y && z == v.z;
	}
	Vector operator*(float f) const {
		return Vector(f*x, f*y, f*z);
	}
	
	Vector &operator*=(float f) {
		x *= f; y *= f; z *= f;
		return *this;
	}
	Vector operator/(float f) const {
//		Assert(f!=0);
		float inv = 1.f / f;
		return Vector(x * inv, y * inv, z * inv);
	}
	
	Vector &operator/=(float f) {
//		Assert(f!=0);
		float inv = 1.f / f;
		x *= inv; y *= inv; z *= inv;
		return *this;
	}
	Vector operator-() const {
		return Vector(-x, -y, -z);
	}
	float operator[](int i) const {
		Assert(i >= 0 && i <= 2);
		return (&x)[i];
	}
	
	float &operator[](int i) {
		Assert(i >= 0 && i <= 2);
		return (&x)[i];
	}
	float LengthSquared() const { return x*x + y*y + z*z; }
	float Length() const { return sqrtf(LengthSquared()); }
	explicit Vector(const Normal &n);
	// Vector Public Data
	float x, y, z;
};
#else
class COREDLL Vector {
public:
	// Vector Public Methods
	Vector(float _x=0, float _y=0, float _z=0)
		: x(_x), y(_y), z(_z) {
	}
	Vector(__m128 _vec)
        : vec(_vec)
    {}
	
	explicit Vector(const Point &p);
	Vector operator+(const Vector &v) const {
		return Vector(x + v.x, y + v.y, z + v.z);
	}
	
	Vector& operator+=(const Vector &v) {
		vec=_mm_add_ps(vec,v.vec);
		return *this;
	}
	Vector operator-(const Vector &v) const {
		return Vector(_mm_sub_ps(vec,v.vec));
	}
	
	Vector& operator-=(const Vector &v) {
		vec=_mm_sub_ps(vec,v.vec);
		return *this;
	}
	bool operator==(const Vector &v) const {
		return x == v.x && y == v.y && z == v.z;
	}
	Vector operator*(float f) const {
		return Vector(_mm_mul_ps(vec,_mm_set_ps1(f)));
	}
	
	Vector &operator*=(float f) {
		vec=_mm_mul_ps(vec,_mm_set_ps1(f));
		return *this;
	}
	Vector operator/(float f) const {
		Assert(f!=0);
		/*float inv = 1.f / f;
		return Vector(x * inv, y * inv, z * inv);*/
		
		//test the following
		
		//BOOST_ASSERT(f!=0);
      	return Vector(_mm_div_ps(vec,_mm_set_ps1(f)));
		  
	}
	
	Vector &operator/=(float f) {
		Assert(f!=0);
		/*
		float inv = 1.f / f;
		x *= inv; y *= inv; z *= inv;
		return *this;*/
		//BOOST_ASSERT(f!=0);
      	vec=_mm_div_ps(vec,_mm_set_ps1(f));
      	return *this;
	}
	Vector operator-() const {
		//return Vector(-x, -y, -z);
		return Vector(_mm_sub_ps(_mm_set_ps1(0.f),vec));
	}
	float operator[](int i) const {
		Assert(i >= 0 && i <= 2);
		return (&x)[i];
	}
	
	float &operator[](int i) {
		Assert(i >= 0 && i <= 2);
		return (&x)[i];
	}
	float LengthSquared() const 
	{ __m128 r = _mm_mul_ps(vec,vec);
      __m128 t = _mm_add_ss(_mm_shuffle_ps(r,r,1), _mm_add_ss(_mm_movehl_ps(r,r),r));
      return *(float *)&t;
    }
	float Length() const { return sqrtf(LengthSquared()); }
	explicit Vector(const Normal &n);
	// Vector Public Data
	//float x, y, z;
	union {
      __m128 vec;
      struct
      {
        float   x,y,z,pad;
      };
    };
};
#endif

#ifndef LUX_USE_SSE
class COREDLL Point {
public:
	// Point Methods
	Point(float _x=0, float _y=0, float _z=0)
		: x(_x), y(_y), z(_z) {
	}
	Point operator+(const Vector &v) const {
		return Point(x + v.x, y + v.y, z + v.z);
	}
	
	Point &operator+=(const Vector &v) {
		x += v.x; y += v.y; z += v.z;
		return *this;
	}
	Vector operator-(const Point &p) const {
		return Vector(x - p.x, y - p.y, z - p.z);
	}
	
	Point operator-(const Vector &v) const {
		return Point(x - v.x, y - v.y, z - v.z);
	}
	
	Point &operator-=(const Vector &v) {
		x -= v.x; y -= v.y; z -= v.z;
		return *this;
	}
	Point &operator+=(const Point &p) {
		x += p.x; y += p.y; z += p.z;
		return *this;
	}
	Point operator+(const Point &p) const {
		return Point(x + p.x, y + p.y, z + p.z);
	}
	Point operator* (float f) const {
		return Point(f*x, f*y, f*z);
	}
	Point &operator*=(float f) {
		x *= f; y *= f; z *= f;
		return *this;
	}
	Point operator/ (float f) const {
		float inv = 1.f/f;
		return Point(inv*x, inv*y, inv*z);
	}
	Point &operator/=(float f) {
		float inv = 1.f/f;
		x *= inv; y *= inv; z *= inv;
		return *this;
	}
	float operator[](int i) const { return (&x)[i]; }
	float &operator[](int i) { return (&x)[i]; }
	// Point Public Data
	float x,y,z;
};
#else
class COREDLL Point {
public:
	// Point Methods
	Point(float _x=0, float _y=0, float _z=0)
		: x(_x), y(_y), z(_z) {
	}
	
	Point(__m128 _vec)
        : vec(_vec)
    {}
	
	Point operator+(const Vector &v) const {
		return Point(_mm_add_ps(vec,v.vec));
	}
	
	Point &operator+=(const Vector &v) {
		vec=_mm_add_ps(vec,v.vec);//vec+=v.vec;
      	return *this;
	}
	Vector operator-(const Point &p) const {
		return Vector(_mm_sub_ps(vec,p.vec));
	}
	
	Point operator-(const Vector &v) const {
		return Point(_mm_sub_ps(vec,v.vec));
	}
	
	Point &operator-=(const Vector &v) {
		vec=_mm_sub_ps(vec,v.vec);//vec-=v.vec;
      return *this;
	}
	Point &operator+=(const Point &p) {
		vec=_mm_add_ps(vec,p.vec);//vec+=p.vec;
      return *this;
	}
	Point operator+(const Point &p) const {
		return Point(_mm_add_ps(vec,p.vec));
	}
	Point operator* (float f) const {
		return Point(_mm_mul_ps(vec,_mm_set_ps1(f)));
	}
	Point &operator*=(float f) {
		vec=_mm_mul_ps(vec,_mm_set_ps1(f));//vec*=_mm_set_ps1(f);
      return *this;
	}
	Point operator/ (float f) const {
		//float inv = 1.f/f;
		//return Point(inv*x, inv*y, inv*z);
		//BOOST_ASSERT(f!=0);
      return Point(_mm_div_ps(vec,_mm_set_ps1(f)));
	}
	Point &operator/=(float f) {
		/*float inv = 1.f/f;
		x *= inv; y *= inv; z *= inv;
		return *this;*/
		vec=_mm_div_ps(vec,_mm_set_ps1(f));//vec/=_mm_set_ps1(f);
      	return *this;
	}
	float operator[](int i) const { return (&x)[i]; }
	float &operator[](int i) { return (&x)[i]; }
	// Point Public Data
	union {
      __m128 vec; //!< Vector representation of the coordinates
      struct
      {
        float   x,y,z,w; //!< Coordinates
      };
    };
    
void* operator new(size_t t) { return _mm_malloc(t,16); }
void operator delete(void* ptr, size_t t) { _mm_free(ptr); }
void* operator new[](size_t t) { return _mm_malloc(t,16); }
void operator delete[] (void* ptr) { _mm_free(ptr); }
};
#endif

class COREDLL Normal {
public:
	// Normal Methods
	Normal(float _x=0, float _y=0, float _z=0)
		: x(_x), y(_y), z(_z) {}
	Normal operator-() const {
		return Normal(-x, -y, -z);
	}
	Normal operator+ (const Normal &v) const {
		return Normal(x + v.x, y + v.y, z + v.z);
	}
	
	Normal& operator+=(const Normal &v) {
		x += v.x; y += v.y; z += v.z;
		return *this;
	}
	Normal operator- (const Normal &v) const {
		return Normal(x - v.x, y - v.y, z - v.z);
	}
	
	Normal& operator-=(const Normal &v) {
		x -= v.x; y -= v.y; z -= v.z;
		return *this;
	}
	Normal operator* (float f) const {
		return Normal(f*x, f*y, f*z);
	}
	
	Normal &operator*=(float f) {
		x *= f; y *= f; z *= f;
		return *this;
	}
	Normal operator/ (float f) const {
		float inv = 1.f/f;
		return Normal(x * inv, y * inv, z * inv);
	}
	
	Normal &operator/=(float f) {
		float inv = 1.f/f;
		x *= inv; y *= inv; z *= inv;
		return *this;
	}
	float LengthSquared() const { return x*x + y*y + z*z; }
	float Length() const        { return sqrtf(LengthSquared()); }
	
	explicit Normal(const Vector &v)
	  : x(v.x), y(v.y), z(v.z) {}
	float operator[](int i) const { return (&x)[i]; }
	float &operator[](int i) { return (&x)[i]; }
	// Normal Public Data
	float x,y,z;
};
class COREDLL Ray {
public:
	// Ray Public Methods
	Ray(): mint(RAY_EPSILON), maxt(INFINITY), time(0.f) {}
	Ray(const Point &origin, const Vector &direction,
		float start = RAY_EPSILON, float end = INFINITY, float t = 0.f)
		: o(origin), d(direction), mint(start), maxt(end), time(t) { }
	Point operator()(float t) const { return o + d * t; }
	// Ray Public Data
	Point o;
	Vector d;
	mutable float mint, maxt;
	float time;

#ifdef LUX_USE_SSE
	float pad;
#endif
	
#ifdef LUX_USE_SSE
void* operator new(size_t t) { return _mm_malloc(t,16); }
void operator delete(void* ptr, size_t t) { _mm_free(ptr); }
void* operator new[](size_t t) { return _mm_malloc(t,16); }
void operator delete[] (void* ptr) { _mm_free(ptr); }
#endif
};
class COREDLL RayDifferential : public Ray {
public:
	// RayDifferential Methods
	RayDifferential() { hasDifferentials = false; }
	RayDifferential(const Point &org, const Vector &dir)
			: Ray(org, dir) {
		hasDifferentials = false;
	}
	explicit RayDifferential(const Ray &ray) : Ray(ray) {
		hasDifferentials = false;
	}
	// RayDifferential Public Data
	
	Ray rx, ry;
	bool hasDifferentials;
	
#ifdef LUX_USE_SSE
void* operator new(size_t t) { return _mm_malloc(t,16); }
void operator delete(void* ptr, size_t t) { _mm_free(ptr); }
void* operator new[](size_t t) { return _mm_malloc(t,16); }
void operator delete[] (void* ptr) { _mm_free(ptr); }
#endif	
};
class COREDLL BBox {
public:
	// BBox Public Methods
	BBox() {
		pMin = Point( INFINITY,  INFINITY,  INFINITY);
		pMax = Point(-INFINITY, -INFINITY, -INFINITY);
	}
	BBox(const Point &p) : pMin(p), pMax(p) { }
	BBox(const Point &p1, const Point &p2) {
		pMin = Point(min(p1.x, p2.x),
					 min(p1.y, p2.y),
					 min(p1.z, p2.z));
		pMax = Point(max(p1.x, p2.x),
					 max(p1.y, p2.y),
					 max(p1.z, p2.z));
	}
	friend inline ostream &
		operator<<(ostream &os, const BBox &b);
	friend COREDLL BBox Union(const BBox &b, const Point &p);
	friend COREDLL BBox Union(const BBox &b, const BBox &b2);
	bool Overlaps(const BBox &b) const {
		bool x = (pMax.x >= b.pMin.x) && (pMin.x <= b.pMax.x);
		bool y = (pMax.y >= b.pMin.y) && (pMin.y <= b.pMax.y);
		bool z = (pMax.z >= b.pMin.z) && (pMin.z <= b.pMax.z);
		return (x && y && z);
	}
	bool Inside(const Point &pt) const {
		return (pt.x >= pMin.x && pt.x <= pMax.x &&
	            pt.y >= pMin.y && pt.y <= pMax.y &&
	            pt.z >= pMin.z && pt.z <= pMax.z);
	}
	void Expand(float delta) {
		pMin -= Vector(delta, delta, delta);
		pMax += Vector(delta, delta, delta);
	}
	float Volume() const {
		Vector d = pMax - pMin;
		return d.x * d.y * d.z;
	}
	int MaximumExtent() const {
		Vector diag = pMax - pMin;
		if (diag.x > diag.y && diag.x > diag.z)
			return 0;
		else if (diag.y > diag.z)
			return 1;
		else
			return 2;
	}
	void BoundingSphere(Point *c, float *rad) const;
	bool IntersectP(const Ray &ray,
	                float *hitt0 = NULL,
					float *hitt1 = NULL) const;
	// BBox Public Data
	Point pMin, pMax;
	
#ifdef LUX_USE_SSE
void* operator new(size_t t) { return _mm_malloc(t,16); }
void operator delete(void* ptr, size_t t) { _mm_free(ptr); }
void* operator new[](size_t t) { return _mm_malloc(t,16); }
void operator delete[] (void* ptr) { _mm_free(ptr); }
#endif
	
};
// Geometry Inline Functions
inline Vector::Vector(const Point &p)
	: x(p.x), y(p.y), z(p.z) {
}
inline ostream &operator<<(ostream &os, const Vector &v) {
	os << v.x << ", " << v.y << ", " << v.z;
	return os;
}
inline Vector operator*(float f, const Vector &v) {
	return v*f;
}
#ifndef LUX_USE_SSE
inline float Dot(const Vector &v1, const Vector &v2) {
	return v1.x * v2.x + v1.y * v2.y + v1.z * v2.z;
}
#else
inline float Dot(const Vector &v1, const Vector &v2) {
	__m128 r = _mm_mul_ps(v1.vec,v2.vec);
    __m128 t = _mm_add_ss(_mm_shuffle_ps(r,r,1), _mm_add_ps(_mm_movehl_ps(r,r),r));
    return *(float *)&t;
}
#endif
inline float AbsDot(const Vector &v1, const Vector &v2) {
	return fabsf(Dot(v1, v2));
}

#ifndef LUX_USE_SSE
inline Vector Cross(const Vector &v1, const Vector &v2) {
	return Vector((v1.y * v2.z) - (v1.z * v2.y),
                  (v1.z * v2.x) - (v1.x * v2.z),
                  (v1.x * v2.y) - (v1.y * v2.x));
}
#else
inline Vector Cross(const Vector &v1, const Vector &v2) {
	__m128 l1, l2, m1, m2;
      l1 = _mm_shuffle_ps(v1.vec,v1.vec, _MM_SHUFFLE(3,1,0,2));
      l2 = _mm_shuffle_ps(v2.vec,v2.vec, _MM_SHUFFLE(3,0,2,1));
      m2 = _mm_mul_ps(l1,l2);
      l1 = _mm_shuffle_ps(v1.vec,v1.vec, _MM_SHUFFLE(3,0,2,1));
      l2 = _mm_shuffle_ps(v2.vec,v2.vec, _MM_SHUFFLE(3,1,0,2));
      m1 = _mm_mul_ps(l1,l2);
      return _mm_sub_ps(m1,m2);
}
#endif
inline Vector Cross(const Vector &v1, const Normal &v2) {
	return Vector((v1.y * v2.z) - (v1.z * v2.y),
                  (v1.z * v2.x) - (v1.x * v2.z),
                  (v1.x * v2.y) - (v1.y * v2.x));
}
inline Vector Cross(const Normal &v1, const Vector &v2) {
	return Vector((v1.y * v2.z) - (v1.z * v2.y),
                  (v1.z * v2.x) - (v1.x * v2.z),
                  (v1.x * v2.y) - (v1.y * v2.x));
}
#ifndef LUX_USE_SSE
inline Vector Normalize(const Vector &v) {
	return v / v.Length();
}
#else
inline Vector Normalize(const Vector &v) {
	return Vector(_mm_div_ps(v.vec,_mm_set_ps1(v.Length())));
}
#endif
inline void CoordinateSystem(const Vector &v1, Vector *v2, Vector *v3) {
	if (fabsf(v1.x) > fabsf(v1.y)) {
		float invLen = 1.f / sqrtf(v1.x*v1.x + v1.z*v1.z);
		*v2 = Vector(-v1.z * invLen, 0.f, v1.x * invLen);
	}
	else {
		float invLen = 1.f / sqrtf(v1.y*v1.y + v1.z*v1.z);
		*v2 = Vector(0.f, v1.z * invLen, -v1.y * invLen);
	}
	*v3 = Cross(v1, *v2);
}
inline float Distance(const Point &p1, const Point &p2) {
	return (p1 - p2).Length();
}
inline float DistanceSquared(const Point &p1, const Point &p2) {
	return (p1 - p2).LengthSquared();
}
inline ostream &operator<<(ostream &os, const Point &v) {
	os << v.x << ", " << v.y << ", " << v.z;
	return os;
}
inline Point operator*(float f, const Point &p) {
	return p*f;
}
inline Normal operator*(float f, const Normal &n) {
	return Normal(f*n.x, f*n.y, f*n.z);
}
inline Normal Normalize(const Normal &n) {
	return n / n.Length();
}
inline Vector::Vector(const Normal &n)
  : x(n.x), y(n.y), z(n.z) { }
inline float Dot(const Normal &n1, const Vector &v2) {
	return n1.x * v2.x + n1.y * v2.y + n1.z * v2.z;
}
inline float Dot(const Vector &v1, const Normal &n2) {
	return v1.x * n2.x + v1.y * n2.y + v1.z * n2.z;
}
inline float Dot(const Normal &n1, const Normal &n2) {
	return n1.x * n2.x + n1.y * n2.y + n1.z * n2.z;
}
inline float AbsDot(const Normal &n1, const Vector &v2) {
	return fabsf(n1.x * v2.x + n1.y * v2.y + n1.z * v2.z);
}
inline float AbsDot(const Vector &v1, const Normal &n2) {
	return fabsf(v1.x * n2.x + v1.y * n2.y + v1.z * n2.z);
}
inline float AbsDot(const Normal &n1, const Normal &n2) {
	return fabsf(n1.x * n2.x + n1.y * n2.y + n1.z * n2.z);
}
inline ostream &operator<<(ostream &os, const Normal &v) {
	os << v.x << ", " << v.y << ", " << v.z;
	return os;
}
inline ostream &operator<<(ostream &os, Ray &r) {
	os << "org: " << r.o << "dir: " << r.d << " range [" <<
		r.mint << "," << r.maxt << "] time = " <<
		r.time;
	return os;
}
inline ostream &operator<<(ostream &os, const BBox &b) {
	os << b.pMin << " -> " << b.pMax;
	return os;
}
inline Vector SphericalDirection(float sintheta,
                              float costheta, float phi) {
	return Vector(sintheta * cosf(phi),
	              sintheta * sinf(phi),
				  costheta);
}
inline Vector SphericalDirection(float sintheta,
                                   float costheta,
								   float phi,
								   const Vector &x,
								   const Vector &y,
								   const Vector &z) {
	return sintheta * cosf(phi) * x +
		sintheta * sinf(phi) * y + costheta * z;
}
inline float SphericalTheta(const Vector &v) {
	return acosf(Clamp(v.z, -1.f, 1.f));
}
inline float SphericalPhi(const Vector &v) {
	float p = atan2f(v.y, v.x);
	return (p < 0.f) ? p + 2.f*M_PI : p;
}
#endif // LUX_GEOMETRY_H
