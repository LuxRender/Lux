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

#include <boost/thread/xtime.hpp>

using namespace lux;

// Lux (copy) constructor
ExPhotonIntegrator* ExPhotonIntegrator::clone() const {
    return new ExPhotonIntegrator(*this);
}

ExPhotonIntegrator::ExPhotonIntegrator(
		RenderingMode rm,
		LightStrategy st,
		int ncaus, int nind, int maxDirPhotons,
        int nl, int mdepth, float mdist, bool fg,
        int gs, float ga,
		PhotonMapRRStrategy rrstrategy, float rrcontprob,
		string *mapsfn,
		bool dbgEnableDirect, bool dbgEnableCaustic,
		bool dbgEnableIndirect, bool dbgEnableSpecular) {
	renderingMode = rm;
	lightStrategy = st;

    nCausticPhotons = ncaus;
    nIndirectPhotons = nind;
	maxDirectPhotons = maxDirPhotons;

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

	mapsFileName = mapsfn;

	debugEnableDirect = dbgEnableDirect;
	debugEnableCaustic = dbgEnableCaustic;
	debugEnableIndirect = dbgEnableIndirect;
	debugEnableSpecular = dbgEnableSpecular;
}

ExPhotonIntegrator::~ExPhotonIntegrator() {
	delete mapsFileName;
    delete causticMap;
    delete indirectMap;
    delete radianceMap;
}

