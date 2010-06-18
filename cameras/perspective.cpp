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

// perspective.cpp*
#include "perspective.h"
#include "sampling.h"
#include "mc.h"
#include "scene.h" // for Intersection
#include "film.h" // for Film
#include "reflection/bxdf.h"
#include "light.h"
#include "paramset.h"
#include "dynload.h"
#include "error.h"
#include "epsilon.h"

using namespace lux;

class  PerspectiveBSDF : public BSDF  {
public:
	// PerspectiveBSDF Public Methods
	PerspectiveBSDF(const DifferentialGeometry &dgs, const Normal &ngeom,
		const Volume *exterior, const Volume *interior,
		const PerspectiveCamera &cam, bool lens, const Point &pL) :
		BSDF(dgs, ngeom, exterior, interior), camera(cam),
		hasLens(lens), p(pL) { }
	virtual inline u_int NumComponents() const { return 1; }
	virtual inline u_int NumComponents(BxDFType flags) const {
		return (flags & (BSDF_REFLECTION | BSDF_DIFFUSE)) ==
			(BSDF_REFLECTION | BSDF_DIFFUSE) ? 1U : 0U;
	}
	virtual bool Sample_f(const TsPack *tspack, const Vector &woW, Vector *wiW,
		float u1, float u2, float u3, SWCSpectrum *const f_, float *pdf,
		BxDFType flags = BSDF_ALL, BxDFType *sampledType = NULL,
		float *pdfBack = NULL, bool reverse = false) const {
		if (!reverse || NumComponents(flags) == 0)
			return false;
		Point pS(camera.RasterToWorld(Point(u1, u2, 0.f)));
		*wiW = pS - camera.pos;
		if (hasLens)
			*wiW -= (p - camera.pos) * (Dot(*wiW, nn) / camera.FocalDistance);
		*wiW = Normalize(*wiW);
		const float cosi = Dot(*wiW, nn);
		const float cosi2 = cosi * cosi;
		*pdf = 1.f / (camera.Apixel * cosi2 * cosi);
		if (pdfBack)
			*pdfBack = 0.f;
		*f_ = SWCSpectrum(*pdf / cosi);
		return true;
	}
	virtual float Pdf(const TsPack *tspack, const Vector &woW,
		const Vector &wiW, BxDFType flags = BSDF_ALL) const {
		const float coso = Dot(woW, nn);
		if (NumComponents(flags) == 1 && coso > 0.f) {
			Point pO;
			if (hasLens)
				camera.WorldToRaster(p + woW * (camera.FocalDistance / coso), &pO);
			else
				camera.WorldToRaster(camera.pos + woW, &pO);
			if (pO.x < camera.xStart || pO.x >= camera.xEnd ||
				pO.y < camera.yStart || pO.y >= camera.yEnd)
				return 0.f;
			const float coso2 = coso * coso;
			return 1.f / (camera.Apixel * coso2 * coso);
		}
		return 0.f;
	}
	virtual SWCSpectrum f(const TsPack *tspack, const Vector &woW,
		const Vector &wiW, BxDFType flags = BSDF_ALL) const {
		const float coso = Dot(woW, nn);
		if (NumComponents(flags) == 1 && coso > 0.f) {
			Point pO;
			if (hasLens)
				camera.WorldToRaster(p + woW * (camera.FocalDistance / coso), &pO);
			else
				camera.WorldToRaster(camera.pos + woW, &pO);
			if (pO.x < camera.xStart || pO.x >= camera.xEnd ||
				pO.y < camera.yStart || pO.y >= camera.yEnd)
				return SWCSpectrum(0.f);
			const float coso2 = coso * coso;
			return SWCSpectrum(1.f / (camera.Apixel * coso2 * coso2));
		}
		return SWCSpectrum(0.f);
	}
	virtual SWCSpectrum rho(const TsPack *tspack,
		BxDFType flags = BSDF_ALL) const { return SWCSpectrum(1.f); }
	virtual SWCSpectrum rho(const TsPack *tspack, const Vector &woW,
		BxDFType flags = BSDF_ALL) const { return SWCSpectrum(1.f); }

protected:
	// PerspectiveBSDF Private Methods
	virtual ~PerspectiveBSDF() { }
	const PerspectiveCamera &camera;
	bool hasLens;
	Point p;
};

