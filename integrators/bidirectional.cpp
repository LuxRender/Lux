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
	if (lightStrategy == SAMPLE_AUTOMATIC) {
		if (scene.lights.size() > 5)
			lightStrategy = SAMPLE_ONE_UNIFORM;
		else
			lightStrategy = SAMPLE_ALL_UNIFORM;
	}

	lightNumOffset = sample->Add1D(1);
	lightComponentOffset = sample->Add1D(1);
	lightPosOffset = sample->Add2D(1);
	lightDirOffset = sample->Add2D(1);
	vector<u_int> structure;
	// Direct lighting samples
	for (u_int i = 0; i < scene.lights.size(); ++i) {
		structure.push_back(2);	//light position
		structure.push_back(1);	//light number or portal
		if (lightStrategy == SAMPLE_ONE_UNIFORM)
			break;
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
	const float lWeight = nLight > 1 ? light[nLight - 2].dARWeight : 0.f;
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
	if (nLight > 1)
		light[nLight - 2].dARWeight = lWeight;
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
	vL.wi = Vector(vL.bsdf->nn);
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
	const float directWeight = (lightStrategy == SAMPLE_ONE_UNIFORM) ?
		1.f / numberOfLights : 1.f;
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
	eye0.wo = Vector(eye0.bsdf->nn);
	eye0.coso = AbsDot(eye0.wo, eye0.bsdf->ng);
	// Light path cannot intersect camera (FIXME)
	eye0.dARWeight = 0.f;

	// Do eye vertex direct lighting
	const float *directData0 = scene.sampler->GetLazyValues(sample,
		sampleDirectOffset, 0);
	switch (lightStrategy) {
		case SAMPLE_ONE_UNIFORM: {
			SWCSpectrum Ld;
			float dWeight;
			float portal = directData0[2] * numberOfLights;
			const u_int lightDirectNumber = min(Floor2UInt(portal),
				numberOfLights - 1U);
			const Light *light = scene.lights[lightDirectNumber];
			portal -= lightDirectNumber;
			if (getDirectLight(scene, sample, *this, eyePath, 1,
				light, directData0[0], directData0[1], portal,
				directWeight, &Ld, &dWeight)) {
				if (light->IsEnvironmental()) {
					if (eyeConnect(sample, eye0,
						XYZColor(sw, Ld), 0.f,
						INFINITY, dWeight,
						lightBufferId, light->group))
						++nrContribs;
				} else {
					if (eyeConnect(sample, eye0,
						XYZColor(sw, Ld), 1.f,
						sqrtf(eye0.d2), dWeight,
						lightBufferId, light->group))
						++nrContribs;
				}
			}
			break;
		}
		case SAMPLE_ALL_UNIFORM: {
			SWCSpectrum Ld;
			float dWeight;
			for (u_int l = 0; l < scene.lights.size(); ++l) {
				const Light *light = scene.lights[l];
				if (getDirectLight(scene, sample, *this,
					eyePath, 1, light,
					directData0[0], directData0[1],
					directData0[2], directWeight,
					&Ld, &dWeight)) {
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
			break;
		}
		default:
			break;
	}

	// Choose light
	const u_int lightNum = min(Floor2UInt(scene.sampler->GetOneD(sample,
		lightNumOffset, 0) * numberOfLights), numberOfLights - 1U);
	const Light *light = scene.lights[lightNum];
	const u_int lightGroup = light->group;
	float lightPos[2];
	scene.sampler->GetTwoD(sample, lightPosOffset, 0, lightPos);
	const float component = scene.sampler->GetOneD(sample,
		lightComponentOffset, 0);
	SWCSpectrum Le;

	// Sample light subpath origin
	u_int nLight = 0;
	float lightDirectPdf = 0.f;
	if (maxLightDepth > 0 && light->SampleL(scene, sample,
		lightPos[0], lightPos[1], component, &lightPath[0].bsdf,
		&lightPath[0].dAWeight, &Le)) {
		BidirVertex &light0(lightPath[0]);
		// Initialize light vertex
		light0.p = light0.bsdf->dgShading.p;
		light0.wi = Vector(light0.bsdf->nn);
		light0.cosi = AbsDot(light0.wi, light0.bsdf->ng);
		// Give the light point probability for the weighting
		// if the light is not delta
		light0.dAWeight /= numberOfLights;
		// Divide by Pdf because this value isn't used when the eye
		// ray hits a light source, only for light paths
		Le *= numberOfLights;
		// Trick to tell subsequent functions that the light is delta
		if (light->IsDeltaLight())
			light0.dAWeight = -light0.dAWeight;
		nLight = 1;

		// Connect light vertex to eye vertex
		// Compute direct lighting pdf for first light vertex
		const float directPdf = light->Pdf(eye0.p, light0.p,
			light0.bsdf->ng) * directWeight;
		SWCSpectrum Ll(Le);
		float weight;
		if (evalPath(scene, sample, *this, eyePath, 1, lightPath,
			nLight, directPdf, false, &weight, &Ll) &&
			eyeConnect(sample, eye0, XYZColor(sw, Ll),
			light->IsEnvironmental() ? 0.f : 1.f,
			light->IsEnvironmental() ? INFINITY : sqrtf(eye0.d2),
			weight, lightBufferId, lightGroup))
			++nrContribs;

		// Sample light subpath initial direction and
		// finish vertex initialization if needed
		const float *data = scene.sampler->GetLazyValues(sample, sampleLightOffset, 0);
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

			// Trace light subpath and connect to eye vertex
			const Volume *volume = light0.bsdf->GetVolume(ray.d);
			bool scattered = light0.bsdf->dgShading.scattered;
			for (u_int sampleIndex = 1; sampleIndex < maxLightDepth; ++sampleIndex) {
				data = scene.sampler->GetLazyValues(sample,
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
						lightPath[0].p,
						lightPath[0].bsdf->ng) * directWeight;

				// Connect light subpath to eye vertex
				if (v.bsdf->NumComponents(BxDFType(~BSDF_SPECULAR)) > 0 && eye0.bsdf->NumComponents(BxDFType(~BSDF_SPECULAR)) > 0) {
					SWCSpectrum Ll(Le);
					float weight;
					if (evalPath(scene, sample, *this,
						eyePath, 1, lightPath, nLight,
						lightDirectPdf, false,
						&weight, &Ll) &&
						eyeConnect(sample, eye0,
						XYZColor(sw, Ll), 1.f,
						sqrtf(eye0.d2), weight,
						lightBufferId, lightGroup))
						++nrContribs;
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

	// Sample eye subpath initial direction and finish vertex initialization
	const float lensU = sample.camera->IsLensBased() ? sample.imageX :
		sample.lensU;
	const float lensV = sample.camera->IsLensBased() ? sample.imageY :
		sample.lensV;
	//Jeanphi - Replace dummy .5f by a sampled value if needed
	SWCSpectrum f0;
	if (maxEyeDepth <= 1 || !eye0.bsdf->SampleF(sw,
		eye0.wo, &eye0.wi, lensU, lensV, .5f,
		&f0, &eye0.pdfR, BSDF_ALL, &eye0.flags,
		&eye0.pdf, true))
			return nrContribs;
	eye0.flux *= f0;
	eye0.cosi = AbsDot(eye0.wi, eye0.bsdf->ng);
	eye0.rr = min(1.f, max(lightThreshold,
		f0.Filter(sw) * eye0.coso / eye0.cosi));
	eye0.rrR = min(1.f, max(eyeThreshold, f0.Filter(sw)));
	Ray ray(eyePath[0].p, eyePath[0].wi);
	ray.time = sample.realTime;
	sample.camera->ClampRay(ray);
	Intersection isect;
	u_int nEye = 1;
	eyePath[nEye].flux = eye0.flux;

	// Trace eye subpath and connect to light subpath
	SWCSpectrum &L(vecL[lightGroup]);
	float &variance(vecV[lightGroup]);
	const Volume *volume = eye0.bsdf->GetVolume(ray.d);
	bool scattered = eye0.bsdf->dgShading.scattered;
	for (u_int sampleIndex = 1; sampleIndex < maxEyeDepth; ++sampleIndex) {
		const float *data = scene.sampler->GetLazyValues(sample,
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
				v.pdf = v.bsdf->Pdf(sw, Vector(v.bsdf->nn),
					v.wo);
				// No check for pdf > 0
				// in the case of portal, the eye path can hit
				// the light outside portals
				v.dAWeight /= scene.lights.size();
				ePdfDirect *= directWeight;
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
				eyePath[0].d2 = INFINITY;
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
				v.pdf = eBsdf->Pdf(sw, Vector(eBsdf->nn), v.wo,
					v.flags);
				Ll *= v.flux;
				// Evaluate factors for path weighting
				v.dAWeight /= scene.lights.size();
				ePdfDirect *= directWeight;
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

		// Connect eye subpath to light subpath
		if (nLight > 0) {
			// Compute direct lighting pdf for first light vertex
			float directPdf = light->Pdf(v.p, lightPath[0].p,
				lightPath[0].bsdf->ng) * directWeight;
			// Go through all light vertices
			for (u_int j = 1; j <= nLight; ++j) {
				// Use general direct lighting pdf
				if (j >= 2)
					directPdf = lightDirectPdf;
				if (v.bsdf->NumComponents(BxDFType(~BSDF_SPECULAR)) == 0 || lightPath[j - 1].bsdf->NumComponents(BxDFType(~BSDF_SPECULAR)) == 0)
					continue;
				SWCSpectrum Ll(Le);
				float weight;
				// Save data modified by evalPath
				BidirVertex &vL(lightPath[j - 1]);
				const BxDFType lflags = vL.flags;
				const float lrr = vL.rr;
				const float lrrR = vL.rrR;
				const float ldARWeight = vL.dARWeight;
				if (evalPath(scene, sample, *this,
					eyePath, nEye, lightPath, j,
					directPdf, false, &weight, &Ll)) {
					L += Ll;
					variance += weight * Ll.Filter(sw);
					++nrContribs;
				}
				// Restore modified data
				vL.flags = lflags;
				vL.rr = lrr;
				vL.rrR = lrrR;
				vL.dARWeight = ldARWeight;
			}
		}

		// Break out if path is too long
		if (sampleIndex >= maxEyeDepth)
			break;

		// Do direct lighting
		const float *directData = scene.sampler->GetLazyValues(sample,
			sampleDirectOffset, sampleIndex);
		switch (lightStrategy) {
			case SAMPLE_ONE_UNIFORM: {
				SWCSpectrum Ld;
				float dWeight;
				float portal = directData[2] * numberOfLights;
				const u_int lightDirectNumber =
					min(Floor2UInt(portal),
					numberOfLights - 1U);
				portal -= lightDirectNumber;
				const Light *directLight = scene.lights[lightDirectNumber];
				if (getDirectLight(scene, sample, *this,
					eyePath, nEye, directLight,
					directData[0], directData[1], portal,
					directWeight, &Ld, &dWeight)) {
					vecL[directLight->group] += Ld;
					vecV[directLight->group] += Ld.Filter(sw) * dWeight;
					++nrContribs;
				}
				break;
			}
			case SAMPLE_ALL_UNIFORM: {
				SWCSpectrum Ld;
				float dWeight;
				for (u_int l = 0; l < scene.lights.size(); ++l) {
					const Light *directLight = scene.lights[l];
					if (getDirectLight(scene, sample, *this,
						eyePath, nEye, directLight,
						directData[0], directData[1],
						directData[2], directWeight,
						&Ld, &dWeight)) {
						vecL[directLight->group] += Ld;
						vecV[directLight->group] += Ld.Filter(sw) * dWeight;
						++nrContribs;
					}
					directData += 3;
				}
				break;
			}
			default:
				break;
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
	const float d = sqrtf(eyePath[0].d2);
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
	return nrContribs;
}

SurfaceIntegrator* BidirIntegrator::CreateSurfaceIntegrator(const ParamSet &params)
{
	int eyeDepth = params.FindOneInt("eyedepth", 8);
	int lightDepth = params.FindOneInt("lightdepth", 8);
	float eyeThreshold = params.FindOneFloat("eyerrthreshold", 0.f);
	float lightThreshold = params.FindOneFloat("lightrrthreshold", 0.f);
	LightStrategy estrategy;
	string st = params.FindOneString("strategy", "auto");
	if (st == "one") estrategy = SAMPLE_ONE_UNIFORM;
	else if (st == "all") estrategy = SAMPLE_ALL_UNIFORM;
	else if (st == "auto") estrategy = SAMPLE_AUTOMATIC;
	else {
		LOG(LUX_WARNING,LUX_BADTOKEN)<<"Strategy  '"<<st<<"' for direct lighting unknown. Using \"auto\".";
		estrategy = SAMPLE_AUTOMATIC;
	}
	bool debug = params.FindOneBool("debug", false);
	return new BidirIntegrator(max(eyeDepth, 0), max(lightDepth, 0),
		eyeThreshold, lightThreshold, estrategy, debug);
}

static DynamicLoader::RegisterSurfaceIntegrator<BidirIntegrator> r("bidirectional");
