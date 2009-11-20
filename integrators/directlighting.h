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

// directlighting.cpp*
#include "lux.h"
#include "light.h"
#include "transport.h"
#include "scene.h"

namespace lux
{

// DirectLightingIntegrator Declarations
class DirectLightingIntegrator : public SurfaceIntegrator {
public:
	// DirectLightingIntegrator Public Methods
	DirectLightingIntegrator(u_int md);

	virtual u_int Li(const TsPack *tspack, const Scene *scene,
		const Sample *sample) const;
	virtual void RequestSamples(Sample *sample, const Scene *scene);
	virtual void Preprocess(const TsPack *tspack, const Scene *scene);

	static SurfaceIntegrator *CreateSurfaceIntegrator(const ParamSet &params);

private:
	u_int LiInternal(const TsPack *tspack, const Scene *scene, const RayDifferential &ray,
		const Sample *sample, vector<SWCSpectrum> &L, float *alpha, float &distance, u_int rayDepth) const;

	u_int maxDepth; // NOBOOK
	// Declare sample parameters for light source sampling
	u_int sampleOffset, bufferId;
};

}//namespace lux
