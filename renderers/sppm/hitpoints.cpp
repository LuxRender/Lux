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
	currentEyePass = 0;
	currentPhotonPass = 0;

	wavelengthSampleScramble = rng->uintValue();
	eyePassWavelengthSample = Halton(0, wavelengthSampleScramble);
	photonPassWavelengthSample = Halton(0, wavelengthSampleScramble);

	// Get the count of hit points required
	int xstart, xend, ystart, yend;
    scene->camera->film->GetSampleExtent(&xstart, &xend, &ystart, &yend);
	pixelSampler = new VegasPixelSampler(xstart, xend, ystart, yend);

	hitPoints = new std::vector<HitPoint>(pixelSampler->GetTotalPixels());
	LOG(LUX_DEBUG, LUX_NOERROR) << "Hit points count: " << hitPoints->size();

	// Initialize hit points field
	const u_int lightGroupsNumber = scene->lightGroups.size();

	for (u_int i = 0; i < (*hitPoints).size(); ++i) {
		HitPoint *hp = &(*hitPoints)[i];

		hp->halton = new PermutedHalton(9, *rng);
		hp->haltonOffset = rng->floatValue();

		hp->lightGroupData.resize(lightGroupsNumber);
		hp->eyePass[0].emittedRadiance.resize(lightGroupsNumber);
		hp->eyePass[1].emittedRadiance.resize(lightGroupsNumber);
		
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
			// Debug code
			//hp->lightGroupData[j].radianceSSE = 0.f;
		}
	}
}

HitPoints::~HitPoints() {
	delete lookUpAccel[0];
	delete lookUpAccel[1];

	for (u_int i = 0; i < (*hitPoints).size(); ++i) {
		HitPoint *hp = &(*hitPoints)[i];

		delete hp->halton;
	}
	delete hitPoints;
	delete pixelSampler;
}

const double HitPoints::GetPhotonHitEfficency() {
	const u_int passIndex = currentPhotonPass % 2;
	const u_int lightGroupsNumber = renderer->scene->lightGroups.size();

	u_int surfaceHitPointsCount = 0;
	u_int hitPointsUpdatedCount = 0;
	for (u_int i = 0; i < GetSize(); ++i) {
		HitPoint *hp = &(*hitPoints)[i];
		HitPointEyePass *hpep = &hp->eyePass[passIndex];

		u_int photonHitsCount = 0;
		for(u_int j = 0; j < lightGroupsNumber; j++)
			photonHitsCount += hp->lightGroupData[j].accumPhotonCount;

		if (hpep->type == SURFACE) {
			++surfaceHitPointsCount;

			if (photonHitsCount > 0)
				++hitPointsUpdatedCount;
		}
	}

	return 100.0 * hitPointsUpdatedCount / surfaceHitPointsCount;
}

