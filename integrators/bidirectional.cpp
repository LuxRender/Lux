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
	BidirVertex() : pdf(0.f), pdfR(0.f), tPdf(1.f), tPdfR(1.f),
		dAWeight(0.f), dARWeight(0.f), rr(1.f), rrR(1.f), prob(1.f),
		flux(0.f), f(0.f), Le(0.f), bsdf(NULL), flags(BxDFType(0)),
		ePdf(0.f), ePdfDirect(0.f), eBsdf(NULL) {}
	float cosi, coso, pdf, pdfR, tPdf, tPdfR, dAWeight, dARWeight, rr, rrR, prob, d2;
	SWCSpectrum flux;
	SWCSpectrum f, Le;
	BSDF *bsdf;
	BxDFType flags;
	Vector wi, wo;
	Normal ng, ns;
	Point p;
	float  ePdf, ePdfDirect;
	BSDF *eBsdf;
	int eGroup;
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

static int generateEyePath(const TsPack *tspack, const Scene *scene, BSDF *bsdf,
	const Sample *sample, const BidirIntegrator &bidir,
	vector<BidirVertex> &vertices, float directWeight)
{
	if (vertices.size() == 0)
		return 0;
	RayDifferential ray;
	Intersection isect;
	ray.d = -Vector(bsdf->dgShading.nn);
	isect.dg.p = bsdf->dgShading.p;
	isect.dg.nn = bsdf->dgShading.nn;
	u_int nVerts = 0;
	const float dummy[] = {0.f, tspack->camera->IsLensBased() ? sample->imageX : sample->lensU, tspack->camera->IsLensBased() ? sample->imageY : sample->lensV, 0.5f};
	const float *data = (const float *)&dummy;
	while (true) {
		// Find next vertex in path and initialize _vertices_
		BidirVertex &v = vertices[nVerts];
		if (nVerts == 0) {
			v.bsdf = bsdf;
		} else {
			v.bsdf = isect.GetBSDF(tspack, ray);
			RayDifferential r(vertices[nVerts - 1].p, vertices[nVerts - 1].wi);
			r.time = tspack->time;
			v.Le = isect.Le(tspack, r, vertices[nVerts - 1].ng,
				&v.eBsdf, &v.ePdf, &v.ePdfDirect);
			if (v.eBsdf) {
				v.ePdf /= scene->lights.size();
				v.ePdfDirect *= directWeight;
				v.eGroup = isect.arealight->group;
			}
		}
		v.wo = -ray.d;
		v.p = isect.dg.p;
		v.ng = isect.dg.nn;
		v.ns = v.bsdf->dgShading.nn;
		v.coso = AbsDot(v.wo, v.ng);
		++nVerts;
		// Possibly terminate bidirectional path sampling
		if (nVerts == vertices.size())
			break;
		if (!v.bsdf->Sample_f(tspack, v.wo, &v.wi, data[1], data[2],
			data[3], &v.f, &v.pdfR, BSDF_ALL, &v.flags, &v.pdf,
			true))
			break;
		bool through = false;
		if (v.flags != (BSDF_TRANSMISSION | BSDF_SPECULAR) ||
			Dot(v.wi, v.wo) > SHADOW_RAY_EPSILON - 1.f ||
			!(v.bsdf->Pdf(tspack, v.wi, v.wo, BxDFType(BSDF_TRANSMISSION | BSDF_SPECULAR)) > 0.f)) {
			v.cosi = AbsDot(v.wi, v.ng);
			const float cosins = AbsDot(v.wi, v.ns);
			v.flux = v.f * cosins;
			v.prob = v.pdfR;
			v.f *= cosins / v.cosi;
			v.rrR = min(1.f, max(bidir.eyeThreshold, v.flux.filter(tspack) / v.prob));
			if (nVerts > 3) {
				if (v.rrR < data[0])
					break;
				v.prob *= v.rrR;
			}
			v.rr = min(1.f, max(bidir.lightThreshold,
				v.f.filter(tspack) * v.coso / v.pdf));
			if (nVerts > 1) {
				v.flux *= vertices[nVerts - 2].flux;
				v.prob *= vertices[nVerts - 2].prob;
			}
		} else {
			const float cosins = AbsDot(v.wi, v.ns);
			vertices[nVerts - 2].flux *= v.f * cosins;
			vertices[nVerts - 2].prob *= v.pdfR;
			vertices[nVerts - 2].tPdfR *= v.pdfR;
			vertices[nVerts - 1].tPdf *= v.pdf;
			--nVerts;
			through = true;
		}
		// Initialize _ray_ for next segment of path
		ray = RayDifferential(v.p, v.wi);
		ray.time = tspack->time;
		if (nVerts == 1 && !through)
			tspack->camera->ClampRay(ray);
		if (!scene->Intersect(ray, &isect)) {
			vertices[nVerts].wo = -ray.d;
			vertices[nVerts].bsdf = NULL;
			++nVerts;
			break;
		}
		data = sample->sampler->GetLazyValues(const_cast<Sample *>(sample), bidir.sampleEyeOffset, nVerts);
	}
	// Initialize additional values in _vertices_
	for (u_int i = 0; i < nVerts - 1; ++i) {
		if (vertices[i + 1].bsdf == NULL)
			break;
		vertices[i].d2 =
			DistanceSquared(vertices[i].p, vertices[i + 1].p);
		vertices[i + 1].dARWeight = vertices[i].pdfR * vertices[i].tPdfR *
			vertices[i + 1].coso / vertices[i].d2;
		vertices[i].dAWeight = vertices[i + 1].pdf * vertices[i + 1].tPdf *
			vertices[i].cosi / vertices[i].d2;
	}
	return nVerts;
}

