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
 *   Lux Renderer website : http://www.luxrender.net                       *
 ***************************************************************************/

#include "photonmap.h"
#include "light.h"
#include "mc.h"
#include "mcdistribution.h"
#include "spectrumwavelengths.h"
#include "primitive.h"
#include "scene.h"
#include "sampling.h"
#include "camera.h"
#include "error.h"
#include "randomgen.h"
#include "osfunc.h"

#include <fstream>
#include <boost/thread/xtime.hpp>

using namespace lux;

namespace lux
{

SWCSpectrum BasicColorPhoton::GetSWCSpectrum(const SpectrumWavelengths &sw) const
{
	const float delta = (sw.w[0] - w[0]) * WAVELENGTH_SAMPLES /
		(WAVELENGTH_END - WAVELENGTH_START);
	SWCSpectrum result;
	if (delta < 0.f) {
		result.c[0] = Lerp(-delta, alpha.c[0], 0.f);
		for (u_int i = 1; i < WAVELENGTH_SAMPLES; ++i)
			result.c[i] = Lerp(-delta, alpha.c[i], alpha.c[i - 1]);
	} else {
		for (u_int i = 0; i < WAVELENGTH_SAMPLES - 1; ++i)
			result.c[i] = Lerp(delta, alpha.c[i], alpha.c[i + 1]);
		result.c[WAVELENGTH_SAMPLES - 1] = Lerp(delta,
			alpha.c[WAVELENGTH_SAMPLES - 1], 0.f);
	}
	return result;
}

void BasicColorPhoton::save(bool isLittleEndian, std::basic_ostream<char> &stream) const
{
	// Point p
	for (u_int i = 0; i < 3; ++i)
		osWriteLittleEndianFloat(isLittleEndian, stream, p[i]);

	// SWCSpectrum alpha
	for (u_int i = 0; i < WAVELENGTH_SAMPLES; ++i)
		osWriteLittleEndianFloat(isLittleEndian, stream, alpha.c[i]);

	// wavelengths
	for (u_int i = 0; i < WAVELENGTH_SAMPLES; ++i)
		osWriteLittleEndianFloat(isLittleEndian, stream, w[i]);
}

void BasicColorPhoton::load(bool isLittleEndian, std::basic_istream<char> &stream)
{
	// Point p
	for (u_int i = 0; i < 3; ++i)
		p[i] = osReadLittleEndianFloat(isLittleEndian, stream);

	// SWCSpectrum alpha
	for (u_int i = 0; i < WAVELENGTH_SAMPLES; ++i)
		alpha.c[i] = osReadLittleEndianFloat(isLittleEndian, stream);

	// wavelengths
	for (u_int i = 0; i < WAVELENGTH_SAMPLES; ++i)
		w[i] = osReadLittleEndianFloat(isLittleEndian, stream);
}

void LightPhoton::save(bool isLittleEndian, std::basic_ostream<char> &stream) const
{
	// BasicColorPhoton
	BasicColorPhoton::save(isLittleEndian, stream);

	// Vector wi
	for (u_int i = 0; i < 3; ++i)
		osWriteLittleEndianFloat(isLittleEndian, stream, wi[i]);
}

void LightPhoton::load(bool isLittleEndian, std::basic_istream<char> &stream)
{
	// BasicColorPhoton
	BasicColorPhoton::load(isLittleEndian, stream);

	// Vector wi
	for (u_int i = 0; i < 3; ++i)
		wi[i] = osReadLittleEndianFloat(isLittleEndian, stream);
}

void RadiancePhoton::save(bool isLittleEndian, std::basic_ostream<char> &stream) const
{
	// BasicColorPhoton
	BasicColorPhoton::save(isLittleEndian, stream);

	// Normal n
	for (u_int i = 0; i < 3; ++i)
		osWriteLittleEndianFloat(isLittleEndian, stream, n[i]);
}

void RadiancePhoton::load(bool isLittleEndian, std::basic_istream<char> &stream)
{
	// BasicColorPhoton
	BasicColorPhoton::load(isLittleEndian, stream);

	// Normal n
	for (u_int i = 0; i < 3; ++i)
		n[i] = osReadLittleEndianFloat(isLittleEndian, stream);
}

SWCSpectrum RadiancePhotonMap::LPhoton(const SpectrumWavelengths &sw,
	const Intersection& isect, const Vector& wo,
	const BxDFType bxdfType) const 
{
	SWCSpectrum L(0.f);
	if (!photonmap)
		return L;

	Normal ng = isect.dg.nn;
	if (Dot(wo, ng) < 0.f)
		ng = -ng;

	const Point& p = isect.dg.p;

	if ((bxdfType & BSDF_REFLECTION) != 0) {
		// Add reflected radiance
		NearPhotonProcess<RadiancePhoton> procRefl(p, ng);
		float md2Refl = maxDistSquared;
		lookup(p, procRefl, md2Refl);
		if (procRefl.photon)
			L += procRefl.photon->GetSWCSpectrum(sw);
	}

	if ((bxdfType & BSDF_TRANSMISSION ) != 0) {
		// Add transmitted radiance
		NearPhotonProcess<RadiancePhoton> procTransm(p, -ng);
		float md2Transm = maxDistSquared;
		lookup(p, procTransm, md2Transm);
		if (procTransm.photon)
			L += procTransm.photon->GetSWCSpectrum(sw);
	}

	return L;
}

SWCSpectrum LightPhotonMap::EPhoton(const SpectrumWavelengths &sw,
	const Point &p, const Normal &n) const
{
	SWCSpectrum E(0.f);
	if ((nPaths <= 0) || (!photonmap))
		return E;

	// Lookup nearby photons at irradiance computation point
	NearSetPhotonProcess<LightPhoton> proc(nLookup, p);
	proc.photons = 
		static_cast<ClosePhoton<LightPhoton> *>(alloca(nLookup * sizeof (ClosePhoton<LightPhoton>)));
	float md2 = maxDistSquared;
	lookup(p, proc, md2);

	// Accumulate irradiance value from nearby photons
	ClosePhoton<LightPhoton> *photons = proc.photons;
	for (u_int i = 0; i < proc.foundPhotons; ++i) {
		if (Dot(n, photons[i].photon->wi) > 0.f)
			E += photons[i].photon->GetSWCSpectrum(sw);
	}

	return E / (nPaths * md2 * M_PI);
}

SWCSpectrum LightPhotonMap::LPhoton(const SpectrumWavelengths &sw,
	const BSDF *bsdf, const Intersection &isect, const Vector &wo,
	const BxDFType bxdfType) const 
{
	SWCSpectrum L(0.f);
	if ((nPaths <= 0) || (!photonmap))
		return L;

	if (bsdf->NumComponents(bxdfType) == 0)
		return L;

	// Initialize _PhotonProcess_ object, _proc_, for photon map lookups
	NearSetPhotonProcess<LightPhoton> proc(nLookup, isect.dg.p);
	proc.photons = 
		static_cast<ClosePhoton<LightPhoton> *>(alloca(nLookup * sizeof (ClosePhoton<LightPhoton>)));
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
		BxDFType flag = BxDFType(bxdfType &
			(Dot(Nf, p->wi) > 0.f ?
			BSDF_ALL_REFLECTION : BSDF_ALL_TRANSMISSION));
		float k = Ekernel(p, isect.dg.p, distSquared);
		const SWCSpectrum alpha = p->GetSWCSpectrum(sw);

		L += (k / nPaths) * bsdf->F(sw, p->wi, wo, false, flag) * alpha;
	}