void HitPoints::Init() {
	// Not using UpdateBBox() because hp->accumPhotonRadius2 is not yet set
	BBox hpBBox = BBox();
	for (u_int i = 0; i < (*hitPoints).size(); ++i) {
		HitPoint *hp = &(*hitPoints)[i];
		HitPointEyePass *hpep = &hp->eyePass[0];

		if (hpep->type == SURFACE)
			hpBBox = Union(hpBBox, hpep->position);
	}

	// Calculate initial radius
	Vector ssize = hpBBox.pMax - hpBBox.pMin;
	initialPhotonRaidus = renderer->sppmi->photonStartRadiusScale *
		((ssize.x + ssize.y + ssize.z) / 3.f) / sqrtf(pixelSampler->GetTotalPixels()) * 2.f;
	const float photonRadius2 = initialPhotonRaidus * initialPhotonRaidus;

	// Expand the bounding box by used radius
	hpBBox.Expand(initialPhotonRaidus);
	// Update hit points information
	hitPointBBox[0] = hpBBox;
	maxHitPointRadius2[0] = photonRadius2;

	LOG(LUX_DEBUG, LUX_NOERROR) << "Hit points bounding box: " << hitPointBBox[0];
	LOG(LUX_DEBUG, LUX_NOERROR) << "Hit points max. radius: " << sqrtf(maxHitPointRadius2[0]);

	// Initialize hit points field
	for (u_int i = 0; i < (*hitPoints).size(); ++i) {
		HitPoint *hp = &(*hitPoints)[i];

		hp->accumPhotonRadius2 = photonRadius2;
	}

	// Allocate hit points lookup accelerator
	switch (renderer->sppmi->lookupAccelType) {
		case HASH_GRID:
			lookUpAccel[0] = new HashGrid(this);
			lookUpAccel[1] = new HashGrid(this);
			break;
		case KD_TREE:
			lookUpAccel[0] = new KdTree(this);
			lookUpAccel[1] = new KdTree(this);
			break;
		case HYBRID_HASH_GRID:
			lookUpAccel[0] = new HybridHashGrid(this);
			lookUpAccel[1] = new HybridHashGrid(this);
			break;
		case STOCHASTIC_HASH_GRID:
			lookUpAccel[0] = new StochasticHashGrid(this);
			lookUpAccel[1] = new StochasticHashGrid(this);
			break;
		case GRID:
			lookUpAccel[0] = new GridLookUpAccel(this);
			lookUpAccel[1] = new GridLookUpAccel(this);
			break;
		case CUCKOO_HASH_GRID:
			lookUpAccel[0] = new CuckooHashGrid(this);
			lookUpAccel[1] = new CuckooHashGrid(this);
			break;
		case HYBRID_MULTIHASH_GRID:
			lookUpAccel[0] = new HybridMultiHashGrid(this);
			lookUpAccel[1] = new HybridMultiHashGrid(this);
			break;
		case STOCHASTIC_MULTIHASH_GRID:
			lookUpAccel[0] = new StochasticMultiHashGrid(this);
			lookUpAccel[1] = new StochasticMultiHashGrid(this);
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

	LOG(LUX_DEBUG, LUX_NOERROR) << "Accumulate photons flux: " << first << " to " << last - 1;

	const u_int lightGroupsNumber = renderer->scene->lightGroups.size();

	const u_int passIndex = currentPhotonPass % 2;
	for (u_int i = first; i < last; ++i) {
		HitPoint *hp = &(*hitPoints)[i];
		HitPointEyePass *hpep = &hp->eyePass[passIndex];

		switch (hpep->type) {
			case CONSTANT_COLOR:
				for (u_int j = 0; j < lightGroupsNumber; ++j) {
					hp->lightGroupData[j].accumRadiance += hpep->emittedRadiance[j];
					hp->lightGroupData[j].constantHitsCount += 1;
				}
				break;
			case SURFACE:
				{
					// Compute g and do radius reduction
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

						// Radius reduction
						hp->accumPhotonRadius2 *= g;

						// update light group flux
						for (u_int j = 0; j < lightGroupsNumber; ++j) {
							hp->lightGroupData[j].reflectedFlux = (hp->lightGroupData[j].reflectedFlux + hp->lightGroupData[j].accumReflectedFlux) * g;
							hp->lightGroupData[j].accumReflectedFlux = XYZColor();

							if (!hpep->emittedRadiance[j].Black()) {
								hp->lightGroupData[j].accumRadiance += hpep->emittedRadiance[j];
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

		// Update radiance
		for(u_int j = 0; j < lightGroupsNumber; ++j) {
			const u_int hitCount = hp->lightGroupData[j].constantHitsCount + hp->lightGroupData[j].surfaceHitsCount;
			if (hitCount > 0) {
				const double k = 1.0 / (M_PI * hp->accumPhotonRadius2 * photonTracedByLightGroup[j]);

				const XYZColor newRadiance = (hp->lightGroupData[j].accumRadiance +
					hp->lightGroupData[j].surfaceHitsCount * hp->lightGroupData[j].reflectedFlux * k) / hitCount;

				// Debug code
				/*// Update Sum Square Error statistic
				if (hitCount > 1) {
					const float v = newRadiance.Y() - hp->lightGroupData[j].radiance.Y();
					hp->lightGroupData[j].radianceSSE += v * v;
				}*/

				hp->lightGroupData[j].radiance = newRadiance;
			}
		}
	}
}

void HitPoints::SetHitPoints(RandomGenerator *rng, const u_int index, const u_int count) {
	const unsigned int workSize = hitPoints->size() / count;
	const unsigned int first = workSize * index;
	const unsigned int last = (index == count - 1) ? hitPoints->size() : (first + workSize);
	assert (first >= 0);
	assert (last <= hitPoints->size());

	LOG(LUX_DEBUG, LUX_NOERROR) << "Building hit points: " << first << " to " << last - 1;

	Scene &scene(*renderer->scene);

	Sample sample(NULL, scene.volumeIntegrator, scene);
	sample.contribBuffer = NULL;
	sample.camera = scene.camera->Clone();
	sample.realTime = 0.f;
	sample.rng = rng;

	int xPos, yPos;
	for (u_int i = first; i < last; ++i) {
		HitPoint *hp = &(*hitPoints)[i];

		// Generate the sample values
		float u[9];
		hp->halton->Sample(currentEyePass, u);
		// Add an offset to the samples to avoid to start with 0.f values
		for (int j = 0; j < 9; ++j) {
			float v = u[j] + hp->haltonOffset;
			u[j] = (v >= 1.f) ? (v - 1.f) : v;
		}

		sample.arena.FreeAll();

		pixelSampler->GetNextPixel(&xPos, &yPos, i);
		sample.imageX = xPos + u[0];
		sample.imageY = yPos + u[1];
		sample.lensU = u[2];
		sample.lensV = u[3];
		sample.time = u[4];
		sample.wavelengths = eyePassWavelengthSample;

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
	HitPointEyePass *hpep = &hp->eyePass[currentEyePass % 2];

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
	hpep->alpha = 1.f;
	hpep->distance = INFINITY;
	u_int vertexIndex = 0;
	const Volume *volume = NULL;

	// TODO: L[light group]

	float data[4];
	for (u_int pathLength = 0; ; ++pathLength) {
		const SWCSpectrum prevThroughput(pathThroughput);

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
			u_int g = scene.volumeIntegrator->Li(scene, ray, sample, &Lv, &hpep->alpha);
			if (!Lv.Black()) {
				Lv *= prevThroughput;
				L[g] += Lv;
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
				hpep->alpha = 0.f;

			hpep->type = CONSTANT_COLOR;
			for(unsigned int j = 0; j < lightGroupCount; ++j)
				hpep->emittedRadiance[j] = XYZColor(sw, L[j] * rayWeight);
			return;
		}
		scattered = bsdf->dgShading.scattered;
		pathThroughput /= spdf;
		if (vertexIndex == 0)
			hpep->distance = ray.maxt * ray.d.Length();

		SWCSpectrum Lv;
		const u_int g = scene.volumeIntegrator->Li(scene, ray, sample, &Lv, &hpep->alpha);
		if (!Lv.Black()) {
			Lv *= prevThroughput;
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
			hpep->type = CONSTANT_COLOR;
			for(unsigned int j = 0; j < lightGroupCount; ++j)
				hpep->emittedRadiance[j] = XYZColor(sw, L[j] * rayWeight);
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
			hpep->type = CONSTANT_COLOR;
			for(unsigned int j = 0; j < lightGroupCount; ++j)
				hpep->emittedRadiance[j] = XYZColor(sw, L[j] * rayWeight);
			return;
		}

		if (flags == BxDFType(BSDF_REFLECTION | BSDF_DIFFUSE)) {
			// It is a valid hit point
			hpep->type = SURFACE;
			hpep->bsdfNG = bsdf->ng;
			hpep->bsdfNS = bsdf->dgShading.nn;
			const Vector vn(hpep->bsdfNS);
			hpep->bsdfRoverPI = bsdf->F(sw, vn, vn,
					false, BxDFType(BSDF_REFLECTION | BSDF_DIFFUSE));
			hpep->pathThroughput = pathThroughput * rayWeight;
			for(unsigned int j = 0; j < lightGroupCount; ++j)
				hpep->emittedRadiance[j] = XYZColor(sw, L[j] * rayWeight);
			hpep->position = p;
			hpep->wo = wo;
			return;
		}

		if (flags != (BSDF_TRANSMISSION | BSDF_SPECULAR) ||
			!(bsdf->Pdf(sw, wi, wo, BxDFType(BSDF_TRANSMISSION | BSDF_SPECULAR)) > 0.f))
			++vertexIndex;

		pathThroughput *= f;
		if (pathThroughput.Black()) {
			hpep->type = CONSTANT_COLOR;
			for(unsigned int j = 0; j < lightGroupCount; ++j)
				hpep->emittedRadiance[j] = XYZColor(sw, L[j] * rayWeight);
			return;
		}

		ray = Ray(p, wi);
		ray.time = sample.realTime;
		volume = bsdf->GetVolume(wi);
	}
}

void HitPoints::UpdatePointsInformation() {
	// Calculate hit points bounding box
	BBox bbox;
	float maxr2 = 0.f;
	const u_int passIndex = currentEyePass % 2;
	for (u_int i = 0; i < (*hitPoints).size(); ++i) {
		HitPoint *hp = &(*hitPoints)[i];
		HitPointEyePass *hpep = &hp->eyePass[passIndex];

		if (hpep->type == SURFACE) {
			bbox = Union(bbox, hpep->position);
			maxr2 = max<float>(maxr2, hp->accumPhotonRadius2);
		}
	}

	LOG(LUX_DEBUG, LUX_NOERROR) << "Hit points bounding box: " << bbox;
	LOG(LUX_DEBUG, LUX_NOERROR) << "Hit points max. radius: " << sqrtf(maxr2);

	hitPointBBox[passIndex] = bbox;
	maxHitPointRadius2[passIndex] = maxr2;
}

void HitPoints::UpdateFilm() {
	const u_int passIndex = currentPhotonPass % 2;

	Scene &scene(*renderer->scene);
	const u_int bufferId = renderer->sppmi->bufferId;
	int xPos, yPos;
	const u_int lightGroupsNumber = scene.lightGroups.size();
	Film &film(*scene.camera->film);

	/*if (renderer->sppmi->dbg_enableradiusdraw) {
		// Draw the radius of hit points
		XYZColor c;
		for (u_int i = 0; i < GetSize(); ++i) {
			HitPoint *hp = GetHitPoint(i);
			pixelSampler->GetNextPixel(&xPos, &yPos, i);

			for(u_int j = 0; j < lightGroupsNumber; j++) {
				if (hp->lightGroupData[j].surfaceHitsCount > 0)
					c.c[1] = sqrtf(hp->accumPhotonRadius2) / initialPhotonRaidus;
				else
					c.c[1] = 0;

				Contribution contrib(xPos, yPos, c, hp->eyeAlpha,
						hp->eyeDistance, 0.f, bufferId, j);
				film.SetSample(&contrib);
			}
		}
	} else if (renderer->sppmi->dbg_enablemsedraw) {
		// Draw the radius of hit points
		XYZColor c;
		for (u_int i = 0; i < GetSize(); ++i) {
			HitPoint *hp = GetHitPoint(i);
			pixelSampler->GetNextPixel(&xPos, &yPos, i);

			for(u_int j = 0; j < lightGroupsNumber; j++) {
				// Radiance Mean Square Error
				c.c[1] = hp->lightGroupData[j].radianceSSE /
						(hp->lightGroupData[j].constantHitsCount + hp->lightGroupData[j].surfaceHitsCount);

				Contribution contrib(xPos, yPos, c, hp->eyeAlpha,
						hp->eyeDistance, 0.f, bufferId, j);
				film.SetSample(&contrib);
			}
		}
	} else {*/
		// Just normal rendering
		for (u_int i = 0; i < GetSize(); ++i) {
			HitPoint *hp = &(*hitPoints)[i];
			HitPointEyePass *hpep = &hp->eyePass[passIndex];
			pixelSampler->GetNextPixel(&xPos, &yPos, i);

			for(u_int j = 0; j < lightGroupsNumber; j++) {
				Contribution contrib(xPos, yPos, hp->lightGroupData[j].radiance, hpep->alpha,
						hpep->distance, 0.f, bufferId, j);
				film.SetSample(&contrib);
			}
		}
	//}

	scene.camera->film->CheckWriteOuputInterval();
}
