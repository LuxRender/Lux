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

/*
 * This code is adapted from Mitsuba renderer by Wenzel Jakob (http://www.mitsuba-renderer.org/)
 * by permission.
 *
 * This file implements the Irawan & Marschner BRDF,
 * a realistic model for rendering woven materials.
 * This spatially-varying reflectance model uses an explicit description
 * of the underlying weave pattern to create fine-scale texture and
 * realistic reflections across a wide range of different weave types.
 * To use the model, you must provide a special weave pattern
 * file---for an example of what these look like, see the
 * examples scenes available on the Mitsuba website.
 *
 * For reference, it is described in detail in the PhD thesis of
 * Piti Irawan ("The Appearance of Woven Cloth").
 *
 * The code in Mitsuba is a modified port of a previous Java implementation
 * by Piti.
 *
 */

// irawan.cpp*
#include "irawan.h"
#include "spectrum.h"
#include "mc.h"
#include "textures/blender_noiselib.h"

using namespace lux;

static uint64_t sampleTEA(uint32_t v0, uint32_t v1, u_int rounds = 4)
{
	uint32_t sum = 0;

	for (u_int i = 0; i < rounds; ++i) {
		sum += 0x9e3779b9;
		v0 += ((v1 << 4) + 0xA341316C) ^ (v1 + sum) ^ ((v1 >> 5) + 0xC8013EA4);
		v1 += ((v0 << 4) + 0xAD90777D) ^ (v0 + sum) ^ ((v0 >> 5) + 0x7E95761E);
	}

	return ((uint64_t) v1 << 32) + v0;
}

static float sampleTEAfloat(uint32_t v0, uint32_t v1, u_int rounds = 4)
{
	/* Trick from MTGP: generate an uniformly distributed
	   single precision number in [1,2) and subtract 1. */
	union {
		uint32_t u;
		float f;
	} x;
	x.u = ((sampleTEA(v0, v1, rounds) & 0xFFFFFFFF) >> 9) | 0x3f800000UL;
	return x.f - 1.0f;
}

static float perlinNoise(float x)
{
	return blender::newPerlin(x, 0.f, 0.f);
}

// von Mises Distribution
static float vonMises(float cos_x, float b)
{
	// assumes a = 0, b > 0 is a concentration parameter.

	const float factor = expf(b * cos_x) * INV_TWOPI;
	const float absB = fabsf(b);
	if (absB <= 3.75f) {
		const float t0 = absB / 3.75f;
		const float t = t0 * t0;
		return factor / (1.0f + t * (3.5156229f + t * (3.0899424f +
			t * (1.2067492f + t * (0.2659732f + t * (0.0360768f +
			t * 0.0045813f))))));
	} else {
		const float t = 3.75f / absB;
		return factor * sqrtf(absB) / (expf(absB) * (0.39894228f +
			t * (0.01328592f + t * (0.00225319f +
			t * (-0.00157565f + t * (0.00916281f +
			t * (-0.02057706f + t * (0.02635537f +
			t * (-0.01647633f + t * 0.00392377f)))))))));
	}
}

// Attenuation term
static float seeliger(float cos_th1, float cos_th2, float sg_a, float sg_s)
{
	const float al = sg_s / (sg_a + sg_s); // albedo
	const float c1 = max(0.f, cos_th1);
	const float c2 = max(0.f, cos_th2);
	if (c1 == 0.0f || c2 == 0.0f)
		return 0.0f;
	return al * INV_TWOPI * .5f * c1 * c2 / (c1 + c2);
}

float Warp::EvalIntegrand(const WeavePattern &weave, const Point &center,
	const Point &xy, Vector &om_i, Vector &om_r) const
{
	float umaxMod = umax;
	if (weave.period > 0.f) {
		/* Number of TEA iterations (the more, the better the
		   quality of the pseudorandom floats) */
		const int teaIterations = 8;

		// Correlated (Perlin) noise.
		// generate 1 seed per yarn segment
		const float random1 = perlinNoise((center.x *
			(weave.tileHeight * weave.repeat_v +
			sampleTEAfloat(center.x, 2.f * center.y,
			teaIterations)) + center.y) / weave.period);
		const float random2 = perlinNoise((center.y *
			(weave.tileWidth * weave.repeat_u +
			sampleTEAfloat(center.x, 2.f * center.y + 1.f,
			teaIterations)) + center.x) / weave.period);
		umaxMod += random1 * weave.dWarpUmaxOverDWarp +
			random2 * weave.dWarpUmaxOverDWeft;
	}

	// Compute u and v.
	// See Chapter 6.
	const float u = xy.y / (length / 2.0f) * umaxMod;
	const float v = xy.x * M_PI / width;

	if (psi != 0.0f)
		return EvalStapleIntegrand(weave, om_i, om_r, u, v, umaxMod)
		 * (weave.warpArea + weave.weftArea) / weave.warpArea;
	else
		return EvalFilamentIntegrand(weave, om_i, om_r, u, v, umaxMod)
		 * (weave.warpArea + weave.weftArea) / weave.warpArea;
}

