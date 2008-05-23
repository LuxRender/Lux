/***************************************************************************
 *   Copyright (C) 1998-2008 by authors (see AUTHORS.txt )                 *
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

// sky.cpp*
#include "lux.h"
#include "light.h"
#include "texture.h"
#include "shape.h"
#include "scene.h"
#include "spd.h"

namespace lux
{

// SkyLight Declarations
class SkyLight : public Light {
public:
	// SkyLight Public Methods
	SkyLight(const Transform &light2world,	const float skyscale, int ns, Vector sd, float turb, float aconst, float bconst, float cconst, float dconst, float econst);
	~SkyLight();
	SWCSpectrum Power(const Scene *scene) const {
		Point worldCenter;
		float worldRadius;
		scene->WorldBound().BoundingSphere(&worldCenter,
		                                    &worldRadius);
		//return Lbase * GetSkySpectralRadiance(.0, .0) * M_PI * worldRadius * worldRadius;
		return skyScale * M_PI * worldRadius * worldRadius;
	}
	bool IsDeltaLight() const { return false; }
	SWCSpectrum Le(const RayDifferential &r) const;
	SWCSpectrum Le(const Scene *scene, const Ray &r,
		const Normal &n, BSDF **bsdf, float *pdf, float *pdfDirect) const;
	SWCSpectrum Sample_L(const Point &p, const Normal &n,
		float u1, float u2, float u3, Vector *wi, float *pdf,
		VisibilityTester *visibility) const;
	SWCSpectrum Sample_L(const Point &p, float u1, float u2, float u3,
		Vector *wi, float *pdf, VisibilityTester *visibility) const;
	SWCSpectrum Sample_L(const Scene *scene, float u1, float u2,
		float u3, float u4, Ray *ray, float *pdf) const;
	float Pdf(const Point &, const Normal &, const Vector &) const;
	float Pdf(const Point &, const Vector &) const;
	SWCSpectrum Sample_L(const Point &P, Vector *w, VisibilityTester *visibility) const;
	SWCSpectrum Sample_L(const Scene *scene, float u1, float u2, BSDF **bsdf, float *pdf) const;
	SWCSpectrum Sample_L(const Scene *scene, const Point &p, const Normal &n, float u1, float u2, float u3, BSDF **bsdf, float *pdf, float *pdfDirect, VisibilityTester *visibility) const;
	float Pdf(const Scene *scene, const Point &p) const;

	static Light *CreateLight(const Transform &light2world,
		const ParamSet &paramSet);

		// internal methods
	Vector		GetSunPosition() const;
	void		SunThetaPhi(float &theta, float &phi) const;
	Spectrum	GetSunSpectralRadiance() const;
	float		GetSunSolidAngle() const;
	void		GetSkySpectralRadiance(const float theta, const float phi, SWCSpectrum * const dst_spect) const;
	void		GetAtmosphericEffects(const Vector &viewer, const Vector &source,
									Spectrum &atmAttenuation, Spectrum &atmInscatter ) const;

	void		InitSunThetaPhi();
	void		ChromaticityToSpectrum(const float x, const float y, SWCSpectrum * const dst_spect) const;
	float		PerezFunction(const float *lam, float theta, float phi, float lvz) const;

private:
	// SkyLight Private Data
	float skyScale;
	Vector  sundir;
	float 	turbidity;
	float	thetaS, phiS;
	float zenith_Y, zenith_x, zenith_y;
	float perez_Y[6], perez_x[6], perez_y[6];
};

}//namespace lux

