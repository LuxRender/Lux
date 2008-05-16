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
#include "light.h"
#include "paramset.h"

using namespace lux;

// Bidirectional Method Definitions
void BidirIntegrator::RequestSamples(Sample *sample, const Scene *scene)
{
	lightNumOffset = sample->Add1D(1);
	lightPosOffset = sample->Add2D(1);
	lightDirOffset = sample->Add2D(1);
	vector<u_int> structure;
	structure.push_back(2);	//light position
	structure.push_back(1);	//light number
	structure.push_back(2);	//bsdf sampling for light
	structure.push_back(1);	//bsdf component for light
	sampleDirectOffset = sample->AddxD(structure, maxEyeDepth);
	structure.clear();
	structure.push_back(1);	//continue eye
	structure.push_back(2);	//bsdf sampling for eye path
	structure.push_back(1);	//bsdf component for eye path
	sampleEyeOffset = sample->AddxD(structure, maxEyeDepth);
	structure.clear();
	structure.push_back(1); //continue light
	structure.push_back(2); //bsdf sampling for light path
	structure.push_back(1); //bsdf component for light path
	sampleLightOffset = sample->AddxD(structure, maxLightDepth);
}
static int generateLightPath(const Scene *scene, BSDF *bsdf,
	const Sample *sample, const int sampleOffset,
	vector<BidirVertex> &vertices)
{
	RayDifferential ray;
	Intersection isect;
	int nVerts = 0;
	while (nVerts < (int)(vertices.size())) {
		BidirVertex &v = vertices[nVerts];
		if (nVerts == 0) {
			v.wi = Vector(bsdf->dgShading.nn);
			v.bsdf = bsdf;
			v.p = bsdf->dgShading.p;
			v.ng = bsdf->dgShading.nn;
		} else {
			v.wi = -ray.d;
			v.bsdf = isect.GetBSDF(ray);
			v.p = isect.dg.p;
			v.ng = isect.dg.nn;
		}
		v.ns = bsdf->dgShading.nn;
		const float *data = sample->sampler->GetLazyValues(const_cast<Sample *>(sample), sampleOffset, nVerts);
		// Possibly terminate bidirectional path sampling
		v.f = v.bsdf->Sample_f(v.wi, &v.wo, data[1], data[2], data[3],
			 &v.bsdfWeight, BSDF_ALL, &v.flags);
		++nVerts;
		if (v.bsdfWeight == 0.f || v.f.Black())
			break;
		if (nVerts > 3) {
			const float q = min(1.,
				v.f.y() * (AbsDot(v.wo, v.ns) / v.bsdfWeight));
			if (q < data[0])
				break;
			v.rrWeight = q;
		}
		// Initialize _ray_ for next segment of path
		ray = RayDifferential(v.p, v.wo);
		if (!scene->Intersect(ray, &isect))
			break;
	}
	// Initialize additional values in _vertices_
	for (int i = 0; i < nVerts - 1; ++i)
		vertices[i + 1].dAWeight = vertices[i].bsdfWeight * vertices[i].rrWeight *
			AbsDot(vertices[i + 1].wi, vertices[i + 1].ns) /
			DistanceSquared(vertices[i].p, vertices[i + 1].p);
	return nVerts;
}
SWCSpectrum BidirIntegrator::Li(const Scene *scene, const RayDifferential &ray,
	const Sample *sample, float *alpha) const
{
	SampleGuard guard(sample->sampler, sample);
	SWCSpectrum L(0.f);
	*alpha = 1.f;
	// Generate eye and light sub-paths
	vector<BidirVertex> eyePath(maxEyeDepth), lightPath(maxLightDepth), directPath(1);
	int nEye = generatePath(scene, ray, sample, sampleEyeOffset, eyePath);
	if (nEye == 0) {
		L = eyePath[0].Le;
		XYZColor color(L.ToXYZ());
		// Set alpha channel
		if (color.Black())
			*alpha = 0.f;
		if (color.y() > 0.f)
			sample->AddContribution(sample->imageX, sample->imageY,
				color, alpha ? *alpha : 1.f);
		return L;
	}
	eyePath[0].dAWeight = 0.f; //FIXME
	// Choose light for bidirectional path
	int numberOfLights = static_cast<int>(scene->lights.size());
	int lightNum = Floor2Int(sample->oneD[lightNumOffset][0] *
		numberOfLights);
	lightNum = min<int>(lightNum, numberOfLights - 1);
	Light *light = scene->lights[lightNum];
	float lightWeight = numberOfLights;
	// Sample ray from light source to start light path
	Ray lightRay;
	float lightPdf;
	float u[4];
	u[0] = sample->twoD[lightPosOffset][0];
	u[1] = sample->twoD[lightPosOffset][1];
	u[2] = sample->twoD[lightDirOffset][0];
	u[3] = sample->twoD[lightDirOffset][1];
/*	SWCSpectrum Le = light->Sample_L(scene, u[0], u[1], u[2], u[3],
		&lightRay, &lightPdf);*/
	// Get light position with associated bsdf and pdf
	BSDF *lightBsdf;
	SWCSpectrum Le = light->Sample_L(scene, u[0], u[1], &lightBsdf, &lightPdf);
	// Generate the light path if a point could be found on the light
	int nLight;
	if (lightPdf == 0.f) {
		Le = 0.f;
		nLight = 0;
	} else {
		Le *= lightWeight / lightPdf;
		nLight = generateLightPath(scene, lightBsdf, sample,
			sampleLightOffset, lightPath);
	}
	// Give the light point probability for the weighting if the light
	// is not delta
	if (nLight > 0 && !light->IsDeltaLight())
		lightPath[0].dAWeight = lightPdf / lightWeight;
	else
		lightPath[0].dAWeight = 0.f;
	// Get the pdf of the point in case of direct lighting
	float lightDirectPdf;
	if (nLight > 1)
		lightDirectPdf = light->Pdf(lightPath[1].p, lightPath[1].ns,
			lightPath[1].wi) *
			DistanceSquared(lightPath[1].p, lightPath[0].p) /
			AbsDot(lightPath[0].ns, lightPath[0].wo);
	else
		lightDirectPdf = 0.f;
	// Connect bidirectional path prefixes and evaluate throughput
	SWCSpectrum directWt(1.f);
	for (int i = 0; i < nEye; ++i) {
		// Check light intersection
		if (i == 0)
			L += eyePath[i].Le; //FIXME
		if (i > 0 && eyePath[i].ePdf > 0.f && !eyePath[i].Le.Black()) {
			// Prepare the light vertex
			directPath[0].wi = Vector(eyePath[i].ng);
			directPath[0].bsdf = eyePath[i].eBsdf;
			directPath[0].p = eyePath[i].p;
			directPath[0].ng = eyePath[i].ng;
			directPath[0].ns = eyePath[i].ns;
			directPath[0].dAWeight = eyePath[i].ePdf;
			L += eyePath[i].Le * evalPath(scene, eyePath, i + 1, directPath, 0) /
				weightPath(eyePath, i + 1, directPath, 0, eyePath[i].ePdfDirect, false);
		}
		// Do direct lighting
		float *data = sample->sampler->GetLazyValues(const_cast<Sample *>(sample), sampleDirectOffset, i);
		// Randomly choose a single light to sample
		float portal = data[2] * numberOfLights;
		int lightDirectNumber = min(Floor2Int(portal),
			numberOfLights - 1);
		portal -= lightDirectNumber;
		Light *lightDirect = scene->lights[lightDirectNumber];
		float pdfDirect;
		BSDF *bsdfDirect;
		VisibilityTester visibility;
		// Sample the chosen light
		SWCSpectrum Ld = lightDirect->Sample_L(scene, eyePath[i].p, eyePath[i].ns,
			data[0], data[1], portal, &bsdfDirect, &pdfDirect,
			&visibility);
		// Test the visibility of the light
		if (pdfDirect > 0.f && !Ld.Black() && visibility.Unoccluded(scene)) {
			// Prepare the light vertex
			directPath[0].wi = Vector(bsdfDirect->dgShading.nn);
			directPath[0].bsdf = bsdfDirect;
			directPath[0].p = bsdfDirect->dgShading.p;
			directPath[0].ng = bsdfDirect->dgShading.nn;
			directPath[0].ns = directPath[0].ng;
			directPath[0].dAWeight = lightDirect->Pdf(scene, directPath[0].p);
			// Add direct lighting contribution
			L += Ld * evalPath(scene, eyePath, i + 1, directPath, 1) /
				weightPath(eyePath, i + 1, directPath, 1, pdfDirect, true);
		}
		// Compute direct lighting pdf
		float directPdf = light->Pdf(eyePath[i].p, eyePath[i].ns,
			eyePath[i].wi) *
			DistanceSquared(eyePath[i].p, lightPath[0].p) /
			AbsDot(lightPath[0].ns, lightPath[0].wo);
		for (int j = 1; j <= nLight; ++j)
			L += Le * evalPath(scene, eyePath, i + 1, lightPath, j) /
				weightPath(eyePath, i + 1, lightPath, j, j == 1 ? directPdf : lightDirectPdf, false);
	}
	XYZColor color(L.ToXYZ());
	if (color.y() > 0.f)
		sample->AddContribution(sample->imageX, sample->imageY,
			color, alpha ? *alpha : 1.f);
	return L;
}
int BidirIntegrator::generatePath(const Scene *scene, const Ray &r,
	const Sample *sample, const int sampleOffset,
	vector<BidirVertex> &vertices) const
{
	RayDifferential ray(r.o, r.d);
	int nVerts = 0;
	while (nVerts < (int)(vertices.size())) {
		// Find next vertex in path and initialize _vertices_
		BidirVertex &v = vertices[nVerts];
		const float *data = sample->sampler->GetLazyValues(const_cast<Sample *>(sample), sampleOffset, nVerts);
		Intersection isect;
		if (!scene->Intersect(ray, &isect))
			break; //FIXME Get point on infinite light
		v.wi = -ray.d;
		v.bsdf = isect.GetBSDF(ray); // do before Ns is set!
		if (nVerts == 0)
			v.Le = isect.Le(v.wi);
		else
			v.Le = isect.Le(ray, vertices[nVerts - 1].ns, &v.eBsdf, &v.ePdf, &v.ePdfDirect);
		v.p = isect.dg.p;
		v.ng = isect.dg.nn;
		v.ns = v.bsdf->dgShading.nn;
		++nVerts;
		// Possibly terminate bidirectional path sampling
		v.f = v.bsdf->Sample_f(v.wi, &v.wo, data[1], data[2], data[3],
			 &v.bsdfWeight, BSDF_ALL, &v.flags);
		if (v.bsdfWeight == 0.f || v.f.Black())
			break;
		if (nVerts > 3) {
			const float q = min(1.,
				v.f.y() * (AbsDot(v.wo, v.ns) / v.bsdfWeight));
			if (q < data[0])
				break;
			v.rrWeight = q;
		}
		// Initialize _ray_ for next segment of path
		ray = RayDifferential(v.p, v.wo);
	}
	// Initialize additional values in _vertices_
	for (int i = 0; i < nVerts - 1; ++i) {
		vertices[i + 1].dAWeight = vertices[i].bsdfWeight * vertices[i].rrWeight *
			AbsDot(vertices[i + 1].wi, vertices[i + 1].ns) /
			DistanceSquared(vertices[i].p, vertices[i + 1].p);
		vertices[i].dARWeight = vertices[i + 1].bsdf->Pdf(vertices[i + 1].wo, vertices[i + 1].wi) * vertices[i + 1].rrWeight * /*FIXME*/
			AbsDot(vertices[i].wo, vertices[i].ns) /
			DistanceSquared(vertices[i].p, vertices[i + 1].p);
	}
	return nVerts;
}