static int generateLightPath(const TsPack *tspack, const Scene *scene,
	BSDF *bsdf, const Sample *sample, const BidirIntegrator &bidir,
	vector<BidirVertex> &vertices)
{
	if (vertices.size() == 0)
		return 0;
	RayDifferential ray;
	Intersection isect;
	ray.d = -Vector(bsdf->dgShading.nn);
	isect.dg.p = bsdf->dgShading.p;
	isect.dg.nn = bsdf->dgShading.nn;
	u_int nVerts = 0;
	while (true) {
		BidirVertex &v = vertices[nVerts];
		const float *data = sample->sampler->GetLazyValues(const_cast<Sample *>(sample), bidir.sampleLightOffset, nVerts);
		if (nVerts == 0) {
			v.bsdf = bsdf;
		} else {
			v.bsdf = isect.GetBSDF(tspack, ray);
		}
		v.wi = -ray.d;
		v.p = isect.dg.p;
		v.ng = isect.dg.nn;
		v.ns = v.bsdf->dgShading.nn;
		v.cosi = AbsDot(v.wi, v.ng);
		++nVerts;
		// Possibly terminate bidirectional path sampling
		if (nVerts == vertices.size())
			break;
		if (!v.bsdf->Sample_f(tspack, v.wi, &v.wo, data[1], data[2],
			data[3], &v.f, &v.pdf, BSDF_ALL, &v.flags, &v.pdfR))
			break;
		if (v.flags != (BSDF_TRANSMISSION | BSDF_SPECULAR) ||
			Dot(v.wi, v.wo) > SHADOW_RAY_EPSILON - 1.f ||
			!(v.bsdf->Pdf(tspack, v.wi, v.wo, BxDFType(BSDF_TRANSMISSION | BSDF_SPECULAR)) > 0.f)) {
			v.flux = v.f;
			v.coso = AbsDot(v.wo, v.ng);
			const float cosins = AbsDot(v.wi, v.ns);
			v.flux *= v.coso * cosins;
			v.prob = v.pdf * v.cosi;
			v.rr = min(1.f, max(bidir.lightThreshold, v.flux.filter(tspack) / v.prob));
			if (nVerts > 3) {
				if (v.rr < data[0])
					break;
				v.prob *= v.rr;
			}
			v.rrR = min(1.f, max(bidir.eyeThreshold,
				v.f.filter(tspack) * cosins / v.pdfR));
			if (nVerts > 1) {
				v.flux *= vertices[nVerts - 2].flux;
				v.prob *= vertices[nVerts - 2].prob;
			}
		} else {
			const float cosins = AbsDot(v.wi, v.ns);
			vertices[nVerts - 2].flux *= v.f * cosins;
			// No need to do '/ v.cosi * v.coso' since cosi==coso
			vertices[nVerts - 2].prob *= v.pdf;
			vertices[nVerts - 2].tPdf *= v.pdf;
			vertices[nVerts - 1].tPdfR *= v.pdfR;
			--nVerts;
		}
		// Initialize _ray_ for next segment of path
		ray = RayDifferential(v.p, v.wo);
		ray.time = tspack->time;
		if (!scene->Intersect(ray, &isect))
			break;
	}
	// Initialize additional values in _vertices_
	for (u_int i = 0; i < nVerts - 1; ++i) {
		vertices[i].d2 =
			DistanceSquared(vertices[i].p, vertices[i + 1].p);
		vertices[i + 1].dAWeight = vertices[i].pdf * vertices[i].tPdf *
			vertices[i + 1].cosi / vertices[i].d2;
		vertices[i].dARWeight = vertices[i + 1].pdfR * vertices[i + 1].tPdfR *
			vertices[i].coso / vertices[i].d2;
	}
	return nVerts;
}

