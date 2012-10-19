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
using namespace blender;

inline uint64_t sampleTEA(uint32_t v0, uint32_t v1, int rounds = 4) {
	uint32_t sum = 0;

	for (int i=0; i<rounds; ++i) {
		sum += 0x9e3779b9;
		v0 += ((v1 << 4) + 0xA341316C) ^ (v1 + sum) ^ ((v1 >> 5) + 0xC8013EA4);
		v1 += ((v0 << 4) + 0xAD90777D) ^ (v0 + sum) ^ ((v0 >> 5) + 0x7E95761E);
	}

	return ((uint64_t) v1 << 32) + v0;
}

inline float sampleTEAfloat(uint32_t v0, uint32_t v1, int rounds = 4) {
	/* Trick from MTGP: generate an uniformly distributed
	   single precision number in [1,2) and subtract 1. */
	union {
		uint32_t u;
		float f;
	} x;
	x.u = ((sampleTEA(v0, v1, rounds) & 0xFFFFFFFF) >> 9) | 0x3f800000UL;
	return x.f - 1.0f;
}

float perlinNoise(Point p) {
	return blender::newPerlin(p.x, p.y, p.z);
}

// Always-positive modulo function (assumes b > 0)
inline int modulo(int a, int b) {
	int r = a % b;
	return (r < 0) ? r+b : r;
}

// Always-positive modulo function, float version (assumes b > 0)
inline float modulo(float a, float b) {
	float r = std::fmod(a, b);
	return (r < 0) ? r+b : r;
}

inline float atanh(float arg) {
	return logf((1.0f + arg) / (1.0f - arg)) / 2.0f;
}

// von Mises Distribution
float vonMises(float cos_x, float b) {
	// assumes a = 0, b > 0 is a concentration parameter.

	float I0, absB = std::abs(b);
	if (std::abs(b) <= 3.75f) {
		float t = absB / 3.75f;
		t = t * t;
		I0 = 1.0f + t*(3.5156229f + t*(3.0899424f + t*(1.2067492f
				+ t*(0.2659732f + t*(0.0360768f + t*0.0045813f)))));
	} else {
		float t = 3.75f / absB;
		I0 = expf(absB) / std::sqrt(absB) * (0.39894228f + t*(0.01328592f
			+ t*(0.00225319f + t*(-0.00157565f + t*(0.00916281f + t*(-0.02057706f
			+ t*(0.02635537f + t*(-0.01647633f + t*0.00392377f))))))));
	}

	return expf(b * cos_x) / (2 * M_PI * I0);
}

// Attenuation term
float seeliger(float cos_th1, float cos_th2, float sg_a, float sg_s) {
	float al = sg_s / (sg_a + sg_s); // albedo
	float c1 = std::max((float) 0, cos_th1);
	float c2 = std::max((float) 0, cos_th2);
	if (c1 == 0.0f || c2 == 0.0f)
		return 0.0f;
	return al / (4.0f * M_PI) * c1 * c2 / (c1 + c2);
}

// S-shaped smoothly varying interpolation between two values
float smoothStep(float min, float max, float value) {
	float v = Clamp((value - min) / (max - min), (float) 0, (float) 1);
	return v * v * (-2 * v  + 3);
}

// Arccosine variant that gracefully handles arguments > 1 that are due to roundoff errors
float safe_acos(float value) {
	return std::acos(std::min(1.0f, std::max(-1.0f, value)));
}

void Irawan::F(const SpectrumWavelengths &sw, const Vector &wo,
	const Vector &wi, SWCSpectrum *const f_) const
{
	eval(wo, wi, U, V, f_, false);
}

