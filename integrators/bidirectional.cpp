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

// bidirectional.cpp*
#include "bidirectional.h"
#include "reflection/bxdf.h"
#include "light.h"
#include "camera.h"
#include "sampling.h"
#include "scene.h"
#include "paramset.h"
#include "dynload.h"

using namespace lux;

static const u_int passThroughLimit = 10000;
static const u_int rrStart = 3;

struct BidirVertex {
	BidirVertex() : pdf(0.f), pdfR(0.f), tPdf(1.f), tPdfR(1.f),
		dAWeight(0.f), dARWeight(0.f), rr(1.f), rrR(1.f),
		flux(0.f), bsdf(NULL), flags(BxDFType(0)) {}
	float cosi, coso, pdf, pdfR, tPdf, tPdfR, dAWeight, dARWeight, rr, rrR, d2, padding;
	SWCSpectrum flux;
	BSDF *bsdf;
	BxDFType flags;
	Vector wi, wo;
	Point p;
};

// Bidirectional Method Definitions
void BidirIntegrator::RequestSamples(Sample *sample, const Scene &scene)
{
	lightNumOffset = sample->Add1D(1);
	lightComponentOffset = sample->Add1D(1);
	lightPosOffset = sample->Add2D(1);
	lightDirOffset = sample->Add2D(1);
	samplingCount = lightDirectStrategy->GetSamplingLimit(scene);
	vector<u_int> structure;
	// Direct lighting samples
	for (u_int i = 0; i < samplingCount; ++i) {
		structure.push_back(1);	//light number or portal
		structure.push_back(2);	//light position
	}
	sampleDirectOffset = sample->AddxD(structure, maxEyeDepth);
	structure.clear();
	// Eye subpath samples
	structure.push_back(1);	//continue eye
	structure.push_back(2);	//bsdf sampling for eye path
	structure.push_back(1);	//bsdf component for eye path
	structure.push_back(1); //scattering
	sampleEyeOffset = sample->AddxD(structure, maxEyeDepth);
	structure.clear();
	// Light subpath samples
	structure.push_back(1); //continue light
	structure.push_back(2); //bsdf sampling for light path
	structure.push_back(1); //bsdf component for light path
	structure.push_back(1); //scattering
	sampleLightOffset = sample->AddxD(structure, maxLightDepth);
}
void BidirIntegrator::Preprocess(const RandomGenerator &rng, const Scene &scene)
{
	// Prepare image buffers
	BufferOutputConfig config = BUF_FRAMEBUFFER;
	if (debug)
		config = BufferOutputConfig(config | BUF_STANDALONE);
	BufferType type = BUF_TYPE_PER_PIXEL;
	scene.sampler->GetBufferType(&type);
	eyeBufferId = scene.camera->film->RequestBuffer(type, config, "eye");
	lightBufferId = scene.camera->film->RequestBuffer(BUF_TYPE_PER_SCREEN,
		config, "light");
	lightDirectStrategy->Init(scene);
}

// Weighting of path with regard to alternate methods of obtaining it
static float weightPath(const vector<BidirVertex> &eye, u_int nEye, u_int eyeDepth,
	const vector<BidirVertex> &light, u_int nLight, u_int lightDepth,
	float pdfLightDirect, bool isLightDirect)
{
	// Weight of the current path without direct sampling
	// Used as a reference to extend eye or light subpaths
	// Current path weight is 1
	const float pBase = (nLight == 1 && isLightDirect) ?
		fabsf(light[0].dAWeight) / pdfLightDirect : 1.f;
	float weight = 1.f, p = pBase;
	// If direct lighting and the light isn't unidirectional
	// the path can also be obtained through connection to
	// the light vertex with normal sampling
	if (nLight == 1) {
		if (isLightDirect) {
			if ((light[0].flags & BSDF_SPECULAR) == 0 && lightDepth > 0)
				weight += pBase * pBase;
		} else {
			const float pDirect = pdfLightDirect / fabsf(light[0].dAWeight);
			weight += pDirect * pDirect;
		}
	}
	// Check for direct path when the eye path hit a light
	// The eye path has at least 2 vertices
	// The light vertex cannot be specular otherwise
	// the eye path wouldn't have received any light
	if (nLight == 0 && (eye[nEye - 2].flags & BSDF_SPECULAR) == 0) {
		float pDirect = pdfLightDirect / eye[nEye - 1].dARWeight;
		if (nEye > rrStart + 1)
			pDirect /= eye[nEye - 2].rrR;
		weight += pDirect * pDirect;
	}
	// Find other paths by extending light path toward eye path
	const u_int nLightExt = min(nEye, lightDepth - min(lightDepth, nLight));
	for (u_int i = 1; i <= nLightExt; ++i) {
		// Exit if the path is impossible
		if (!(eye[nEye - i].dARWeight > 0.f && eye[nEye - i].dAWeight > 0.f))
			break;
		// Compute new path relative probability
		p *= eye[nEye - i].dAWeight / eye[nEye - i].dARWeight;
		// Adjust for round robin termination
		if (nEye - i > rrStart)
			p /= eye[nEye - i - 1].rrR;
		if (nLight + i > rrStart + 1) {
			if (i == 1)
				p *= light[nLight - 1].rr;
			else
				p *= eye[nEye - i + 1].rr;
		}
		// The path can only be obtained if none of the vertices
		// is specular
		if ((eye[nEye - i].flags & BSDF_SPECULAR) == 0 &&
			(i == nEye || (eye[nEye - i - 1].flags & BSDF_SPECULAR) == 0))
			weight += p * p;
	}
	// Reinitialize p to search paths in the other direction
	p = pBase;
	// Find other paths by extending eye path toward light path
	u_int nEyeExt = min(nLight, eyeDepth - min(eyeDepth, nEye));
	for (u_int i = 1; i <= nEyeExt; ++i) {
		// Exit if the path is impossible
		if (!(light[nLight - i].dARWeight > 0.f && light[nLight - i].dAWeight > 0.f))
				break;
		// Compute new path relative probability
		p *= light[nLight - i].dARWeight / light[nLight - i].dAWeight;
		// Adjust for round robin termination
		if (nLight - i > rrStart)
			p /= light[nLight - i - 1].rr;
		if (nEye + i > rrStart + 1) {
			if (i == 1)
				p *= eye[nEye - 1].rrR;
			else
				p *= light[nLight - i + 1].rrR;
		}
		// The path can only be obtained if none of the vertices
		// is specular
		if ((light[nLight - i].flags & BSDF_SPECULAR) == 0 &&
			(i == nLight || (light[nLight - i - 1].flags & BSDF_SPECULAR) == 0))
			weight += p * p;
		// Check for direct path
		// Light path has at least 2 vertices here
		// The path can only be obtained if the second vertex
		// is not specular.
		// Even if the light source vertex is specular,
		// the special sampling for direct lighting will get it
		if (i == nLight - 1 && (light[1].flags & BSDF_SPECULAR) == 0) {
			const float pDirect = p * pdfLightDirect / fabsf(light[0].dAWeight);
			weight += pDirect * pDirect;
		}
	}
	return weight;
}

/*
 * Modified fields:
 * eyeV.flags
 * lightV.flags
 * eyeV.rr
 * eyeV.rrR
 * eyeV.dAWeight
 * lightV.rr
 * lightV.rrR
 * lightV.dARWeight
 * light[nLight - 2].dARWeight
 * eyeV.wi
 * eyeV.d2
 */
static bool evalPath(const Scene &scene, const Sample &sample,
	const BidirIntegrator &bidir,
	vector<BidirVertex> &eye, u_int nEye,
	vector<BidirVertex> &light, u_int nLight,
	float pdfLightDirect, bool isLightDirect, float *weight, SWCSpectrum *L)
{
	if (nLight <= 0 || nEye <= 0)
		return false;
	const SpectrumWavelengths &sw(sample.swl);
	const u_int eyeDepth = bidir.maxEyeDepth;
	const u_int lightDepth = bidir.maxLightDepth;
	const float eyeThreshold = bidir.eyeThreshold;
	const float lightThreshold = bidir.lightThreshold;
	*weight = 0.f;
	// Be carefull, eye and light last vertex can be modified here
	// If each path has at least 1 vertex, connect them
	BidirVertex &eyeV(eye[nEye - 1]);
	BidirVertex &lightV(light[nLight - 1]);
	// Check Connectability
	eyeV.flags = BxDFType(~BSDF_SPECULAR);
	const Vector ewi(Normalize(lightV.p - eyeV.p));
	const SWCSpectrum ef(eyeV.bsdf->F(sw, ewi, eyeV.wo, true, eyeV.flags));
	if (ef.Black())
		return false;
	lightV.flags = BxDFType(~BSDF_SPECULAR);
	const Vector lwo(-ewi);
	const SWCSpectrum lf(lightV.bsdf->F(sw, lightV.wi, lwo, false, lightV.flags));
	if (lf.Black())
		return false;
	const float epdfR = eyeV.bsdf->Pdf(sw, eyeV.wo, ewi, eyeV.flags);
	const float lpdf = lightV.bsdf->Pdf(sw, lightV.wi, lwo, lightV.flags);
	float ltPdf = 1.f;
	float etPdfR = 1.f;
	const Volume *volume = eyeV.bsdf->GetVolume(ewi);
	if (!volume)
		volume = lightV.bsdf->GetVolume(lwo);
	const bool eScat = eyeV.bsdf->dgShading.scattered;
	const bool lScat = lightV.bsdf->dgShading.scattered;
	if (!scene.Connect(sample, volume, eScat, lScat, eyeV.p, lightV.p,
		nEye == 1, L, &ltPdf, &etPdfR))
		return false;
	// Prepare eye vertex for connection
	const float ecosi = AbsDot(ewi, eyeV.bsdf->ng);
	const float d2 = DistanceSquared(eyeV.p, lightV.p);
	if (d2 < max(MachineEpsilon::E(eyeV.p), MachineEpsilon::E(lightV.p)))
		return false;
	SWCSpectrum eflux(ef); // No pdf as it is a direct connection
	eflux *= eyeV.flux;
	// Prepare light vertex for connection
	const float lcoso = AbsDot(lwo, lightV.bsdf->ng);
	SWCSpectrum lflux(lf); // No pdf as it is a direct connection
	lflux *= lightV.flux;
	// Connect eye and light vertices
	*L *= eflux;
	*L *= lflux;
	*L /= d2;
	if (L->Black())
		return false;
	// Evaluate factors for eye path weighting
	const float epdf = eyeV.bsdf->Pdf(sw, ewi, eyeV.wo, eyeV.flags);
	if (nEye == 1)
		eyeV.rr = 1.f;
	else
		eyeV.rr = min(1.f, max(lightThreshold, ef.Filter(sw) *
			eyeV.coso / (ecosi * epdf)));
	eyeV.rrR = min(1.f, max(eyeThreshold, ef.Filter(sw) / epdfR));
	eyeV.dAWeight = lpdf * ltPdf / d2;
	if (!eScat)
		eyeV.dAWeight *= ecosi;
	const float eWeight = nEye > 1 ? eye[nEye - 2].dAWeight : 0.f;
	if (nEye > 1) {
		eye[nEye - 2].dAWeight = epdf * eyeV.tPdf / eye[nEye - 2].d2;
		if (!eye[nEye - 2].bsdf->dgShading.scattered)
			eye[nEye - 2].dAWeight *= eye[nEye - 2].cosi;
	}
	// Evaluate factors for light path weighting
	const float lpdfR = lightV.bsdf->Pdf(sw, lwo, lightV.wi, lightV.flags);
	lightV.rr = min(1.f, max(lightThreshold, lf.Filter(sw) / lpdf));
	if (nLight == 1)
		lightV.rrR = 1.f;
	else
		lightV.rrR = min(1.f, max(eyeThreshold, lf.Filter(sw) *
			lightV.cosi / (lcoso * lpdfR)));
	lightV.dARWeight = epdfR * etPdfR / d2;
	if (!lScat)
		lightV.dARWeight *= lcoso;
	if (nLight > 1) {
		light[nLight - 2].dARWeight = lpdfR * lightV.tPdfR /
			light[nLight - 2].d2;
		if (!light[nLight - 2].bsdf->dgShading.scattered)
			light[nLight - 2].dARWeight *= light[nLight - 2].coso;
	}
	const float w = 1.f / weightPath(eye, nEye, eyeDepth, light, nLight,
		lightDepth, pdfLightDirect, isLightDirect);
	*weight = w;
	*L *= w;
	if (nEye > 1)
		eye[nEye - 2].dAWeight = eWeight;
	// return back some eye data
	eyeV.wi = ewi;
	eyeV.d2 = d2;
	return true;
}