float BidirIntegrator::weightPath(vector<BidirVertex> &eye, int nEye,
	vector<BidirVertex> &light, int nLight, float pdfLight, bool directLight) const
{
	// Current path weight is 1
	float weight = 1.f, p = 1.f, pdfDirect = 0.f;
	int eyePos, lightPos;
	// Weight alternate paths obtained by extending the light path toward
	// the eye path. If there is no light path vertex (eye path hit a light)
	// there is a special case for the first alternate path
	if (nLight == 0 && nEye > 1) {
		p *= light[0].dAWeight / eye[nEye - 1].dAWeight;
		// The path can only be obtained if the last eye bounce isn't
		// specular, the last eye point cannot be specular otherwise
		// there would be no light
		if ((eye[nEye - 2].flags & BSDF_SPECULAR) == 0)
			weight += p;
		eyePos = nEye - 2;
		lightPos = 0;
	} else {
		eyePos = nEye - 1;
		lightPos = nLight - 1;
	}
	// In the case of a light path with at most one vertex,
	// the path can be obtained with direct lighting so compute the
	// direct lighting probability and add it to the weight
	if (nLight <= 1 && pdfLight > 0.f && light[0].dAWeight > 0.f && (nLight == 1 || nEye <= 1 || (eye[nEye - 2].flags & BSDF_SPECULAR) == 0)) {
		pdfDirect = p * pdfLight / light[0].dAWeight;
		weight += pdfDirect;
	}
	// The next alternate path is obtained by connecting the last vertex of
	// the light path (or the light point) to the last vertex of the eye
	// path (or the one before it hit the light)
	// If nLight is already maxLightDepth, alternate paths cannot be
	// obtained this way
	if (eyePos > 0 && nLight < maxLightDepth) {
		Vector w = light[lightPos].p - eye[eyePos].p;
		float squaredDistance = Dot(w, w);
		w /= sqrtf(squaredDistance);
		float pdf = light[lightPos].bsdf->Pdf(light[lightPos].wi, -w);
		pdf *= AbsDot(eye[eyePos].ns, w) / squaredDistance;
		// Compute RR probability if needed
		if (nLight > 3) {
			if (pdf > 0.f)
				pdf = min<float>(1.f, light[lightPos].bsdf->f(light[lightPos].wi, -w).y() *
					AbsDot(light[lightPos].ns, -w));
			else
				pdf = 0.f;
		}
		p *= pdf / eye[eyePos].dAWeight;
		// If neither bounce is specular, this path can be found so
		// add its probability to the weight
		if (p > 0.f && eye[eyePos].dAWeight > 0.f && (eyePos == 0 || (eye[eyePos - 1].flags & BSDF_SPECULAR) == 0))
			weight += p;
	}
	// Continue the process of extending the light path and reducing the
	// eye path until the light path reaches the last eye vertex before
	// the eye
	for (int i = eyePos - 1; i > max(0, nEye + nLight - maxLightDepth - 1); --i) {
		p *= eye[i].dARWeight / eye[i].dAWeight;
		// If neither bounce is specular, this path can be found so
		// add its probability to the weight
		if ((eye[i].flags & BSDF_SPECULAR) == 0 &&
			(eye[i - 1].flags & BSDF_SPECULAR) == 0)
			weight += p;
	}
	// If the light path can land on the eye, add the corresponding
	// probability to the weight
	if (nEye > 0 && nEye + nLight - maxLightDepth - 1 <= 0 && eye[0].dAWeight > 0.f) {
		p *= eye[0].dARWeight / eye[0].dAWeight;
		weight += p;
	}
	// Now we look for alternate paths in the other direction
	// so we reinitialize the path probability to 1 (current path)
	p = 1.f;
	// Weight alternate paths obtained by extending the light path toward
	// the eye path. If there is no light path vertex (eye path hit a light)
	// there is a special case for the first alternate path
	if (nEye == 0) {
		p *= eye[0].dAWeight / light[nLight - 1].dAWeight;
		lightPos = nLight - 2;
		eyePos = 0;
	} else {
		lightPos = nLight - 1;
		eyePos = nEye - 1;
	}
	// The next alternate path is obtained by connecting the last vertex of
	// the light path (or the light point) to the last vertex of the eye
	// path (or the one before it hit the light)
	// If nLight is already maxLightDepth, alternate paths cannot be
	// obtained this way
	if (lightPos > 0 && nEye < maxEyeDepth) {
		Vector w = eye[eyePos].p - light[lightPos].p;
		float squaredDistance = Dot(w, w);
		w /= sqrtf(squaredDistance);
		float pdf = eye[eyePos].bsdf->Pdf(eye[eyePos].wi, -w);
		pdf *= AbsDot(light[lightPos].ns, w) / squaredDistance;
		// Compute RR probability if needed
		if (nEye > 3) {
			if (pdf > 0.f)
				pdf = min<float>(1.f, eye[eyePos].bsdf->f(eye[eyePos].wi, -w).y() *
					AbsDot(eye[eyePos].ns, -w));
			else
				pdf = 0.f;
		}
		p *= pdf / light[lightPos].dAWeight;
		// If neither bounce is specular, this path can be found so
		// add its probability to the weight
		if (p > 0.f && light[lightPos].dAWeight > 0.f && (lightPos == 0 || (light[lightPos - 1].flags & BSDF_SPECULAR) == 0))
			weight += p;
	}
	// Continue the process of extending the eye path and reducing the
	// light path until the eye path reaches the last light vertex before
	// the light
	for (int i = lightPos - 1; i > max(0, nLight + nEye - maxEyeDepth - 1); --i) {
		p *= light[i].dARWeight / light[i].dAWeight;
		// If neither bounce is specular, this path can be found so
		// add its probability to the weight
		if ((light[i].flags & BSDF_SPECULAR) == 0 &&
			(light[i - 1].flags & BSDF_SPECULAR) == 0)
			weight += p;
	}
	// If the light path had at least 2 vertices, this last path
	// alternative can be obtained through direct lighting too
	// so compute the probability and add it to the weight
	if (nLight > 1 && nLight + nEye - maxEyeDepth - 1 <= 0 && pdfLight > 0. && light[0].dAWeight > 0.f) {

		pdfDirect = p * pdfLight / light[0].dAWeight;
		weight += pdfDirect;
	}
	// If the eye path can land on the light, add the corresponding
	// probability to the weight
	if (nLight > 0 && nLight + nEye - maxEyeDepth - 1 <= 0 && light[0].dAWeight > 0.f) {
		p *= light[0].dARWeight / light[0].dAWeight;
		weight += p;
	}
	// The weight has been computed as if it was a normal path, however it
	// needs to be corrected if the path is a direct lighting path
	if (directLight && pdfDirect > 0.f)
		weight /= pdfDirect;
	return weight;
}
SWCSpectrum BidirIntegrator::evalPath(const Scene *scene, vector<BidirVertex> &eye,
	int nEye, vector<BidirVertex> &light, int nLight) const
{
	SWCSpectrum L(1.f);
	for (int i = 0; i < nEye - 1; ++i)
		L *= eye[i].f * AbsDot(eye[i].wo, eye[i].ns) /
			(eye[i].bsdfWeight * eye[i].rrWeight);
	if (nLight > 0 && nEye > 0) {
		Vector w = Normalize(light[nLight - 1].p - eye[nEye - 1].p);
		L *= eye[nEye - 1].bsdf->f(eye[nEye - 1].wi, w) *
			G(eye[nEye - 1], light[nLight - 1]) *
			light[nLight - 1].bsdf->f(light[nLight - 1].wi, -w);
	}
	for (int i = nLight - 2; i >= 0; --i)
		L *= light[i].f * AbsDot(light[i].wo, light[i].ns) /
			(light[i].bsdfWeight * light[i].rrWeight);
	if (L.Black())
		return 0.;
	if (nLight > 0 && nEye > 0 && !visible(scene, eye[nEye - 1].p, light[nLight - 1].p))
		return 0.;
	return L;
}
float BidirIntegrator::G(const BidirVertex &v0, const BidirVertex &v1)
{
	Vector w = Normalize(v1.p - v0.p);
	return AbsDot(v0.ns, w) * AbsDot(v1.ns, -w) /
		DistanceSquared(v0.p, v1.p);
}
bool BidirIntegrator::visible(const Scene *scene, const Point &P0,
	const Point &P1)
{
	Ray ray(P0, P1 - P0, RAY_EPSILON, 1.f - RAY_EPSILON);
	return !scene->IntersectP(ray);
}
SurfaceIntegrator* BidirIntegrator::CreateSurfaceIntegrator(const ParamSet &params)
{
	int eyeDepth = params.FindOneInt("eyedepth", 8);
	int lightDepth = params.FindOneInt("lightdepth", 8);
	return new BidirIntegrator(eyeDepth, lightDepth);
}