	return L;
}

SWCSpectrum LightPhotonMap::LPhotonDiffuseApprox(const SpectrumWavelengths &sw,
	const BSDF *bsdf, const Intersection &isect, const Vector &wo,
	const BxDFType bxdfType) const
{
	SWCSpectrum L(0.f);
	if ((nPaths <= 0) || (!photonmap))
		return L;

	if (bsdf->NumComponents(bxdfType) == 0)
		return L;

	// Initialize _PhotonProcess_ object, _proc_, for photon map lookups
	NearSetPhotonProcess<LightPhoton> proc(nLookup, isect.dg.p);
	proc.photons = 
		static_cast<ClosePhoton<LightPhoton> *>(alloca(nLookup * sizeof (ClosePhoton<LightPhoton>)));
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
	SWCSpectrum Lr(0.f), Lt(0.f);

	for (u_int i = 0; i < nFound; ++i) {
		const LightPhoton *p = photons[i].photon;
		const SWCSpectrum alpha = p->GetSWCSpectrum(sw);

		float k = Ekernel(p, isect.dg.p, distSquared);
		if (Dot(Nf, photons[i].photon->wi) > 0.f)
			Lr += (k / nPaths) * alpha;
		else
			Lt += (k / nPaths) * alpha;
	}

	if ((bxdfType & BSDF_REFLECTION) != 0)
		L += Lr * bsdf->rho(sw, wo, BxDFType(bxdfType & BSDF_ALL_REFLECTION)) * INV_PI;
	if ((bxdfType & BSDF_TRANSMISSION) != 0)
		L += Lt * bsdf->rho(sw, wo, BxDFType(bxdfType & BSDF_ALL_TRANSMISSION)) * INV_PI;

	return L;
}

