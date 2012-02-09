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

SPPMIntegrator::SPPMIntegrator(): SurfaceIntegrator(), hints() {
	AddStringConstant(*this, "name", "Name of current surface integrator", "sppm");
}

SPPMIntegrator::~SPPMIntegrator() {
}

void SPPMIntegrator::Preprocess(const RandomGenerator &rng, const Scene &scene) {
	BufferOutputConfig config = BUF_FRAMEBUFFER;
	if (debug)
		config = BufferOutputConfig(config | BUF_STANDALONE);
	bufferPhotonId = scene.camera->film->RequestBuffer(BUF_TYPE_PER_SCREEN_SCALED, config, "photons");
	bufferEyeId = scene.camera->film->RequestBuffer(BUF_TYPE_PER_PIXEL, config, "eye");
	scene.camera->film->CreateBuffers();

	hints.InitStrategies(scene);
}

u_int SPPMIntegrator::Li(const Scene &scene, const Sample &sample) const {
	// Something has gone wrong
	LOG(LUX_ERROR, LUX_SEVERE)<< "Internal error: called SPPMIntegrator::Li()";

	return 0;
}

SurfaceIntegrator *SPPMIntegrator::CreateSurfaceIntegrator(const ParamSet &params) {
	SPPMIntegrator *sppmi =  new SPPMIntegrator();

	// SPPM rendering parameters

	string psamp = params.FindOneString("photonsampler", "halton");
	if (psamp == "halton") sppmi->photonSamplerType = HALTON;
	else if (psamp == "amc") sppmi->photonSamplerType = AMC;
	else {
		LOG(LUX_WARNING,LUX_BADTOKEN) << "Photon Sampler  '" << psamp <<"' unknown. Using \"halton\".";
		sppmi->photonSamplerType = HALTON;
	}

	string acc = params.FindOneString("lookupaccel", "hybridhashgrid");
	if (acc == "hashgrid") sppmi->lookupAccelType = HASH_GRID;
	else if (acc == "kdtree") sppmi->lookupAccelType = KD_TREE;
	else if (acc == "hybridhashgrid") sppmi->lookupAccelType = HYBRID_HASH_GRID;
	else if (acc == "parallelhashgrid") sppmi->lookupAccelType = PARALLEL_HASH_GRID;
	else {
		LOG(LUX_WARNING,LUX_BADTOKEN) << "Lookup accelerator  '" << acc <<"' unknown. Using \"hybridhashgrid\".";
		sppmi->lookupAccelType = HYBRID_HASH_GRID;
	}

	sppmi->PixelSampler = params.FindOneString("pixelsampler", "hilbert");

	sppmi->maxEyePathDepth = params.FindOneInt("maxeyedepth", 16);
	sppmi->photonAlpha = params.FindOneFloat("alpha", .7f);
	sppmi->photonStartRadiusScale = params.FindOneFloat("startradius", 2.f);
	//sppmi->photonStartK = params.FindOneInt("startk", 0);
	// TODO: disable because of incorrect
	sppmi->photonStartK = 0;
	sppmi->maxPhotonPathDepth = params.FindOneInt("maxphotondepth", 16);
	sppmi->GlossyThreshold = params.FindOneFloat("glossythreshold", 100.f);

	sppmi->parallelHashGridSpare = params.FindOneFloat("parallelhashgridspare", 1.0f);
	sppmi->photonPerPass = params.FindOneInt("photonperpass", 1000000);

	sppmi->includeEnvironment = params.FindOneBool("includeenvironment", true);
	sppmi->directLightSampling = params.FindOneBool("directlightsampling", false);
	sppmi->useproba = params.FindOneBool("useproba", false);

	sppmi->wavelengthStratification = max(params.FindOneInt("wavelengthstratificationpasses", 8), 0);

	/*sppmi->dbg_enableradiusdraw = params.FindOneBool("dbg_enableradiusdraw", false);
	sppmi->dbg_enablemsedraw = params.FindOneBool("dbg_enablemsedraw", false);*/

	sppmi->debug = params.FindOneBool("debug", false);

	// Initialize the rendering hints
	sppmi->hints.InitParam(params);

	return sppmi;
}

static DynamicLoader::RegisterSurfaceIntegrator<SPPMIntegrator> r("sppm");
