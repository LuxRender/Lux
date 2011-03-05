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

#include "renderers/sppmrenderer.h"
#include "integrators/sppm.h"
#include "camera.h"
#include "film.h"
#include "sampling.h"
#include "light.h"
#include "reflection/bxdf.h"
#include "pixelsamplers/vegas.h"

using namespace lux;

//------------------------------------------------------------------------------
// HitPoints methods
//------------------------------------------------------------------------------

HitPoints::HitPoints(SPPMRenderer *engine, RandomGenerator *rng)  {
	renderer = engine;
	Scene *scene = renderer->scene;
	pass = 0;

	// Get the count of hit points required
	int xstart, xend, ystart, yend;
    scene->camera->film->GetSampleExtent(&xstart, &xend, &ystart, &yend);
	pixelSampler = new VegasPixelSampler(xstart, xend, ystart, yend);

	hitPoints = new std::vector<HitPoint>(pixelSampler->GetTotalPixels());
	LOG(LUX_INFO, LUX_NOERROR) << "Hit points count: " << hitPoints->size();

	// Initialize hit points field


	const u_int lightGroupsNumber = scene->lightGroups.size();

	for (u_int i = 0; i < (*hitPoints).size(); ++i) {
		HitPoint *hp = &(*hitPoints)[i];

		hp->sample = new Sample(NULL, scene->volumeIntegrator, *scene);

		Sample &sample(*hp->sample);
		sample.contribBuffer = NULL;
		sample.camera = scene->camera->Clone();
		sample.realTime = 0.f;

		hp->halton = new PermutedHalton(9, *rng);
		hp->haltonOffset = rng->floatValue();

		hp->lightGroupData.resize(lightGroupsNumber);
		
		for(u_int j = 0; j < lightGroupsNumber; j++) {
			hp->lightGroupData[j].photonCount = 0;
			hp->lightGroupData[j].reflectedFlux = XYZColor();

			// hp->accumPhotonRadius2 is initialized in the Init() method
			hp->lightGroupData[j].accumPhotonCount = 0;
			hp->lightGroupData[j].accumReflectedFlux = XYZColor();

			hp->lightGroupData[j].constantHitsCount = 0;
			hp->lightGroupData[j].surfaceHitsCount = 0;
			hp->lightGroupData[j].accumRadiance = XYZColor();
			hp->lightGroupData[j].radiance = XYZColor();
		}
	}
}

HitPoints::~HitPoints() {
	delete lookUpAccel;

	for (u_int i = 0; i < (*hitPoints).size(); ++i) {
		HitPoint *hp = &(*hitPoints)[i];

		delete hp->sample;
		delete hp->halton;
	}
	delete hitPoints;
	delete pixelSampler;
}

void HitPoints::Init() {
	// Not using UpdateBBox() because hp->accumPhotonRadius2 is not yet set
	BBox hpBBox = BBox();
	for (u_int i = 0; i < (*hitPoints).size(); ++i) {
		HitPoint *hp = &(*hitPoints)[i];

		if (hp->type == SURFACE)
			hpBBox = Union(hpBBox, hp->position);
	}

	// Calculate initial radius
	Vector ssize = hpBBox.pMax - hpBBox.pMin;
	const float photonRadius = renderer->sppmi->photonStartRadiusScale *
		((ssize.x + ssize.y + ssize.z) / 3.f) / sqrtf(pixelSampler->GetTotalPixels()) * 2.f;
	const float photonRadius2 = photonRadius * photonRadius;

	// Expand the bounding box by used radius
	hpBBox.Expand(photonRadius);
	// Update hit points information
	bbox = hpBBox;
	maxPhotonRaidus2 = photonRadius2;

	// Initialize hit points field
	for (u_int i = 0; i < (*hitPoints).size(); ++i) {
		HitPoint *hp = &(*hitPoints)[i];

		hp->accumPhotonRadius2 = photonRadius2;
	}

	// Allocate hit points lookup accelerator
	switch (renderer->sppmi->lookupAccelType) {
		case HASH_GRID:
			lookUpAccel = new HashGrid(this);
			break;
		case KD_TREE:
			lookUpAccel = new KdTree(this);
			break;
		case HYBRID_HASH_GRID:
			lookUpAccel = new HybridHashGrid(this);
			break;
		default:
			assert (false);
	}
}

