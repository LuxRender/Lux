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

// path.cpp*
#include "lux.h"
#include "transport.h"
#include "metropolis.h"
#include "scene.h"

namespace lux
{

// PathIntegrator Declarations
class PathIntegrator : public SurfaceIntegrator {
public:
	// PathIntegrator Public Methods
	Spectrum Li(const Scene *scene, const RayDifferential &ray, const Sample *sample, float *alpha) const;
	void RequestSamples(Sample *sample, const Scene *scene);
	PathIntegrator(int md, float cp, bool mlt, int maxreject, float plarge) { 
			maxDepth = md; continueProbability = cp; 
			useMlt = mlt; maxReject = maxreject; pLarge = plarge;
			lightPositionOffset = new int[maxDepth];
			lightNumOffset = new int[maxDepth];
			bsdfDirectionOffset = new int[maxDepth];
			bsdfComponentOffset = new int[maxDepth];
			continueOffset = new int[maxDepth];
			outgoingDirectionOffset = new int[maxDepth];
			outgoingComponentOffset = new int[maxDepth]; }
	virtual PathIntegrator* clone() const; // Lux (copy) constructor for multithreading
	virtual ~PathIntegrator() { delete[] outgoingDirectionOffset; delete[] outgoingComponentOffset; }
	IntegrationSampler* HasIntegrationSampler(IntegrationSampler *isa);
	static SurfaceIntegrator *CreateSurfaceIntegrator(const ParamSet &params);
private:
	// PathIntegrator Private Data
	int maxDepth;
	float continueProbability;
	bool useMlt;
	int maxReject;
	float pLarge;
	IntegrationSampler *mltIntegrationSampler;
	int *lightPositionOffset;
	int *lightNumOffset;
	int *bsdfDirectionOffset;
	int *bsdfComponentOffset;
	int *continueOffset;
	int *outgoingDirectionOffset;
	int *outgoingComponentOffset;
};

}//namespace lux

