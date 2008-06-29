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

#include "importancepath.h"
#include "bxdf.h"
#include "light.h"
#include "paramset.h"
#include "spectrumwavelengths.h"

#include <boost/thread/xtime.hpp>

using namespace lux;

// thread specific wavelengths
extern boost::thread_specific_ptr<SpectrumWavelengths> thread_wavelengths;

// Lux (copy) constructor
ImportancePathIntegrator* ImportancePathIntegrator::clone() const {
    return new ImportancePathIntegrator(*this);
}

// ImportancePathIntegrator Method Definitions
IPhotonProcess::IPhotonProcess(u_int mp, const Point &P)
: p(P) {
    photons = 0;
    nLookup = mp;
    foundPhotons = 0;
}

void IPhotonProcess::operator()(const IPhoton &photon,
        float distSquared, float &maxDistSquared) const {
    // Do usual photon heap management
    if (foundPhotons < nLookup) {
        // Add photon to unordered array of photons
        photons[foundPhotons++] = IClosePhoton(&photon, distSquared);
        if (foundPhotons == nLookup) {
            std::make_heap(&photons[0], &photons[nLookup]);
            maxDistSquared = photons[0].distanceSquared;
        }
    } else {
        // Remove most distant photon from heap and add new photon
        std::pop_heap(&photons[0], &photons[nLookup]);
        photons[nLookup - 1] = IClosePhoton(&photon, distSquared);
        std::push_heap(&photons[0], &photons[nLookup]);
        maxDistSquared = photons[0].distanceSquared;
    }
}

ImportancePathIntegrator::ImportancePathIntegrator(
		LightStrategy st,
		int nphoton,
		float mdist,
		int impdepth,
		int impwidth,
		int mdepth,
		RRStrategy rrstrat,
		float contProb) {
	lightStrategy = st;

    nIndirectPhotons = nphoton;
	maxDistSquared = mdist * mdist;
	importanceMap = NULL;

	impDepth = impdepth;
	impTableWidth = impwidth;
	impTableHeight = impTableWidth / 2;
	impTableSize = impTableWidth * impTableHeight;

    maxDepth = mdepth;
	
	rrStrategy = rrstrat;
	continueProbability = contProb;
}

ImportancePathIntegrator::~ImportancePathIntegrator() {
    delete importanceMap;
}

void ImportancePathIntegrator::RequestSamples(Sample *sample,
        const Scene *scene) {
	if (lightStrategy == SAMPLE_AUTOMATIC) {
		if (scene->lights.size() > 5)
			lightStrategy = SAMPLE_ONE_UNIFORM;
		else
			lightStrategy = SAMPLE_ALL_UNIFORM;
	}

	// Dade - allocate and request samples
 	vector<u_int> structure;

	structure.push_back(2);	// light position sample
	structure.push_back(2);	// bsdf direction sample for light
	structure.push_back(1);	// bsdf component sample for light
	structure.push_back(2);	// bsdf direction sample for path
	structure.push_back(1);	// bsdf component sample for path
	structure.push_back(1);	// russian roulette
	structure.push_back(1);	// importance

	if (lightStrategy == SAMPLE_ONE_UNIFORM)
		structure.push_back(1);	// light number sample

	sampleOffset = sample->AddxD(structure, maxDepth + 1);
}

