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

// exphotonmap.cpp*
#include "exphotonmap.h"
#include "bxdf.h"
#include "light.h"
#include "paramset.h"
#include "spectrumwavelengths.h"
#include "dynload.h"
#include "camera.h"
#include "sampling.h"
#include "scene.h"
#include "color.h"
#include "mc.h"

using namespace lux;

ExPhotonIntegrator::ExPhotonIntegrator(RenderingMode rm,
	u_int ndir, u_int ncaus, u_int nindir, u_int nrad, u_int nl,
	u_int mdepth, u_int mpdepth, float mdist, bool fg, u_int gs, float ga,
	PhotonMapRRStrategy rrstrategy, float rrcontprob, float distThreshold,
	string *mapsfn, bool dbgEnableDirect, bool dbgUseRadianceMap,
	bool dbgEnableCaustic, bool dbgEnableIndirect, bool dbgEnableSpecular) : SurfaceIntegrator()
{
	renderingMode = rm;

	nDirectPhotons = ndir;
	nCausticPhotons = ncaus;
	nIndirectPhotons = nindir;
	nRadiancePhotons = nrad;

	nLookup = nl;
	maxDistSquared = mdist * mdist;
	maxDepth = mdepth;
	maxPhotonDepth = mpdepth;
	causticMap = indirectMap = NULL;
	radianceMap = NULL;
	finalGather = fg;
	gatherSamples = gs;
	cosGatherAngle = cos(Radians(ga));

	rrStrategy = rrstrategy;
	rrContinueProbability = rrcontprob;

	distanceThreshold = distThreshold;

	mapsFileName = mapsfn;

	debugEnableDirect = dbgEnableDirect;
	debugUseRadianceMap = dbgUseRadianceMap;
	debugEnableCaustic = dbgEnableCaustic;
	debugEnableIndirect = dbgEnableIndirect;
	debugEnableSpecular = dbgEnableSpecular;

	AddStringConstant(*this, "name", "Name of current surface integrator", "exphotonmap");
}

ExPhotonIntegrator::~ExPhotonIntegrator()
{
	delete mapsFileName;
	delete causticMap;
	delete indirectMap;
	delete radianceMap;
}

void ExPhotonIntegrator::RequestSamples(Sample *sample, const Scene &scene)
{
	// Dade - allocate and request samples
	if (renderingMode == RM_DIRECTLIGHTING) {
		vector<u_int> structure;

		structure.push_back(2);	// reflection bsdf direction sample
		structure.push_back(1);	// reflection bsdf component sample
		structure.push_back(1); // scattering

		sampleOffset = sample->AddxD(structure, maxDepth + 1);

		if (finalGather) {
			// Dade - use half samples for sampling along the BSDF and the other
			// half to sample along photon incoming direction

			// Dade - request n samples for the final gather step
			structure.clear();
			structure.push_back(2);	// gather bsdf direction sample 1
			structure.push_back(1);	// gather bsdf component sample 1
			if (rrStrategy != RR_NONE)
				structure.push_back(1); // RR
			sampleFinalGather1Offset = sample->AddxD(structure, gatherSamples);

			structure.clear();
			structure.push_back(2);	// gather bsdf direction sample 2
			structure.push_back(1);	// gather bsdf component sample 2
			if (rrStrategy != RR_NONE)
				structure.push_back(1); // RR
			sampleFinalGather2Offset = sample->AddxD(structure, gatherSamples);
		}
	} else if (renderingMode == RM_PATH) {
		vector<u_int> structure;
		structure.push_back(2);	// bsdf direction sample for path
		structure.push_back(1);	// bsdf component sample for path
		structure.push_back(2);	// bsdf direction sample for indirect light
		structure.push_back(1);	// bsdf component sample for indirect light
		structure.push_back(1); // scattering

		if (rrStrategy != RR_NONE)
			structure.push_back(1);	// continue sample

		sampleOffset = sample->AddxD(structure, maxDepth + 1);
	} else
		BOOST_ASSERT(false);

	// Allocate and request samples for light sampling
	hints.RequestSamples(sample, scene, maxDepth + 1);
}

