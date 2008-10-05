/***************************************************************************
 *   Copyright (C) 1998-2008 by authors (see AUTHORS.txt )                 *
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

using namespace lux;

// Camera Method Definitions
Camera::~Camera() {
	delete film;
}
Camera::Camera(const Transform &world2cam,
               float hither, float yon,
		       float sopen, float sclose, Film *f) {
	WorldToCamera = world2cam;
	CameraToWorld = WorldToCamera.GetInverse();
	ClipHither = hither;
	ClipYon = yon;
	ShutterOpen = sopen;
	ShutterClose = sclose;
	film = f;
	if (WorldToCamera.HasScale())
		luxError(LUX_UNIMPLEMENT,LUX_WARNING,"Scaling detected in world-to-camera transformation!\n The system has numerous assumptions, implicit and explicit,\nthat this transform will have no scale factors in it.\nProceed at your own risk; your image may have errors or\nthe system may crash as a result of this.");
}
bool Camera::IsDelta() const
{
	luxError(LUX_BUG,LUX_SEVERE,"Unimplemented Camera::IsDelta() method called");
	return true;
}

ProjectiveCamera::ProjectiveCamera(const Transform &w2c,
		const Transform &proj, const float Screen[4],
		float hither, float yon, float sopen,
		float sclose, float lensr, float focald, Film *f)
	: Camera(w2c, hither, yon, sopen, sclose, f) {
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