// Connecting factor
static float G(float cosins, float coso, float d2)
{
	return cosins * coso / d2;
}

// Visibility test
static bool visible(const TsPack *tspack, const Scene *scene, const Point &P0,
	const Point &P1, float *pdf, float *pdfR, SWCSpectrum *f)
{
	VisibilityTester vt;
	vt.SetSegment(P0, P1, tspack->time);
	return vt.TestOcclusion(tspack, scene, f, pdf, pdfR);
}

// Weighting of path with regard to alternate methods of obtaining it
static float weightPath(const vector<BidirVertex> &eye, int nEye, int eyeDepth,
	const vector<BidirVertex> &light, int nLight, int lightDepth,
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
		if (nEye > 4)
			pDirect /= eye[nEye - 2].rrR;
		weight += pDirect * pDirect;
	}
	// Find other paths by extending light path toward eye path
	for (int i = nEye - 1; i >= max(0, nEye - (lightDepth - nLight)); --i) {
		// Exit if the path is impossible
		if (!(eye[i].dARWeight > 0.f && eye[i].dAWeight > 0.f))
			break;
		// Compute new path relative probability
		p *= eye[i].dAWeight / eye[i].dARWeight;
		// Adjust for round robin termination
		if (i > 3)
			p /= eye[i - 1].rrR;
		if (nLight + nEye - i > 3) {
			if (i == nEye - 1)
				p *= light[nLight - 1].rr;
			else
				p *= eye[i + 1].rr;
		}
		// The path can only be obtained if none of the vertices
		// is specular
		if ((eye[i].flags & BSDF_SPECULAR) == 0 &&
			(i == 0 || (eye[i - 1].flags & BSDF_SPECULAR) == 0))
			weight += p * p;
	}
	// Reinitialize p to search paths in the other direction
	p = pBase;
	// Find other paths by extending eye path toward light path
	for (int i = nLight - 1; i >= max(0, nLight - (eyeDepth - nEye)); --i) {
		// Exit if the path is impossible
		if (!(light[i].dARWeight > 0.f && light[i].dAWeight > 0.f))
				break;
		// Compute new path relative probability
		p *= light[i].dARWeight / light[i].dAWeight;
		// Adjust for round robin termination
		if (i > 3)
			p /= light[i - 1].rr;
		if (nEye + nLight - i > 3) {
			if (i == nLight - 1)
				p *= eye[nEye - 1].rrR;
			else
				p *= light[i + 1].rrR;
		}
		// The path can only be obtained if none of the vertices
		// is specular
		if ((light[i].flags & BSDF_SPECULAR) == 0 &&
			(i == 0 || (light[i - 1].flags & BSDF_SPECULAR) == 0))
			weight += p * p;
		// Check for direct path
		// Light path has at least 2 vertices here
		if (i == 1) {
			float pDirect = p * pdfLightDirect / fabsf(light[0].dAWeight);
			// The path can only be obtained if the second vertex
			// is not specular.
			// Even if the light source vertex is specular,
			// the special sampling for direct lighting will get it
			if ((light[1].flags & BSDF_SPECULAR) == 0)
				weight += pDirect * pDirect;
		}
	}
	return weight;
}

