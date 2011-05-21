/***************************************************************************
 *   Copyright (C) 1998-2010 by authors (see AUTHORS.txt )                 *
 *                                                                         *
 *   This file is part of LuxRays.                                         *
 *                                                                         *
 *   LuxRays is free software; you can redistribute it and/or modify       *
 *   it under the terms of the GNU General Public License as published by  *
 *   the Free Software Foundation; either version 3 of the License, or     *
 *   (at your option) any later version.                                   *
 *                                                                         *
 *   LuxRays is distributed in the hope that it will be useful,            *
 *   but WITHOUT ANY WARRANTY; without even the implied warranty of        *
 *   MERCHANTABILITY or FITNESS FOR A PARTICULAR PURPOSE.  See the         *
 *   GNU General Public License for more details.                          *
 *                                                                         *
 *   You should have received a copy of the GNU General Public License     *
 *   along with this program.  If not, see <http://www.gnu.org/licenses/>. *
 *                                                                         *
 *   LuxRays website: http://www.luxrender.net                             *
 ***************************************************************************/

#include <cstdlib>

#include "camera.h"
#include "photonsampler.h"

using namespace lux;

PhotonSampler::PhotonSampler(const Scene &scene, const RandomGenerator *rng) {
	sample = new Sample(NULL, scene.volumeIntegrator, scene);
	// Initialized later
	sample->rng = rng;
	sample->camera = scene.camera->Clone();
	sample->realTime = sample->camera->GetTime(.5f); //FIXME sample it
	sample->camera->SampleMotion(sample->realTime);
}

PhotonSampler::~PhotonSampler() {
	delete sample;
}

//------------------------------------------------------------------------------
// Halton Photon Sampler
//------------------------------------------------------------------------------

HaltonPhotonSampler::HaltonPhotonSampler(const Scene &scene,
		const RandomGenerator *randomGen) : PhotonSampler(scene, randomGen) {
	rng = randomGen;
}

HaltonPhotonSampler::~HaltonPhotonSampler() {
}
