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
		const SWCSpectrum &Alpha, float d, float r, float p);
	virtual ~SchlickBRDF() { }
	virtual void f(const TsPack *tspack, const Vector &wo, const Vector &wi, SWCSpectrum *const f) const;
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
// Alternative with interreflection in the coating creases
//		return (0.f - G * (0.f - SchlickZ(fabsf(H.z)) *
//			SchlickA(H))) / (4.f * M_PI * cos1 * cos2);
		return G * SchlickZ(fabsf(H.z)) * SchlickA(H) /
			(4.f * M_PI * cos1 * cos2);
	}
	virtual bool Sample_f(const TsPack *tspack, const Vector &wo, Vector *wi,
		float u1, float u2, SWCSpectrum *const f, float *pdf, float *pdfBack = NULL,
		bool reverse = false) const;
	virtual float Pdf(const TsPack *tspack, const Vector &wi, const Vector &wo) const;
	
private:
	// SchlickBRDF Private Data
	SWCSpectrum Rd, Rs, Alpha;
	float depth, roughness, anisotropy;
};

}//namespace lux

#endif // LUX_SCHLICKBRDF_H