// PerspectiveCamera Method Definitions
PerspectiveCamera::
    PerspectiveCamera(const Transform &world2camStart,
		const Transform &world2camEnd,
		const float Screen[4], float hither, float yon,
		float sopen, float sclose, int sdist,
		float lensr, float focald, bool autofocus,
		float fov1, int dist, int sh, int pow, Film *f)
	: ProjectiveCamera(world2camStart, world2camEnd,
	    Perspective(fov1, hither, yon),
		Screen, hither, yon, sopen, sclose, sdist,
		lensr, focald, f),
		distribution(dist), shape(sh), power(pow),
		autoFocus(autofocus) {
	pos = CameraToWorld(Point(0,0,0));
	normal = CameraToWorld(Normal(0,0,1));
	fov = Radians(fov1);

	if (LensRadius > 0.f)
		posPdf = 1.0f/(M_PI*LensRadius*LensRadius);
	else
		posPdf = 1.f;

	int xS, xE, yS, yE;
	f->GetSampleExtent(&xS, &xE, &yS, &yE);
	xStart = xS;
	xEnd = xE;
	yStart = yS;
	yEnd = yE;
	const float R = 1.f;
	const float templength = R * tanf(fov / 2.f) * 2.f;	
	const float xPixelWidth = templength * (Screen[1] - Screen[0]) / 2.f *
		(xEnd - xStart) / f->xResolution;
	const float yPixelHeight = templength * (Screen[3] - Screen[2]) / 2.f *
		(yEnd - yStart) / f->yResolution;
	Apixel = xPixelWidth * yPixelHeight;
}

void PerspectiveCamera::SampleMotion(float time)
{
	if (!CameraMotion.isActive)
		return;

	// call base method to sample transform
	ProjectiveCamera::SampleMotion(time);
	// then update derivative transforms
	pos = CameraToWorld(Point(0,0,0));
	normal = CameraToWorld(Normal(0,0,1));
}

void PerspectiveCamera::AutoFocus(Scene* scene)
{
	if (autoFocus) {
		std::stringstream ss;

		// Dade - trace a ray in the middle of the screen
		
		int xstart, xend, ystart, yend;
		film->GetSampleExtent(&xstart, &xend, &ystart, &yend);
		Point Pras((xend - xstart) / 2, (yend - ystart) / 2, 0);

		Point Pcamera;
		RasterToCamera(Pras, &Pcamera);
		Ray ray;
		ray.o = Pcamera;
		ray.d = Vector(Pcamera.x, Pcamera.y, Pcamera.z);
		ray.d = Normalize(ray.d);

		// Dade - I wonder what time I could use here
		ray.time = 0.0f;
		
		ray.mint = 0.f;
		ray.maxt = (ClipYon - ClipHither) / ray.d.z;
		CameraToWorld(ray, &ray);

		Intersection isect;
		if (scene->Intersect(ray, &isect))
			FocalDistance = ray.maxt;
		else
			LOG(LUX_WARNING,LUX_NOERROR)<<"Unable to define the Autofocus focal distance";

		LOG(LUX_INFO,LUX_NOERROR)<<"Autofocus focal distance: " << FocalDistance;
	}
}

float PerspectiveCamera::GenerateRay(const Sample &sample, Ray *ray) const
{
	// Generate raster and camera samples
	Point Pras(sample.imageX, sample.imageY, 0);
	Point Pcamera;
	RasterToCamera(Pras, &Pcamera);
	ray->o = Pcamera;
	ray->d = Vector(Pcamera.x, Pcamera.y, Pcamera.z);
	// Set ray time value
	ray->time = GetTime(sample.time);
	// Modify ray for depth of field
	if (LensRadius > 0.f)
	{
		Point lenP;
		Point lenPCamera;
		// Sample point on lens
		SampleLens(sample.lensU, sample.lensV, &(lenPCamera.x), &(lenPCamera.y));
		lenPCamera.x *= LensRadius;
		lenPCamera.y *= LensRadius;
		lenPCamera.z = 0;
		CameraToWorld(lenPCamera, &lenP);
		float lensU = lenPCamera.x;
		float lensV = lenPCamera.y;

		// Compute point on plane of focus
		float ft = (FocalDistance - ClipHither) / ray->d.z;
		Point Pfocus = (*ray)(ft);
		// Update ray for effect of lens
		ray->o.x += lensU * (FocalDistance - ClipHither) / FocalDistance;
		ray->o.y += lensV * (FocalDistance - ClipHither) / FocalDistance;
		ray->d = Pfocus - ray->o;
	}
	ray->d = Normalize(ray->d);
	ray->mint = 0.;
	ray->maxt = (ClipYon - ClipHither) / ray->d.z;
	CameraToWorld(*ray, ray);
	return 1.f;
}