void ImportancePathIntegrator::Preprocess(const Scene *scene) {
    if (scene->lights.size() == 0) return;

    // Dade - shoot photons
    std::stringstream ss;
    ss << "Shooting photons: " << nIndirectPhotons;
    luxError(LUX_NOERROR, LUX_INFO, ss.str().c_str());

    vector<IPhoton> importancePhotons;
    importancePhotons.reserve(nIndirectPhotons);

	// Dade - initialize SpectrumWavelengths
    SpectrumWavelengths *thr_wl = thread_wavelengths.get();

    bool importanceDone = (nIndirectPhotons == 0);

	// Compute light power CDF for photon shooting
	int nLights = int(scene->lights.size());
	float *lightPower = (float *)alloca(nLights * sizeof(float));
	float *lightCDF = (float *)alloca((nLights+1) * sizeof(float));

	// Dade - avarge the light power
	const int spectrumSamples = 128;
	for (int i = 0; i < nLights; ++i)
		lightPower[i] +=0.0f;
	for (int j = 0; j < spectrumSamples; j++) {
		thr_wl->Sample((float)RadicalInverse(j, 2),
			(float)RadicalInverse(j, 3));

		for (int i = 0; i < nLights; ++i)
			lightPower[i] += scene->lights[i]->Power(scene).y();
	}
	for (int i = 0; i < nLights; ++i)
		lightPower[i] *= 1.0f / spectrumSamples;

	float totalPower;
	ComputeStep1dCDF(lightPower, nLights, &totalPower, lightCDF);

	boost::xtime lastUpdateTime;
	boost::xtime_get(&lastUpdateTime, boost::TIME_UTC);
	int nshot = 0;
    while (!importanceDone) {
		// Dade - print some progress information
		boost::xtime currentTime;
		boost::xtime_get(&currentTime, boost::TIME_UTC);
		if (currentTime.sec - lastUpdateTime.sec > 5) {
			ss.str("");
			ss << "Indirect photonmap size: " << importancePhotons.size();
			if (nIndirectPhotons > 0)
				ss << " (" << (100 * importancePhotons.size() / nIndirectPhotons) << "%)";
			else
				ss << " (100%)";
			luxError(LUX_NOERROR, LUX_INFO, ss.str().c_str());

			lastUpdateTime = currentTime;
		}
		
		++nshot;

        // Give up if we're not storing enough photons
		if ((nshot > 500000) && (unsuccessful(nIndirectPhotons, importancePhotons.size(), nshot))) {
			luxError(LUX_CONSISTENCY, LUX_ERROR, "Unable to store enough photons in the importance photonmap.  Giving up. I will unable to reder the image.");
			return;
        }

        // Trace a photon path and store contribution
        // Choose 4D sample values for photon
        float u[4];
        u[0] = RadicalInverse(nshot, 2);
        u[1] = RadicalInverse(nshot, 3);
        u[2] = RadicalInverse(nshot, 5);
        u[3] = RadicalInverse(nshot, 7);

        // Dade - for SpectrumWavelengths
        thr_wl->Sample(RadicalInverse(nshot, 23),
				RadicalInverse(nshot, 29));

        // Choose light to shoot photon from
		float lightPdf;
		float uln = RadicalInverse(nshot, 11);
		int lightNum = Floor2Int(SampleStep1d(lightPower, lightCDF,
				totalPower, nLights, uln, &lightPdf) * nLights);
		lightNum = min(lightNum, nLights - 1);
		const Light *light = scene->lights[lightNum];

        // Generate _photonRay_ from light source and initialize _alpha_
        RayDifferential photonRay;
        float pdf;
        SWCSpectrum alpha = light->Sample_L(scene, u[0], u[1], u[2], u[3],
                &photonRay, &pdf);
        if (pdf == 0.f || alpha.Black()) continue;
        alpha /= pdf * lightPdf;

        if (!alpha.Black()) {
            // Follow photon path through scene and record intersections
            Intersection photonIsect;
            int nIntersections = 0;
            while (scene->Intersect(photonRay, &photonIsect)) {
                ++nIntersections;

                // Handle photon/surface intersection
                alpha *= scene->Transmittance(photonRay);
                Vector wo = -photonRay.d;

				// Process importance lighting photon intersection
				if (!importanceDone && (nIntersections > 1)) {
					IPhoton photon(photonIsect.dg.p, FromXYZ(alpha.ToXYZ()), wo);
					importancePhotons.push_back(photon);

					if (importancePhotons.size() == nIndirectPhotons) {
						importanceDone = true;
						nImportancePaths = nshot;
						importanceMap = new KdTree<IPhoton, IPhotonProcess >(importancePhotons);
					}
				}

                // Sample new photon ray direction
                Vector wi;
                float pdf;
                BxDFType flags;
                // Get random numbers for sampling outgoing photon direction
                float u1, u2, u3;
                if (nIntersections == 1) {
                    u1 = RadicalInverse(nshot, 13);
                    u2 = RadicalInverse(nshot, 17);
                    u3 = RadicalInverse(nshot, 19);
                } else {
                    u1 = lux::random::floatValue();
                    u2 = lux::random::floatValue();
                    u3 = lux::random::floatValue();
                }

				BSDF *photonBSDF = photonIsect.GetBSDF(photonRay, lux::random::floatValue());

                // Compute new photon weight and possibly terminate with RR
                SWCSpectrum fr = photonBSDF->Sample_f(wo, &wi, u1, u2, u3,
                        &pdf, BSDF_ALL, &flags);
                if (fr.Black() || pdf == 0.f)
                    break;
                SWCSpectrum anew = alpha * fr *
                        AbsDot(wi, photonBSDF->dgShading.nn) / pdf;
                float continueProb = min<float>(1.0f, anew.filter() / alpha.filter());
                if (lux::random::floatValue() > continueProb || nIntersections > 10)
                    break;
				alpha = anew / continueProb;
                photonRay = RayDifferential(photonIsect.dg.p, wi);
            }
        }

        BSDF::FreeAll();
    }

	luxError(LUX_NOERROR, LUX_INFO, "Photon shooting done");
}

