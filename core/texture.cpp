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

// texture.cpp*
#include "texture.h"
#include "paramset.h"
#include "shape.h"
#include "spectrum.h"
#include "fresnelgeneral.h"

namespace lux
{

// Texture Forward Declarations
inline float Grad(int x, int y, int z, float dx, float dy, float dz);
inline float NoiseWeight(float t);
// Perlin Noise Data
#define NOISE_PERM_SIZE 256
static int NoisePerm[2 * NOISE_PERM_SIZE] = {
	151, 160, 137, 91, 90, 15, 131, 13, 201, 95, 96,
	53, 194, 233, 7, 225, 140, 36, 103, 30, 69, 142,
	// Rest of noise permutation table
	8, 99, 37, 240, 21, 10, 23,
	   190,  6, 148, 247, 120, 234, 75, 0, 26, 197, 62, 94, 252, 219, 203, 117, 35, 11, 32, 57, 177, 33,
	   88, 237, 149, 56, 87, 174, 20, 125, 136, 171, 168,  68, 175, 74, 165, 71, 134, 139, 48, 27, 166,
	   77, 146, 158, 231, 83, 111, 229, 122, 60, 211, 133, 230, 220, 105, 92, 41, 55, 46, 245, 40, 244,
	   102, 143, 54,  65, 25, 63, 161,  1, 216, 80, 73, 209, 76, 132, 187, 208,  89, 18, 169, 200, 196,
	   135, 130, 116, 188, 159, 86, 164, 100, 109, 198, 173, 186,  3, 64, 52, 217, 226, 250, 124, 123,
	   5, 202, 38, 147, 118, 126, 255, 82, 85, 212, 207, 206, 59, 227, 47, 16, 58, 17, 182, 189, 28, 42,
	   223, 183, 170, 213, 119, 248, 152,  2, 44, 154, 163,  70, 221, 153, 101, 155, 167,  43, 172, 9,
	   129, 22, 39, 253,  19, 98, 108, 110, 79, 113, 224, 232, 178, 185,  112, 104, 218, 246, 97, 228,
	   251, 34, 242, 193, 238, 210, 144, 12, 191, 179, 162, 241,  81, 51, 145, 235, 249, 14, 239, 107,
	   49, 192, 214,  31, 181, 199, 106, 157, 184,  84, 204, 176, 115, 121, 50, 45, 127,  4, 150, 254,
	   138, 236, 205, 93, 222, 114, 67, 29, 24, 72, 243, 141, 128, 195, 78, 66, 215, 61, 156, 180,
	   151, 160, 137, 91, 90, 15,
	   131, 13, 201, 95, 96, 53, 194, 233, 7, 225, 140, 36, 103, 30, 69, 142, 8, 99, 37, 240, 21, 10, 23,
	   190,  6, 148, 247, 120, 234, 75, 0, 26, 197, 62, 94, 252, 219, 203, 117, 35, 11, 32, 57, 177, 33,
	   88, 237, 149, 56, 87, 174, 20, 125, 136, 171, 168,  68, 175, 74, 165, 71, 134, 139, 48, 27, 166,
	   77, 146, 158, 231, 83, 111, 229, 122, 60, 211, 133, 230, 220, 105, 92, 41, 55, 46, 245, 40, 244,
	   102, 143, 54,  65, 25, 63, 161,  1, 216, 80, 73, 209, 76, 132, 187, 208,  89, 18, 169, 200, 196,
	   135, 130, 116, 188, 159, 86, 164, 100, 109, 198, 173, 186,  3, 64, 52, 217, 226, 250, 124, 123,
	   5, 202, 38, 147, 118, 126, 255, 82, 85, 212, 207, 206, 59, 227, 47, 16, 58, 17, 182, 189, 28, 42,
	   223, 183, 170, 213, 119, 248, 152,  2, 44, 154, 163,  70, 221, 153, 101, 155, 167,  43, 172, 9,
	   129, 22, 39, 253,  19, 98, 108, 110, 79, 113, 224, 232, 178, 185,  112, 104, 218, 246, 97, 228,
	   251, 34, 242, 193, 238, 210, 144, 12, 191, 179, 162, 241,  81, 51, 145, 235, 249, 14, 239, 107,
	   49, 192, 214,  31, 181, 199, 106, 157, 184,  84, 204, 176, 115, 121, 50, 45, 127,  4, 150, 254,
	   138, 236, 205, 93, 222, 114, 67, 29, 24, 72, 243, 141, 128, 195, 78, 66, 215, 61, 156, 180
};
// Texture Method Definitions
UVMapping2D::UVMapping2D(float _su, float _sv, float _du, float _dv)
{
	su = _su;
	sv = _sv;
	du = _du;
	dv = _dv;
}
void UVMapping2D::Map(const DifferentialGeometry &dg, float *s, float *t) const
{
	*s = su * dg.u + du;
	*t = sv * dg.v + dv;
}
void UVMapping2D::MapDxy(const DifferentialGeometry &dg, float *s, float *t,
	float *dsdx, float *dtdx, float *dsdy, float *dtdy) const
{
	Map(dg, s, t);
	// Compute texture differentials for 2D identity mapping
	*dsdx = su * dg.dudx;
	*dtdx = sv * dg.dvdx;
	*dsdy = su * dg.dudy;
	*dtdy = sv * dg.dvdy;
}
void UVMapping2D::MapDuv(const DifferentialGeometry &dg, float *s, float *t,
	float *dsdu, float *dtdu, float *dsdv, float *dtdv) const
{
	Map(dg, s, t);
	// Compute texture differentials for 2D identity mapping
	*dsdu = su;
	*dsdv = 0.f;
	*dtdu = 0.f;
	*dtdv = sv;
}

void SphericalMapping2D::Map(const DifferentialGeometry &dg,
	float *s, float *t) const
{
	const Vector p(Normalize(Vector(WorldToTexture(dg.p))));
	*s = SphericalPhi(p)   * scaledInvTwoPi + du;
	*t = SphericalTheta(p) * scaledInvPi    + dv;
}
void SphericalMapping2D::MapDxy(const DifferentialGeometry &dg,
	float *s, float *t,
	float *dsdx, float *dtdx, float *dsdy, float *dtdy) const
{
	const Vector p(Normalize(Vector(WorldToTexture(dg.p))));
	*s = SphericalPhi(p)   * scaledInvTwoPi + du;
	*t = SphericalTheta(p) * scaledInvPi    + dv;
	// Compute texture coordinate differentials for sphere $(u,v)$ mapping
	const Vector dpdx(WorldToTexture(dg.dpdx));
	const Vector dpdy(WorldToTexture(dg.dpdy));
	const float scaledInvTwoPiOverXY2 = scaledInvTwoPi / (p.x * p.x + p.y * p.y);
	*dsdx = (dpdx.y * p.x - p.y * dpdx.x) * scaledInvTwoPiOverXY2;
	*dsdy = (dpdy.y * p.x - p.y * dpdy.x) * scaledInvTwoPiOverXY2;
	const float scaledInvPiOverHypo = scaledInvPi / sqrtf(1.f - p.z * p.z);
	*dtdx = dpdx.z * scaledInvPiOverHypo;
	*dtdy = dpdy.z * scaledInvPiOverHypo;
}
void SphericalMapping2D::MapDuv(const DifferentialGeometry &dg,
	float *s, float *t,
	float *dsdu, float *dtdu, float *dsdv, float *dtdv) const
{
	const Vector p(Normalize(Vector(WorldToTexture(dg.p))));
	*s = SphericalPhi(p)   * scaledInvTwoPi + du;
	*t = SphericalTheta(p) * scaledInvPi    + dv;
	// Compute texture coordinate differentials for sphere $(u,v)$ mapping
	const Vector dpdu(WorldToTexture(dg.dpdu));
	const Vector dpdv(WorldToTexture(dg.dpdv));
	const float scaledInvTwoPiOverXY2 = scaledInvTwoPi / (p.x * p.x + p.y * p.y);
	*dsdu = (dpdu.y * p.x - p.y * dpdu.x) * scaledInvTwoPiOverXY2;
	*dsdv = (dpdv.y * p.x - p.y * dpdv.x) * scaledInvTwoPiOverXY2;
	const float scaledInvPiOverHypo = scaledInvPi / sqrtf(1.f - p.z * p.z);
	*dtdu = dpdu.z * scaledInvPiOverHypo;
	*dtdv = dpdv.z * scaledInvPiOverHypo;
}

void CylindricalMapping2D::Map(const DifferentialGeometry &dg,
	float *s, float *t) const
{
	Vector p(WorldToTexture(dg.p));
	const float lenXY = sqrtf(p.x * p.x + p.y * p.y);
	p.x /= lenXY;
	p.y /= lenXY;
	*s = SphericalPhi(p) * scaledInvTwoPi + du;
	*t = 0.5f - 0.5f * p.z;
}
void CylindricalMapping2D::MapDxy(const DifferentialGeometry &dg,
	float *s, float *t,
	float *dsdx, float *dtdx, float *dsdy, float *dtdy) const
{
	Vector p(WorldToTexture(dg.p));
	const float lenXY = sqrtf(p.x * p.x + p.y * p.y);
	p.x /= lenXY;
	p.y /= lenXY;
	*s = SphericalPhi(p) * scaledInvTwoPi + du;
	*t = 0.5f - 0.5f * p.z;
	// Compute texture coordinate differentials for cylinder $(u,v)$ mapping
	const Vector dpdx(WorldToTexture(dg.dpdx));
	const Vector dpdy(WorldToTexture(dg.dpdy));
	*dsdx = (dpdx.y * p.x - p.y * dpdx.x) * scaledInvTwoPi;
	*dsdy = (dpdy.y * p.x - p.y * dpdy.x) * scaledInvTwoPi;
	*dtdx = -0.5f * dpdx.z;
	*dtdy = -0.5f * dpdy.z;
}
void CylindricalMapping2D::MapDuv(const DifferentialGeometry &dg,
	float *s, float *t,
	float *dsdu, float *dtdu, float *dsdv, float *dtdv) const
{
	Vector p(WorldToTexture(dg.p));
	const float lenXY = sqrtf(p.x * p.x + p.y * p.y);
	p.x /= lenXY;
	p.y /= lenXY;
	*s = SphericalPhi(p) * scaledInvTwoPi + du;
	*t = 0.5f - 0.5f * p.z;
	// Compute texture coordinate differentials for cylinder $(u,v)$ mapping
	const Vector dpdu(WorldToTexture(dg.dpdu));
	const Vector dpdv(WorldToTexture(dg.dpdv));
	*dsdu = (dpdu.y * p.x - p.y * dpdu.x) * scaledInvTwoPi;
	*dsdv = (dpdv.y * p.x - p.y * dpdv.x) * scaledInvTwoPi;
	*dtdu = -0.5f * dpdu.z;
	*dtdv = -0.5f * dpdv.z;
}

PlanarMapping2D::PlanarMapping2D(const Vector &_v1, const Vector &_v2,
	float _ds, float _dt)
{
	vs = _v1;
	vt = _v2;
	ds = _ds;
	dt = _dt;
}
void PlanarMapping2D::Map(const DifferentialGeometry &dg,
	float *s, float *t) const
{
	const Vector p(dg.p);
	*s = ds + Dot(p, vs);
	*t = dt + Dot(p, vt);
}
void PlanarMapping2D::MapDxy(const DifferentialGeometry &dg, float *s, float *t,
	float *dsdx, float *dtdx, float *dsdy, float *dtdy) const
{
	Map(dg, s, t);
	*dsdx = Dot(dg.dpdx, vs);
	*dtdx = Dot(dg.dpdx, vt);
	*dsdy = Dot(dg.dpdy, vs);
	*dtdy = Dot(dg.dpdy, vt);
}
void PlanarMapping2D::MapDuv(const DifferentialGeometry &dg, float *s, float *t,
	float *dsdu, float *dtdu, float *dsdv, float *dtdv) const
{
	Map(dg, s, t);
	*dsdu = Dot(dg.dpdu, vs);
	*dtdu = Dot(dg.dpdu, vt);
	*dsdv = Dot(dg.dpdv, vs);
	*dtdv = Dot(dg.dpdv, vt);
}

Point IdentityMapping3D::Map(const DifferentialGeometry &dg) const
{
	return WorldToTexture(dg.p);
}
Point IdentityMapping3D::MapDxy(const DifferentialGeometry &dg,
	Vector *dpdx, Vector *dpdy) const
{
	*dpdx = WorldToTexture(dg.dpdx);
	*dpdy = WorldToTexture(dg.dpdy);
	return Map(dg);
}
Point IdentityMapping3D::MapDuv(const DifferentialGeometry &dg,
	Vector *dpdu, Vector *dpdv) const
{
	*dpdu = WorldToTexture(dg.dpdu);
	*dpdv = WorldToTexture(dg.dpdv);
	return Map(dg);
}
void IdentityMapping3D::Apply3DTextureMappingOptions(const ParamSet &tp)
{
	// Apply inverted scale
	Vector scale = tp.FindOneVector("scale", Vector(1.f, 1.f, 1.f));
	WorldToTexture = WorldToTexture * Scale(1.f / scale.x, 1.f / scale.y,
		1.f / scale.z);
	// Apply rotations on X Y and Z axii
	Vector rotate = tp.FindOneVector("rotate", Vector(0.f, 0.f, 0.f));
	WorldToTexture = WorldToTexture * RotateX(rotate.x);
	WorldToTexture = WorldToTexture * RotateY(rotate.y);
	WorldToTexture = WorldToTexture * RotateZ(rotate.z);
	// Apply negated Translation
	Vector translate = tp.FindOneVector("translate", Vector(0.f, 0.f, 0.f));
	WorldToTexture = WorldToTexture * Translate(-translate);
}

void LatLongMapping::Map(const Vector &wh, float *s, float *t, float *pdf) const
{
	const float theta = SphericalTheta(wh);
	*s = SphericalPhi(wh) * INV_TWOPI;
	*t = theta * INV_PI;
	if (pdf)
		*pdf = INV_TWOPI * INV_PI / sinf(theta);
}
void LatLongMapping::Map(float s, float t, Vector *wh, float *pdf) const
{
	const float phi = s * 2.f * M_PI;
	const float theta = t * M_PI;
	const float sinTheta = sinf(theta);
	*wh = SphericalDirection(sinTheta, cosf(theta), phi);
	if (pdf)
		*pdf = INV_TWOPI * INV_PI / sinTheta;
}

void AngularMapping::Map(const Vector &wh, float *s, float *t, float *pdf) const
{
	const float sinTheta = sqrtf(wh.y*wh.y + wh.z*wh.z);
	const float r = INV_TWOPI * acosf(Clamp(-wh.x, -1.f, 1.f));
	if (sinTheta > 1e-9f) {
		const float factor = r / sinTheta;
		*s = 0.5f - wh.y * factor;
		*t = 0.5f - wh.z * factor;
	} else if (fabsf(wh.y) > fabsf(wh.z)) {
		*s = 0.5f * (1.f - SignOf(wh.y));
		*t = 0.5f;
	} else {
		*s = 0.5f;
		*t = 0.5f * (1.f - SignOf(wh.z));
	}
	if (pdf) {
		if (r > 1e-9f)
			*pdf = INV_TWOPI * sinTheta / r;
		else
			*pdf = 1.f;
	}
}
void AngularMapping::Map(float s, float t, Vector *wh, float *pdf) const
{
	const float r = sqrtf((s - .5f) * (s - .5f) + (t - .5f) * (t - .5f));
	if (r > .5f) {
		if (pdf)
			*pdf = 0.f;
		return;
	}
	const float theta = 2.f * M_PI * r;
	wh->x = -cosf(theta);
	const float phi = atan2f(t - .5f, s - .5f);
	const float sinTheta = sinf(theta);
	wh->y = sinTheta * cosf(phi);
	wh->z = sinTheta * sinf(phi);
	if (pdf) {
		if (r > 1e-9f)
			*pdf = INV_TWOPI * sinTheta / r;
		else
			*pdf = 1.f;
	}
}

void VerticalCrossMapping::Map(const Vector &wh, float *s, float *t, float *pdf) const {
	int axis = 0;
	float ma = fabsf(wh.x);
	if (fabsf(wh.y) > ma) {
		ma = fabsf(wh.y);
		axis = 1;
	}
	if (fabsf(wh.z) > ma) {
		ma = fabsf(wh.z);
		axis = 2;
	}
	const float ima = 1.f / ma;
	float sc = 0.0f, tc = 0.0f;
	float so = 0.0f, to = 0.0f;
	// select cube face based on major axis
	switch (axis) {
		case 0:
			if (wh.x > 0) {
				sc = -wh.y;
				tc = wh.z;
				so = 1.f;
				to = 3.f;
			} else {
				sc = -wh.y;
				tc = -wh.z;
				so = 1.f;
				to = 1.f;
			}
			break;
		case 1:
			if (wh.y > 0) {
				sc = -wh.x;
				tc = -wh.z;
				so = 0.f;
				to = 1.f;
			} else {
				sc = wh.x;
				tc = -wh.z;
				so = 2.f;
				to = 1.f;
			}
			break;
		case 2:
			if (wh.z > 0) {
				sc = -wh.y;
				tc = -wh.x;
				so = 1.f;
				to = 0.f;
			} else {
				sc = -wh.y;
				tc = wh.x;
				so = 1.f;
				to = 2.f;
			}
			break;
	}
	*s = Clamp((sc * ima + 1.f) * 0.5f, 0.f, 1.f);
	*t = Clamp((tc * ima + 1.f) * 0.5f, 0.f, 1.f);
	// rescale and offset to correct cube face in cross
	*s = (*s + so) * (1.f / 3.f);
	*t = (*t + to) * (1.f / 4.f);
	if (pdf)
		*pdf = ima * ima * ima / 48.f;
}
void VerticalCrossMapping::Map(float s, float t, Vector *wh, float *pdf) const
{
	const u_int so = min(2U, Floor2UInt(3.f * s));
	const u_int to = min(3U, Floor2UInt(4.f * t));
	const float sc = (3.f * s - so) * 2.f - 1.f;
	const float tc = (4.f * t - to) * 2.f - 1.f;
	switch (4 * so + to) {
		case 1:
			*wh = Vector(-sc, 1.f, -tc);
			break;
		case 4:
			*wh = Vector(-tc, -sc, 1.f);
			break;
		case 5:
			*wh = Vector(-1.f, -sc, -tc);
			break;
		case 6:
			*wh = Vector(tc, -sc, -1.f);
			break;
		case 7:
			*wh = Vector(1.f, -sc, tc);
			break;
		case 9:
			*wh = Vector(sc, -1.f, -tc);
			break;
		default:
			if (pdf)
				*pdf = 0.f;
			return;
	}
	const float ima = 1.f / wh->Length();
	*wh *= ima;
	if (pdf)
		*pdf = ima * ima * ima / 48.f;
}

template<> float Texture<SWCSpectrum>::EvalFloat(const SpectrumWavelengths &sw,
	const DifferentialGeometry &dg) const
{
	return Evaluate(sw, dg).Filter(sw);
}
template<> float Texture<FresnelGeneral>::EvalFloat(const SpectrumWavelengths &sw,
	const DifferentialGeometry &dg) const
{
	return Evaluate(sw, dg).Index(sw);
}

// Texture Function Definitions
float Noise(float x, float y, float z)
{
	// Compute noise cell coordinates and offsets
	int ix = Floor2Int(x);
	int iy = Floor2Int(y);
	int iz = Floor2Int(z);
	const float dx = x - ix, dy = y - iy, dz = z - iz;
	// Compute gradient weights
	ix &= (NOISE_PERM_SIZE-1);
	iy &= (NOISE_PERM_SIZE-1);
	iz &= (NOISE_PERM_SIZE-1);
	const float w000 = Grad(ix,   iy,   iz,   dx,   dy,   dz);
	const float w100 = Grad(ix+1, iy,   iz,   dx-1, dy,   dz);
	const float w010 = Grad(ix,   iy+1, iz,   dx,   dy-1, dz);
	const float w110 = Grad(ix+1, iy+1, iz,   dx-1, dy-1, dz);
	const float w001 = Grad(ix,   iy,   iz+1, dx,   dy,   dz-1);
	const float w101 = Grad(ix+1, iy,   iz+1, dx-1, dy,   dz-1);
	const float w011 = Grad(ix,   iy+1, iz+1, dx,   dy-1, dz-1);
	const float w111 = Grad(ix+1, iy+1, iz+1, dx-1, dy-1, dz-1);
	// Compute trilinear interpolation of weights
	const float wx = NoiseWeight(dx);
	const float wy = NoiseWeight(dy);
	const float wz = NoiseWeight(dz);
	const float x00 = Lerp(wx, w000, w100);
	const float x10 = Lerp(wx, w010, w110);
	const float x01 = Lerp(wx, w001, w101);
	const float x11 = Lerp(wx, w011, w111);
	const float y0 = Lerp(wy, x00, x10);
	const float y1 = Lerp(wy, x01, x11);
	return Lerp(wz, y0, y1);
}
float Noise(const Point &P)
{
	return Noise(P.x, P.y, P.z);
}

inline float Grad(int x, int y, int z, float dx, float dy, float dz)
{
 	const int h = NoisePerm[NoisePerm[NoisePerm[x] + y] + z] & 15;
	const float u = h < 8 || h == 12 || h == 13 ? dx : dy;
	const float v = h < 4 || h == 12 || h == 13 ? dy : dz;
	return ((h&1) ? -u : u) + ((h&2) ? -v : v);
}

inline float NoiseWeight(float t)
{
	const float t3 = t * t * t;
	const float t4 = t3 * t;
	return 6.f*t4*t - 15.f*t4 + 10.f*t3;
}

float FBm(const Point &P, const Vector &dpdx, const Vector &dpdy,
	float omega, int maxOctaves)
{
	// Compute number of octaves for anti-aliased FBm
	const float s2 = max(dpdx.LengthSquared(), dpdy.LengthSquared());
	const float foctaves = min(static_cast<float>(maxOctaves),
	                     1.f - .5f * Log2(s2));
	const int octaves = Floor2Int(foctaves);
	// Compute sum of octaves of noise for FBm
	float sum = 0.f, lambda = 1.f, o = 1.f;
	for (int i = 0; i < octaves; ++i) {
		sum += o * Noise(lambda * P);
		lambda *= 1.99f;
		o *= omega;
	}
	const float partialOctave = foctaves - static_cast<float>(octaves);
	sum += o * SmoothStep(.3f, .7f, partialOctave) *
	       Noise(lambda * P);
	return sum;
}

float Turbulence(const Point &P, const Vector &dpdx, const Vector &dpdy,
	float omega, int maxOctaves)
{
	// Compute number of octaves for anti-aliased FBm
	const float s2 = max(dpdx.LengthSquared(), dpdy.LengthSquared());
	const float foctaves = min(static_cast<float>(maxOctaves),
	                     1.f - .5f * Log2(s2));
	const int octaves = Floor2Int(foctaves);
	// Compute sum of octaves of noise for turbulence
	float sum = 0.f, lambda = 1.f, o = 1.f;
	for (int i = 0; i < octaves; ++i) {
		sum += o * fabsf(Noise(lambda * P));
		lambda *= 1.99f;
		o *= omega;
	}
	const float partialOctave = foctaves - static_cast<float>(octaves);
	sum += o * SmoothStep(.3f, .7f, partialOctave) *
	       fabsf(Noise(lambda * P));
	return sum;
}

float Lanczos(float x, float tau)
{
	x = fabsf(x);
	if (x < 1e-5f)
		return 1.f;
	else if (x > 1.f)
		return 0.f;
	x *= M_PI;
	const float s = sinf(x * tau) / (x * tau);
	const float lanczos = sinf(x) / x;
	return s * lanczos;
}
 
}//namespace lux

