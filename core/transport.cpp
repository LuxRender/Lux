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

// transport.cpp*
#include "transport.h"
#include "scene.h"
#include "bxdf.h"
#include "light.h"
#include "mc.h"
#include "volume.h"
#include "camera.h"
#include "material.h"

namespace lux
{

// Integrator Method Definitions
bool VolumeIntegrator::Intersect(const TsPack *tspack, const Scene *scene,
	const Volume *volume, const RayDifferential &ray, Intersection *isect,
	BSDF **bsdf, SWCSpectrum *L) const
{
	const bool hit = scene->Intersect(ray, isect);
	if (hit) {
		isect->dg.ComputeDifferentials(ray);
		DifferentialGeometry dgShading;
		isect->primitive->GetShadingGeometry(isect->WorldToObject.GetInverse(),
			isect->dg, &dgShading);
		isect->material->GetShadingGeometry(tspack, isect->dg.nn,
			&dgShading);
		if (Dot(ray.d, dgShading.nn) > 0.f) {
			if (!volume)
				volume = isect->interior;
			else if (!isect->interior)
				isect->interior = volume;
		} else {
			if (!volume)
				volume = isect->exterior;
			else if (!isect->exterior)
				isect->exterior = volume;
		}
		if (bsdf)
			*bsdf = isect->material->GetBSDF(tspack, isect->dg,
				dgShading, isect->exterior, isect->interior);
	}
	if (volume && L)
		*L *= Exp(-volume->Tau(tspack, ray));
	if (L)
		Transmittance(tspack, scene, ray, NULL, NULL, L);
	return hit;
}

bool VolumeIntegrator::Connect(const TsPack *tspack, const Scene *scene,
	const Volume *volume, const Point &p0, const Point &p1, bool clip,
	SWCSpectrum *f, float *pdf, float *pdfR) const
{
	const Vector w = p1 - p0;
	const float length = w.Length();
	const float shadowRayEpsilon = max(MachineEpsilon::E(p0),
		MachineEpsilon::E(length));
	if (shadowRayEpsilon >= length * .5f)
		return false;
	const float maxt = length - shadowRayEpsilon;
	RayDifferential ray(Ray(p0, w / length, shadowRayEpsilon, maxt));
	ray.time = tspack->time;
	if (clip)
		tspack->camera->ClampRay(ray);
	const Vector d(ray.d);
	Intersection isect;
	const BxDFType flags(BxDFType(BSDF_SPECULAR | BSDF_TRANSMISSION));
	// The for loop prevents an infinite sequence when the ray is almost
	// parallel to the surface and is self shadowed
	// This should be much less frequent with dynamic epsilon,
	// but it's safer to keep it
	for (u_int i = 0; i < 10000; ++i) {
		BSDF *bsdf;
		if (!scene->Intersect(tspack, volume, ray, &isect, &bsdf, f))
			return true;

		*f *= bsdf->f(tspack, d, -d, flags);
		Transmittance(tspack, scene, ray, NULL, NULL, f);
		if (f->Black())
			return false;
		const float cost = Dot(bsdf->nn, d);
		if (cost > 0.f)
			volume = isect.exterior;
		else
			volume = isect.interior;
		*f *= fabsf(cost);
		if (pdf)
			*pdf *= bsdf->Pdf(tspack, d, -d);
		if (pdfR)
			*pdfR *= bsdf->Pdf(tspack, -d, d);

		ray.mint = ray.maxt + MachineEpsilon::E(ray.maxt);
		ray.maxt = maxt;
	}
	return false;
}

// Integrator Utility Functions
SWCSpectrum UniformSampleAllLights(const TsPack *tspack, const Scene *scene,
	const Point &p, const Normal &n, const Vector &wo, BSDF *bsdf,
	const Sample *sample,
	const float *lightSample, const float *lightNum,
	const float *bsdfSample, const float *bsdfComponent)
{
	SWCSpectrum L(0.f);
	for (u_int i = 0; i < scene->lights.size(); ++i) {
		L += EstimateDirect(tspack, scene, scene->lights[i], p, n, wo, bsdf,
			sample, lightSample[0], lightSample[1], *lightNum,
			bsdfSample[0], bsdfSample[1], *bsdfComponent);
	}
	return L;
}

u_int UniformSampleOneLight(const TsPack *tspack, const Scene *scene,
	const Point &p, const Normal &n, const Vector &wo, BSDF *bsdf,
	const Sample *sample,
	const float *lightSample, const float *lightNum,
	const float *bsdfSample, const float *bsdfComponent, SWCSpectrum *L)
{
	// Randomly choose a single light to sample, _light_
	u_int nLights = scene->lights.size();
	if (nLights == 0) {
		*L = 0.f;
		return 0;
	}
	float ls3 = *lightNum * nLights;
	const u_int lightNumber = min(Floor2UInt(ls3), nLights - 1);
	ls3 -= lightNumber;
	Light *light = scene->lights[lightNumber];
	*L = static_cast<float>(nLights) * EstimateDirect(tspack, scene, light,
		p, n, wo, bsdf, sample, lightSample[0], lightSample[1], ls3,
		bsdfSample[0], bsdfSample[1], *bsdfComponent);
	return scene->lights[lightNumber]->group;
}

SWCSpectrum EstimateDirect(const TsPack *tspack, const Scene *scene, const Light *light,
	const Point &p, const Normal &n, const Vector &wo, BSDF *bsdf, const Sample *sample, 
	float ls1, float ls2, float ls3, float bs1, float bs2, float bcs)
{
	SWCSpectrum Ld(0.f);

	// Check if MIS is needed
	const BxDFType noDiffuse = BxDFType(BSDF_ALL & ~(BSDF_DIFFUSE));
	const bool mis = !(light->IsDeltaLight()) &&
		(bsdf->NumComponents(noDiffuse) > 0);
	// Trace a shadow ray by sampling the light source
	float lightPdf;
	SWCSpectrum Li;
	BSDF *lightBsdf;
	if (light->Sample_L(tspack, scene, p, ls1, ls2, ls3, &lightBsdf,
		NULL, &lightPdf, &Li)) {
		const Point &pL(lightBsdf->dgShading.p);
		const Vector wi0(pL - p);
		const Volume *volume = bsdf->GetVolume(wi0);
		if (!volume)
			volume = lightBsdf->GetVolume(-wi0);
		if (scene->Connect(tspack, volume, p, pL, false, &Li, NULL,
			NULL)) {
			const float d2 = wi0.LengthSquared();
			const Vector wi(wi0 / sqrtf(d2));
			Li *= lightBsdf->f(tspack, Vector(lightBsdf->nn), -wi);
			Li *= bsdf->f(tspack, wi, wo);
			if (!Li.Black()) {
				const float lightPdf2 = lightPdf * d2 /
					AbsDot(wi, lightBsdf->nn);
				if (mis) {
					const float bsdfPdf = bsdf->Pdf(tspack,
						wo, wi);
					Li *= PowerHeuristic(1, lightPdf2,
						1, bsdfPdf);
				}
				// Add light's contribution
				Ld += Li * (AbsDot(wi, n) / lightPdf2);
			}
		}
	}
	if (mis) {
		// Trace a second shadow ray by sampling the BSDF
		Vector wi;
		float bsdfPdf;
		BxDFType sampledType;
		if (bsdf->Sample_f(tspack, wo, &wi, bs1, bs2, bcs,
			&Li, &bsdfPdf, BSDF_ALL, &sampledType, NULL, true) &&
			(sampledType & BSDF_SPECULAR) == 0) {
			// Add light contribution from BSDF sampling
			Intersection lightIsect;
			RayDifferential ray(p, wi);
			ray.time = tspack->time;
			BSDF *ibsdf;
			const Volume *volume = bsdf->GetVolume(wi);
			bool lit = false;
			if (!scene->Intersect(tspack, volume, ray, &lightIsect,
				&ibsdf, &Li))
				lit = light->Le(tspack, scene, ray, &ibsdf,
					NULL, &lightPdf, &Li);
			else if (lightIsect.arealight == light) {
				Li *= lightIsect.Le(tspack, ray, &ibsdf,
					NULL, &lightPdf);
				lit = !Li.Black();
			}
			if (lit) {
				const float d2 = DistanceSquared(p, ibsdf->dgShading.p);
				const float lightPdf2 = lightPdf * d2 /
					AbsDot(wi, lightBsdf->nn);
				const float weight = PowerHeuristic(1, bsdfPdf,
					1, lightPdf2);
				Ld += Li * (AbsDot(wi, n) * weight / bsdfPdf);
			}
		}
	}

	return Ld;
}

}//namespace lux
