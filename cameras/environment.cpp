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

// environment.cpp*
#include "environment.h"
#include "sampling.h"
#include "scene.h" // for struct Intersection
#include "film.h" // for Film
#include "paramset.h"

using namespace lux;

// EnvironmentCamera Method Definitions
EnvironmentCamera::
    EnvironmentCamera(const Transform &world2cam,
		float hither, float yon, float sopen, float sclose,
		Film *film)
	: Camera(world2cam, hither, yon, sopen, sclose, film) {
	rayOrigin = CameraToWorld(Point(0,0,0));
}
float EnvironmentCamera::GenerateRay(const Sample &sample,
		Ray *ray) const {
	ray->o = rayOrigin;
	// Generate environment camera ray direction
	float theta = M_PI * sample.imageY / film->yResolution;
	float phi = 2 * M_PI * sample.imageX / film->xResolution;
	Vector dir(sinf(theta) * cosf(phi), cosf(theta),
		sinf(theta) * sinf(phi));
	CameraToWorld(dir, &ray->d);
	// Set ray time value
	ray->time = Lerp(sample.time, ShutterOpen, ShutterClose);
	ray->mint = ClipHither;
	ray->maxt = ClipYon;
	return 1.f;
}
bool EnvironmentCamera::GenerateSample(const Point &p, Sample *sample) const
{
	Vector dir_world(p-rayOrigin);
	Vector dir_camera;
	WorldToCamera(dir_world, &dir_camera);
	dir_camera = Normalize(dir_camera);
	float theta, phi;
	theta = acosf(dir_camera.y);
	phi = acosf(dir_camera.x/sinf(theta));
	if (isnan(phi))
		phi=atanf(dir_camera.z/dir_camera.x);
	if (dir_camera.z<0)
		phi = M_PI * 2 - phi;

	sample->imageX = min (phi * INV_TWOPI, 1.0f) * film->xResolution ;
	sample->imageY = min (theta * INV_PI, 1.0f) * film->yResolution ;
	//return (sample->imageX>=film->xPixelStart &&
	//	sample->imageX<film->xPixelStart+film->xPixelCount &&
	//	sample->imageY>=film->yPixelStart &&
	//	sample->imageY<film->yPixelStart+film->yPixelCount);

	//static float mm=0;
	//if (phi*INV_TWOPI>1)
	//	printf("\n%f,%f\n",mm=max(phi*INV_TWOPI,mm),theta*INV_PI);

	return true;
}
bool EnvironmentCamera::IsVisibleFromEyes(const Scene *scene, const Point &lenP, const Point &worldP, Sample* sample_gen, Ray *ray_gen) const
{
	bool isVisible;
	if (GenerateSample(worldP, sample_gen))
	{
		GenerateRay(*sample_gen, ray_gen);
		ray_gen->maxt = Distance(ray_gen->o, worldP)*(1-RAY_EPSILON);
		isVisible = !scene->IntersectP(*ray_gen);
	}
	else
		isVisible = false;
	return isVisible;
}
float EnvironmentCamera::GetConnectingFactor(const Point &lenP, const Point &worldP, const Vector &wo, const Normal &n) const
{
	return AbsDot(wo, n) / DistanceSquared(lenP, worldP);
}
void EnvironmentCamera::GetFlux2RadianceFactors(Film *film, float *factors, int xPixelCount, int yPixelCount) const
{
	float Apixel,R = 100.0f;
	int x,y;
	for (y = 0; y < yPixelCount; ++y) {
		for (x = 0; x < xPixelCount; ++x) {
			Apixel = 2*M_PI/film->xResolution*R*sinf(M_PI*(y+0.5f)/film->yResolution) * M_PI/film->yResolution*R;
			factors[x+y*xPixelCount] =  R*R / Apixel;
		}
	}
}
void EnvironmentCamera::SamplePosition(float u1, float u2, Point *p, float *pdf) const
{
	*p = rayOrigin;
	*pdf = 1.0f;
}
float EnvironmentCamera::EvalPositionPdf() const
{
	return 1.0f;
}
	
Camera* EnvironmentCamera::CreateCamera(const ParamSet &params,
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
	(void) lensradius; // don't need this
	(void) focaldistance; // don't need this
	return new EnvironmentCamera(world2cam, hither, yon,
		shutteropen, shutterclose, film);
}
