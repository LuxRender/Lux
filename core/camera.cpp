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
#include "sampling.h"
#include "error.h"
#include "mc.h"

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
	warnOnce = false;
}
bool Camera::IsDelta() const {
	LOG(LUX_SEVERE,LUX_BUG)<<"Unimplemented Camera::IsDelta() method called";
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
	RasterToCamera = CameraToScreen.GetInverse() * RasterToScreen;
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
	WorldToRaster = ScreenToRaster * WorldToScreen;
	RasterToWorld = WorldToRaster.GetInverse();
}
bool ProjectiveCamera::GenerateSample(const Point &p, Sample *sample) const
{
	Point p_raster = WorldToRaster(p);
	sample->imageX = p_raster.x;
	sample->imageY = p_raster.y;

	//if (sample->imageX>=film->xPixelStart && sample->imageX<film->xPixelStart+film->xPixelCount &&
	//	sample->imageY>=film->yPixelStart && sample->imageY<film->yPixelStart+film->yPixelCount )
	if (sample->imageX>=0 && sample->imageX<film->xResolution &&
		sample->imageY>=0 && sample->imageY<film->yResolution )
		return true;
	else
		return false;

	return true;
}
