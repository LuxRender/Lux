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

// camera.cpp*
#include "lux.h"
#include "camera.h"
#include "film.h"
#include "mc.h"
#include "geometry/raydifferential.h"
#include "sampling.h"
#include "bxdf.h"

using namespace lux;

// Camera Method Definitions
Camera::~Camera() {
	delete film;
}
Camera::Camera(const Transform &w2cstart,
			   const Transform &w2cend,
               float hither, float yon,
			   float sopen, float sclose, int sdist, Film *f) 
			   : CameraMotion(sopen, sclose, w2cstart, w2cend) {
	WorldToCamera = w2cstart;
	CameraToWorld = WorldToCamera.GetInverse();
	ClipHither = hither;
	ClipYon = yon;
	ShutterOpen = sopen;
	ShutterClose = sclose;
	ShutterDistribution = sdist;
	film = f;
}

float Camera::GenerateRay(const Scene &scene, const Sample &sample,
	Ray *ray) const
{
	const SpectrumWavelengths &sw(sample.swl);
	if (IsLensBased()) {
		const float o1 = sample.lensU;
		const float o2 = sample.lensV;
		const float d1 = sample.imageX;
		const float d2 = sample.imageY;
		if (!GenerateRay(sample.arena, sw, scene, o1, o2, d1, d2, ray))
			return 0.f;
	} else {
		const float o1 = sample.imageX;
		const float o2 = sample.imageY;
		const float d1 = sample.lensU;
		const float d2 = sample.lensV;
		if (!GenerateRay(sample.arena, sw, scene, o1, o2, d1, d2, ray))
			return 0.f;
	}

	// Set ray time value
	ray->time = GetTime(sample.time);

	return 1.f;
}

bool Camera::GenerateRay(MemoryArena &arena, const SpectrumWavelengths &sw,
	const Scene &scene, float o1, float o2, float d1, float d2, Ray *ray) const
{
	SWCSpectrum We;
	BSDF *bsdf;
	float pdf;
	// Sample ray origin
	//FIXME: Replace dummy .5f by a sampled value if needed
	if (!Sample_W(arena, sw, scene, o1, o2, .5f, &bsdf, &pdf, &We))
		return false;
	ray->o = bsdf->dgShading.p;

	// Sample ray direction
	//FIXME: Replace dummy .5f by a sampled value if needed
	if (!bsdf->Sample_f(sw, Vector(bsdf->nn), &(ray->d), d1, d2, .5f,
		&We, &pdf, BSDF_ALL, NULL, NULL, true))
		return false;

	// Do depth clamping
	ClampRay(*ray);

	return true;
}

void Camera::SampleMotion(float time) {
	if (!CameraMotion.isActive)
		return;

	WorldToCamera = CameraMotion.Sample(time);
	CameraToWorld = WorldToCamera.GetInverse();
}

float Camera::GetTime(float u1) const {
	if(ShutterDistribution == 0)
		return Lerp(u1, ShutterOpen, ShutterClose);
	else { 
		// gaussian distribution
		// default uses 2 standard deviations.
		const float sigma = 2.f;
		float x = NormalCDFInverse(u1);
		// clamping leads to lumping at endpoints
		// so redistribute points around the mean instead
		if (fabsf(x) > sigma)
			x = NormalCDFInverse(0.5f + u1 - Round2Int(u1));
		
		x = Clamp(x / (2.f*sigma) + 0.5f, 0.f, 1.f);
		return Lerp(x, ShutterOpen, ShutterClose);
	}

	return 0.f;
}

ProjectiveCamera::ProjectiveCamera(const Transform &w2cs,
	    const Transform &w2ce,
		const Transform &proj, const float Screen[4],
		float hither, float yon, float sopen,
		float sclose, int sdist, float lensr, float focald, Film *f)
	: Camera(w2cs, w2ce, hither, yon, sopen, sclose, sdist, f) {
	// Initialize depth of field parameters
	LensRadius = lensr;
	FocalDistance = focald;
	// Compute projective camera transformations
	CameraToScreen = proj;
	WorldToScreen = CameraToScreen * WorldToCamera;
	// Compute projective camera screen transformations
	ScreenToRaster = Scale(float(film->xResolution),
	                       float(film->yResolution), 1.f) *
		  Scale(1.f / (Screen[1] - Screen[0]),
				1.f / (Screen[2] - Screen[3]), 1.f) *
		 Translate(Vector(-Screen[0], -Screen[3], 0.f));
	RasterToScreen = ScreenToRaster.GetInverse();
	CameraToRaster = ScreenToRaster * CameraToScreen;
	RasterToCamera = CameraToRaster.GetInverse();
	WorldToRaster = ScreenToRaster * WorldToScreen;
	RasterToWorld = WorldToRaster.GetInverse();
}
void ProjectiveCamera::SampleMotion(float time) {

	if (!CameraMotion.isActive)
		return;

	// call base method to sample transform
	Camera::SampleMotion(time);
	// then update derivative transforms
	WorldToScreen = CameraToScreen * WorldToCamera;
	WorldToRaster = CameraToRaster * WorldToCamera;
	RasterToWorld = WorldToRaster.GetInverse();
}