void ExPhotonIntegrator::Preprocess(const RandomGenerator &rng,
	const Scene &scene)
{
	// Prepare image buffers
	BufferType type = BUF_TYPE_PER_PIXEL;
	scene.sampler->GetBufferType(&type);
	bufferId = scene.camera->film->RequestBuffer(type, BUF_FRAMEBUFFER, "eye");

	hints.InitStrategies(scene);

	// Create the photon maps
	causticMap = new LightPhotonMap(nLookup, maxDistSquared);
	indirectMap = new LightPhotonMap(nLookup, maxDistSquared);

	if (finalGather)
		radianceMap = new RadiancePhotonMap(nLookup, maxDistSquared);
	else {
		nDirectPhotons = 0;
		nRadiancePhotons = 0;
	}

	PhotonMapPreprocess(rng, scene, mapsFileName,
		BxDFType(BSDF_DIFFUSE | BSDF_GLOSSY | BSDF_REFLECTION | BSDF_TRANSMISSION),
		BxDFType(BSDF_ALL),
		nDirectPhotons, nRadiancePhotons, radianceMap, nIndirectPhotons,
		indirectMap, nCausticPhotons, causticMap, maxPhotonDepth);
}

u_int ExPhotonIntegrator::Li(const Scene &scene, const Sample &sample) const 
{
	Ray ray;
	float xi, yi;
	float rayWeight = sample.camera->GenerateRay(scene, sample, &ray, &xi, &yi);

	SWCSpectrum L(0.f);
	float alpha = 1.f;
	switch (renderingMode) {
		case RM_DIRECTLIGHTING:
			L = LiDirectLightingMode(scene, sample, NULL, false,
				ray, &alpha, 0, true);
			break;
		case RM_PATH:
			L = LiPathMode(scene, sample, ray, &alpha);
			break;
		default:
			BOOST_ASSERT(false);
	}

	sample.AddContribution(xi, yi,
		XYZColor(sample.swl, L) * rayWeight, alpha, 0.f, 0.f,
		bufferId, 0);

	return L.Black() ? 0 : 1;
}

SWCSpectrum ExPhotonIntegrator::LiDirectLightingMode(const Scene &scene,
	const Sample &sample, const Volume *volume, bool scattered,
	const Ray &ray, float *alpha, const u_int reflectionDepth,
	const bool specularBounce) const 
{
	// Compute reflected radiance with photon map
	SWCSpectrum L(0.f), Lt(1.f);
	// TODO
	//u_int nContribs = 0;
	const float nLights = scene.lights.size();
	const u_int lightGroupCount = scene.lightGroups.size();
	const SpectrumWavelengths &sw(sample.swl);
	vector<SWCSpectrum> Ld(lightGroupCount, 0.f);

	Intersection isect;
	BSDF *bsdf;
	const float *sampleData = sample.sampler->GetLazyValues(sample, sampleOffset, reflectionDepth);
	float spdf;
	if (scene.Intersect(sample, volume, scattered, ray, sampleData[3],
		&isect, &bsdf, &spdf, NULL, &Lt)) {
		Vector wo = -ray.d;

		const Point &p = bsdf->dgShading.p;
		const Normal &ns = bsdf->dgShading.nn;
		const Normal &ng = isect.dg.nn;

		// Compute emitted light if ray hit an area light source
		if (specularBounce && isect.arealight) {
			BSDF *ibsdf;
			L += isect.Le(sample, ray, &ibsdf, NULL, NULL);
		}

		// Compute direct lighting
		if (debugEnableDirect && (nLights > 0)) {
			for (u_int i = 0; i < lightGroupCount; ++i)
				Ld[i] = 0.f;

			hints.SampleLights(scene, sample, p, ns, wo, bsdf,
				reflectionDepth, 1.f, Ld);

			for (u_int i = 0; i < lightGroupCount; ++i)
				L += Ld[i];
		}

		if (debugUseRadianceMap) {
			// Dade - for debugging
			Normal nGather = ng;
			if (Dot(nGather, wo) < 0.f)
				nGather = -nGather;

			NearPhotonProcess<RadiancePhoton> proc(p, nGather);
			float md2 = radianceMap->maxDistSquared;

			radianceMap->lookup(p, proc, md2);
			if (proc.photon)
				L += proc.photon->GetSWCSpectrum(sw);
		}

		if (debugEnableCaustic && (!causticMap->IsEmpty())) {
			// Compute indirect lighting for photon map integrator
			L += causticMap->LDiffusePhoton(sw, bsdf, isect, wo);
		}

		if (debugEnableIndirect) {
			if (finalGather)
				L += PhotonMapFinalGatherWithImportaceSampling(
					scene, sample, sampleFinalGather1Offset,
					sampleFinalGather2Offset, gatherSamples,
					cosGatherAngle, rrStrategy,
					rrContinueProbability, indirectMap,
					radianceMap, wo, bsdf,
					BxDFType(BSDF_DIFFUSE | BSDF_REFLECTION | BSDF_TRANSMISSION));
			else
				L += indirectMap->LDiffusePhoton(sw, bsdf,
					isect, wo);
		}

        if (debugEnableSpecular && (reflectionDepth < maxDepth)) {
			// Collect samples
			float u1 = sampleData[0];
			float u2 = sampleData[1];
			float u3 = sampleData[2];

			Vector wi;
			float pdf;
			SWCSpectrum f;
			BxDFType sampledType;
			// Trace rays for specular reflection and refraction
			if (bsdf->SampleF(sw, wo, &wi, u1, u2, u3, &f, &pdf,
				BxDFType(BSDF_REFLECTION | BSDF_TRANSMISSION | BSDF_SPECULAR | BSDF_GLOSSY), &sampledType, NULL, true)) {
				// Compute ray differential _rd_ for specular reflection
				Ray rd(p, wi);
				L += LiDirectLightingMode(scene, sample,
					bsdf->GetVolume(wi),
					bsdf->dgShading.scattered, rd, alpha,
					reflectionDepth + 1,
					(sampledType & BSDF_SPECULAR) != 0) * f;
			}
		}
	} else {
		// Handle ray with no intersection
		if (specularBounce) {
			BSDF *ibsdf;
			for (u_int i = 0; i < nLights; ++i) {
				SWCSpectrum Le(1.f);
				if (scene.lights[i]->Le(scene, sample, ray,
					&ibsdf, NULL, NULL, &Le))
					L += Le;
			}
		}

		if (reflectionDepth == 0)
			*alpha = 0.f;
	}
	Lt /= spdf;

	SWCSpectrum Lv;
	scene.volumeIntegrator->Li(scene, ray, sample, &Lv, alpha);
	return L * Lt + Lv;
}

