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

class  UniformBSDF : public BSDF  {
public:
	// UniformBSDF Public Methods
	UniformBSDF(const DifferentialGeometry &dgs, const Normal &ngeom,
		const Volume *exterior, const Volume *interior) :
		BSDF(dgs, ngeom, exterior, interior) { }
	virtual inline u_int NumComponents() const { return 1; }
	virtual inline u_int NumComponents(BxDFType flags) const {
		return (flags & BSDF_DIFFUSE) == BSDF_DIFFUSE ? 1U : 0U;
	}
	virtual bool Sample_f(const TsPack *tspack, const Vector &woW, Vector *wiW,
		float u1, float u2, float u3, SWCSpectrum *const f_, float *pdf,
		BxDFType flags = BSDF_ALL, BxDFType *sampledType = NULL,
		float *pdfBack = NULL, bool reverse = false) const {
		if (reverse || NumComponents(flags) == 0)
			return false;
		*wiW = UniformSampleSphere(u1, u2);
		const float cosi = AbsDot(*wiW, nn);
		if (sampledType)
			*sampledType = BSDF_DIFFUSE;
		*pdf = .25f * INV_PI;
		if (pdfBack)
			*pdfBack = 0.f;
		*f_ = SWCSpectrum(.25f * INV_PI / cosi);
		return true;
	}
	virtual float Pdf(const TsPack *tspack, const Vector &woW,
		const Vector &wiW, BxDFType flags = BSDF_ALL) const {
		if (NumComponents(flags) == 1)
			return .25f * INV_PI;
		return 0.f;
	}
	virtual SWCSpectrum f(const TsPack *tspack, const Vector &woW,
		const Vector &wiW, BxDFType flags = BSDF_ALL) const {
		if (NumComponents(flags) == 1)
			return SWCSpectrum(.25f * INV_PI / AbsDot(wiW, nn));
		return SWCSpectrum(0.f);
	}
	virtual SWCSpectrum rho(const TsPack *tspack,
		BxDFType flags = BSDF_ALL) const { return SWCSpectrum(1.f); }
	virtual SWCSpectrum rho(const TsPack *tspack, const Vector &woW,
		BxDFType flags = BSDF_ALL) const { return SWCSpectrum(1.f); }

protected:
	// UniformBSDF Private Methods
	virtual ~UniformBSDF() { }
};

class  GonioBSDF : public BSDF  {
public:
	// GonioBSDF Public Methods
	GonioBSDF(const DifferentialGeometry &dgs, const Normal &ngeom,
		const Volume *exterior, const Volume *interior,
		const SampleableSphericalFunction *func) :
		BSDF(dgs, ngeom, exterior, interior), sf(func) { }
	virtual inline u_int NumComponents() const { return 1; }
	virtual inline u_int NumComponents(BxDFType flags) const {
		return (flags & BSDF_DIFFUSE) == BSDF_DIFFUSE ? 1U : 0U;
	}
	virtual bool Sample_f(const TsPack *tspack, const Vector &woW, Vector *wiW,
		float u1, float u2, float u3, SWCSpectrum *const f_, float *pdf,
		BxDFType flags = BSDF_ALL, BxDFType *sampledType = NULL,
		float *pdfBack = NULL, bool reverse = false) const {
		if (reverse || NumComponents(flags) == 0)
			return false;
		*f_ = sf->Sample_f(tspack, u1, u2, wiW, pdf);
		*f_ /= sf->Average_f() * fabsf(wiW->z);
		*wiW = Normalize(LocalToWorld(*wiW));
		if (sampledType)
			*sampledType = BSDF_DIFFUSE;
		if (pdfBack)
			*pdfBack = 0.f;
		return true;
	}
	virtual float Pdf(const TsPack *tspack, const Vector &woW,
		const Vector &wiW, BxDFType flags = BSDF_ALL) const {
		if (NumComponents(flags) == 1)
			return sf->Pdf(WorldToLocal(wiW));
		return 0.f;
	}
	virtual SWCSpectrum f(const TsPack *tspack, const Vector &woW,
		const Vector &wiW, BxDFType flags = BSDF_ALL) const {
		if (NumComponents(flags) == 1)
			return sf->f(tspack, WorldToLocal(wiW)) /
				(sf->Average_f() * AbsDot(wiW, nn));
		return SWCSpectrum(0.f);
	}
	virtual SWCSpectrum rho(const TsPack *tspack,
		BxDFType flags = BSDF_ALL) const { return SWCSpectrum(1.f); }
	virtual SWCSpectrum rho(const TsPack *tspack, const Vector &woW,
		BxDFType flags = BSDF_ALL) const { return SWCSpectrum(1.f); }

protected:
	// GonioBSDF Private Methods
	virtual ~GonioBSDF() { }
	//GonioBSDF Private Data
	const SampleableSphericalFunction *sf;
};

