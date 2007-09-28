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

// igi.cpp*
#include "lux.h"
#include "transport.h"
#include "scene.h"
#include "mc.h"
#include "sampling.h"

// IGI Local Structures
struct VirtualLight {
	VirtualLight() { }
	VirtualLight(const Point &pp, const Normal &nn, const Spectrum &le)
		: p(pp), n(nn), Le(le) { }
	Point p;
	Normal n;
	Spectrum Le;
};

class IGIIntegrator : public SurfaceIntegrator {
public:
	// IGIIntegrator Public Methods
	IGIIntegrator(int nl, int ns, float md, float rrt, float is);
	Spectrum Li(MemoryArena &arena, const Scene *scene, const RayDifferential &ray,
		const Sample *sample, float *alpha) const;
	void RequestSamples(Sample *sample, const Scene *scene);
	void Preprocess(const Scene *);
    virtual IGIIntegrator* clone() const; // Lux (copy) constructor for multithreading

	static SurfaceIntegrator *CreateSurfaceIntegrator(const ParamSet &params);
private:
	// IGI Private Data
	u_int nLightPaths, nLightSets;
	vector<VirtualLight> *virtualLights;
	mutable int specularDepth;
	int maxSpecularDepth;
	float minDist2, rrThreshold, indirectScale;
	int vlSetOffset;

	int *lightSampleOffset, lightNumOffset;
	int *bsdfSampleOffset, *bsdfComponentOffset;
};
