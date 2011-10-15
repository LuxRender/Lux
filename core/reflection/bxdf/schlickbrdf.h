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

#ifndef LUX_SCHLICKBRDF_H
#define LUX_SCHLICKBRDF_H
// schlickbrdf.h*
#include "lux.h"
#include "bxdf.h"
#include "spectrum.h"

namespace lux
{

class  SchlickBRDF : public BxDF
{
public:
	// SchlickBRDF Public Methods
	SchlickBRDF(const SWCSpectrum &Rd, const SWCSpectrum &Rs,
		const SWCSpectrum &Alpha, float d, float r, float p, bool mb);
	virtual ~SchlickBRDF() { }
	virtual void F(const SpectrumWavelengths &sw, const Vector &wo,
		const Vector &wi, SWCSpectrum *const f) const;
	SWCSpectrum SchlickFresnel(float costheta) const {
		return Rs + powf(1.f - costheta, 5.f) * (SWCSpectrum(1.f) - Rs);
	}
	float SchlickG(float costheta) const {
		return costheta / (costheta * (1.f - roughness) + roughness);
	}
	float SchlickZ(float cosNH) const {
		const float d = 1.f + (roughness - 1) * cosNH * cosNH;
		return roughness > 0.f ? roughness / (d * d) : INFINITY;
	}
	float SchlickA(const Vector &H) const {
		const float h = sqrtf(H.x * H.x + H.y * H.y);
		if (h > 0.f) {
			const float w = (anisotropy > 0.f ? H.x : H.y) / h;
			const float p = 1.f - fabsf(anisotropy);
			return sqrtf(p / (p * p + w * w * (1.f - p * p)));
		}
		return 1.f;
	}
	float SchlickD(float cos1, float cos2, const Vector &H) const {
		const float G = SchlickG(cos1) * SchlickG(cos2);
		const float den = 4.f * M_PI * cos1 * cos2;
		// Alternative with interreflection in the coating creases
		if (multibounce)
			return G * SchlickZ(fabsf(H.z)) * SchlickA(H) / den +
				Clamp((1.f - G) / den, 0.f, 1.f);
		else
			return G * SchlickZ(fabsf(H.z)) * SchlickA(H) / den;
	}
	virtual bool SampleF(const SpectrumWavelengths &sw, const Vector &wo,
		Vector *wi, float u1, float u2, SWCSpectrum *const f,
		float *pdf, float *pdfBack = NULL, bool reverse = false) const;
	virtual float Pdf(const SpectrumWavelengths &sw, const Vector &wi,
		const Vector &wo) const;

protected:
	// SchlickBRDF Private Data
	SWCSpectrum Rd, Rs, Alpha;
	float depth, roughness, anisotropy;
	bool multibounce;
};

class  SchlickGlossyBRDF : public BxDF
{
public:
	// SchlickGlossyBRDF Public Methods
	SchlickGlossyBRDF(const Fresnel *fr, float r, float p, bool mb);
	virtual ~SchlickGlossyBRDF() { }
	virtual void F(const SpectrumWavelengths &sw, const Vector &wo,
		const Vector &wi, SWCSpectrum *const f) const;
	float SchlickG(float costheta) const {
		return costheta / (costheta * (1.f - roughness) + roughness);
	}
	float SchlickZ(float cosNH) const {
		const float d = 1.f + (roughness - 1) * cosNH * cosNH;
		return roughness > 0.f ? roughness / (d * d) : INFINITY;
	}
	float SchlickA(const Vector &H) const {
		const float h = sqrtf(H.x * H.x + H.y * H.y);
		if (h > 0.f) {
			const float w = (anisotropy > 0.f ? H.x : H.y) / h;
			const float p = 1.f - fabsf(anisotropy);
			return sqrtf(p / (p * p + w * w * (1.f - p * p)));
		}
		return 1.f;
	}
	float SchlickD(float cos1, float cos2, const Vector &H) const {
		const float G = SchlickG(cos1) * SchlickG(cos2);
		const float den = 4.f * M_PI * cos1 * cos2;
		// Alternative with interreflection in the coating creases
		if (multibounce)
			return G * SchlickZ(fabsf(H.z)) * SchlickA(H) / den +
				Clamp((1.f - G) / den, 0.f, 1.f);
		else
			return G * SchlickZ(fabsf(H.z)) * SchlickA(H) / den;
	}
	virtual bool SampleF(const SpectrumWavelengths &sw, const Vector &wo,
		Vector *wi, float u1, float u2, SWCSpectrum *const f,
		float *pdf, float *pdfBack = NULL, bool reverse = false) const;
	virtual float Pdf(const SpectrumWavelengths &sw, const Vector &wi,
		const Vector &wo) const;
	virtual float Weight(const SpectrumWavelengths &sw,
		const Vector &wo) const;

protected:
	// SchlickGlossyBRDF Private Data
	const Fresnel *fresnel;
	float roughness, anisotropy;
	bool multibounce;
};

class  SchlickDoubleSidedBRDF : public SchlickBRDF
{
public:
	// SchlickDoubleSidedBRDF Public Methods
	SchlickDoubleSidedBRDF(const SWCSpectrum &Rd,
		const SWCSpectrum &Rs, const SWCSpectrum &Rs2,
		const SWCSpectrum &Alpha, const SWCSpectrum &Alpha2,
		float d, float d2, float r, float r2, float p, float p2,
		bool mb, bool mb2);
	virtual ~SchlickDoubleSidedBRDF() { }
	virtual void F(const SpectrumWavelengths &sw, const Vector &wo,
		const Vector &wi, SWCSpectrum *const f) const;
	SWCSpectrum SchlickFresnelBack(float costheta) const {
		return Rs_bf + powf(1.f - costheta, 5.f) * (SWCSpectrum(1.f) - Rs_bf);
	}
	float SchlickGBack(float costheta) const {
		return costheta / (costheta * (1.f - roughness_bf) + roughness_bf);
	}
	float SchlickZBack(float cosNH) const {
		const float d = 1.f + (roughness_bf - 1) * cosNH * cosNH;
		return roughness_bf > 0.f ? roughness_bf / (d * d) : INFINITY;
	}
	float SchlickABack(const Vector &H) const {
		const float h = sqrtf(H.x * H.x + H.y * H.y);
		if (h > 0.f) {
			const float w = (anisotropy_bf > 0.f ? H.x : H.y) / h;
			const float p = 1.f - fabsf(anisotropy_bf);
			return sqrtf(p / (p * p + w * w * (1.f - p * p)));
		}
		return 1.f;
	}
	float SchlickDBack(float cos1, float cos2, const Vector &H) const {
		const float G = SchlickGBack(cos1) * SchlickGBack(cos2);
		const float den = 4.f * M_PI * cos1 * cos2;
		// Alternative with interreflection in the coating creases
		if (multibounce_bf)
			return G * SchlickZBack(fabsf(H.z)) * SchlickABack(H) / den +
				Clamp((1.f - G) / den, 0.f, 1.f);
		else
			return G * SchlickZBack(fabsf(H.z)) * SchlickABack(H) / den;
	}
	virtual bool SampleF(const SpectrumWavelengths &sw, const Vector &wo,
		Vector *wi, float u1, float u2, SWCSpectrum *const f,
		float *pdf, float *pdfBack = NULL, bool reverse = false) const;
	virtual float Pdf(const SpectrumWavelengths &sw, const Vector &wi,
		const Vector &wo) const;

private:
	// SchlickDoubleSidedBRDF Private Data
	SWCSpectrum Rs_bf, Alpha_bf;
	float depth_bf, roughness_bf, anisotropy_bf;
	bool multibounce_bf;
};

}//namespace lux

#endif // LUX_SCHLICKBRDF_H

