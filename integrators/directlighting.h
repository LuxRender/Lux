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

// directlighting.cpp*
#include "lux.h"
#include "transport.h"
#include "scene.h"
// DirectLighting Declarations
enum LightStrategy { SAMPLE_ALL_UNIFORM, SAMPLE_ONE_UNIFORM,
	SAMPLE_ONE_WEIGHTED  // NOBOOK
};
class DirectLighting : public SurfaceIntegrator {
public:
	// DirectLighting Public Methods
	DirectLighting(LightStrategy ls, int md);
	~DirectLighting();
	Spectrum Li(MemoryArena &arena, const Scene *scene, const RayDifferential &ray, const Sample *sample,
		float *alpha) const;
	void RequestSamples(Sample *sample, const Scene *scene) {
		if (strategy == SAMPLE_ALL_UNIFORM) {
			// Allocate and request samples for sampling all lights
			u_int nLights = scene->lights.size();
			lightSampleOffset = new int[nLights];
			bsdfSampleOffset = new int[nLights];
			bsdfComponentOffset = new int[nLights];
			for (u_int i = 0; i < nLights; ++i) {
				const Light *light = scene->lights[i];
				int lightSamples =
					scene->sampler->RoundSize(light->nSamples);
				lightSampleOffset[i] = sample->Add2D(lightSamples);
				bsdfSampleOffset[i] = sample->Add2D(lightSamples);
				bsdfComponentOffset[i] = sample->Add1D(lightSamples);
			}
			lightNumOffset = -1;
		}
		else {
			// Allocate and request samples for sampling one light
			lightSampleOffset = new int[1];
			lightSampleOffset[0] = sample->Add2D(1);
			lightNumOffset = sample->Add1D(1);
			bsdfSampleOffset = new int[1];
			bsdfSampleOffset[0] = sample->Add2D(1);
			bsdfComponentOffset = new int[1];
			bsdfComponentOffset[0] = sample->Add1D(1);
		}
	}
	virtual DirectLighting* clone() const; // Lux (copy) constructor for multithreading

	static SurfaceIntegrator *CreateSurfaceIntegrator(const ParamSet &params);
private:
	// DirectLighting Private Data
	LightStrategy strategy;
	mutable int rayDepth; // NOBOOK
	int maxDepth; // NOBOOK
	// Declare sample parameters for light source sampling
	int *lightSampleOffset, lightNumOffset;
	int *bsdfSampleOffset, *bsdfComponentOffset;
	mutable float *avgY, *avgYsample, *cdf;
	mutable float overallAvgY;
};