static bool eyeConnect(const Sample &sample, const BidirVertex &eye,
	const XYZColor &color, float alpha, float distance, float weight,
	u_int bufferId, u_int groupId)
{
	float xd, yd;
	if (!sample.camera->GetSamplePosition(eye.p, eye.wi, distance,
		&xd, &yd))
		return false;
	sample.AddContribution(xd, yd, color, alpha, distance, weight,
		bufferId, groupId);
	return true;
}

static bool getDirectLight(const Scene &scene, const Sample &sample,
	const BidirIntegrator &bidir, vector<BidirVertex> &eyePath,
	u_int length, const Light *light, float u0, float u1, float portal,
	float directWeight, SWCSpectrum *Ld, float *weight)
{
	vector<BidirVertex> lightPath(1);
	BidirVertex &vE(eyePath[length - 1]);
	BidirVertex &vL(lightPath[0]);
	float ePdfDirect;
	// Sample the chosen light
	if (!light->SampleL(scene, sample, vE.p, u0, u1, portal,
		&vL.bsdf, &vL.dAWeight, &ePdfDirect, Ld))
		return false;
	vL.p = vL.bsdf->dgShading.p;
	vL.wi = Vector(vL.bsdf->dgShading.nn);
	vL.cosi = AbsDot(vL.wi, vL.bsdf->ng);
	vL.dAWeight /= scene.lights.size();
	if (light->IsDeltaLight())
		vL.dAWeight = -vL.dAWeight;
	ePdfDirect *= directWeight;
	vL.flux = SWCSpectrum(1.f / directWeight);
	if (!evalPath(scene, sample, bidir, eyePath, length, lightPath, 1,
		ePdfDirect, true, weight, Ld))
		return false;
	return true;
}