SWCSpectrum ExPhotonIntegrator::LiPathMode(const Scene &scene,
	const Sample &sample, const Ray &r, float *alpha) const
{
	SWCSpectrum L(0.f);
	// TODO
	//u_int nContribs = 0;
	const float nLights = scene.lights.size();

	// Declare common path integration variables
	Ray ray(r);
	const SpectrumWavelengths &sw(sample.swl);
	SWCSpectrum pathThroughput(1.f);
	bool specularBounce = true, specular = true, scattered = false;
	const Volume *volume = NULL;

	for (u_int pathLength = 0; ; ++pathLength) {
		const float *sampleData = sample.sampler->GetLazyValues(sample,
			sampleOffset, pathLength);
		// Find next vertex of path
		Intersection isect;
		BSDF *bsdf;
		float spdf;
		if (!scene.Intersect(sample, volume, scattered, ray,
			sampleData[6], &isect, &bsdf, &spdf, NULL,
			&pathThroughput)) {
			pathThroughput /= spdf;
			// Stop path sampling since no intersection was found
			SWCSpectrum Lv;
			scene.volumeIntegrator->Li(scene, ray, sample, &Lv,
				alpha);
			Lv *= pathThroughput;
			L += Lv;

			// Possibly add emitted light if this was a specular reflection
			if (specularBounce) {
				SWCSpectrum Le(0.f);
				BSDF *ibsdf;
				for (u_int i = 0; i < nLights; ++i) {
					SWCSpectrum Ll(1.f);
					if (scene.lights[i]->Le(scene, sample,
						ray, &ibsdf, NULL, NULL, &Ll))
						Le += Ll;
				}
				L += Le * pathThroughput;
			}

			// Set alpha channel
			if (pathLength == 0)
				*alpha = 0.f;
			break;
		}
		scattered = bsdf->dgShading.scattered;
		if (pathLength == 0)
			r.maxt = ray.maxt;

		SWCSpectrum Lv;
		scene.volumeIntegrator->Li(scene, ray, sample, &Lv, alpha);
		Lv *= pathThroughput;
		L += Lv;

		SWCSpectrum currL(0.f);

		// Possibly add emitted light at path vertex
		Vector wo(-ray.d);
		
		if (specularBounce && isect.arealight) {
			BSDF *ibsdf;
			L += isect.Le(sample, ray, &ibsdf, NULL, NULL);
		}

		if (pathLength == maxDepth) {
			L += currL * pathThroughput;
			break;
		}

		// Dade - collect samples
		const float *pathSample = &sampleData[0];
		const float *pathComponent = &sampleData[2];
		const float *indirectSample = &sampleData[3];
		const float *indirectComponent = &sampleData[5];

		const float *rrSample;
		if (rrStrategy != RR_NONE)
			rrSample = &sampleData[7];
		else
			rrSample = NULL;

		// Compute direct lighting
		const Point &p = bsdf->dgShading.p;
		const Normal &n = bsdf->dgShading.nn;
		if (debugEnableDirect && (nLights > 0)) {
			const u_int lightGroupCount = scene.lightGroups.size();
			vector<SWCSpectrum> Ld(lightGroupCount, 0.f);
		// Sample illumination from lights to find path contribution
			hints.SampleLights(scene, sample, p, n, wo, bsdf,
				pathLength, 1.f, Ld);

			for (u_int i = 0; i < lightGroupCount; ++i)
				currL += Ld[i];
		}

		if (debugUseRadianceMap) {
			// Dade - for debugging
			currL += radianceMap->LPhoton(sw, isect, wo, BSDF_ALL);
		}

		bool sampledDiffuse = true;

		// Dade - add indirect lighting
		if (debugEnableIndirect) {
			BxDFType diffuseType = 
				BxDFType(BSDF_REFLECTION | BSDF_TRANSMISSION | BSDF_DIFFUSE);
			if (bsdf->NumComponents(diffuseType) > 0) {
				// Dade - use an additional ray for a finalgather-like step only
				// if we are on a specular path. Otherwise use directly the radiance map.

				if (specular) {
					Vector wi;
					float u1 = indirectSample[0];
					float u2 = indirectSample[1];
					float u3 = indirectComponent[0];
					float pdf;
					SWCSpectrum fr;
					if (bsdf->SampleF(sw, wo, &wi, u1, u2,
						u3, &fr, &pdf, diffuseType,
						NULL, NULL, true)) {
						Ray bounceRay(p, wi);

						Intersection gatherIsect;
						if (scene.Intersect(sample,
							bsdf->GetVolume(wi),
							scattered, bounceRay,
							1.f, &gatherIsect, NULL,
							NULL, NULL, &fr)) {
							// Dade - check the distance threshold option, if the intersection
							// distance is smaller than the threshold, revert to standard path
							// tracing in order to avoid corner artifacts

							if (bounceRay.maxt > distanceThreshold) {
								// Compute exitant radiance using precomputed irradiance
								SWCSpectrum Lindir = radianceMap->LPhoton(sw, gatherIsect, 
									-bounceRay.d, BSDF_ALL);
								if (!Lindir.Black()) {
									scene.Transmittance(bounceRay, sample, &Lindir);
									currL += fr * Lindir;
								}
							} else {
								// Dade - the intersection is too near, fall back to
								// standard path tracing

								sampledDiffuse = false;
							}
						}
					}
				} else {
					currL += indirectMap->LDiffusePhoton(sw, bsdf, isect, wo);
				}
			}
		}

		BxDFType componentsToSample = BSDF_ALL;
		if (sampledDiffuse) {
			// Dade - add caustic
			if (debugEnableCaustic && (!causticMap->IsEmpty())) {
				currL += causticMap->LDiffusePhoton(sw, bsdf, isect, wo);
			}

			componentsToSample = BxDFType(componentsToSample & (~BSDF_DIFFUSE));
		}
		if (!debugEnableSpecular)
			componentsToSample = BxDFType(componentsToSample & (~(BSDF_GLOSSY | BSDF_SPECULAR)));

		L += currL * pathThroughput;

		if (bsdf->NumComponents(componentsToSample) == 0)
			break;

		// Sample BSDF to get new path direction
		Vector wi;
		float pdf;
		BxDFType sampledType;
		SWCSpectrum f;
		if (!bsdf->SampleF(sw, wo, &wi, pathSample[0], pathSample[1], pathComponent[0],
			&f, &pdf, componentsToSample, &sampledType, NULL, true))
			break;

		// Possibly terminate the path
		if (pathLength > 3) {
			if (rrStrategy == RR_EFFICIENCY) { // use efficiency optimized RR
				const float q = min<float>(1.f, f.Filter(sw));
				if (q < rrSample[0])
					break;
				// increase path contribution
				pathThroughput /= q;
			} else if (rrStrategy == RR_PROBABILITY) { // use normal/probability RR
				if (rrContinueProbability < rrSample[0])
					break;
				// increase path contribution
				pathThroughput /= rrContinueProbability;
			}
		}

		specularBounce = (sampledType & BSDF_SPECULAR) != 0;
		specular = specular && specularBounce;
		pathThroughput *= f;

		ray = Ray(p, wi);
		volume = bsdf->GetVolume(wi);
	}

	return L;
}

