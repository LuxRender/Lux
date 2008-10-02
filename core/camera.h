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
#include "spectrum.h"
#include "error.h"

namespace lux
{
// Camera Declarations
class  Camera {
public:
	// Camera Interface
	Camera(const Transform &world2cam, float hither, float yon,
		float sopen, float sclose, Film *film);
	virtual ~Camera();
	virtual float GenerateRay(const Sample &sample, Ray *ray) const = 0;
	virtual SWCSpectrum Sample_W(const TsPack *tspack, const Scene *scene, float u1, float u2, float u3, BSDF **bsdf, float *pdf) const {luxError(LUX_BUG, LUX_SEVERE, "Unimplemented Camera::Sample_W"); return 0.f;}
	virtual SWCSpectrum Sample_W(const TsPack *tspack, const Scene *scene, const Point &p, const Normal &n, float u1, float u2, float u3, BSDF **bsdf, float *pdf, float *pdfDirect, VisibilityTester *visibility) const {luxError(LUX_BUG, LUX_SEVERE, "Unimplemented Camera::Sample_W"); return 0.f;}
	virtual float Pdf(const Point &p, const Normal &n, const Vector &wi) const {return 0.f;}
	virtual void GetSamplePosition(const Point &p, const Vector &wi, float *x, float *y) const {}
	virtual bool IsVisibleFromEyes(const Scene *scene, const Point &lenP, const Point &worldP, Sample* sample_gen, Ray *ray_gen) const;
	virtual float GetConnectingFactor(const Point &lenP, const Point &worldP, const Vector &wo, const Normal &n) const;
	virtual void GetFlux2RadianceFactors(Film *film, float *factors, int xPixelCount, int yPixelCount) const;
	virtual bool IsDelta() const;
	virtual void SamplePosition(float u1, float u2, float u3, Point *p, float *pdf) const;
	virtual float EvalPositionPdf() const;
	virtual bool Intersect(const Ray &ray, Intersection *isect) const;
	virtual void AutoFocus(Scene* scene) { }

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
	Transform WorldToRaster, RasterToWorld;
	float LensRadius, FocalDistance;
};

}//namespace lux

#endif // LUX_CAMERA_H