bool PerspectiveCamera::Sample_W(const TsPack *tspack, const Scene *scene, float u1, float u2, float u3, BSDF **bsdf, float *pdf, SWCSpectrum *We) const
{
	Point psC(0.f);
	if (LensRadius > 0.f) {
		SampleLens(u1, u2, &psC.x, &psC.y);
		psC.x *= LensRadius;
		psC.y *= LensRadius;
	}
	Point ps = CameraToWorld(psC);
	DifferentialGeometry dg(ps, normal, CameraToWorld(Vector(1, 0, 0)), CameraToWorld(Vector(0, 1, 0)), Normal(0, 0, 0), Normal(0, 0, 0), 0, 0, NULL);
	*bsdf = ARENA_ALLOC(tspack->arena, PerspectiveBSDF)(dg, normal,
		NULL, NULL, *this, LensRadius > 0.f, ps);
	*pdf = posPdf;
	*We = SWCSpectrum(posPdf);
	return true;
}
bool PerspectiveCamera::Sample_W(const TsPack *tspack, const Scene *scene, const Point &p, const Normal &n, float u1, float u2, float u3, BSDF **bsdf, float *pdf, float *pdfDirect, VisibilityTester *visibility, SWCSpectrum *We) const
{
	Point psC(0.f);
	if (LensRadius > 0.f) {
		SampleLens(u1, u2, &psC.x, &psC.y);
		psC.x *= LensRadius;
		psC.y *= LensRadius;
	}
	Point ps = CameraToWorld(psC);
	DifferentialGeometry dg(ps, normal, CameraToWorld(Vector(1, 0, 0)), CameraToWorld(Vector(0, 1, 0)), Normal(0, 0, 0), Normal(0, 0, 0), 0, 0, NULL);
	*bsdf = ARENA_ALLOC(tspack->arena, PerspectiveBSDF)(dg, normal,
		NULL, NULL, *this, LensRadius > 0.f, ps);
	*pdf = posPdf;
	*pdfDirect = posPdf;
	visibility->SetSegment(ps, p, tspack->time, true);
	*We = SWCSpectrum(posPdf);
	return true;
}

BBox PerspectiveCamera::Bounds() const
{
	BBox bound(Point(-LensRadius, -LensRadius, 0.f),
		Point(LensRadius, LensRadius, 0.f));
	bound = CameraToWorld(bound);
	bound.Expand(MachineEpsilon::E(bound));
	return bound;
}

bool PerspectiveCamera::GetSamplePosition(const Point &p, const Vector &wi,
	float distance, float *x, float *y) const
{
	const Vector direction(normal);
	const float cosi = Dot(wi, direction);
	if (cosi <= 0.f || (!isinf(distance) && (distance * cosi < ClipHither || distance * cosi > ClipYon)))
		return false;
	if (LensRadius > 0.f) {
		const Point pO(WorldToRaster(p + wi * (FocalDistance / cosi)));
		*x = pO.x;
		*y = pO.y;
	} else {
		const Point pO(WorldToRaster(pos + wi / cosi));
		*x = pO.x;
		*y = pO.y;
	}
	return true;
}

void PerspectiveCamera::ClampRay(Ray &ray) const
{
	const float cosi = Dot(ray.d, normal);
	ray.mint = max(ray.mint, ClipHither / cosi);
	ray.maxt = min(ray.maxt, ClipYon / cosi);
}