float Weft::EvalIntegrand(const WeavePattern &weave, const Point &center,
	const Point &xy, Vector &om_i, Vector &om_r) const
{
	float umaxMod = umax;
	if (weave.period > 0.f) {
		/* Number of TEA iterations (the more, the better the
		   quality of the pseudorandom floats) */
		const int teaIterations = 8;

		// Correlated (Perlin) noise.
		// generate 1 seed per yarn segment
		const float random1 = perlinNoise((center.x *
			(weave.tileHeight * weave.repeat_v +
			sampleTEAfloat(center.x, 2.f * center.y,
			teaIterations)) + center.y) / weave.period);
		const float random2 = perlinNoise((center.y *
			(weave.tileWidth * weave.repeat_u +
			sampleTEAfloat(center.x, 2.f * center.y + 1.f,
			teaIterations)) + center.x) / weave.period);
		umaxMod += random1 * weave.dWeftUmaxOverDWarp +
			random2 * weave.dWeftUmaxOverDWeft;
	}

	// Compute u and v.
	// See Chapter 6.
	// Rotate pi/2 radians around z axis
	const float u = xy.x / (length / 2.0f) * umaxMod;
	const float v = -xy.y * M_PI / width;
	swap(om_i.x, om_i.y);
	om_i.x = -om_i.x;
	swap(om_r.x, om_r.y);
	om_r.x = -om_r.x;

	if (psi != 0.0f)
		return EvalStapleIntegrand(weave, om_i, om_r, u, v, umaxMod)
		 * (weave.warpArea + weave.weftArea) / weave.weftArea;
	else
		return EvalFilamentIntegrand(weave, om_i, om_r, u, v, umaxMod)
		  * (weave.warpArea + weave.weftArea) / weave.weftArea;
}

void Irawan::F(const SpectrumWavelengths &sw, const Vector &wo,
	const Vector &wi, SWCSpectrum *const f_) const
{
	const float scale = evalSpecular(wo, wi);
	*f_ += Ks * (scale * specularNormalization) *
		(fabsf(wo.z) * INV_PI);
}

bool Irawan::SampleF(const SpectrumWavelengths &sw, const Vector &wo,
	Vector *wi, float u1, float u2, SWCSpectrum *const f, float *pdf,
	float *pdfBack, bool reverse) const
{
	// Cosine-sample the hemisphere, flipping the direction if necessary
	*wi = CosineSampleHemisphere(u1, u2);
	if (wo.z < 0.f)
		wi->z = -(wi->z);
	*pdf = Pdf(sw, wo, *wi);
	if (pdfBack)
		*pdfBack = Pdf(sw, *wi, wo);
	float scale;
	if (reverse)
		scale = evalSpecular(*wi, wo);
	else
		scale = evalSpecular(wo, *wi);

	*f = Ks * (scale * specularNormalization);
	return true;
}

const Yarn *WeavePattern::GetYarn(float u_i, float v_i, Point *center, Point *xy) const
{
	const float u = u_i * repeat_u;
	const int bu = Floor2Int(u);
	const float ou = u - bu;
	const float v = v_i * repeat_v;
	const int bv = Floor2Int(v);
	const float ov = v - bv;
	const u_int lx = min(tileWidth - 1, Floor2UInt(ou * tileWidth));
	const u_int ly = tileHeight - 1 -
		min(tileHeight - 1, Floor2UInt(ov * tileHeight));

	const int yarnID = pattern[lx + tileWidth * ly] - 1;
	const Yarn * const yarn = yarns.at(yarnID);

	center->x = (bu + yarn->centerU) * tileWidth;
	center->y = (bv + yarn->centerV) * tileHeight;
	xy->x = (ou - yarn->centerU) * tileWidth;
	xy->y = (ov - yarn->centerV) * tileHeight;

	return yarn;
}

