/***************************************************************************
 *   Copyright (C) 1998-2013 by authors (see AUTHORS.txt)                  *
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
#include "mc.h"
#include "scene.h" // for struct Intersection
#include "film.h" // for Film
#include "bxdf.h"
#include "singlebsdf.h"
#include "light.h"
#include "paramset.h"
#include "dynload.h"

using namespace lux;

class EnvironmentBxDF : public BxDF {
public:
	EnvironmentBxDF() :
		BxDF(BxDFType(BSDF_REFLECTION | BSDF_DIFFUSE)) {}
	virtual ~EnvironmentBxDF() { }
	virtual void F(const SpectrumWavelengths &sw, const Vector &wo,
		const Vector &wi, SWCSpectrum *const F_) const {
		*F_ += SWCSpectrum(SameHemisphere(wo, wi) ? fabsf(wo.z) * INV_PI : 0.f);
	}
};

// EnvironmentCamera Method Definitions
EnvironmentCamera::EnvironmentCamera(const MotionSystem &world2cam,
	float hither, float yon, float sopen,
	float sclose, int sdist, Film *film)
	: Camera(world2cam, hither, yon, sopen, sclose,
		sdist, film)
{
		pos = CameraToWorld * Point(0, 0, 0);
}

void EnvironmentCamera::SampleMotion(float time)
{
	if (CameraMotion.IsStatic())
		return;

	// call base method to sample transform
	Camera::SampleMotion(time);
	// then update derivative transforms
	pos = CameraToWorld * Point(0,0,0);
}

bool EnvironmentCamera::SampleW(MemoryArena &arena,
	const SpectrumWavelengths &sw, const Scene &scene,
	float u1, float u2, float u3, BSDF **bsdf, float *pdf,
	SWCSpectrum *We) const
{
	const float theta = M_PI * u2 / film->yResolution;
	const float phi = 2 * M_PI * u1 / film->xResolution;
	Normal ns(-sinf(theta) * sinf(phi), cosf(theta),
		-sinf(theta) * cosf(phi));
	ns *= CameraToWorld;
	Vector dpdu, dpdv;
	CoordinateSystem(Vector(ns), &dpdu, &dpdv);
	DifferentialGeometry dg(pos, ns, dpdu, dpdv, Normal(0, 0, 0), Normal(0, 0, 0), 0, 0, NULL);
	const Volume *v = GetVolume();
	*bsdf = ARENA_ALLOC(arena, SingleBSDF)(dg, ns,
		ARENA_ALLOC(arena, EnvironmentBxDF)(), v, v);
	*pdf = 1.f / (2.f * M_PI * M_PI * sinf(theta));
	*We = SWCSpectrum(1.f);
	return true;
}
bool EnvironmentCamera::SampleW(MemoryArena &arena,
	const SpectrumWavelengths &sw, const Scene &scene,
	const Point &p, const Normal &n, float u1, float u2, float u3,
	BSDF **bsdf, float *pdf, float *pdfDirect, SWCSpectrum *We) const
{
	const Vector w(p - pos);
	Normal ns(Normalize(w));
	Vector dpdu, dpdv;
	CoordinateSystem(Vector(ns), &dpdu, &dpdv);
	DifferentialGeometry dg(pos, ns, dpdu, dpdv, Normal(0, 0, 0), Normal(0, 0, 0), 0, 0, NULL);
	const Volume *v = GetVolume();
	*bsdf = ARENA_ALLOC(arena, SingleBSDF)(dg, ns,
		ARENA_ALLOC(arena, EnvironmentBxDF)(), v, v);
	*pdf = 1.f / (2.f * M_PI * M_PI * sqrtf(max(0.f, 1.f - ns.y * ns.y)));
	*pdfDirect = 1.f;
	*We = SWCSpectrum(1.f);
	return true;
}

BBox EnvironmentCamera::Bounds() const
{
	// TODO - improve this
	BBox bound;
	for (int i = 1024; i >= 0; i--) {
		// ugly hack, but last thing we do is to sample StartTime, so should be ok
		const_cast<EnvironmentCamera*>(this)->SampleMotion(Lerp(static_cast<float>(i) / 1024.f, CameraMotion.StartTime(), CameraMotion.EndTime()));
		bound = Union(bound, BBox(pos));
	}

	bound.Expand(max(1.f, MachineEpsilon::E(bound)));

	return bound;
}

bool EnvironmentCamera::GetSamplePosition(const Point &p, const Vector &wi,
	float distance, float *x, float *y) const
{
	if (!isinf(distance) && (distance < ClipHither || distance > ClipYon))
		return false;
	const Vector w(Inverse(CameraToWorld) * wi);
	const float cosTheta = w.y;
	const float theta = acosf(min(1.f, cosTheta));
	*y = theta * film->yResolution * INV_PI;
	const float sinTheta = sqrtf(Clamp(1.f - cosTheta * cosTheta, 1e-5f, 1.f));
	const float cosPhi = -w.z / sinTheta;
	const float phi = acosf(Clamp(cosPhi, -1.f, 1.f));
	if (w.x >= 0.f)
		*x = (2.f * M_PI - phi) * film->xResolution * INV_TWOPI;
	else
		*x = phi * film->xResolution * INV_TWOPI;

	return true;
}

void EnvironmentCamera::ClampRay(Ray &ray) const
{
	ray.mint = max(ray.mint, ClipHither);
	ray.maxt = min(ray.maxt, ClipYon);
}

Camera* EnvironmentCamera::CreateCamera(const MotionSystem &world2cam,
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
		LOG(LUX_WARNING,LUX_BADTOKEN) << "Distribution  '" <<
			shutterdistribution <<
			"' for environment camera shutter sampling unknown. Using \"uniform\".";
		shutterdist = 0;
	}

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
	u_int swi;
	const float *sw = params.FindFloat("screenwindow", &swi);
	if (sw && swi == 4)
		memcpy(screen, sw, 4*sizeof(float));
	(void) lensradius; // don't need this
	(void) focaldistance; // don't need this
	return new EnvironmentCamera(world2cam, hither, yon,
		shutteropen, shutterclose, shutterdist, film);
}

static DynamicLoader::RegisterCamera<EnvironmentCamera> r("environment");
