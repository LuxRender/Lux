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
#include "light.h"
#include "integrators/sppm.h"
#include "reflection/bxdf.h"
#include "renderers/sppmrenderer.h"
#include "mcdistribution.h"

#include "photonsampler.h"

using namespace lux;

// Photon tracing

bool PhotonSampler::ContinueTracing() const
{
	if(renderer->paused())
		return false;

	return renderer->photonTracedPass < renderer->sppmi->photonPerPass;
}

void PhotonSampler::IncPhoton() const
{
	osAtomicInc(&renderer->photonTracedPass);
}

void PhotonSampler::AddFluxToHitPoint(const Sample *sample, const u_int lightGroup, HitPoint * const hp, const XYZColor flux)
{
	// TODO: it should be more something like:
	//XYZColor flux = XYZColor(sw, photonFlux * f) * XYZColor(hp->sample->swl, hp->eyeThroughput);
	hp->IncPhoton();

	sample->AddContribution(hp->imageX, hp->imageY,
		flux, hp->eyePass.alpha, hp->eyePass.distance,
		0, renderer->sppmi->bufferPhotonId, lightGroup);
};
//------------------------------------------------------------------------------
// Tracing photons for Photon Sampler
//------------------------------------------------------------------------------

void PhotonSampler::TracePhoton(
	Sample *sample,
	Distribution1D *lightCDF)
{
	Scene &scene = *renderer->scene;
	// I have to make a copy of SpectrumWavelengths because it can be modified
	// even if passed as a const argument !
	SpectrumWavelengths sw(sample->swl);

	// Trace a photon path and store contribution
	float u[2];
	// Choose light to shoot photon from
	float lightPdf;
	u_int lightNum = lightCDF->SampleDiscrete(GetOneD(*sample, 0, 0), &lightPdf);
	const Light *light = scene.lights[lightNum];

	// Generate _photonRay_ from light source and initialize _alpha_
	BSDF *bsdf;
	float pdf;
	SWCSpectrum alpha;
	GetTwoD(*sample, 0, 0, u);
	if (!light->SampleL(scene, *sample, u[0], u[1],
		GetOneD(*sample, 1, 0), &bsdf, &pdf, &alpha))
		return;

	Ray photonRay;
	photonRay.o = bsdf->dgShading.p;
	float pdf2;
	SWCSpectrum alpha2;
	GetTwoD(*sample, 1, 0, u);
	if (!bsdf->SampleF(sw, Vector(bsdf->dgShading.nn), &photonRay.d,
		u[0], u[1], GetOneD(*sample, 2, 0), &alpha2,
		&pdf2))
		return;
	alpha *= alpha2;
	alpha /= lightPdf;

	const bool directLightSampling = renderer->sppmi->directLightSampling;

	// store the state of the path:
	//     - if directLightPath is true, the photon is still on a direct light
	//     path and should not be accounted for if directLightSampling is true
	//     - else, the photon has survived an indirect bounce, so it must be
	//     accounted in the density estimation.
	bool directLightPath = true;

	if (!alpha.Black()) {
		// Follow photon path through scene and record intersections
		Intersection photonIsect;
		const Volume *volume = bsdf->GetVolume(photonRay.d);
		BSDF *photonBSDF;
		u_int nIntersections = 0;
		u_int diffuseVertices = 0;
		while (scene.Intersect(*sample, volume, false,
			photonRay, 1.f, &photonIsect, &photonBSDF,
			NULL, NULL, &alpha)) {
			++nIntersections;

			// Handle photon/surface intersection
			Vector wi = -photonRay.d;

			// Deposit Flux (only if we have hit a diffuse or glossy surface)
			// Note: the hitpoint BSDF allready handle this test, but it optimise a bit and avoid same bias
			if(!directLightPath || !directLightSampling)
				if (photonBSDF->NumComponents(BxDFType(BSDF_REFLECTION | BSDF_TRANSMISSION | BSDF_GLOSSY | BSDF_DIFFUSE)) > 0)
				{
					PhotonData photon;
					photon.p = photonIsect.dg.p;
					photon.wi = wi;
					photon.alpha = alpha;
					photon.lightGroup = light->group;

					renderer->hitPoints->AddFlux(*sample, photon);
				}

			if (nIntersections > renderer->sppmi->maxPhotonPathDepth)
				break;

			// Sample new photon ray direction
			Vector wo;
			float pdfo;
			BxDFType flags;
			// Get random numbers for sampling outgoing photon direction
			float *data = GetLazyValues(*sample, 0, nIntersections);

			// Compute new photon weight and possibly terminate with RR
			SWCSpectrum fr;
			if (!photonBSDF->SampleF(sw, wi, &wo, data[0],
				data[1], data[2], &fr, &pdfo, BSDF_ALL,
				&flags))
				break;

			diffuseVertices += (flags & BSDF_DIFFUSE) ? 1 : 0;
			if (diffuseVertices > 0) {
				// Russian Roulette
				const float continueProb = min(1.f, fr.Filter(sw));
				if (data[3] > continueProb)
					break;

				alpha /= continueProb;
			}

			alpha *= fr;
			photonRay = Ray(photonIsect.dg.p, wo);
			volume = photonBSDF->GetVolume(photonRay.d);

			// Check if the scattering is not a passthrough event
			if (flags != (BSDF_TRANSMISSION | BSDF_SPECULAR) ||
				!(photonBSDF->Pdf(sw, wo, wi, BxDFType(BSDF_TRANSMISSION | BSDF_SPECULAR)) > 0.f)) {
				// this is not a passthrough event, so now the photon path is indirect light
				directLightPath = false;
			}
		}
	}
	sample->arena.FreeAll();
}