float Irawan::evalSpecular(const Vector &wo, const Vector &wi) const
{
	// Get incident and exitant directions.
	Vector om_i(wi);
	if (om_i.z < 0.f)
		om_i = -om_i;
	Vector om_r(wo);
	if (om_r.z < 0.f)
		om_r = -om_r;

	/* Number of TEA iterations (the more, the better the
	   quality of the pseudorandom floats) */
	const int teaIterations = 8;

	// Compute specular contribution.
	const float integrand = yarn->EvalIntegrand(*weave, center, xy, om_i, om_r);

	// Compute random variation and scale specular component.
	if (weave->fineness > 0.0f) {
		// Initialize random number generator based on texture location.
		// Generate fineness^2 seeds per 1 unit of texture.
		const uint32_t index1 = (uint32_t) ((center.x + xy.x) * weave->fineness);
		const uint32_t index2 = (uint32_t) ((center.y + xy.y) * weave->fineness);

		const float xi = sampleTEAfloat(index1, index2, teaIterations);
		return integrand * min(-logf(xi), 10.0f);
	}

	return integrand;
}

/** parameters:
 *	u	 to be compared to u(v) in texturing
 *	v	 for filament, we compute u(v)
 *	om_i  incident direction
 *	om_r  exitant direction
 *  yarn geometry
 *	psi   fiber twist angle; because this is filament, psi = pi/2
 *	umax  maximum inclination angle
 *	kappa spine curvature parameter
 *  weave pattern
 *	w	 width of segment rectangle
 *	l	 length of segment rectangle
 */
float Yarn::EvalFilamentIntegrand(const WeavePattern &weave, const Vector &om_i,
	const Vector &om_r, float u, float v, float umaxMod) const
{
	// 0 <= ss < 1.0
	if (weave.ss < 0.0f || weave.ss >= 1.0f)
		return 0.0f;

	// w * sin(umax) < l
	if (width * sinf(umaxMod) >= length)
		return 0.0f;

	// -1 < kappa < inf
	if (kappa < -1.0f)
		return 0.0f;

	// h is the half vector
	const Vector h(Normalize(om_r + om_i));

	// u_of_v is location of specular reflection.
	const float u_of_v = atan2f(h.y, h.z);

	// Check if u_of_v within the range of valid u values
	if (fabsf(u_of_v) >= umaxMod)
		return 0.f;

	// Highlight has constant width delta_y on screen.
	const float delta_y = length * weave.hWidth;

	// Clamp y_of_v between -(l - delta_y)/2 and (l - delta_y)/2.
	const float y_of_v = .5f * Clamp(u_of_v * length / umaxMod,
		delta_y - length, length - delta_y);

	// Check if |y(u(v)) - y(u)| < delta_y/2.
	if (fabsf(y_of_v - u * 0.5f * length / umaxMod) >= 0.5f * delta_y)
		return 0.f;

	// n is normal to the yarn surface
	// t is tangent of the fibers.
	const Normal n(Normalize(Normal(sinf(v), sinf(u_of_v) * cosf(v),
		cosf(u_of_v) * cosf(v))));
	const Vector t(Normalize(Vector(0.0f, cosf(u_of_v), -sinf(u_of_v))));

	// R is radius of curvature.
	const float R = RadiusOfCurvature(min(fabsf(u_of_v),
		(1.f - weave.ss) * umaxMod), (1.f - weave.ss) * umaxMod);

	// G is geometry factor.
	const float a = 0.5f * width;
	const Vector om_i_plus_om_r(om_i + om_r), t_cross_h(Cross(t, h));
	const float Gu = a * (R + a * cosf(v)) /
		(om_i_plus_om_r.Length() * fabsf(t_cross_h.x));

	// fc is phase function
	const float fc = weave.alpha + vonMises(-Dot(om_i, om_r), weave.beta);

	// attenuation function without smoothing.
	float As = seeliger(Dot(n, om_i), Dot(n, om_r), 0, 1);
	// As is attenuation function with smoothing.
	if (weave.ss > 0.0f)
		As *= SmoothStep(0.f, 1.f, (umaxMod - fabsf(u_of_v)) /
			(weave.ss * umaxMod));

	// fs is scattering function.
	const float fs = Gu * fc * As;

	// Domain transform.
	return fs * M_PI * length / delta_y;
}

