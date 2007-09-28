/***************************************************************************
 *   Copyright (C) 1998-2007 by authors (see AUTHORS.txt )                 *
 *                                                                         *
 *   This file is part of Lux Renderer.                                    *
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
 *   Lux Renderer website : http://www.luxrender.org                       *
 ***************************************************************************/

// irradiancecache.cpp*
#include "lux.h"
#include "transport.h"
#include "scene.h"
#include "mc.h"
#include "octree.h"
// IrradianceCache Forward Declarations
struct IrradianceSample;
struct IrradProcess;
// IrradianceCache Declarations
class IrradianceCache : public SurfaceIntegrator {
public:
	// IrradianceCache Public Methods
	IrradianceCache(int maxspec, int maxind, float maxerr, int nsamples);
	~IrradianceCache();
	Spectrum Li(MemoryArena &arena, const Scene *scene, const RayDifferential &ray, const Sample *sample, float *alpha) const;
	void RequestSamples(Sample *sample, const Scene *scene);
	void Preprocess(const Scene *);
	virtual IrradianceCache* clone() const; // Lux (copy) constructor for multithreading

	static SurfaceIntegrator *CreateSurfaceIntegrator(const ParamSet &params);
private:
	// IrradianceCache Data
	float maxError;
	int nSamples;
	int maxSpecularDepth, maxIndirectDepth;
	mutable int specularDepth;
	// Declare sample parameters for light source sampling
	int *lightSampleOffset, lightNumOffset;
	int *bsdfSampleOffset, *bsdfComponentOffset;
	mutable Octree<IrradianceSample, IrradProcess> *octree;
	// IrradianceCache Private Methods
	Spectrum IndirectLo(const Point &p, const Normal &n,
		const Vector &wo, BSDF *bsdf, BxDFType flags,
		const Sample *sample, const Scene *scene) const;
	bool InterpolateIrradiance(const Scene *scene,
			const Point &p, const Normal &n, Spectrum *E) const;
};
struct IrradianceSample {
	// IrradianceSample Constructor
	IrradianceSample() { }
	IrradianceSample(const Spectrum &e, const Point &P, const Normal &N,
		float md) : E(e), n(N), p(P) {
		maxDist = md;
	}
	Spectrum E;
	Normal n;
	Point p;
	float maxDist;
};
struct IrradProcess {
	// IrradProcess Public Methods
	IrradProcess(const Normal &N, float me) {
		n = N;
		maxError = me;
		nFound = samplesChecked = 0;
		sumWt = 0.;
		E = 0.;
	}
	void operator()(const Point &P, const IrradianceSample &sample) const;
	bool Successful() {
		return (sumWt > 0. && nFound > 0);
	}
	Spectrum GetIrradiance() const { return E / sumWt; }
	Normal n;
	float maxError;
	mutable int nFound, samplesChecked;
	mutable float sumWt;
	mutable Spectrum E;
};