u_int BidirIntegrator::Li(const Scene &scene, const Sample &sample) const
{
	u_int nrContribs = 0;
	// If no eye vertex, return immediately
	//FIXME: this is not necessary if light path can intersect the camera
	// or if direct connection to the camera is implemented
	if (maxEyeDepth <= 0)
		return nrContribs;
	vector<BidirVertex> eyePath(maxEyeDepth), lightPath(maxLightDepth);
	const u_int nGroups = scene.lightGroups.size();
	const u_int numberOfLights = scene.lights.size();
	// If there are no lights, the scene is black
	//FIXME: unless there are emissive volumes
	if (numberOfLights == 0)
		return nrContribs;
	const SpectrumWavelengths &sw(sample.swl);
	vector<SWCSpectrum> vecL(nGroups, SWCSpectrum(0.f));
	vector<float> vecV(nGroups, 0.f);
	float alpha = 1.f;

	// Sample eye subpath origin
	const float posX = sample.camera->IsLensBased() ? sample.lensU : sample.imageX;
	const float posY = sample.camera->IsLensBased() ? sample.lensV : sample.imageY;
	//FIXME: Replace dummy .5f by a sampled value if needed
	//FIXME: the return is not necessary if direct connection to the camera
	// is implemented
	if (!sample.camera->SampleW(sample.arena, sw, scene,
		posX, posY, .5f, &eyePath[0].bsdf, &eyePath[0].dARWeight,
		&eyePath[0].flux))
		return nrContribs;
	BidirVertex &eye0(eyePath[0]);
	// Initialize eye vertex
	eye0.p = eye0.bsdf->dgShading.p;
	eye0.wo = Vector(eye0.bsdf->dgShading.nn);
	eye0.coso = AbsDot(eye0.wo, eye0.bsdf->ng);
	// Light path cannot intersect camera (FIXME)
	eye0.dARWeight = 0.f;
	u_int nEye = 1;

	// Do eye vertex direct lighting
	const float *directData0 = sample.sampler->GetLazyValues(sample,
		sampleDirectOffset, 0);
	for (u_int l = 0; l < samplingCount; ++l) {
		SWCSpectrum Ld;
		float dWeight, dPdf;
		float portal = directData0[0];
		const Light *light = lightDirectStrategy->SampleLight(scene, l,
			&portal, &dPdf);
		if (getDirectLight(scene, sample, *this, eyePath, 1, light,
			directData0[1], directData0[2], portal, dPdf, &Ld,
			&dWeight)) {
			if (light->IsEnvironmental()) {
				if (eyeConnect(sample, eye0,
					XYZColor(sw, Ld), 0.f,
					INFINITY, dWeight,
					lightBufferId,
					light->group))
					++nrContribs;
			} else {
				if (eyeConnect(sample, eye0,
					XYZColor(sw, Ld), 1.f,
					sqrtf(eye0.d2), dWeight,
					lightBufferId,
					light->group))
					++nrContribs;
			}
		}
		directData0 += 3;
	}

	// Sample eye subpath initial direction and finish vertex initialization
	const float lensU = sample.camera->IsLensBased() ? sample.imageX :
		sample.lensU;
	const float lensV = sample.camera->IsLensBased() ? sample.imageY :
		sample.lensV;
	//Jeanphi - Replace dummy .5f by a sampled value if needed
	if (maxEyeDepth > 1) {
		SWCSpectrum f0;
		if (!eye0.bsdf->SampleF(sw,
			eye0.wo, &eye0.wi, lensU, lensV, .5f,
			&f0, &eye0.pdfR, BSDF_ALL, &eye0.flags,
			&eye0.pdf, true))
				return nrContribs;
		eye0.flux *= f0;
		eye0.cosi = AbsDot(eye0.wi, eye0.bsdf->ng);
		eye0.rr = min(1.f, max(lightThreshold,
			f0.Filter(sw) * eye0.coso / eye0.cosi));
		eye0.rrR = min(1.f, max(eyeThreshold, f0.Filter(sw)));
		Ray ray(eye0.p, eye0.wi);
		ray.time = sample.realTime;
		sample.camera->ClampRay(ray);
		Intersection isect;
		eyePath[nEye].flux = eye0.flux;

		// Trace eye subpath and do direct lighting
		const Volume *volume = eye0.bsdf->GetVolume(ray.d);
		bool scattered = eye0.bsdf->dgShading.scattered;
		for (u_int sampleIndex = 1; sampleIndex < maxEyeDepth; ++sampleIndex) {
			const float *data = sample.sampler->GetLazyValues(sample,
				sampleEyeOffset, sampleIndex);
			BidirVertex &v = eyePath[nEye];
			float spdf, spdfR;
			if (!scene.Intersect(sample, volume, scattered, ray, data[4],
				&isect, &v.bsdf, &spdfR, &spdf, &v.flux)) {
				v.flux /= spdfR;
				vector<BidirVertex> path(0);
				// Reinitalize ray origin to the previous
				// non passthrough intersection
				ray.o = eyePath[nEye - 1].p;
				for (u_int lightNumber = 0; lightNumber < scene.lights.size(); ++lightNumber) {
					const Light *light = scene.lights[lightNumber];
					if (!light->IsEnvironmental())
						continue;
					float ePdfDirect;
					SWCSpectrum Le(v.flux);
					// No check for dAWeight > 0
					// in the case of portal, the eye path can hit
					// the light outside portals
					if (!light->Le(scene, sample, ray, &v.bsdf,
						&v.dAWeight, &ePdfDirect, &Le))
						continue;
					v.wo = -ray.d;
					v.flags = BxDFType(~BSDF_SPECULAR);
					v.p = v.bsdf->dgShading.p;
					v.coso = AbsDot(v.wo, v.bsdf->ng);
					eyePath[nEye - 1].d2 =
						DistanceSquared(eyePath[nEye - 1].p, v.p);
					// Evaluate factors for path weighting
					v.dARWeight = eyePath[nEye - 1].pdfR *
						eyePath[nEye - 1].tPdfR * spdfR /
						eyePath[nEye - 1].d2;
					if (!v.bsdf->dgShading.scattered)
						v.dARWeight *= v.coso;
					v.pdf = v.bsdf->Pdf(sw, Vector(v.bsdf->dgShading.nn),
						v.wo);
					// No check for pdf > 0
					// in the case of portal, the eye path can hit
					// the light outside portals
					v.dAWeight /= scene.lights.size();
					ePdfDirect *= lightDirectStrategy->Pdf(scene,
						lightNumber);
					eyePath[nEye - 1].dAWeight = v.pdf * v.tPdf *
						spdf / eyePath[nEye - 1].d2;
					if (!eyePath[nEye - 1].bsdf->dgShading.scattered)
						eyePath[nEye - 1].dAWeight *= eyePath[nEye - 1].cosi;
					vector<BidirVertex> path(0);
					const float w = weightPath(eyePath,
						nEye + 1, maxEyeDepth, path, 0,
						maxLightDepth, ePdfDirect,
						false);
					const u_int eGroup = light->group;
					Le /= w;
					vecV[eGroup] += Le.Filter(sw) / w;
					vecL[eGroup] += Le;
					++nrContribs;
				}
				if (nEye == 1) {
					// Remove directly visible environment
					// to allow compositing
					alpha = 0.f;
					// Tweak intersection distance for Z buffer
					eye0.d2 = INFINITY;
				}
				// End eye path tracing
				break;
			}

			// Initialize new intersection vertex
			scattered = v.bsdf->dgShading.scattered;
			v.flux /= spdfR;
			eyePath[nEye - 1].tPdfR *= spdfR;
			v.tPdf *= spdf;
			v.wo = -ray.d;
			v.p = isect.dg.p;
			v.coso = AbsDot(v.wo, v.bsdf->ng);
			eyePath[nEye - 1].d2 =
				DistanceSquared(eyePath[nEye - 1].p, v.p);
			v.dARWeight = eyePath[nEye - 1].pdfR *
				eyePath[nEye - 1].tPdfR / eyePath[nEye - 1].d2;
			if (!scattered)
				v.dARWeight *= v.coso;
			++nEye;

			// Test intersection with a light source
			if (isect.arealight) {
				BSDF *eBsdf;
				float ePdfDirect;
				// Reinitalize ray origin to the previous
				// non passthrough intersection
				ray.o = eyePath[nEye - 2].p;
				SWCSpectrum Ll(isect.Le(sample, ray, &eBsdf,
					&v.dAWeight, &ePdfDirect));
				if (eBsdf && !Ll.Black()) {
					v.flags = BxDFType(~BSDF_SPECULAR);
					v.pdf = eBsdf->Pdf(sw, Vector(eBsdf->dgShading.nn), v.wo,
						v.flags);
					Ll *= v.flux;
					// Evaluate factors for path weighting
					v.dAWeight /= scene.lights.size();
					ePdfDirect *= lightDirectStrategy->Pdf(scene,
						isect.arealight);
					eyePath[nEye - 2].dAWeight = v.pdf * v.tPdf /
						eyePath[nEye - 2].d2;
					if (!eyePath[nEye - 2].bsdf->dgShading.scattered)
						eyePath[nEye - 2].dAWeight *= eyePath[nEye - 2].cosi;
					vector<BidirVertex> path(0);
					const float w = weightPath(eyePath,
						nEye, maxEyeDepth, path, 0,
						maxLightDepth, ePdfDirect,
						false);
					const u_int eGroup = isect.arealight->group;
					Ll /= w;
					vecV[eGroup] += Ll.Filter(sw) / w;
					vecL[eGroup] += Ll;
					++nrContribs;
				}
			}

			// Break out if path is too long
			if (sampleIndex >= maxEyeDepth)
				break;

			// Do direct lighting
			const float *directData = sample.sampler->GetLazyValues(sample,
				sampleDirectOffset, sampleIndex);
			for (u_int l = 0; l < samplingCount; ++l) {
				SWCSpectrum Ld;
				float dWeight, dPdf;
				float portal = directData[0];
				const Light *directLight =
					lightDirectStrategy->SampleLight(scene, l,
					&portal, &dPdf);
				if (getDirectLight(scene, sample, *this, eyePath, nEye,
					directLight, directData[1], directData[2],
					portal, dPdf, &Ld, &dWeight)) {
					vecL[directLight->group] +=Ld;
					vecV[directLight->group] += Ld.Filter(sw) *
						dWeight;
					++nrContribs;
				}
				directData += 3;
			}

			SWCSpectrum f;
			if (!v.bsdf->SampleF(sw, v.wo, &v.wi, data[1], data[2],
				data[3], &f, &v.pdfR, BSDF_ALL, &v.flags, &v.pdf, true))
				break;

			// Check if the scattering is a passthrough event
			if (v.flags != (BSDF_TRANSMISSION | BSDF_SPECULAR) ||
				!(v.bsdf->Pdf(sw, v.wo, v.wi, BxDFType(BSDF_TRANSMISSION | BSDF_SPECULAR)) > 0.f)) {
				// Possibly terminate path sampling
				if (nEye == maxEyeDepth)
					break;
				eyePath[nEye - 2].dAWeight = v.pdf * v.tPdf /
					eyePath[nEye - 2].d2;
				if (!eyePath[nEye - 2].bsdf->dgShading.scattered)
					eyePath[nEye - 2].dAWeight *= eyePath[nEye - 2].cosi;
				v.cosi = AbsDot(v.wi, v.bsdf->ng);
				v.rr = min(1.f, max(lightThreshold,
					f.Filter(sw) * v.coso / v.cosi));
				v.rrR = min(1.f, max(eyeThreshold, f.Filter(sw)));
				v.flux *= f;
				if (nEye > rrStart) {
					if (v.rrR < data[0])
						break;
					v.flux /= v.rrR;
				}
				eyePath[nEye].flux = v.flux;
			} else {
				--nEye;
				v.flux *= f;
				eyePath[nEye - 1].tPdfR *= v.pdfR;
				v.tPdf *= v.pdf;
				if (sampleIndex + 1 >= maxEyeDepth) {
					eyePath[nEye - 1].rrR = 0.f;
					break;
				}
			}

			// Initialize _ray_ for next segment of path
			ray = Ray(v.p, v.wi);
			ray.time = sample.realTime;
			volume = v.bsdf->GetVolume(ray.d);
		}
	}
	const float d = sqrtf(eye0.d2);

	// Choose light
	const u_int lightNum = min(Floor2UInt(sample.sampler->GetOneD(sample,
		lightNumOffset, 0) * numberOfLights), numberOfLights - 1U);
	const Light *light = scene.lights[lightNum];
	const u_int lightGroup = light->group;
	const float directWeight = lightDirectStrategy->Pdf(scene, lightNum);
	float lightPos[2];
	sample.sampler->GetTwoD(sample, lightPosOffset, 0, lightPos);
	const float component = sample.sampler->GetOneD(sample,
		lightComponentOffset, 0);
	SWCSpectrum Le;

	// Sample light subpath origin
	if (maxLightDepth > 0 && light->SampleL(scene, sample,
		lightPos[0], lightPos[1], component, &lightPath[0].bsdf,
		&lightPath[0].dAWeight, &Le)) {
		BidirVertex &light0(lightPath[0]);
		u_int nLight = 0;
		float lightDirectPdf = 0.f;
		// Initialize light vertex
		light0.p = light0.bsdf->dgShading.p;
		light0.wi = Vector(light0.bsdf->dgShading.nn);
		light0.cosi = AbsDot(light0.wi, light0.bsdf->ng);
		// Give the light point probability for the weighting
		// if the light is not delta
		light0.dAWeight /= numberOfLights;
		// Divide by the light selection Pdf (light position is already
		// accounted for
		Le *= numberOfLights;
		light0.flux = SWCSpectrum(1.f);
		// Trick to tell subsequent functions that the light is delta
		if (light->IsDeltaLight())
			light0.dAWeight = -light0.dAWeight;
		nLight = 1;

		SWCSpectrum &L(vecL[lightGroup]);
		float &variance(vecV[lightGroup]);

		// Connect eye subpath to light vertex
		// Go through all eye vertices
		for (u_int j = 1; j <= nEye; ++j) {
			// Compute direct lighting pdf for first light vertex
			const float directPdf = light->Pdf(eyePath[j - 1].p,
				light0.bsdf->dgShading) * directWeight;
			if (light0.bsdf->NumComponents(BxDFType(~BSDF_SPECULAR)) == 0 || eyePath[j - 1].bsdf->NumComponents(BxDFType(~BSDF_SPECULAR)) == 0)
				continue;
			SWCSpectrum Ll(Le);
			float weight;
			// Save data modified by evalPath
			BidirVertex &vE(eyePath[j - 1]);
			const BxDFType eflags = vE.flags;
			const float err = vE.rr;
			const float errR = vE.rrR;
			const float edAWeight = vE.dAWeight;
			const Vector ewi(vE.wi);
			const float ed2 = vE.d2;
			if (evalPath(scene, sample, *this, eyePath, j,
				lightPath, nLight, directPdf, false, &weight,
				&Ll)) {
				if (j > 1) {
					L += Ll;
					variance += weight * Ll.Filter(sw);
					++nrContribs;
				} else if (eyeConnect(sample, vE,
					XYZColor(sw, Ll),
					light->IsEnvironmental() ? 0.f : 1.f,
					light->IsEnvironmental() ? INFINITY : sqrtf(vE.d2),
					weight, lightBufferId, lightGroup))
					++nrContribs;
			}
			// Restore modified data
			vE.flags = eflags;
			vE.rr = err;
			vE.rrR = errR;
			vE.dAWeight = edAWeight;
			vE.wi = ewi;
			vE.d2 = ed2;
		}

		// Sample light subpath initial direction and
		// finish vertex initialization if needed
		const float *data = sample.sampler->GetLazyValues(sample, sampleLightOffset, 0);
		if (maxLightDepth > 1 && light0.bsdf->SampleF(sw, light0.wi,
			&light0.wo, data[1], data[2], data[3],
			&light0.flux, &light0.pdf, BSDF_ALL, &light0.flags,
			&light0.pdfR)) {
			light0.coso = AbsDot(light0.wo, light0.bsdf->ng);
			light0.rrR = min(1.f, max(eyeThreshold,
				light0.flux.Filter(sw) * light0.cosi /
				light0.coso));
			light0.rr = min(1.f, max(lightThreshold,
				light0.flux.Filter(sw)));
			Ray ray(light0.p, light0.wo);
			ray.time = sample.realTime;
			Intersection isect;
			lightPath[nLight].flux = light0.flux;

			// Trace light subpath and connect to eye subpath
			const Volume *volume = light0.bsdf->GetVolume(ray.d);
			bool scattered = light0.bsdf->dgShading.scattered;
			for (u_int sampleIndex = 1; sampleIndex < maxLightDepth; ++sampleIndex) {
				data = sample.sampler->GetLazyValues(sample,
					sampleLightOffset, sampleIndex);
				BidirVertex &v = lightPath[nLight];
				float spdf, spdfR;
				if (!scene.Intersect(sample, volume, scattered,
					ray, data[4], &isect, &v.bsdf, &spdf,
					&spdfR, &v.flux))
					break;
				scattered = v.bsdf->dgShading.scattered;
				v.tPdfR *= spdfR;
				v.flux /= spdf;
				lightPath[nLight - 1].tPdf *= spdf;
				++nLight;

				// Initialize new intersection vertex
				v.wi = -ray.d;
				v.p = isect.dg.p;
				v.cosi = AbsDot(v.wi, v.bsdf->ng);
				lightPath[nLight - 2].d2 =
					DistanceSquared(lightPath[nLight - 2].p, v.p);
				v.dAWeight = lightPath[nLight - 2].pdf *
					lightPath[nLight - 2].tPdf /
					lightPath[nLight - 2].d2;
				if (!scattered)
					v.dAWeight *= v.cosi;
				// Compute light direct Pdf between
				// the first 2 vertices
				if (nLight == 2)
					lightDirectPdf = light->Pdf(v.p,
						lightPath[0].bsdf->dgShading) *
						directWeight;

				// Connect eye subpath to light subpath
				// Go through all eye vertices
				for (u_int j = 1; j <= nEye; ++j) {
					// Use general direct lighting pdf otherwise
					if (v.bsdf->NumComponents(BxDFType(~BSDF_SPECULAR)) == 0 || eyePath[j - 1].bsdf->NumComponents(BxDFType(~BSDF_SPECULAR)) == 0)
						continue;
					SWCSpectrum Ll(Le);
					float weight;
					// Save data modified by evalPath
					BidirVertex &vE(eyePath[j - 1]);
					const BxDFType eflags = vE.flags;
					const float err = vE.rr;
					const float errR = vE.rrR;
					const float edAWeight = vE.dAWeight;
					const Vector ewi(vE.wi);
					const float ed2 = vE.d2;
					if (evalPath(scene, sample, *this,
						eyePath, j, lightPath, nLight,
						lightDirectPdf, false, &weight, &Ll)) {
						if (j > 1) {
							L += Ll;
							variance += weight * Ll.Filter(sw);
							++nrContribs;
						} else if (eyeConnect(sample,
							eye0, XYZColor(sw, Ll),
							1.f, sqrtf(eye0.d2),
							weight, lightBufferId,
							lightGroup))
							++nrContribs;
					}
					// Restore modified data
					vE.flags = eflags;
					vE.rr = err;
					vE.rrR = errR;
					vE.dAWeight = edAWeight;
					vE.wi = ewi;
					vE.d2 = ed2;
				}

				// Break out if path is too long
				if (sampleIndex >= maxLightDepth)
					break;
				SWCSpectrum f;
				if (!v.bsdf->SampleF(sw, v.wi, &v.wo,
					data[1], data[2], data[3], &f, &v.pdf,
					BSDF_ALL, &v.flags, &v.pdfR))
					break;

				// Check if the scattering is a passthrough event
				if (v.flags != (BSDF_TRANSMISSION | BSDF_SPECULAR) ||
					!(v.bsdf->Pdf(sw, v.wi, v.wo, BxDFType(BSDF_TRANSMISSION | BSDF_SPECULAR)) > 0.f)) {
					// Possibly terminate path sampling
					if (nLight == maxLightDepth)
						break;
					lightPath[nLight - 2].dARWeight =
						v.pdfR * v.tPdfR /
						lightPath[nLight - 2].d2;
					if (!lightPath[nLight - 2].bsdf->dgShading.scattered)
						lightPath[nLight - 2].dARWeight *= lightPath[nLight - 2].coso;
					v.coso = AbsDot(v.wo, v.bsdf->ng);
					v.rrR = min(1.f, max(eyeThreshold,
						f.Filter(sw) * v.cosi / v.coso));
					v.rr = min(1.f, max(lightThreshold,
						f.Filter(sw)));
					v.flux *= f;
					if (nLight > rrStart) {
						if (v.rr < data[0])
							break;
						v.flux /= v.rr;
					}
					lightPath[nLight].flux = v.flux;
				} else {
					--nLight;
					v.flux *= f;
					lightPath[nLight - 1].tPdf *= v.pdf;
					v.tPdfR *= v.pdfR;
					if (sampleIndex + 1 >= maxLightDepth) {
						lightPath[nLight - 1].rr = 0.f;
						break;
					}
				}

				// Initialize _ray_ for next segment of path
				ray = Ray(v.p, v.wo);
				ray.time = sample.realTime;
				volume = v.bsdf->GetVolume(ray.d);
			}
		}
	}

	if (maxEyeDepth > 1) {
		float xl, yl;
		if (!sample.camera->GetSamplePosition(eyePath[0].p, eyePath[0].wi, d, &xl, &yl))
			return nrContribs;
		for (u_int i = 0; i < nGroups; ++i) {
			if (!vecL[i].Black())
				vecV[i] /= vecL[i].Filter(sw);
			XYZColor color(sw, vecL[i]);
			sample.AddContribution(xl, yl,
				color, alpha, d, vecV[i], eyeBufferId, i);
		}
	}
	return nrContribs;
}