SurfaceIntegrator* ExPhotonIntegrator::CreateSurfaceIntegrator(const ParamSet &params) {
	int maxDepth = params.FindOneInt("maxdepth", 5);
	int maxPhotonDepth = params.FindOneInt("maxphotondepth", 10);

	int nDirect = params.FindOneInt("directphotons", 200000);
	int nCaustic = params.FindOneInt("causticphotons", 20000);
	int nIndirect = params.FindOneInt("indirectphotons", 200000);
	int nRadiance = params.FindOneInt("radiancephotons", 200000);

	int nUsed = params.FindOneInt("nphotonsused", 50);
	float maxDist = params.FindOneFloat("maxphotondist", 0.5f);

	bool finalGather = params.FindOneBool("finalgather", true);
	// Dade - use half samples for sampling along the BSDF and the other
	// half to sample along photon incoming direction
	int gatherSamples = params.FindOneInt("finalgathersamples", 32) / 2;

	string smode =  params.FindOneString("renderingmode", "directlighting");

	RenderingMode renderingMode;
	if (smode == "directlighting")
		renderingMode = RM_DIRECTLIGHTING;
	else if (smode == "path") {
		renderingMode = RM_PATH;
		finalGather = true;
	} else {
		LOG(LUX_WARNING,LUX_BADTOKEN)<<"Strategy  '" << smode << "' for rendering mode unknown. Using \"path\".";
		renderingMode = RM_PATH;
	}

	float gatherAngle = params.FindOneFloat("gatherangle", 10.0f);

	PhotonMapRRStrategy rstrategy;
	string rst = params.FindOneString("rrstrategy", "efficiency");
	if (rst == "efficiency")
		rstrategy = RR_EFFICIENCY;
	else if (rst == "probability")
		rstrategy = RR_PROBABILITY;
	else if (rst == "none")
		rstrategy = RR_NONE;
	else {
		LOG(LUX_WARNING,LUX_BADTOKEN)<<"Strategy  '" << rst << "' for russian roulette path termination unknown. Using \"efficiency\".";
		rstrategy = RR_EFFICIENCY;
	}
	// continueprobability for plain RR (0.0-1.0)
	float rrcontinueProb = params.FindOneFloat("rrcontinueprob", 0.65f);

	float distanceThreshold = params.FindOneFloat("distancethreshold", maxDist * 1.25f);

	string *mapsFileName = NULL;
	string sfn = AdjustFilename(params.FindOneString("photonmapsfile", ""));
	if (sfn != "")
		mapsFileName = new string(sfn);

	bool debugEnableDirect = params.FindOneBool("dbg_enabledirect", true);
	bool debugUseRadianceMap = params.FindOneBool("dbg_enableradiancemap", false);
	bool debugEnableCaustic = params.FindOneBool("dbg_enableindircaustic", true);
	bool debugEnableIndirect = params.FindOneBool("dbg_enableindirdiffuse", true);
	bool debugEnableSpecular = params.FindOneBool("dbg_enableindirspecular", true);

    ExPhotonIntegrator *epi =  new ExPhotonIntegrator(renderingMode,
			max(nDirect, 0), max(nCaustic, 0), max(nIndirect, 0), max(nRadiance, 0),
            max(nUsed, 0), max(maxDepth, 0), max(maxPhotonDepth, 0), maxDist, finalGather, max(gatherSamples, 0), gatherAngle,
			rstrategy, rrcontinueProb,
			distanceThreshold,
			mapsFileName,
			debugEnableDirect, debugUseRadianceMap, debugEnableCaustic,
			debugEnableIndirect, debugEnableSpecular);
	// Initialize the rendering hints
	epi->hints.InitParam(params);

	return epi;
}

static DynamicLoader::RegisterSurfaceIntegrator<ExPhotonIntegrator> r("exphotonmap");
