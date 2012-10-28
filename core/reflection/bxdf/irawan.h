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

#ifndef LUX_IRAWAN_H
#define LUX_IRAWAN_H
// irawan.h*
#include "lux.h"
#include "bxdf.h"
#include "spectrum.h"

namespace lux
{

class WeavePattern;

// Data structure describing the properties of a single yarn
class Yarn {
public:
	// Fiber twist angle
	float psi;
	// Maximum inclination angle
	float umax;
	// Spine curvature
	float kappa;
	// Width of segment rectangle
	float width;
	// Length of segment rectangle
	float length;
	/*! u coordinate of the yarn segment center,
	 * assumes that the tile covers 0 <= u, v <= 1.
	 * (0, 0) is lower left corner of the weave pattern
	 */
	float centerU;
	// v coordinate of the yarn segment center
	float centerV;

	// Texture index
	u_int index;

	Yarn(float y_psi, float y_umax, float y_kappa,
		float y_width, float y_length, float y_centerU,
		float y_centerV, u_int y_index) : psi(Radians(y_psi)),
		umax(Radians(y_umax)), kappa(y_kappa), width(y_width),
		length(y_length), centerU(y_centerU), centerV(y_centerV),
		index(y_index) {  }
	virtual void GetUV(const WeavePattern &weave, const Point &center,
		const Point &xy, Point *uv, float *umaxMod) const = 0;
	virtual float EvalIntegrand(const WeavePattern &weave, const Point &uv,
		float umaxMod, Vector &om_i, Vector &om_r) const = 0;

protected:
	float EvalFilamentIntegrand(const WeavePattern &weave, const Vector &om_i,
		const Vector &om_r, float u, float v, float umaxMod) const;
	float EvalStapleIntegrand(const WeavePattern &weave, const Vector &om_i,
		const Vector &om_r, float u, float v, float umaxMod) const;
	float RadiusOfCurvature(float u, float umaxMod) const;
};

class Warp : public Yarn
{
public:
	Warp(float y_psi, float y_umax, float y_kappa, float y_width,
		float y_length, float y_centerU, float y_centerV,
		u_int y_index) : Yarn(y_psi, y_umax, y_kappa, y_width, y_length,
		y_centerU, y_centerV, y_index) { }
	virtual void GetUV(const WeavePattern &weave, const Point &center,
		const Point &xy, Point *uv, float *umaxMod) const;
	virtual float EvalIntegrand(const WeavePattern &weave, const Point &uv,
		float umaxMod, Vector &om_i, Vector &om_r) const;
};

class Weft : public Yarn
{
public:
	Weft(float y_psi, float y_umax, float y_kappa, float y_width,
		float y_length, float y_centerU, float y_centerV,
		u_int y_index) : Yarn(y_psi, y_umax, y_kappa, y_width, y_length,
		y_centerU, y_centerV, y_index) { }
	virtual void GetUV(const WeavePattern &weave, const Point &center,
		const Point &xy, Point *uv, float *umaxMod) const;
	virtual float EvalIntegrand(const WeavePattern &weave, const Point &uv,
		float umaxMod, Vector &om_i, Vector &om_r) const;
};

class WeavePattern {
public:
	// Name of the weave pattern
	std::string name;
	// Uniform scattering parameter
	float alpha;
	// Forward scattering parameter
	float beta;
	// Filament smoothing
	float ss;
	// Highlight width
	float hWidth;
	// Combined area taken up by the warp & weft
	float warpArea, weftArea;

	// Size of the weave pattern
	u_int tileWidth, tileHeight;

	// Noise-related parameters
	float dWarpUmaxOverDWarp;
	float dWarpUmaxOverDWeft;
	float dWeftUmaxOverDWarp;
	float dWeftUmaxOverDWeft;
	float fineness, period;
	float repeat_u, repeat_v;

	// Detailed weave pattern
	std::vector<u_int> pattern;

	// List of all yarns referenced in pattern
	std::vector<Yarn *> yarns;

	WeavePattern(std::string w_name, u_int w_tileWidth, u_int w_tileHeight,
		float w_alpha, float w_beta, float w_ss, float w_hWidth,
		float w_warpArea, float w_weftArea, float w_fineness,
		float w_repeat_u, float w_repeat_v,
		float w_dWarpUmaxOverDWarp, float w_dWarpUmaxOverDWeft,
		float w_dWeftUmaxOverDWarp, float w_dWeftUmaxOverDWeft,
		float w_period) : name(w_name), alpha(w_alpha), beta(w_beta),
		ss(w_ss), hWidth(w_hWidth), warpArea(w_warpArea),
		weftArea(w_weftArea), tileWidth(w_tileWidth),
		tileHeight(w_tileHeight),
		dWarpUmaxOverDWarp(Radians(w_dWarpUmaxOverDWarp)),
		dWarpUmaxOverDWeft(Radians(w_dWarpUmaxOverDWeft)),
		dWeftUmaxOverDWarp(Radians(w_dWeftUmaxOverDWarp)),
		dWeftUmaxOverDWeft(Radians(w_dWeftUmaxOverDWeft)),
		fineness(w_fineness), period(w_period),
		repeat_u(w_repeat_u), repeat_v(w_repeat_v) { }

	~WeavePattern() {
		for (u_int i = 0; i < yarns.size(); ++i)
			delete yarns.at(i);
	 }
	const Yarn *GetYarn(float u_i, float v_i, Point *uv, float *umax,
		float *scale) const;
};

class  Irawan : public BxDF {
public:
	// Irawan Public Methods
	Irawan(const SWCSpectrum &ks,
		const Point &pos, float um, const Yarn *y,
		const WeavePattern *pattern, float spec_norm) :
		BxDF(BxDFType(BSDF_REFLECTION | BSDF_GLOSSY)),
		Ks(ks), uv(pos), umax(um), yarn(y), weave(pattern),
		specularNormalization(spec_norm) { }
	virtual ~Irawan() { }
	virtual void F(const SpectrumWavelengths &sw, const Vector &wo,
		const Vector &wi, SWCSpectrum *const f) const;
	virtual bool SampleF(const SpectrumWavelengths &sw, const Vector &wo,
		Vector *wi, float u1, float u2, SWCSpectrum *const f,
		float *pdf, float *pdfBack, bool reverse) const;
	float evalSpecular(const Vector &wo, const Vector &wi) const;

private:
	// Irawan Private Data
	SWCSpectrum Ks;
	Point uv;
	float umax;
	const Yarn *yarn;
	const WeavePattern *weave;
	float specularNormalization;
};

}//namespace lux

#endif // LUX_IRAWAN_H