void PerspectiveCamera::SampleLens(float u1, float u2, float *dx, float *dy) const
{
	if (shape < 3) {
		ConcentricSampleDisk(u1, u2, dx, dy);
		return;
	}

	static const float halfAngle = M_PI / shape;
	static const float honeyRadius = cosf(halfAngle);

	const float theta = 2.f * M_PI * u2;

	const u_int sector = Floor2UInt(theta / halfAngle);
	const float rho = (sector % 2 == 0) ? theta - sector * halfAngle :
		(sector + 1) * halfAngle - theta;

	float r = honeyRadius / cosf(rho);
	switch (distribution) {
		case 0:
			r *= sqrtf(u1);
			break;
		case 1:
			r *= sqrtf(ExponentialSampleDisk(u1, power));
			break;
		case 2:
			r *= sqrtf(InverseExponentialSampleDisk(u1, power));
			break;
		case 3:
			r *= sqrtf(GaussianSampleDisk(u1));
			break;
		case 4:
			r *= sqrtf(InverseGaussianSampleDisk(u1));
			break;
		case 5:
			r *= sqrtf(TriangularSampleDisk(u1));
			break;
	}
	*dx = r * cosf(theta);
	*dy = r * sinf(theta);
}

Camera* PerspectiveCamera::CreateCamera(const Transform &world2camStart, const Transform &world2camEnd, const ParamSet &params,
	Film *film)
{
	// Extract common camera parameters from _ParamSet_
	float hither = max(1e-3f, params.FindOneFloat("hither", 1e-3f));
	float yon = Clamp(params.FindOneFloat("yon", 1e30f), hither, 1e30f);

	float shutteropen = params.FindOneFloat("shutteropen", 0.f);
	float shutterclose = params.FindOneFloat("shutterclose", 1.f);
	int shutterdist = 0;
	string shutterdistribution = params.FindOneString("shutterdistribution", "uniform");
	if (shutterdistribution == "uniform") shutterdist = 0;
	else if (shutterdistribution == "gaussian") shutterdist = 1;
	else {
		LOG(LUX_WARNING,LUX_BADTOKEN)<<"Distribution  '"<<shutterdistribution<<"' for perspective camera shutter sampling unknown. Using \"uniform\".";
		shutterdist = 0;
	}

	float lensradius = params.FindOneFloat("lensradius", 0.f);
	float focaldistance = params.FindOneFloat("focaldistance", 1e30f);
	bool autofocus = params.FindOneBool("autofocus", false);
	float frame = params.FindOneFloat("frameaspectratio",
		float(film->xResolution)/float(film->yResolution));
	float screen[4];
	if (frame > 1.f) {
		screen[0] = -frame;
		screen[1] =  frame;
		screen[2] = -1.f;
		screen[3] =  1.f;
	}
	else {
		screen[0] = -1.f;
		screen[1] =  1.f;
		screen[2] = -1.f / frame;
		screen[3] =  1.f / frame;
	}
	u_int swi;
	const float *sw = params.FindFloat("screenwindow", &swi);
	if (sw && swi == 4)
		memcpy(screen, sw, 4*sizeof(float));
	float fov = params.FindOneFloat("fov", 90.);

	int distribution = 0;
	string dist = params.FindOneString("distribution", "uniform");
	if (dist == "uniform") distribution = 0;
	else if (dist == "exponential") distribution = 1;
	else if (dist == "inverse exponential") distribution = 2;
	else if (dist == "gaussian") distribution = 3;
	else if (dist == "inverse gaussian") distribution = 4;
	else {
		LOG(LUX_WARNING,LUX_BADTOKEN)<<"Distribution  '"<<dist<<"' for perspective camera DOF sampling unknown. Using \"uniform\".";
		distribution = 0;
	}

	int shape = params.FindOneInt("blades", 0);
	int power = params.FindOneInt("power", 3);

	return new PerspectiveCamera(world2camStart, world2camEnd, screen, hither, yon,
		shutteropen, shutterclose, shutterdist, lensradius, focaldistance, autofocus,
		fov, distribution, shape, power, film);
}

static DynamicLoader::RegisterCamera<PerspectiveCamera> r("perspective");
