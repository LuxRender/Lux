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
SWCSpectrum BidirIntegrator::Li(const Scene *scene, const RayDifferential &ray,
	const Sample *sample, float *alpha) const
{
	SampleGuard guard(sample->sampler, sample);
	SWCSpectrum L(0.f);
	*alpha = 1.f;
	// Generate eye and light sub-paths
	BidirVertex eyePath[maxEyeDepth], lightPath[maxLightDepth];
	int nEye = generatePath(scene, ray, sample, sampleEyeOffset, eyePath,
		maxEyeDepth);
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
	// Choose light for bidirectional path
	int lightNum = Floor2Int(sample->oneD[lightNumOffset][0] *
		scene->lights.size());
	lightNum = min<int>(lightNum, scene->lights.size() - 1);
	Light *light = scene->lights[lightNum];
	float lightWeight = scene->lights.size();
	// Sample ray from light source to start light path
	Ray lightRay;
	float lightPdf;
	float u[4];
	u[0] = sample->twoD[lightPosOffset][0];
	u[1] = sample->twoD[lightPosOffset][1];
	u[2] = sample->twoD[lightDirOffset][0];
	u[3] = sample->twoD[lightDirOffset][1];
	SWCSpectrum Le = light->Sample_L(scene, u[0], u[1], u[2], u[3],
		&lightRay, &lightPdf);
	int nLight;
	if (lightPdf == 0.f) {
		Le = 0.f;
		nLight = 0;
	} else {
		Le *= lightWeight / lightPdf;
		nLight = generatePath(scene, lightRay, sample,
			sampleLightOffset, lightPath, maxLightDepth);
	}
	// Connect bidirectional path prefixes and evaluate throughput
	SWCSpectrum directWt(1.f);
	bool specular = true;
	int consecDiffuse = 0;
	for (int i = 0; i < nEye; ++i) {
		// Handle direct lighting for bidirectional integrator
		if ((eyePath[i].flags & BSDF_SPECULAR) == 0)
			++consecDiffuse;
		else if (consecDiffuse < 2)
			consecDiffuse = 0;
		if (consecDiffuse < 2) {
			float *data = sample->sampler->GetLazyValues(const_cast<Sample *>(sample), sampleDirectOffset, i);
			L += directWt *
				UniformSampleOneLight(scene, eyePath[i].p,
					eyePath[i].ns, eyePath[i].wi, eyePath[i].bsdf,
					sample, data, data + 2, data + 3, data + 5);
			if (specular)
				L += directWt * eyePath[i].Le;
		}
		specular = (eyePath[i].flags & BSDF_SPECULAR) != 0;
		directWt *= eyePath[i].f *
			(AbsDot(eyePath[i].wo, eyePath[i].ns) /
			(eyePath[i].bsdfWeight * eyePath[i].rrWeight));
		for (int j = 1; j <= nLight; ++j)
			L += Le * evalPath(scene, eyePath, i + 1, lightPath, j) /
				weightPath(eyePath, i + 1, lightPath, j);
	}
	XYZColor color(L.ToXYZ());
	if (color.y() > 0.f)
		sample->AddContribution(sample->imageX, sample->imageY,
			color, alpha ? *alpha : 1.f);
	return L;
}
int BidirIntegrator::generatePath(const Scene *scene, const Ray &r,
	const Sample *sample, const int sampleOffset,
	BidirVertex *vertices, int maxVerts) const
{
	int nVerts = 0;
	RayDifferential ray(r.o, r.d);
	while (nVerts < maxVerts) {
		// Find next vertex in path and initialize _vertices_
		Intersection isect;
		if (!scene->Intersect(ray, &isect)) {
			SWCSpectrum Le(0.f);
			for (u_int i = 0; i < scene->lights.size(); ++i)
				Le += scene->lights[i]->Le(ray);
			if (nVerts > 0 && !Le.Black())
				Le *= vertices[nVerts - 1].f *
					(AbsDot(vertices[nVerts - 1].wo, vertices[nVerts - 1].ns) /
					(vertices[nVerts - 1].bsdfWeight * vertices[nVerts - 1].rrWeight));
			vertices[max(0, nVerts - 1)].Le += Le;
			break;
		}
		BidirVertex &v = vertices[nVerts];
		MemoryArena arena;	// DUMMY ARENA TODO FIX THESE
		v.bsdf = isect.GetBSDF(ray); // do before Ns is set!
		v.p = isect.dg.p;
		v.ng = isect.dg.nn;
		v.ns = v.bsdf->dgShading.nn;
		v.wi = -ray.d;
		v.Le = isect.Le(v.wi);
		const float *data = sample->sampler->GetLazyValues(const_cast<Sample *>(sample), sampleOffset, nVerts);
		++nVerts;
		// Possibly terminate bidirectional path sampling
		v.f = v.bsdf->Sample_f(v.wi, &v.wo, data[1], data[2], data[3],
			 &v.bsdfWeight, BSDF_ALL, &v.flags);
		if (v.bsdfWeight == 0.f || v.f.Black())
			break;
		if (nVerts > 2) {
			const float q = min(1.,
				v.f.y() * (AbsDot(v.wo, v.ns) / v.bsdfWeight));
			if (data[0] > q)
				break;
			v.rrWeight = 1.f / q;
		}
		// Initialize _ray_ for next segment of path
		ray = RayDifferential(v.p, v.wo);
	}
	// Initialize additional values in _vertices_
	for (int i = 0; i < nVerts - 1; ++i)
		vertices[i].dAWeight = vertices[i].bsdfWeight * vertices[i].rrWeight *
			AbsDot(-vertices[i].wo, vertices[i + 1].ng) /
			DistanceSquared(vertices[i].p, vertices[i + 1].p);
	return nVerts;
}

