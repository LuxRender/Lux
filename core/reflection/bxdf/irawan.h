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

// Data structure describing the properties of a single yarn
class Yarn {
public:
	enum EYarnType {
		EWarp = 0,
		EWeft = 1
	};

	// Type of yarn (warp or weft)
	EYarnType type;
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

	Yarn() : type(EWarp), psi(0), umax(0), kappa(0), width(0), length(0),
		centerU(0), centerV(0) { }

	Yarn(EYarnType y_type, float y_psi, float y_umax, float y_kappa,
		float y_width, float y_length, float y_centerU,
		float y_centerV) : type(y_type), psi(y_psi * M_PI / 180.0f),
		umax(y_umax * M_PI / 180.0f), kappa(y_kappa), width(y_width),
		length(y_length), centerU(y_centerU), centerV(y_centerV) {  }
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

	WeavePattern() : name(""),
		alpha(0), beta(0), ss(0), hWidth(0), warpArea(0), weftArea(0),
		tileWidth(0), tileHeight(0),
		dWarpUmaxOverDWarp(0), dWarpUmaxOverDWeft(0),
		dWeftUmaxOverDWarp(0), dWeftUmaxOverDWeft(0),
		fineness(0), period(0), repeat_u(0), repeat_v(0) { }

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
		dWarpUmaxOverDWarp(w_dWarpUmaxOverDWarp * M_PI / 180.0f),
		dWarpUmaxOverDWeft(w_dWarpUmaxOverDWeft * M_PI / 180.0f),
		dWeftUmaxOverDWarp(w_dWeftUmaxOverDWarp * M_PI / 180.0f),
		dWeftUmaxOverDWeft(w_dWeftUmaxOverDWeft * M_PI / 180.0f),
		fineness(w_fineness), period(w_period),
		repeat_u(w_repeat_u), repeat_v(w_repeat_v) { }

	~WeavePattern() {
		for (u_int i = 0; i < yarns.size(); ++i)
			delete yarns.at(i);
	 }
};

class  Irawan : public BxDF {
public:
	// Irawan Public Methods
	Irawan(const SWCSpectrum &warp_kd, const SWCSpectrum &warp_ks,
		const SWCSpectrum &weft_kd, const SWCSpectrum &weft_ks,
		const float u, const float v, float spec_norm,
		boost::shared_ptr<WeavePattern> pattern) :
		BxDF(BxDFType(BSDF_REFLECTION | BSDF_GLOSSY)),
		warp_Kd(warp_kd), warp_Ks(warp_ks),
		weft_Kd(weft_kd), weft_Ks(weft_ks), weave(pattern), U(u), V(v),
		specularNormalization(spec_norm) { }
	virtual ~Irawan() { }
	virtual void F(const SpectrumWavelengths &sw, const Vector &wo,
		const Vector &wi, SWCSpectrum *const f) const;
	virtual bool SampleF(const SpectrumWavelengths &sw, const Vector &wo,
		Vector *wi, float u1, float u2, SWCSpectrum *const f,
		float *pdf, float *pdfBack, bool reverse) const;
	float evalSpecular(const Vector &wo, const Vector &wi,
		const float u_i, const float v_i, int *type) const;

private:
	float evalFilamentIntegrand(float u, float v, const Vector &om_i,
		const Vector &om_r, float umax, float kappa,
		float w, float l) const;

	float evalStapleIntegrand(float u, float v, const Vector &om_i,
		const Vector &om_r, float psi, float umax, float kappa,
		float w, float l) const;

	float radiusOfCurvature(float u, float umax, float kappa,
		float w, float l) const;

	// Irawan Private Data
	SWCSpectrum warp_Kd, warp_Ks, weft_Kd, weft_Ks;
	boost::shared_ptr<WeavePattern> weave;
	float U, V;
	float specularNormalization;
};

}//namespace lux

#endif // LUX_IRAWAN_H
