/***************************************************************************
 *   Copyright (C) 1998-2008 by authors (see AUTHORS.txt )                 *
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
 *   Lux Renderer website : http://www.luxrender.net                       *
 ***************************************************************************/

#include "photonmap.h"
#include "light.h"
#include "mc.h"
#include "spectrumwavelengths.h"
#include "error.h"
#include "osfunc.h"
#include "mc.h"

#include <fstream>
#include <boost/thread/xtime.hpp>

using namespace lux;

namespace lux
{

void LightPhoton::save(bool isLittleEndian, std::basic_ostream<char> &stream) {
	// Point p
	for (int i = 0; i < 3; i++)
		osWriteLittleEndianFloat(isLittleEndian, stream, p[i]);

	// SWCSpectrum alpha
	for (int i = 0; i < 3; i++)
		osWriteLittleEndianFloat(isLittleEndian, stream, alpha.c[i]);

	// Vector wi
	for (int i = 0; i < 3; i++)
		osWriteLittleEndianFloat(isLittleEndian, stream, wi[i]);
}

void RadiancePhoton::save(bool isLittleEndian, std::basic_ostream<char> &stream) {
	// Point p
	for (int i = 0; i < 3; i++)
		osWriteLittleEndianFloat(isLittleEndian, stream, p[i]);

	// SWCSpectrum alpha
	for (int i = 0; i < 3; i++)
		osWriteLittleEndianFloat(isLittleEndian, stream, alpha.c[i]);

	// Normal n
	for (int i = 0; i < 3; i++)
		osWriteLittleEndianFloat(isLittleEndian, stream, n[i]);
}

SWCSpectrum RadiancePhotonMap::LPhoton(
	const TsPack *tspack, 
	const Intersection& isect, 
	const Vector& wo, 
	const BxDFType bxdfType) const 
{
	SWCSpectrum L(0.0f);
	if (!photonmap)
		return L;

	Normal ng = isect.dg.nn;
	if(Dot(wo, ng) < 0.f) ng = -ng;

	const Point& p = isect.dg.p;

	if( ( bxdfType & BSDF_REFLECTION ) != 0 ) {
		// Add reflected radiance
		NearPhotonProcess<RadiancePhoton> procRefl(p, ng);
		float md2Refl = maxDistSquared;
		lookup(p, procRefl, md2Refl);
		if (procRefl.photon) {
			L += procRefl.photon->GetSWCSpectrum( tspack );
		}
	}

	if( ( bxdfType & BSDF_TRANSMISSION ) != 0 ) {
		// Add transmitted radiance
		NearPhotonProcess<RadiancePhoton> procTransm(p, -ng);
		float md2Transm = maxDistSquared;
		lookup(p, procTransm, md2Transm);
		if (procTransm.photon) {
			L += procTransm.photon->GetSWCSpectrum( tspack );
		}
	}

	return L;
}

SWCSpectrum LightPhotonMap::EPhoton(
	const TsPack *tspack, 
	const Point &p, 
	const Normal &n) const 
{
    SWCSpectrum E(0.0f);
	if ((nPaths <= 0) || (!photonmap))
		return E;

    // Lookup nearby photons at irradiance computation point
    NearSetPhotonProcess<LightPhoton> proc(nLookup, p);
    proc.photons = 
		(ClosePhoton<LightPhoton> *) alloca(nLookup * sizeof (ClosePhoton<LightPhoton>));
    float md2 = maxDistSquared;
    lookup(p, proc, md2);

    // Accumulate irradiance value from nearby photons
    ClosePhoton<LightPhoton> *photons = proc.photons;
	for (u_int i = 0; i < proc.foundPhotons; ++i) {
		if (Dot(n, photons[i].photon->wi) > 0.f)
            E += photons[i].photon->GetSWCSpectrum( tspack );
	}

    return E / (float(nPaths) * md2 * M_PI);
}

SWCSpectrum LightPhotonMap::LPhoton(
	const TsPack* tspack,
	const BSDF *bsdf,
	const Intersection &isect,
	const Vector &wo,
	const BxDFType bxdfType) const 
{
    SWCSpectrum L(0.0f);
	if ((nPaths <= 0) || (!photonmap))
		return L;

    if (bsdf->NumComponents(bxdfType) == 0)
        return L;

	// Initialize _PhotonProcess_ object, _proc_, for photon map lookups
    NearSetPhotonProcess<LightPhoton> proc(nLookup, isect.dg.p);
    proc.photons = 
		(ClosePhoton<LightPhoton> *) alloca(nLookup * sizeof (ClosePhoton<LightPhoton>));
    // Do photon map lookup
	float md2 = maxDistSquared;
    lookup(isect.dg.p, proc, md2);
    // Accumulate light from nearby photons
    // Estimate reflected light from photons
    const ClosePhoton<LightPhoton> *photons = proc.photons;
    const u_int nFound = proc.foundPhotons;
    const Normal Nf = Dot(wo, isect.dg.nn) < 0 ? -isect.dg.nn : isect.dg.nn;
	const float distSquared = md2;

    // Compute exitant radiance from photons for a surface
    for (u_int i = 0; i < nFound; ++i) {
		const LightPhoton *p = photons[i].photon;
        BxDFType flag = 
			BxDFType( bxdfType & 
			          (Dot(Nf, p->wi) > 0.f ? BSDF_ALL_REFLECTION : BSDF_ALL_TRANSMISSION) );
		float k = Ekernel(p, isect.dg.p, distSquared);
		const SWCSpectrum& alpha = p->GetSWCSpectrum( tspack );

        L += (k / nPaths) * bsdf->f(tspack, wo, p->wi, flag) * alpha;
    }

    return L;
}

SWCSpectrum LightPhotonMap::LPhotonDiffuseApprox(
	const TsPack* tspack,
	const BSDF *bsdf,
	const Intersection &isect,
	const Vector &wo,
	const BxDFType bxdfType) const 
{
    SWCSpectrum L(0.0f);
	if ((nPaths <= 0) || (!photonmap))
		return L;

    if (bsdf->NumComponents(bxdfType) == 0)
        return L;

	// Initialize _PhotonProcess_ object, _proc_, for photon map lookups
    NearSetPhotonProcess<LightPhoton> proc(nLookup, isect.dg.p);
    proc.photons = 
		(ClosePhoton<LightPhoton> *) alloca(nLookup * sizeof (ClosePhoton<LightPhoton>));
    // Do photon map lookup
	float md2 = maxDistSquared;
    lookup(isect.dg.p, proc, md2);
    // Accumulate light from nearby photons
    // Estimate reflected light from photons
    const ClosePhoton<LightPhoton> *photons = proc.photons;
    const u_int nFound = proc.foundPhotons;
    const Normal Nf = Dot(wo, isect.dg.nn) < 0 ? -isect.dg.nn : isect.dg.nn;
	const float distSquared = md2;

    // Compute exitant radiance from photons, estimate may be not-so-good for non-diffuse surface
    SWCSpectrum Lr(0.), Lt(0.);

	for (u_int i = 0; i < nFound; ++i) {
		const LightPhoton *p = photons[i].photon;
		const SWCSpectrum& alpha = p->GetSWCSpectrum( tspack );

        if (Dot(Nf, photons[i].photon->wi) > 0.f) {
            float k = Ekernel(p, isect.dg.p, distSquared);
            Lr += (k / nPaths) * alpha;
        } else {
            float k = Ekernel(p, isect.dg.p, distSquared);
            Lt += (k / nPaths) * alpha;
        }
    }

	if( (bxdfType & BSDF_REFLECTION) != 0 )
		L += Lr * bsdf->rho(tspack, wo, BxDFType(bxdfType & BSDF_ALL_REFLECTION)) * INV_PI;
	if( (bxdfType & BSDF_TRANSMISSION) != 0 )
		L += Lt * bsdf->rho(tspack, wo, BxDFType(bxdfType & BSDF_ALL_TRANSMISSION)) * INV_PI;

    return L;
}

SWCSpectrum LightPhotonMap::LDiffusePhoton(
	const TsPack* tspack,
	const BSDF *bsdf,
	const Intersection &isect,
	const Vector &wo) const 
{
    SWCSpectrum L(0.0f);
	if ((nPaths <= 0) || (!photonmap))
		return L;

	const BxDFType diffuseType = BxDFType(BSDF_DIFFUSE | BSDF_REFLECTION | BSDF_TRANSMISSION);

    if (bsdf->NumComponents(diffuseType) == 0)
        return L;

	// Initialize _PhotonProcess_ object, _proc_, for photon map lookups
    NearSetPhotonProcess<LightPhoton> proc(nLookup, isect.dg.p);
    proc.photons = 
		(ClosePhoton<LightPhoton> *) alloca(nLookup * sizeof (ClosePhoton<LightPhoton>));
    // Do photon map lookup
	float md2 = maxDistSquared;
    lookup(isect.dg.p, proc, md2);
    // Accumulate light from nearby photons
    // Estimate reflected light from photons
    const ClosePhoton<LightPhoton> *photons = proc.photons;
    const u_int nFound = proc.foundPhotons;
	const Normal Nf = Dot(wo, isect.dg.nn) < 0 ? -isect.dg.nn : isect.dg.nn;
	const float distSquared = md2;

    // Compute exitant radiance from photons
    SWCSpectrum Lr(0.), Lt(0.);

    for (u_int i = 0; i < nFound; ++i) {
		const LightPhoton *p = photons[i].photon;
		const SWCSpectrum& alpha = p->GetSWCSpectrum( tspack );

        if (Dot(Nf, photons[i].photon->wi) > 0.f) {
            float k = Ekernel(p, isect.dg.p, distSquared);
            Lr += (k / nPaths) * alpha;
        } else {
            float k = Ekernel(p, isect.dg.p, distSquared);
            Lt += (k / nPaths) * alpha;
        }
    }

	L = Lr * bsdf->rho(tspack, wo, BxDFType(BSDF_DIFFUSE | BSDF_REFLECTION)) * INV_PI +
		Lt * bsdf->rho(tspack, wo, BxDFType(BSDF_DIFFUSE | BSDF_TRANSMISSION)) * INV_PI;

    return L;
}

static bool unsuccessful(int needed, int found, int shot) {
	return (found < needed && (found == 0 || found < shot / 1024));
}

void PhotonMapPreprocess(
	const TsPack *tspack, 
	const Scene *scene, 
	const string *mapFileName,
	const BxDFType photonBxdfType,
	const BxDFType radianceBxdfType,
	u_int nDirectPhotons,
	u_int nRadiancePhotons, RadiancePhotonMap *radianceMap,
	u_int nIndirectPhotons, LightPhotonMap *indirectMap,
	u_int nCausticPhotons, LightPhotonMap *causticMap) 
{
	if (scene->lights.size() == 0) return;

	std::stringstream ss;

	// Dade - read the photon maps from file if required
	bool mapsFileExist = false;
	if (mapFileName) {
		// Dade - check if the maps file exists
		std::ifstream ifs(mapFileName->c_str(), std::ios_base::in | std::ios_base::binary);

        if(ifs.good()) {
			mapsFileExist = true;

			ss.str("");
			ss << "Reading photon maps file: " << (*mapFileName);
			luxError(LUX_NOERROR, LUX_INFO, ss.str().c_str());

			// Dade - read the maps from the file
			luxError(LUX_NOERROR, LUX_INFO, "Reading radiance photon map...");
			RadiancePhotonMap::load(ifs, radianceMap);
			ss.str("");
			ss << "Read " << radianceMap->getPhotonCount() << " radiance photons";
			luxError(LUX_NOERROR, LUX_INFO, ss.str().c_str());

			luxError(LUX_NOERROR, LUX_INFO, "Reading indirect photon map...");
			LightPhotonMap::load(ifs, indirectMap);
			ss.str("");
			ss << "Read " << indirectMap->getPhotonCount() << " light photons";
			luxError(LUX_NOERROR, LUX_INFO, ss.str().c_str());

			luxError(LUX_NOERROR, LUX_INFO, "Reading caustic photon map...");
			LightPhotonMap::load(ifs, causticMap);
			ss.str("");
			ss << "Read " << causticMap->getPhotonCount() << " light photons";
			luxError(LUX_NOERROR, LUX_INFO, ss.str().c_str());

			ifs.close();

			return;
		} else {
			luxError(LUX_NOERROR, LUX_INFO, "Photon maps file doesn't exist yet");
			ifs.close();
		}
	}

	// Dade - check if have to build the radiancem map
	bool finalGather = (nRadiancePhotons > 0);

    // Dade - shoot photons
	ss << "Shooting photons (target: " << (nCausticPhotons + nIndirectPhotons) << ")...";
    luxError(LUX_NOERROR, LUX_INFO, ss.str().c_str());

	vector<LightPhoton> directPhotons;
	directPhotons.reserve(nDirectPhotons);
	bool directDone = (nDirectPhotons == 0);

	vector<LightPhoton> causticPhotons;
	causticPhotons.reserve(nCausticPhotons);
	bool causticDone = (nCausticPhotons == 0);

    vector<LightPhoton> indirectPhotons;
	indirectPhotons.reserve(nIndirectPhotons);
    bool indirectDone = (nIndirectPhotons == 0);

	vector<RadiancePhoton> radiancePhotons;
	radiancePhotons.reserve(nRadiancePhotons);
	bool radianceDone = (nRadiancePhotons == 0);

	// Dade - initialize SpectrumWavelengths
	SpectrumWavelengths *thr_wl = tspack->swl;
	thr_wl->Sample(0.5f, 0.5f);

	// Compute light power CDF for photon shooting
	int nLights = int(scene->lights.size());
	float *lightPower = (float *)alloca(nLights * sizeof(float));
	float *lightCDF = (float *)alloca((nLights+1) * sizeof(float));

	// Dade - avarge the light power
	const int spectrumSamples = 128;
	for (int i = 0; i < nLights; ++i)
		lightPower[i] = 0.0f;
	for (int j = 0; j < spectrumSamples; j++) {
		thr_wl->Sample((float)RadicalInverse(j, 2),
			           (float)RadicalInverse(j, 3));

		for (int i = 0; i < nLights; ++i)
			lightPower[i] += scene->lights[i]->Power(tspack, scene).y( tspack );
	}

	// Dade - I'm setting a minimum value in order to not have numerical
	// problems when lights have a huge power range (i.e. sun and sky)
	float maxLightPower = 0.0;
	for (int i = 0; i < nLights; ++i) {
		lightPower[i] *= 1.0f / spectrumSamples;
		if (lightPower[i] > maxLightPower)
			maxLightPower = lightPower[i];
	}
	// Dade - the most powerful light can be only 10 times more "important" than
	// other light sources
	float lightPowerClipValue = maxLightPower * 0.1f;
	for (int i = 0; i < nLights; ++i)
		if (lightPower[i] < lightPowerClipValue)
			lightPower[i] = lightPowerClipValue;

	float totalPower;
	ComputeStep1dCDF(lightPower, nLights, &totalPower, lightCDF);

    // Declare radiance photon reflectance arrays
    vector<SWCSpectrum> rpReflectances;
	rpReflectances.reserve(nRadiancePhotons);
	vector<SWCSpectrum> rpTransmittances;
	rpTransmittances.reserve(nRadiancePhotons);

	boost::xtime photonShootingStartTime;
	boost::xtime lastUpdateTime;
	boost::xtime_get(&photonShootingStartTime, boost::TIME_UTC);
	boost::xtime_get(&lastUpdateTime, boost::TIME_UTC);
	int nshot = 0;
    while (!radianceDone || !directDone || !causticDone || !indirectDone) {
		// Dade - print some progress information
		boost::xtime currentTime;
		boost::xtime_get(&currentTime, boost::TIME_UTC);
		if (currentTime.sec - lastUpdateTime.sec > 5) {
			ss.str("");
			ss << "Photon shooting progress: Direct[" << directPhotons.size();
			if (nDirectPhotons > 0)
				ss << " (" << (100 * directPhotons.size() / nDirectPhotons) << "% limit)";
			else
				ss << " (100% limit)";
			ss << "] Caustic[" << causticPhotons.size();
			if (nCausticPhotons > 0)
				ss << " (" << (100 * causticPhotons.size() / nCausticPhotons) << "%)";
			else
				ss << " (100%)";
			ss << "] Indirect[" << indirectPhotons.size();
			if (nIndirectPhotons > 0)
				ss << " (" << (100 * indirectPhotons.size() / nIndirectPhotons) << "%)";
			else
				ss << " (100%)";
			ss << "] Radiance[" << radiancePhotons.size();
			if (nRadiancePhotons > 0)
				ss << " (" << (100 * radiancePhotons.size() / nRadiancePhotons) << "% limit)";
			else
				ss << " (100% limit)";
			ss << "]";
			luxError(LUX_NOERROR, LUX_INFO, ss.str().c_str());

			lastUpdateTime = currentTime;
		}
		
		++nshot;

        // Give up if we're not storing enough photons
		if (nshot > 500000) {
			if (indirectDone && unsuccessful(nCausticPhotons, causticPhotons.size(), nshot)) {
				// Dade - disable castic photon map: we ar eunable to store
				// enough photons
				luxError(LUX_CONSISTENCY, LUX_WARNING, "Unable to store enough photons in the caustic photonmap. Giving up and disabling the map.");

				causticPhotons.clear();
				causticDone = true;
				nCausticPhotons = 0;
			}

			if(unsuccessful(nIndirectPhotons, indirectPhotons.size(), nshot)) {
				luxError(LUX_CONSISTENCY, LUX_ERROR, "Unable to store enough photons in the indirect photonmap. Unable to render the image.");
				return;
			}
        }

        // Trace a photon path and store contribution
        // Choose 4D sample values for photon
        float u[4];
        u[0] = RadicalInverse(nshot, 2);
        u[1] = RadicalInverse(nshot, 3);
        u[2] = RadicalInverse(nshot, 5);
        u[3] = RadicalInverse(nshot, 7);

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
        SWCSpectrum alpha = light->Sample_L(tspack, scene, u[0], u[1], u[2], u[3],
                &photonRay, &pdf);
        if (pdf == 0.f || alpha.Black()) continue;
        alpha /= pdf * lightPdf;

        if (!alpha.Black()) {
            // Follow photon path through scene and record intersections
            bool specularPath = false;
            Intersection photonIsect;
            int nIntersections = 0;
            while (scene->Intersect(photonRay, &photonIsect)) {
                ++nIntersections;

                // Handle photon/surface intersection
				//Lotus - disabled due to lack of non-optional sample
                //scene->Transmittance(tspack, photonRay, NULL, alpha);
                Vector wo = -photonRay.d;

				BSDF *photonBSDF = photonIsect.GetBSDF(tspack, photonRay, tspack->rng->floatValue());
                if (photonBSDF->NumComponents(photonBxdfType) > 0) {
                    // Deposit photon at surface
					LightPhoton photon(photonIsect.dg.p, alpha.ToXYZ( tspack ).ToRGB(), wo);

                    if (nIntersections == 1) {
						if (finalGather && (!directDone)) {
							// Deposit direct photon
							directPhotons.push_back(photon);

							// Dade - check if we have enough direct photons
							if (directPhotons.size() == nDirectPhotons)
								directDone = true;
						}
                    } else {
                        // Deposit either caustic or indirect photon
                        if (specularPath) {
                            // Process caustic photon intersection
                            if (!causticDone) {
                                causticPhotons.push_back(photon);

                                if (causticPhotons.size() == nCausticPhotons) {
                                    causticDone = true;
                                    causticMap->init(nshot, causticPhotons);
                                }
                            }
                        } else {
                            // Process indirect lighting photon intersection
                            if (!indirectDone) {
                                indirectPhotons.push_back(photon);

                                if (indirectPhotons.size() == nIndirectPhotons) {
                                    indirectDone = true;
                                    indirectMap->init(nshot, indirectPhotons);
                                }
                            }
                        }
                    }

					if (finalGather && 
						(!radianceDone) && 
						(photonBSDF->NumComponents(radianceBxdfType) > 0) && 
						(tspack->rng->floatValue() < 0.25f)) 
					{
						SWCSpectrum rho_t = 
							photonBSDF->rho(tspack, BxDFType(radianceBxdfType & BSDF_ALL_TRANSMISSION));
						SWCSpectrum rho_r = 
							photonBSDF->rho(tspack, BxDFType(radianceBxdfType & BSDF_ALL_REFLECTION));

						if(!rho_t.Black() || !rho_r.Black()) {
							// Store data for radiance photon
							Normal n = photonIsect.dg.nn;
							if (Dot(n, photonRay.d) > 0.f) n = -n;
							radiancePhotons.push_back(RadiancePhoton(photonIsect.dg.p, n));

							rpReflectances.push_back(rho_r);
							rpTransmittances.push_back(rho_t);
							
							if (radiancePhotons.size() == nRadiancePhotons)
								radianceDone = true;
						}
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
                    u1 = tspack->rng->floatValue();
                    u2 = tspack->rng->floatValue();
                    u3 = tspack->rng->floatValue();
                }

                // Compute new photon weight and possibly terminate with RR
                SWCSpectrum fr;
                if (!photonBSDF->Sample_f(tspack, wo, &wi, u1, u2, u3, &fr, &pdf, BSDF_ALL, &flags))
                    break;
                SWCSpectrum anew = alpha * fr * AbsDot(wi, photonBSDF->dgShading.nn) / pdf;
                float continueProb = min<float>(1.0f, anew.filter(tspack) / alpha.filter(tspack));
                if (tspack->rng->floatValue() > continueProb || nIntersections > 10)
                    break;
				alpha = anew / continueProb;
                specularPath = (nIntersections == 1 || specularPath) &&
                        ((flags & BSDF_SPECULAR) != 0);
                photonRay = RayDifferential(photonIsect.dg.p, wi);
            }
        }

        BSDF::FreeAll(tspack);
    }

	boost::xtime photonShootingEndTime;
	boost::xtime_get(&photonShootingEndTime, boost::TIME_UTC);
	ss.str("");
	ss << "Photon shooting done (" << ( photonShootingEndTime.sec - photonShootingStartTime.sec ) << "s)";
	luxError(LUX_NOERROR, LUX_INFO, ss.str().c_str());

    if (finalGather) {
		luxError(LUX_NOERROR, LUX_INFO, "Computing radiance photon map...");

		// Precompute radiance at a subset of the photons
		LightPhotonMap directMap(radianceMap->nLookup, radianceMap->maxDistSquared);
		directMap.init(nDirectPhotons, directPhotons);

        for (u_int i = 0; i < radiancePhotons.size(); ++i) {
			// Dade - print some progress info
			boost::xtime currentTime;
			boost::xtime_get(&currentTime, boost::TIME_UTC);
			if (currentTime.sec - lastUpdateTime.sec > 5) {
				ss.str("");
				ss << "Radiance photon map computation progress: " << i << " (" << (100 * i / radiancePhotons.size()) << "%)";
				luxError(LUX_NOERROR, LUX_INFO, ss.str().c_str());

				lastUpdateTime = currentTime;
			}

            // Compute radiance for radiance photon _i_
            RadiancePhoton &rp = radiancePhotons[i];
            const SWCSpectrum &rho_r = rpReflectances[i];
            const SWCSpectrum &rho_t = rpTransmittances[i];
            const Point& p = rp.p;
            const Normal& n = rp.n;
			SWCSpectrum alpha = 0.f;

            if (!rho_r.Black()) {
                SWCSpectrum E = directMap.EPhoton(tspack, p, n);
				E += indirectMap->EPhoton(tspack, p, n);
				E += causticMap->EPhoton(tspack, p, n);

                alpha += E * INV_PI * rho_r;
            }

            if (!rho_t.Black()) {
                SWCSpectrum E = directMap.EPhoton(tspack, p, -n);
				E += indirectMap->EPhoton(tspack, p, -n);
				E += causticMap->EPhoton(tspack, p, -n);

                alpha += E * INV_PI * rho_t;
            }

			rp.alpha = alpha.ToXYZ( tspack ).ToRGB();
        }

        radianceMap->init(radiancePhotons);

		
		boost::xtime radianceComputeEndTime;
		boost::xtime_get(&radianceComputeEndTime, boost::TIME_UTC);
		ss.str("");
		ss << "Radiance photon map computed (" << ( radianceComputeEndTime.sec - photonShootingEndTime.sec ) << "s)";
		luxError(LUX_NOERROR, LUX_INFO, ss.str().c_str());
    }

	// Dade - check if we have to save maps to a file
	if (mapFileName && !mapsFileExist) {
		luxError(LUX_NOERROR, LUX_INFO, "Saving photon maps to file" );

		std::ofstream ofs(mapFileName->c_str(), std::ios_base::out | std::ios_base::binary);
        if(ofs.good()) {
            // Dade - read the data

			ss.str("");
			ss << "Writing photon maps to '" << (*mapFileName) << "'...";
			luxError(LUX_NOERROR, LUX_INFO, ss.str().c_str());

			bool isLittleEndian = osIsLittleEndian();

			// Dade - save radiance photon map
			if (radianceMap) {
				radianceMap->save(ofs);

				ss.str("");
				ss << "Written " << radianceMap->getPhotonCount() << " radiance photons";
				luxError(LUX_NOERROR, LUX_INFO, ss.str().c_str());
			} else
				osWriteLittleEndianInt(isLittleEndian, ofs, 0);

			// Dade - save indirect photon map
			if (indirectMap) {
				indirectMap->save(ofs);

				ss.str("");
				ss << "Written " << indirectMap->getPhotonCount() << " indirect photons";
				luxError(LUX_NOERROR, LUX_INFO, ss.str().c_str());
			} else
				osWriteLittleEndianInt(isLittleEndian, ofs, 0);

			// Dade - save indirect photon map
			if (causticMap) {
				causticMap->save(ofs);

				ss.str("");
				ss << "Written " << causticMap->getPhotonCount() << " caustic photons";
				luxError(LUX_NOERROR, LUX_INFO, ss.str().c_str());
			} else
				osWriteLittleEndianInt(isLittleEndian, ofs, 0);

			if(!ofs.good()) {
				std::stringstream ss;
				ss << "Error while writing photon maps to file";
				luxError(LUX_SYSTEM, LUX_SEVERE, ss.str().c_str());
			}

			ofs.close();
		} else {
			std::stringstream ss;
			ss << "Cannot open file '" << (*mapFileName) << "' for writing photon maps";
			luxError(LUX_SYSTEM, LUX_SEVERE, ss.str().c_str());
		}
	}
}

SWCSpectrum PhotonMapFinalGatherWithImportaceSampling(
	const TsPack* tspack,
	const Scene *scene,
	const Sample *sample,
	int sampleFinalGather1Offset,
	int sampleFinalGather2Offset,
	int gatherSamples,
	float cosGatherAngle,
	PhotonMapRRStrategy rrStrategy,
	float rrContinueProbability,
	const LightPhotonMap *indirectMap,
	const RadiancePhotonMap *radianceMap,
	const Vector &wo,
	const BSDF *bsdf,
	const BxDFType bxdfType) 
{
	SWCSpectrum L(0.0f);

	// Do one-bounce final gather for photon map
	if (bsdf->NumComponents(bxdfType) > 0) {
		const Point &p = bsdf->dgShading.p;
        const Normal &n = bsdf->dgShading.nn;

		// Find indirect photons around point for importance sampling
		u_int nIndirSamplePhotons = indirectMap->nLookup;
		NearSetPhotonProcess<LightPhoton> proc(nIndirSamplePhotons, p);
		proc.photons = 
			(ClosePhoton<LightPhoton> *) alloca(nIndirSamplePhotons * sizeof(ClosePhoton<LightPhoton>));
		float searchDist2 = indirectMap->maxDistSquared;

		int sanityCheckIndex = 0;
		while (proc.foundPhotons < nIndirSamplePhotons) {
			float md2 = searchDist2;
			proc.foundPhotons = 0;
			indirectMap->lookup(p, proc, md2);

			searchDist2 *= 2.0f;

			if(sanityCheckIndex++ > 32) {
				// Dade - something wrong here
				std::stringstream ss;
				ss << "Internal error in photonmap: point = (" <<
						p << ") searchDist2 = " << searchDist2;
				luxError(LUX_SYSTEM, LUX_ERROR, ss.str().c_str());
				break;
			}
		}

		// Copy photon directions to local array
		Vector *photonDirs = (Vector *) alloca(nIndirSamplePhotons * sizeof(Vector));
		for (u_int i = 0; i < nIndirSamplePhotons; ++i)
			photonDirs[i] = proc.photons[i].photon->wi;

		const float scaledCosGatherAngle = 0.999f * cosGatherAngle;
		// Use BSDF to do final gathering
		SWCSpectrum Li = 0.;
		for (int i = 0; i < gatherSamples ; ++i) {
			float *sampleFGData = sample->sampler->GetLazyValues(
				const_cast<Sample *>(sample), sampleFinalGather1Offset, i);

			// Sample random direction from BSDF for final gather ray
			Vector wi;
			float u1 = sampleFGData[0];
			float u2 = sampleFGData[1];
			float u3 = sampleFGData[2];
			float pdf;
			SWCSpectrum fr;
			if (!bsdf->Sample_f(tspack, wo, &wi, u1, u2, u3, &fr, &pdf, bxdfType)) 
				continue;

			// Dade - russian roulette
			if (rrStrategy == RR_EFFICIENCY) { // use efficiency optimized RR
				const float dp = AbsDot(wi, n) / pdf;
				const float q = min<float>(1.0f, fr.filter(tspack) * dp);
				if (q < sampleFGData[3])
					continue;

				// increase contribution
				fr /= q;
			} else if (rrStrategy == RR_PROBABILITY) { // use normal/probability RR
				if (rrContinueProbability < sampleFGData[3])
					continue;

				// increase path contribution
				fr /= rrContinueProbability;
			}

			// Trace BSDF final gather ray and accumulate radiance
			RayDifferential bounceRay(p, wi);
			Intersection gatherIsect;
			if (scene->Intersect(bounceRay, &gatherIsect)) {
				// Compute exitant radiance using precomputed irradiance
				Normal nGather = gatherIsect.dg.nn;
				if (Dot(nGather, bounceRay.d) > 0) nGather = -nGather;
				NearPhotonProcess<RadiancePhoton> proc(gatherIsect.dg.p, nGather);
				float md2 = radianceMap->maxDistSquared;

				radianceMap->lookup(gatherIsect.dg.p, proc, md2);
				if (proc.photon) {
					SWCSpectrum Lindir = proc.photon->GetSWCSpectrum( tspack );

					scene->Transmittance(tspack, bounceRay, sample, &Lindir);
					// Compute MIS weight for BSDF-sampled gather ray
					// Compute PDF for photon-sampling of direction _wi_
					float photonPdf = 0.f;
					float conePdf = UniformConePdf(cosGatherAngle);
					for (u_int j = 0; j < nIndirSamplePhotons; ++j) {
						if (Dot(photonDirs[j], wi) > scaledCosGatherAngle)
							photonPdf += conePdf;
					}
					photonPdf /= nIndirSamplePhotons;
					float wt = PowerHeuristic(gatherSamples, pdf, gatherSamples, photonPdf);
					Li += fr * Lindir * (AbsDot(wi, n) * wt / pdf);
				}
			}
		}
		L += Li / gatherSamples;

		// Use nearby photons to do final gathering
		Li = 0.;
		for (int i = 0; i < gatherSamples; ++i) {
			float *sampleFGData = sample->sampler->GetLazyValues(
				const_cast<Sample *>(sample), sampleFinalGather2Offset, i);

			// Sample random direction using photons for final gather ray
			float u1 = sampleFGData[2];
			float u2 = sampleFGData[0];
			float u3 = sampleFGData[1];
			int photonNum = min((int) nIndirSamplePhotons - 1,
					Floor2Int(u1 * nIndirSamplePhotons));
			// Sample gather ray direction from _photonNum_
			Vector vx, vy;
			CoordinateSystem(photonDirs[photonNum], &vx, &vy);
			Vector wi = UniformSampleCone(u2, u3, cosGatherAngle, vx, vy,
					photonDirs[photonNum]);
			// Trace photon-sampled final gather ray and accumulate radiance
			SWCSpectrum fr = bsdf->f(tspack, wo, wi, bxdfType);
			if (fr.Black()) continue;

			// Compute PDF for photon-sampling of direction _wi_
			float photonPdf = 0.f;
			float conePdf = UniformConePdf(cosGatherAngle);
			for (u_int j = 0; j < nIndirSamplePhotons; ++j) {
				if (Dot(photonDirs[j], wi) > scaledCosGatherAngle)
					photonPdf += conePdf;
			}
			photonPdf /= nIndirSamplePhotons;

			// Dade - russian roulette
			if (rrStrategy == RR_EFFICIENCY) { // use efficiency optimized RR
				const float dp = 1.0f / photonPdf;
				const float q = min<float>(1.0f, fr.filter(tspack) * dp);
				if (q < sampleFGData[3])
					continue;

				// increase contribution
				fr /= q;
			} else if (rrStrategy == RR_PROBABILITY) { // use normal/probability RR
				if (rrContinueProbability < sampleFGData[3])
					continue;

				// increase path contribution
				fr /= rrContinueProbability;
			}

			RayDifferential bounceRay(p, wi);
			Intersection gatherIsect;
			if (scene->Intersect(bounceRay, &gatherIsect)) {
				// Compute exitant radiance using precomputed irradiance
				Normal nGather = gatherIsect.dg.nn;
				if (Dot(nGather, bounceRay.d) > 0) nGather = -nGather;
				NearPhotonProcess<RadiancePhoton> proc(gatherIsect.dg.p, nGather);
				float md2 = radianceMap->maxDistSquared;

				radianceMap->lookup(gatherIsect.dg.p, proc, md2);
				if (proc.photon) {
					SWCSpectrum Lindir = proc.photon->GetSWCSpectrum( tspack );

					scene->Transmittance(tspack, bounceRay, sample, &Lindir);
					// Compute MIS weight for photon-sampled gather ray
					float bsdfPdf = bsdf->Pdf(tspack, wo, wi, bxdfType);
					float wt = PowerHeuristic(gatherSamples, photonPdf, gatherSamples, bsdfPdf);
					Li += fr * Lindir * (AbsDot(wi, n) * wt / photonPdf);
				}
			}
		}

		L += Li / gatherSamples;
	}

	return L;
}

SWCSpectrum PhotonMapFinalGather(
	const TsPack *tspack,
	const Scene *scene,
	const Sample *sample,
	int sampleFinalGatherOffset,
	int gatherSamples,
	PhotonMapRRStrategy rrStrategy,
	float rrContinueProbability,
	const LightPhotonMap *indirectMap,
	const RadiancePhotonMap *radianceMap,
	const Vector &wo,
	const BSDF *bsdf,
	const BxDFType bxdfType) 
{
	SWCSpectrum L(0.0f);

	// Do one-bounce final gather for photon map
	if (bsdf->NumComponents(bxdfType) > 0) {
		const Point &p = bsdf->dgShading.p;
        const Normal &n = bsdf->dgShading.nn;

		// Use BSDF to do final gathering
		SWCSpectrum Li = 0.;
		for (int i = 0; i < gatherSamples ; ++i) {
			float *sampleFGData = sample->sampler->GetLazyValues(
				const_cast<Sample *>(sample), sampleFinalGatherOffset, i);

			// Sample random direction from BSDF for final gather ray
			Vector wi;
			float u1 = sampleFGData[0];
			float u2 = sampleFGData[1];
			float u3 = sampleFGData[2];
			float pdf;
			SWCSpectrum fr;
			if (!bsdf->Sample_f(tspack, wo, &wi, u1, u2, u3, &fr, &pdf, bxdfType)) 
				continue;

			// Dade - russian roulette
			if (rrStrategy == RR_EFFICIENCY) { // use efficiency optimized RR
				const float dp = AbsDot(wi, n) / pdf;
				const float q = min<float>(1.0f, fr.filter(tspack) * dp);
				if (q < sampleFGData[3])
					continue;

				// increase contribution
				fr /= q;
			} else if (rrStrategy == RR_PROBABILITY) { // use normal/probability RR
				if (rrContinueProbability < sampleFGData[3])
					continue;

				// increase path contribution
				fr /= rrContinueProbability;
			}

			// Trace BSDF final gather ray and accumulate radiance
			RayDifferential bounceRay(p, wi);
			Intersection gatherIsect;
			if (scene->Intersect(bounceRay, &gatherIsect)) {
				// Compute exitant radiance using precomputed irradiance
				Normal nGather = gatherIsect.dg.nn;
				if (Dot(nGather, bounceRay.d) > 0) nGather = -nGather;
				NearPhotonProcess<RadiancePhoton> proc(gatherIsect.dg.p, nGather);
				float md2 = radianceMap->maxDistSquared;

				radianceMap->lookup(gatherIsect.dg.p, proc, md2);
				if (proc.photon) {
					SWCSpectrum Lindir = proc.photon->GetSWCSpectrum( tspack );
					scene->Transmittance(tspack, bounceRay, sample, &Lindir);

					Li += fr * Lindir * (AbsDot(wi, n) / pdf);
				}
			}
		}
		L += Li / gatherSamples;
	}

	return L;
}

void LightPhotonMap::load(std::basic_istream<char> &stream, LightPhotonMap *map) {
	bool isLittleEndian = osIsLittleEndian();

	// Dade - read the size of the map
	int count;
	osReadLittleEndianInt(isLittleEndian, stream, &count);

	int npaths;
	osReadLittleEndianInt(isLittleEndian, stream, &npaths);

	vector<LightPhoton> photons;
	for (int i = 0; i < count; i++) {
		Point p;
		for (int j = 0; j < 3; j++)
			osReadLittleEndianFloat(isLittleEndian, stream, &p[j]);

		XYZColor alpha;
		for (int j = 0; j < WAVELENGTH_SAMPLES; j++)
			osReadLittleEndianFloat(isLittleEndian, stream, &alpha.c[j]);

		Vector wi;
		for (int j = 0; j < 3; j++)
			osReadLittleEndianFloat(isLittleEndian, stream, &wi[j]);

		if (map) {
			LightPhoton lp(p, alpha, wi);

			photons.push_back(lp);
		}
	}

	if (map && (count > 0))
		map->init(npaths, photons);
}

void LightPhotonMap::save(
	std::basic_ostream<char> &stream) const {
	bool isLittleEndian = osIsLittleEndian();

	// Dade - write the size of the map
	osWriteLittleEndianInt(isLittleEndian, stream, photonCount);
	osWriteLittleEndianInt(isLittleEndian, stream, nPaths);

	if (photonmap != NULL) {
		LightPhoton *photons = photonmap->getNodeData();
		for (int i = 0; i < photonCount; i++)
			photons[i].save(isLittleEndian, stream);
	}
}

void RadiancePhotonMap::load(std::basic_istream<char> &stream, RadiancePhotonMap *map) {
	bool isLittleEndian = osIsLittleEndian();

	// Dade - read the size of the map
	int count;
	osReadLittleEndianInt(isLittleEndian, stream, &count);

	vector<RadiancePhoton> photons;
	for (int i = 0; i < count; i++) {
		Point p;
		for (int j = 0; j < 3; j++)
			osReadLittleEndianFloat(isLittleEndian, stream, &p[j]);

		XYZColor alpha;
		for (int j = 0; j < WAVELENGTH_SAMPLES; j++)
			osReadLittleEndianFloat(isLittleEndian, stream, &alpha.c[j]);

		Normal n;
		for (int j = 0; j < 3; j++)
			osReadLittleEndianFloat(isLittleEndian, stream, &n[j]);

		if (map) {
			RadiancePhoton lp(p, alpha, n);

			photons.push_back(lp);
		}
	}

	if (map && (count > 0))
		map->init(photons);
}

void RadiancePhotonMap::save(
	std::basic_ostream<char> &stream) const {
	bool isLittleEndian = osIsLittleEndian();

	// Dade - write the size of the map
	osWriteLittleEndianInt(isLittleEndian, stream, photonCount);

	if (photonmap != NULL) {
		RadiancePhoton *photons = photonmap->getNodeData();
		for (int i = 0; i < photonCount; i++)
			photons[i].save(isLittleEndian, stream);
	}
}

}//namespace lux