void HitPoints::AccumulateFlux(const vector<unsigned long long> &photonTracedByLightGroup,
		const u_int index, const u_int count) {
	const unsigned int workSize = hitPoints->size() / count;
	const unsigned int first = workSize * index;
	const unsigned int last = (index == count - 1) ? hitPoints->size() : (first + workSize);
	assert (first >= 0);
	assert (last <= hitPoints->size());

	LOG(LUX_INFO, LUX_NOERROR) << "Accumulate photons flux: " << first << " to " << last - 1;

	const u_int lightGroupsNumber = renderer->scene->lightGroups.size();

	for (u_int i = first; i < last; ++i) {
		HitPoint *hp = &(*hitPoints)[i];

		switch (hp->type) {
			case CONSTANT_COLOR:
				for (u_int j = 0; j < lightGroupsNumber; ++j) {
					hp->lightGroupData[j].accumRadiance += hp->lightGroupData[j].eyeRadiance;
					hp->lightGroupData[j].constantHitsCount += 1;
				}
				break;
			case SURFACE:
				{
					// compute g and do radius reduction
					unsigned long long photonCount = 0;
					unsigned long long accumPhotonCount = 0;
					for (u_int j = 0; j < lightGroupsNumber; ++j) {
						photonCount += hp->lightGroupData[j].photonCount;
						accumPhotonCount += hp->lightGroupData[j].accumPhotonCount;

						hp->lightGroupData[j].photonCount += hp->lightGroupData[j].accumPhotonCount;
						hp->lightGroupData[j].accumPhotonCount = 0;
					}
					if (accumPhotonCount > 0) {
						const unsigned long long pcount = photonCount + accumPhotonCount;
						const double alpha = renderer->sppmi->photonAlpha;
						const float g = alpha * pcount / (photonCount * alpha + accumPhotonCount);

						// radius reduction
						hp->accumPhotonRadius2 *= g;

						// update light group flux
						for (u_int j = 0; j < lightGroupsNumber; ++j) {
							hp->lightGroupData[j].reflectedFlux = (hp->lightGroupData[j].reflectedFlux + hp->lightGroupData[j].accumReflectedFlux) * g;
							hp->lightGroupData[j].accumReflectedFlux = XYZColor();

							if (!hp->lightGroupData[j].eyeRadiance.Black()) {
								hp->lightGroupData[j].accumRadiance += hp->lightGroupData[j].eyeRadiance;
								hp->lightGroupData[j].constantHitsCount += 1;
							}
						}
					}

					for (u_int j = 0; j < lightGroupsNumber; ++j)
						hp->lightGroupData[j].surfaceHitsCount += 1;
				}
				break;
			default:
				assert (false);
		}

		// update radiance
		for(u_int j = 0; j < lightGroupsNumber; ++j) {
			const u_int hitCount = hp->lightGroupData[j].constantHitsCount + hp->lightGroupData[j].surfaceHitsCount;
			if (hitCount > 0) {
				const double k = 1.0 / (M_PI * hp->accumPhotonRadius2 * photonTracedByLightGroup[j]);

				hp->lightGroupData[j].radiance = (hp->lightGroupData[j].accumRadiance + hp->lightGroupData[j].surfaceHitsCount * hp->lightGroupData[j].reflectedFlux * k) / hitCount;
			}
		}
	}
}

