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
#include "disk.h"
#include "error.h"

using namespace lux;

#define honeyRad 0.866025403
#define radIndex 57.2957795

class PerspectiveBxDF : public BxDF
{
public:
	PerspectiveBxDF(bool lens, float FD, float f, float A, const Point &pL,
		const Transform &R2C, float xS, float xE, float yS, float yE) :
		BxDF(BxDFType(BSDF_REFLECTION | BSDF_DIFFUSE)), hasLens(lens),
		FocalDistance(FD), fov(f), xStart(xS), xEnd(xE),
		yStart(yS), yEnd(yE), Area(A),
		p(pL), RasterToCamera(R2C) {}
	virtual ~PerspectiveBxDF() { }
	virtual void f(const TsPack *tspack, const Vector &wo, const Vector &wi, SWCSpectrum *const f) const
	{
		Vector wo0(wo);
		wo0.y = -wo0.y;//FIXME
		if (hasLens) {
			wo0 *= FocalDistance / wo.z;
			wo0 += Vector(p.x, p.y, p.z);
		}
		const float cos = Normalize(wo0).z;
		const float cos2 = cos * cos;
		wo0 *= RasterToCamera(Point(0, 0, 0)).z / wo0.z;
		Point p0(RasterToCamera.GetInverse()(Point(wo0.x, wo0.y, wo0.z)));
		if (p0.x < xStart || p0.x >= xEnd || p0.y < yStart || p0.y >= yEnd)
			return;
		*f += SWCSpectrum(1.f / (Area * cos2 * cos2));
	}
	virtual bool Sample_f(const TsPack *tspack, const Vector &wo, Vector *wi, float u1, float u2,
		SWCSpectrum *const f, float *pdf, float *pdfBack = NULL, bool reverse = false) const
	{
		Point pS(RasterToCamera(Point(u1, u2, 0.f)));
		*wi = Vector(pS.x, pS.y, pS.z);
		const float cos = Normalize(*wi).z;
		const float cos2 = cos * cos;
		if (hasLens) {
			Point pF(Point(0, 0, 0) + *wi * (FocalDistance / wi->z));
			*wi = pF - p;
		}
		*wi = Normalize(*wi);
		wi->y = -wi->y;//FIXME
		*pdf = 1.f / (Area * cos2 * cos);
		if (pdfBack)
			*pdfBack = *pdf;//FIXME
		*f = SWCSpectrum(1.f / (Area * cos2 * cos2));
		return true;
	}
	virtual float Pdf(const TsPack *tspack, const Vector &wi, const Vector &wo) const
	{
		Vector wo0(wo);
		wo0.y = -wo0.y;//FIXME
		if (hasLens) {
			wo0 *= FocalDistance / wo.z;
			wo0 += Vector(p.x, p.y, p.z);
		}
		const float cos = Normalize(wo0).z;
		const float cos2 = cos * cos;
		wo0 *= RasterToCamera(Point(0, 0, 0)).z / wo0.z;
		Point p0(RasterToCamera.GetInverse()(Point(wo0.x, wo0.y, wo0.z)));
		if (p0.x < xStart || p0.x >= xEnd || p0.y < yStart || p0.y >= yEnd)
			return 0.f;
		else 
			return 1.f / (Area * cos2 * cos);
	}
private:
	bool hasLens;
	float FocalDistance, fov, xStart, xEnd, yStart, yEnd, Area;
	Point p;
	const Transform &RasterToCamera;
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

	ParamSet paramSet;
	paramSet.AddFloat("radius", &LensRadius);
	lens = boost::shared_ptr<Shape>(Disk::CreateShape(CameraToWorld, false, paramSet));

	if (LensRadius > 0.f)
		posPdf = 1.0f/(M_PI*LensRadius*LensRadius);
	else
		posPdf = 1.f;