static bool evalPath(const TsPack *tspack, const Scene *scene,
	const BidirIntegrator &bidir,
	vector<BidirVertex> &eye, int nEye,
	vector<BidirVertex> &light, int nLight,
	float pdfLightDirect, bool isLightDirect, float *weight, SWCSpectrum *L)
{
	const int eyeDepth = bidir.maxEyeDepth;
	const int lightDepth = bidir.maxLightDepth;
	const float eyeThreshold = bidir.eyeThreshold;
	const float lightThreshold = bidir.lightThreshold;
	*weight = 0.f;
	float eWeight = 0.0f, lWeight = 0.0f;
	float prob;
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
			return false;
		lightV.flags = BxDFType(~BSDF_SPECULAR);
		lightV.wo = -eyeV.wi;
		lightV.pdf = lightV.bsdf->Pdf(tspack, lightV.wi, lightV.wo, lightV.flags);
		if (!(lightV.pdf > 0.f))
			return false;
		eyeV.f = eyeV.bsdf->f(tspack, eyeV.wi, eyeV.wo, eyeV.flags);
		if (eyeV.f.Black())
			return false;
		lightV.f = lightV.bsdf->f(tspack, lightV.wi, lightV.wo, lightV.flags);
		if (lightV.f.Black())
			return false;
		lightV.tPdf = 1.f;
		eyeV.tPdfR = 1.f;
		if (!visible(tspack, scene, lightV.p, eyeV.p, &lightV.tPdf,
			&eyeV.tPdfR, L))
			return false;
		// Prepare eye vertex for connection
		eyeV.cosi = AbsDot(eyeV.wi, eyeV.ng);
		eyeV.d2 = DistanceSquared(eyeV.p, lightV.p);
		if (eyeV.d2 < SHADOW_RAY_EPSILON)
			return false;
		const float ecosins = AbsDot(eyeV.wi, eyeV.ns);
		eyeV.flux = eyeV.f; // No pdf as it is a direct connection
		if (nEye > 1) {
			eyeV.flux *= eye[nEye - 2].flux;
			eyeV.prob = eye[nEye - 2].prob;
		}
		// Prepare light vertex for connection
		lightV.coso = AbsDot(lightV.wo, lightV.ng);
		lightV.d2 = eyeV.d2;
		const float lcosins = AbsDot(lightV.wi, lightV.ns);
		lightV.flux = lightV.f * lcosins; // No pdf as it is a direct connection
		lightV.prob = lightV.cosi;
		if (nLight > 1) {
			lightV.flux *= light[nLight - 2].flux;
			lightV.prob *= light[nLight - 2].prob;
		}
		// Connect eye and light vertices
		prob = eyeV.prob * lightV.prob;
		if (!(prob > 0.f))
			return false;
		*L *= eyeV.flux;
		*L *= lightV.flux;
		*L *= G(ecosins, lightV.coso, eyeV.d2);
		if (L->Black())
			return false;
		// Evaluate factors for eye path weighting
		eyeV.pdf = eyeV.bsdf->Pdf(tspack, eyeV.wi, eyeV.wo, eyeV.flags);
		eyeV.rr = min(1.f, max(lightThreshold, eyeV.f.filter(tspack) *
			ecosins / eyeV.cosi * eyeV.coso / eyeV.pdf));
		eyeV.rrR = min(1.f, max(eyeThreshold, eyeV.f.filter(tspack) *
			ecosins / eyeV.pdfR));
		eyeV.dAWeight = lightV.pdf * lightV.tPdf * eyeV.cosi / eyeV.d2;
		if (nEye > 1) {
			eWeight = eye[nEye - 2].dAWeight;
			eye[nEye - 2].dAWeight = eyeV.pdf * eyeV.tPdf *
				eye[nEye - 2].cosi / eye[nEye - 2].d2;
			eyeV.dARWeight = eye[nEye - 2].pdfR * eye[nEye - 2].tPdfR *
				eyeV.coso / eye[nEye - 2].d2;
		}
		// Evaluate factors for light path weighting
		lightV.pdfR = lightV.bsdf->Pdf(tspack, lightV.wo, lightV.wi, lightV.flags);
		lightV.rr = min(1.f, max(lightThreshold, lightV.f.filter(tspack) *
			lcosins / lightV.cosi * lightV.coso / lightV.pdf));
		lightV.rrR = min(1.f, max(eyeThreshold, lightV.f.filter(tspack) *
			lcosins / lightV.pdfR));
		lightV.dARWeight = eyeV.pdfR * eyeV.tPdfR * lightV.coso / lightV.d2;
		if (nLight > 1) {
			lWeight = light[nLight - 2].dARWeight;
			light[nLight - 2].dARWeight = lightV.pdfR * lightV.tPdfR *
				light[nLight - 2].coso / light[nLight - 2].d2;
			lightV.dAWeight = light[nLight - 2].pdf * light[nLight - 2].tPdf *
				lightV.cosi / light[nLight - 2].d2;
		}
	} else if (nEye > 1) { // Evaluate eye path without light path
		BidirVertex &v(eye[nEye - 1]);
		// Evaluate factors for path weighting
		v.dAWeight = v.ePdf;
		v.flags = BxDFType(~BSDF_SPECULAR);
		v.pdf = v.eBsdf->Pdf(tspack, Vector(v.ng), v.wo, v.flags);
		if (!(v.pdf > 0.f))
			return false;
		prob = eye[nEye - 2].prob;
		if (!(prob > 0.f))
			return false;
		*L *= eye[nEye - 2].flux;
		if (L->Black())
			return false;
		eWeight = eye[nEye - 2].dAWeight;
		// Don't take v.tPdf into account because the light ray
		// hasn't traveled yet
		eye[nEye - 2].dAWeight = v.pdf *
			eye[nEye - 2].cosi / eye[nEye - 2].d2;
	} else if (nLight > 1) { // Evaluate light path without eye path
		BidirVertex &v(light[nLight - 1]);
		// Evaluate factors for path weighting
		v.dARWeight = v.ePdf;
		v.flags = BxDFType(~BSDF_SPECULAR);
		v.pdfR = v.eBsdf->Pdf(tspack, v.wo, Vector(v.ns), v.flags);
		if (!(v.pdfR > 0.f))
			return false;
		prob = light[nLight - 2].prob;
		if (!(prob > 0.f))
			return false;
		*L *= light[nLight - 2].flux;//FIXME: not implemented yet
		if (L->Black())
			return false;
		lWeight = light[nLight - 2].dARWeight;
		// Don't take v.tPdfR into account because the eye ray
		// hasn't traveled yet
		light[nLight - 2].dARWeight = v.pdfR *
			light[nLight - 2].coso / light[nLight - 2].d2;
	} else
		return false;
	const float w = weightPath(eye, nEye, eyeDepth, light, nLight,
		lightDepth, pdfLightDirect, isLightDirect);
	*weight = 1.f / w;
	*L /= w * prob;
	if (nEye > 1)
		eye[nEye - 2].dAWeight = eWeight;
	if (nLight > 1)
		light[nLight - 2].dARWeight = lWeight;
	return true;
}