SWCSpectrum LightPhotonMap::LDiffusePhoton(const SpectrumWavelengths &sw,
	const BSDF *bsdf, const Intersection &isect, const Vector &wo) const
{
	SWCSpectrum L(0.f);
	if ((nPaths <= 0) || (!photonmap))
		return L;

	const BxDFType diffuseType = BxDFType(BSDF_DIFFUSE | BSDF_REFLECTION | BSDF_TRANSMISSION);

	if (bsdf->NumComponents(diffuseType) == 0)
		return L;

	// Initialize _PhotonProcess_ object, _proc_, for photon map lookups
	NearSetPhotonProcess<LightPhoton> proc(nLookup, isect.dg.p);
	proc.photons = 
		static_cast<ClosePhoton<LightPhoton> *>(alloca(nLookup * sizeof (ClosePhoton<LightPhoton>)));
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
	SWCSpectrum Lr(0.f), Lt(0.f);

	for (u_int i = 0; i < nFound; ++i) {
		const LightPhoton *p = photons[i].photon;
		const SWCSpectrum alpha = p->GetSWCSpectrum(sw);

		float k = Ekernel(p, isect.dg.p, distSquared);
		if (Dot(Nf, photons[i].photon->wi) > 0.f)
			Lr += (k / nPaths) * alpha;
		else
			Lt += (k / nPaths) * alpha;
	}

	L = Lr * bsdf->rho(sw, wo, BxDFType(BSDF_DIFFUSE | BSDF_REFLECTION)) * INV_PI +
		Lt * bsdf->rho(sw, wo, BxDFType(BSDF_DIFFUSE | BSDF_TRANSMISSION)) * INV_PI;

	return L;
}

static bool unsuccessful(u_int needed, u_int found, u_int shot)
{
	return (found < needed && (found == 0 || found < shot / 1024));
}

