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
#include "motionsystem.h"
#include "error.h"

namespace lux
{
// Camera Declarations
class  Camera {
public:
	// Camera Interface
	Camera(const Transform &w2cstart, const Transform &w2cend, float hither, float yon,
		float sopen, float sclose, int sdist, Film *film);
	virtual ~Camera();
	virtual float GenerateRay(const Sample &sample, Ray *ray) const = 0;
	virtual bool Sample_W(const TsPack *tspack, const Scene *scene, float u1, float u2, float u3, BSDF **bsdf, float *pdf, SWCSpectrum *We) const {
		if (!warnOnce)
			luxError(LUX_BUG, LUX_SEVERE, "Unimplemented Camera::Sample_W");
		warnOnce = true;
		return false;
	}
	virtual bool Sample_W(const TsPack *tspack, const Scene *scene, const Point &p, const Normal &n, float u1, float u2, float u3, BSDF **bsdf, float *pdf, float *pdfDirect, VisibilityTester *visibility, SWCSpectrum *We) const {
		if (!warnOnce)
			luxError(LUX_BUG, LUX_SEVERE, "Unimplemented Camera::Sample_W");
		warnOnce = true;
		return false;
	}
	virtual bool GetSamplePosition(const Point &p, const Vector &wi, float *x, float *y) const { return false; }
	virtual bool IsDelta() const;
	virtual bool IsLensBased() const { return true; }
	virtual void AutoFocus(Scene* scene) { }
	virtual BBox Bounds() const { return BBox(); }

	float GetTime(float u1) const;

	virtual void SampleMotion(float time);

	virtual Camera* Clone() const = 0;

	// Camera Public Data
	Film *film;
protected:
	// Camera Protected Data
	Transform WorldToCamera, CameraToWorld;
	MotionSystem CameraMotion;
	float ClipHither, ClipYon;
	float ShutterOpen, ShutterClose;
	int ShutterDistribution;
	mutable bool warnOnce;
};
class  ProjectiveCamera : public Camera {
public:
	// ProjectiveCamera Public Methods
	ProjectiveCamera(const Transform &world2cam,
		const Transform &world2camEnd,
	    const Transform &proj, const float Screen[4],
		float hither, float yon,
		float sopen, float sclose, int sdist,
		float lensr, float focald, Film *film);

	void SampleMotion(float time);

protected:
	bool GenerateSample(const Point &p, Sample *sample) const;
	// ProjectiveCamera Protected Data
	Transform CameraToScreen, WorldToScreen, RasterToCamera;
	Transform ScreenToRaster, RasterToScreen;
	Transform WorldToRaster, RasterToWorld;
	float LensRadius, FocalDistance;
};

}//namespace lux

#endif // LUX_CAMERA_H