static bool getLightHit(const TsPack *tspack, const Scene *scene,
	const BidirIntegrator &bidir,
	vector<BidirVertex> &eyePath, int length,
	vector<SWCSpectrum> &Le, vector<float> &weight)
{
	BidirVertex &v(eyePath[length - 1]);
	if (!(v.ePdf > 0.f) || v.Le.Black())
		return false;
	vector<BidirVertex> path(0);
	BidirVertex e(v);
	float totalWeight;
	if (!evalPath(tspack, scene, bidir, eyePath, length, path, 0,
		v.ePdfDirect, false, &totalWeight, &(v.Le))) {
		v = e;
		return false;
	}
	Le[v.eGroup] += v.Le;
	weight[v.eGroup] += v.Le.filter(tspack) * totalWeight;
	v = e;
	return true;
}

static bool getEnvironmentLight(const TsPack *tspack, const Scene *scene,
	const BidirIntegrator &bidir,
	vector<BidirVertex> &eyePath, int length, float directWeight,
	vector<SWCSpectrum> &Le, vector<float> &weight,
	int &nrContribs)
{
	BidirVertex &v(eyePath[length - 1]);
	if (v.bsdf)
		return false;
	// The eye path has at least 2 vertices
	vector<BidirVertex> path(0);
	float totalWeight = 0.f;
	for (u_int lightNumber = 0; lightNumber < scene->lights.size(); ++lightNumber) {
		const Light *light = scene->lights[lightNumber];
		if (!light->IsEnvironmental())
			continue;
		RayDifferential ray(eyePath[length - 2].p,
			eyePath[length - 2].wi);
		ray.time = tspack->time;
		v.Le = light->Le(tspack, scene, ray, eyePath[length - 2].ng,
			&v.eBsdf, &v.ePdf, &v.ePdfDirect);
		if (v.eBsdf == NULL || v.Le.Black())
			continue;
		v.p = v.eBsdf->dgShading.p;
		v.ns = v.eBsdf->dgShading.nn;
		v.ng = v.ns;
		v.coso = AbsDot(v.wo, v.ng);
		v.ePdf /= scene->lights.size();
		v.ePdfDirect *= directWeight;
		// This can be overwritten as it won't be reused
		eyePath[length - 2].d2 =
			DistanceSquared(eyePath[length - 2].p, v.p);
		v.dARWeight = eyePath[length - 2].pdfR * eyePath[length - 2].tPdfR *
			v.coso / eyePath[length - 2].d2;
		if (evalPath(tspack, scene, bidir, eyePath, length, path, 0,
			v.ePdfDirect, false, &totalWeight, &(v.Le))) {
			Le[light->group] += v.Le;
			weight[light->group] += totalWeight *
				Le[light->group].filter(tspack);
			++nrContribs;
		}
	}
	v.bsdf = NULL;
	return true;
}