void HitPoints::SetHitPoints(RandomGenerator *rng, const u_int pass,
		const u_int index, const u_int count) {
	const unsigned int workSize = hitPoints->size() / count;
	const unsigned int first = workSize * index;
	const unsigned int last = (index == count - 1) ? hitPoints->size() : (first + workSize);
	assert (first >= 0);
	assert (last <= hitPoints->size());

	LOG(LUX_INFO, LUX_NOERROR) << "Building hit points: " << first << " to " << last - 1;

	int xPos, yPos;
	for (u_int i = first; i < last; ++i) {
		HitPoint *hp = &(*hitPoints)[i];

		// Generate the sample values
		float u[9];
		hp->halton->Sample(pass, u);
		// Add an offset to the samples to avoid to start with 0.f values
		for (int j = 0; j < 9; ++j) {
			float v = u[j] + hp->haltonOffset;
			u[j] = (v >= 1.f) ? (v - 1.f) : v;
		}

		Sample &sample(*hp->sample);
		sample.arena.FreeAll();
		sample.rng = rng;

		pixelSampler->GetNextPixel(&xPos, &yPos, i);
		sample.imageX = xPos + u[0];
		sample.imageY = yPos + u[1];
		sample.lensU = u[2];
		sample.lensV = u[3];
		sample.time = u[4];
		sample.wavelengths = renderer->currentWavelengthSample;

		// This may be required by the volume integrator
		for (u_int j = 0; j < sample.n1D.size(); ++j)
			for (u_int k = 0; k < sample.n1D[j]; ++k)
				sample.oneD[j][k] = rng->floatValue();

		// Save ray time value
		sample.realTime = sample.camera->GetTime(sample.time);
		// Sample camera transformation
		sample.camera->SampleMotion(sample.realTime);

		// Sample new SWC thread wavelengths
		sample.swl.Sample(sample.wavelengths);

		// Trace the eye path
		TraceEyePath(hp, sample, &u[5]);
	}
}

void HitPoints::TraceEyePath(HitPoint *hp, const Sample &sample, const float *u) {
	Scene &scene(*renderer->scene);
	const RandomGenerator &rng(*sample.rng);
	const bool includeEnvironment = renderer->sppmi->includeEnvironment;
	const u_int maxDepth = renderer->sppmi->maxEyePathDepth;

	//--------------------------------------------------------------------------
	// Following code is, given or taken, a copy of path integrator Li() method
	//--------------------------------------------------------------------------

	// Declare common path integration variables
	const SpectrumWavelengths &sw(sample.swl);
	Ray ray;
	const float rayWeight = sample.camera->GenerateRay(scene, sample, &ray);

	const float nLights = scene.lights.size();
	SWCSpectrum pathThroughput(1.f);
	const u_int lightGroupCount = scene.lightGroups.size();
	vector<SWCSpectrum> L(lightGroupCount, 0.f);
	bool scattered = false;
	hp->eyeAlpha = 1.f;
	hp->eyeDistance = INFINITY;
	u_int vertexIndex = 0;
	const Volume *volume = NULL;

	// TODO: L[light group]

	float data[4];
	for (u_int pathLength = 0; ; ++pathLength) {
		if (pathLength == 0) {
			data[0] = u[0];
			data[1] = u[1];
			data[2] = u[2];
			data[3] = u[3];
		} else {
			data[0] = rng.floatValue();
			data[1] = rng.floatValue();
			data[2] = rng.floatValue();
			data[3] = rng.floatValue();
		}

		// Find next vertex of path
		Intersection isect;
		BSDF *bsdf;
		float spdf;
		if (!scene.Intersect(sample, volume, scattered, ray, data[3], &isect,
			&bsdf, &spdf, NULL, &pathThroughput)) {
			pathThroughput /= spdf;
			// Dade - now I know ray.maxt and I can call volumeIntegrator
			SWCSpectrum Lv;
			u_int g = scene.volumeIntegrator->Li(scene, ray, sample, &Lv, &hp->eyeAlpha);
			if (!Lv.Black()) {
				// TODO: copied from path integrator. Why not += ?
				L[g] = Lv;
			}

			// Stop path sampling since no intersection was found
			// Possibly add horizon in render & reflections
			if (includeEnvironment || (vertexIndex > 0)) {
				BSDF *ibsdf;
				for (u_int i = 0; i < nLights; ++i) {
					SWCSpectrum Le(pathThroughput);
					if (scene.lights[i]->Le(scene, sample, ray, &ibsdf, NULL, NULL, &Le))
						L[scene.lights[i]->group] += Le;
				}
			}

			// Set alpha channel
			if (vertexIndex == 0)
				hp->eyeAlpha = 0.f;

			hp->type = CONSTANT_COLOR;
			for(unsigned int j = 0; j < lightGroupCount; ++j)
				hp->lightGroupData[j].eyeRadiance = XYZColor(sw, L[j] * rayWeight);
			return;
		}
		scattered = bsdf->dgShading.scattered;
		pathThroughput /= spdf;
		if (vertexIndex == 0)
			hp->eyeDistance = ray.maxt * ray.d.Length();

		SWCSpectrum Lv;
		const u_int g = scene.volumeIntegrator->Li(scene, ray, sample, &Lv, &hp->eyeAlpha);
		if (!Lv.Black()) {
			Lv *= pathThroughput;
			L[g] += Lv;
		}

		// Possibly add emitted light at path vertex
		Vector wo(-ray.d);
		if (isect.arealight) {
			BSDF *ibsdf;
			SWCSpectrum Le(isect.Le(sample, ray, &ibsdf, NULL, NULL));
			if (!Le.Black()) {
				Le *= pathThroughput;
				L[isect.arealight->group] += Le;
			}
		}

		if (pathLength == maxDepth) {
			hp->type = CONSTANT_COLOR;
			for(unsigned int j = 0; j < lightGroupCount; ++j)
				hp->lightGroupData[j].eyeRadiance = XYZColor(sw, L[j] * rayWeight);
			return;
		}

		const Point &p = bsdf->dgShading.p;

		// Sample BSDF to get new path direction
		Vector wi;
		float pdf;
		BxDFType flags;
		SWCSpectrum f;
		if (!bsdf->SampleF(sw, wo, &wi, data[0], data[1], data[2], &f,
			&pdf, BSDF_ALL, &flags, NULL, true)) {
			hp->type = CONSTANT_COLOR;
			for(unsigned int j = 0; j < lightGroupCount; ++j)
				hp->lightGroupData[j].eyeRadiance = XYZColor(sw, L[j] * rayWeight);
			return;
		}

		if ((flags == BxDFType(BSDF_REFLECTION | BSDF_DIFFUSE)) ||
				(flags == BxDFType(BSDF_TRANSMISSION | BSDF_DIFFUSE))) {
			// It is a valid hit point
			hp->type = SURFACE;
			hp->bsdf = bsdf;
			hp->eyeThroughput = pathThroughput * rayWeight;
			for(unsigned int j = 0; j < lightGroupCount; ++j)
				hp->lightGroupData[j].eyeRadiance = XYZColor(sw, L[j] * rayWeight);
			hp->position = p;
			hp->wo = wo;
			return;
		}

		if (flags != (BSDF_TRANSMISSION | BSDF_SPECULAR) ||
			!(bsdf->Pdf(sw, wi, wo, BxDFType(BSDF_TRANSMISSION | BSDF_SPECULAR)) > 0.f))
			++vertexIndex;

		pathThroughput *= f;
		if (pathThroughput.Black()) {
			hp->type = CONSTANT_COLOR;
			for(unsigned int j = 0; j < lightGroupCount; ++j)
				hp->lightGroupData[j].eyeRadiance = XYZColor(sw, L[j] * rayWeight);
			return;
		}

		ray = Ray(p, wi);
		ray.time = sample.realTime;
		volume = bsdf->GetVolume(wi);
	}
}