/** parameters:
 *	u	 for staple, we compute v(u)
 *	v	 to be compared to v(u) in texturing
 *	om_i  incident direction
 *	om_r  exitant direction
 *  yarn geometry
 *	psi   fiber twist angle
 *	umax  maximum inclination angle
 *	kappa spine curvature parameter
 *  weave pattern
 *	w	 width of segment rectangle
 *	l	 length of segment rectangle
 */
float Yarn::EvalStapleIntegrand(const WeavePattern &weave, const Vector &om_i,
	const Vector &om_r, float u, float v, float umaxMod) const
{
	// w * sin(umax) < l
	if (width * sinf(umaxMod) >= length)
		return 0.0f;

	// -1 < kappa < inf
	if (kappa < -1.0f)
		return 0.0f;

	// h is the half vector
	const Vector h(Normalize(om_i + om_r));

	// v_of_u is location of specular reflection.
	const float D = (h.y * cosf(u) - h.z * sinf(u)) /
		(sqrtf(h.x * h.x + powf(h.y * sinf(u) + h.z * cosf(u),
		2.0f)) * tanf(psi));
	if (!(fabsf(D) < 1.f))
		return 0.f;
	const float v_of_u = atan2f(-h.y * sinf(u) - h.z * cosf(u), h.x) +
		acosf(D);

	// Highlight has constant width delta_x on screen.
	const float delta_x = width * weave.hWidth;

	// Clamp x_of_u between (w - delta_x)/2 and -(w - delta_x)/2.
	const float x_of_u = Clamp(v_of_u * width / M_PI,
		delta_x - width, width - delta_x);

	// Check if |x(v(u)) - x(v)| < delta_x/2.
	if (fabsf(x_of_u - v * width / M_PI) >= 0.5f * delta_x)
		return 0.f;

	// n is normal to the yarn surface.
	const Vector n(Normalize(Vector(sinf(v_of_u), sinf(u) * cosf(v_of_u),
		cosf(u) * cosf(v_of_u))));

	// R is radius of curvature.
	const float R = RadiusOfCurvature(fabsf(u), umaxMod);

	// G is geometry factor.
	const float a = 0.5f * width;
	const Vector om_i_plus_om_r(om_i + om_r);
	const float Gv = a * (R + a * cosf(v_of_u)) /
		(om_i_plus_om_r.Length() * Dot(n, h) * fabsf(sinf(psi)));

	// fc is phase function.
	const float fc = weave.alpha + vonMises(-Dot(om_i, om_r), weave.beta);

	// A is attenuation function without smoothing.
	const float A = seeliger(Dot(n, om_i), Dot(n, om_r), 0, 1);

	// fs is scattering function.
	const float fs = Gv * fc * A;

	// Domain transform.
	return fs * 2.0f * width * umaxMod / delta_x;
}

float Yarn::RadiusOfCurvature(float u, float umaxMod) const
{
	// rhat determines whether the spine is a segment
	// of an ellipse, a parabole, or a hyperbola.
	// See Section 5.3.
	const float rhat = 1.0f + kappa * (1.0f + 1.0f / tanf(umaxMod));

	const float a = 0.5f * width;
	if (rhat == 1.0f) { // circle; see Subsection 5.3.1.
		return 0.5f * length / sinf(umaxMod) - a;
	} else if (rhat > 0.0f) { // ellipsis
		const float tmax = atanf(rhat * tanf(umaxMod));
		const float bhat = (0.5f * length - a * sinf(umaxMod)) / sinf(tmax);
		const float ahat = bhat / rhat;
		const float t = atanf(rhat * tanf(u));
		return powf(bhat * bhat * cosf(t) * cosf(t) +
			ahat * ahat * sinf(t) * sinf(t), 1.5f) / (ahat * bhat);
	} else if (rhat < 0.0f) { // hyperbola; see Subsection 5.3.3.
		const float tmax = -atanhf(rhat * tanf(umaxMod));
		const float bhat = (0.5f * length - a * sinf(umaxMod)) / sinhf(tmax);
		const float ahat = bhat / rhat;
		const float t = -atanhf(rhat * tanf(u));
		return -powf(bhat * bhat * coshf(t) * coshf(t) +
			ahat * ahat * sinhf(t) * sinhf(t), 1.5f) / (ahat * bhat);
	} else { // rhat == 0  // parabola; see Subsection 5.3.2.
		const float tmax = tanf(umaxMod);
		const float ahat = (0.5f * length - a * sinf(umaxMod)) / (2.f * tmax);
		const float t = tanf(u);
		return 2.f * ahat * powf(1.f + t * t, 1.5f);
	}
}
