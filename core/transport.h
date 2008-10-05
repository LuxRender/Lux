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

#ifndef LUX_TRANSPORT_H
#define LUX_TRANSPORT_H
// transport.h*
#include "lux.h"
#include "spectrum.h"

namespace lux
{

// Integrator Declarations
class  Integrator {
public:
	// Integrator Interface
	virtual ~Integrator();
	virtual SWCSpectrum Li(const TsPack *tspack, const Scene *scene,
					    const RayDifferential &ray,
					    const Sample *sample,
					    float *alpha) const = 0;
	virtual void Preprocess(const TsPack *tspack, const Scene *scene) {
	}
	virtual void RequestSamples(Sample *sample,
	                            const Scene *scene) {
	}
};

class SurfaceIntegrator : public Integrator {
public:
	// Dade - Exphotonmap is the only integrator not supporting SWC at the moment
	virtual bool IsSWCSupported() {
		return true;
	}
};

class VolumeIntegrator : public Integrator {
public:
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
	const Sample *sample, float *lightSample = NULL,
	float *lightNum = NULL, float *bsdfSample = NULL,
	float *bsdfComponent = NULL);
 SWCSpectrum UniformSampleAllLights(const TsPack *tspack, const Scene *scene,
	const Point &p, const Normal &n, const Vector &wo,
	BSDF *bsdf, const Sample *sample,
	int *lightSampleOffset, int *bsdfSampleOffset,
	int *bsdfComponentOffset);
 SWCSpectrum UniformSampleOneLight(const TsPack *tspack, const Scene *scene, const Point &p,
	const Normal &n, const Vector &wo, BSDF *bsdf,
	const Sample *sample, float *lightSample = NULL,
	float *lightNum = NULL, float *bsdfSample = NULL,
	float *bsdfComponent = NULL);
 SWCSpectrum WeightedSampleOneLight(const TsPack *tspack, const Scene *scene, const Point &p,
	const Normal &n, const Vector &wo, BSDF *bsdf,
	const Sample *sample, int lightSampleOffset, int lightNumOffset,
	int bsdfSampleOffset, int bsdfComponentOffset, float *&avgY,
	float *&avgYsample, float *&cdf, float &overallAvgY);
 
}//namespace lux
 
#endif // LUX_TRANSPORT_H
