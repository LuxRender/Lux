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
#include "context.h"
#include "dynload.h"

using namespace lux;

//------------------------------------------------------------------------------
// HaltonEyeSampler methods
//------------------------------------------------------------------------------

HaltonEyeSampler::HaltonEyeSampler(int x0, int x1, int y0, int y1,
	const string &ps) : Sampler(x0, x1, y0, y1, 1)
{
	pixelSampler = MakePixelSampler(ps, x0, x1, y0, y1);
	nPixels = pixelSampler->GetTotalPixels();
	halton.reserve(nPixels);
	haltonOffset.reserve(nPixels);
}

//------------------------------------------------------------------------------
// HitPoints methods
//------------------------------------------------------------------------------

HitPoints::HitPoints(SPPMRenderer *engine, RandomGenerator *rng)  {
	renderer = engine;
	Scene *scene = renderer->scene;
	currentPass = 0;

	wavelengthSampleScramble = rng->uintValue();
	timeSampleScramble = rng->uintValue();
	wavelengthStratPasses = RoundUpPow2(renderer->sppmi->wavelengthStratification);
	if (wavelengthStratPasses > 0) {
		LOG(LUX_DEBUG, LUX_NOERROR) << "Non-random wavelength stratification for " << wavelengthStratPasses << " passes";
		wavelengthSample = 0.5f; // non-randomly stratified in IncPass for first N passes
	} else
		wavelengthSample = Halton(0, wavelengthSampleScramble);
	timeSample = Halton(0, timeSampleScramble);

	// Get the count of hit points required
	int xstart, xend, ystart, yend;
	scene->camera->film->GetSampleExtent(&xstart, &xend, &ystart, &yend);

	// Set the sampler
	eyeSampler = new HaltonEyeSampler(xstart, xend, ystart, yend,
		renderer->sppmi->PixelSampler);

	hitPoints = new std::vector<HitPoint>(eyeSampler->GetTotalSamplePos());
	LOG(LUX_DEBUG, LUX_NOERROR) << "Hit points count: " << hitPoints->size();

	// Initialize hit points field
	for (u_int i = 0; i < (*hitPoints).size(); ++i) {
		HitPoint *hp = &(*hitPoints)[i];

		hp->InitStats();
	}
}

HitPoints::~HitPoints() {
	delete lookUpAccel;
	delete hitPoints;
	delete eyeSampler;
}

const double HitPoints::GetPhotonHitEfficency() {
	u_int surfaceHitPointsCount = 0;
	u_int hitPointsUpdatedCount = 0;
	for (u_int i = 0; i < GetSize(); ++i) {
		HitPoint *hp = &(*hitPoints)[i];

		if (hp->IsSurface()) {
			++surfaceHitPointsCount;

			if (hp->GetPhotonCount() > 0)
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

		if (hp->IsSurface())
			hpBBox = Union(hpBBox, hp->GetPosition());
	}

	// Calculate initial radius
	Vector ssize = hpBBox.pMax - hpBBox.pMin;
	initialPhotonRadius = renderer->sppmi->photonStartRadiusScale *
		((ssize.x + ssize.y + ssize.z) / 3.f) / sqrtf(eyeSampler->GetTotalSamplePos()) * 2.f;
	const float photonRadius2 = initialPhotonRadius * initialPhotonRadius;

	// Expand the bounding box by used radius
	hpBBox.Expand(initialPhotonRadius);
	// Update hit points information
	hitPointBBox = hpBBox;
	maxHitPointRadius2 = photonRadius2;

	LOG(LUX_DEBUG, LUX_NOERROR) << "Hit points bounding box: " << hitPointBBox;
	LOG(LUX_DEBUG, LUX_NOERROR) << "Hit points max. radius: " << sqrtf(maxHitPointRadius2);

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
		case PARALLEL_HASH_GRID:
			lookUpAccel = new ParallelHashGrid(this, renderer->sppmi->parallelHashGridSpare);
			break;
		default:
			assert (false);
	}
}

void HitPoints::AccumulateFlux(const u_int index, const u_int count) {
	const unsigned int workSize = hitPoints->size() / count;
	const unsigned int first = workSize * index;
	const unsigned int last = (index == count - 1) ? hitPoints->size() : (first + workSize);
	assert (first >= 0);
	assert (last <= hitPoints->size());

	LOG(LUX_DEBUG, LUX_NOERROR) << "Accumulate photons flux: " << first << " to " << last - 1;

	for (u_int i = first; i < last; ++i) {
		HitPoint *hp = &(*hitPoints)[i];

		hp->DoRadiusReduction(renderer->sppmi->photonAlpha, GetPassCount(), renderer->sppmi->useproba);
	}
}

