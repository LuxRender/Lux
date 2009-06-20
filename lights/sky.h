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

// sky.h*
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
	SWCSpectrum Power(const TsPack *tspack, const Scene *scene) const {
		Point worldCenter;
		float worldRadius;
		scene->WorldBound().BoundingSphere(&worldCenter,
		                                    &worldRadius);
		SWCSpectrum zenith;
		GetSkySpectralRadiance(tspack, 0.f, 0.f, &zenith);
		return zenith * (havePortalShape ? PortalArea : 4.f * M_PI * worldRadius * worldRadius) * 2.f * M_PI;
		//return skyScale * M_PI * worldRadius * worldRadius;
	}
	bool IsDeltaLight() const { return false; }
	bool IsEnvironmental() const { return true; }
	SWCSpectrum Le(const TsPack *tspack, const RayDifferential &r) const;
	SWCSpectrum Le(const TsPack *tspack, const Scene *scene, const Ray &r,
		const Normal &n, BSDF **bsdf, float *pdf, float *pdfDirect) const;
	SWCSpectrum Sample_L(const TsPack *tspack, const Point &p, const Normal &n,
		float u1, float u2, float u3, Vector *wi, float *pdf,
		VisibilityTester *visibility) const;
	SWCSpectrum Sample_L(const TsPack *tspack, const Point &p, float u1, float u2, float u3,
		Vector *wi, float *pdf, VisibilityTester *visibility) const;
	SWCSpectrum Sample_L(const TsPack *tspack, const Scene *scene, float u1, float u2,
		float u3, float u4, Ray *ray, float *pdf) const;
	float Pdf(const Point &, const Normal &, const Vector &) const;
	float Pdf(const Point &, const Vector &) const;
	float Pdf(const Point &p, const Normal &n,
		const Point &po, const Normal &ns) const;
	bool Sample_L(const TsPack *tspack, const Scene *scene, float u1, float u2, float u3, BSDF **bsdf, float *pdf, SWCSpectrum *Le) const;
	bool Sample_L(const TsPack *tspack, const Scene *scene, const Point &p, const Normal &n, float u1, float u2, float u3, BSDF **bsdf, float *pdf, float *pdfDirect, VisibilityTester *visibility, SWCSpectrum *Le) const;

	static Light *CreateLight(const Transform &light2world,
		const ParamSet &paramSet, const TextureParams &tp);

		// internal methods
	Vector		GetSunPosition() const;
	void		SunThetaPhi(float &theta, float &phi) const;
	RGBColor	GetSunSpectralRadiance() const;
	float		GetSunSolidAngle() const;
	void		GetSkySpectralRadiance(const TsPack *tspack, const float theta, const float phi, SWCSpectrum * const dst_spect) const;
	void		GetAtmosphericEffects(const Vector &viewer, const Vector &source,
									RGBColor &atmAttenuation, RGBColor &atmInscatter ) const;

	void		InitSunThetaPhi();
	void		ChromaticityToSpectrum(const TsPack *tspack, const float x, const float y, SWCSpectrum * const dst_spect) const;

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