// PointLight Method Definitions
PointLight::PointLight(
		const Transform &light2world,
		const boost::shared_ptr< Texture<SWCSpectrum> > &L,
		float g,
		SampleableSphericalFunction *ssf)
	: Light(light2world), Lbase(L) {
	lightPos = LightToWorld(Point(0,0,0));
	Lbase->SetIlluminant();
	gain = g;
	func = ssf;
}
PointLight::~PointLight() {
	if(func)
		delete func;
}
float PointLight::Power(const Scene *) const {
	return Lbase->Y() * gain * 4.f * M_PI;
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
		SWCSpectrum f(func->Sample_f(tspack, u1, u2, &w, pdf));
		ray->d = LightToWorld(w);
		return Lbase->Evaluate(tspack, dummydg) * gain * f;
	} else {
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
	return Lbase->Evaluate(tspack, dummydg) * gain *
		(func ? func->f(tspack, w) : 1.f);
}
bool PointLight::Sample_L(const TsPack *tspack, const Scene *scene, float u1, float u2, float u3, BSDF **bsdf, float *pdf, SWCSpectrum *Le) const
{
	*pdf = 1.f;
	const Normal ns(0, 0, 1);
	DifferentialGeometry dg(lightPos, Normalize(LightToWorld(ns)),
		Normalize(LightToWorld(Vector(1, 0, 0))),
		Normalize(LightToWorld(Vector(0, 1, 0))),
		Normal(0, 0, 0), Normal(0, 0, 0), 0, 0, NULL);
	if(func)
		*bsdf = ARENA_ALLOC(tspack->arena, GonioBSDF)(dg, ns,
			NULL, NULL, func);
	else
		*bsdf = ARENA_ALLOC(tspack->arena, UniformBSDF)(dg, ns,
			NULL, NULL);
	*Le = Lbase->Evaluate(tspack, dg) * (gain * 4.f * M_PI);
	return true;
}
bool PointLight::Sample_L(const TsPack *tspack, const Scene *scene, const Point &p, const Normal &n,
	float u1, float u2, float u3, BSDF **bsdf, float *pdf, float *pdfDirect,
	VisibilityTester *visibility, SWCSpectrum *Le) const
{
	const Normal ns(0, 0, 1);
	DifferentialGeometry dg(lightPos, Normalize(LightToWorld(ns)),
		Normalize(LightToWorld(Vector(1, 0, 0))),
		Normalize(LightToWorld(Vector(0, 1, 0))),
		Normal(0, 0, 0), Normal(0, 0, 0), 0, 0, NULL);
	*pdfDirect = 1.f;
	*pdf = 1.f;
	if (func)
		*bsdf = ARENA_ALLOC(tspack->arena, GonioBSDF)(dg, ns,
			NULL, NULL, func);
	else
		*bsdf = ARENA_ALLOC(tspack->arena, UniformBSDF)(dg, ns,
			NULL, NULL);
	visibility->SetSegment(p, lightPos, tspack->time);
	*Le = Lbase->Evaluate(tspack, dg) * (gain * 4.f * M_PI);
	return true;
}
SWCSpectrum PointLight::Le(const TsPack *tspack, const Scene *scene, const Ray &r,
	const Normal &n, BSDF **bsdf, float *pdf, float *pdfDirect) const
{
	return SWCSpectrum(0.f);
}
Light* PointLight::CreateLight(const Transform &light2world,
		const ParamSet &paramSet) {
	boost::shared_ptr<Texture<SWCSpectrum> > L(paramSet.GetSWCSpectrumTexture("L", RGBColor(1.f)));
	float g = paramSet.FindOneFloat("gain", 1.f);

	boost::shared_ptr<const SphericalFunction> sf(CreateSphericalFunction(paramSet));
	SampleableSphericalFunction *ssf = NULL;
	if(sf)
		ssf = new SampleableSphericalFunction(sf);

	Point P = paramSet.FindOnePoint("from", Point(0,0,0));
	Transform l2w = Translate(Vector(P.x, P.y, P.z)) * light2world;

	PointLight *l = new PointLight(l2w, L, g, ssf);
	l->hints.InitParam(paramSet);
	return l;
}

static DynamicLoader::RegisterLight<PointLight> r("point");
static DynamicLoader::RegisterLight<PointLight> rb("goniometric"); // Backwards compability for removed 'GonioPhotometricLight'