void HitPoints::SetHitPoints(Sample &sample, RandomGenerator *rng, const u_int index, const u_int count) {
	const unsigned int workSize = hitPoints->size() / count;
	const unsigned int first = workSize * index;
	const unsigned int last = (index == count - 1) ? hitPoints->size() : (first + workSize);

	assert (first >= 0);
	assert (last <= hitPoints->size());

	LOG(LUX_DEBUG, LUX_NOERROR) << "Building hit points: " << first << " to " << last - 1;
	sample.arena.FreeAll();
	for (u_int i = first; i < last; ++i) {
		static_cast<HaltonEyeSampler::HaltonEyeSamplerData *>(sample.samplerData)->index = i; //FIXME sampler data shouldn't be accessed directly
		static_cast<HaltonEyeSampler::HaltonEyeSamplerData *>(sample.samplerData)->pathCount = currentPass; //FIXME sampler data shouldn't be accessed directly
		sample.wavelengths = wavelengthSample;
		sample.time = timeSample;
		sample.swl.Sample(sample.wavelengths);
		sample.realTime = sample.camera->GetTime(sample.time);
		sample.camera->SampleMotion(sample.realTime);
		// Generate the sample values
		eyeSampler->GetNextSample(&sample);
		HitPoint *hp = &(*hitPoints)[i];

		// Trace the eye path
		TraceEyePath(hp, sample);

		// add contributions directly so we don't increase sample count
		// as sample count is a proxy for photon count which is used for 
		// weighting the photon buffer
		// eye buffer weighting is done per-pixel, so should work out
		for (u_int si = 0; si < sample.contributions.size(); ++si) {
			sample.contribBuffer->Add(sample.contributions[si], 1.f);
		}
		sample.contributions.clear();

		hp->imageX = sample.imageX;
		hp->imageY = sample.imageY;
	}
}

void HitPoints::TraceEyePath(HitPoint *hp, const Sample &sample)
{
	HitPointEyePass *hpep = &hp->eyePass;

	Scene &scene(*renderer->scene);
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
	const u_int lightGroupCount = scene.lightGroups.size();

	vector<SWCSpectrum> Ld(lightGroupCount, 0.f);
	// Direct lighting samples variance
	vector<float> Vd(lightGroupCount, 0.f);
	SWCSpectrum pathThroughput(1.f);
	vector<SWCSpectrum> L(lightGroupCount, 0.f);
	vector<float> V(lightGroupCount, 0.f);
	float VContrib = .1f;

	bool scattered = false;
	hpep->alpha = 1.f;
	hpep->distance = INFINITY;
	u_int vertexIndex = 0;
	const Volume *volume = NULL;

	for (u_int pathLength = 0; ; ++pathLength) {
		const SWCSpectrum prevThroughput(pathThroughput);

		float *data = eyeSampler->GetLazyValues(sample, 0, pathLength);

		// Find next vertex of path
		Intersection isect;
		BSDF *bsdf;
		float spdf;
		sample.arena.Begin();
		if (!scene.Intersect(sample, volume, scattered, ray, data[0],
			&isect, &bsdf, &spdf, NULL, &pathThroughput)) {
			pathThroughput /= spdf;
			// Dade - now I know ray.maxt and I can call volumeIntegrator
			SWCSpectrum Lv;
			u_int g = scene.volumeIntegrator->Li(scene, ray, sample,
				&Lv, &hpep->alpha);
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
					if (scene.lights[i]->Le(scene, sample,
						ray, &ibsdf, NULL, NULL, &Le))
						L[scene.lights[i]->group] += Le;
				}
			}

			// Set alpha channel
			if (vertexIndex == 0)
				hpep->alpha = 0.f;

			hp->SetConstant();
			break;
		}
		sample.arena.End();
		scattered = bsdf->dgShading.scattered;
		pathThroughput /= spdf;
		if (vertexIndex == 0)
			hpep->distance = ray.maxt * ray.d.Length();

		SWCSpectrum Lv;
		const u_int g = scene.volumeIntegrator->Li(scene, ray, sample,
			&Lv, &hpep->alpha);
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
				V[isect.arealight->group] += Le.Filter(sw) * VContrib;
			}
		}


		const Point &p = bsdf->dgShading.p;
		const Normal &n = bsdf->dgShading.nn;

		// Estimate direct lighting
		if (renderer->sppmi->directLightSampling && (nLights > 0)) {
			for (u_int i = 0; i < lightGroupCount; ++i) {
				Ld[i] = 0.f;
				Vd[i] = 0.f;
			}

			renderer->sppmi->hints.SampleLights(scene, sample, p, n,
				wo, bsdf, pathLength, pathThroughput, Ld, &Vd);

			for (u_int i = 0; i < lightGroupCount; ++i) {
				L[i] += Ld[i];
				V[i] += Vd[i] * VContrib;
			}
		}

		// Choose between storing or bouncing the hitpoint on the surface
		BxDFType store_component, bounce_component;

		store_component = BxDFType(BSDF_DIFFUSE | BSDF_REFLECTION | BSDF_TRANSMISSION);
		bounce_component = BxDFType(BSDF_SPECULAR | BSDF_GLOSSY | BSDF_REFLECTION | BSDF_TRANSMISSION);

		bool const has_store_component = bsdf->NumComponents(store_component) > 0;
		bool const has_bounce_component = bsdf->NumComponents(bounce_component) > 0;

		float pdf_event;
		bool store;

		if (has_store_component && has_bounce_component)
		{
			// There is both bounce and store component, we choose with a
			// random number
			// TODO: do this by importance
			store = data[4] < .5f;
			pdf_event = 0.5;
		}
		else
		{
			// If there is only bounce/store component, we bounce/store
			// accordingly
			store = has_store_component;
			pdf_event = 1.f;
		}

		if(store)
		{
			hp->SetSurface();
			hpep->pathThroughput = pathThroughput * rayWeight / pdf_event;
			hpep->wo = wo;

			hpep->bsdf = bsdf;
			sample.arena.Commit();
			break;
		}

		// Sample BSDF to get new path direction
		Vector wi;
		float pdf;
		BxDFType flags;
		SWCSpectrum f;
		if (pathLength == maxDepth || !bsdf->SampleF(sw, wo, &wi,
			data[1], data[2], data[3], &f, &pdf, bounce_component, &flags,
			NULL, true)) {
			hp->SetConstant();
			break;
		}

		if (flags != (BSDF_TRANSMISSION | BSDF_SPECULAR) ||
			!(bsdf->Pdf(sw, wi, wo, BxDFType(BSDF_TRANSMISSION | BSDF_SPECULAR)) > 0.f))
			++vertexIndex;

		pathThroughput *= f / pdf_event;
		if (pathThroughput.Black()) {
			hp->SetConstant();
			break;
		}

		ray = Ray(p, wi);
		ray.time = sample.realTime;
		volume = bsdf->GetVolume(wi);
	}
	for(unsigned int i = 0; i < lightGroupCount; ++i)
	{
		if (!L[i].Black())
			V[i] /= L[i].Filter(sw);
		sample.AddContribution(sample.imageX, sample.imageY,
			XYZColor(sw, L[i]) * rayWeight, hp->eyePass.alpha, hp->eyePass.distance,
			0, renderer->sppmi->bufferEyeId, i);
	}
}

