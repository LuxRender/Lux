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

// particletracing.h*
#include "lux.h"
#include "transport.h"
#include "scene.h"
#include "camera.h"	// For Sample_stub

namespace lux
{
// ParticleTracingIntegrator Declarations
class ParticleTracingIntegrator : public SurfaceIntegrator {
public:
	// ParticleTracingIntegrator Public Methods
	ParticleTracingIntegrator(int md, float rrpdf) { maxDepth = md; continueProbability = rrpdf;}
	SWCSpectrum Li(const Scene *scene, const RayDifferential &ray, const Sample *sample, float *alpha) const;
	void RequestSamples(Sample *sample, const Scene *scene);
	void Preprocess(const Scene *);
	ParticleTracingIntegrator* clone() const {
		return new ParticleTracingIntegrator(*this);
	}
	bool IsFluxBased() {
		return true;
	}
	bool NeedAddSampleInRender() {
		return false;
	}
	static SurfaceIntegrator *CreateSurfaceIntegrator(const ParamSet &params);
private:
	// ParticleTracingIntegrator Private Data
	int maxDepth;
	int sampleOffset;
	float continueProbability;
	int numOfLights;
	mutable Sample_stub sample_gen;
	mutable Ray ray_gen;
	vector <int> bufferIds;
};

}//namespace lux
