/***************************************************************************
 *   Copyright (C) 1998-2013 by authors (see AUTHORS.txt)                  *
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
#include "geometry/raydifferential.h"
#include "sampling.h"
#include "bxdf.h"

#include "luxrays/utils/mc.h"

using namespace luxrays;
using namespace lux;

// Camera Method Definitions
Camera::Camera(const MotionSystem &w2c, float hither, float yon,
	float sopen, float sclose, int sdist, Film *f) : CameraMotion(w2c)
{
	CameraToWorld = Inverse(CameraMotion.Sample(sopen));
	ClipHither = hither;
	ClipYon = yon;
	ShutterOpen = sopen;
	ShutterClose = sclose;
	ShutterDistribution = sdist;
	film = f;
}

void Camera::AddAttributes(Queryable *q) const
{
/*	AddFloatAttribute(*q, "ShutterOpen", "Time when shutter opens", 0.f, &Camera::ShutterOpen);
	AddFloatAttribute(*q, "ShutterClose", "Time when shutter closes", 1.f, &Camera::ShutterClose);*/
	AddFloatConstant(*q, "ShutterOpen", "Time when shutter opens", ShutterOpen);
	AddFloatConstant(*q, "ShutterClose", "Time when shutter closes", ShutterClose);
	AddFloatConstant(*q, "ClipHither", "Near clip plane", ClipHither);
	AddFloatConstant(*q, "ClipYon", "Far clip plane", ClipYon);
}

float Camera::GenerateRay(const Scene &scene, const Sample &sample,
	Ray *ray, float *x, float *y) const
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
	ray->time = sample.realTime;

	// Do depth clamping
	ClampRay(*ray);

	return GetSamplePosition(ray->o, ray->d, INFINITY, x, y) ? 1.f : 0.f;
}

bool Camera::GenerateRay(MemoryArena &arena, const SpectrumWavelengths &sw,
	const Scene &scene, float o1, float o2, float d1, float d2, Ray *ray) const
{
	SWCSpectrum We;
	BSDF *bsdf;
	float pdf;
	// Sample ray origin
	//FIXME: Replace dummy .5f by a sampled value if needed
	if (!SampleW(arena, sw, scene, o1, o2, .5f, &bsdf, &pdf, &We))
		return false;

	// Sample ray direction
	//FIXME: Replace dummy .5f by a sampled value if needed
	Vector w;
	if (!bsdf->SampleF(sw, Vector(bsdf->dgShading.nn), &w, d1, d2, .5f,
		&We, &pdf, BSDF_ALL, NULL, NULL, true))
		return false;

	//Initialize ray
	*ray = Ray(bsdf->dgShading.p, w);

	return true;
}

void Camera::SampleMotion(float time) {
	if (CameraMotion.IsStatic())
		return;

	CameraToWorld = Inverse(CameraMotion.Sample(time));
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

ProjectiveCamera::ProjectiveCamera(const MotionSystem &w2c,
		const Transform &proj, const float screen[4],
		float hither, float yon, float sopen,
		float sclose, int sdist, float lensr, float focald, Film *f)
	: Camera(w2c, hither, yon, sopen, sclose, sdist, f) {
	ScreenWindow[0] = screen[0];
	ScreenWindow[1] = screen[1];
	ScreenWindow[2] = screen[2];
	ScreenWindow[3] = screen[3];

	// Initialize depth of field parameters
	LensRadius = lensr;
	FocalDistance = focald;
	// Compute projective camera transformations
	ScreenToCamera = Inverse(proj);
	ScreenToWorld = CameraToWorld * ScreenToCamera;
	// Compute projective camera screen transformations
	RasterToScreen = Translate(Vector(ScreenWindow[0], ScreenWindow[3], 0.f)) *
		Scale(ScreenWindow[1] - ScreenWindow[0], ScreenWindow[2] - ScreenWindow[3], 1.f) *
		Scale(1.f / film->xResolution, 1.f / film->yResolution, 1.f);
	RasterToCamera = ScreenToCamera * RasterToScreen;
	RasterToWorld = ScreenToWorld * RasterToScreen;
}

void ProjectiveCamera::AddAttributes(Queryable *q) const
{
	Camera::AddAttributes(q);
	AddFloatConstant(*q, "LensRadius", "Lens radius", LensRadius);
	AddFloatConstant(*q, "FocalDistance", "Focal distance", FocalDistance);
	AddFloatConstant(*q, "ScreenWindow.0", "Screen window 0", ScreenWindow[0]);
	AddFloatConstant(*q, "ScreenWindow.1", "Screen window 1", ScreenWindow[1]);
	AddFloatConstant(*q, "ScreenWindow.2", "Screen window 2", ScreenWindow[2]);
	AddFloatConstant(*q, "ScreenWindow.3", "Screen window 3", ScreenWindow[3]);
}

void ProjectiveCamera::SampleMotion(float time) {

	if (CameraMotion.IsStatic())
		return;

	// call base method to sample transform
	Camera::SampleMotion(time);
	// then update derivative transforms
	ScreenToWorld = CameraToWorld * ScreenToCamera;
	RasterToCamera = ScreenToCamera * RasterToScreen;
	RasterToWorld = ScreenToWorld * RasterToScreen;
}

SceneCamera::SceneCamera(Camera *cam) : Queryable("camera"), camera(cam)
{
	camera->AddAttributes(this);
}

SceneCamera::~SceneCamera()
{
	delete camera->film;
	delete camera;
}
