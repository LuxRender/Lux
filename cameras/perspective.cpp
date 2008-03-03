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
#include "mc.h"
#include "scene.h" // for struct Intersection

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
}
float PerspectiveCamera::GenerateRay(const Sample &sample,
		Ray *ray) const {
	// Generate raster and camera samples
	Point Pras(sample.imageX, sample.imageY, 0);
	Point Pcamera;
	RasterToCamera(Pras, &Pcamera);
	ray->o = Pcamera;
	ray->d = Vector(Pcamera.x, Pcamera.y, Pcamera.z);
	// Set ray time value
	ray->time = Lerp(sample.time, ShutterOpen, ShutterClose);
	// Modify ray for depth of field
	if (LensRadius > 0.) {
		// Sample point on lens
		float lensU, lensV;
		ConcentricSampleDisk(sample.lensU, sample.lensV,
		                     &lensU, &lensV);
		lensU *= LensRadius;
		lensV *= LensRadius;
		// Compute point on plane of focus
		float ft = (FocalDistance - ClipHither) / ray->d.z;
		Point Pfocus = (*ray)(ft);
		// Update ray for effect of lens
		ray->o.x += lensU;
		ray->o.y += lensV;
		ray->d = Pfocus - ray->o;
	}
	ray->d = Normalize(ray->d);
	ray->mint = 0.;
	ray->maxt = (ClipYon - ClipHither) / ray->d.z;
	CameraToWorld(*ray, ray);
	return 1.f;
}
bool PerspectiveCamera::IsVisibleFromEyes(const Scene *scene, const Point &p, Sample_stub * sample_gen, Ray *ray_gen)
{
	//TODO: check whether IsVisibleFromEyes() can alway return correct answer.
	bool isVisible;
	if (GenerateSample(p, (Sample *)sample_gen))
	{
		GenerateRay(*(Sample *)sample_gen, ray_gen);
		Vector dd(pos-p);
		Ray ray1(p, -ray_gen->d);

		if (Dot(dd,normal)<0)
		{
			Intersection isect1;
			if (scene->Intersect(ray1,&isect1))
				isVisible = WorldToCamera(isect1.dg.p).z<0 ;
			else
				isVisible = true;
		}
		else
			isVisible = false;
	}
	else
		isVisible = false;
	return isVisible;
}
float PerspectiveCamera::GetConnectingFactor(const Point &p, const Vector &wo, const Normal &n)
{
	return AbsDot(wo, normal)*AbsDot(wo, n)/DistanceSquared(pos, p);
}
void PerspectiveCamera::GetFlux2RadianceFactor(Film *film, int xPixelCount, int yPixelCount)
{
	float templength, frameaspectratio;
	float xWidth, yHeight, xPixelWidth, yPixelHeight, Apixel;
	float R,d2,cos2,cos4,detaX,detaY;
	int x,y;
	R = 100;
	templength=R * tan(fov*0.5)*2;	
	frameaspectratio=float(film->xResolution)/float(film->yResolution);

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
	xPixelWidth = xWidth / film->xResolution;
	yPixelHeight = yHeight / film->yResolution;
	Apixel = xPixelWidth * yPixelHeight;


	for (y = 0; y < yPixelCount; ++y) {
		for (x = 0; x < xPixelCount; ++x) {
			detaX = 0.5*xWidth - (x+0.5)*xPixelWidth;
			detaY = 0.5*yHeight - (y+0.5)*yPixelHeight;
			d2 = detaX*detaX + detaY*detaY + R*R;
			cos2 = R*R / d2;
			cos4 = cos2 * cos2;
			film->flux2radiance[x+y*xPixelCount] =  R*R / (Apixel*cos4);
		}
	}
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