//------------------------------------------------------------------------------
// DataParallel integrator BidirPathState code
//
// The general idea here is to trace the light and eye path on the CPU than
// generate in a single shot all shadow rays for direct lighting and
// eye/light path connections. The generated (long) list of rays will
// be traced on the GPUs.
//------------------------------------------------------------------------------

BidirPathState::BidirPathState(const Scene &scene, ContributionBuffer *contribBuffer, RandomGenerator *rng) {
	BidirIntegrator *bidir = (BidirIntegrator *)scene.surfaceIntegrator;

	scene.surfaceIntegrator->RequestSamples(&sample, scene);
	scene.volumeIntegrator->RequestSamples(&sample, scene);
	scene.sampler->InitSample(&sample);
	sample.contribBuffer = contribBuffer;
	sample.camera = scene.camera->Clone();
	sample.realTime = 0.f;
	sample.rng = rng;

	eyePath = new BidirStateVertex[bidir->maxEyeDepth];
	eyePathLength = 0;

	lightPath = new BidirStateVertex[bidir->maxLightDepth];
	lightPathLength = 0;

	Ld = new SWCSpectrum[bidir->maxEyeDepth];
	LdGroup = new u_int[bidir->maxEyeDepth];

	Lc = new SWCSpectrum[bidir->maxEyeDepth * bidir->maxLightDepth];

	LlightPath = new SWCSpectrum[bidir->maxLightDepth];
	distanceLightPath = new float[bidir->maxLightDepth];
	imageXYLightPath = new float[2 * bidir->maxLightDepth];

	const u_int lightGroupCount = scene.lightGroups.size();
	L = new SWCSpectrum[lightGroupCount];
	V = new float[lightGroupCount];

	state = TO_INIT;
}