void HitPoints::UpdatePointsInformation() {
	// Calculate hit points bounding box
	BBox bbox;
	float maxr2, minr2, meanr2;
	u_int minp, maxp, meanp;
	u_int surfaceHits, constantHits, zeroHits;

	surfaceHits = constantHits = zeroHits = 0;

	assert((*hitPoints).size() > 0);
	HitPoint *hp = &(*hitPoints)[0];

	maxr2 = minr2 = meanr2 = hp->accumPhotonRadius2;
	minp = maxp = meanp = hp->GetPhotonCount();

	for (u_int i = 1; i < (*hitPoints).size(); ++i) {
		hp = &(*hitPoints)[i];

		if (hp->IsSurface()) {
			u_int pc = hp->GetPhotonCount();
			if(pc == 0)
				++zeroHits;

			bbox = Union(bbox, hp->GetPosition());

			maxr2 = max<float>(maxr2, hp->accumPhotonRadius2);
			minr2 = min<float>(minr2, hp->accumPhotonRadius2);
			meanr2 += hp->accumPhotonRadius2;


			maxp = max<float>(maxp, pc);
			minp = min<float>(minp, pc);
			meanp += pc;

			++surfaceHits;
		}
		else
			++constantHits;
	}

	LOG(LUX_DEBUG, LUX_NOERROR) << "Hit points stats:";
	LOG(LUX_DEBUG, LUX_NOERROR) << "\tbounding box: " << bbox;
	LOG(LUX_DEBUG, LUX_NOERROR) << "\tmin/max radius: " << sqrtf(minr2) << "/" << sqrtf(maxr2);
	LOG(LUX_DEBUG, LUX_NOERROR) << "\tmin/max photonCount: " << minp << "/" << maxp;
	LOG(LUX_DEBUG, LUX_NOERROR) << "\tmean radius/photonCount: " << sqrtf(meanr2 / surfaceHits) << "/" << meanp / surfaceHits;
	LOG(LUX_DEBUG, LUX_NOERROR) << "\tconstant/zero hits: " << constantHits << "/" << zeroHits;

	hitPointBBox = bbox;
	maxHitPointRadius2 = maxr2;
}
