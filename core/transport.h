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
	virtual SWCSpectrum Li(const Scene *scene,
					    const RayDifferential &ray,
					    const Sample *sample,
					    float *alpha) const = 0;
	virtual void Preprocess(const Scene *scene) {
	}
	virtual void RequestSamples(Sample *sample,
	                            const Scene *scene) {
	}
	virtual Integrator* clone() const = 0;   // Lux Virtual (Copy) Constructor for multithreading
};
class SurfaceIntegrator : public Integrator {
public:
	virtual SurfaceIntegrator* clone() const = 0;
};
class VolumeIntegrator : public Integrator {
public:
	virtual SWCSpectrum Transmittance(const Scene *scene,
		const Ray &ray, const Sample *sample,
		float *alpha) const = 0;
	virtual VolumeIntegrator* clone() const = 0;
};
 SWCSpectrum UniformSampleAllLights(const Scene *scene,
	const Point &p, const Normal &n, const Vector &wo,
	BSDF *bsdf, const Sample *sample,
	int *lightSampleOffset, int *bsdfSampleOffset,
	int *bsdfComponentOffset);
 SWCSpectrum UniformSampleOneLight(const Scene *scene, const Point &p,
	const Normal &n, const Vector &wo, BSDF *bsdf,
	const Sample *sample, int lightSampleOffset,
	int lightNumOffset, int bsdfSampleOffset,
	int bsdfComponentOffset);
 SWCSpectrum UniformSampleOneLight(const Scene *scene, const Point &p,
	const Normal &n, const Vector &wo, BSDF *bsdf,
	const Sample *sample, float *lightSampleOffset = NULL,
	float *lightNumOffset = NULL, float *bsdfSampleOffset = NULL,
	float *bsdfComponentOffset = NULL);
 SWCSpectrum WeightedSampleOneLight(const Scene *scene, const Point &p,
	const Normal &n, const Vector &wo, BSDF *bsdf,
	const Sample *sample, int lightSampleOffset, int lightNumOffset,
	int bsdfSampleOffset, int bsdfComponentOffset, float *&avgY,
	float *&avgYsample, float *&cdf, float &overallAvgY);
 
}//namespace lux
 
#endif // LUX_TRANSPORT_H