bool BidirPathState::Init(const Scene &scene) {
	distance = INFINITY;
	const u_int lightGroupCount = scene.lightGroups.size();
	for (u_int i = 0; i < lightGroupCount; ++i) {
		L[i] = 0.f;
		V[i] = 0.f;
	}
	contribCount = 0;

	//--------------------------------------------------------------------------
	// Initialize the Sample
	//--------------------------------------------------------------------------

	// Free BSDF memory from computing image sample value
	sample.arena.FreeAll();

	const bool result = sample.sampler->GetNextSample(&sample);

	// save ray time value
	sample.realTime = sample.camera->GetTime(sample.time);
	// sample camera transformation
	sample.camera->SampleMotion(sample.realTime);

	// Sample new SWC thread wavelengths
	sample.swl.Sample(sample.wavelengths);
	const SpectrumWavelengths &sw(sample.swl);

	//--------------------------------------------------------------------------
	// Build light and eye paths (on the CPU)
	//--------------------------------------------------------------------------

	const u_int numberOfLights = scene.lights.size();

	BidirIntegrator *bidir = (BidirIntegrator *)scene.surfaceIntegrator;
	const u_int maxEyeDepth = bidir->maxEyeDepth;
	const u_int maxLightDepth = bidir->maxLightDepth;
	const float eyeThreshold = bidir->eyeThreshold;
	const float lightThreshold = bidir->lightThreshold;

	float pdf, pdfR;

	//--------------------------------------------------------------------------
	// Build light path (on the CPU)
	//--------------------------------------------------------------------------

	lightPathLength = 0;
	if (maxLightDepth > 0) {
		// Choose light
		const u_int lightNum = min(Floor2UInt(sample.sampler->GetOneD(sample,
			bidir->lightNumOffset, 0) * numberOfLights), numberOfLights - 1U);
		light = scene.lights[lightNum];

		float lightPos[2];
		sample.sampler->GetTwoD(sample, bidir->lightPosOffset, 0, lightPos);
		const float component = sample.sampler->GetOneD(sample,
			bidir->lightComponentOffset, 0);

		// Sample light subpath origin
		BidirStateVertex &light0(lightPath[0]);
		if (light->SampleL(
				scene, sample,
				lightPos[0], lightPos[1], component, &lightPath[0].bsdf,
				&light0.pdf, &Le)) {
			++lightPathLength;

			// Initialize light vertex
			light0.wi = Vector(light0.bsdf->dgShading.nn);
			// ONE light strategy
			light0.pdf /= numberOfLights;

			// pdf of ONE_UNIFORM light sampling strategy
			Le *= numberOfLights;

			if (maxLightDepth > 1) {
				//--------------------------------------------------------------
				// Sample light subpath initial direction and
				// finish vertex initialization if needed
				//--------------------------------------------------------------

				const float *data = sample.sampler->GetLazyValues(sample, bidir->sampleLightOffset, 0);

				if (light0.bsdf->SampleF(sw, light0.wi,
					&light0.wo, data[1], data[2], data[3],
					&light0.throughput, &light0.pdf, BSDF_ALL, &light0.flags,
					&light0.pdfR)) {
					Ray ray(light0.bsdf->dgShading.p, light0.wo);
					ray.time = sample.realTime;
					Intersection isect;

					// Trace light subpath and connect to eye vertex
					const Volume *volume = light0.bsdf->GetVolume(ray.d);
					bool scattered = light0.bsdf->dgShading.scattered;

					u_int nLight = 1;
					lightPath[1].throughput = lightPath[0].throughput;

					for (u_int sampleIndex = 1; sampleIndex < maxLightDepth; ++sampleIndex) {
						data = sample.sampler->GetLazyValues(sample,
							bidir->sampleLightOffset, sampleIndex);

						BidirStateVertex &lightVertex = lightPath[nLight];

						if (!scene.Intersect(sample, volume, scattered,
							ray, data[4], &isect, &lightVertex.bsdf, &pdf,
							&pdfR, &lightVertex.throughput))
							break;

						// Initialize new path vertex
						lightVertex.wi = -ray.d;
						lightVertex.throughput /= pdf;
						scattered = lightVertex.bsdf->dgShading.scattered;

						// Extend the light path
						++lightPathLength;

						SWCSpectrum f;
						if (!lightVertex.bsdf->SampleF(sw, lightVertex.wi, &lightVertex.wo,
							data[1], data[2], data[3], &f, &lightVertex.pdf,
							BSDF_ALL, &lightVertex.flags, &lightVertex.pdfR))
							break;

						// Check if passthrough specular transmission. In this
						// case I don't record the hit point as a path vertex
						if (lightVertex.flags != (BSDF_TRANSMISSION | BSDF_SPECULAR) ||
							!(lightVertex.bsdf->Pdf(sw, lightVertex.wo, lightVertex.wi, BxDFType(BSDF_TRANSMISSION | BSDF_SPECULAR)) > 0.f)) {
							lightVertex.throughput *= f;

							// Russian Roulette, ossibly terminate the path
							const float cosi = AbsDot(lightVertex.wi, lightVertex.bsdf->ng);
							const float coso = AbsDot(lightVertex.wo, lightVertex.bsdf->ng);
							lightVertex.rr = min(1.f, max(lightThreshold,
									f.Filter(sw)));
							lightVertex.rrR = min(1.f, max(eyeThreshold, f.Filter(sw) * cosi / coso));

							if (lightPathLength > rrStart) {
								if (lightVertex.rr < data[0])
									break;
								// increase path contribution
								lightVertex.throughput /= lightVertex.rr;
							}

							// Initialize ray for next segment of path
							ray = Ray(lightVertex.bsdf->dgShading.p, lightVertex.wo);
							ray.time = sample.realTime;
							volume = lightVertex.bsdf->GetVolume(ray.d);

							// Switch to the next vertex
							++nLight;
							if (nLight >= maxLightDepth)
								break;

							lightPath[nLight].throughput =
									lightPath[nLight - 1].throughput;
						} else {
							// It a passthrough specular transmission
							lightPath[nLight - 1].throughput *= f;
							lightPath[nLight].throughput =
									lightPath[nLight - 1].throughput;

							// Roolback the light path
							--lightPathLength;

							if (sampleIndex + 1 >= maxLightDepth) {
								lightPath[nLight - 1].rr = 0.f;
								break;
							}

							// Initialize ray for next segment of path
							ray = Ray(lightVertex.bsdf->dgShading.p, lightVertex.wo);
							ray.time = sample.realTime;
							volume = lightVertex.bsdf->GetVolume(ray.d);
						}
					}
				}
			}
		}
	}

	//LOG(LUX_DEBUG, LUX_NOERROR) << "Light path length: " << lightPathLength;

	//--------------------------------------------------------------------------
	// Build eye path (on the CPU)
	// and add light emitted by surfaces and received by escaped rays
	//--------------------------------------------------------------------------

	eyePathLength = 0;

	// Sample eye subpath origin
	const float posX = sample.camera->IsLensBased() ? sample.lensU : sample.imageX;
	const float posY = sample.camera->IsLensBased() ? sample.lensV : sample.imageY;
	//FIXME: Replace dummy .5f by a sampled value if needed
	//FIXME: the return is not necessary if direct connection to the camera
	// is implemented
	BidirStateVertex &eye0(eyePath[0]);
	if (!sample.camera->SampleW(sample.arena, sw, scene,
		posX, posY, .5f, &eye0.bsdf, &pdf,
		&eye0.throughput))
		return result;

	// Initialize eye vertex
	eye0.wo = Vector(eye0.bsdf->dgShading.nn);

	// Sample eye subpath initial direction and finish vertex initialization
	const float lensU = sample.camera->IsLensBased() ? sample.imageX : sample.lensU;
	const float lensV = sample.camera->IsLensBased() ? sample.imageY : sample.lensV;

	//Jeanphi - Replace dummy .5f by a sampled value if needed
	SWCSpectrum f0;
	if ((maxEyeDepth <= 1) || !eye0.bsdf->SampleF(sw,
		eye0.wo, &eye0.wi, lensU, lensV, .5f,
		&f0, &eye0.pdfR, BSDF_ALL, &eye0.flags,
		&eye0.pdf, true))
			return result;

	eye0.throughput *= f0;

	Ray ray(eye0.bsdf->dgShading.p, eyePath[0].wi);
	ray.time = sample.realTime;
	sample.camera->ClampRay(ray);
	Intersection isect;
	++eyePathLength;

	// Trace eye subpath
	const Volume *volume = eye0.bsdf->GetVolume(ray.d);
	bool scattered = eye0.bsdf->dgShading.scattered;
	bool specularBounce = true;

	u_int nEye = 1;
	eyePath[1].throughput = eyePath[0].throughput;

	for (u_int sampleIndex = 1; sampleIndex < maxEyeDepth; ++sampleIndex) {
		const float *data = sample.sampler->GetLazyValues(sample,
			bidir->sampleEyeOffset, sampleIndex);

		BidirStateVertex &eyeVertex = eyePath[nEye];

		if (!scene.Intersect(sample, volume, scattered, ray, data[4],
			&isect, &eyeVertex.bsdf, &pdfR, &pdf, &eyeVertex.throughput)) {
			if (nEye == 1) {
				alpha = 0.f;
				// Tweak intersection distance for Z buffer
				distance = INFINITY;
			}

			const SWCSpectrum pathThroughput = eyePath[nEye - 1].throughput / pdfR;

			// Dade - now I know ray.maxt and I can call volumeIntegrator
			SWCSpectrum Lv;
			u_int g = scene.volumeIntegrator->Li(scene, ray, sample,
				&Lv, &alpha);
			if (!Lv.Black()) {
				Lv *= pathThroughput;
				L[g] += Lv;
				V[g] += Lv.Filter(sw); // TOFIX
				++contribCount;
			}

			// Stop path sampling since no intersection was found
			// Possibly add horizon in render & reflections
			if (specularBounce) {
				BSDF *ibsdf;
				for (u_int i = 0; i < numberOfLights; ++i) {
					SWCSpectrum Le(pathThroughput);
					float lpdf;
					if (scene.lights[i]->Le(scene, sample,
						ray, &ibsdf, &lpdf, NULL, &Le)) {
						float pathWeight;
						if (bidir->hybridUseMIS) {
							// ONE light strategy
							const float lightPdf =  lpdf / numberOfLights;
							pathWeight = EvalPathMISWeight(eyePath, nEye, lightPdf);
						} else
							pathWeight = EvalPathWeight(eyePath, nEye, ibsdf->NumComponents(BSDF_SPECULAR));

						Le *= pathWeight;
						L[scene.lights[i]->group] += Le;
						V[scene.lights[i]->group] += Le.Filter(sw); // TOFIX
						++contribCount;
					}
				}
			}

			// End eye path tracing
			break;
		}

		if (nEye == 1) {
			alpha = 1.f;
			distance = ray.maxt * ray.d.Length();
		}

		// Possibly add emitted light at path vertex
		if (specularBounce && isect.arealight) {
			BSDF *ibsdf;
			float lpdf;
			SWCSpectrum Le(isect.Le(sample, ray, &ibsdf, &lpdf, NULL));
			if (!Le.Black()) {
				float pathWeight;
				if (bidir->hybridUseMIS) {
					// ONE light strategy
					const float lightPdf =  lpdf / numberOfLights;
					pathWeight = EvalPathMISWeight(eyePath, nEye, lightPdf);
				} else
					pathWeight = EvalPathWeight(eyePath, nEye, ibsdf->NumComponents(BSDF_SPECULAR));

				Le *= eyePath[nEye - 1].throughput * pathWeight;
				L[isect.arealight->group] += Le;
				V[isect.arealight->group] += Le.Filter(sw); // TOFIX
				++contribCount;
			}
		}

		// Initialize new path vertex
		eyeVertex.wo = -ray.d;
		eyeVertex.throughput /= pdfR;
		scattered = eyeVertex.bsdf->dgShading.scattered;

		// Extend the light path
		++eyePathLength;

		SWCSpectrum f;
		if (!eyeVertex.bsdf->SampleF(sw, eyeVertex.wo, &eyeVertex.wi, data[1], data[2],
			data[3], &f, &eyeVertex.pdfR, BSDF_ALL, &eyeVertex.flags, &eyeVertex.pdf, true))
			break;

		// Check if passthrough specular transmission. In this
		// case I don't record the hit point as a path vertex
		if (eyeVertex.flags != (BSDF_TRANSMISSION | BSDF_SPECULAR) ||
			!(eyeVertex.bsdf->Pdf(sw, eyeVertex.wo, eyeVertex.wi, BxDFType(BSDF_TRANSMISSION | BSDF_SPECULAR)) > 0.f)) {
			eyeVertex.throughput *= f;
			specularBounce = ((eyeVertex.flags & BSDF_SPECULAR) != 0);

			// Russian Roulette, ossibly terminate the path
			const float cosi = AbsDot(eyeVertex.wi, eyeVertex.bsdf->ng);
			const float coso = AbsDot(eyeVertex.wo, eyeVertex.bsdf->ng);
			eyeVertex.rr = min(1.f, max(lightThreshold,
					f.Filter(sw) * coso / cosi));
			eyeVertex.rrR = min(1.f, max(eyeThreshold, f.Filter(sw)));

			if (nEye > rrStart) {
				if (eyeVertex.rrR < data[0])
					break;
				// Increase path contribution
				eyeVertex.throughput /= eyeVertex.rrR;
			}

			// Initialize ray for next segment of path
			ray = Ray(eyeVertex.bsdf->dgShading.p, eyeVertex.wi);
			ray.time = sample.realTime;
			volume = eyeVertex.bsdf->GetVolume(ray.d);

			// Switch to the next vertex
			++nEye;
			if (nEye >= maxEyeDepth)
				break;

			eyePath[nEye].throughput =
					eyePath[nEye - 1].throughput;
		} else {
			// It a passthrough specular transmission
			eyePath[nEye - 1].throughput *= f;
			eyePath[nEye].throughput =
					eyePath[nEye - 1].throughput;

			// Rollback the eye path
			--eyePathLength;

			if (sampleIndex + 1 >= maxEyeDepth) {
				eyePath[nEye - 1].rrR = 0.f;
				break;
			}

			// Initialize ray for next segment of path
			ray = Ray(eyeVertex.bsdf->dgShading.p, eyeVertex.wi);
			ray.time = sample.realTime;
			volume = eyeVertex.bsdf->GetVolume(ray.d);
		}
	}

	//LOG(LUX_DEBUG, LUX_NOERROR) << "Eye path length: " << eyePathLength;

	state = TRACE_SHADOWRAYS;

	return result;
}

