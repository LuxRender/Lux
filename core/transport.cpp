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
#include "sampling.h"
#include "material.h"

namespace lux
{

// Integrator Method Definitions
// This is a very basic implementation without any volumetric support
// Look at the emission, single or multi integrators for proper support
bool VolumeIntegrator::Intersect(const Scene &scene, const Sample &sample,
	const Volume *volume, bool scatteredStart, const Ray &ray, float u,
	Intersection *isect, BSDF **bsdf, float *pdf, float *pdfBack,
	SWCSpectrum *L) const
{
	const bool hit = scene.Intersect(ray, isect);
	if (hit) {
		// Proper volume setting is still required for eg glass2
		if (Dot(ray.d, isect->dg.nn) > 0.f) {
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
			*bsdf = isect->GetBSDF(sample.arena, sample.swl, ray);
	}
	if (pdf)
		*pdf = 1.f;
	if (pdfBack)
		*pdfBack = 1.f;
	return hit;
}

// This is a very basic implementation without any volumetric support
// Look at the emission, single or multi integrators for proper support
bool VolumeIntegrator::Intersect(const Scene &scene, const Sample &sample,
	const Volume *volume, bool scatteredStart, const Ray &ray,
	const luxrays::RayHit &rayHit, float u, Intersection *isect,
	BSDF **bsdf, float *pdf, float *pdfBack, SWCSpectrum *L) const
{
	const bool hit = scene.Intersect(rayHit, isect);
	if (hit) {
		// Proper volume setting is still required for eg glass2
		if (Dot(ray.d, isect->dg.nn) > 0.f) {
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
			*bsdf = isect->GetBSDF(sample.arena, sample.swl, ray);
	}
	if (pdf)
		*pdf = 1.f;
	if (pdfBack)
		*pdfBack = 1.f;
	return hit;
}

bool VolumeIntegrator::Connect(const Scene &scene, const Sample &sample,
	const Volume *volume, bool scatteredStart, bool scatteredEnd,
	const Point &p0, const Point &p1, bool clip,
	SWCSpectrum *f, float *pdf, float *pdfR) const
{
	const Vector w = p1 - p0;
	const float length = w.Length();
	const float shadowRayEpsilon = max(MachineEpsilon::E(p0),
		MachineEpsilon::E(length));
	if (shadowRayEpsilon >= length * .5f)
		return false;
	const float maxt = length - shadowRayEpsilon;
	Ray ray(p0, w / length, shadowRayEpsilon, maxt);
	ray.time = sample.realTime;
	if (clip)
		sample.camera->ClampRay(ray);
	const Vector d(ray.d);
	Intersection isect;
	const BxDFType flags(BxDFType(BSDF_SPECULAR | BSDF_TRANSMISSION));
	// The for loop prevents an infinite sequence when the ray is almost
	// parallel to the surface and is self shadowed
	// This should be much less frequent with dynamic epsilon,
	// but it's safer to keep it
	for (u_int i = 0; i < 10000; ++i) {
		BSDF *bsdf;
		float spdf, spdfBack;
		isect.dg.scattered = scatteredEnd;
		if (!Intersect(scene, sample, volume, scatteredStart, ray, 1.f,
			&isect, &bsdf, &spdf, &spdfBack, f)) {
			if (pdf)
				*pdf *= spdfBack;
			if (pdfR)
				*pdfR *= spdf;
			return true;
		}

		*f *= bsdf->F(sample.swl, d, -d, true, flags);
		if (f->Black())
			return false;
		volume = bsdf->GetVolume(d);
		if (pdf)
			*pdf *= bsdf->Pdf(sample.swl, d, -d) * spdfBack;
		if (pdfR)
			*pdfR *= bsdf->Pdf(sample.swl, -d, d) * spdf;

		ray.mint = ray.maxt + MachineEpsilon::E(ray.maxt);
		ray.maxt = maxt;
	}
	return false;
}

int VolumeIntegrator::Connect(const Scene &scene, const Sample &sample,
	const Volume *volume, bool scatteredStart, bool scatteredEnd,
	const Ray &ray, const luxrays::RayHit &rayHit,
	SWCSpectrum *f, float *pdf, float *pdfR) const
{
	BSDF *bsdf;
	Intersection isect;
	float spdf, spdfBack;
	isect.dg.scattered = scatteredEnd;
	if (!Intersect(scene, sample, volume, scatteredStart, ray, rayHit, 1.f,
		&isect, &bsdf, &spdf, &spdfBack, f)) {
		if (pdf)
			*pdf *= spdfBack;
		if (pdfR)
			*pdfR *= spdf;
		return 1;
	}

	const Vector d(ray.d);
	const BxDFType flags(BxDFType(BSDF_SPECULAR | BSDF_TRANSMISSION));
	*f *= bsdf->F(sample.swl, d, -d, true, flags);
	if (f->Black())
		return -1;
	volume = bsdf->GetVolume(d);
	if (pdf)
		*pdf *= bsdf->Pdf(sample.swl, d, -d) * spdfBack;
	if (pdfR)
		*pdfR *= bsdf->Pdf(sample.swl, -d, d) * spdf;

	ray.mint = rayHit.t + MachineEpsilon::E(rayHit.t);
	return 0;
}

// Integrator Utility Functions
SWCSpectrum UniformSampleAllLights(const Scene &scene, const Sample &sample,
	const Point &p, const Normal &n, const Vector &wo, BSDF *bsdf,
	const float *lightSample, const float *lightNum,
	const float *bsdfSample, const float *bsdfComponent)
{
	SWCSpectrum L(0.f);
	for (u_int i = 0; i < scene.lights.size(); ++i) {
		L += EstimateDirect(scene, *(scene.lights[i]), sample, p, n, wo,
			bsdf, lightSample[0], lightSample[1], *lightNum,
			bsdfSample[0], bsdfSample[1], *bsdfComponent);
	}
	return L;
}

u_int UniformSampleOneLight(const Scene &scene, const Sample &sample,
	const Point &p, const Normal &n, const Vector &wo, BSDF *bsdf,
	const float *lightSample, const float *lightNum,
	const float *bsdfSample, const float *bsdfComponent, SWCSpectrum *L)
{
	// Randomly choose a single light to sample, _light_
	u_int nLights = scene.lights.size();
	if (nLights == 0) {
		*L = 0.f;
		return 0;
	}
	float ls3 = *lightNum * nLights;
	const u_int lightNumber = min(Floor2UInt(ls3), nLights - 1);
	ls3 -= lightNumber;
	const Light &light(*(scene.lights[lightNumber]));
	*L = static_cast<float>(nLights) * EstimateDirect(scene, light, sample,
		p, n, wo, bsdf, lightSample[0], lightSample[1], ls3,
		bsdfSample[0], bsdfSample[1], *bsdfComponent);
	return light.group;
}

SWCSpectrum EstimateDirect(const Scene &scene, const Light &light,
	const Sample &sample, const Point &p, const Normal &n, const Vector &wo,
	BSDF *bsdf, float ls1, float ls2, float ls3,
	float bs1, float bs2, float bcs)
{
	SWCSpectrum Ld(0.f);

	// Check if MIS is needed
	const BxDFType noDiffuse = BxDFType(BSDF_ALL & ~(BSDF_DIFFUSE));
	const bool mis = !(light.IsDeltaLight()) &&
		(bsdf->NumComponents(noDiffuse) > 0);
	// Trace a shadow ray by sampling the light source
	float lightPdf;
	SWCSpectrum Li;
	BSDF *lightBsdf;
	if (light.SampleL(scene, sample, p, ls1, ls2, ls3,
		&lightBsdf, NULL, &lightPdf, &Li)) {
		const Point &pL(lightBsdf->dgShading.p);
		const Vector wi0(pL - p);
		const Volume *volume = bsdf->GetVolume(wi0);
		if (!volume)
			volume = lightBsdf->GetVolume(-wi0);
		if (scene.Connect(sample, volume, bsdf->dgShading.scattered,
			false, p, pL, false, &Li, NULL, NULL)) {
			const float d2 = wi0.LengthSquared();
			const Vector wi(wi0 / sqrtf(d2));
			Li *= lightBsdf->F(sample.swl, Vector(lightBsdf->nn), -wi, false);
			Li *= bsdf->F(sample.swl, wi, wo, true);
			if (!Li.Black()) {
				if (mis) {
					const float bsdfPdf = bsdf->Pdf(sample.swl,
						wo, wi);
					Li *= PowerHeuristic(1, lightPdf * d2 /
						AbsDot(wi, lightBsdf->nn),
						1, bsdfPdf);
				}
				// Add light's contribution
				Ld += Li / d2;
			}
		}
	}
	if (mis) {
		// Trace a second shadow ray by sampling the BSDF
		Vector wi;
		float bsdfPdf;
		BxDFType sampledType;
		if (bsdf->SampleF(sample.swl, wo, &wi, bs1, bs2, bcs,
			&Li, &bsdfPdf, BSDF_ALL, &sampledType, NULL, true) &&
			(sampledType & BSDF_SPECULAR) == 0) {
			// Add light contribution from BSDF sampling
			Intersection lightIsect;
			Ray ray(p, wi);
			ray.time = sample.time;
			BSDF *ibsdf;
			const Volume *volume = bsdf->GetVolume(wi);
			bool lit = false;
			if (!scene.Intersect(sample, volume,
				bsdf->dgShading.scattered, ray, 1.f,
				&lightIsect, &ibsdf, NULL, NULL, &Li))
				lit = light.Le(scene, sample, ray, &lightBsdf,
					NULL, &lightPdf, &Li);
			else if (lightIsect.arealight == &light) {
				Li *= lightIsect.Le(sample, ray, &lightBsdf,
					NULL, &lightPdf);
				lit = !Li.Black();
			}
			if (lit) {
				const float d2 = DistanceSquared(p, lightBsdf->dgShading.p);
				const float lightPdf2 = lightPdf * d2 /
					AbsDot(wi, lightBsdf->nn);
				const float weight = PowerHeuristic(1, bsdfPdf,
					1, lightPdf2);
				Ld += Li * weight;
			}
		}
	}

	return Ld;
}

}//namespace lux
