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

#ifndef LUX_VECTOR_H
#define LUX_VECTOR_H

#include <iostream>


namespace lux
{

class Point;
class Normal;

class  Vector {
	friend class boost::serialization::access;
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
//		BOOST_ASSERT(f!=0);
		float inv = 1.f / f;
		return Vector(x * inv, y * inv, z * inv);
	}
	
	Vector &operator/=(float f) {
//		BOOST_ASSERT(f!=0);
		float inv = 1.f / f;
		x *= inv; y *= inv; z *= inv;
		return *this;
	}
	Vector operator-() const {
		return Vector(-x, -y, -z);
	}
	float operator[](int i) const {
		BOOST_ASSERT(i >= 0 && i <= 2);
		return (&x)[i];
	}
	
	float &operator[](int i) {
		BOOST_ASSERT(i >= 0 && i <= 2);
		return (&x)[i];
	}
	float LengthSquared() const { return x*x + y*y + z*z; }
	float Length() const { return sqrtf(LengthSquared()); }
	explicit Vector(const Normal &n);
	// Vector Public Data
	float x, y, z;
	
private:
	template<class Archive>
			void serialize(Archive & ar, const unsigned int version)
			{
				ar & x;
				ar & y;
				ar & z;
			}
};

inline ostream &operator<<(ostream &os, const Vector &v) {
	os << v.x << ", " << v.y << ", " << v.z;
	return os;
}
inline Vector operator*(float f, const Vector &v) {
	return v*f;
}

}//namespace lux

#endif //LUX_VECTOR_H