void HitPoints::UpdatePointsInformation() {
	// Calculate hit points bounding box
	bbox = BBox();
	maxPhotonRaidus2 = 0.f;
	for (u_int i = 0; i < (*hitPoints).size(); ++i) {
		HitPoint *hp = &(*hitPoints)[i];

		if (hp->type == SURFACE) {
			bbox = Union(bbox, hp->position);
			maxPhotonRaidus2 = max<float>(maxPhotonRaidus2, hp->accumPhotonRadius2);
		}
	}
	LOG(LUX_INFO, LUX_NOERROR) << "Hit points bounding box: " << bbox;
}

void HitPoints::UpdateFilm() {
	// Assume a linear pixel sampler
	Scene &scene(*renderer->scene);
	const u_int bufferId = renderer->sppmi->bufferId;
	int xPos, yPos;
	u_int lightGroupsNumber = scene.lightGroups.size();

	Film &film(*scene.camera->film);
	for (u_int i = 0; i < GetSize(); ++i) {
		HitPoint *hp = GetHitPoint(i);
		pixelSampler->GetNextPixel(&xPos, &yPos, i);

		for(u_int j = 0; j < lightGroupsNumber; j++) {
			Contribution contrib(xPos, yPos, hp->lightGroupData[j].radiance, hp->eyeAlpha,
					hp->eyeDistance, 0.f, bufferId, j);
			film.SetSample(&contrib);
		}
	}
	scene.camera->film->CheckWriteOuputInterval();
}