void Irawan::eval(const Vector &wo, const Vector &wi, const float u_i, const float v_i, SWCSpectrum *const f_, bool init) const
{
	if (CosTheta(wi) <= 0 ||
		CosTheta(wo) <= 0)
		return;

	Point uv = Point(u_i * weave->repeat_u, (1 - v_i) * weave->repeat_v);
	Point xy(uv.x * weave->tileWidth, uv.y * weave->tileHeight);

	Point lookup(
		modulo((int) xy.x, weave->tileWidth),
		modulo((int) xy.y, weave->tileHeight));

	int yarnID = weave->pattern[(u_int)lookup.x + (u_int)lookup.y * weave->tileWidth] - 1;

	Yarn *yarn = weave->yarns.at(yarnID);
	// store center of the yarn segment
	Point center
		(((int) xy.x / weave->tileWidth) * weave->tileWidth
			+ yarn->centerU * weave->tileWidth,
		 ((int) xy.y / weave->tileHeight) * weave->tileHeight
			+ (1 - yarn->centerV) * weave->tileHeight);

	// transform x and y to new coordinate system with (0,0) at the
	// center of the yarn segment
	xy.x =	  xy.x - center.x;
	xy.y = - (xy.y - center.y);

	int type = yarn->type;
	float w = yarn->width;
	float l = yarn->length;

	SWCSpectrum kd = (yarn->type == Yarn::EWarp) ? warp_Kd : weft_Kd;
	SWCSpectrum ks = (yarn->type == Yarn::EWarp) ? warp_Ks : weft_Ks;

	// Get incident and exitant directions.
	Vector om_i = wi;
	Vector om_r = wo;

	float psi = yarn->psi;
	float umax = yarn->umax;
	float kappa = yarn->kappa;

	float dUmaxOverDWarp, dUmaxOverDWeft;
	if (type == Yarn::EWarp) {
		dUmaxOverDWarp = weave->dWarpUmaxOverDWarp;
		dUmaxOverDWeft = weave->dWarpUmaxOverDWeft;
	} else { // type == EWeft
		dUmaxOverDWarp = weave->dWeftUmaxOverDWarp;
		dUmaxOverDWeft = weave->dWeftUmaxOverDWeft;
		// Rotate xy, incident, and exitant directions pi/2 radian about z-axis
		float tmp = xy.x;
		xy.x = -xy.y;
		xy.y = tmp;
		tmp = om_i.x;
		om_i.x = -om_i.y;
		om_i.y = tmp;
		tmp = om_r.x;
		om_r.x = -om_r.y;
		om_r.y = tmp;
	}

	// Correlated (Perlin) noise.
	float random1 = 1.0f;
	float random2 = 1.0f;

	/* Number of TEA iterations (the more, the better the
	   quality of the pseudorandom floats) */
	const int teaIterations = 8;

	if (weave->period > 0.0f) {
		// generate 1 seed per yarn segment
		Point pos(center);

		random1 = perlinNoise(Point(
			(center.x * (weave->tileHeight * weave->repeat_v
				+ sampleTEAfloat(pos.x, 2*pos.y, teaIterations)) + center.y) / weave->period, 0, 0));
		random2 = perlinNoise(Point(
			(center.y * (weave->tileWidth * weave->repeat_u
				+ sampleTEAfloat(pos.x, 2*pos.y+1, teaIterations)) + center.x) / weave->period, 0, 0));
		umax = umax + random1 * dUmaxOverDWarp + random2 * dUmaxOverDWeft;
	}

	// Compute u and v.
	// See Chapter 6.
	float u = xy.y / (l / 2.0f) * umax;
	float v = xy.x * M_PI / w;

	// Compute specular contribution.
	SWCSpectrum result(0.0f);
	float integrand;
	if (psi != 0.0f)
		integrand = evalStapleIntegrand(u, v, om_i, om_r, weave->alpha,
				weave->beta, psi, umax, kappa, w, l);
	else
		integrand = evalFilamentIntegrand(u, v, om_i, om_r, weave->alpha,
				weave->beta, weave->ss, umax, kappa, w, l);

	// Initialize random number generator based on texture location.
	float intensityVariation = 1.0f;
	if (weave->fineness > 0.0f) {
		// Compute random variation and scale specular component.
		// Generate fineness^2 seeds per 1 unit of texture.
		uint32_t index1 = (uint32_t) ((center.x + xy.x) * weave->fineness);
		uint32_t index2 = (uint32_t) ((center.y + xy.y) * weave->fineness);

		float xi = sampleTEAfloat(index1, index2, teaIterations);
		intensityVariation = std::min(-logf(xi), (float) 10.0f);
	}

	// Only used when estimating normalization factor
	if (!init)
		result = ks * (intensityVariation * integrand * specularNormalization);
	else
		result = SWCSpectrum(intensityVariation * integrand);

	if (type == Yarn::EWarp)
		result *= (weave->warpArea + weave->weftArea) / weave->warpArea;
	else
		result *= (weave->warpArea + weave->weftArea) / weave->weftArea;

	if (!init)
		result += kd * INV_PI;

	*f_ += result * CosTheta(wo);
}