void PhotonMapPreprocess(const RandomGenerator &rng, const Scene &scene, 
	const string *mapFileName, const BxDFType photonBxdfType,
	const BxDFType radianceBxdfType, u_int nDirectPhotons,
	u_int nRadiancePhotons, RadiancePhotonMap *radianceMap,
	u_int nIndirectPhotons, LightPhotonMap *indirectMap,
	u_int nCausticPhotons, LightPhotonMap *causticMap,
	u_int maxDepth)
{
	if (scene.lights.size() == 0)
		return;

	std::stringstream ss;

	// Dade - try to read the photon maps from file
	if (mapFileName) {
		// Dade - check if the maps file exists
		std::ifstream ifs(mapFileName->c_str(), std::ios_base::in | std::ios_base::binary);

		if (ifs.good()) {
			LOG( LUX_INFO,LUX_NOERROR) << "Found photon maps file: " << *mapFileName;

			bool isLittleEndian = osIsLittleEndian();

			bool ok = true; // flag indicating whether all is ok or not

			// Dade - read the settings
			int storedPhotonBxdfType;
			int storedRadianceBxdfType;
			u_int storedNDirectPhotons;
			u_int storedNRadiancePhotons;
			u_int storedNIndirectPhotons;
			u_int storedNCausticPhotons;
			storedPhotonBxdfType = osReadLittleEndianInt(isLittleEndian, ifs);
			storedRadianceBxdfType = osReadLittleEndianInt(isLittleEndian, ifs);
			storedNDirectPhotons = osReadLittleEndianUInt(isLittleEndian, ifs);
			storedNRadiancePhotons = osReadLittleEndianUInt(isLittleEndian, ifs);
			storedNIndirectPhotons = osReadLittleEndianUInt(isLittleEndian, ifs);
			storedNCausticPhotons = osReadLittleEndianUInt(isLittleEndian, ifs);
			if (storedPhotonBxdfType != photonBxdfType ||
				storedRadianceBxdfType != radianceBxdfType ||
				storedNDirectPhotons != nDirectPhotons ||
				storedNRadiancePhotons != nRadiancePhotons ||
				storedNIndirectPhotons != nIndirectPhotons ||
				storedNCausticPhotons != nCausticPhotons) {
				LOG( LUX_INFO,LUX_NOERROR)<< "Some photon map settings changed, rebuilding photon maps...";
				ok = false;
			}
			if (ok) {
				//TODO should compare a scene hash or something
				u_int storedNLights;
				storedNLights = osReadLittleEndianUInt(isLittleEndian, ifs);
				if (storedNLights != scene.lights.size() ) {
					LOG( LUX_INFO,LUX_NOERROR)<< "Scene changed, rebuilding photon maps...";
					ok = false;
				}
			}

			// Dade - read the data
			if (ok) {
				LOG( LUX_INFO,LUX_NOERROR)<< "Reading radiance photon map...";
				RadiancePhotonMap::load(ifs, radianceMap);
				LOG(LUX_INFO,LUX_NOERROR) << "Read " << radianceMap->getPhotonCount() << " radiance photons";

				if (!ifs.good()) {
					LOG( LUX_INFO,LUX_NOERROR)<< "Failed to read all photon maps";
					ok = false;
				}
			}
			if (ok) {
				LOG( LUX_INFO,LUX_NOERROR)<< "Reading indirect photon map...";
				LightPhotonMap::load(ifs, indirectMap);
				LOG(LUX_INFO,LUX_NOERROR) << "Read " << indirectMap->getPhotonCount() << " light photons";

				if (!ifs.good()) {
					LOG( LUX_INFO,LUX_NOERROR)<< "Failed to read all photon maps";
					ok = false;
				}
			}
			if (ok) {
				LOG( LUX_INFO,LUX_NOERROR)<< "Reading caustic photon map...";
				LightPhotonMap::load(ifs, causticMap);
				LOG(LUX_INFO,LUX_NOERROR) << "Read " << causticMap->getPhotonCount() << " light photons";

				if (!ifs.good()) {
					LOG( LUX_INFO,LUX_NOERROR)<< "Failed to read all photon maps";
					ok = false;
				}
			}
			
			// Close file
			ifs.close();

			// Return if all is ok
			if (ok)
				return;
		} else {
			LOG( LUX_INFO,LUX_NOERROR)<< "Photon maps file doesn't exist yet";
			ifs.close();
		}
	}

	// Dade - check if have to build the radiancemap
	bool computeRadianceMap = (nRadiancePhotons > 0);

	// Dade - shoot photons
	const u_int targetPhotons = nCausticPhotons + nIndirectPhotons;
	LOG(LUX_INFO,LUX_NOERROR) << "Shooting photons (target: " << targetPhotons << ")...";

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
	Sample sample(scene.surfaceIntegrator, scene.volumeIntegrator, scene);
	sample.rng = &rng;
	SpectrumWavelengths &sw(sample.swl);
	sample.camera = scene.camera->Clone();
	sample.realTime = sample.camera->GetTime(.5f); //FIXME sample it
	sample.camera->SampleMotion(sample.realTime);

	// Compute light power CDF for photon shooting
	u_int nLights = scene.lights.size();
	float *lightPower = new float[nLights];
	for (u_int i = 0; i < nLights; ++i)
		lightPower[i] = scene.lights[i]->Power(scene);
	Distribution1D lightCDF(lightPower, nLights);
	delete[] lightPower;

	// Declare radiance photon reflectance arrays
	vector<SWCSpectrum> rpReflectances;
	rpReflectances.reserve(nRadiancePhotons);
	vector<SWCSpectrum> rpTransmittances;
	rpTransmittances.reserve(nRadiancePhotons);

	boost::xtime photonShootingStartTime;
	boost::xtime lastUpdateTime;
	boost::xtime_get(&photonShootingStartTime, boost::TIME_UTC);
	boost::xtime_get(&lastUpdateTime, boost::TIME_UTC);
	u_int nshot = 0;
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
			LOG(LUX_INFO,LUX_NOERROR)<< ss.str().c_str();

			lastUpdateTime = currentTime;
		}
		
		++nshot;

		// Give up if we're not storing enough photons
		if (nshot > max(500000U, targetPhotons * 10)) {
			if (indirectDone && unsuccessful(nCausticPhotons, causticPhotons.size(), nshot)) {
				// Dade - disable castic photon map: we are unable to store
				// enough photons
				LOG( LUX_WARNING,LUX_CONSISTENCY)<< "Unable to store enough photons in the caustic photonmap. Giving up and disabling the map.";

				causticPhotons.clear();
				causticDone = true;
				nCausticPhotons = 0;
			}

			if (unsuccessful(nIndirectPhotons, indirectPhotons.size(), nshot)) {
				LOG( LUX_ERROR,LUX_CONSISTENCY)<< "Unable to store enough photons in the indirect photonmap. Unable to render the image.";
				return;
			}
		}
		// Sample the wavelengths
		sw.Sample(RadicalInverse(nshot, 2));

		// Trace a photon path and store contribution
		// Choose 6D sample values for photon
		float u[6];
		u[0] = RadicalInverse(nshot, 3);
		u[1] = RadicalInverse(nshot, 5);
		u[2] = RadicalInverse(nshot, 7);
		u[3] = RadicalInverse(nshot, 11);
		u[4] = RadicalInverse(nshot, 13);
		u[5] = RadicalInverse(nshot, 17);

		// Choose light to shoot photon from
		float lightPdf;
		float uln = RadicalInverse(nshot, 19);
		u_int lightNum = lightCDF.SampleDiscrete(uln, &lightPdf);
		const Light *light = scene.lights[lightNum];

		// Generate _photonRay_ from light source and initialize _alpha_
		BSDF *bsdf;
		float pdf;
		SWCSpectrum alpha;
		if (!light->SampleL(scene, sample, u[0], u[1], u[2],
			&bsdf, &pdf, &alpha))
			continue;
		Ray photonRay;
		photonRay.o = bsdf->dgShading.p;
		float pdf2;
		SWCSpectrum alpha2;
		if (!bsdf->SampleF(sw, Vector(bsdf->nn), &photonRay.d,
			u[3], u[4], u[5], &alpha2, &pdf2))
			continue;
		alpha *= alpha2;
		alpha /= lightPdf;

		if (!alpha.Black()) {
			// Follow photon path through scene and record intersections
			bool specularPath = false, directPhoton = true;
			Intersection photonIsect;
			const Volume *volume = NULL; //FIXME: try to get volume from light
			BSDF *photonBSDF;
			u_int nIntersections = 0;
			while (scene.Intersect(sample, volume, false,
				photonRay, 1.f, &photonIsect, &photonBSDF,
				NULL, NULL, &alpha)) {
				++nIntersections;

				// Handle photon/surface intersection
				Vector wo = -photonRay.d;

				if (photonBSDF->NumComponents(photonBxdfType) > 0) {
					// Deposit photon at surface
					LightPhoton photon(sw, photonIsect.dg.p, alpha, wo);

					if (directPhoton) {
						if (computeRadianceMap && (!directDone)) {
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

					if (computeRadianceMap && 
						(!radianceDone) && 
						(photonBSDF->NumComponents(radianceBxdfType) > 0) && 
						(rng.floatValue() < 0.125f)) {
						SWCSpectrum rho_t =
							photonBSDF->rho(sw, BxDFType(radianceBxdfType & BSDF_ALL_TRANSMISSION));
						SWCSpectrum rho_r = 
							photonBSDF->rho(sw, BxDFType(radianceBxdfType & BSDF_ALL_REFLECTION));

						if(!rho_t.Black() || !rho_r.Black()) {
							// Store data for radiance photon
							Normal n = photonIsect.dg.nn;
							if (Dot(n, photonRay.d) > 0.f)
								n = -n;
							radiancePhotons.push_back(RadiancePhoton(sw, photonIsect.dg.p, n));

							rpReflectances.push_back(rho_r);
							rpTransmittances.push_back(rho_t);
							
							if (radiancePhotons.size() == nRadiancePhotons)
								radianceDone = true;
						}
					}
				}

				// Sample new photon ray direction
				Vector wi;
				float pdfo;
				BxDFType flags;
				// Get random numbers for sampling outgoing photon direction
				float u1, u2, u3;
				if (nIntersections == 1) {
					u1 = RadicalInverse(nshot, 23);
					u2 = RadicalInverse(nshot, 29);
					u3 = RadicalInverse(nshot, 31);
				} else {
					u1 = rng.floatValue();
					u2 = rng.floatValue();
					u3 = rng.floatValue();
				}

				// Compute new photon weight and possibly terminate with RR
				SWCSpectrum fr;
				if (!photonBSDF->SampleF(sw, wo, &wi, u1, u2, u3, &fr, &pdfo, BSDF_ALL, &flags))
					break;
				SWCSpectrum anew = fr;
				float continueProb = min(1.f, anew.Filter(sw));
				if (nIntersections > maxDepth || rng.floatValue() > continueProb)
					break;
				alpha *= anew / continueProb;
				const bool passThrough = flags == (BSDF_TRANSMISSION | BSDF_SPECULAR) &&
					photonBSDF->Pdf(sw, wo, wi, BxDFType(BSDF_TRANSMISSION | BSDF_SPECULAR)) > 0.f;
				if (!passThrough) {
					specularPath = (directPhoton || specularPath) &&
						((flags & BSDF_SPECULAR) != 0 || pdfo > 100.f);
					directPhoton = false;
				}
				photonRay = Ray(photonIsect.dg.p, wi);
				volume = photonBSDF->GetVolume(photonRay.d);
			}
		}

		sample.arena.FreeAll();
	}

	boost::xtime photonShootingEndTime;
	boost::xtime_get(&photonShootingEndTime, boost::TIME_UTC);
	LOG(LUX_INFO,LUX_NOERROR) << "Photon shooting done (" << ( photonShootingEndTime.sec - photonShootingStartTime.sec ) << "s)";

	if (computeRadianceMap) {
		LOG( LUX_INFO,LUX_NOERROR)<< "Computing radiance photon map...";

		// Precompute radiance at a subset of the photons
		LightPhotonMap directMap(radianceMap->nLookup, radianceMap->maxDistSquared);
		directMap.init(nDirectPhotons, directPhotons);

		for (u_int i = 0; i < radiancePhotons.size(); ++i) {
			// Dade - print some progress info
			boost::xtime currentTime;
			boost::xtime_get(&currentTime, boost::TIME_UTC);
			if (currentTime.sec - lastUpdateTime.sec > 5) {
				LOG(LUX_INFO,LUX_NOERROR) << "Radiance photon map computation progress: " << i << " (" << (100 * i / radiancePhotons.size()) << "%)";

				lastUpdateTime = currentTime;
			}

			// Compute radiance for radiance photon _i_
			RadiancePhoton &rp = radiancePhotons[i];
			const SWCSpectrum &rho_r = rpReflectances[i];
			const SWCSpectrum &rho_t = rpTransmittances[i];
			const Point& p = rp.p;
			const Normal& n = rp.n;
			SWCSpectrum alpha(0.f);
			for (u_int j = 0; j < WAVELENGTH_SAMPLES; ++j)
				sw.w[j] = rp.w[j];

			if (!rho_r.Black()) {
				SWCSpectrum E = directMap.EPhoton(sw, p, n);
				E += indirectMap->EPhoton(sw, p, n);
				E += causticMap->EPhoton(sw, p, n);

				alpha += E * INV_PI * rho_r;
			}

			if (!rho_t.Black()) {
				SWCSpectrum E = directMap.EPhoton(sw, p, -n);
				E += indirectMap->EPhoton(sw, p, -n);
				E += causticMap->EPhoton(sw, p, -n);

				alpha += E * INV_PI * rho_t;
			}

			rp.alpha = alpha;
		}

		radianceMap->init(radiancePhotons);


		boost::xtime radianceComputeEndTime;
		boost::xtime_get(&radianceComputeEndTime, boost::TIME_UTC);
		LOG(LUX_INFO,LUX_NOERROR) << "Radiance photon map computed (" << ( radianceComputeEndTime.sec - photonShootingEndTime.sec ) << "s)";
	}

	// Dade - check if we have to save maps to a file
	if (mapFileName) {
		LOG( LUX_INFO,LUX_NOERROR)<< "Saving photon maps to file";

		std::ofstream ofs(mapFileName->c_str(), std::ios_base::out | std::ios_base::binary);
		if(ofs.good()) {
			LOG(LUX_INFO,LUX_NOERROR)<< "Writing photon maps to '" << (*mapFileName) << "'...";

			bool isLittleEndian = osIsLittleEndian();

			// Write settings
			osWriteLittleEndianUInt(isLittleEndian, ofs, photonBxdfType);
			osWriteLittleEndianUInt(isLittleEndian, ofs, radianceBxdfType);
			osWriteLittleEndianUInt(isLittleEndian, ofs, nDirectPhotons);
			osWriteLittleEndianUInt(isLittleEndian, ofs, nRadiancePhotons);
			osWriteLittleEndianUInt(isLittleEndian, ofs, nIndirectPhotons);
			osWriteLittleEndianUInt(isLittleEndian, ofs, nCausticPhotons);
			osWriteLittleEndianUInt(isLittleEndian, ofs, scene.lights.size());

			// Dade - write the data
			// Dade - save radiance photon map
			if (radianceMap) {
				radianceMap->save(ofs);

				LOG(LUX_INFO,LUX_NOERROR) << "Written " << radianceMap->getPhotonCount() << " radiance photons";
			} else
				osWriteLittleEndianInt(isLittleEndian, ofs, 0);

			// Dade - save indirect photon map
			if (indirectMap) {
				indirectMap->save(ofs);

				LOG(LUX_INFO,LUX_NOERROR) << "Written " << indirectMap->getPhotonCount() << " indirect photons";
			} else
				osWriteLittleEndianInt(isLittleEndian, ofs, 0);

			// Dade - save indirect photon map
			if (causticMap) {
				causticMap->save(ofs);

				LOG(LUX_INFO,LUX_NOERROR) << "Written " << causticMap->getPhotonCount() << " caustic photons";
			} else
				osWriteLittleEndianInt(isLittleEndian, ofs, 0);

			if(!ofs.good()) {
				LOG( LUX_SEVERE,LUX_SYSTEM) << "Error while writing photon maps to file";
			}

			ofs.close();
		} else {
			LOG(LUX_SEVERE,LUX_SYSTEM)<< "Cannot open file '" << (*mapFileName) << "' for writing photon maps";
		}
	}
}

