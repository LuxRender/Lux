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
void BidirIntegrator::RequestSamples(Sample *sample, const Scene *scene)
{
	if (lightStrategy == SAMPLE_AUTOMATIC) {
		if (scene->sampler->IsMutating() || scene->lights.size() > 5)
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
	for (u_int i = 0; i < scene->lights.size(); ++i) {
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
	sampleEyeOffset = sample->AddxD(structure, maxEyeDepth);
	structure.clear();
	// Light subpath samples
	structure.push_back(1); //continue light
	structure.push_back(2); //bsdf sampling for light path
	structure.push_back(1); //bsdf component for light path
	sampleLightOffset = sample->AddxD(structure, maxLightDepth);
}
void BidirIntegrator::Preprocess(const TsPack *tspack, const Scene *scene)
{
	// Prepare image buffers
	BufferOutputConfig config = BUF_FRAMEBUFFER;
	if (debug)
		config = BufferOutputConfig(config | BUF_STANDALONE);
	BufferType type = BUF_TYPE_PER_PIXEL;
	scene->sampler->GetBufferType(&type);
	eyeBufferId = scene->camera->film->RequestBuffer(type, config, "eye");
	lightBufferId = scene->camera->film->RequestBuffer(BUF_TYPE_PER_SCREEN,
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

static bool evalPath(const TsPack *tspack, const Scene *scene,
	const BidirIntegrator &bidir,
	vector<BidirVertex> &eye, u_int nEye,
	vector<BidirVertex> &light, u_int nLight,
	float pdfLightDirect, bool isLightDirect, float *weight, SWCSpectrum *L)
{
	if (nLight <= 0 || nEye <= 0)
		return false;
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
	const SWCSpectrum ef(eyeV.bsdf->f(tspack, ewi, eyeV.wo, eyeV.flags));
	if (ef.Black())
		return false;
	lightV.flags = BxDFType(~BSDF_SPECULAR);
	const Vector lwo(-ewi);
	const SWCSpectrum lf(lightV.bsdf->f(tspack, lightV.wi, lwo, lightV.flags));
	if (lf.Black())
		return false;
	const float epdfR = eyeV.bsdf->Pdf(tspack, eyeV.wo, ewi, eyeV.flags);
	const float lpdf = lightV.bsdf->Pdf(tspack, lightV.wi, lwo, lightV.flags);
	float ltPdf = 1.f;
	float etPdfR = 1.f;
	if (!scene->volumeIntegrator->Connect(tspack, scene, NULL,
		eyeV.p, lightV.p, nEye == 1, L, &ltPdf, &etPdfR))
		return false;
	// Prepare eye vertex for connection
	const float ecosi = AbsDot(ewi, eyeV.bsdf->ng);
	const float d2 = DistanceSquared(eyeV.p, lightV.p);
	if (d2 < max(MachineEpsilon::E(eyeV.p), MachineEpsilon::E(lightV.p)))
		return false;
	const float ecosins = AbsDot(ewi, eyeV.bsdf->nn);
	SWCSpectrum eflux(ef); // No pdf as it is a direct connection
	eflux *= eyeV.flux;
	// Prepare light vertex for connection
	const float lcoso = AbsDot(lwo, lightV.bsdf->ng);
	const float lcosins = AbsDot(lightV.wi, lightV.bsdf->nn);
	SWCSpectrum lflux(lf * (lcosins / lightV.cosi)); // No pdf as it is a direct connection
	lflux *= lightV.flux;
	// Connect eye and light vertices
	*L *= eflux;
	*L *= lflux;
	*L *= ecosins * lcoso / d2;
	if (L->Black())
		return false;
	// Evaluate factors for eye path weighting
	const float epdf = eyeV.bsdf->Pdf(tspack, ewi, eyeV.wo, eyeV.flags);
	if (nEye == 1)
		eyeV.rr = 1.f;
	else
		eyeV.rr = min(1.f, max(lightThreshold, ef.Filter(tspack) *
			eyeV.coso * ecosins / (ecosi * epdf)));
	eyeV.rrR = min(1.f, max(eyeThreshold, ef.Filter(tspack) *
		ecosins / epdfR));
	eyeV.dAWeight = lpdf * ltPdf * ecosi / d2;
	if (nEye > 1)
		eye[nEye - 2].dAWeight = epdf * eyeV.tPdf *
			eye[nEye - 2].cosi / eye[nEye - 2].d2;
	// Evaluate factors for light path weighting
	const float lpdfR = lightV.bsdf->Pdf(tspack, lwo, lightV.wi, lightV.flags);
	lightV.rr = min(1.f, max(lightThreshold, lf.Filter(tspack) *
		lcoso * lcosins / (lightV.cosi * lpdf)));
	if (nLight == 1)
		lightV.rrR = 1.f;
	else
		lightV.rrR = min(1.f, max(eyeThreshold, lf.Filter(tspack) *
			lcosins / lpdfR));
	lightV.dARWeight = epdfR * etPdfR * lcoso / d2;
	const float lWeight = nLight > 1 ? light[nLight - 2].dARWeight : 0.f;
	if (nLight > 1)
		light[nLight - 2].dARWeight = lpdfR * lightV.tPdfR *
			light[nLight - 2].coso / light[nLight - 2].d2;
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

static bool eyeConnect(const TsPack *tspack, const Sample *sample, const BidirVertex &eye,
	const XYZColor &color, float alpha, float distance, float weight,
	u_int bufferId, u_int groupId)
{
	float xd, yd;
	if (!tspack->camera->GetSamplePosition(eye.p, eye.wi, distance,
		&xd, &yd))
		return false;
	sample->AddContribution(xd, yd, color, alpha, distance, weight,
		bufferId, groupId);
	return true;
}

static bool getDirectLight(const TsPack *tspack, const Scene *scene,
	const BidirIntegrator &bidir,
	vector<BidirVertex> &eyePath, u_int length,
	const Light *light, float u0, float u1, float portal,
	float directWeight, SWCSpectrum *Ld, float *weight)
{
	vector<BidirVertex> lightPath(1);
	BidirVertex &vE(eyePath[length - 1]);
	BidirVertex &vL(lightPath[0]);
	VisibilityTester visibility;
	float ePdfDirect;
	// Sample the chosen light
	if (!light->Sample_L(tspack, scene, vE.p, vE.bsdf->ng, u0, u1, portal,
		&vL.bsdf, &vL.dAWeight, &ePdfDirect, &visibility, Ld))
		return false;
	vL.p = vL.bsdf->dgShading.p;
	vL.wi = Vector(vL.bsdf->nn);
	vL.cosi = AbsDot(vL.wi, vL.bsdf->ng);
	vL.dAWeight /= scene->lights.size();
	if (light->IsDeltaLight())
		vL.dAWeight = -vL.dAWeight;
	ePdfDirect *= directWeight;
	vL.flux = SWCSpectrum(1.f / ePdfDirect);
	if (!evalPath(tspack, scene, bidir, eyePath, length,
		lightPath, 1, ePdfDirect, true, weight, Ld))
		return false;
	return true;
}

u_int BidirIntegrator::Li(const TsPack *tspack, const Scene *scene,
	const Sample *sample) const
{
	u_int nrContribs = 0;
	// If no eye vertex, return immediately
	//FIXME: this is not necessary if light path can intersect the camera
	// or if direct connection to the camera is implemented
	if (maxEyeDepth <= 0)
		return nrContribs;
	vector<BidirVertex> eyePath(maxEyeDepth), lightPath(maxLightDepth);
	const u_int nGroups = scene->lightGroups.size();
	const u_int numberOfLights = scene->lights.size();
	// If there are no lights, the scene is black
	if (numberOfLights == 0)
		return nrContribs;
	const float directWeight = (lightStrategy == SAMPLE_ONE_UNIFORM) ?
		1.f / numberOfLights : 1.f;
	vector<SWCSpectrum> vecL(nGroups, SWCSpectrum(0.f));
	vector<float> vecV(nGroups, 0.f);
	float alpha = 1.f;

	// Sample eye subpath origin
	SWCSpectrum We;
	const float posX = tspack->camera->IsLensBased() ? sample->lensU : sample->imageX;
	const float posY = tspack->camera->IsLensBased() ? sample->lensV : sample->imageY;
	//Jeanphi - Replace dummy .5f by a sampled value if needed
	//FIXME: the return is not necessary if direct connection to the camera
	// is implemented
	if (!tspack->camera->Sample_W(tspack, scene,
		posX, posY, .5f, &eyePath[0].bsdf, &eyePath[0].dARWeight, &We))
		return nrContribs;
	BidirVertex &eye0(eyePath[0]);
	// Initialize eye vertex
	eye0.p = eye0.bsdf->dgShading.p;
	eye0.wo = Vector(eye0.bsdf->nn);
	eye0.coso = AbsDot(eye0.wo, eye0.bsdf->ng);
	We /= eye0.dARWeight;
	// Light path cannot intersect camera (FIXME)
	eye0.dARWeight = 0.f;
	eye0.flux = SWCSpectrum(1.f);

	// Do eye vertex direct lighting
	const float *directData0 = sample->sampler->GetLazyValues(const_cast<Sample *>(sample), sampleDirectOffset, 0);
	switch (lightStrategy) {
		case SAMPLE_ONE_UNIFORM: {
			SWCSpectrum Ld;
			float dWeight;
			float portal = directData0[2] * numberOfLights;
			const u_int lightDirectNumber = min(Floor2UInt(portal),
				numberOfLights - 1U);
			Light *light = scene->lights[lightDirectNumber];
			portal -= lightDirectNumber;
			if (getDirectLight(tspack, scene, *this, eyePath, 1,
				light, directData0[0], directData0[1], portal,
				directWeight, &Ld, &dWeight)) {
				Ld *= We;
				if (light->IsEnvironmental()) {
					if (eyeConnect(tspack, sample, eye0,
						XYZColor(tspack, Ld), 0.f,
						INFINITY, dWeight,
						lightBufferId, light->group))
						++nrContribs;
				} else {
					if (eyeConnect(tspack, sample, eye0,
						XYZColor(tspack, Ld), 1.f,
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
			for (u_int l = 0; l < scene->lights.size(); ++l) {
				Light *light = scene->lights[l];
				if (getDirectLight(tspack, scene, *this,
					eyePath, 1, light,
					directData0[0], directData0[1],
					directData0[2], directWeight,
					&Ld, &dWeight)) {
					Ld *= We;
					if (light->IsEnvironmental()) {
						if (eyeConnect(tspack, sample,
							eye0,
							XYZColor(tspack, Ld),
							0.f, INFINITY, dWeight,
							lightBufferId,
							light->group))
							++nrContribs;
					} else {
						if (eyeConnect(tspack, sample,
							eye0,
							XYZColor(tspack, Ld),
							1.f, sqrtf(eye0.d2),
							dWeight, lightBufferId,
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
	const u_int lightNum = min(Floor2UInt(sample->oneD[lightNumOffset][0] *
		numberOfLights), numberOfLights - 1U);
	const Light *light = scene->lights[lightNum];
	const u_int lightGroup = light->group;
	const float *data = sample->twoD[lightPosOffset];
	const float component = sample->oneD[lightComponentOffset][0];
	SWCSpectrum Le;

	// Sample light subpath origin
	u_int nLight = 0;
	float lightDirectPdf = 0.f;
	if (maxLightDepth > 0 && light->Sample_L(tspack, scene,
		data[0], data[1], component, &lightPath[0].bsdf,
		&lightPath[0].dAWeight, &Le)) {
		BidirVertex &light0(lightPath[0]);
		// Initialize light vertex
		light0.p = light0.bsdf->dgShading.p;
		light0.wi = Vector(light0.bsdf->nn);
		light0.cosi = AbsDot(light0.wi, light0.bsdf->ng);
		// Give the light point probability for the weighting
		// if the light is not delta
		light0.dAWeight /= numberOfLights;
		light0.dAWeight = max(light0.dAWeight, 0.f);
		// Divide by Pdf because this value isn't used when the eye
		// ray hits a light source, only for light paths
		Le /= light0.dAWeight;
		// Trick to tell subsequent functions that the light is delta
		if (light->IsDeltaLight())
			light0.dAWeight = -light0.dAWeight;
		nLight = 1;

		// Connect light vertex to eye vertex
		// Compute direct lighting pdf for first light vertex
		const float directPdf = light->Pdf(tspack, eye0.p, eye0.bsdf->ng,
			light0.p, light0.bsdf->ng) * directWeight;
		SWCSpectrum Ll(Le);
		float weight;
		if (evalPath(tspack, scene, *this, eyePath, 1, lightPath, nLight,
			directPdf, false, &weight, &Ll) &&
			eyeConnect(tspack, sample, eye0,
			XYZColor(tspack, Ll * We),
			light->IsEnvironmental() ? 0.f : 1.f,
			light->IsEnvironmental() ? INFINITY : sqrtf(eye0.d2),
			weight, lightBufferId, lightGroup))
			++nrContribs;

		// Sample light subpath initial direction and
		// finish vertex initialization if needed
		data = sample->sampler->GetLazyValues(const_cast<Sample *>(sample), sampleLightOffset, nLight - 1);
		if (maxLightDepth > 1 && light0.bsdf->Sample_f(tspack,
			light0.wi, &light0.wo, data[1], data[2], data[3],
			&light0.flux, &light0.pdf, BSDF_ALL, &light0.flags,
			&light0.pdfR)) {
			light0.coso = AbsDot(light0.wo, light0.bsdf->ng);
			const float cosins0 = AbsDot(light0.wi, light0.bsdf->nn);
			light0.rrR = min(1.f, max(eyeThreshold,
				light0.flux.Filter(tspack) * cosins0 / light0.pdfR));
			light0.flux *= (light0.coso * cosins0) /
				(light0.pdf * light0.cosi);
			light0.rr = min(1.f, max(lightThreshold,
				light0.flux.Filter(tspack)));
			RayDifferential ray(light0.p, light0.wo);
			ray.time = tspack->time;
			Intersection isect;
			u_int through = 0;
			lightPath[nLight].flux = light0.flux;

			// Trace light subpath and connect to eye vertex
			while (true) {
				BidirVertex &v = lightPath[nLight];
				if (!scene->Intersect(tspack,
					lightPath[nLight - 1].bsdf->GetVolume(ray.d),
					ray, &isect, &v.bsdf, &v.flux))
					break;
				++nLight;

				// Initialize new intersection vertex
				v.wi = -ray.d;
				v.p = isect.dg.p;
				v.cosi = AbsDot(v.wi, v.bsdf->ng);
				lightPath[nLight - 2].d2 =
					DistanceSquared(lightPath[nLight - 2].p, v.p);
				v.dAWeight = lightPath[nLight - 2].pdf *
					lightPath[nLight - 2].tPdf *
					v.cosi / lightPath[nLight - 2].d2;
				// Compute light direct Pdf between
				// the first 2 vertices
				if (nLight == 2)
					lightDirectPdf = light->Pdf(tspack, v.p,
						v.bsdf->ng, lightPath[0].p,
						lightPath[0].bsdf->ng) * directWeight;

				// Connect light subpath to eye vertex
				if (v.bsdf->NumComponents(BxDFType(~BSDF_SPECULAR)) > 0 && eye0.bsdf->NumComponents(BxDFType(~BSDF_SPECULAR)) > 0) {
					SWCSpectrum Ll(Le);
					float weight;
					if (evalPath(tspack, scene, *this,
						eyePath, 1, lightPath, nLight,
						lightDirectPdf, false,
						&weight, &Ll) &&
						eyeConnect(tspack, sample, eye0,
						XYZColor(tspack, Ll * We), 1.f,
						sqrtf(eye0.d2), weight,
						lightBufferId, lightGroup))
						++nrContribs;
				}

				// Possibly terminate path sampling
				if (nLight == maxLightDepth)
					break;
				data = sample->sampler->GetLazyValues(const_cast<Sample *>(sample), sampleLightOffset, nLight - 1);
				SWCSpectrum f;
				if (!v.bsdf->Sample_f(tspack, v.wi, &v.wo,
					data[1], data[2], data[3], &f, &v.pdf,
					BSDF_ALL, &v.flags, &v.pdfR))
					break;

				// Check if the scattering is a passthrough event
				if (v.flags != (BSDF_TRANSMISSION | BSDF_SPECULAR) ||
					!(v.bsdf->Pdf(tspack, v.wi, v.wo, BxDFType(BSDF_TRANSMISSION | BSDF_SPECULAR)) > 0.f)) {
					lightPath[nLight - 2].dARWeight =
						v.pdfR * v.tPdfR *
						lightPath[nLight - 2].coso /
						lightPath[nLight - 2].d2;
					v.coso = AbsDot(v.wo, v.bsdf->ng);
					const float cosins = AbsDot(v.wi, v.bsdf->nn);
					v.rrR = min(1.f, max(eyeThreshold,
						f.Filter(tspack) * cosins / v.pdfR));
					f *= (v.coso * cosins) /
						(v.pdf * v.cosi);
					v.rr = min(1.f, max(lightThreshold,
						f.Filter(tspack)));
					v.flux *= f;
					if (nLight > rrStart) {
						if (v.rr < data[0])
							break;
						v.flux /= v.rr;
					}
					lightPath[nLight].flux = v.flux;
					through = 0;
				} else {
					const float cosins = AbsDot(v.wi, v.bsdf->nn);
					// No need to do '/ v.cosi * v.coso' since cosi==coso
					v.flux *= f * (cosins / v.pdf);
					lightPath[nLight - 2].tPdf *= v.pdf;
					v.tPdfR *= v.pdfR;
					--nLight;
					if (through++ > passThroughLimit)
						break;
				}

				// Initialize _ray_ for next segment of path
				ray = RayDifferential(v.p, v.wo);
				ray.time = tspack->time;
			}
		}
	}

	// Sample eye subpath initial direction and finish vertex initialization
	const float lensU = tspack->camera->IsLensBased() ? sample->imageX :
		sample->lensU;
	const float lensV = tspack->camera->IsLensBased() ? sample->imageY :
		sample->lensV;
	//Jeanphi - Replace dummy .5f by a sampled value if needed
	if (maxEyeDepth <= 1 || !eye0.bsdf->Sample_f(tspack,
		eye0.wo, &eye0.wi, lensU, lensV, .5f,
		&eye0.flux, &eye0.pdfR, BSDF_ALL, &eye0.flags,
		&eye0.pdf, true))
			return nrContribs;
	eye0.cosi = AbsDot(eye0.wi, eye0.bsdf->ng);
	const float cosins0 = AbsDot(eye0.wi, eye0.bsdf->nn);
	eye0.rr = min(1.f, max(lightThreshold,
		eye0.flux.Filter(tspack) * (cosins0 * eye0.coso / (eye0.cosi * eye0.pdf))));
	eye0.flux *= (cosins0 / eye0.pdfR);
	eye0.rrR = min(1.f, max(eyeThreshold, eye0.flux.Filter(tspack)));
	RayDifferential ray(eyePath[0].p, eyePath[0].wi);
	ray.time = tspack->time;
	tspack->camera->ClampRay(ray);
	Intersection isect;
	u_int nEye = 1;
	u_int through = 0;
	eyePath[nEye].flux = eye0.flux;

	// Trace eye subpath and connect to light subpath
	SWCSpectrum &L(vecL[lightGroup]);
	float &variance(vecV[lightGroup]);
	while (true) {
		BidirVertex &v = eyePath[nEye];
		if (!scene->Intersect(tspack,
			eyePath[nEye - 1].bsdf->GetVolume(ray.d), ray, &isect,
			&v.bsdf, &v.flux)) {
			vector<BidirVertex> path(0);
			for (u_int lightNumber = 0; lightNumber < scene->lights.size(); ++lightNumber) {
				const Light *light = scene->lights[lightNumber];
				if (!light->IsEnvironmental())
					continue;
				RayDifferential r(eyePath[nEye - 1].p, eyePath[nEye - 1].wi);
				r.time = tspack->time;
				BSDF *eBsdf;
				float ePdfDirect;
				SWCSpectrum Le(light->Le(tspack, scene, r,
					eyePath[nEye - 1].bsdf->ng, &eBsdf,
					&v.dAWeight, &ePdfDirect));
				// No check for dAWeight > 0
				// in the case of portal, the eye path can hit
				// the light outside portals
				if (eBsdf == NULL || Le.Black())
					continue;
				v.wo = -ray.d;
				v.flags = BxDFType(~BSDF_SPECULAR);
				v.pdf = eBsdf->Pdf(tspack, Vector(eBsdf->nn), v.wo,
					v.flags);
				// No check for pdf > 0
				// in the case of portal, the eye path can hit
				// the light outside portals
				v.p = eBsdf->dgShading.p;
				v.coso = AbsDot(v.wo, eBsdf->ng);
				eyePath[nEye - 1].d2 =
					DistanceSquared(eyePath[nEye - 1].p, v.p);
				v.dARWeight = eyePath[nEye - 1].pdfR *
					eyePath[nEye - 1].tPdfR *
					v.coso / eyePath[nEye - 1].d2;
				Le *= v.flux;
				// Evaluate factors for path weighting
				v.dAWeight /= scene->lights.size();
				ePdfDirect *= directWeight;
				eyePath[nEye - 1].dAWeight = v.pdf * v.tPdf *
					eyePath[nEye - 1].cosi /
					eyePath[nEye - 1].d2;
				vector<BidirVertex> path(0);
				const float w = weightPath(eyePath,
					nEye + 1, maxEyeDepth, path, 0,
					maxLightDepth, ePdfDirect,
					false);
				const u_int eGroup = light->group;
				Le /= w;
				vecV[eGroup] += Le.Filter(tspack);
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
		++nEye;

		// Initialize new intersection vertex
		v.wo = -ray.d;
		v.p = isect.dg.p;
		v.coso = AbsDot(v.wo, v.bsdf->ng);
		eyePath[nEye - 2].d2 =
			DistanceSquared(eyePath[nEye - 2].p, v.p);
		v.dARWeight = eyePath[nEye - 2].pdfR *
			eyePath[nEye - 2].tPdfR * v.coso /
			eyePath[nEye - 2].d2;

		// Test intersection with a light source
		if (isect.arealight) {
			RayDifferential r(eyePath[nEye - 2].p, eyePath[nEye - 2].wi);
			r.time = tspack->time;
			BSDF *eBsdf;
			float ePdfDirect;
			SWCSpectrum Ll(isect.Le(tspack, r,
				eyePath[nEye - 2].bsdf->ng, &eBsdf, &v.dAWeight,
				&ePdfDirect));
			if (eBsdf && !Ll.Black()) {
				v.flags = BxDFType(~BSDF_SPECULAR);
				v.pdf = eBsdf->Pdf(tspack, Vector(eBsdf->nn), v.wo,
					v.flags);
				Ll *= v.flux;
				// Evaluate factors for path weighting
				v.dAWeight /= scene->lights.size();
				ePdfDirect *= directWeight;
				eyePath[nEye - 2].dAWeight = v.pdf * v.tPdf *
					eyePath[nEye - 2].cosi /
					eyePath[nEye - 2].d2;
				vector<BidirVertex> path(0);
				const float w = weightPath(eyePath,
					nEye, maxEyeDepth, path, 0,
					maxLightDepth, ePdfDirect,
					false);
				const u_int eGroup = isect.arealight->group;
				Ll /= w;
				vecV[eGroup] += Ll.Filter(tspack) / w;
				vecL[eGroup] += Ll;
				++nrContribs;
			}
		}

		// Do direct lighting
		const float *directData = sample->sampler->GetLazyValues(const_cast<Sample *>(sample), sampleDirectOffset, nEye - 1);
		switch (lightStrategy) {
			case SAMPLE_ONE_UNIFORM: {
				SWCSpectrum Ld;
				float dWeight;
				float portal = directData[2] * numberOfLights;
				const u_int lightDirectNumber =
					min(Floor2UInt(portal),
					numberOfLights - 1U);
				portal -= lightDirectNumber;
				const Light *directLight = scene->lights[lightDirectNumber];
				if (getDirectLight(tspack, scene, *this,
					eyePath, nEye, directLight,
					directData[0], directData[1], portal,
					directWeight, &Ld, &dWeight)) {
					vecL[directLight->group] += Ld;
					vecV[directLight->group] += Ld.Filter(tspack) * dWeight;
					++nrContribs;
				}
				break;
			}
			case SAMPLE_ALL_UNIFORM: {
				SWCSpectrum Ld;
				float dWeight;
				for (u_int l = 0; l < scene->lights.size(); ++l) {
					const Light *directLight = scene->lights[l];
					if (getDirectLight(tspack, scene, *this,
						eyePath, nEye, directLight,
						directData[0], directData[1],
						directData[2], directWeight,
						&Ld, &dWeight)) {
						vecL[directLight->group] += Ld;
						vecV[directLight->group] += Ld.Filter(tspack) * dWeight;
						++nrContribs;
					}
					directData += 3;
				}
				break;
			}
			default:
				break;
		}

		// Connect eye subpath to light subpath
		if (nLight > 0) {
			// Compute direct lighting pdf for first light vertex
			float directPdf = light->Pdf(tspack, v.p, v.bsdf->ng, lightPath[0].p,
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
				if (evalPath(tspack, scene, *this,
					eyePath, nEye, lightPath, j,
					directPdf, false, &weight, &Ll)) {
					L += Ll;
					variance += weight * Ll.Filter(tspack);
					++nrContribs;
				}
				// Restore modified data
				vL.flags = lflags;
				vL.rr = lrr;
				vL.rrR = lrrR;
				vL.dARWeight = ldARWeight;
			}
		}

		// Possibly terminate path sampling
		if (nEye == maxEyeDepth)
			break;
		data = sample->sampler->GetLazyValues(const_cast<Sample *>(sample), sampleEyeOffset, nEye - 1);
		SWCSpectrum f;
		if (!v.bsdf->Sample_f(tspack, v.wo, &v.wi, data[1], data[2],
			data[3], &f, &v.pdfR, BSDF_ALL, &v.flags, &v.pdf, true))
			break;

		// Check if the scattering is a passthrough event
		if (v.flags != (BSDF_TRANSMISSION | BSDF_SPECULAR) ||
			!(v.bsdf->Pdf(tspack, v.wi, v.wo, BxDFType(BSDF_TRANSMISSION | BSDF_SPECULAR)) > 0.f)) {
			eyePath[nEye - 2].dAWeight = v.pdf * v.tPdf *
				eyePath[nEye - 2].cosi /
				eyePath[nEye - 2].d2;
			v.cosi = AbsDot(v.wi, v.bsdf->ng);
			const float cosins = AbsDot(v.wi, v.bsdf->nn);
			v.rr = min(1.f, max(lightThreshold,
				f.Filter(tspack) * (cosins * v.coso / (v.cosi * v.pdf))));
			f *= cosins / v.pdfR;
			v.rrR = min(1.f, max(eyeThreshold, f.Filter(tspack)));
			v.flux *= f;
			if (nEye > rrStart) {
				if (v.rrR < data[0])
					break;
				v.flux /= v.rrR;
			}
			eyePath[nEye].flux = v.flux;
			through = 0;
		} else {
			const float cosins = AbsDot(v.wi, v.bsdf->nn);
			v.flux *= f * (cosins / v.pdfR);
			eyePath[nEye - 2].tPdfR *= v.pdfR;
			v.tPdf *= v.pdf;
			--nEye;
			if (through++ > passThroughLimit)
				break;
		}

		// Initialize _ray_ for next segment of path
		ray = RayDifferential(v.p, v.wi);
		ray.time = tspack->time;
	}
	const float d = sqrtf(eyePath[0].d2);
	float xl, yl;
	if (!tspack->camera->GetSamplePosition(eyePath[0].p, eyePath[0].wi, d, &xl, &yl))
		return nrContribs;
	for (u_int i = 0; i < nGroups; ++i) {
		if (!vecL[i].Black())
			vecV[i] /= vecL[i].Filter(tspack);
		vecL[i] *= We;
		XYZColor color(tspack, vecL[i]);
		sample->AddContribution(xl, yl,
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
		std::stringstream ss;
		ss<<"Strategy  '"<<st<<"' for direct lighting unknown. Using \"auto\".";
		luxError(LUX_BADTOKEN,LUX_WARNING,ss.str().c_str());
		estrategy = SAMPLE_AUTOMATIC;
	}
	bool debug = params.FindOneBool("debug", false);
	return new BidirIntegrator(max(eyeDepth, 0), max(lightDepth, 0),
		eyeThreshold, lightThreshold, estrategy, debug);
}

static DynamicLoader::RegisterSurfaceIntegrator<BidirIntegrator> r("bidirectional");
