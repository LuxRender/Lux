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

#ifndef LUX_CAMERA_H
#define LUX_CAMERA_H
// camera.h*
#include "lux.h"
#include "color.h"
#include "spectrum.h"
#include "sampling.h"
#include "geometry.h"
//#include "transform.h"

namespace lux
{

// Camera Declarations
class  Camera {
public:
	// Camera Interface
	virtual float GenerateRay(const Sample &sample,
		                      Ray *ray) const = 0;
	virtual ~Camera();
	Camera(const Transform &world2cam, float hither, float yon,
		float sopen, float sclose, Film *film);
	// Camera Public Data
	Film *film;
protected:
	// Camera Protected Data
	Transform WorldToCamera, CameraToWorld;
	float ClipHither, ClipYon;
	float ShutterOpen, ShutterClose;
};
class  ProjectiveCamera : public Camera {
public:
	// ProjectiveCamera Public Methods
	ProjectiveCamera(const Transform &world2cam,
	    const Transform &proj, const float Screen[4],
		float hither, float yon,
		float sopen, float sclose,
		float lensr, float focald, Film *film);
protected:
	// ProjectiveCamera Protected Data
	Transform CameraToScreen, WorldToScreen, RasterToCamera;
	Transform ScreenToRaster, RasterToScreen;
	float LensRadius, FocalDistance;
};

}//namespace lux

#endif // LUX_CAMERA_H
