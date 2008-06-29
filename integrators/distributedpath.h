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

// distributedpath.cpp*
#include "lux.h"
#include "light.h"
#include "transport.h"
#include "scene.h"

namespace lux
{

// DistributedPath Declarations

class DistributedPath : public SurfaceIntegrator {
public:
	// DistributedPath types
	enum LightStrategy { SAMPLE_ALL_UNIFORM, SAMPLE_ONE_UNIFORM,
		SAMPLE_AUTOMATIC
	};

	// DistributedPath Public Methods
	DistributedPath(LightStrategy ls, int dd, int gd, int sd);
	~DistributedPath() { }

	SWCSpectrum Li(const Scene *scene, const RayDifferential &ray, const Sample *sample,
		float *alpha) const;
	void RequestSamples(Sample *sample, const Scene *scene);

	static SurfaceIntegrator *CreateSurfaceIntegrator(const ParamSet &params);

private:
	SWCSpectrum LiInternal(const Scene *scene, const RayDifferential &ray,
		const Sample *sample, float *alpha, int rayDepth, bool includeEmit, bool caustic) const;

	// DistributedPath Private Data
	LightStrategy lightStrategy;

	int diffuseDepth, glossyDepth, specularDepth; // NOBOOK
	// Declare sample parameters for light source sampling
	int sampleOffset;
};

}//namespace lux

