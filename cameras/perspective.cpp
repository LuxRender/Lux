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
#include "disk.h"

using namespace lux;

// PerspectiveCamera Method Definitions
PerspectiveCamera::
    PerspectiveCamera(const Transform &world2cam,
		const float Screen[4], float hither, float yon,
		float sopen, float sclose,
		float lensr, float focald,
		float fov1, Film *f)
	: ProjectiveCamera(world2cam,
	    Perspective(fov1, hither, yon),
		Screen, hither, yon, sopen, sclose,
		lensr, focald, f) {
	pos = CameraToWorld(Point(0,0,0));
	normal = CameraToWorld(Normal(0,0,1));
	fov = Radians(fov1);

	ParamSet paramSet;
	paramSet.AddFloat("radius", &LensRadius);
	lens = boost::shared_ptr<Shape>(Disk::CreateShape(CameraToWorld, false, paramSet));

	posPdf = 1.0f/(M_PI*LensRadius*LensRadius);

	//screen[0]=Screen[0];
	//screen[1]=Screen[1];
	//screen[2]=Screen[2];
	//screen[3]=Screen[3];
	//R = 1/tan(fov*0.5f);
	//xPixelWidth = (screen[1]-screen[0]) / f->xResolution;
	//yPixelHeight = (screen[3]-screen[2]) / f->yResolution;
	//Apixel = xPixelWidth * yPixelHeight;

	R = 100;
	float templength=R * tan(fov*0.5f)*2;	
	float frameaspectratio=float(f->xResolution)/float(f->yResolution);
	if (frameaspectratio > 1.f)
	{
		xWidth=templength*frameaspectratio;
		yHeight=templength;
	}
	else
	{
		xWidth=templength;
		yHeight=templength / frameaspectratio;
	}
	xPixelWidth = xWidth / f->xResolution;
	yPixelHeight = yHeight / f->yResolution;
	Apixel = xPixelWidth * yPixelHeight;

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
	ray->time = Lerp(sample.time, ShutterOpen, ShutterClose);
	// Modify ray for depth of field
	if (LensRadius > 0.)
	{
		Point lenP;
		float lenPdf;
		// Sample point on lens
		SamplePosition(sample.lensU, sample.lensV, &lenP, &lenPdf);
		Point lenPCamera(WorldToCamera(lenP));
		float lensU = lenPCamera.x;
		float lensV = lenPCamera.y;;
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
SWCSpectrum PerspectiveCamera::Sample_W(const Scene *scene, float u1, float u2, BSDF **bsdf, float *pdf) const
{
	Point ps(0.f);
	ConcentricSampleDisk(u1, u2, &ps.x, &ps.y);
	ps.x *= LensRadius;
	ps.y *= LensRadius;
	CameraToWorld(ps, &ps);
	Normal ns(CameraToWorld(Normal(0, 0, 1)));
	DifferentialGeometry dg(ps, ns, CameraToWorld(Vector(1, 0, 0)), CameraToWorld(Vector(0, 1, 0)), Vector(0, 0, 0), Vector(0, 0, 0), 0, 0, NULL);
	*bsdf = BSDF_ALLOC(BSDF)(dg, ns);
//	(*bsdf)->Add(BSDF_ALLOC(PerspectiveBxDF)());
	*pdf = posPdf;
	return SWCSpectrum(1.f);
}
SWCSpectrum PerspectiveCamera::Sample_W(const Scene *scene, const Point &p, const Normal &n, float u1, float u2, BSDF **bsdf, float *pdf, float *pdfDirect, VisibilityTester *visibility) const
{
	Point ps(0.f);
	ConcentricSampleDisk(u1, u2, &ps.x, &ps.y);
	ps.x *= LensRadius;
	ps.y *= LensRadius;
	CameraToWorld(ps, &ps);
	Normal ns(CameraToWorld(Normal(0, 0, 1)));
	DifferentialGeometry dg(ps, ns, CameraToWorld(Vector(1, 0, 0)), CameraToWorld(Vector(0, 1, 0)), Vector(0, 0, 0), Vector(0, 0, 0), 0, 0, NULL);
	*bsdf = BSDF_ALLOC(BSDF)(dg, ns);
//	(*bsdf)->Add(BSDF_ALLOC(PerspectiveBxDF)());
	*pdf = posPdf;
	*pdfDirect = posPdf;
	visibility->SetSegment(p, ps);
	return SWCSpectrum(1.f);
}
float PerspectiveCamera::Pdf(const Point &p, const Normal &n, const Vector &wi) const
{
	return posPdf;
}
void PerspectiveCamera::GetSamplePosition(const Point &p, const Vector &wi, float *x, float *y) const
{
}

bool PerspectiveCamera::IsVisibleFromEyes(const Scene *scene, const Point &lenP, const Point &worldP, Sample* sample_gen, Ray *ray_gen) const
{
	//TODO: check whether IsVisibleFromEyes() can always return correct answer.
	bool isVisible;
	Point point;
	if (LensRadius > 0)
	{
		Ray r(WorldToCamera(Ray(lenP, worldP - lenP)));
		float ft = FocalDistance / r.d.z;
		point = CameraToWorld(r(ft));
	}
	else
	{
		point = worldP;
	}

	if (GenerateSample(point, sample_gen))
	{
		GenerateRay(*sample_gen, ray_gen);
		//Ray ray1(worldP, -ray_gen->d);
		Ray ray1(worldP, lenP-worldP, RAY_EPSILON, 1.f - RAY_EPSILON);

		//if (Dot(ray_gen->d,normal)<0)
		{
			Intersection isect1;
			if (scene->Intersect(ray1,&isect1))
			{
				float z = WorldToCamera(isect1.dg.p).z;
				isVisible = z<ClipHither || z>ClipYon;
			}
			else
				isVisible = true;
		}
		//else
		//	isVisible = false;
	}
	else
		isVisible = false;
	return isVisible;
}
float PerspectiveCamera::GetConnectingFactor(const Point &lenP, const Point &worldP, const Vector &wo, const Normal &n) const
{
	return AbsDot(wo, normal)*AbsDot(wo, n)/DistanceSquared(lenP, worldP);
}
void PerspectiveCamera::GetFlux2RadianceFactors(Film *film, float *factors, int xPixelCount, int yPixelCount) const
{
	float d2,cos2,cos4,detaX,detaY;
	int x,y;
	for (y = 0; y < yPixelCount; ++y)
	{
		for (x = 0; x < xPixelCount; ++x)
		{
			//detaX = screen[0] - (x + 0.5f) * xPixelWidth;
			//detaY = screen[2] - (y + 0.5f) * yPixelHeight;

			detaX = 0.5f * xWidth - (x+ 0.5f) * xPixelWidth;
			detaY = 0.5f * yHeight - (y + 0.5f) * yPixelHeight;
			d2 = detaX*detaX + detaY*detaY + R*R;
			cos2 = R*R / d2;
			cos4 = cos2 * cos2;
			factors[x+y*xPixelCount] =  R*R / (Apixel*cos4);
		}
	}
}
void PerspectiveCamera::SamplePosition(float u1, float u2, Point *p, float *pdf) const
{
	if (LensRadius==0.0f)
	{
		*p = pos;
		*pdf = 1.0f;
	}
	ConcentricSampleDisk(u1, u2, &(p->x), &(p->y));
	p->x *= LensRadius;
	p->y *= LensRadius;
	p->z = 0;
	*p = CameraToWorld(*p);
	*pdf = posPdf;
}
float  PerspectiveCamera::EvalPositionPdf() const
{
	return LensRadius==0.0f ? 0 : posPdf;
}
bool PerspectiveCamera::Intersect(const Ray &ray, Intersection *isect) const
{
	// Backface culling
	if (WorldToCamera(ray.d).z>0)
		return false;
	// Copy from GeometricPrimitive::Intersect()
	float thit;
	if (!lens->Intersect(ray, &thit, &isect->dg))
		return false;
	// no primitive, only a disk shape
	isect->primitive = NULL;
	isect->WorldToObject = lens->WorldToObject;
	ray.maxt = thit;
	return true;
}
Camera* PerspectiveCamera::CreateCamera(const ParamSet &params,
		const Transform &world2cam, Film *film) {
	// Extract common camera parameters from _ParamSet_
	float hither = max(1e-4f, params.FindOneFloat("hither", 1e-3f));
	float yon = min(params.FindOneFloat("yon", 1e30f), 1e30f);
	float shutteropen = params.FindOneFloat("shutteropen", 0.f);
	float shutterclose = params.FindOneFloat("shutterclose", 1.f);
	float lensradius = params.FindOneFloat("lensradius", 0.f);
	float focaldistance = params.FindOneFloat("focaldistance", 1e30f);
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
	return new PerspectiveCamera(world2cam, screen, hither, yon,
		shutteropen, shutterclose, lensradius, focaldistance,
		fov, film);
}
