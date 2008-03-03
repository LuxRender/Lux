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

#ifndef LUX_CAMERA_H
#define LUX_CAMERA_H
// camera.h*
#include "lux.h"
#include "geometry/transform.h"
#include "error.h"

namespace lux
{
	struct Sample_stub {
		// Reference to the sampler for lazy evaluation
		Sampler *sampler;
		// Camera _Sample_ Data
		float imageX, imageY;
		float lensU, lensV;
		float time;
		float wavelengths, singleWavelength;
		// Integrator _Sample_ Data
		mutable int stamp;
		vector<u_int> n1D, n2D, nxD, dxD;
		vector<vector<u_int> > sxD;
		float **oneD, **twoD, **xD;
		int **timexD;
	};

// Camera Declarations
class  Camera {
public:
	// Camera Interface
	virtual float GenerateRay(const Sample &sample,
		                      Ray *ray) const = 0;
	virtual bool IsVisibleFromEyes(const Scene *scene, const Point &p, Sample_stub * sample_gen, Ray *ray_gen)
	{
		luxError(LUX_BUG,LUX_SEVERE,"Unimplemented Camera::IsVisibleFromEyes() method called");
		return false;
	}
	virtual float GetConnectingFactor(const Point &p, const Vector &wo, const Normal &n)
	{
		luxError(LUX_BUG,LUX_SEVERE,"Unimplemented Camera::GetConnectingFactor() method called");
		return 0;
	}
	virtual void GetFlux2RadianceFactor(Film *film, int xPixelCount, int yPixelCount)
	{
		luxError(LUX_BUG,LUX_SEVERE,"Unimplemented Camera::GetFlux2RadianceFactor() method called");
	}
	virtual bool IsDelta() const
	{
		luxError(LUX_BUG,LUX_SEVERE,"Unimplemented Camera::IsDelta() method called");
		return true;
	}
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
	bool GenerateSample(const Point &p, Sample *sample) const;
	// ProjectiveCamera Protected Data
	Transform CameraToScreen, WorldToScreen, RasterToCamera;
	Transform ScreenToRaster, RasterToScreen;
	Transform WorldToRaster;
	float LensRadius, FocalDistance;
};

}//namespace lux

#endif // LUX_CAMERA_H