	//screen[0]=Screen[0];
	//screen[1]=Screen[1];
	//screen[2]=Screen[2];
	//screen[3]=Screen[3];
	//R = 1/tan(fov*0.5f);
	//xPixelWidth = (screen[1]-screen[0]) / f->xResolution;
	//yPixelHeight = (screen[3]-screen[2]) / f->yResolution;
	//Apixel = xPixelWidth * yPixelHeight;

	R = 1.f;
	float templength = R * tan(fov / 2.f) * 2.f;	
	float frameaspectratio = float(f->xResolution) / float(f->yResolution);
	if (frameaspectratio > 1.f)
	{
		xWidth = templength * frameaspectratio;
		yHeight = templength;
	}
	else
	{
		xWidth = templength;
		yHeight = templength / frameaspectratio;
	}
	int xS, xE, yS, yE;
	f->GetSampleExtent(&xS, &xE, &yS, &yE);
	xStart = xS;
	xEnd = xE;
	yStart = yS;
	yEnd = yE;
	xPixelWidth = xWidth * (Screen[1] - Screen[0]) / 2.f / (xEnd - xStart);
	yPixelHeight = yHeight * (Screen[3] - Screen[2]) / 2.f / (yEnd - yStart);
	Apixel = xPixelWidth * yPixelHeight;
	RasterToCameraBidir = Perspective(fov1, RAY_EPSILON, INFINITY).GetInverse() * RasterToScreen;
	WorldToRasterBidir = RasterToCameraBidir.GetInverse() * WorldToCamera;
}

