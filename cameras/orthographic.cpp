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

// orthographic.cpp*
#include "orthographic.h"
#include "sampling.h"
#include "mc.h"
#include "scene.h" // for struct Intersection
#include "film.h" // for Film
#include "specularreflection.h"
#include "fresnelnoop.h"
#include "singlebsdf.h"
#include "paramset.h"
#include "dynload.h"
#include "motionsystem.h"

using namespace lux;

// OrthographicCamera Definitions
OrthoCamera::OrthoCamera(const MotionSystem &world2cam,
	const float Screen[4], float hither, float yon,
	float sopen, float sclose, int sdist, float lensr,
	float focald, bool autofocus, Film *f)
	: ProjectiveCamera(world2cam, 
		Orthographic(hither, yon), Screen, hither, yon, sopen, sclose,
		sdist, lensr, focald, f), autoFocus(autofocus)
{
	screenDx = Screen[1] - Screen[0];
	screenDy = Screen[3] - Screen[2];
	posPdf = (film->xResolution * film->yResolution) / (screenDx * screenDy);
	normal = CameraToWorld * Normal(0, 0, 1);
}

void OrthoCamera::SampleMotion(float time)
{
	if (CameraMotion.IsStatic())
		return;

	// call base method to sample transform
	ProjectiveCamera::SampleMotion(time);
	// then update derivative transforms
	normal = CameraToWorld * Normal(0,0,1);
}

void OrthoCamera::AutoFocus(const Scene &scene)
{
	if (autoFocus) {
		std::stringstream ss;

		// Dade - trace a ray in the middle of the screen
		
		int xstart, xend, ystart, yend;
		film->GetSampleExtent(&xstart, &xend, &ystart, &yend);
		Point Pras((xend - xstart) / 2, (yend - ystart) / 2, 0);

		Point Pcamera(RasterToCamera * Pras);
		Ray ray;
		ray.o = Pcamera;
		ray.d = Vector(0,0,1);

		// Dade - I wonder what time I could use here
		ray.time = 0.0f;
		
		ray.mint = 0.f;
		ray.maxt = ClipYon - ClipHither;
		ray *= CameraToWorld;

		Intersection isect;
		if (scene.Intersect(ray, &isect))
			FocalDistance = ray.maxt;
		else
			LOG(LUX_WARNING, LUX_NOERROR) <<
				"Unable to define the Autofocus focal distance";

		LOG(LUX_INFO, LUX_NOERROR) << "Autofocus focal distance: " <<
			FocalDistance;
	}
}

bool OrthoCamera::SampleW(MemoryArena &arena, const SpectrumWavelengths &sw,
	const Scene &scene, float u1, float u2, float u3, BSDF **bsdf,
	float *pdf, SWCSpectrum *We) const
{
	Point psC(RasterToCamera * Point(u1, u2, 0.f));
	psC.z = 0.f;
	const Point ps(CameraToWorld * psC);
	DifferentialGeometry dg(ps, normal, CameraToWorld * Vector(1, 0, 0),
		CameraToWorld * Vector(0, 1, 0), Normal(0, 0, 0),
		Normal(0, 0, 0), 0, 0, NULL);
	const Volume *v = GetVolume();
	*bsdf = ARENA_ALLOC(arena, SingleBSDF)(dg, normal,
		ARENA_ALLOC(arena, SpecularReflection)(SWCSpectrum(1.f),
		ARENA_ALLOC(arena, FresnelNoOp)(), 0.f, 0.f), v, v);
	*pdf = posPdf;
	*We = SWCSpectrum(1.f);
	return true;
}

bool OrthoCamera::SampleW(MemoryArena &arena, const SpectrumWavelengths &sw,
	const Scene &scene, const Point &p, const Normal &n,
	float u1, float u2, float u3,
	BSDF **bsdf, float *pdf, float *pdfDirect, SWCSpectrum *We) const
{
	return false;
}

bool OrthoCamera::GetSamplePosition(const Point &p, const Vector &wi,
	float distance, float *x, float *y) const
{
	if (Dot(wi, normal) < 1.f - MachineEpsilon::E(1.f) ||
		(!isinf(distance) && (distance < ClipHither ||
		distance > ClipYon)))
		return false;
	Point ps(Inverse(RasterToWorld) * p);
	*x = ps.x;
	*y = ps.y;
	return true;
}

void OrthoCamera::ClampRay(Ray &ray) const
{
	ray.mint = max(ray.mint, ClipHither);
	ray.maxt = min(ray.maxt, ClipYon);
}

BBox OrthoCamera::Bounds() const
{
	BBox orig_bound(Point(-1, -1, 0), Point(1, 1, 0));
	// TODO - improve this
	BBox bound;
	for (int i = 1024; i >= 0; i--) {
		// ugly hack, but last thing we do is to sample StartTime, so should be ok
		const_cast<OrthoCamera*>(this)->SampleMotion(Lerp(static_cast<float>(i) / 1024.f, CameraMotion.StartTime(), CameraMotion.EndTime()));
		bound = Union(bound, ScreenToWorld * orig_bound);
	}
	bound.Expand(max(1.f, MachineEpsilon::E(bound)));
	return bound;
}

Camera* OrthoCamera::CreateCamera(const MotionSystem &world2cam,
	const ParamSet &params, Film *film)
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
		LOG(LUX_WARNING, LUX_BADTOKEN) << "Distribution  '" <<
			shutterdistribution <<
			"' for orthographic camera shutter sampling unknown. Using \"uniform\".";
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
	} else {
		screen[0] = -1.f;
		screen[1] =  1.f;
		screen[2] = -1.f / frame;
		screen[3] =  1.f / frame;
	}
	u_int swi;
	const float *sw = params.FindFloat("screenwindow", &swi);
	if (sw && swi == 4)
		memcpy(screen, sw, 4*sizeof(float));
	return new OrthoCamera(world2cam, screen, hither,
		yon, shutteropen, shutterclose, shutterdist, lensradius,
		focaldistance, autofocus, film);
}

static DynamicLoader::RegisterCamera<OrthoCamera> r("orthographic");
