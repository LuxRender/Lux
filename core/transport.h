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

#include "luxrays/luxrays.h"

namespace lux
{

// Integrator Declarations
class  Integrator {
public:
	// Integrator Interface
	virtual ~Integrator() { }
	virtual void Preprocess(const RandomGenerator &rng, const Scene &scene) { }
	virtual void RequestSamples(Sample *sample, const Scene &scene) { }
};

class SurfaceIntegratorState {
public:
	virtual ~SurfaceIntegratorState() { }

	virtual bool Init(const Scene &scene) = 0;
};

class SurfaceIntegrator : public Integrator {
public:
	virtual ~SurfaceIntegrator() { }
	virtual u_int Li(const Scene &scene, const Sample &sample) const = 0;

	// DataParallel interface, optionally supported, used by HybridRenderer
	virtual bool IsDataParallelSupported() const { return false; }
	//FIXME: just to check SurfaceIntegratorRenderingHints light strategy, to remove
	virtual bool CheckLightStrategy() const { return false; }
	virtual SurfaceIntegratorState *NewState(const Scene &,
		ContributionBuffer *contribBuffer, RandomGenerator *rng) {
		throw std::runtime_error("Internal error: called SurfaceIntegrator::NewSurfaceIntegratorState()");
	}
	virtual bool GenerateRays(const Scene &,
		SurfaceIntegratorState *state, luxrays::RayBuffer *rayBuffer) {
		throw std::runtime_error("Internal error: called SurfaceIntegrator::GenerateRays()");
	}
	virtual bool NextState(const Scene &, SurfaceIntegratorState *state, luxrays::RayBuffer *rayBuffer, u_int *nrContribs) {
		throw std::runtime_error("Internal error: called SurfaceIntegrator::NextState()");
	}
};

class VolumeIntegrator : public Integrator {
public:
	virtual ~VolumeIntegrator() { }
	virtual u_int Li(const Scene &scene, const Ray &ray,
		const Sample &sample, SWCSpectrum *L, float *alpha) const = 0;
	// modulates the supplied SWCSpectrum with the transmittance along the ray
	virtual void Transmittance(const Scene &scene, const Ray &ray,
		const Sample &sample, float *alpha, SWCSpectrum *const L) const = 0;
	virtual bool Intersect(const Scene &scene, const Sample &sample,
		const Volume *volume, const Ray &ray,
		Intersection *isect, BSDF **bsdf, SWCSpectrum *L) const;
	// Used to complete intersection data with LuxRays
	virtual bool Intersect(const Scene &scene, const Sample &sample,
		const Volume *volume, const Ray &ray, const luxrays::RayHit &rayHit,
		Intersection *isect, BSDF **bsdf, SWCSpectrum *L) const;
	virtual bool Connect(const Scene &scene, const Sample &sample,
		const Volume *volume, const Point &p0, const Point &p1,
		bool clip, SWCSpectrum *f, float *pdf, float *pdfR) const;
};

SWCSpectrum EstimateDirect(const Scene &scene, const Light &light,
	const Sample &sample, const Point &p, const Normal &n, const Vector &wo,
	BSDF *bsdf, float ls1, float ls2, float ls3,
	float bs1, float bs2, float bcs);
SWCSpectrum UniformSampleAllLights(const Scene &scene, const Sample &sample,
	const Point &p, const Normal &n, const Vector &wo, BSDF *bsdf,
	const float *lightSample, const float *lightNum,
	const float *bsdfSample, const float *bsdfComponent);
u_int UniformSampleOneLight(const Scene &scene, const Sample &sample,
	const Point &p,	const Normal &n, const Vector &wo, BSDF *bsdf,
	const float *lightSample, const float *lightNum,
	const float *bsdfSample, const float *bsdfComponent, SWCSpectrum *L);

}//namespace lux
 
#endif // LUX_TRANSPORT_H