void BidirPathState::Free(const Scene &scene) {
	delete[] eyePath;
	delete[] lightPath;
	delete[] LlightPath;
	delete[] distanceLightPath;
	delete[] imageXYLightPath;
	delete[] L;
	delete[] V;
	scene.sampler->FreeSample(&sample);
}

const BidirPathState::BidirStateVertex *BidirPathState::GetPathVertex(const u_int index,
		const BidirStateVertex *eyePath, const u_int eyePathVertexCount,
		const BidirStateVertex *lightPath, const u_int lightPathVertexCount) {
	assert (eyePathVertexCount >= 1);
	assert (lightPathVertexCount >= 1);

	if (index < eyePathVertexCount)
		return &eyePath[index];
	else
		return &lightPath[index - eyePathVertexCount];
	
}

//------------------------------------------------------------------------------
// Evaluation of total path with MIS
//------------------------------------------------------------------------------

// This method is used for weight of the path for path tracing and
// direct light sampling
float BidirPathState::EvalPathMISWeight(
		const BidirStateVertex *eyePath,
		const u_int eyePathVertexCount,
		const float lightPdf) {
	// The pdf of the current path
	float pathPdf = 1.f;
	for (u_int i = 0; i < eyePathVertexCount; ++i) {
		pathPdf *= eyePath[i].pdfR;
		if (i > rrStart)
			pathPdf *= eyePath[i].rrR;
	}
	// Power heuristic pdf^2
	pathPdf *= pathPdf;

	// The sum of all pdf for all possible ways to sample this path
	const u_int totalPathVertexCount = eyePathVertexCount + 1;
	float totalPdf = 0.f;

	// Account for: Path tracing
	if (totalPathVertexCount == 2) {
		// Russian Roulette can not be applied
		const float pdf = eyePath[0].pdfR;
		// Power heuristic pdf^2
		totalPdf += pdf * pdf;
	} else if ((totalPathVertexCount >= 3) && (eyePath[eyePathVertexCount - 1].flags & BSDF_SPECULAR)) {
		// I have already this pdf^2
		totalPdf += pathPdf;
	}

	// Account for: Direct light sampling
	if ((totalPathVertexCount >= 3) &&
			!(eyePath[eyePathVertexCount - 1].flags & BSDF_SPECULAR)) {
		float pdf = lightPdf;
		for (u_int i = 0; i < eyePathVertexCount - 1; ++i) {
			pdf *= eyePath[i].pdfR;
			if (i > rrStart)
				pdf *= eyePath[i].rrR;
		}

		// Power heuristic pdf^2
		totalPdf += pdf * pdf;
	}

	// Account for: Eye path and light path connections
	/*if (totalPathVertexCount >= 4) {
		float accPdf = 1.f;

		for (u_int i = 1; i < eyePathVertexCount - 1; ++i) {
			if (!(eyePath[i].flags & BSDF_SPECULAR) && !(eyePath[i + 1].flags & BSDF_SPECULAR)) {
				float pdf = 1.f;

				// Eye path pdf
				for (u_int s = 0; s <= i; ++s) {
					pdf *= eyePath[s].pdfR;
					if (i > rrStart)
						pdf *= eyePath[s].rrR;
				}

				// Light path pdf
				const BidirStateVertex *lightPathVertex = &eyePath[eyePathVertexCount - 1];
				for (u_int t = 0; t <= i; ++t, --lightPathVertex) {
					pdf *= lightPathVertex->pdf;
					if (t > rrStart)
						pdf *= lightPathVertex->rr;
				}

				// Power heuristic pdf^2
				accPdf *= pdf * pdf;
			}
		}

		totalPdf *= accPdf;
	}*/

	// Account for: Light path to eye (i.e. eye[0]) connections
	if ((totalPathVertexCount >= 3) && !(eyePath[1].flags & BSDF_SPECULAR)) {
		float pdf = lightPdf;
		for (u_int t = 0; t < eyePathVertexCount; ++t) {
			pdf *= eyePath[t].pdf;
			if (t > rrStart)
				pdf *= eyePath[t].rr;
		}

		// Power heuristic pdf^2
		totalPdf += pdf * pdf;
	}

	if (totalPdf > 0)
		return pathPdf / totalPdf;
	else
		return 0.f;
}

// This method is used for weight of the path when connecting light path
// vertices directly to the eye
float BidirPathState::EvalPathMISWeight(
		const SpectrumWavelengths &sw,
		const BidirStateVertex &eyeVertex0,
		const float lightStrategyPdf,
		const BidirStateVertex *lightPath,
		const u_int lightPathVertexCount) {
	// The pdf of the current path
	const BidirStateVertex &lastLightVertex(lightPath[lightPathVertexCount - 1]);
	const Vector d(eyeVertex0.bsdf->dgShading.p - lastLightVertex.bsdf->dgShading.p);
	const float connectPdf = eyeVertex0.bsdf->Pdf(sw, eyeVertex0.wo, -d, eyeVertex0.flags) *
			lastLightVertex.bsdf->Pdf(sw, lastLightVertex.wi, d, lastLightVertex.flags);
	float pathPdf = connectPdf;
	
	for (u_int i = 0; i < lightPathVertexCount; ++i) {
		pathPdf *= lightPath[i].pdf;
		if (i > rrStart)
			pathPdf *= lightPath[i].rr;
	}
	// Power heuristic pdf^2
	pathPdf *= pathPdf;

	// The sum of all pdf for all possible ways to sample this path
	const u_int totalPathVertexCount = 1 + lightPathVertexCount;
	float totalPdf = 0.f;

	if (totalPathVertexCount == 2) {
		// Russian Roulette can not be applied
		// Power heuristic pdf^2
		totalPdf += connectPdf *  connectPdf;
	} else if ((totalPathVertexCount >= 3) && (lightPath[1].flags & BSDF_SPECULAR)) {
		float pdf = connectPdf;
		for (u_int s = 0; s < lightPathVertexCount; ++s) {
			pdf *= lightPath[s].pdfR;
			if (s > rrStart)
				pdf *= lightPath[s].rrR;
		}
		
		// Power heuristic pdf^2
		totalPdf += pdf * pdf;
	}

	// Account for: Direct light sampling
	if ((totalPathVertexCount >= 3) &&
			!(lightPath[1].flags & BSDF_SPECULAR)) {
		float pdf = connectPdf * lightStrategyPdf;
		for (u_int s = 0; s < lightPathVertexCount - 1; ++s) {
			pdf *= lightPath[s].pdfR;
			if (s > rrStart)
				pdf *= lightPath[s].rrR;
		}

		// Power heuristic pdf^2
		totalPdf += pathPdf;
	}
		
	// Account for: Eye path and light path connections
	/*if (totalPathVertexCount >= 4) {
		for (u_int i = 1; i < lightPathVertexCount - 1; ++i) {
			if (!(lightPath[i].flags & BSDF_SPECULAR) && !(lightPath[i + 1].flags & BSDF_SPECULAR))
				++samplingWays;
		}
	}*/

	// Account for: Light path to eye (i.e. eye[0]) connections
	if ((totalPathVertexCount >= 3) && !(lightPath[lightPathVertexCount - 1].flags & BSDF_SPECULAR)) {
		// I have already this pdf^2
		totalPdf += pathPdf;
	}

	if (totalPdf > 0)
		return pathPdf / totalPdf;
	else
		return 0.f;
}

//------------------------------------------------------------------------------
// Evaluation of total path weight by averaging
//------------------------------------------------------------------------------

// This method is used for weight of the path for path tracing and
// direct light sampling
float BidirPathState::EvalPathWeight(const BidirStateVertex *eyePath,
		const u_int eyePathVertexCount, const bool isLightVertexSpecular) {
	const u_int totalPathVertexCount = eyePathVertexCount + 1;

	u_int samplingWays = 0;

	// Account for: Path tracing
	if ((totalPathVertexCount == 2) ||
			((totalPathVertexCount >= 3) && (eyePath[eyePathVertexCount - 1].flags & BSDF_SPECULAR)))
		++samplingWays;

	// Account for: Direct light sampling
	if ((totalPathVertexCount >= 3) &&
			!(eyePath[eyePathVertexCount - 1].flags & BSDF_SPECULAR))
		++samplingWays;

	// Account for: Eye path and light path connections
	if (totalPathVertexCount >= 4) {
		for (u_int i = 1; i < eyePathVertexCount - 1; ++i) {
			if (!(eyePath[i].flags & BSDF_SPECULAR) && !(eyePath[i + 1].flags & BSDF_SPECULAR))
				++samplingWays;
		}
	}

	// Account for: Light path to eye (i.e. eye[0]) connections
	if ((totalPathVertexCount >= 3) && !(eyePath[1].flags & BSDF_SPECULAR))
		++samplingWays;

	if (samplingWays > 0)
		return 1.f / samplingWays;
	else
		return 0.f;
}