static bool getDirectLight(const TsPack *tspack, const Scene *scene,
	const BidirIntegrator &bidir,
	vector<BidirVertex> &eyePath, int length,
	const Light *light, float u0, float u1, float portal,
	float directWeight, const SWCSpectrum &We,
	const Sample *sample, vector<SWCSpectrum> &Ld, vector<float> &weight)
{
	vector<BidirVertex> lightPath(1);
	BidirVertex &vE(eyePath[length - 1]);
	BidirVertex &vL(lightPath[0]);
	VisibilityTester visibility;
	SWCSpectrum L;
	// Sample the chosen light
	if (!light->Sample_L(tspack, scene, vE.p, vE.ng, u0, u1, portal,
		&vL.bsdf, &vL.dAWeight, &vL.ePdfDirect, &visibility, &L))
		return false;
	vL.p = vL.bsdf->dgShading.p;
	vL.ns = vL.bsdf->dgShading.nn;
	vL.ng = vL.ns;
	vL.wi = Vector(vL.ns);
	vL.cosi = AbsDot(vL.wi, vL.ng);
	vL.dAWeight /= scene->lights.size();
	if (light->IsDeltaLight())
		vL.dAWeight = -vL.dAWeight;
	vL.ePdfDirect *= directWeight;
	BidirVertex e(vE);
	float totalWeight;
	if (!evalPath(tspack, scene, bidir, eyePath, length,
		lightPath, 1, vL.ePdfDirect, true, &totalWeight, &L)) {
		vE = e;
		return false;
	}
	L /= vL.ePdfDirect;
	if (length > 1) {
		Ld[light->group] += L;
		weight[light->group] += L.filter(tspack) * totalWeight;
	} else {
		const float distance = sqrtf(lightPath[0].d2);
		float xd, yd;
		if (!tspack->camera->GetSamplePosition(vE.p, vE.wi, distance, &xd, &yd)) {
			vE = e;
			return false;
		}
		L *= We;
		XYZColor color(L.ToXYZ(tspack));
		const float alpha = light->IsEnvironmental() ? 0.f : 1.f;
		sample->AddContribution(xd, yd, color, alpha, distance, totalWeight, bidir.lightBufferId, light->group);
	}
	vE = e;
	return true;
}

