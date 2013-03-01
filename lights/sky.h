/***************************************************************************
 *   Copyright (C) 1998-2013 by authors (see AUTHORS.txt)                  *
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

// sky.h*
#include "lux.h"
#include "light.h"

namespace lux
{

// SkyLight Declarations
class SkyLight : public Light {
public:
	// SkyLight Public Methods
	SkyLight(const Transform &light2world, float skyscale, u_int ns,
		Vector sd, float turb, float aconst, float bconst,
		float cconst, float dconst, float econst);
	virtual ~SkyLight();
	virtual float Power(const Scene &scene) const;
	virtual bool IsDeltaLight() const { return false; }
	virtual bool IsEnvironmental() const { return true; }
	virtual bool Le(const Scene &scene, const Sample &sample, const Ray &r,
		BSDF **bsdf, float *pdf, float *pdfDirect,
		SWCSpectrum *L) const;
	virtual float Pdf(const Point &p, const PartialDifferentialGeometry &dg) const;
	virtual bool SampleL(const Scene &scene, const Sample &sample,
		float u1, float u2, float u3, BSDF **bsdf, float *pdf,
		SWCSpectrum *Le) const;
	virtual bool SampleL(const Scene &scene, const Sample &sample,
		const Point &p, float u1, float u2, float u3, BSDF **bsdf,
		float *pdf, float *pdfDirect, SWCSpectrum *Le) const;
	void GetSkySpectralRadiance(const SpectrumWavelengths &sw,
		const Vector &w, SWCSpectrum *const dst_spect) const;

	static Light *CreateLight(const Transform &light2world,
		const ParamSet &paramSet);

private:
	// Used by Queryable interface
	float GetDirectionX() { return sundir.x; }
	float GetDirectionY() { return sundir.y; }
	float GetDirectionZ() { return sundir.z; }

	// internal methods
	Vector GetSunPosition() const;
	void SunThetaPhi(float &theta, float &phi) const;

	void InitSunThetaPhi();
	void ChromaticityToSpectrum(const SpectrumWavelengths &sw,
		const float x, const float y,
		SWCSpectrum *const dst_spect) const;

	// SkyLight Private Data
	float skyScale;
	Vector  sundir;
	float 	turbidity;
	float	thetaS, phiS;
	float zenith_Y, zenith_x, zenith_y;
	float perez_Y[6], perez_x[6], perez_y[6];
};

}//namespace lux

