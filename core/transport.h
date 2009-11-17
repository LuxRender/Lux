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

#ifndef LUX_TRANSPORT_H
#define LUX_TRANSPORT_H
// transport.h*
#include "lux.h"
#include "spectrum.h"
#include "renderinghints.h"

namespace lux
{

// Integrator Declarations
class  Integrator {
public:
	// Integrator Interface
	virtual ~Integrator();
	virtual void Preprocess(const TsPack *tspack, const Scene *scene) {
	}
	virtual void RequestSamples(Sample *sample,
	                            const Scene *scene) {
	}
};

class SurfaceIntegrator : public Integrator {
public:
	virtual u_int Li(const TsPack *tspack, const Scene *scene,
		const Sample *sample) const = 0;

protected:
	SurfaceIntegratorRenderingHints hints;
};

class VolumeIntegrator : public Integrator {
public:
	virtual u_int Li(const TsPack *tspack, const Scene *scene,
		const RayDifferential &ray, const Sample *sample,
		SWCSpectrum *L, float *alpha) const = 0;
	// modulates the supplied SWCSpectrum with the transmittance along the ray
	virtual void Transmittance(const TsPack *tspack, const Scene *scene,
		const Ray &ray, const Sample *sample, float *alpha, SWCSpectrum *const L) const = 0;
};

 SWCSpectrum EstimateDirect(const TsPack *tspack, const Scene *scene, const Light *light,
	const Point &p, const Normal &n, const Vector &wo, BSDF *bsdf, 
	const Sample *sample, 
	float ls1, float ls2, float ls3, float bs1, float bs2, float bcs);
 SWCSpectrum UniformSampleAllLights(const TsPack *tspack, const Scene *scene, const Point &p,
	const Normal &n, const Vector &wo, BSDF *bsdf,
	const Sample *sample, const float *lightSample = NULL,
	const float *lightNum = NULL, const float *bsdfSample = NULL,
	const float *bsdfComponent = NULL);
 u_int UniformSampleOneLight(const TsPack *tspack, const Scene *scene, const Point &p,
	const Normal &n, const Vector &wo, BSDF *bsdf,
	const Sample *sample, const float *lightSample,
	const float *lightNum, const float *bsdfSample,
	const float *bsdfComponent, SWCSpectrum *L);

// Note - Radiance - disabled as this code is broken. (not threadsafe)
/*
 SWCSpectrum WeightedSampleOneLight(const TsPack *tspack, const Scene *scene, const Point &p,
	const Normal &n, const Vector &wo, BSDF *bsdf,
	const Sample *sample, int lightSampleOffset, int lightNumOffset,
	int bsdfSampleOffset, int bsdfComponentOffset, float *&avgY,
	float *&avgYsample, float *&cdf, float &overallAvgY);
*/

}//namespace lux
 
#endif // LUX_TRANSPORT_H