void PerspectiveCamera::AutoFocus(Scene* scene)
{
	if (autoFocus) {
		std::stringstream ss;

		// Dade - trace a ray in the middle of the screen
		
		int xstart, xend, ystart, yend;
		film->GetSampleExtent(&xstart, &xend, &ystart, &yend);
		Point Pras((xend - xstart) / 2, (yend - ystart) / 2, 0);

		// Dade - debug code
		//ss.str("");
		//ss << "Raster point: " << Pras;
		//luxError(LUX_NOERROR, LUX_INFO, ss.str().c_str());
		
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

		// Dade - debug code
		//ss.str("");
		//ss << "Ray.o: " << ray.o << " Ray.d: " << ray.d;
		//luxError(LUX_NOERROR, LUX_INFO, ss.str().c_str());

		Intersection isect;
		if (scene->Intersect(ray, &isect))
			FocalDistance = ray.maxt;
		else
			luxError(LUX_NOERROR, LUX_WARNING, "Unable to define the Autofocus focal distance");

		ss.str("");
		ss << "Autofocus focal distance: " << FocalDistance;
		luxError(LUX_NOERROR, LUX_INFO, ss.str().c_str());
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
	if (LensRadius > 0.)
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
	DifferentialGeometry dg(ps, normal, CameraToWorld(Vector(1, 0, 0)), CameraToWorld(Vector(0, 1, 0)), Vector(0, 0, 0), Vector(0, 0, 0), 0, 0, NULL);
	*bsdf = BSDF_ALLOC(tspack, SingleBSDF)(dg, normal,
		BSDF_ALLOC(tspack, PerspectiveBxDF)(LensRadius > 0.f, FocalDistance,
		fov, Apixel, psC, RasterToCameraBidir,
		xStart, xEnd, yStart, yEnd));
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
	DifferentialGeometry dg(ps, normal, CameraToWorld(Vector(1, 0, 0)), CameraToWorld(Vector(0, 1, 0)), Vector(0, 0, 0), Vector(0, 0, 0), 0, 0, NULL);
	*bsdf = BSDF_ALLOC(tspack, SingleBSDF)(dg, normal,
		BSDF_ALLOC(tspack, PerspectiveBxDF)(LensRadius > 0.f, FocalDistance,
		fov, Apixel, psC, RasterToCameraBidir,
		xStart, xEnd, yStart, yEnd));
	*pdf = posPdf;
	*pdfDirect = posPdf;
	visibility->SetSegment(p, ps, tspack->time);
	*We = SWCSpectrum(posPdf);
	return true;
}

BBox PerspectiveCamera::Bounds() const
{
	BBox bound(Point(-LensRadius, -LensRadius, 0.f),
		Point(LensRadius, LensRadius, 0.f));
	bound = CameraToWorld(bound);
	bound.Expand(SHADOW_RAY_EPSILON);
	return bound;
}

bool PerspectiveCamera::GetSamplePosition(const Point &p, const Vector &wi, float distance, float *x, float *y) const
{
	Vector direction(normal);
	const float cosi = Dot(wi, direction);
	if (cosi <= 0.f || (distance != -1.f && (distance / cosi < ClipHither || distance / cosi > ClipYon)))
		return false;
	if (LensRadius > 0.f) {
		Point pFC(p + wi * (FocalDistance / Dot(wi, direction)));
		Vector wi0(pFC - pos);
		Point pO(pos + wi0 / Dot(wi0, direction));
		WorldToRasterBidir(pO, &pO);
		*x = pO.x;
		*y = pO.y;
	} else {
		Point pO = WorldToRasterBidir(pos + wi / Dot(wi, direction));
		*x = pO.x;
		*y = pO.y;
	}
	return true;
}

void PerspectiveCamera::ClampRay(Ray &ray) const
{
	const float cosi = Dot(ray.d, normal);
	ray.mint = ClipHither / cosi;
	ray.maxt = ClipYon / cosi;
}

void PerspectiveCamera::SampleLens(float u1, float u2, float *dx, float *dy) const
{
	if (shape < 3) {
		ConcentricSampleDisk(u1, u2, dx, dy);
		return;
	}

	static const int subDiv = 360 / (shape * 2);
	static const float index = subDiv / radIndex;
	static const float honeyRadius = cosf(index);

	int temp = rand() % (shape * 2); //FIXME don't use rand()

	float theta;
	if (shape == 3 && temp % 2 == 0)
		theta = 2.f * M_PI * (temp + sqrtf(u2)) / (shape * 2);
	else
		theta = 2.f * M_PI * (temp + u2) / (shape * 2);

	const int sector = theta / index;
	const float rho = (sector % 2 == 0) ? theta - sector * index :
		(sector - 1) * index - theta;

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
	float hither = max(1e-4f, params.FindOneFloat("hither", 1e-3f));
	float yon = min(params.FindOneFloat("yon", 1e30f), 1e30f);

	float shutteropen = params.FindOneFloat("shutteropen", 0.f);
	float shutterclose = params.FindOneFloat("shutterclose", 1.f);
	int shutterdist = 0;
	string shutterdistribution = params.FindOneString("shutterdistribution", "uniform");
	if (shutterdistribution == "uniform") shutterdist = 0;
	else if (shutterdistribution == "gaussian") shutterdist = 1;
	else {
		std::stringstream ss;
		ss<<"Distribution  '"<<shutterdistribution<<"' for perspective camera shutter sampling unknown. Using \"uniform\".";
		luxError(LUX_BADTOKEN,LUX_WARNING,ss.str().c_str());
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
	int swi;
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
		std::stringstream ss;
		ss<<"Distribution  '"<<dist<<"' for perspective camera DOF sampling unknown. Using \"uniform\".";
		luxError(LUX_BADTOKEN,LUX_WARNING,ss.str().c_str());
		distribution = 0;
	}

	int shape = params.FindOneInt("blades", 0);
	int power = params.FindOneInt("power", 3);

	return new PerspectiveCamera(world2camStart, world2camEnd, screen, hither, yon,
		shutteropen, shutterclose, shutterdist, lensradius, focaldistance, autofocus,
		fov, distribution, shape, power, film);
}

static DynamicLoader::RegisterCamera<PerspectiveCamera> r("perspective");