void ExPhotonIntegrator::RequestSamples(Sample *sample,
        const Scene *scene) {
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

void ExPhotonIntegrator::Preprocess(const Scene *scene) {
	if (finalGather)
		radianceMap = new RadiancePhotonMap(nLookup, maxDistSquared);
	else
		maxDirectPhotons = 0;

	indirectMap = new LightPhotonMap(nLookup, maxDistSquared);
	causticMap = new LightPhotonMap(nLookup, maxDistSquared);

	PhotonMapPreprocess(
			scene, mapsFileName,
			maxDirectPhotons, radianceMap,
			nIndirectPhotons, indirectMap,
			nCausticPhotons, causticMap);
}

SWCSpectrum ExPhotonIntegrator::Li(const Scene *scene,
        const RayDifferential &ray, const Sample *sample,
        float *alpha) const {
    SampleGuard guard(sample->sampler, sample);

	switch(renderingMode) {
		case RM_DIRECTLIGHTING: {
			SWCSpectrum L = LiDirectLigthtingMode(0, scene, ray, sample, alpha);
			sample->AddContribution(sample->imageX, sample->imageY,
					L.ToXYZ(), alpha ? *alpha : 1.0f);
		}
			break;
		case RM_PATH:
			LiPathMode(scene, ray, sample, alpha);
			break;
		default:
			BOOST_ASSERT(false);
	}

    return SWCSpectrum(-1.f);
}

SWCSpectrum ExPhotonIntegrator::LiDirectLigthtingMode(
        const int specularDepth,
        const Scene *scene,
        const RayDifferential &ray, const Sample *sample,
        float *alpha) const {
    // Compute reflected radiance with photon map
    SWCSpectrum L(0.0f);

	if (!indirectMap)
		return L;

    Intersection isect;
    if (scene->Intersect(ray, &isect)) {
		// Dade - collect samples
		float *sampleData = sample->sampler->GetLazyValues(const_cast<Sample *>(sample), sampleOffset, specularDepth);
		float *lightSample = &sampleData[0];
		float *bsdfSample = &sampleData[3];
		float *bsdfComponent = &sampleData[5];

		float *reflectionSample = &sampleData[6];
		float *reflectionComponent = &sampleData[8];
		float *transmissionSample = &sampleData[9];
		float *transmissionComponent = &sampleData[11];

		float *lightNum;
		if (lightStrategy == SAMPLE_ONE_UNIFORM)
			lightNum = &sampleData[2];
		else
			lightNum = NULL;

        if (alpha) *alpha = 1.0f;
        Vector wo = -ray.d;

        // Compute emitted light if ray hit an area light source
        L += isect.Le(wo);

        // Evaluate BSDF at hit point
        BSDF *bsdf = isect.GetBSDF(ray, fabsf(2.f *
				bsdfComponent[0]) - 1.0f);
        const Point &p = bsdf->dgShading.p;
        const Normal &n = bsdf->dgShading.nn;

		// Dade - a sanity check
		if (isnan(p.x) || isnan(p.y) || isnan(p.z)) {
			// Dade - something wrong here
			std::stringstream ss;
			ss << "Internal error in photonmap, received a NaN point in differential shading geometry: point = (" << p << ")";
			luxError(LUX_SYSTEM, LUX_ERROR, ss.str().c_str());
			return L;
		}

		if (debugEnableDirect) {
			// Apply direct lighting strategy
			switch (lightStrategy) {
				case SAMPLE_ALL_UNIFORM:
					L += UniformSampleAllLights(scene, p, n,
							wo, bsdf, sample,
							lightSample, bsdfSample, bsdfComponent);
					break;
				case SAMPLE_ONE_UNIFORM:
					L += UniformSampleOneLight(scene, p, n,
							wo, bsdf, sample,
							lightSample, lightNum, bsdfSample, bsdfComponent);
					break;
				default:
					break;
			}
		}

		if (debugEnableCaustic && (!causticMap->isEmpty())) {
			// Compute indirect lighting for photon map integrator
			L += causticMap->LPhoton(bsdf, isect, wo);
		}

		if (debugEnableIndirect) {
			if (finalGather)
				L += PhotonMapFinalGather(
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
						bsdf);
			else
				L += indirectMap->LPhoton(bsdf, isect, wo);
		}

        if (debugEnableSpecular && (specularDepth < maxDepth)) {
			float u1 = reflectionSample[0];
			float u2 = reflectionSample[1];
			float u3 = reflectionComponent[0];

            Vector wi;
			float pdf;
            // Trace rays for specular reflection and refraction
            SWCSpectrum f = bsdf->Sample_f(wo, &wi, u1, u2, u3,
                    &pdf, BxDFType(BSDF_REFLECTION | BSDF_SPECULAR | BSDF_GLOSSY));
            if ((!f.Black()) || (pdf > 0.0f)) {
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
                float dDNdx = Dot(dwodx, n) + Dot(wo, dndx);
                float dDNdy = Dot(dwody, n) + Dot(wo, dndy);
                rd.rx.d = wi -
                        dwodx + 2 * Vector(Dot(wo, n) * dndx +
                        dDNdx * n);
                rd.ry.d = wi -
                        dwody + 2 * Vector(Dot(wo, n) * dndy +
                        dDNdy * n);
                L += LiDirectLigthtingMode(specularDepth + 1, scene, rd, sample, alpha) *
					(f * (1.0f / pdf)) * AbsDot(wi, n);
            }

			u1 = transmissionSample[0];
			u2 = transmissionSample[1];
			u3 = transmissionComponent[0];

            f = bsdf->Sample_f(wo, &wi, u1, u2, u3,
                    &pdf, BxDFType(BSDF_TRANSMISSION | BSDF_SPECULAR | BSDF_GLOSSY));
            if ((!f.Black()) || (pdf > 0.0f)) {
                // Compute ray differential _rd_ for specular transmission
                RayDifferential rd(p, wi);
                rd.hasDifferentials = true;
                rd.rx.o = p + isect.dg.dpdx;
                rd.ry.o = p + isect.dg.dpdy;

                float eta = bsdf->eta;
                Vector w = -wo;
                if (Dot(wo, n) < 0) eta = 1.f / eta;

                Normal dndx = bsdf->dgShading.dndu * bsdf->dgShading.dudx + bsdf->dgShading.dndv * bsdf->dgShading.dvdx;
                Normal dndy = bsdf->dgShading.dndu * bsdf->dgShading.dudy + bsdf->dgShading.dndv * bsdf->dgShading.dvdy;

                Vector dwodx = -ray.rx.d - wo, dwody = -ray.ry.d - wo;
                float dDNdx = Dot(dwodx, n) + Dot(wo, dndx);
                float dDNdy = Dot(dwody, n) + Dot(wo, dndy);

                float mu = eta * Dot(w, n) - Dot(wi, n);
                float dmudx = (eta - (eta * eta * Dot(w, n)) / Dot(wi, n)) * dDNdx;
                float dmudy = (eta - (eta * eta * Dot(w, n)) / Dot(wi, n)) * dDNdy;

                rd.rx.d = wi + eta * dwodx - Vector(mu * dndx + dmudx * n);
                rd.ry.d = wi + eta * dwody - Vector(mu * dndy + dmudy * n);
                L += LiDirectLigthtingMode(specularDepth + 1, scene, rd, sample, alpha) *
					(f * (1.0f / pdf)) * AbsDot(wi, n);
            }
        }
    } else {
        // Handle ray with no intersection
        if (alpha) *alpha = 0.0f;
        for (u_int i = 0; i < scene->lights.size(); ++i)
            L += scene->lights[i]->Le(ray);
        if (alpha && !L.Black()) *alpha = 1.0f;
    }

    return L * scene->volumeIntegrator->Transmittance(scene, ray, sample, alpha) +
			scene->volumeIntegrator->Li(scene, ray, sample, alpha);
}

void ExPhotonIntegrator::LiPathMode(const Scene *scene,
		const RayDifferential &r, const Sample *sample,
		float *alpha) const
{
	SampleGuard guard(sample->sampler, sample);
	// Declare common path integration variables
	RayDifferential ray(r);
	SWCSpectrum pathThroughput(1.0f);
	XYZColor color;
	float V = .1f;
	bool specularBounce = true, specular = true;
	if (alpha) *alpha = 1.;
	for (int pathLength = 0; ; ++pathLength) {
		// Find next vertex of path
		Intersection isect;
		if (!scene->Intersect(ray, &isect)) {
			if (pathLength == 0) {
				// Dade - now I know ray.maxt and I can call volumeIntegrator
				SWCSpectrum L = scene->volumeIntegrator->Li(scene, ray, sample, alpha);
				color = L.ToXYZ();
				if (color.y() > 0.f)
					sample->AddContribution(sample->imageX, sample->imageY,
						color, alpha ? *alpha : 1.f, V);
				pathThroughput = scene->volumeIntegrator->Transmittance(scene, ray, sample, alpha);
			}

			// Stop path sampling since no intersection was found
			// Possibly add emitted light
			// NOTE - Added by radiance - adds horizon in render & reflections
			if (specularBounce) {
				SWCSpectrum Le(0.f);
				for (u_int i = 0; i < scene->lights.size(); ++i)
					Le += scene->lights[i]->Le(ray);
				Le *= pathThroughput;
				color = Le.ToXYZ();
			}
			// Set alpha channel
			if (pathLength == 0 && alpha && !(color.y() > 0.f))
				*alpha = 0.;
			if (color.y() > 0.f)
				sample->AddContribution(sample->imageX, sample->imageY,
					color, alpha ? *alpha : 1.f, V);
			break;
		}
		if (pathLength == 0)
			r.maxt = ray.maxt;

		SWCSpectrum Lv(scene->volumeIntegrator->Li(scene, ray, sample, alpha));
		Lv *= pathThroughput;
		color = Lv.ToXYZ();
		if (color.y() > 0.f)
			sample->AddContribution(sample->imageX, sample->imageY,
				color, alpha ? *alpha : 1.f, V);
		pathThroughput *= scene->volumeIntegrator->Transmittance(scene, ray, sample, alpha);

		// Possibly add emitted light at path vertex
		Vector wo(-ray.d);
		if (specularBounce) {
			SWCSpectrum Le(isect.Le(wo));
			Le *= pathThroughput;
			color = Le.ToXYZ();
			if (color.y() > 0.f)
				sample->AddContribution(sample->imageX, sample->imageY,
					color, alpha ? *alpha : 1.f, V);
		}
		if (pathLength == maxDepth)
			break;

		// Dade - collect samples
		float *sampleData = sample->sampler->GetLazyValues(const_cast<Sample *>(sample), sampleOffset, pathLength);
		float *lightSample = &sampleData[0];
		float *bsdfSample = &sampleData[3];
		float *bsdfComponent = &sampleData[5];
		float *pathSample = &sampleData[6];
		float *pathComponent = &sampleData[8];
		float *indirectSample = &sampleData[9];
		float *indirectComponent = &sampleData[11];

		float *lightNum;
		if (lightStrategy == SAMPLE_ONE_UNIFORM) {
			lightNum = &sampleData[2];
		} else
			lightNum = NULL;

		float *rrSample;
		if (rrStrategy != RR_NONE)
			rrSample = &sampleData[12];
		else
			rrSample = NULL;
		
		// Evaluate BSDF at hit point
		BSDF *bsdf = isect.GetBSDF(ray, fabsf(2.f * bsdfComponent[0] - 1.f));
		// Sample illumination from lights to find path contribution
		const Point &p = bsdf->dgShading.p;
		const Normal &n = bsdf->dgShading.nn;

		// Dade - direct lighting
		if (debugEnableDirect) {
			SWCSpectrum Ll;
			switch (lightStrategy) {
				case SAMPLE_ALL_UNIFORM:
					Ll = UniformSampleAllLights(scene, p, n,
						wo, bsdf, sample,
						lightSample, bsdfSample, bsdfComponent);
					break;
				case SAMPLE_ONE_UNIFORM:
					Ll = UniformSampleOneLight(scene, p, n,
						wo, bsdf, sample,
						lightSample, lightNum, bsdfSample, bsdfComponent);
					break;
				default:
					Ll = 0.f;
			}
			Ll *= pathThroughput;
			color = Ll.ToXYZ();
			if (color.y() > 0.f)
				sample->AddContribution(sample->imageX, sample->imageY,
					color, alpha ? *alpha : 1.f, V);
		}

		// Dade - add caustic		
		if (debugEnableCaustic && (!causticMap->isEmpty())) {
			// Compute indirect lighting for photon map integrator
			SWCSpectrum Lc = causticMap->LPhoton(bsdf, isect, wo);
			Lc *= pathThroughput;
			color = Lc.ToXYZ();
			if (color.y() > 0.f)
				sample->AddContribution(sample->imageX, sample->imageY,
					color, alpha ? *alpha : 1.f, V);
		}

		// Dade - add indirect lighting
		if (debugEnableIndirect) {
			BxDFType nonSpecularGlossy = BxDFType(BSDF_REFLECTION |
					BSDF_TRANSMISSION | BSDF_DIFFUSE);
			if (bsdf->NumComponents(nonSpecularGlossy) > 0) {

				Vector wi;
				float u1 = indirectSample[0];
				float u2 = indirectSample[1];
				float u3 = indirectComponent[0];
				float pdf;
				SWCSpectrum fr = bsdf->Sample_f(wo, &wi, u1, u2, u3,
						&pdf, nonSpecularGlossy);
				if (!fr.Black() && (pdf != 0.f)) {
					RayDifferential bounceRay(p, wi);

					Intersection gatherIsect;
					if (scene->Intersect(bounceRay, &gatherIsect)) {
						// Compute exitant radiance using precomputed irradiance
						SWCSpectrum Lindir = 0.f;
						Normal nGather = gatherIsect.dg.nn;
						if (Dot(nGather, bounceRay.d) > 0) nGather = -nGather;
						NearPhotonProcess<RadiancePhoton> proc(gatherIsect.dg.p, nGather);
						float md2 = INFINITY;

						radianceMap->lookup(gatherIsect.dg.p, proc, md2);
						if (proc.photon) {
							Lindir = proc.photon->alpha;

							Lindir *= scene->Transmittance(bounceRay);
							SWCSpectrum Li = fr * Lindir * (AbsDot(wi, n) / pdf);

							Li *= pathThroughput;
							color = Li.ToXYZ();
							if (color.y() > 0.f)
								sample->AddContribution(sample->imageX, sample->imageY,
									color, alpha ? *alpha : 1.f, V);
						}
					}
				}
			}
		}

		BxDFType specularGlossy = BxDFType(BSDF_ALL & (~BSDF_DIFFUSE));
		if (bsdf->NumComponents(specularGlossy) <= 0)
			break;

		// Sample BSDF to get new path direction
		Vector wi;
		float pdf;
		BxDFType flags;
		SWCSpectrum f = bsdf->Sample_f(wo, &wi, pathSample[0], pathSample[1], pathComponent[0],
			&pdf, specularGlossy, &flags);
		if (pdf == .0f || f.Black())
			break;

		const float dp = AbsDot(wi, n) / pdf;

		// Possibly terminate the path
		if (pathLength > 3) {
			if (rrStrategy == RR_EFFICIENCY) { // use efficiency optimized RR
				const float q = min<float>(1.f, f.filter() * dp);
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

		specularBounce = (flags & BSDF_SPECULAR) != 0;
		specular = specular && specularBounce;
		pathThroughput *= f;
		pathThroughput *= dp;
		if (!specular)
			V += dp;

		ray = RayDifferential(p, wi);
	}
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

    int nCaustic = params.FindOneInt("causticphotons", 20000);
    int nIndirect = params.FindOneInt("indirectphotons", 200000);
	int maxDirect = params.FindOneInt("maxdirectphotons", 1000000);

	int nUsed = params.FindOneInt("nused", 50);
    int maxDepth = params.FindOneInt("maxdepth", 6);

	bool finalGather = params.FindOneBool("finalgather", true);
	// Dade - use half samples for sampling along the BSDF and the other
	// half to sample along photon incoming direction, this why I'm dividing
	// the toal by 2
    int gatherSamples = params.FindOneInt("finalgathersamples", 32) / 2;
	string smode =  params.FindOneString("renderingmode", "directlighting");

	RenderingMode renderingMode;
	if (smode == "directlighting") renderingMode = RM_DIRECTLIGHTING;
	else if (smode == "path") {
		renderingMode = RM_PATH;
		finalGather = true;
	} else {
		std::stringstream ss;
		ss<<"Strategy  '" << smode << "' for rendering mode unknown. Using \"directlighting\".";
		luxError(LUX_BADTOKEN,LUX_WARNING,ss.str().c_str());
		renderingMode = RM_DIRECTLIGHTING;
	}

	float maxDist = params.FindOneFloat("maxdist", 0.5f);
    float gatherAngle = params.FindOneFloat("gatherangle", 10.0f);

	PhotonMapRRStrategy rstrategy;
	string rst = params.FindOneString("gatherrrstrategy", "efficiency");
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
	float rrcontinueProb = params.FindOneFloat("gatherrrcontinueprob", 0.65f);

	string *mapsFileName = NULL;
	string sfn = params.FindOneString("mapsfile", "");
	if (sfn != "")
		mapsFileName = new string(sfn);

	bool debugEnableDirect = params.FindOneBool("dbg_enabledirect", true);
	bool debugEnableCaustic = params.FindOneBool("dbg_enablecaustic", true);
	bool debugEnableIndirect = params.FindOneBool("dbg_enableindirect", true);
	bool debugEnableSpecular = params.FindOneBool("dbg_enablespecular", true);

    return new ExPhotonIntegrator(renderingMode, estrategy, nCaustic, nIndirect, maxDirect,
            nUsed, maxDepth, maxDist, finalGather, gatherSamples, gatherAngle,
			rstrategy, rrcontinueProb,
			mapsFileName,
			debugEnableDirect, debugEnableCaustic, debugEnableIndirect, debugEnableSpecular);
}