static bool getLight(const TsPack *tspack, const Scene *scene,
	const BidirIntegrator &bidir,
	vector<BidirVertex> &eyePath, int nEye,
	vector<BidirVertex> &lightPath, int nLight,
	float lightDirectPdf, SWCSpectrum *Ll, float *weight)
{
	BidirVertex vE(eyePath[nEye - 1]), vL(lightPath[nLight - 1]);
	bool result = evalPath(tspack, scene, bidir, eyePath, nEye,
		lightPath, nLight, lightDirectPdf, false, weight, Ll);
	eyePath[nEye - 1] = vE;
	lightPath[nLight - 1] = vL;
	return result;
}

int BidirIntegrator::Li(const TsPack *tspack, const Scene *scene,
	const Sample *sample) const
{
	SampleGuard guard(sample->sampler, sample);
	int nrContribs = 0;
	vector<BidirVertex> eyePath(maxEyeDepth), lightPath(maxLightDepth);
	float alpha = 1.f;
	const int numberOfLights = static_cast<int>(scene->lights.size());
	const float directWeight = (lightStrategy == SAMPLE_ONE_UNIFORM) ?
		1.f / numberOfLights : 1.f;
	// Trace eye path
	BSDF *eyeBsdf;
	float eyePdf;
	SWCSpectrum We;
	const float posX = tspack->camera->IsLensBased() ? sample->lensU : sample->imageX;
	const float posY = tspack->camera->IsLensBased() ? sample->lensV : sample->imageY;
	//Jeanphi - Replace dummy .5f by a sampled value if needed
	if (!tspack->camera->Sample_W(tspack, scene,
		posX, posY, .5f, &eyeBsdf, &eyePdf, &We))
		return nrContribs;	//FIXME not necessarily true if special sampling for direct connection to the eye
	We /= eyePdf;
	int nEye = generateEyePath(tspack, scene, eyeBsdf, sample,
		*this, eyePath, directWeight);
	// Light path cannot intersect camera (FIXME)
	eyePath[0].dARWeight = 0.f;
	// Choose light
	const int nGroups = scene->lightGroups.size();
	const int lightNum = min(Floor2Int(sample->oneD[lightNumOffset][0] *
		numberOfLights), numberOfLights - 1);
	const Light *light = scene->lights[lightNum];
	const int lightGroup = light->group;
	const float *data = sample->twoD[lightPosOffset];
	const float component = sample->oneD[lightComponentOffset][0];
	BSDF *lightBsdf;
	float lightPdf;
	SWCSpectrum Le;
	int nLight = 0;
	if (light->Sample_L(tspack, scene, data[0], data[1], component,
		&lightBsdf, &lightPdf, &Le)) {
		lightPdf /= numberOfLights;
		// Divide by Pdf because this value isn't used when the eye
		// ray hits a light source, only for light paths
		Le /= lightPdf;
		nLight = generateLightPath(tspack, scene, lightBsdf, sample,
			*this, lightPath);
	}
	float lightDirectPdf = 0.f;
	if (nLight > 0) {
		// Give the light point probability for the weighting
		// if the light is not delta
		lightPath[0].dAWeight = max(lightPdf, 0.f);
		if (light->IsDeltaLight())
			lightPath[0].dAWeight = -lightPath[0].dAWeight;
		if (nLight > 1)
			lightDirectPdf = light->Pdf(lightPath[1].p,
				lightPath[1].ng, lightPath[0].p,
				lightPath[0].ng) * directWeight;
	}
	vector<SWCSpectrum> vecL(nGroups, SWCSpectrum(0.f));
	SWCSpectrum &L(vecL[lightGroup]);
	vector<float> vecV(nGroups, 0.f);
	float &variance(vecV[lightGroup]);
	// Connect paths
	for (int i = 1; i <= nEye; ++i) {
		// Check eye path light intersection
		if (i > 1) {
			if (getEnvironmentLight(tspack, scene, *this,
				eyePath, i, directWeight, vecL, vecV,
				nrContribs)) {
				if (i == 2)
					alpha = 0.f; // Remove directly visible environment to allow compositing
				break; //from now on the eye path does not intersect anything
			} else if (getLightHit(tspack, scene, *this, eyePath, i,
				vecL, vecV))
				++nrContribs;
		}
		// Do direct lighting
		const float *directData = sample->sampler->GetLazyValues(const_cast<Sample *>(sample), sampleDirectOffset, i - 1);
		switch(lightStrategy) {
			case SAMPLE_ONE_UNIFORM: {
				float portal = directData[2] * numberOfLights;
				const int lightDirectNumber =
					min(Floor2Int(portal),
					numberOfLights - 1);
				portal -= lightDirectNumber;
				if (getDirectLight(tspack, scene, *this,
					eyePath, i,
					scene->lights[lightDirectNumber],
					directData[0], directData[1], portal,
					directWeight, We, sample, vecL, vecV)) {
					++nrContribs;
				}
				break;
			}
			case SAMPLE_ALL_UNIFORM:
				for (u_int l = 0; l < scene->lights.size(); ++l) {
					if (getDirectLight(tspack, scene, *this,
						eyePath, i, scene->lights[l],
						directData[0], directData[1],
						directData[2], directWeight,
						We, sample, vecL, vecV)) {
						++nrContribs;
					}
					directData += 3;
				}
				break;
			default:
				break;
		}
		if (nLight > 0) {
			// Compute direct lighting pdf for first light vertex
			float directPdf = light->Pdf(eyePath[i - 1].p,
				eyePath[i - 1].ng, lightPath[0].p,
				lightPath[0].ng) * directWeight;
			// Connect to light subpath
			for (int j = 1; j <= nLight; ++j) {
				if ((i == nEye || eyePath[i - 1].flags & (BSDF_DIFFUSE | BSDF_GLOSSY)) && (j == nLight || lightPath[j - 1].flags & (BSDF_DIFFUSE | BSDF_GLOSSY))) {
				SWCSpectrum Ll(Le);
				float weight;
				if (getLight(tspack, scene, *this,
					eyePath, i, lightPath, j, directPdf,
					&Ll, &weight)) {
					if (i > 1) {
						L += Ll;
						variance += weight *
							Ll.filter(tspack);
					} else {
						const float d = Distance(lightPath[j - 1].p, eyePath[0].p);
						float xl, yl;
						if (tspack->camera->GetSamplePosition(eyePath[0].p, (lightPath[j - 1].p - eyePath[0].p) / d, d, &xl, &yl)) {
							Ll *= We;
							XYZColor color(Ll.ToXYZ(tspack));
							const float a = (j == 1 && light->IsEnvironmental()) ? 0.f : 1.f;
							sample->AddContribution(xl, yl, color, a, d, weight, lightBufferId, lightGroup);
						}
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
	const float d = (nEye > 1 && eyePath[1].bsdf) ? eyePath[0].d2 : INFINITY;
	float xl, yl;
	if (!tspack->camera->GetSamplePosition(eyePath[0].p, eyePath[0].wi, sqrtf(eyePath[0].d2), &xl, &yl))
		return nrContribs;
	for (int i = 0; i < nGroups; ++i) {
		if (!vecL[i].Black())
			vecV[i] /= vecL[i].filter(tspack);
		vecL[i] *= We;
		XYZColor color(vecL[i].ToXYZ(tspack));
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
	return new BidirIntegrator(eyeDepth, lightDepth, eyeThreshold, lightThreshold, estrategy, debug);
}

static DynamicLoader::RegisterSurfaceIntegrator<BidirIntegrator> r("bidirectional");
