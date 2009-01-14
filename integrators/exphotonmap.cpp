/***************************************************************************
 *   Copyright (C) 1998-2008 by authors (see AUTHORS.txt )                 *
 *                                                                         *
 *   This file is part of Lux Renderer.                                    *
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
#include "mc.h"

#include <boost/thread/xtime.hpp>

using namespace lux;

ExPhotonIntegrator::ExPhotonIntegrator(
		RenderingMode rm,
		LightStrategy st,
		int ndir, int ncaus, int nindir, int nrad,
        int nl, int mdepth, float mdist, bool fg,
        int gs, float ga,
		PhotonMapRRStrategy rrstrategy, float rrcontprob,
		float distThreshold,
		string *mapsfn,
		bool dbgEnableDirect, bool dbgUseRadianceMap, bool dbgEnableCaustic,
		bool dbgEnableIndirect, bool dbgEnableSpecular) {
	renderingMode = rm;
	lightStrategy = st;

	nDirectPhotons = ndir;
    nCausticPhotons = ncaus;
    nIndirectPhotons = nindir;
	nRadiancePhotons = nrad;

    nLookup = nl;
    maxDistSquared = mdist * mdist;
    maxDepth = mdepth;
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
}

ExPhotonIntegrator::~ExPhotonIntegrator() {
	if( mapsFileName )
		delete mapsFileName;
    delete causticMap;
    delete indirectMap;
	if( radianceMap )
		delete radianceMap;
}

void ExPhotonIntegrator::RequestSamples(Sample *sample, const Scene *scene) {
	if (lightStrategy == SAMPLE_AUTOMATIC) {
		if (scene->lights.size() > 5)
			lightStrategy = SAMPLE_ONE_UNIFORM;
		else
			lightStrategy = SAMPLE_ALL_UNIFORM;
	}

	// Dade - allocate and request samples
	if (renderingMode == RM_DIRECTLIGHTING) {
		vector<u_int> structure;

		structure.push_back(2);	// light position sample
		structure.push_back(1);	// light number/portal sample
		structure.push_back(2);	// bsdf direction sample for light
		structure.push_back(1);	// bsdf component sample for light

		structure.push_back(2);	// reflection bsdf direction sample
		structure.push_back(1);	// reflection bsdf component sample
		structure.push_back(2);	// transmission bsdf direction sample
		structure.push_back(1);	// transmission bsdf component sample

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
		structure.push_back(2);	// light position sample
		structure.push_back(1);	// light number/portal sample
		structure.push_back(2);	// bsdf direction sample for light
		structure.push_back(1);	// bsdf component sample for light
		structure.push_back(2);	// bsdf direction sample for path
		structure.push_back(1);	// bsdf component sample for path
		structure.push_back(2);	// bsdf direction sample for indirect light
		structure.push_back(1);	// bsdf component sample for indirect light

		if (rrStrategy != RR_NONE)
			structure.push_back(1);	// continue sample
		
		sampleOffset = sample->AddxD(structure, maxDepth + 1);
	} else
		BOOST_ASSERT(false);
}

void ExPhotonIntegrator::Preprocess(const TsPack *tspack, const Scene *scene) {
	// Prepare image buffers
	BufferType type = BUF_TYPE_PER_PIXEL;
	scene->sampler->GetBufferType(&type);
	bufferId = scene->camera->film->RequestBuffer(type, BUF_FRAMEBUFFER, "eye");

	// Create the photon maps
	causticMap = new LightPhotonMap(nLookup, maxDistSquared);
	indirectMap = new LightPhotonMap(nLookup, maxDistSquared);

	if (finalGather)
		radianceMap = new RadiancePhotonMap(nLookup, maxDistSquared);
	else {
		nDirectPhotons = 0;
		nRadiancePhotons = 0;
	}

	PhotonMapPreprocess(
		tspack, 
		scene, 
		mapsFileName,
		BxDFType(BSDF_DIFFUSE | BSDF_REFLECTION | BSDF_TRANSMISSION),
		BxDFType(BSDF_DIFFUSE | BSDF_GLOSSY | BSDF_REFLECTION | BSDF_TRANSMISSION),
		nDirectPhotons,
		nRadiancePhotons, radianceMap,
		nIndirectPhotons, indirectMap,
		nCausticPhotons, causticMap);
}

SWCSpectrum ExPhotonIntegrator::Li(const TsPack *tspack, const Scene *scene, 
								   const RayDifferential &ray, const Sample *sample, float *alpha) const 
{
    SampleGuard guard(sample->sampler, sample);

	SWCSpectrum L = 0.0f;
	switch(renderingMode) {
		case RM_DIRECTLIGHTING:
			L = LiDirectLightingMode(tspack, scene, ray, sample, alpha, 0, true);
			break;
		case RM_PATH:
			L = LiPathMode(tspack, scene, ray, sample, alpha);
			break;
		default:
			BOOST_ASSERT(false);
	}

	sample->AddContribution(sample->imageX, sample->imageY,
		L.ToXYZ(tspack), alpha ? (*alpha) : 1.0f, bufferId);

    return 1.f;
}

SWCSpectrum ExPhotonIntegrator::LiDirectLightingMode(
	const TsPack *tspack, const Scene *scene, 
	const RayDifferential &ray, const Sample *sample, float *alpha,
	const int reflectionDepth, const bool specularBounce) const 
{
    // Compute reflected radiance with photon map
    SWCSpectrum L(0.0f);

	if (!indirectMap)
		return L;

	Intersection isect;
    if (scene->Intersect(ray, &isect)) {
		// Dade - collect samples
		float *sampleData = sample->sampler->GetLazyValues(const_cast<Sample *>(sample), sampleOffset, reflectionDepth);
		float *lightSample = &sampleData[0];
		float *bsdfSample = &sampleData[3];
		float *bsdfComponent = &sampleData[5];

		float *reflectionSample = &sampleData[6];
		float *reflectionComponent = &sampleData[8];
		float *transmissionSample = &sampleData[9];
		float *transmissionComponent = &sampleData[11];

		float *lightNum = &sampleData[2];

        if (alpha) *alpha = 1.0f;
        Vector wo = -ray.d;

		// Compute emitted light if ray hit an area light source
		if( specularBounce )
			L += isect.Le(tspack, wo);

        // Evaluate BSDF at hit point
        BSDF *bsdf = isect.GetBSDF(tspack, ray, fabsf(2.f * bsdfComponent[0] - 1.f));
        const Point &p = bsdf->dgShading.p;
        const Normal &ns = bsdf->dgShading.nn;
		const Normal &ng = isect.dg.nn;

		if (debugEnableDirect) {
			// Apply direct lighting strategy
			switch (lightStrategy) {
				case SAMPLE_ALL_UNIFORM:
					L += UniformSampleAllLights(tspack, scene, p, ns,
						wo, bsdf, sample, lightSample, lightNum, bsdfSample, bsdfComponent);
					break;
				case SAMPLE_ONE_UNIFORM:
					L += UniformSampleOneLight(tspack, scene, p, ns,
						wo, bsdf, sample, lightSample, lightNum, bsdfSample, bsdfComponent);
					break;
				default:
					break;
			}
		}

		if (debugUseRadianceMap) {
			// Dade - for debugging
			Normal nGather = ng;
			if (Dot(nGather, wo) < 0) nGather = -nGather;

			NearPhotonProcess<RadiancePhoton> proc(p, nGather);
			float md2 = radianceMap->maxDistSquared;

			radianceMap->lookup(p, proc, md2);
			if (proc.photon)
				L += proc.photon->GetSWCSpectrum( tspack );
		}

		if (debugEnableCaustic && (!causticMap->isEmpty())) {
			// Compute indirect lighting for photon map integrator
			L += causticMap->LDiffusePhoton(tspack, bsdf, isect, wo);
		}

		if (debugEnableIndirect) {
			if (finalGather)
				L += PhotonMapFinalGatherWithImportaceSampling(
						tspack,
						scene,
						sample,
						sampleFinalGather1Offset,
						sampleFinalGather2Offset,
						gatherSamples,
						cosGatherAngle,
						rrStrategy,
						rrContinueProbability,
						indirectMap,
						radianceMap,
						wo,
						bsdf,
						BxDFType(BSDF_DIFFUSE | BSDF_REFLECTION | BSDF_TRANSMISSION));
			else
				L += indirectMap->LDiffusePhoton(tspack, bsdf, isect, wo);
		}

        if (debugEnableSpecular && (reflectionDepth < maxDepth)) {
			float u1 = reflectionSample[0];
			float u2 = reflectionSample[1];
			float u3 = reflectionComponent[0];

            Vector wi;
			float pdf;
			SWCSpectrum f;
			BxDFType sampledType;
            // Trace rays for specular reflection and refraction
            if (bsdf->Sample_f(tspack, wo, &wi, u1, u2, u3, &f, &pdf,
					BxDFType(BSDF_REFLECTION | BSDF_SPECULAR | BSDF_GLOSSY), &sampledType)) {
                // Compute ray differential _rd_ for specular reflection
                RayDifferential rd(p, wi);
                rd.hasDifferentials = true;
                rd.rx.o = p + isect.dg.dpdx;
                rd.ry.o = p + isect.dg.dpdy;
                // Compute differential reflected directions
                Normal dndx = bsdf->dgShading.dndu * bsdf->dgShading.dudx +
                        bsdf->dgShading.dndv * bsdf->dgShading.dvdx;
                Normal dndy = bsdf->dgShading.dndu * bsdf->dgShading.dudy +
                        bsdf->dgShading.dndv * bsdf->dgShading.dvdy;
                Vector dwodx = -ray.rx.d - wo, dwody = -ray.ry.d - wo;
                float dDNdx = Dot(dwodx, ns) + Dot(wo, dndx);
                float dDNdy = Dot(dwody, ns) + Dot(wo, dndy);
                rd.rx.d = wi -
                        dwodx + 2 * Vector(Dot(wo, ns) * dndx +
                        dDNdx * ns);
                rd.ry.d = wi -
                        dwody + 2 * Vector(Dot(wo, ns) * dndy +
                        dDNdy * ns);
                L += LiDirectLightingMode(tspack, scene, rd, sample, alpha, reflectionDepth + 1, (sampledType & BSDF_SPECULAR) != 0 ) *
					f * (AbsDot(wi, ns) / pdf);
            }

			u1 = transmissionSample[0];
			u2 = transmissionSample[1];
			u3 = transmissionComponent[0];

            if (bsdf->Sample_f(tspack, wo, &wi, u1, u2, u3, &f, &pdf, 
					BxDFType(BSDF_TRANSMISSION | BSDF_SPECULAR | BSDF_GLOSSY), &sampledType)) {
                // Compute ray differential _rd_ for specular transmission
                RayDifferential rd(p, wi);
                rd.hasDifferentials = true;
                rd.rx.o = p + isect.dg.dpdx;
                rd.ry.o = p + isect.dg.dpdy;

                float eta = bsdf->eta;
                Vector w = -wo;
                if (Dot(wo, ns) < 0) eta = 1.f / eta;

                Normal dndx = bsdf->dgShading.dndu * bsdf->dgShading.dudx + bsdf->dgShading.dndv * bsdf->dgShading.dvdx;
                Normal dndy = bsdf->dgShading.dndu * bsdf->dgShading.dudy + bsdf->dgShading.dndv * bsdf->dgShading.dvdy;

                Vector dwodx = -ray.rx.d - wo, dwody = -ray.ry.d - wo;
                float dDNdx = Dot(dwodx, ns) + Dot(wo, dndx);
                float dDNdy = Dot(dwody, ns) + Dot(wo, dndy);

                float mu = eta * Dot(w, ns) - Dot(wi, ns);
                float dmudx = (eta - (eta * eta * Dot(w, ns)) / Dot(wi, ns)) * dDNdx;
                float dmudy = (eta - (eta * eta * Dot(w, ns)) / Dot(wi, ns)) * dDNdy;

                rd.rx.d = wi + eta * dwodx - Vector(mu * dndx + dmudx * ns);
                rd.ry.d = wi + eta * dwody - Vector(mu * dndy + dmudy * ns);
                L += LiDirectLightingMode(tspack, scene, rd, sample, alpha, reflectionDepth + 1, (sampledType & BSDF_SPECULAR) != 0) *
					(f * (1.0f / pdf)) * AbsDot(wi, ns);
            }
        }
    } else {
        // Handle ray with no intersection
		if( specularBounce ) {
			for (u_int i = 0; i < scene->lights.size(); ++i)
				L += scene->lights[i]->Le(tspack, ray);
		}

        if (alpha && !L.Black()) *alpha = 1.0f;
		else if(alpha) *alpha = 0.0f;
    }

	scene->volumeIntegrator->Transmittance(tspack, scene, ray, sample, alpha, &L);
	L += scene->volumeIntegrator->Li(tspack, scene, ray, sample, alpha);
    return L;
}

SWCSpectrum ExPhotonIntegrator::LiPathMode(
	const TsPack *tspack, const Scene *scene,
	const RayDifferential &r, const Sample *sample, float *alpha) const
{
	SWCSpectrum L(0.0f);

	if (!indirectMap)
		return L;

	// Declare common path integration variables
	RayDifferential ray(r);
	SWCSpectrum pathThroughput(1.0f);
	XYZColor color;
	bool specularBounce = true, specular = true;
	if (alpha) *alpha = 1.;

	for (int pathLength = 0; ; ++pathLength) {
		// Find next vertex of path
		Intersection isect;
		if (!scene->Intersect(ray, &isect)) {
			// Stop path sampling since no intersection was found
			L += scene->volumeIntegrator->Li(tspack, scene, ray, sample, alpha) * pathThroughput;

			// Possibly add emitted light if this was a specular reflection
			if (specularBounce) {
				scene->volumeIntegrator->Transmittance(tspack, scene, ray, sample, alpha, &pathThroughput);

				SWCSpectrum Le(0.f);
				for (u_int i = 0; i < scene->lights.size(); ++i)
					Le += scene->lights[i]->Le(tspack, ray);
				L += Le * pathThroughput;
			}

			// Set alpha channel
			if (pathLength == 0 && alpha && !(color.y() > 0.f))
				*alpha = 0.0f;
			break;
		}
		if (pathLength == 0)
			r.maxt = ray.maxt;

		L += scene->volumeIntegrator->Li(tspack, scene, ray, sample, alpha) * pathThroughput;

		scene->volumeIntegrator->Transmittance(tspack, scene, ray, sample, alpha, &pathThroughput);

		SWCSpectrum currL(0.f);

		// Possibly add emitted light at path vertex
		Vector wo(-ray.d);
		if (specularBounce) {
			currL += isect.Le(tspack, wo);
		}

		if (pathLength == maxDepth) {
			L += currL * pathThroughput;
			break;
		}

		// Dade - collect samples
		float *sampleData = sample->sampler->GetLazyValues(const_cast<Sample *>(sample), sampleOffset, pathLength);
		float *lightSample = &sampleData[0];
		float *bsdfSample = &sampleData[3];
		float *bsdfComponent = &sampleData[5];
		float *pathSample = &sampleData[6];
		float *pathComponent = &sampleData[8];
		float *indirectSample = &sampleData[9];
		float *indirectComponent = &sampleData[11];

		float *lightNum = &sampleData[2];

		float *rrSample;
		if (rrStrategy != RR_NONE)
			rrSample = &sampleData[12];
		else
			rrSample = NULL;
		
		// Evaluate BSDF at hit point
		BSDF *bsdf = isect.GetBSDF(tspack, ray, fabsf(2.f * bsdfComponent[0] - 1.f));
		// Sample illumination from lights to find path contribution
		const Point &p = bsdf->dgShading.p;
		const Normal &n = bsdf->dgShading.nn;

		// Dade - direct lighting
		if (debugEnableDirect) {
			switch (lightStrategy) {
				case SAMPLE_ALL_UNIFORM:
					currL += UniformSampleAllLights(tspack, scene, p, n,
						wo, bsdf, sample, lightSample, lightNum, bsdfSample, bsdfComponent);
					break;
				case SAMPLE_ONE_UNIFORM:
					currL += UniformSampleOneLight(tspack, scene, p, n,
						wo, bsdf, sample, lightSample, lightNum, bsdfSample, bsdfComponent);
					break;
				default:
					BOOST_ASSERT(false);
			}
		}

		if (debugUseRadianceMap) {
			// Dade - for debugging
			currL += radianceMap->LPhoton( tspack, isect, wo, BSDF_ALL );
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
					if (bsdf->Sample_f(tspack, wo, &wi, u1, u2, u3, &fr, &pdf, diffuseType)) {
						RayDifferential bounceRay(p, wi);

						Intersection gatherIsect;
						if (scene->Intersect(bounceRay, &gatherIsect)) {
							// Dade - check the distance threshold option, if the intersection
							// distance is smaller than the threshold, revert to standard path
							// tracing in order to avoid corner artifacts

							if (bounceRay.maxt > distanceThreshold) {
								// Compute exitant radiance using precomputed irradiance
								SWCSpectrum Lindir = radianceMap->LPhoton( tspack, gatherIsect, 
									-bounceRay.d, BSDF_ALL );
								if (!Lindir.Black()) {
									scene->Transmittance(tspack, bounceRay, sample, &Lindir);
									currL += fr * Lindir * (AbsDot(wi, n) / pdf);
								}
							} else {
								// Dade - the intersection is too near, fall back to
								// standard path tracing

								sampledDiffuse = false;
							}
						}
					}
				} else {
					currL += indirectMap->LDiffusePhoton(tspack, bsdf, isect, wo);
				}
			}
		}

		BxDFType componentsToSample = BSDF_ALL;
		if(sampledDiffuse) {
			// Dade - add caustic
			if (debugEnableCaustic && (!causticMap->isEmpty())) {
				currL += causticMap->LDiffusePhoton(tspack, bsdf, isect, wo);
			}

			componentsToSample = BxDFType(componentsToSample & (~BSDF_DIFFUSE));
		}
		if( !debugEnableSpecular )
			componentsToSample = BxDFType(componentsToSample & (~(BSDF_GLOSSY & BSDF_SPECULAR)));

		L += currL * pathThroughput;

		if (bsdf->NumComponents(componentsToSample) == 0)
			break;

		// Sample BSDF to get new path direction
		Vector wi;
		float pdf;
		BxDFType sampledType;
		SWCSpectrum f;
		if (!bsdf->Sample_f(tspack, wo, &wi, pathSample[0], pathSample[1], pathComponent[0],
				&f, &pdf, componentsToSample, &sampledType))
		{
			break;
		}

		const float dp = AbsDot(wi, n) / pdf;

		// Possibly terminate the path
		if (pathLength > 3) {
			if (rrStrategy == RR_EFFICIENCY) { // use efficiency optimized RR
				const float q = min<float>(1.f, f.filter(tspack) * dp);
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
		pathThroughput *= dp;

		ray = RayDifferential(p, wi);
	}

	return L;
}

SurfaceIntegrator* ExPhotonIntegrator::CreateSurfaceIntegrator(const ParamSet &params) {
	LightStrategy estrategy;
	string st = params.FindOneString("strategy", "auto");
	if (st == "one") estrategy = SAMPLE_ONE_UNIFORM;
	else if (st == "all") estrategy = SAMPLE_ALL_UNIFORM;
	else if (st == "auto") estrategy = SAMPLE_AUTOMATIC;
	else {
		std::stringstream ss;
		ss<<"Strategy  '"<<st<<"' for direct lighting unknown. Using \"auto\".";
		luxError(LUX_BADTOKEN,LUX_WARNING,ss.str().c_str());
		estrategy = SAMPLE_AUTOMATIC;
	}

	int maxDepth = params.FindOneInt("maxdepth", 5);

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

	string smode =  params.FindOneString("renderingmode", "path");

	RenderingMode renderingMode;
	if (smode == "directlighting") renderingMode = RM_DIRECTLIGHTING;
	else if (smode == "path") {
		renderingMode = RM_PATH;
		finalGather = true;
	} else {
		std::stringstream ss;
		ss<<"Strategy  '" << smode << "' for rendering mode unknown. Using \"path\".";
		luxError(LUX_BADTOKEN,LUX_WARNING,ss.str().c_str());
		renderingMode = RM_PATH;
	}

    float gatherAngle = params.FindOneFloat("gatherangle", 10.0f);

	PhotonMapRRStrategy rstrategy;
	string rst = params.FindOneString("rrstrategy", "efficiency");
	if (rst == "efficiency") rstrategy = RR_EFFICIENCY;
	else if (rst == "probability") rstrategy = RR_PROBABILITY;
	else if (rst == "none") rstrategy = RR_NONE;
	else {
		std::stringstream ss;
		ss<<"Strategy  '"<<st<<"' for russian roulette path termination unknown. Using \"efficiency\".";
		luxError(LUX_BADTOKEN,LUX_WARNING,ss.str().c_str());
		rstrategy = RR_EFFICIENCY;
	}
	// continueprobability for plain RR (0.0-1.0)
	float rrcontinueProb = params.FindOneFloat("rrcontinueprob", 0.65f);

	float distanceThreshold = params.FindOneFloat("distancethreshold", maxDist * 1.25f);

	string *mapsFileName = NULL;
	string sfn = params.FindOneString("photonmapsfile", "");
	if (sfn != "")
		mapsFileName = new string(sfn);

	bool debugEnableDirect = params.FindOneBool("dbg_enabledirect", true);
	bool debugUseRadianceMap = params.FindOneBool("dbg_enableradiancemap", false);
	bool debugEnableCaustic = params.FindOneBool("dbg_enableindircaustic", true);
	bool debugEnableIndirect = params.FindOneBool("dbg_enableindirdiffuse", true);
	bool debugEnableSpecular = params.FindOneBool("dbg_enableindirspecular", true);

    return new ExPhotonIntegrator(renderingMode, estrategy,
			nDirect, nCaustic, nIndirect, nRadiance,
            nUsed, maxDepth, maxDist, finalGather, gatherSamples, gatherAngle,
			rstrategy, rrcontinueProb,
			distanceThreshold,
			mapsFileName,
			debugEnableDirect, debugUseRadianceMap, debugEnableCaustic,
			debugEnableIndirect, debugEnableSpecular);
}

static DynamicLoader::RegisterSurfaceIntegrator<ExPhotonIntegrator> r("exphotonmap");
