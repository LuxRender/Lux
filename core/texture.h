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

#ifndef LUX_TEXTURE_H
#define LUX_TEXTURE_H
// texture.h*
#include "lux.h"
#include "geometry/transform.h"
#include "spectrum.h"

namespace lux
{

// Texture Declarations
class  TextureMapping2D {
public:
	// TextureMapping2D Interface
	virtual ~TextureMapping2D() { }
	virtual void Map(const DifferentialGeometry &dg,
		float *s, float *t, float *dsdx, float *dtdx,
		float *dsdy, float *dtdy) const = 0;
};
class  UVMapping2D : public TextureMapping2D {
public:
	// UVMapping2D Public Methods
	UVMapping2D(float su = 1, float sv = 1,
		float du = 0, float dv = 0);
	virtual ~UVMapping2D() { }
	virtual void Map(const DifferentialGeometry &dg, float *s, float *t,
		float *dsdx, float *dtdx,
		float *dsdy, float *dtdy) const;
private:
	float su, sv, du, dv;
};
class  SphericalMapping2D : public TextureMapping2D {
public:
	// SphericalMapping2D Public Methods
	SphericalMapping2D(const Transform &toSph)
		: WorldToTexture(toSph) {
	}
	virtual ~SphericalMapping2D() { }
	virtual void Map(const DifferentialGeometry &dg, float *s, float *t,
		float *dsdx, float *dtdx,
		float *dsdy, float *dtdy) const;
private:
	void sphere(const Point &P, float *s, float *t) const;
	Transform WorldToTexture;
};
class
 CylindricalMapping2D : public TextureMapping2D {
public:
	// CylindricalMapping2D Public Methods
	CylindricalMapping2D(const Transform &toCyl)
		: WorldToTexture(toCyl) {
	}
	virtual ~CylindricalMapping2D() { }
	virtual void Map(const DifferentialGeometry &dg, float *s, float *t,
		float *dsdx, float *dtdx,
		float *dsdy, float *dtdy) const;
private:
	void cylinder(const Point &P, float *s, float *t) const;
	Transform WorldToTexture;
};
class  PlanarMapping2D : public TextureMapping2D {
public:
	// PlanarMapping2D Public Methods
	PlanarMapping2D(const Vector &v1, const Vector &v2,
		float du = 0, float dv = 0);
	virtual ~PlanarMapping2D() { }
	virtual void Map(const DifferentialGeometry &dg, float *s, float *t,
		float *dsdx, float *dtdx,
		float *dsdy, float *dtdy) const;
private:
	Vector vs, vt;
	float ds, dt;
};
class  TextureMapping3D {
public:
	// TextureMapping3D Interface
	virtual ~TextureMapping3D() { }
	virtual Point Map(const DifferentialGeometry &dg,
		Vector *dpdx, Vector *dpdy) const = 0;
};
class  IdentityMapping3D : public TextureMapping3D {
public:
	IdentityMapping3D(const Transform &x)
		: WorldToTexture(x) { }
	virtual ~IdentityMapping3D() { }
	virtual Point Map(const DifferentialGeometry &dg, Vector *dpdx,
		Vector *dpdy) const;
		void Apply3DTextureMappingOptions(const TextureParams &tp);
//private:
	Transform WorldToTexture;
};
class  EnvironmentMapping {
public:
	// EnvironmentMapping Interface
	virtual ~EnvironmentMapping() { }
	virtual void Map(const Vector &wh, float *s, float *t) const = 0;
};
class  LatLongMapping : public EnvironmentMapping {
public:
	// LatLongMapping Public Methods
	LatLongMapping() {}
	virtual ~LatLongMapping() { }
	virtual void Map(const Vector &wh, float *s, float *t) const;
};
class  AngularMapping : public EnvironmentMapping {
public:
	// AngularMapping Public Methods
	AngularMapping() {}
	virtual ~AngularMapping() { }
	virtual void Map(const Vector &wh, float *s, float *t) const;
};
class  VerticalCrossMapping : public EnvironmentMapping {
public:
	// VerticalCross Public Methods
	VerticalCrossMapping() {}
	virtual ~VerticalCrossMapping() { }
	virtual void Map(const Vector &wh, float *s, float *t) const;
};

template <class T> class Texture {
public:
	//typedef boost::shared_ptr<Texture> TexturePtr; <<! Not working with GCC
	// Texture Interface
	virtual T Evaluate(const TsPack *tspack, const DifferentialGeometry &) const = 0;
	virtual void SetPower(float power, float area) { }
	virtual void SetIlluminant() { }
	virtual ~Texture() { }
};

  float Noise(float x, float y = .5f, float z = .5f);
  float Noise(const Point &P);
  float FBm(const Point &P, const Vector &dpdx, const Vector &dpdy,
	float omega, int octaves);
  float Turbulence(const Point &P, const Vector &dpdx, const Vector &dpdy,
	float omega, int octaves);
  float Lanczos(float, float tau=2);

}//namespace lux

#endif // LUX_TEXTURE_H