/** parameters:
 *	u	 to be compared to u(v) in texturing
 *	v	 for filament, we compute u(v)
 *	om_i  incident direction
 *	om_r  exitant direction
 *	ss	filament smoothing parameter
 *  fiber properties
 *	alpha uniform scattering
 *	beta  forward scattering
 *  yarn geometry
 *	psi   fiber twist angle; because this is filament, psi = pi/2
 *	umax  maximum inclination angle
 *	kappa spine curvature parameter
 *  weave pattern
 *	w	 width of segment rectangle
 *	l	 length of segment rectangle
 */
float Irawan::evalFilamentIntegrand(float u, float v, const Vector &om_i,
		const Vector &om_r, float alpha, float beta, float ss,
		float umax, float kappa, float w, float l) const {
	// 0 <= ss < 1.0
	if (ss < 0.0f || ss >= 1.0f)
		return 0.0f;

	// w * sin(umax) < l
	if (w * std::sin(umax) >= l)
		return 0.0f;

	// -1 < kappa < inf
	if (kappa < -1.0f)
		return 0.0f;

	// h is the half vector
	Vector h = Normalize(om_r + om_i);

	// u_of_v is location of specular reflection.
	float u_of_v = std::atan(h.y / h.z);

	// Check if u_of_v within the range of valid u values
	if (std::abs(u_of_v) < umax) {
		// n is normal to the yarn surface
		// t is tangent of the fibers.
		Normal n = Normalize(Normal(std::sin(v), std::sin(u_of_v) * std::cos(v),
				std::cos(u_of_v) * std::cos(v)));
		Vector t = Normalize(Vector(0.0f, std::cos(u_of_v), -std::sin(u_of_v)));

		// R is radius of curvature.
		float R = radiusOfCurvature(std::min(std::abs(u_of_v),
			(1-ss)*umax), (1-ss)*umax, kappa, w, l);

		// G is geometry factor.
		float a = 0.5f * w;
		Vector om_i_plus_om_r = om_i + om_r,
			   t_cross_h = Cross(t, h);
		float Gu = a * (R + a * std::cos(v)) /
			(om_i_plus_om_r.Length() * std::abs(t_cross_h.x));

		// fc is phase function
		float fc = alpha + vonMises(-Dot(om_i, om_r), beta);

		// A is attenuation function without smoothing.
		// As is attenuation function with smoothing.
		float A = seeliger(Dot(n, om_i), Dot(n, om_r), 0, 1);
		float As;
		if (ss == 0.0f)
			As = A;
		else
			As = A * (1.0f - smoothStep(0, 1, (std::abs(u_of_v)
				- (1.0f - ss) * umax) / (ss * umax)));

		// fs is scattering function.
		float fs = Gu * fc * As;

		// Domain transform.
		fs = fs * M_PI * l;

		// Highlight has constant width delta_y on screen.
		float delta_y = l * weave->hWidth;

		// Clamp y_of_v between -(l - delta_y)/2 and (l - delta_y)/2.
		float y_of_v = u_of_v * 0.5f * l / umax;
		if (y_of_v > 0.5f * (l - delta_y))
			y_of_v = 0.5f * (l - delta_y);
		else if (y_of_v < 0.5f * (delta_y - l))
			y_of_v = 0.5f * (delta_y - l);

		// Check if |y(u(v)) - y(u)| < delta_y/2.
		if (std::abs(y_of_v - u * 0.5f * l / umax) < 0.5f * delta_y)
			return fs / delta_y;
	}
	return 0.0f;
}

/** parameters:
 *	u	 for staple, we compute v(u)
 *	v	 to be compared to v(u) in texturing
 *	om_i  incident direction
 *	om_r  exitant direction
 *  fiber properties
 *	alpha uniform scattering
 *	beta  forward scattering
 *  yarn geometry
 *	psi   fiber twist angle
 *	umax  maximum inclination angle
 *	kappa spine curvature parameter
 *  weave pattern
 *	w	 width of segment rectangle
 *	l	 length of segment rectangle
 */