SWCSpectrum ImportancePathIntegrator::Li(const Scene *scene,
        const RayDifferential &ray, const Sample *sample,
        float *alpha) const {
    SampleGuard guard(sample->sampler, sample);

    SWCSpectrum L = LiInternal(0, scene, ray, sample, alpha);
	sample->AddContribution(sample->imageX, sample->imageY,
			L.ToXYZ(), alpha ? *alpha : 1.0f);

    return SWCSpectrum(-1.f);
}

SWCSpectrum ImportancePathIntegrator::LiInternal(
        const int rayDepth,
        const Scene *scene,
        const RayDifferential &ray, const Sample *sample,
        float *alpha) const {
	SWCSpectrum L(0.0f);

	if (!importanceMap)
		return L;

    Intersection isect;
    if (scene->Intersect(ray, &isect)) {
		// Dade - collect samples
		float *sampleData = sample->sampler->GetLazyValues(const_cast<Sample *>(sample), sampleOffset, rayDepth);
		float *lightSample = &sampleData[0];
		float *bsdfSample = &sampleData[2];
		float *bsdfComponent = &sampleData[4];
		float *bsdfPathSample = &sampleData[5];
		float *bsdfPathComponent = &sampleData[7];
		float *rrSample = &sampleData[8];
		float *importanceSample = &sampleData[9];

		float *lightNum;
		if (lightStrategy == SAMPLE_ONE_UNIFORM)
			lightNum = &sampleData[10];
		else
			lightNum = NULL;

        if (alpha && (rayDepth == 0)) *alpha = 1.0f;
        Vector wo = -ray.d;

        // Compute emitted light if ray hit an area light source
        L += isect.Le(wo);

        // Evaluate BSDF at hit point
        BSDF *bsdf = isect.GetBSDF(ray, fabsf(2.f *
				bsdfComponent[0]) - 1.0f);
        const Point &p = bsdf->dgShading.p;
        const Normal &n = bsdf->dgShading.nn;

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

		if (rayDepth < maxDepth) {
			bool usedImportanceMap;
			if (rayDepth < impDepth) {
				// TODO: use importance map only for diffuse/glossy BSDF

				// Dade - looks for near photons
				u_int nImportSamplePhotons = 50;
				IPhotonProcess proc(nImportSamplePhotons, p);
				proc.photons = (IClosePhoton *) alloca(
						nImportSamplePhotons * sizeof (IClosePhoton));
				float searchDist2 = maxDistSquared;

				int sanityCheckIndex = 0;
				while (proc.foundPhotons < nImportSamplePhotons) {
					float md2 = searchDist2;
					proc.foundPhotons = 0;
					importanceMap->Lookup(p, proc, md2);

					searchDist2 *= 2.0f;

					if(sanityCheckIndex++ > 32) {
						// Dade - something wrong here
						std::stringstream ss;
						ss << "Internal error in photonmap: point = (" <<
								p << ") searchDist2 = " << searchDist2;
						luxError(LUX_SYSTEM, LUX_ERROR, ss.str().c_str());
						proc.foundPhotons = 0;
						break;
					}
				}

				// Dade - check if we have found enough photons
				if (proc.foundPhotons >= nImportSamplePhotons) {
					// Dade - allocate importanceTable
					float *importanceTable = (float *) alloca(impTableSize * sizeof(float));
					for (int i = 0; i < impTableSize; i++)
						importanceTable[i] = 0.0f;

					// Dade - build the reference coordinate system around the normal
					Vector vx, vy;
					const Vector vz = Vector(n);
					CoordinateSystem(vz, &vx, &vy);

					for (u_int i = 0; i < nImportSamplePhotons; ++i) {
						const Vector& pwi = proc.photons[i].photon->wi;

						// Dade - compute the importance of the photon
						SWCSpectrum fr = bsdf->f(wo, pwi);
						SWCSpectrum importanceSpectrum = fr * proc.photons[i].photon->alpha;
						float importance = importanceSpectrum.y();

						// Dade - tansform the photon incoming direction to the
						// reference coordinate system
						const Vector refpwi = Vector(
								Dot(pwi, vx),
								Dot(pwi, vy),
								Dot(pwi, vz));
						float phi = SphericalPhi(refpwi);
						float theta = SphericalTheta(refpwi);

						// Dade - look for the position in the importance table
						int indexPhi = min<float>(
								Floor2Int((phi + M_PI) * INV_TWOPI * impTableWidth),
								impTableWidth - 1);
						int indexTheta = min<float>(
								Floor2Int(theta * (2.0f / M_PI) * impTableHeight),
								impTableHeight - 1);

						// Dade - add importance to the table
						importanceTable[indexTheta * impTableWidth + indexPhi] +=
								importance;
					}

					// Dade - look for the max importance value
					float maxImportance = importanceTable[0];
					for (int i = 1; i < impTableSize; i++) {
						if (importanceTable[i] > maxImportance)
							maxImportance = importanceTable[i];
					}

					// Dade - in order to avoid bias add a 5% of max value to
					// all cells
					const float unbiasFactor = maxImportance * 0.05f;
					float totImportance = 0.0f;
					for (int i = 0; i < impTableSize; i++) {
						importanceTable[i] += unbiasFactor;
						totImportance += importanceTable[i];
					}

					// Dade - normalize the importance table
					const float invTotImportance = 1.0f / totImportance;
					for (int i = 0; i < impTableSize; i++)
						importanceTable[i] *= invTotImportance;

					// Dade - debug code
					/*printf("----------------------------------------------------\n");
					for (int y = 0; y < impTableHeight; y++) {
						for (int x = 0; x < impTableWidth; x++) {
							printf("%f ", importanceTable[x + y * impTableWidth]);
						}
						printf("\n");
					}*/

					// Dade - calculate the CDF
					float totalPower;
					float *importanceCDF = (float *) alloca((impTableSize + 1) * sizeof(float));
					ComputeStep1dCDF(importanceTable, impTableSize, &totalPower, importanceCDF);

					// Dade - select the cell for sampling
					float importancePdf;
					int cellNum = Floor2Int(SampleStep1d(importanceTable, importanceCDF,
							totalPower, impTableSize, importanceSample[0], &importancePdf) * impTableSize);

					// TODO: use a lookup table
					float samplePhi = (cellNum % impTableWidth + lux::random::floatValue()) /
						float(impTableWidth) * (2.0f * M_PI) - M_PI;
					float sampleTheta = (cellNum / impTableWidth + lux::random::floatValue()) /
						float(impTableHeight) * (M_PI / 2.0f) ;
					Vector sampleWi = SphericalDirection(sinf(sampleTheta),
							cosf(sampleTheta), samplePhi,
							vx, vy, vz);

					// Dade - check if it is worth sampling
					SWCSpectrum bsdfValue = bsdf->f(wo, sampleWi);
					if (!bsdfValue.Black()) {
						RayDifferential bounceRay = RayDifferential(p, sampleWi);
						L += bsdfValue *
								LiInternal(rayDepth + 1, scene, bounceRay, sample, alpha) *
								AbsDot(sampleWi, n) / importancePdf;
					}
					
					usedImportanceMap = true;
				} else
					usedImportanceMap = false;
			} else
				usedImportanceMap = false;

			if (!usedImportanceMap) {
				// Dade - use standard BSDf based sampling

				Vector wi;
				float pdf;
				BxDFType flags;
				SWCSpectrum bsdfBSDF = bsdf->Sample_f(
						wo, &wi,
						bsdfPathSample[0], bsdfPathSample[1], bsdfPathComponent[0],
						&pdf, BSDF_ALL, &flags);
				if ((pdf != .0f) && (!bsdfBSDF.Black())) {
					bool isBSDFSampleValid = true;
					if (rayDepth > 3) {
						// Dade - russian roulette
						if (rrStrategy == RR_EFFICIENCY) { // use efficiency optimized RR
							const float dp = AbsDot(wi, n) / pdf;
							const float q = min<float>(1.0f, bsdfBSDF.filter() * dp);
							if (q < rrSample[0])
								isBSDFSampleValid = false;
							else {
								// increase contribution
								bsdfBSDF /= q;
							}
						} else if (rrStrategy == RR_PROBABILITY) { // use normal/probability RR
							if (continueProbability < rrSample[0])
								isBSDFSampleValid = false;
							else {
								// increase path contribution
								bsdfBSDF /= continueProbability;
							}
						}
					}

					if (isBSDFSampleValid) {
						RayDifferential bounceRay = RayDifferential(p, wi);
						L += bsdfBSDF *
								LiInternal(rayDepth + 1, scene, bounceRay, sample, alpha) *
								AbsDot(wi, n) / pdf;
					}
				}
			}
		}
    } else {
        // Handle ray with no intersection
        if (alpha && (rayDepth == 0)) *alpha = 0.0f;
        for (u_int i = 0; i < scene->lights.size(); ++i)
            L += scene->lights[i]->Le(ray);
        if (alpha && (rayDepth == 0) && !L.Black()) *alpha = 1.0f;
	}

	// Dade - I do volume rendering only for eye rays
	if (rayDepth == 0) {
		SWCSpectrum pathThroughput(scene->volumeIntegrator->Transmittance(scene, ray, sample, alpha));
		SWCSpectrum pathEmit(scene->volumeIntegrator->Li(scene, ray, sample, alpha));

		return L * pathThroughput + pathEmit;
	} else
		return L;
}

SurfaceIntegrator* ImportancePathIntegrator::CreateSurfaceIntegrator(const ParamSet &params) {
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

    int nIndirect = params.FindOneInt("importancephotons", 200000);
	float maxDist = params.FindOneFloat("maxdist", 0.5f);

	int impDepth = params.FindOneInt("impdepth", 3);
	int impWidth = params.FindOneInt("impwidth", 8);
	if (impWidth % 2 !=0) {
		impWidth = (impWidth / 2 + 1) * 2;
		std::stringstream ss;
		ss<<"Importance map width must be a multiple of 2, using'"<<
				impWidth <<"' width.";
		luxError(LUX_BADTOKEN,LUX_WARNING,ss.str().c_str());
	}
    int maxDepth = params.FindOneInt("maxdepth", 10);

	RRStrategy rstrategy;
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
	float rrcontinueProb = params.FindOneFloat("gatherrrcontinueprob", 0.65f);

    return new ImportancePathIntegrator(
			estrategy,
			nIndirect,
			maxDist,
			impDepth,
			impWidth,
			maxDepth,
			rstrategy,
			rrcontinueProb);
}
