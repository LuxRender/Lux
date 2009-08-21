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

// environment.cpp*
#include "environment.h"
#include "sampling.h"
#include "mc.h"
#include "scene.h" // for struct Intersection
#include "film.h" // for Film
#include "reflection/bxdf.h"
#include "light.h"
#include "paramset.h"
#include "dynload.h"

using namespace lux;

class EnvironmentBxDF : public BxDF {
public:
	EnvironmentBxDF() :
		BxDF(BxDFType(BSDF_REFLECTION | BSDF_DIFFUSE)) {}
	virtual ~EnvironmentBxDF() { }
	virtual void f(const TsPack *tspack, const Vector &wo, const Vector &wi, SWCSpectrum *const F) const {
		*F += SWCSpectrum(SameHemisphere(wo, wi) ? fabsf(wi.z) * INV_PI : 0.f);
	}
};

// EnvironmentCamera Method Definitions
EnvironmentCamera::
    EnvironmentCamera(const Transform &world2camStart, const Transform &world2camEnd, 
		float hither, float yon, float sopen, float sclose, int sdist,
		Film *film)
	: Camera(world2camStart, world2camEnd, hither, yon, sopen, sclose, sdist, film) {
		pos = CameraToWorld(Point(0, 0, 0));
}
float EnvironmentCamera::GenerateRay(const Sample &sample,
		Ray *ray) const {
	ray->o = CameraToWorld(Point(0,0,0));
	// Generate environment camera ray direction
	float theta = M_PI * sample.imageY / film->yResolution;
	float phi = 2 * M_PI * sample.imageX / film->xResolution;
	Vector dir(sinf(theta) * cosf(phi), cosf(theta),
		sinf(theta) * sinf(phi));
	CameraToWorld(dir, &ray->d);
	// Set ray time value
	ray->time = GetTime(sample.time);
	ray->mint = ClipHither;
	ray->maxt = ClipYon;
	return 1.f;
}
	
bool EnvironmentCamera::Sample_W(const TsPack *tspack, const Scene *scene, float u1, float u2, float u3, BSDF **bsdf, float *pdf, SWCSpectrum *We) const
{
	const float theta = M_PI * u2 / film->yResolution;
	const float phi = 2 * M_PI * u1 / film->xResolution;
	Normal ns(sinf(theta) * cosf(phi), cosf(theta),
		sinf(theta) * sinf(phi));
	CameraToWorld(ns, &ns);
	Vector dpdu, dpdv;
	CoordinateSystem(Vector(ns), &dpdu, &dpdv);
	DifferentialGeometry dg(pos, ns, dpdu, dpdv, Normal(0, 0, 0), Normal(0, 0, 0), 0, 0, NULL);
	*bsdf = BSDF_ALLOC(tspack, SingleBSDF)(dg, ns,
		BSDF_ALLOC(tspack, EnvironmentBxDF)());
	*pdf = UniformSpherePdf();
	*We = SWCSpectrum(*pdf);
	return true;
}
bool EnvironmentCamera::Sample_W(const TsPack *tspack, const Scene *scene, const Point &p, const Normal &n, float u1, float u2, float u3, BSDF **bsdf, float *pdf, float *pdfDirect, VisibilityTester *visibility, SWCSpectrum *We) const
{
	const Vector w(p - pos);
	Normal ns(Normalize(w));
	Vector dpdu, dpdv;
	CoordinateSystem(Vector(ns), &dpdu, &dpdv);
	DifferentialGeometry dg(pos, ns, dpdu, dpdv, Normal(0, 0, 0), Normal(0, 0, 0), 0, 0, NULL);
	*bsdf = BSDF_ALLOC(tspack, SingleBSDF)(dg, ns,
		BSDF_ALLOC(tspack, EnvironmentBxDF)());
	*pdf = UniformSpherePdf();
	*pdfDirect = 1.f;
	visibility->SetSegment(p, pos, tspack->time);
	*We = SWCSpectrum(*pdf);
	return true;
}

BBox EnvironmentCamera::Bounds() const
{
	BBox bound(pos);
	bound.Expand(SHADOW_RAY_EPSILON);
	return bound;
}

bool EnvironmentCamera::GetSamplePosition(const Point &p, const Vector &wi, float distance, float *x, float *y) const
{
	if (distance < ClipHither || distance > ClipYon)
		return false;
	const Vector w = WorldToCamera(wi);
	const float cosTheta = w.y;
	const float theta = acos(min(1.f, cosTheta));
	*y = theta * film->yResolution * INV_PI;
	const float sinTheta = sqrtf(Clamp(1.f - cosTheta * cosTheta, 1e-5f, 1.f));
	const float cosPhi = w.x / sinTheta;
	const float phi = acos(Clamp(cosPhi, -1.f, 1.f));
	if (w.z >= 0.f)
		*x = phi * film->xResolution * INV_TWOPI;
	else
		*x = (2.f * M_PI - phi) * film->xResolution * INV_TWOPI;

	return true;
}

void EnvironmentCamera::ClampRay(Ray &ray) const
{
	ray.mint = ClipHither;
	ray.maxt = ClipYon;
}

Camera* EnvironmentCamera::CreateCamera(const Transform &world2camStart, const Transform &world2camEnd, 
	const ParamSet &params,	Film *film)
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
	return new EnvironmentCamera(world2camStart, world2camEnd, hither, yon,
		shutteropen, shutterclose, shutterdist, film);
}

static DynamicLoader::RegisterCamera<EnvironmentCamera> r("environment");
