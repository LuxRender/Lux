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

namespace lux
{

// Texture Declarations
class  TextureMapping2D {
public:
	// TextureMapping2D Interface
	virtual ~TextureMapping2D() { }
	virtual void Map(const DifferentialGeometry &dg,
		float *s, float *t) const = 0;
	virtual void MapDuv(const DifferentialGeometry &dg,
		float *s, float *t, float *dsdu, float *dtdu,
		float *dsdv, float *dtdv) const = 0;
};
class  UVMapping2D : public TextureMapping2D {
public:
	// UVMapping2D Public Methods
	UVMapping2D(float su = 1, float sv = 1,
		float du = 0, float dv = 0);
	virtual ~UVMapping2D() { }
	virtual void Map(const DifferentialGeometry &dg,
		float *s, float *t) const;
	virtual void MapDuv(const DifferentialGeometry &dg,
		float *s, float *t, float *dsdu, float *dtdu,
		float *dsdv, float *dtdv) const;
private:
	float su, sv, du, dv;
};
class  SphericalMapping2D : public TextureMapping2D {
public:
	// SphericalMapping2D Public Methods
	SphericalMapping2D(const Transform &toSph,
	                   float _su = 1.f, float _sv = 1.f,
	                   float _du = 0.f, float _dv = 0.f)
		: WorldToTexture(toSph),
		  du(_du), dv(_dv),
		  scaledInvTwoPi(INV_TWOPI * _su), scaledInvPi(INV_PI * _sv)
	{ }
	virtual ~SphericalMapping2D() { }
	virtual void Map(const DifferentialGeometry &dg,
		float *s, float *t) const;
	virtual void MapDuv(const DifferentialGeometry &dg,
		float *s, float *t, float *dsdu, float *dtdu,
		float *dsdv, float *dtdv) const;
private:
	Transform WorldToTexture;
	float     du, dv;
	float     scaledInvTwoPi, scaledInvPi;

};
class CylindricalMapping2D : public TextureMapping2D {
public:
	// CylindricalMapping2D Public Methods
	CylindricalMapping2D(const Transform &toCyl,
	                     float _su = 1.f, float _du = 0.f)
		: WorldToTexture(toCyl),
		  du(_du),
		  scaledInvTwoPi(INV_TWOPI * _su)
	{	}
	virtual ~CylindricalMapping2D() { }
	virtual void Map(const DifferentialGeometry &dg,
		float *s, float *t) const;
	virtual void MapDuv(const DifferentialGeometry &dg,
		float *s, float *t, float *dsdu, float *dtdu,
		float *dsdv, float *dtdv) const;
private:
	Transform WorldToTexture;
	float     du, scaledInvTwoPi;
};
class  PlanarMapping2D : public TextureMapping2D {
public:
	// PlanarMapping2D Public Methods
	PlanarMapping2D(const Vector &v1, const Vector &v2,
		float du = 0, float dv = 0);
	virtual ~PlanarMapping2D() { }
	virtual void Map(const DifferentialGeometry &dg,
		float *s, float *t) const;
	virtual void MapDuv(const DifferentialGeometry &dg,
		float *s, float *t, float *dsdu, float *dtdu,
		float *dsdv, float *dtdv) const;
private:
	Vector vs, vt;
	float ds, dt;
};
class  TextureMapping3D {
public:
	// TextureMapping3D Interface
	TextureMapping3D(const Transform &x) : WorldToTexture(x) { }
	virtual ~TextureMapping3D() { }
	virtual Point Map(const DifferentialGeometry &dg) const = 0;
	virtual Point MapDuv(const DifferentialGeometry &dg,
		Vector *dpdu, Vector *dpdv) const = 0;
	void Apply3DTextureMappingOptions(const ParamSet &tp);
//private:
	Transform WorldToTexture;
};
// 3D base: global, object (from original), uv (from dupli),
// original coords (from dupli), sticky, window, normal, reflection, stress,
// tangent
// 3D->2D (except for UV and window): flat, cube, tube, sphere
// matrix
// process: 3Dbase->matrix->3D2D
class GlobalMapping3D : public TextureMapping3D {
public:
	GlobalMapping3D(const Transform &x) : TextureMapping3D(x) { }
	virtual ~GlobalMapping3D() { }
	virtual Point Map(const DifferentialGeometry &dg) const;
	virtual Point MapDuv(const DifferentialGeometry &dg,
		Vector *dpdu, Vector *dpdv) const;
};
class LocalMapping3D : public TextureMapping3D {
public:
	LocalMapping3D(const Transform &x) : TextureMapping3D(x) { }
	virtual ~LocalMapping3D() { }
	virtual Point Map(const DifferentialGeometry &dg) const;
	virtual Point MapDuv(const DifferentialGeometry &dg,
		Vector *dpdu, Vector *dpdv) const;
};
class  UVMapping3D : public TextureMapping3D {
public:
	UVMapping3D(const Transform &x) : TextureMapping3D(x) { }
	virtual ~UVMapping3D() { }
	virtual Point Map(const DifferentialGeometry &dg) const;
	virtual Point MapDuv(const DifferentialGeometry &dg,
		Vector *dpdu, Vector *dpdv) const;
};
class  EnvironmentMapping {
public:
	// EnvironmentMapping Interface
	virtual ~EnvironmentMapping() { }
	virtual void Map(const Vector &wh, float *s, float *t,
		float *pdf = NULL) const = 0;
	virtual void Map(float s, float t, Vector *wh,
		float *pdf = NULL) const = 0;
};
class  LatLongMapping : public EnvironmentMapping {
public:
	// LatLongMapping Public Methods
	LatLongMapping() {}
	virtual ~LatLongMapping() { }
	virtual void Map(const Vector &wh, float *s, float *t,
		float *pdf = NULL) const;
	virtual void Map(float s, float t, Vector *wh, float *pdf = NULL) const;
};
class  AngularMapping : public EnvironmentMapping {
public:
	// AngularMapping Public Methods
	AngularMapping() {}
	virtual ~AngularMapping() { }
	virtual void Map(const Vector &wh, float *s, float *t,
		float *pdf = NULL) const;
	virtual void Map(float s, float t, Vector *wh, float *pdf = NULL) const;
};
class  VerticalCrossMapping : public EnvironmentMapping {
public:
	// VerticalCross Public Methods
	VerticalCrossMapping() {}
	virtual ~VerticalCrossMapping() { }
	virtual void Map(const Vector &wh, float *s, float *t,
		float *pdf = NULL) const;
	virtual void Map(float s, float t, Vector *wh, float *pdf = NULL) const;
};

template <class T> class Texture {
public:
	//typedef boost::shared_ptr<Texture> TexturePtr; <<! Not working with GCC
	// Texture Interface
	virtual T Evaluate(const SpectrumWavelengths &sw,
		const DifferentialGeometry &) const = 0;
	float EvalFloat(const SpectrumWavelengths &sw,
		const DifferentialGeometry &dg) const;
	virtual float Y() const = 0;
	virtual float Filter() const { return Y(); }
	virtual void SetIlluminant() { }
	virtual void GetDuv(const SpectrumWavelengths &sw,
		const DifferentialGeometry &dg, float delta,
		float *du, float *dv) const = 0;
	virtual void GetMinMaxFloat(float *minValue, float *maxValue) const {
		LOG(LUX_WARNING, LUX_SYSTEM) << "Invalid call to Texture<T>::GetMinMaxFloat";
		*minValue = -1.f;
		*maxValue = 1.f;
	}
	virtual ~Texture() { }
};

template<> inline float Texture<float>::EvalFloat(const SpectrumWavelengths &sw,
	const DifferentialGeometry &dg) const
{
	return Evaluate(sw, dg);
}

float Noise(float x, float y = .5f, float z = .5f);
float Noise(const Point &P);
float FBm(const Point &P, const Vector &dpdx, const Vector &dpdy,
	float omega, int octaves);
float Turbulence(const Point &P, const Vector &dpdx, const Vector &dpdy,
	float omega, int octaves);
float Lanczos(float, float tau=2);

}//namespace lux

#endif // LUX_TEXTURE_H