float Irawan::evalStapleIntegrand(float u, float v, const Vector &om_i,
		const Vector &om_r, float alpha, float beta, float psi,
		float umax, float kappa, float w, float l) const {
	// w * sin(umax) < l
	if (w * std::sin(umax) >= l)
		return 0.0f;

	// -1 < kappa < inf
	if (kappa < -1.0f)
		return 0.0f;

	// h is the half vector
	Vector h = Normalize(om_i + om_r);

	// v_of_u is location of specular reflection.
	float D = (h.y*std::cos(u) - h.z*std::sin(u))
		/ (std::sqrt(h.x * h.x + std::pow(h.y * std::sin(u) + h.z * std::cos(u), (float) 2.0f)) * std::tan(psi));
	float v_of_u = std::atan2(-h.y * std::sin(u) - h.z * std::cos(u), h.x) + safe_acos(D);

	// Check if v_of_u within the range of valid v values
	if (std::abs(D) < 1.0f && std::abs(v_of_u) < M_PI / 2.0f) {
		// n is normal to the yarn surface.
		// t is tangent of the fibers.

		Vector n = Normalize(Vector(std::sin(v_of_u), std::sin(u)
				* std::cos(v_of_u), std::cos(u) * std::cos(v_of_u)));

		/*Vector t = normalize(Vector(-std::cos(v_of_u) * std::sin(psi),
				std::cos(u) * std::cos(psi) + std::sin(u) * std::sin(v_of_u) * std::sin(psi),
				-std::sin(u) * std::cos(psi) + std::cos(u) * std::sin(v_of_u) * std::sin(psi))); */

		// R is radius of curvature.
		float R = radiusOfCurvature(std::abs(u), umax, kappa, w, l);

		// G is geometry factor.
		float a = 0.5f * w;
		Vector om_i_plus_om_r(om_i + om_r);
		float Gv = a * (R + a * std::cos(v_of_u))
			/ (om_i_plus_om_r.Length() * Dot(n, h) * std::abs(std::sin(psi)));

		// fc is phase function.
		float fc = alpha + vonMises(-Dot(om_i, om_r), beta);

		// A is attenuation function without smoothing.
		float A = seeliger(Dot(n, om_i), Dot(n, om_r), 0, 1);

		// fs is scattering function.
		float fs = Gv * fc * A;

		// Domain transform.
		fs = fs * 2.0f * w * umax;

		// Highlight has constant width delta_x on screen.
		float delta_x = w * weave->hWidth;

		// Clamp x_of_u between (w - delta_x)/2 and -(w - delta_x)/2.
		float x_of_u = v_of_u * w / M_PI;
		if (x_of_u > 0.5f * (w - delta_x))
			x_of_u = 0.5f * (w - delta_x);
		else if (x_of_u < 0.5f * (delta_x - w))
			x_of_u = 0.5f * (delta_x - w);

		// Check if |x(v(u)) - x(v)| < delta_x/2.
		if (std::abs(x_of_u - v * w / M_PI) < 0.5f * delta_x)
			return fs / delta_x;
	}
	return 0.0f;
}

float Irawan::radiusOfCurvature(float u, float umax, float kappa, float w, float l) const {
	// rhat determines whether the spine is a segment
	// of an ellipse, a parabole, or a hyperbola.
	// See Section 5.3.
	float rhat = 1.0f + kappa * (1.0f + 1.0f / std::tan(umax));

	float a = 0.5f * w, R = 0;
	if (rhat == 1.0f) { // circle; see Subsection 5.3.1.
		R = (0.5f * l - a * std::sin(umax)) / std::sin(umax);
	} else if (rhat > 0.0f) {
		float tmax = std::atan(rhat * std::tan(umax));
		float bhat = (0.5f * l - a * std::sin(umax)) / std::sin(tmax);
		float ahat = bhat / rhat;
		float t = std::atan(rhat * std::tan(u));
		R = std::pow(bhat * bhat * std::cos(t) * std::cos(t)
		  + ahat * ahat * std::sin(t) * std::sin(t),(float) 1.5f) / (ahat * bhat);
	} else if (rhat < 0.0f) { // hyperbola; see Subsection 5.3.3.
		float tmax = -atanh(rhat * std::tan(umax));
		float bhat = (0.5f * l - a * std::sin(umax)) / std::sinh(tmax);
		float ahat = bhat / rhat;
		float t = -atanh(rhat * std::tan(u));
		R = -std::pow(bhat * bhat * std::cosh(t) * std::cosh(t)
			+ ahat * ahat * std::sinh(t) * std::sinh(t), (float) 1.5f) / (ahat * bhat);
	} else { // rhat == 0  // parabola; see Subsection 5.3.2.
		float tmax = std::tan(umax);
		float ahat = (0.5f * l - a * std::sin(umax)) / (2 * tmax);
		float t = std::tan(u);
		R = 2 * ahat * std::pow(1 + t * t, (float) 1.5f);
	}
	return R;
}
