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

// point.cpp*
#include "pointlight.h"
#include "mc.h"
#include "reflection/bxdf.h"
#include "reflection/bxdf/lambertian.h"
#include "paramset.h"
#include "dynload.h"

using namespace lux;

class GonioBxDF : public BxDF {
public:
	GonioBxDF(const Transform &WToL, const SampleableSphericalFunction *func) :
		BxDF(BxDFType(BSDF_DIFFUSE)), WorldToLight(WToL), sf(func) { }
	virtual ~GonioBxDF() { }
	virtual bool Sample_f(const TsPack *tspack, const Vector &wo,
		Vector *wi, float u1, float u2, SWCSpectrum *const f_,
		float *pdf, float *pdfBack = NULL, bool reverse = false) const {
		Vector w;
		*f_ += SWCSpectrum(tspack, sf->Sample_f(u1, u2, &w, pdf));
		*wi = Normalize(WorldToLight.GetInverse()(w));
		*f_ /= fabsf(wi->z);
		*pdfBack = 0.f;
		return true;
	}
	virtual void f(const TsPack *tspack, const Vector &wo, const Vector &wi, SWCSpectrum *const F) const {
		// Transform to light coordinate system
		const Vector wL(Normalize(WorldToLight(wi)));
		*F += SWCSpectrum(tspack, sf->f(wL)) / fabsf(wi.z);
	}
	virtual float Pdf(const TsPack *tspack, const Vector &wi,
		const Vector &wo) const {
		const Vector wL(Normalize(WorldToLight(wi)));
		return sf->Pdf(wL);
	}
private:
	const Transform &WorldToLight;
	const SampleableSphericalFunction *sf;
};

class UniformBxDF : public BxDF {
public:
	UniformBxDF() : BxDF(BSDF_DIFFUSE) { }
	virtual ~UniformBxDF() { }
	virtual bool Sample_f(const TsPack *tspack, const Vector &wo,
		Vector *wi, float u1, float u2, SWCSpectrum *const f_,
		float *pdf, float *pdfBack = NULL, bool reverse = false) const {
		*wi = UniformSampleSphere(u1, u2);
		*pdf = UniformSpherePdf();
		*f_ += 1.f / fabsf(wi->z);
		*pdfBack = 0.f;
		return true;
	}
	virtual void f(const TsPack *tspack, const Vector &wo, const Vector &wi, SWCSpectrum *const F) const {
		// Transform to light coordinate system
		*F += 1.f / fabsf(wi.z);
	}
	virtual float Pdf(const TsPack *tspack, const Vector &wi,
		const Vector &wo) const {
		return UniformSpherePdf();
	}
};