float BidirIntegrator::weightPath(BidirVertex *eye, int nEye,
	BidirVertex *light, int nLight) const
{
	float weight = 1.f, p = 1.f;
	if (nEye > 1 && nLight > 0) {
		Vector w = light[nLight - 1].p - eye[nEye - 1].p;
		float squaredDistance = Dot(w, w);
		w /= sqrtf(squaredDistance);
		float pdf = light[nLight - 1].bsdf->Pdf(light[nLight - 1].wi, -w);
		p *= pdf * AbsDot(eye[nEye - 1].ns, w) / squaredDistance;
		if (nLight > 3) {
			float f = light[nLight - 1].bsdf->f(light[nLight - 1].wi, -w).y();
			if (f > 0.f)
				p *= max(1.f, pdf / (f * AbsDot(light[nLight - 1].ns, -w)));
			else
				p = 0.f;
		}
		p /= eye[nEye - 2].dAWeight;
		if ((eye[nEye - 2].flags & BSDF_SPECULAR) == 0)
			weight += p;
	}
	for (int i = nEye - 2; i > 0; --i) {
		p *= eye[i].dAWeight / eye[i - 1].dAWeight;
		if ((eye[i].flags & BSDF_SPECULAR) == 0 &&
			(eye[i - 1].flags & BSDF_SPECULAR) == 0)
			weight += p;
	}
	p = 1.f;
	if (nEye > 0 && nLight > 1) {
		Vector w = eye[nEye - 1].p - light[nLight - 1].p;
		float squaredDistance = Dot(w, w);
		w /= sqrtf(squaredDistance);
		float pdf = eye[nEye - 1].bsdf->Pdf(eye[nEye - 1].wi, -w);
		p *= pdf * AbsDot(light[nLight - 1].ns, w) / squaredDistance;
		if (nEye > 3) {
			float f = eye[nEye - 1].bsdf->f(eye[nEye - 1].wi, -w).y();
			if (f > 0.f)
				p *= max(1.f, pdf / (f * AbsDot(eye[nEye - 1].ns, -w)));
			else
				p = 0.f;
		}
		p /= light[nLight - 2].dAWeight;
		if ((light[nLight - 2].flags & BSDF_SPECULAR) == 0)
			weight += p;
	}
	for (int i = nLight - 2; i > 0; --i) {
		p *= light[i].dAWeight / light[i - 1].dAWeight;
		if ((light[i].flags & BSDF_SPECULAR) == 0 &&
			(light[i - 1].flags & BSDF_SPECULAR) == 0)
			weight += p;
	}
	return weight;
}
SWCSpectrum BidirIntegrator::evalPath(const Scene *scene, BidirVertex *eye,
	int nEye, BidirVertex *light, int nLight) const
{
	SWCSpectrum L(1.f);
	for (int i = 0; i < nEye - 1; ++i)
		L *= eye[i].f * AbsDot(eye[i].wo, eye[i].ns) /
			(eye[i].bsdfWeight * eye[i].rrWeight);
	Vector w = light[nLight - 1].p - eye[nEye - 1].p;
	L *= eye[nEye - 1].bsdf->f(eye[nEye - 1].wi, w) *
		G(eye[nEye - 1], light[nLight - 1]) *
		light[nLight - 1].bsdf->f(-w, light[nLight - 1].wi) /
		(eye[nEye - 1].rrWeight * light[nLight - 1].rrWeight);
	for (int i = nLight - 2; i >= 0; --i)
		L *= light[i].f * AbsDot(light[i].wo, light[i].ns) /
			(light[i].bsdfWeight * light[i].rrWeight);
	if (L.Black())
		return L;
	if (!visible(scene, eye[nEye - 1].p, light[nLight - 1].p))
		return 0.;
	return L;
}
float BidirIntegrator::G(const BidirVertex &v0, const BidirVertex &v1)
{
	Vector w = Normalize(v1.p - v0.p);
	return AbsDot(v0.ng, w) * AbsDot(v1.ng, -w) /
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