//------------------------------------------------------------------------------
// Photon Sampler
//------------------------------------------------------------------------------

void PhotonSampler::TracePhotons(
		Sample *sample,
		Distribution1D *lightCDF)
{
	while(ContinueTracing())
	{
		GetNextSample(sample);

		IncPhoton();
		TracePhoton(sample, lightCDF);

		dynamic_cast<Sampler*>(this)->AddSample(*sample);
	}
}

//------------------------------------------------------------------------------
// Halton Photon Sampler
//------------------------------------------------------------------------------

//------------------------------------------------------------------------------
// Adaptive Markov Chain Sampler
//------------------------------------------------------------------------------

void AMCMCPhotonSampler::TracePhotons(
		Sample *sample,
		Distribution1D *lightCDF)
{
	// Sample uniform
	do
	{
		GetNextSample(sample, true);
		TracePhoton(sample, lightCDF);
	} while(!pathCandidate->isVisible());

	swap(); // Current = Candidate

	while(ContinueTracing()) {
		IncPhoton();

		// Sample Uniform
		GetNextSample(sample, true);
		TracePhoton(sample, lightCDF);

		if(pathCandidate->isVisible())
		{
			swap();
			osAtomicInc(&renderer->uniformCount);
		}
		else
		{
			++mutated;

			// Sample mutated
			GetNextSample(sample, false);
			TracePhoton(sample, lightCDF);

			if(pathCandidate->isVisible())
			{
				++accepted;
				swap();
			}

			const float R = accepted / (float)mutated;
			mutationSize += (R - 0.234f) / mutated;

		}
		pathCurrent->Splat(sample, this);
		dynamic_cast<Sampler*>(this)->AddSample(*sample);
	}

	LOG(LUX_DEBUG, LUX_NOERROR) << "AMCMC mutationSize " << mutationSize << " accepted " << accepted << " mutated " << mutated << " uniform " << renderer->uniformCount;
}

// -------------------------------------
// AMCMCPhotonSampler sampler data
// -------------------------------------

void AMCMCPhotonSampler::AMCMCPhotonSamplerData::Mutate(const RandomGenerator * const rng, AMCMCPhotonSamplerData &source, const float mutationSize) const {
	for (size_t i = 0; i < n; ++i)
		values[0][i] = MutateSingle(rng, source.values[0][i], mutationSize);
}

float AMCMCPhotonSampler::AMCMCPhotonSamplerData::MutateSingle(const RandomGenerator * const rng, const float u, const float mutationSize) {
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
