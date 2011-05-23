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

PhotonSampler::PhotonSampler(const RandomGenerator *randomGen) {
	rng = randomGen;
}

//------------------------------------------------------------------------------
// Halton Photon Sampler
//------------------------------------------------------------------------------

HaltonPhotonSampler::HaltonPhotonSampler(const Scene &scene,
		const RandomGenerator *randomGen) : PhotonSampler(randomGen) {
	sample = new Sample(NULL, scene.volumeIntegrator, scene);
	// Initialized later
	sample->rng = rng;
	sample->camera = scene.camera->Clone();
	sample->realTime = sample->camera->GetTime(.5f); //FIXME sample it
	sample->camera->SampleMotion(sample->realTime);
}

HaltonPhotonSampler::~HaltonPhotonSampler() {
	delete sample;
}

//------------------------------------------------------------------------------
// Adaptive Markov Chain Sampler
//------------------------------------------------------------------------------

AMCMCPhotonSampler::AMCMCPhotonSampler(const u_int maxPhotonDepth, const Scene &scene,
		const RandomGenerator *randomGen) : PhotonSampler(randomGen) {
	sample = new Sample(NULL, scene.volumeIntegrator, scene);
	// Initialized later
	sample->rng = rng;
	sample->camera = scene.camera->Clone();
	sample->realTime = sample->camera->GetTime(.5f); //FIXME sample it
	sample->camera->SampleMotion(sample->realTime);

	// 7 samples for the light source plus 4 samples for each photon path vertex
	const size_t size = 7 + 4 * maxPhotonDepth;
	data[0].resize(size, 0.f);
	data[1].resize(size, 0.f);

	currentIndex = 0;
	candidateIndex = 1;
}

AMCMCPhotonSampler::~AMCMCPhotonSampler() {
	delete sample;
}

float AMCMCPhotonSampler::MutateSingle(const float u, const float mutationSize) {
	// Delta U = SGN(2 E0 - 1) E1 ^ (1 / mutationSize + 1)

	const float du = powf(rng->floatValue(), 1.f / mutationSize + 1.f);

	if (rng->floatValue() < 0.5f) {
		float u1 = u + du;
		return (u1 < 1.f) ? u1 : u1 - 1.f;
	} else {
		float u1 = u - du;
		return (u1 < 0.f) ? u1 + 1.f : u1;
	}
}

void AMCMCPhotonSampler::Mutate(const float mutationSize) {
	vector<float> &current = data[currentIndex];
	vector<float> &candidate = data[candidateIndex];

	for (size_t i = 0; i < candidate.size(); ++i)
		candidate[i] = MutateSingle(current[i], mutationSize);
}
