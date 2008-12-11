/***************************************************************************
 *   Copyright (C) 1998-2007 by authors (see AUTHORS.txt )                 *
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

struct BidirVertex {
	BidirVertex() : bsdf(NULL), eBsdf(NULL), pdf(0.f), dAWeight(0.f),
		rr(1.f), pdfR(0.f), dARWeight(0.f), rrR(1.f),
		ePdf(0.f), ePdfDirect(0.f), flags(BxDFType(0)),
		f(0.f), flux(0.f), Le(0.f) {}
	BSDF *bsdf, *eBsdf;
	Point p;
	Normal ng, ns;
	Vector wi, wo;
	float cosi, coso, d2, pdf, dAWeight, rr, pdfR, dARWeight, rrR, ePdf, ePdfDirect;
	BxDFType flags;
	SWCSpectrum f, flux, Le;
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
	BufferType type = BUF_TYPE_PER_PIXEL;
	scene->sampler->GetBufferType(&type);
	eyeBufferId = scene->camera->film->RequestBuffer(type, BUF_FRAMEBUFFER, "eye");
	lightBufferId = scene->camera->film->RequestBuffer(BUF_TYPE_PER_SCREEN, BUF_FRAMEBUFFER, "light");
}

static int generateEyePath(const TsPack *tspack, const Scene *scene, BSDF *bsdf,
	const Sample *sample, const int sampleOffset,
	vector<BidirVertex> &vertices, float directWeight, float time)
{
	RayDifferential ray;
	Intersection isect;
	u_int nVerts = 0;
	while (nVerts < vertices.size()) {
		// Find next vertex in path and initialize _vertices_
		BidirVertex &v = vertices[nVerts];
		const float *data;
		if (nVerts == 0) {
			data = NULL;
			v.wo = Vector(bsdf->dgShading.nn);
			v.bsdf = bsdf;
			v.p = bsdf->dgShading.p;
			v.ng = bsdf->dgShading.nn;
		} else {
			data = sample->sampler->GetLazyValues(const_cast<Sample *>(sample), sampleOffset, nVerts);
			v.wo = -ray.d;
			v.bsdf = isect.GetBSDF(tspack, ray, fabsf(2.f * data[3] - 1.f));
			v.p = isect.dg.p;
			v.ng = isect.dg.nn;
			v.Le = isect.Le(tspack, ray, vertices[nVerts - 1].ns, &v.eBsdf, &v.ePdf, &v.ePdfDirect);
			v.ePdf /= scene->lights.size();
			v.ePdfDirect *= directWeight;
		}
		v.ns = v.bsdf->dgShading.nn;
		v.coso = AbsDot(v.wo, v.ng);
		++nVerts;
		// Possibly terminate bidirectional path sampling
		if (nVerts == vertices.size())
			break;
		bool sampled;
		if (nVerts == 1)
			sampled = v.bsdf->Sample_f(tspack, v.wo, &v.wi, sample->imageX, sample->imageY, 0.5,
				&v.f, &v.pdfR, BSDF_ALL, &v.flags, &v.pdf, true);
		else
			sampled = v.bsdf->Sample_f(tspack, v.wo, &v.wi, data[1], data[2], data[3],
				&v.f, &v.pdfR, BSDF_ALL, &v.flags, &v.pdf, true);
		if (!sampled)
			break;
		v.cosi = AbsDot(v.wi, v.ng);
		v.f *= AbsDot(v.wi, v.ns) / v.cosi;
		v.flux = v.f;
		v.flux *= v.cosi / v.pdfR;
		v.rrR = min<float>(1.f, v.flux.filter(tspack));
		if (nVerts > 3) {
			if (v.rrR < data[0])
				break;
			v.flux /= v.rrR;
		}
		v.rr = min<float>(1.f, v.f.filter(tspack) * v.coso / v.pdf);
		if (nVerts > 1)
			v.flux *= vertices[nVerts - 2].flux;
		// Initialize _ray_ for next segment of path
		ray = RayDifferential(v.p, v.wi);
		ray.time = time;
		if (!scene->Intersect(ray, &isect)) {
			vertices[nVerts].wo = -ray.d;
			vertices[nVerts].bsdf = NULL;
			++nVerts;
			break;
		}
	}
	// Initialize additional values in _vertices_
	for (u_int i = 0; i < nVerts - 1; ++i) {
		if (vertices[i + 1].bsdf == NULL)
			break;
		vertices[i].d2 = DistanceSquared(vertices[i].p, vertices[i + 1].p);
		vertices[i + 1].dARWeight = vertices[i].pdfR *
			vertices[i + 1].coso / vertices[i].d2;
		vertices[i].dAWeight = vertices[i + 1].pdf *
			vertices[i].cosi / vertices[i].d2;
	}
	return nVerts;
}

static int generateLightPath(const TsPack *tspack, const Scene *scene, BSDF *bsdf,
	const Sample *sample, const int sampleOffset,
	vector<BidirVertex> &vertices, float time)
{
	RayDifferential ray;
	Intersection isect;
	u_int nVerts = 0;
	while (nVerts < vertices.size()) {
		BidirVertex &v = vertices[nVerts];
		const float *data = sample->sampler->GetLazyValues(const_cast<Sample *>(sample), sampleOffset, nVerts);
		if (nVerts == 0) {
			v.wi = Vector(bsdf->dgShading.nn);
			v.bsdf = bsdf;
			v.p = bsdf->dgShading.p;
			v.ng = bsdf->dgShading.nn;
		} else {
			v.wi = -ray.d;
			v.bsdf = isect.GetBSDF(tspack, ray, fabsf(2.f * data[3] - 1.f));
			v.p = isect.dg.p;
			v.ng = isect.dg.nn;
		}
		v.ns = v.bsdf->dgShading.nn;
		v.cosi = AbsDot(v.wi, v.ng);
		++nVerts;
		// Possibly terminate bidirectional path sampling
		if (nVerts == vertices.size())
			break;
		if (!v.bsdf->Sample_f(tspack, v.wi, &v.wo, data[1], data[2], data[3],
			 &v.f, &v.pdf, BSDF_ALL, &v.flags, &v.pdfR))
			break;
		v.coso = AbsDot(v.wo, v.ng);
		v.f *= AbsDot(v.wi, v.ns) / v.cosi;
		v.flux = v.f;
		v.flux *= v.coso / v.pdf;
		v.rr = min<float>(1.f, v.flux.filter(tspack));
		if (nVerts > 3) {
			if (v.rr < data[0])
				break;
			v.flux /= v.rr;
		}
		v.rrR = min<float>(1.f, v.f.filter(tspack) * v.cosi / v.pdfR);
		if (nVerts > 1)
			v.flux *= vertices[nVerts - 2].flux;
		// Initialize _ray_ for next segment of path
		ray = RayDifferential(v.p, v.wo);
		ray.time = time;
		if (!scene->Intersect(ray, &isect))
			break;
	}
	// Initialize additional values in _vertices_
	for (u_int i = 0; i < nVerts - 1; ++i) {
		vertices[i].d2 = DistanceSquared(vertices[i].p, vertices[i + 1].p);
		vertices[i + 1].dAWeight = vertices[i].pdf *
			vertices[i + 1].cosi / vertices[i].d2;
		vertices[i].dARWeight = vertices[i + 1].pdfR *
			vertices[i].coso / vertices[i].d2;
	}
	return nVerts;
}

// Connecting factor
static float G(const BidirVertex &eye, const BidirVertex &light)
{
	return eye.cosi * light.coso / eye.d2;
}

// Visibility test
static bool visible(const TsPack *tspack, const Scene *scene, const Point &P0,
	const Point &P1, SWCSpectrum *f)
{
	VisibilityTester vt;
	vt.SetSegment(P0, P1);
	return vt.TestOcclusion(tspack, scene, f);
}

// Weighting of path with regard to alternate methods of obtaining it
static float weightPath(const vector<BidirVertex> &eye, int nEye, int eyeDepth,
	const vector<BidirVertex> &light, int nLight, int lightDepth,
	float pdfLightDirect, bool isLightDirect)
{
	// Current path weight is 1;
	float weight = 1.f, p = 1.f, pDirect = 0.f;
	if (nLight == 1) {
		pDirect = pdfLightDirect;
		if (light[0].dAWeight > 0.f)
			pDirect /= light[0].dAWeight;
		else
			p = 0.f;
		// Unconditional add since the last eye vertex can't be specular
		// otherwise the connectio wouldn't have been possible
		// and weightPath wouldn't have been called
		weight += pDirect * pDirect;
		// If the light is unidirectional the path can only be obtained
		// by sampling the light directly
		if (isLightDirect && ((light[0].flags & BSDF_SPECULAR) != 0 ||
			!(light[0].dAWeight > 0.f)))
			weight -= 1.f;
	}
	// Find other paths by extending light path toward eye path
	for (int i = nEye - 1; i >= max(0, nEye - (lightDepth - nLight)); --i) {
		// Exit if the path is impossible
		if (!(eye[i].dARWeight > 0.f))
			break;
		// Exit if the path is impossible
		if (!(eye[i].dAWeight > 0.f))
			break;
		p *= eye[i].dAWeight / eye[i].dARWeight;
		if (i > 3)
			p /= eye[i - 1].rrR;
		if (nLight + nEye - i > 3) {
			if (i < nEye - 1)
				p *= eye[i + 1].rr;
			else if (nLight > 0)
				p *= light[nLight - 1].rr;
		}
		// Check for direct path
		// only possible if the eye path hit a light
		// So the eye path has at least 2 vertices
		if (nLight + nEye - i == 1) {
			pDirect = p * pdfLightDirect / eye[i].dAWeight;
			// The light vertex cannot be specular otherwise
			// the eye path wouldn't have received any light
			if (i > 0 && (eye[i - 1].flags & BSDF_SPECULAR) == 0)
				weight += pDirect * pDirect;
		}
		// The path can only be obtained if none of the vertices
		// is specular
		if ((eye[i].flags & BSDF_SPECULAR) == 0 &&
			(i == 0 || (eye[i - 1].flags & BSDF_SPECULAR) == 0))
			weight += p * p;
	}
	// Reinitialize p to search paths in the other direction
	p = 1.f;
	// Find other paths by extending eye path toward light path
	for (int i = nLight - 1; i >= max(0, nLight - (eyeDepth - nEye)); --i) {
		// Exit if the path is impossible
		if (!(light[i].dARWeight > 0.f))
			break;
		// Exit if the path is impossible
		if (!(light[i].dAWeight > 0.f))
			break;
		p *= light[i].dARWeight / light[i].dAWeight;
		if (i > 3)
			p /= light[i - 1].rr;
		if (nEye + nLight - i > 3) {
			if (i < nLight - 1)
				p *= light[i + 1].rrR;
			else if (nEye > 0)
				p *= eye[nEye - 1].rrR;
		}
		// Check for direct path
		// light path has at least 2 remaining vertices here
		if (i == 1) {
			pDirect = p * pdfLightDirect / light[0].dAWeight;
			// The path can only be obtained if none of the vertices
			// is specular
			if ((light[1].flags & BSDF_SPECULAR) == 0 &&
				(light[0].flags & BSDF_SPECULAR) == 0)
				weight += pDirect * pDirect;
		}
		// The path can only be obtained if none of the vertices
		// is specular
		if ((light[i].flags & BSDF_SPECULAR) == 0 &&
			(i == 0 || (light[i - 1].flags & BSDF_SPECULAR) == 0))
			weight += p * p;
	}
	// The weight is computed for normal lighting,
	// if we have direct lighting, compensate for it
	if (isLightDirect && pDirect > 0.f)
		weight /= pDirect * pDirect;
	return weight;
}

static SWCSpectrum evalPath(const TsPack *tspack, const Scene *scene,
	vector<BidirVertex> &eye, int nEye, int eyeDepth,
	vector<BidirVertex> &light, int nLight, int lightDepth,
	float pdfLightDirect, bool isLightDirect, float *weight)
{
	SWCSpectrum L;
	*weight = 0.f;
	float eWeight = 0.0f, lWeight = 0.0f;
	// Be carefull, eye and light last vertex can be modified here
	// If each path has at least 1 vertex, connect them
	if (nLight > 0 && nEye > 0) {
		BidirVertex &eyeV(eye[nEye - 1]);
		BidirVertex &lightV(light[nLight - 1]);
		// Check Connectability
		eyeV.flags = BxDFType(~BSDF_SPECULAR);
		eyeV.wi = Normalize(lightV.p - eyeV.p);
		eyeV.pdfR = eyeV.bsdf->Pdf(tspack, eyeV.wo, eyeV.wi, eyeV.flags);
		if (!(eyeV.pdfR > 0.f))
			return 0.f;
		eyeV.f = eyeV.bsdf->f(tspack, eyeV.wi, eyeV.wo, eyeV.flags);
		if (eyeV.f.Black())
			return 0.f;
		lightV.flags = BxDFType(~BSDF_SPECULAR);
		lightV.wo = -eyeV.wi;
		lightV.pdf = lightV.bsdf->Pdf(tspack, lightV.wi, lightV.wo, lightV.flags);
		if (!(lightV.pdf > 0.f))
			return 0.f;
		lightV.f = lightV.bsdf->f(tspack, lightV.wi, lightV.wo, lightV.flags);
		if (lightV.f.Black())
			return 0.f;
		SWCSpectrum fConnect;
		if (!visible(tspack, scene, lightV.p, eyeV.p, &fConnect))
			return 0.f;
		// Prepare eye vertex for connection
		eyeV.cosi = AbsDot(eyeV.wi, eyeV.ng);
		eyeV.d2 = DistanceSquared(eyeV.p, lightV.p);
		eyeV.f *= AbsDot(eyeV.wi, eyeV.ns) / eyeV.cosi;
		eyeV.flux = eyeV.f; // No pdf as it is a direct connection
		if (nEye > 1)
			eyeV.flux *= eye[nEye - 2].flux;
		// Prepare light vertex for connection
		lightV.coso = AbsDot(lightV.wo, lightV.ng);
		lightV.d2 = eyeV.d2;
		lightV.f *= AbsDot(lightV.wi, lightV.ns) / lightV.cosi;
		lightV.flux = lightV.f; // No pdf as it is a direct connection
		if (nLight > 1)
			lightV.flux *= light[nLight - 2].flux;
		// Connect eye and light vertices
		L = eyeV.flux * G(eyeV, lightV) * lightV.flux;
		if (L.Black())
			return 0.f;
		L *= fConnect;
		// Evaluate factors for eye path weighting
		eyeV.pdf = eyeV.bsdf->Pdf(tspack, eyeV.wi, eyeV.wo);
		eyeV.rr = min<float>(1.f, eyeV.f.filter(tspack) *
			eyeV.coso / eyeV.pdf);
		eyeV.rrR = min<float>(1.f, eyeV.f.filter(tspack) *
			eyeV.cosi / eyeV.pdfR);
		eyeV.dAWeight = lightV.pdf * eyeV.cosi / eyeV.d2;
		if (nEye > 1) {
			eWeight = eye[nEye - 2].dAWeight;
			eye[nEye - 2].dAWeight = eyeV.pdf *
				eye[nEye - 2].cosi / eye[nEye - 2].d2;
			eyeV.dARWeight = eye[nEye - 2].pdfR *
				eyeV.coso / eye[nEye - 2].d2;
		}
		// Evaluate factors for light path weighting
		lightV.pdfR = lightV.bsdf->Pdf(tspack, lightV.wo, lightV.wi);
		lightV.rr = min<float>(1.f, lightV.f.filter(tspack) *
			lightV.coso / lightV.pdf);
		lightV.rrR = min<float>(1.f, lightV.f.filter(tspack) *
			lightV.cosi / lightV.pdfR);
		lightV.dARWeight = eyeV.pdfR * lightV.coso / lightV.d2;
		if (nLight > 1) {
			lWeight = light[nLight - 2].dARWeight;
			light[nLight - 2].dARWeight = lightV.pdfR *
				light[nLight - 2].coso / light[nLight - 2].d2;
			lightV.dAWeight = light[nLight - 2].pdf *
				lightV.cosi / light[nLight - 2].d2;
		}
	} else if (nEye > 1) { // Evaluate eye path without light path
		BidirVertex &v(eye[nEye - 1]);
		L = eye[nEye - 2].flux;
		// Evaluate factors for path weighting
		v.dAWeight = v.ePdf;
		v.flags = BxDFType(~BSDF_SPECULAR);
		v.pdf = v.eBsdf->Pdf(tspack, Vector(v.ns), v.wo, v.flags);
		eWeight = eye[nEye - 2].dAWeight;
		eye[nEye - 2].dAWeight = v.pdf *
			eye[nEye - 2].cosi / eye[nEye - 2].d2;
		v.dARWeight = eye[nEye - 2].pdfR * v.coso / eye[nEye - 2].d2;
	} else if (nLight > 1) { // Evaluate light path without eye path
		BidirVertex &v(light[nLight - 1]);
		L = light[nLight - 2].flux;//FIXME: not implemented yet
		// Evaluate factors for path weighting
		v.dARWeight = v.ePdf;
		v.flags = BxDFType(~BSDF_SPECULAR);
		v.pdfR = v.eBsdf->Pdf(tspack, v.wo, Vector(v.ns), v.flags);
		lWeight = light[nLight - 2].dARWeight;
		light[nLight - 2].dARWeight = v.pdfR *
			light[nLight - 2].coso / light[nLight - 2].d2;
		v.dAWeight = light[nLight - 2].pdf * v.cosi / light[nLight - 2].d2;
	} else
		return 0.f;
	float w = 1.f / weightPath(eye, nEye, eyeDepth, light, nLight, lightDepth,
		pdfLightDirect, isLightDirect);
	*weight = w;
	L *= w;
	if (nEye > 1)
		eye[nEye - 2].dAWeight = eWeight;
	if (nLight > 1)
		light[nLight - 2].dARWeight = lWeight;
	return L;
}

static bool getLightHit(const TsPack *tspack, const Scene *scene, vector<BidirVertex> &eyePath,
	int length, int eyeDepth, int lightDepth, SWCSpectrum *Le, float *weight)
{
	BidirVertex &v(eyePath[length - 1]);
	if (!(v.ePdf > 0.f) || v.Le.Black())
		return false;
	*Le = v.Le;
	vector<BidirVertex> path(0);
	BidirVertex e(v);
	*Le *= evalPath(tspack, scene, eyePath, length, eyeDepth, path, 0, lightDepth,
		v.ePdfDirect, false, weight);
	v = e;
	return !Le->Black();
}

static bool getEnvironmentLight(const TsPack *tspack, const Scene *scene,
	vector<BidirVertex> &eyePath, int length, int eyeDepth, int lightDepth,
	SWCSpectrum *Le, float *weight, float directWeight, float time)
{
	BidirVertex &v(eyePath[length - 1]);
	if (v.bsdf)
		return false;
	// The eye path has at least 2 vertices
	vector<BidirVertex> path(0);
	float totalWeight = 0.f;
	for (u_int lightNumber = 0; lightNumber < scene->lights.size(); ++lightNumber) {
		const Light *light = scene->lights[lightNumber];
		RayDifferential ray(eyePath[length - 2].p, eyePath[length - 2].wi);
		ray.time = time;
		v.Le = light->Le(tspack, scene, ray, eyePath[length - 2].ns,
			&v.eBsdf, &v.ePdf, &v.ePdfDirect);
		if (v.eBsdf == NULL || !(v.ePdf > 0.f) || v.Le.Black())
			continue;
		v.p = v.eBsdf->dgShading.p;
		v.ns = v.eBsdf->dgShading.nn;
		v.ng = v.ns;
		v.coso = AbsDot(v.wo, v.ng);
		v.ePdf /= scene->lights.size();
		v.ePdfDirect *= directWeight;
		// This can be overwritten as it won't be reused
		eyePath[length - 2].d2 = DistanceSquared(eyePath[length - 2].p, v.p);
		v.Le *= evalPath(tspack, scene, eyePath, length, eyeDepth, path, 0,
			lightDepth, v.ePdfDirect, false, weight);
		if (!v.Le.Black()) {
			*Le += v.Le;
			totalWeight += v.Le.filter(tspack) * *weight;
		}
	}
	if (totalWeight > 0.f)
		totalWeight /= Le->filter(tspack);
	*weight = totalWeight;
	return true;
}

static bool getDirectLight(const TsPack *tspack, const Scene *scene, vector<BidirVertex> &eyePath,
	int length, int eyeDepth, int lightDepth, const Light *light,
	float u0, float u1, float portal, SWCSpectrum *Ld, float *weight,
	Vector *w, float directWeight)
{
	vector<BidirVertex> lightPath(1);
	BidirVertex &vE(eyePath[length - 1]);
	BidirVertex &vL(lightPath[0]);
	VisibilityTester visibility;
	// Sample the chosen light
	if (!light->Sample_L(tspack, scene, vE.p, vE.ns, u0, u1, portal, &vL.bsdf,
		&vL.dAWeight, &vL.ePdfDirect, &visibility, Ld))
		return false;
	vL.p = vL.bsdf->dgShading.p;
	vL.ns = vL.bsdf->dgShading.nn;
	vL.ng = vL.ns;
	vL.wi = Vector(vL.ns);
	vL.cosi = AbsDot(vL.wi, vL.ng);
	vL.dAWeight /= scene->lights.size();
	vL.ePdfDirect *= directWeight;
	BidirVertex e(vE);
	*Ld *= evalPath(tspack, scene, eyePath, length, eyeDepth,
		lightPath, 1, lightDepth, vL.ePdfDirect, true, weight);
	vE = e;
	*Ld /= vL.ePdfDirect;
	if (length == 1)
		*w = -vL.wo;
	return !Ld->Black();
}

static bool getLight(const TsPack *tspack, const Scene *scene,
	vector<BidirVertex> &eyePath, int nEye, int eyeDepth,
	vector<BidirVertex> &lightPath, int nLight, int lightDepth,
	float lightDirectPdf, SWCSpectrum *Ll, float *weight)
{
	BidirVertex vE(eyePath[nEye - 1]), vL(lightPath[nLight - 1]);
	*Ll *= evalPath(tspack, scene, eyePath, nEye, eyeDepth,
		lightPath, nLight, lightDepth, lightDirectPdf, false, weight);
	eyePath[nEye - 1] = vE;
	lightPath[nLight - 1] = vL;
	return !Ll->Black();
}

SWCSpectrum BidirIntegrator::Li(const TsPack *tspack, const Scene *scene, const RayDifferential &ray,
	const Sample *sample, float *alpha) const
{
	SampleGuard guard(sample->sampler, sample);
	float nrContribs = 0.f;
	vector<BidirVertex> eyePath(maxEyeDepth), lightPath(maxLightDepth);
	SWCSpectrum L(0.f);
	float variance = 0.f;
	if (alpha)
		*alpha = 1.f;
	int numberOfLights = static_cast<int>(scene->lights.size());
	const float directWeight = (lightStrategy == SAMPLE_ONE_UNIFORM) ? 1.f / numberOfLights : 1.f;
	// Trace eye path
	BSDF *eyeBsdf;
	float eyePdf;
	//Jeanphi - Replace dummy .5f by a sampled value if needed
	SWCSpectrum We;
	if (!scene->camera->Sample_W(tspack, scene,
		sample->lensU, sample->lensV, .5f, &eyeBsdf, &eyePdf, &We))
		return nrContribs;	//FIXME not necessarily true if special sampling for direct connection to the eye
	We /= eyePdf;
	int nEye = generateEyePath(tspack, scene, eyeBsdf, sample, sampleEyeOffset, eyePath, directWeight, ray.time);
	// Light path cannot intersect camera (FIXME)
	eyePath[0].dARWeight = 0.f;
	// Choose light
	int lightNum = Floor2Int(sample->oneD[lightNumOffset][0] *
		numberOfLights);
	lightNum = min<int>(lightNum, numberOfLights - 1);
	Light *light = scene->lights[lightNum];
	const float *data = sample->twoD[lightPosOffset];
	const float component = sample->oneD[lightComponentOffset][0];
	BSDF *lightBsdf;
	float lightPdf;
	SWCSpectrum Le;
	int nLight = 0;
	if (light->Sample_L(tspack, scene, data[0], data[1], component, &lightBsdf,
		&lightPdf, &Le)) {
		lightPdf /= numberOfLights;
		Le /= lightPdf;
		nLight = generateLightPath(tspack, scene, lightBsdf, sample,
			sampleLightOffset, lightPath, ray.time);
	}
	float lightDirectPdf = 0.f;
	if (nLight > 0) {
		// Give the light point probability for the weighting
		// if the light is not delta
		if (lightPdf > 0.f && !light->IsDeltaLight())
			lightPath[0].dAWeight = lightPdf;
		else
			lightPath[0].dAWeight = 0.f;
		if (nLight > 1)
			lightDirectPdf = light->Pdf(lightPath[1].p, lightPath[1].ns,
				lightPath[1].wi) / //FIXME
				lightPath[0].d2 * lightPath[0].coso * directWeight;
	}
	// Get back normal image position
	float x, y;
	scene->camera->GetSamplePosition(eyePath[0].p, eyePath[0].wi, &x, &y);
	// Connect paths
	for (int i = 1; i <= nEye; ++i) {
		float weight;
		// Check eye path light intersection
		if (i > 1) {
			SWCSpectrum Lh(0.f);
			if (getEnvironmentLight(tspack, scene, eyePath, i, maxEyeDepth,
				maxLightDepth, &Lh, &weight, directWeight, ray.time)) {
				if (!Lh.Black()) {
					L += Lh;
					variance += weight * Lh.filter(tspack);
					++nrContribs;
				}
				break; //from now on the eye path does not intersect anything
			} else if (getLightHit(tspack, scene, eyePath, i, maxEyeDepth,
				maxLightDepth, &Lh, &weight)) {
				if (!Lh.Black()) {
					L += Lh;
					variance += weight * Lh.filter(tspack);
					++nrContribs;
				}
			}
		}
		// Do direct lighting
		SWCSpectrum Ld(0.f);
		Vector w;
		const float *directData = sample->sampler->GetLazyValues(const_cast<Sample *>(sample), sampleDirectOffset, i - 1);
		switch(lightStrategy) {
			case SAMPLE_ONE_UNIFORM: {
				float portal = directData[2] * numberOfLights;
				const int lightDirectNumber = min(Floor2Int(portal), numberOfLights - 1);
				portal -= lightDirectNumber;
				if (getDirectLight(tspack, scene, eyePath, i,
					maxEyeDepth, maxLightDepth,
					scene->lights[lightDirectNumber],
					directData[0], directData[1], portal, &Ld, &weight,
					&w, directWeight)) {
					if (!Ld.Black()) {
						if (i > 1) {
							L += Ld;
							variance += weight * Ld.filter(tspack);
						} else {
							Ld *= We;
							float xd, yd;
							scene->camera->GetSamplePosition(eyePath[0].p, w, &xd, &yd);
							XYZColor color(Ld.ToXYZ(tspack));
							sample->AddContribution(xd, yd, color, alpha ? *alpha : 1.f, weight, lightBufferId);
						}
						++nrContribs;
					}
				}
				break;
			}
			case SAMPLE_ALL_UNIFORM:
				for (u_int l = 0; l < scene->lights.size(); ++l) {
					if (getDirectLight(tspack, scene, eyePath, i,
						maxEyeDepth, maxLightDepth,
						scene->lights[l],
						directData[0], directData[1], directData[2], &Ld,
						&weight, &w, directWeight)) {
						if (!Ld.Black()) {
							if (i > 1) {
								L += Ld;
								variance += weight * Ld.filter(tspack);
							} else {
								Ld *= We;
								float xd, yd;
								scene->camera->GetSamplePosition(eyePath[0].p, w, &xd, &yd);
								XYZColor color(Ld.ToXYZ(tspack));
								sample->AddContribution(xd, yd, color, alpha ? *alpha : 1.f, weight, lightBufferId);
							}
							++nrContribs;
						}
					}
					directData += 3;
				}
				break;
			default:
				break;
		}
		if (nLight > 0) {
			// Compute direct lighting pdf for first light vertex link
			Vector wDirect(Normalize(lightPath[0].p - eyePath[i - 1].p));
			float directPdf = light->Pdf(eyePath[i - 1].p, eyePath[i - 1].ns,
				wDirect) / //FIXME
				wDirect.LengthSquared() *
				AbsDot(wDirect, lightPath[0].ns) * directWeight;
			// Connect to light subpath
			for (int j = 1; j <= nLight; ++j) {
				SWCSpectrum Ll(Le);
				if (getLight(tspack, scene, eyePath, i, maxEyeDepth,
					lightPath, j, maxLightDepth, directPdf, &Ll,
					&weight)) {
					if (!Ll.Black()) {
						if (i > 1) {
							L += Ll;
							variance += weight * Ll.filter(tspack);
						} else {
							Ll *= We;
							float xl, yl;
							scene->camera->GetSamplePosition(eyePath[0].p, Normalize(lightPath[j - 1].p - eyePath[i - 1].p), &xl, &yl);
							XYZColor color(Ll.ToXYZ(tspack));
							sample->AddContribution(xl, yl, color, alpha ? *alpha : 1.f, weight, lightBufferId);
						}
						++nrContribs;
					}
				}
				// Use general direct lighting pdf for next events
				if (j == 1)
					directPdf = lightDirectPdf;
			}
		}
	}
	L *= We;
	variance /= L.filter(tspack);
	XYZColor color(L.ToXYZ(tspack));
	sample->AddContribution(x, y, color, alpha ? *alpha : 1.f, variance, eyeBufferId);
	return nrContribs;
}

SurfaceIntegrator* BidirIntegrator::CreateSurfaceIntegrator(const ParamSet &params)
{
	int eyeDepth = params.FindOneInt("eyedepth", 8);
	int lightDepth = params.FindOneInt("lightdepth", 8);
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
	return new BidirIntegrator(eyeDepth, lightDepth, estrategy);
}

static DynamicLoader::RegisterSurfaceIntegrator<BidirIntegrator> r("bidirectional");
