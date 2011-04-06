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
 *   Lux Renderer website : http://www.luxrender.org                       *
 ***************************************************************************/

#include "sppm.h"
#include "sampling.h"
#include "scene.h"
#include "bxdf.h"
#include "light.h"
#include "camera.h"
#include "paramset.h"
#include "dynload.h"
#include "path.h"
#include "mc.h"

using namespace lux;

SPPMIntegrator::SPPMIntegrator() {
	bufferId = 0;
}

SPPMIntegrator::~SPPMIntegrator() {
}

void SPPMIntegrator::RequestSamples(Sample *sample, const Scene &scene) {
}

void SPPMIntegrator::Preprocess(const RandomGenerator &rng, const Scene &scene) {
	// Prepare image buffers
	BufferType type = BUF_TYPE_PER_PIXEL;
	scene.sampler->GetBufferType(&type);
	bufferId = scene.camera->film->RequestBuffer(type, BUF_FRAMEBUFFER, "eye");
	scene.camera->film->CreateBuffers();
}

u_int SPPMIntegrator::Li(const Scene &scene, const Sample &sample) const {
	// Something has gone wrong
	LOG(LUX_ERROR, LUX_SEVERE)<< "Internal error: called SPPMIntegrator::Li()";

	return 0;
}

SurfaceIntegrator* SPPMIntegrator::CreateSurfaceIntegrator(const ParamSet &params) {
	SPPMIntegrator *sppmi =  new SPPMIntegrator();

	// SPPM rendering parameters

	sppmi->lookupAccelType = HYBRID_HASH_GRID;
	sppmi->maxEyePathDepth = 16;
	sppmi->photonAlpha = 0.7f;
	sppmi->photonStartRadiusScale = 1.f;
	sppmi->maxPhotonPathDepth = 8;

	sppmi->stochasticInterval = 5000000;
	sppmi->useDirectLightSampling = false;

	return sppmi;
}

static DynamicLoader::RegisterSurfaceIntegrator<SPPMIntegrator> r("sppm");
