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

#ifndef LUX_CAMERA_H
#define LUX_CAMERA_H
// camera.h*
#include "lux.h"
#include "geometry/transform.h"
#include "motionsystem.h"
#include "queryable.h"

namespace lux
{
// Camera Declarations
class  Camera : public Queryable {
public:
	// Camera Interface
	Camera(const Transform &w2cstart, const Transform &w2cend, float hither,
		float yon, float sopen, float sclose, int sdist, Film *film);
	virtual ~Camera();
	const Volume *GetVolume() const { return volume.get(); }
	void SetVolume(boost::shared_ptr<Volume> &v) {
		// Create a temporary to increase shared count
		// The assignment is just a swap
		boost::shared_ptr<Volume> vol(v);
		volume = vol;
	}
	float GenerateRay(const Scene &scene, const Sample &sample,
		Ray *ray) const;
	virtual bool SampleW(MemoryArena &arena, const SpectrumWavelengths &sw,
		const Scene &scene, float u1, float u2, float u3, BSDF **bsdf,
		float *pdf, SWCSpectrum *We) const = 0;
	virtual bool SampleW(MemoryArena &arena, const SpectrumWavelengths &sw,
		const Scene &scene, const Point &p, const Normal &n,
		float u1, float u2, float u3, BSDF **bsdf, float *pdf,
		float *pdfDirect, SWCSpectrum *We) const = 0;
	virtual bool GetSamplePosition(const Point &p, const Vector &wi,
		float distance, float *x, float *y) const = 0;
	virtual void ClampRay(Ray &ray) const { }
	virtual bool IsDelta() const = 0;
	virtual bool IsLensBased() const = 0;
	virtual void AutoFocus(const Scene &scene) { }
	virtual BBox Bounds() const = 0;

	float GetTime(float u1) const;

	virtual void SampleMotion(float time);

	virtual Camera* Clone() const = 0;

	// Camera Public Data
	Film *film;
	Transform WorldToCamera, CameraToWorld;
protected:
	bool GenerateRay(MemoryArena &arena, const SpectrumWavelengths &sw,
		const Scene &scene, float o1, float o2, float d1, float d2,
		Ray *ray) const;
	// Camera Protected Data
	MotionSystem CameraMotion;
	float ClipHither, ClipYon;
	float ShutterOpen, ShutterClose;
	int ShutterDistribution;
	boost::shared_ptr<Volume> volume;
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
	virtual ~ProjectiveCamera() { }

	virtual void SampleMotion(float time);

protected:
	// ProjectiveCamera Protected Data
	Transform CameraToScreen, WorldToScreen;
	Transform ScreenToRaster, RasterToScreen;
public:
	Transform CameraToRaster, RasterToCamera;
	Transform WorldToRaster, RasterToWorld;
	float LensRadius, FocalDistance;
};

}//namespace lux

#endif // LUX_CAMERA_H