float BidirPathState::EvalPathWeight(
		const BidirStateVertex *eyePath, const u_int eyePathVertexCount,
		const BidirStateVertex *lightPath, const u_int lightPathVertexCount) {
	const u_int totalPathVertexCount = eyePathVertexCount + lightPathVertexCount;

	u_int samplingWays = 0;

	// Account for: Path tracing
	if ((totalPathVertexCount == 2) ||
			((totalPathVertexCount >= 3) &&
			(GetPathVertex(totalPathVertexCount - 2, eyePath, eyePathVertexCount, lightPath, lightPathVertexCount)->flags & BSDF_SPECULAR)))
		++samplingWays;

	// Account for: Direct light sampling
	if ((totalPathVertexCount >= 3) &&
			!(GetPathVertex(totalPathVertexCount - 2, eyePath, eyePathVertexCount, lightPath, lightPathVertexCount)->flags & BSDF_SPECULAR))
		++samplingWays;

	// Account for: Eye path and light path connections
	if (totalPathVertexCount >= 4) {
		for (u_int i = 1; i < totalPathVertexCount - 2; ++i) {
			if (!(GetPathVertex(i, eyePath, eyePathVertexCount, lightPath, lightPathVertexCount)->flags & BSDF_SPECULAR) &&
					!(GetPathVertex(i + 1, eyePath, eyePathVertexCount, lightPath, lightPathVertexCount)->flags & BSDF_SPECULAR))
				++samplingWays;
		}
	}

	// Account for: Light path to eye (i.e. eye[0]) connections
	if ((totalPathVertexCount >= 3) && !(eyePath[1].flags & BSDF_SPECULAR))
		++samplingWays;


	if (samplingWays > 0)
		return 1.f / samplingWays;
	else
		return 0.f;
}

// This method is used for weight of the path when connecting light path
// vertices directly to the eye
float BidirPathState::EvalPathWeight(const bool isEyeVertexSpecular,
		const BidirStateVertex *lightPath, const u_int lightPathVertexCount) {
	const u_int totalPathVertexCount = 1 + lightPathVertexCount;

	u_int samplingWays = 0;

	// Account for: Path tracing
	if ((totalPathVertexCount == 2) ||
			((totalPathVertexCount >= 3) && (lightPath[1].flags & BSDF_SPECULAR)))
		++samplingWays;

	// Account for: Direct light sampling
	if ((totalPathVertexCount >= 3) &&
			!(lightPath[1].flags & BSDF_SPECULAR))
		++samplingWays;

	// Account for: Eye path and light path connections
	if (totalPathVertexCount >= 4) {
		for (u_int i = 1; i < lightPathVertexCount - 1; ++i) {
			if (!(lightPath[i].flags & BSDF_SPECULAR) && !(lightPath[i + 1].flags & BSDF_SPECULAR))
				++samplingWays;
		}
	}

	// Account for: Light path to eye (i.e. eye[0]) connections
	if ((totalPathVertexCount >= 3) && !(lightPath[lightPathVertexCount - 1].flags & BSDF_SPECULAR))
		++samplingWays;

	if (samplingWays > 0)
		return 1.f / samplingWays;
	else
		return 0.f;
}

void BidirPathState::Connect(const Scene &scene, luxrays::RayBuffer *rayBuffer,
		u_int &rayIndex, const BSDF *bsdf, SWCSpectrum *Li,
		SWCSpectrum *Lresult, float *Vresult) {
	if (!Li->Black()) {
		const SpectrumWavelengths &sw(sample.swl);

		const size_t ri = raysIndexStart + rayIndex;
		const luxrays::Ray &firstShadowRay = (rayBuffer->GetRayBuffer())[ri];
		// A pointer trick
		const Point *ro = (const Point *)(&firstShadowRay.o);
		// A pointer trick
		const Vector *rd = (const Vector *)(&firstShadowRay.d);
		Ray shadowRay(*ro, *rd, firstShadowRay.mint, firstShadowRay.maxt, sample.realTime);
		const luxrays::RayHit &shadowRayHit = *(rayBuffer->GetRayHit(ri));
		const Volume *vol = bsdf->GetVolume(*rd);

		int result = scene.Connect(sample, &vol,
					bsdf->dgShading.scattered, false,
					shadowRay, shadowRayHit,
					Li, NULL, NULL);

		if (result == 1) {
			*Lresult += *Li;
			if (Vresult)
				*Vresult += Li->Filter(sw); // TOFIX
			++contribCount;
		} else if (result == 0) {
			// I have to continue to trace the ray. I do on the CPU,
			// it is a lot simpler (and faster in most cases).

			Intersection lightIsect;
			BSDF *ibsdf;
			const Volume *volume = bsdf->GetVolume(shadowRay.d);
			for (u_int n = 0; n < passThroughLimit; ++n) {
				if (!scene.Intersect(sample, volume,
					bsdf->dgShading.scattered, shadowRay, 1.f,
					&lightIsect, &ibsdf, NULL, NULL, Li)) {
					*Lresult += *Li;
					if (Vresult)
						*Vresult += Li->Filter(sw); // TOFIX

					++contribCount;
					break;
				} else {
					if (ibsdf->Pdf(sample.swl, -shadowRay.d, shadowRay.d,
						BxDFType(BSDF_TRANSMISSION | BSDF_SPECULAR)) <= 0.f)
						break;
					shadowRay.mint = shadowRay.maxt + MachineEpsilon::E(shadowRay.maxt);
					shadowRay.maxt = firstShadowRay.maxt;
					*Li *= ibsdf->F(sample.swl, -shadowRay.d, shadowRay.d, true, BxDFType(BSDF_TRANSMISSION | BSDF_SPECULAR));
					volume = ibsdf->GetVolume(shadowRay.d);
				}
			}
		}

		++rayIndex;
	}
}

void BidirPathState::Terminate(const Scene &scene,
		const u_int eyeBufferId, const u_int lightBufferId) {
	// Add the eye buffer samples
	const u_int lightGroupCount = scene.lightGroups.size();
	for (u_int i = 0; i < lightGroupCount; ++i) {
		if (!L[i].Black())
			V[i] /= L[i].Filter(sample.swl);

		sample.AddContribution(sample.imageX, sample.imageY,
			XYZColor(sample.swl, L[i]), alpha, distance,
			V[i], eyeBufferId, i);
	}

	// Add the light buffer samples
	const bool lightAlpha = light->IsEnvironmental();
	for (u_int s = 1; s < lightPathLength; ++s) {
		if (LlightPath[s].Black())
			continue;

		const u_int index = 2 * s;
		const float xd = imageXYLightPath[index];
		const float yd = imageXYLightPath[index + 1];

		sample.AddContribution(xd , yd,
			XYZColor(sample.swl, LlightPath[s]),
			lightAlpha,
			light->IsEnvironmental() ? INFINITY : distanceLightPath[s],
			0.f, // TOFIX
			lightBufferId, light->group);
	}

	sample.sampler->AddSample(sample);

	state = TERMINATE;
}

//------------------------------------------------------------------------------
// DataParallel integrator BidirIntegrator code
//------------------------------------------------------------------------------

SurfaceIntegratorState *BidirIntegrator::NewState(const Scene &scene,
		ContributionBuffer *contribBuffer, RandomGenerator *rng) {
	return new BidirPathState(scene, contribBuffer, rng);
}

static void InitRay(luxrays::Ray *r, const Point &o, const Vector &d,
		const float mint, const float maxt) {
	// LuxRays Point/Vector data type are different from LuxRender Point/Vector
	r->o.x = o.x;
	r->o.y = o.y;
	r->o.z = o.z;
	r->d.x = d.x;
	r->d.y = d.y;
	r->d.z = d.z;
	r->mint = mint;
	r->maxt = maxt;
}