SWCSpectrum PhotonMapFinalGatherWithImportaceSampling(const Scene &scene,
	const Sample &sample, u_int sampleFinalGather1Offset,
	u_int sampleFinalGather2Offset, u_int gatherSamples, float cosGatherAngle,
	PhotonMapRRStrategy rrStrategy, float rrContinueProbability,
	const LightPhotonMap *indirectMap, const RadiancePhotonMap *radianceMap,
	const Vector &wo, const BSDF *bsdf, const BxDFType bxdfType) 
{
	SWCSpectrum L(0.f);

	// Do one-bounce final gather for photon map
	if (bsdf->NumComponents(bxdfType) > 0 && !indirectMap->IsEmpty()) {
		const Point &p = bsdf->dgShading.p;

		// Find indirect photons around point for importance sampling
		u_int nIndirSamplePhotons = indirectMap->nLookup;
		NearSetPhotonProcess<LightPhoton> proc(nIndirSamplePhotons, p);
		proc.photons = 
			static_cast<ClosePhoton<LightPhoton> *>(alloca(nIndirSamplePhotons * sizeof(ClosePhoton<LightPhoton>)));
		float searchDist2 = indirectMap->maxDistSquared;

		u_int sanityCheckIndex = 0;
		while (proc.foundPhotons < nIndirSamplePhotons) {
			float md2 = searchDist2;
			proc.foundPhotons = 0;
			indirectMap->lookup(p, proc, md2);

			searchDist2 *= 2.f;

			if (sanityCheckIndex++ > 32) {
				// Dade - something wrong here
				LOG( LUX_ERROR,LUX_SYSTEM)
					<< "Internal error in photonmap: point = (" <<
					p << ") searchDist2 = " << searchDist2;
				break;
			}
		}

		// Copy photon directions to local array
		Vector *photonDirs = static_cast<Vector *>(alloca(nIndirSamplePhotons * sizeof(Vector)));
		for (u_int i = 0; i < nIndirSamplePhotons; ++i)
			photonDirs[i] = proc.photons[i].photon->wi;

		const float scaledCosGatherAngle = 0.999f * cosGatherAngle;
		const SpectrumWavelengths &sw(sample.swl);
		// Use BSDF to do final gathering
		SWCSpectrum Li(0.f);
		for (u_int i = 0; i < gatherSamples ; ++i) {
			float *sampleFGData = scene.sampler->GetLazyValues(
				sample, sampleFinalGather1Offset, i);

			// Sample random direction from BSDF for final gather ray
			Vector wi;
			float u1 = sampleFGData[0];
			float u2 = sampleFGData[1];
			float u3 = sampleFGData[2];
			float pdf;
			SWCSpectrum fr;
			if (!bsdf->SampleF(sw, wo, &wi, u1, u2, u3, &fr, &pdf, bxdfType, NULL, NULL, true)) 
				continue;

			// Dade - russian roulette
			if (rrStrategy == RR_EFFICIENCY) { // use efficiency optimized RR
				const float q = min(1.f, fr.Filter(sw));
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
			Ray bounceRay(p, wi);
			Intersection gatherIsect;
			if (scene.Intersect(sample, bsdf->GetVolume(wi), false,
				bounceRay, 1.f, &gatherIsect, NULL, NULL, NULL,
				&fr)) {
				// Compute exitant radiance using precomputed irradiance
				Normal nGather = gatherIsect.dg.nn;
				if (Dot(nGather, bounceRay.d) > 0)
					nGather = -nGather;
				NearPhotonProcess<RadiancePhoton> procir(gatherIsect.dg.p, nGather);
				float md2 = radianceMap->maxDistSquared;

				radianceMap->lookup(gatherIsect.dg.p, procir, md2);
				if (procir.photon) {
					SWCSpectrum Lindir = procir.photon->GetSWCSpectrum(sw);

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
					// Limit weight when intersection point is close
					if (bounceRay.maxt < sqrtf(md2))
						wt *= (1.f + bounceRay.maxt / sqrtf(md2)) / 2.f;
					Li += fr * Lindir * wt;
				}
			}
		}
		L += Li / gatherSamples;

		// Use nearby photons to do final gathering
		Li = 0.f;
		for (u_int i = 0; i < gatherSamples; ++i) {
			float *sampleFGData = scene.sampler->GetLazyValues(
				sample, sampleFinalGather2Offset, i);

			// Sample random direction using photons for final gather ray
			float u1 = sampleFGData[2];
			float u2 = sampleFGData[0];
			float u3 = sampleFGData[1];
			u_int photonNum = min(nIndirSamplePhotons - 1,
					Floor2UInt(u1 * nIndirSamplePhotons));
			// Sample gather ray direction from _photonNum_
			Vector vx, vy;
			CoordinateSystem(photonDirs[photonNum], &vx, &vy);
			Vector wi = UniformSampleCone(u2, u3, cosGatherAngle, vx, vy,
				photonDirs[photonNum]);
			// Trace photon-sampled final gather ray and accumulate radiance
			SWCSpectrum fr(bsdf->F(sw, wi, wo, true, bxdfType));
			if (fr.Black())
				continue;

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
				const float dp = 1.f / photonPdf;
				const float q = min(1.f, fr.Filter(sw) * dp);
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

			Ray bounceRay(p, wi);
			Intersection gatherIsect;
			if (scene.Intersect(sample, bsdf->GetVolume(wi), false,
				bounceRay, 1.f, &gatherIsect, NULL, NULL, NULL,
				&fr)) {
				// Compute exitant radiance using precomputed irradiance
				Normal nGather = gatherIsect.dg.nn;
				if (Dot(nGather, bounceRay.d) > 0)
					nGather = -nGather;
				NearPhotonProcess<RadiancePhoton> procir(gatherIsect.dg.p, nGather);
				float md2 = radianceMap->maxDistSquared;

				radianceMap->lookup(gatherIsect.dg.p, procir, md2);
				if (procir.photon) {
					SWCSpectrum Lindir = procir.photon->GetSWCSpectrum(sw);

					// Compute MIS weight for photon-sampled gather ray
					float bsdfPdf = bsdf->Pdf(sw, wi, wo, bxdfType);
					float wt = PowerHeuristic(gatherSamples, photonPdf, gatherSamples, bsdfPdf);
					// Limit weight when intersection point is close
					if (bounceRay.maxt < sqrtf(md2))
						wt *= (1.f + bounceRay.maxt / sqrtf(md2)) / 2.f;
					Li += fr * Lindir * (wt / photonPdf);
				}
			}
		}

		L += Li / gatherSamples;
	}

	return L;
}

SWCSpectrum PhotonMapFinalGather(const Scene &scene, const Sample &sample,
	u_int sampleFinalGatherOffset, u_int gatherSamples,
	PhotonMapRRStrategy rrStrategy, float rrContinueProbability,
	const LightPhotonMap *indirectMap, const RadiancePhotonMap *radianceMap,
	const Vector &wo, const BSDF *bsdf, const BxDFType bxdfType) 
{
	SWCSpectrum L(0.f);

	// Do one-bounce final gather for photon map
	if (bsdf->NumComponents(bxdfType) > 0 && !radianceMap->IsEmpty()) {
		const Point &p = bsdf->dgShading.p;
		const SpectrumWavelengths &sw(sample.swl);

		// Use BSDF to do final gathering
		SWCSpectrum Li(0.f);
		for (u_int i = 0; i < gatherSamples ; ++i) {
			float *sampleFGData = scene.sampler->GetLazyValues(
				sample, sampleFinalGatherOffset, i);

			// Sample random direction from BSDF for final gather ray
			Vector wi;
			float u1 = sampleFGData[0];
			float u2 = sampleFGData[1];
			float u3 = sampleFGData[2];
			float pdf;
			SWCSpectrum fr;
			if (!bsdf->SampleF(sw, wo, &wi, u1, u2, u3, &fr, &pdf, bxdfType, NULL, NULL, true)) 
				continue;

			// Dade - russian roulette
			if (rrStrategy == RR_EFFICIENCY) { // use efficiency optimized RR
				const float q = min(1.f, fr.Filter(sw));
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
			Ray bounceRay(p, wi);
			Intersection gatherIsect;
			if (scene.Intersect(sample, bsdf->GetVolume(wi), false,
				bounceRay, 1.f, &gatherIsect, NULL, NULL, NULL,
				&fr)) {
				// Compute exitant radiance using precomputed irradiance
				Normal nGather = gatherIsect.dg.nn;
				if (Dot(nGather, bounceRay.d) > 0)
					nGather = -nGather;
				NearPhotonProcess<RadiancePhoton> proc(gatherIsect.dg.p, nGather);
				float md2 = radianceMap->maxDistSquared;

				radianceMap->lookup(gatherIsect.dg.p, proc, md2);
				if (proc.photon) {
					SWCSpectrum Lindir = proc.photon->GetSWCSpectrum(sw);

					Li += fr * Lindir;
				}
			}
		}
		L += Li / gatherSamples;
	}

	return L;
}

void LightPhotonMap::load(std::basic_istream<char> &stream, LightPhotonMap *map)
{
	if (!map)
		return;

	bool isLittleEndian = osIsLittleEndian();

	// Dade - read the size of the map
	u_int count = osReadLittleEndianUInt(isLittleEndian, stream);

	u_int npaths = osReadLittleEndianUInt(isLittleEndian, stream);

	vector<LightPhoton> photons(count);
	for (u_int i = 0; i < count; ++i)
		photons[i].load(isLittleEndian, stream);

	if (count > 0)
		map->init(npaths, photons);
}

void LightPhotonMap::save(std::basic_ostream<char> &stream) const
{
	bool isLittleEndian = osIsLittleEndian();

	// Dade - write the size of the map
	osWriteLittleEndianUInt(isLittleEndian, stream, photonCount);
	osWriteLittleEndianUInt(isLittleEndian, stream, nPaths);

	if (photonmap != NULL) {
		LightPhoton *photons = photonmap->getNodeData();
		for (u_int i = 0; i < photonCount; i++)
			photons[i].save(isLittleEndian, stream);
	}
}

void RadiancePhotonMap::load(std::basic_istream<char> &stream, RadiancePhotonMap *map)
{
	if(!map)
		return;

	bool isLittleEndian = osIsLittleEndian();

	// Dade - read the size of the map
	u_int count = osReadLittleEndianUInt(isLittleEndian, stream);

	vector<RadiancePhoton> photons(count);
	for (u_int i = 0; i < count; ++i)
		photons[i].load(isLittleEndian, stream);

	if (count > 0)
		map->init(photons);
}

void RadiancePhotonMap::save(std::basic_ostream<char> &stream) const
{
	bool isLittleEndian = osIsLittleEndian();

	// Dade - write the size of the map
	osWriteLittleEndianUInt(isLittleEndian, stream, photonCount);

	if (photonmap != NULL) {
		RadiancePhoton *photons = photonmap->getNodeData();
		for (u_int i = 0; i < photonCount; ++i)
			photons[i].save(isLittleEndian, stream);
	}
}

}//namespace lux