// PointLight Method Definitions
PointLight::PointLight(
		const Transform &light2world,
		const boost::shared_ptr< Texture<SWCSpectrum> > L,
		float g,
		SampleableSphericalFunction *ssf)
	: Light(light2world) {
	lightPos = LightToWorld(Point(0,0,0));
	Lbase = L;
	Lbase->SetIlluminant();
	gain = g;
	func = ssf;
}
PointLight::~PointLight() {
	if(func)
		delete func;
}
float PointLight::Power(const Scene *) const {
	return Lbase->Y() * gain * 4.f * M_PI * (func ? func->Average_f() : 1.f);
}
SWCSpectrum PointLight::Sample_L(const TsPack *tspack, const Point &P, float u1, float u2,
		float u3, Vector *wo, float *pdf,
		VisibilityTester *visibility) const {
	*wo = Normalize(lightPos - P);
	*pdf = 1.f;
	visibility->SetSegment(P, lightPos, tspack->time);
	return L(tspack, WorldToLight(-*wo)) / DistanceSquared(lightPos, P);
}
SWCSpectrum PointLight::Sample_L(const TsPack *tspack, const Scene *scene, float u1, float u2,
		float u3, float u4, Ray *ray, float *pdf) const {
	ray->o = lightPos;
	if(func) {
		Vector w;
		RGBColor f = func->Sample_f(u1, u2, &w, pdf);
		ray->d = LightToWorld(w);
		return Lbase->Evaluate(tspack, dummydg) * gain * SWCSpectrum(tspack, f);
	}
	else {
		ray->d = UniformSampleSphere(u1, u2);
		*pdf = UniformSpherePdf();
		return Lbase->Evaluate(tspack, dummydg) * gain;
	}
}
float PointLight::Pdf(const TsPack *, const Point &, const Vector &) const {
	return 0.;
}
float PointLight::Pdf(const TsPack *tspack, const Point &p, const Normal &n,
	const Point &po, const Normal &ns) const
{
	return 1.f;
}
SWCSpectrum PointLight::L(const TsPack *tspack, const Vector &w) const {
	return Lbase->Evaluate(tspack, dummydg) * SWCSpectrum(tspack, gain * (func ? func->f(w) : 1.f));
}
bool PointLight::Sample_L(const TsPack *tspack, const Scene *scene, float u1, float u2, float u3, BSDF **bsdf, float *pdf, SWCSpectrum *Le) const
{
	*pdf = 1.f;
	const Normal ns(0, 0, 1);
	DifferentialGeometry dg(lightPos, ns, Vector(1, 0, 0), Vector(0, 1, 0),
		Normal(0, 0, 0), Normal(0, 0, 0), 0, 0, NULL);
	if(func)
		*bsdf = ARENA_ALLOC(tspack->arena, SingleBSDF)(dg, ns,
			ARENA_ALLOC(tspack->arena, GonioBxDF)(WorldToLight, func));
	else
		*bsdf = ARENA_ALLOC(tspack->arena, SingleBSDF)(dg, ns,
			ARENA_ALLOC(tspack->arena, UniformBxDF)());
	*Le = Lbase->Evaluate(tspack, dg) * gain;
	return true;
}
bool PointLight::Sample_L(const TsPack *tspack, const Scene *scene, const Point &p, const Normal &n,
	float u1, float u2, float u3, BSDF **bsdf, float *pdf, float *pdfDirect,
	VisibilityTester *visibility, SWCSpectrum *Le) const
{
	const Normal ns(0, 0, 1);
	Vector dpdu, dpdv;
	DifferentialGeometry dg(lightPos, ns, Vector(1, 0, 0), Vector(0, 1, 0),
		Normal(0, 0, 0), Normal(0, 0, 0), 0, 0, NULL);
	*pdfDirect = 1.f;
	*pdf = 1.f;
	if (func)
		*bsdf = ARENA_ALLOC(tspack->arena, SingleBSDF)(dg, ns,
			ARENA_ALLOC(tspack->arena, GonioBxDF)(WorldToLight, func));
	else
		*bsdf = ARENA_ALLOC(tspack->arena, SingleBSDF)(dg, ns,
			ARENA_ALLOC(tspack->arena, UniformBxDF)());
	visibility->SetSegment(p, lightPos, tspack->time);
	*Le = Lbase->Evaluate(tspack, dg) * gain;
	return true;
}
SWCSpectrum PointLight::Le(const TsPack *tspack, const Scene *scene, const Ray &r,
	const Normal &n, BSDF **bsdf, float *pdf, float *pdfDirect) const
{
	return SWCSpectrum(0.f);
}
Light* PointLight::CreateLight(const Transform &light2world,
		const ParamSet &paramSet) {
	boost::shared_ptr<Texture<SWCSpectrum> > L = paramSet.GetSWCSpectrumTexture("L", RGBColor(1.f));
	float g = paramSet.FindOneFloat("gain", 1.f);

	const SphericalFunction *sf = CreateSphericalFunction(paramSet);
	SampleableSphericalFunction *ssf = NULL;
	if(sf)
		ssf = new SampleableSphericalFunction(boost::shared_ptr<const SphericalFunction>(sf));

	Point P = paramSet.FindOnePoint("from", Point(0,0,0));
	Transform l2w = Translate(Vector(P.x, P.y, P.z)) * light2world;

	PointLight *l = new PointLight(l2w, L, g, ssf);
	l->hints.InitParam(paramSet);
	return l;
}

static DynamicLoader::RegisterLight<PointLight> r("point");
static DynamicLoader::RegisterLight<PointLight> rb("goniometric"); // Backwards compability for removed 'GonioPhotometricLight'