bool BidirIntegrator::GenerateRays(const Scene &scene,
		SurfaceIntegratorState *s, luxrays::RayBuffer *rayBuffer) {
	BidirPathState *bidirState = (BidirPathState *)s;

	const u_int eyePathLength = bidirState->eyePathLength;
	const u_int lightPathLength = bidirState->lightPathLength ;

	if ((eyePathLength == 0) /*|| (lightPathLength == 0)*/) {
		// TODO
		return true;
	}

	// Generate the rays
	bidirState->raysCount = 0;
	// Direct light sampling rays + eye/light connection rays
	luxrays::Ray *shadowRays = (luxrays::Ray *)alloca(sizeof(luxrays::Ray) *
			((maxEyeDepth - 1) + // Direct light sampling rays
			(maxEyeDepth - 1) * (maxLightDepth - 1) + // Eye/light connection rays
			(maxLightDepth - 1) // Light path vertex to the eye connection rays
			));
	const Sample &sample(bidirState->sample);
	const SpectrumWavelengths &sw(bidirState->sample.swl);

	//--------------------------------------------------------------------------
	// Direct light sampling rays
	//--------------------------------------------------------------------------

	const u_int nLights = scene.lights.size();
	if (nLights > 0) {
		// ONE light strategy
		const float lightSelectionInvPdf = nLights;

		for (u_int t = 1; t < eyePathLength; ++t) {
			const float *sampleData = sample.sampler->GetLazyValues(sample,
				sampleDirectOffset, t);

			BidirPathState::BidirStateVertex &eyePath = bidirState->eyePath[t];
			SWCSpectrum &stateLd(bidirState->Ld[t]);
			stateLd = SWCSpectrum(0.f);

			if (eyePath.bsdf->NumComponents(BxDFType(~BSDF_SPECULAR)) == 0)
				continue;

			float portal = sampleData[2] * nLights;
			const u_int lightDirectNumber = min(Floor2UInt(portal), nLights - 1U);
			const Light *light = scene.lights[lightDirectNumber];
			portal -= lightDirectNumber;

			const Point &p = eyePath.bsdf->dgShading.p;

			// Trace a shadow ray by sampling the light source
			float lightPdf;
			SWCSpectrum Li;
			BSDF *lightBsdf;
			if (!light->SampleL(scene, sample, p, sampleData[0], sampleData[1], portal,
				&lightBsdf, NULL, &lightPdf, &Li))
				continue;

			Li *= lightSelectionInvPdf; // ONE_UNIFORM Strategy Pdf

			const Point &pL(lightBsdf->dgShading.p);
			const Vector wi0(pL - p);
			const float d2 = wi0.LengthSquared();
			const float length = sqrtf(d2);
			const Vector wi(wi0 / length);

			Vector wo(eyePath.wi);

			Li *= lightBsdf->F(sw, Vector(lightBsdf->dgShading.nn), -wi, false);
			Li *= eyePath.bsdf->F(sw, wi, wo, true);

			if (Li.Black())
				continue;

			const float shadowRayEpsilon = max(MachineEpsilon::E(pL),
				MachineEpsilon::E(length));

			if (length * .5f <= shadowRayEpsilon)
				continue;

			// Store light's contribution
			float pathWeight;
			if (hybridUseMIS)
				pathWeight = BidirPathState::EvalPathWeight(
						bidirState->eyePath, t + 1, lightPdf / lightSelectionInvPdf);
			else
				pathWeight = BidirPathState::EvalPathWeight(
						bidirState->eyePath, t + 1, lightBsdf->NumComponents(BSDF_SPECULAR));

			SWCSpectrum Ld = (eyePath.throughput * Li * pathWeight) / d2;
			if (Ld.Black())
				continue;

			stateLd = Ld;
			bidirState->LdGroup[t] = light->group;

			const float maxt = length - shadowRayEpsilon;
			InitRay(&shadowRays[bidirState->raysCount],p ,wi, shadowRayEpsilon, maxt);
			++(bidirState->raysCount);
		}
	}

	//--------------------------------------------------------------------------
	// Eye and Light path connection rays
	//--------------------------------------------------------------------------

	// For each eye path vertex
	for (u_int t = 1; t < eyePathLength; ++t) {
		// For each light path vertex
		for (u_int s = 1; s < lightPathLength; ++s) {
			BidirPathState::BidirStateVertex &eyePath = bidirState->eyePath[t];
			BidirPathState::BidirStateVertex &lightPath = bidirState->lightPath[s];

			SWCSpectrum &stateLc(bidirState->Lc[t + s * eyePathLength]);
			stateLc = SWCSpectrum(0.f);

			if ((eyePath.bsdf->NumComponents(BxDFType(~BSDF_SPECULAR)) == 0) ||
				(lightPath.bsdf->NumComponents(BxDFType(~BSDF_SPECULAR)) == 0))
				continue;

			const Point &p = eyePath.bsdf->dgShading.p;
			const Point &pL = lightPath.bsdf->dgShading.p;
			Vector d(pL - p);
			const float d2 = d.LengthSquared();
			const float length = sqrtf(d2);
			d /= length;

			const float shadowRayEpsilon = max(MachineEpsilon::E(p),
					MachineEpsilon::E(length));

			if (length * .5f <= shadowRayEpsilon)
				continue;

			const SWCSpectrum ef(eyePath.bsdf->F(sw, d, eyePath.wo, true, eyePath.flags));
			if (ef.Black())
				continue;

			const SWCSpectrum lf(lightPath.bsdf->F(sw, lightPath.wi, -d, false, lightPath.flags));
			if (lf.Black())
				continue;

			float pathWeight;
			if (hybridUseMIS)
				pathWeight = 0.f;
			else
				pathWeight = BidirPathState::EvalPathWeight(
						bidirState->eyePath, t + 1, bidirState->lightPath, s + 1);

			SWCSpectrum Lc = (eyePath.throughput * ef * lf * lightPath.throughput * bidirState->Le * pathWeight) / d2;
			if (Lc.Black())
				continue;

			stateLc = Lc;

			const float maxt = length - shadowRayEpsilon;
			InitRay(&shadowRays[bidirState->raysCount],p ,d, shadowRayEpsilon, maxt);
			++(bidirState->raysCount);
		}
	}

	//--------------------------------------------------------------------------
	// Light path vertex to the eye connection rays
	//--------------------------------------------------------------------------

	BidirPathState::BidirStateVertex &eye0 = bidirState->eyePath[0];
	if (eye0.bsdf->NumComponents(BxDFType(~BSDF_SPECULAR)) == 0) {
		for (u_int s = 1; s < lightPathLength; ++s)
			bidirState->LlightPath[s] = SWCSpectrum(0.f);
	} else {
		const Point &p = eye0.bsdf->dgShading.p;
		const float lightStrategyPdf = 1.f / nLights;

		// For each light path vertex
		for (u_int s = 1; s < lightPathLength; ++s) {
			BidirPathState::BidirStateVertex &lightPath = bidirState->lightPath[s];

			SWCSpectrum &stateLlightPath(bidirState->LlightPath[s]);
			stateLlightPath = SWCSpectrum(0.f);

			if (lightPath.bsdf->NumComponents(BxDFType(~BSDF_SPECULAR)) == 0)
				continue;

			Vector d = lightPath.bsdf->dgShading.p - p;
			const float d2 = d.LengthSquared();
			const float length = sqrtf(d2);
			d /= length;

			if (!sample.camera->GetSamplePosition(p, d, length,
				&bidirState->imageXYLightPath[2 * s], &bidirState->imageXYLightPath[2 * s + 1]))
				continue;

			const SWCSpectrum ef(eye0.bsdf->F(sw, d, eye0.wo, true, eye0.flags));
			if (ef.Black())
				continue;

			const SWCSpectrum lf(lightPath.bsdf->F(sw, lightPath.wi, -d, false, lightPath.flags));
			if (lf.Black())
				continue;

			const float shadowRayEpsilon = max(MachineEpsilon::E(p),
					MachineEpsilon::E(length));

			if (length * .5f <= shadowRayEpsilon)
				continue;

			// Store light's contribution
			float pathWeight;
			if (hybridUseMIS)
				pathWeight = BidirPathState::EvalPathMISWeight(sw, eye0, lightStrategyPdf,
						bidirState->lightPath, s + 1);
			else
				pathWeight = BidirPathState::EvalPathWeight(
						eye0.bsdf->NumComponents(BSDF_SPECULAR), bidirState->lightPath, s + 1);

			SWCSpectrum LlightPath = (eye0.throughput * ef * lf * lightPath.throughput * bidirState->Le * pathWeight) / d2;
			if (LlightPath.Black())
				continue;

			stateLlightPath = LlightPath;

			const float maxt = length - shadowRayEpsilon;
			InitRay(&shadowRays[bidirState->raysCount],p ,d, shadowRayEpsilon, maxt);
			++(bidirState->raysCount);
		}
	}

	//LOG(LUX_DEBUG, LUX_NOERROR) << "Generated rays: " << bidirState->raysCount;

	// Check if there is enough space in the RayBuffer to store all shadow rays
	if (bidirState->raysCount > rayBuffer->LeftSpace())
		return false;

	// Add all shadow rays to the  RayBuffer
	bidirState->raysIndexStart = rayBuffer->AddRays(shadowRays, bidirState->raysCount);

	return true;
}

bool BidirIntegrator::NextState(const Scene &scene, SurfaceIntegratorState *s,
		luxrays::RayBuffer *rayBuffer, u_int *nrContribs) {
	BidirPathState *bidirState = (BidirPathState *)s;

	u_int rayIndex = 0;

	//--------------------------------------------------------------------------
	// Direct light sampling rays
	//--------------------------------------------------------------------------

	if (scene.lights.size() > 0) {
		for (u_int t = 1; t < bidirState->eyePathLength; ++t) {
			const u_int lightGroup = bidirState->LdGroup[t];
			bidirState->Connect(scene, rayBuffer, rayIndex, bidirState->eyePath[t].bsdf,
					&bidirState->Ld[t], &bidirState->L[lightGroup], &bidirState->V[lightGroup]);
		}
	}

	//--------------------------------------------------------------------------
	// Eye and Light path connections
	//--------------------------------------------------------------------------

	// For each eye path vertex
	for (u_int t = 1; t < bidirState->eyePathLength; ++t) {
		// For each light path vertex
		const u_int lightGroup = bidirState->light->group;
		for (u_int s = 1; s < bidirState->lightPathLength; ++s) {
			SWCSpectrum &Lc(bidirState->Lc[t + s * bidirState->eyePathLength]);

			bidirState->Connect(scene, rayBuffer, rayIndex, bidirState->eyePath[t].bsdf,
					&Lc, &bidirState->L[lightGroup], &bidirState->V[lightGroup]);
		}
	}

	//--------------------------------------------------------------------------
	// Light paths to the eye connections
	//--------------------------------------------------------------------------

	// For each light path vertex
	for (u_int s = 1; s < bidirState->lightPathLength; ++s) {
		SWCSpectrum &LlightPath(bidirState->LlightPath[s]);
		SWCSpectrum Ll(0.f);

		bidirState->Connect(scene, rayBuffer, rayIndex, bidirState->eyePath[0].bsdf,
					&LlightPath, &Ll, NULL);
		LlightPath = Ll;
	}
	/*for (u_int s = 1; s < bidirState->lightPathLength; ++s) {
		SWCSpectrum &LlightPath(bidirState->LlightPath[s]);
		SWCSpectrum Ll(0.f);
		LlightPath = Ll;
	}*/

	bidirState->Terminate(scene, eyeBufferId, lightBufferId);

	*nrContribs = bidirState->contribCount;

	return true;
}

//------------------------------------------------------------------------------
// Integrator parsing code
//------------------------------------------------------------------------------

SurfaceIntegrator* BidirIntegrator::CreateSurfaceIntegrator(const ParamSet &params)
{
	int eyeDepth = params.FindOneInt("eyedepth", 8);
	int lightDepth = params.FindOneInt("lightdepth", 8);
	float eyeThreshold = params.FindOneFloat("eyerrthreshold", 0.f);
	float lightThreshold = params.FindOneFloat("lightrrthreshold", 0.f);
	LightsSamplingStrategy *lds = LightsSamplingStrategy::Create(params);
	// This parameter works only with hybrid BiDir, it will be removed
	// once MIS code is complete
	bool mis = params.FindOneBool("hybridusemis", false);
	bool debug = params.FindOneBool("debug", false);

	return new BidirIntegrator(max(eyeDepth, 0), max(lightDepth, 0),
		eyeThreshold, lightThreshold, lds, mis, debug);
}

static DynamicLoader::RegisterSurfaceIntegrator<BidirIntegrator> r("bidirectional");
