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

// area.cpp*
#include "light.h"
#include "randomgen.h" //FIXME
#include "mc.h"
#include "shape.h"
#include "context.h"
#include "paramset.h"
#include "rgbillum.h"
#include "blackbodyspd.h"
#include "reflection/bxdf.h"
#include "reflection/bxdf/lambertian.h"
#include "spectrum.h"
#include "texture.h"
#include "sphericalfunction.h"
#include "dynload.h"

using namespace lux;

class  UniformAreaBSDF : public BSDF  {
public:
	// UniformAreaBSDF Public Methods
	UniformAreaBSDF(const DifferentialGeometry &dgs, const Normal &ngeom,
		const Volume *exterior, const Volume *interior) :
		BSDF(dgs, ngeom, exterior, interior) { }
	virtual inline u_int NumComponents() const { return 1; }
	virtual inline u_int NumComponents(BxDFType flags) const {
		return (flags & (BSDF_REFLECTION | BSDF_DIFFUSE)) ==
			(BSDF_REFLECTION | BSDF_DIFFUSE) ? 1U : 0U;
	}
	virtual bool Sample_f(const TsPack *tspack, const Vector &woW, Vector *wiW,
		float u1, float u2, float u3, SWCSpectrum *const f_, float *pdf,
		BxDFType flags = BSDF_ALL, BxDFType *sampledType = NULL,
		float *pdfBack = NULL, bool reverse = false) const {
		if (reverse || NumComponents(flags) == 0)
			return false;
		*wiW = CosineSampleHemisphere(u1, u2);
		const float cosi = wiW->z;
		*wiW = LocalToWorld(*wiW);
		if (!(Dot(*wiW, ng) > 0.f))
			return false;
		if (sampledType)
			*sampledType = BxDFType(BSDF_REFLECTION | BSDF_DIFFUSE);
		*pdf = cosi * INV_PI;
		if (pdfBack)
			*pdfBack = 0.f;
		*f_ = SWCSpectrum(INV_PI);
		return true;
	}
	virtual float Pdf(const TsPack *tspack, const Vector &woW,
		const Vector &wiW, BxDFType flags = BSDF_ALL) const {
		if (NumComponents(flags) == 1 &&
			Dot(wiW, ng) > 0.f) {
			const float cosi = Dot(wiW, nn);
			if (cosi > 0.f)
				return cosi * INV_PI;
		}
		return 0.f;
	}
	virtual SWCSpectrum f(const TsPack *tspack, const Vector &woW,
		const Vector &wiW, BxDFType flags = BSDF_ALL) const {
		if (NumComponents(flags) == 1 && Dot(wiW, ng) > 0.f)
			return SWCSpectrum(INV_PI);
		return SWCSpectrum(0.f);
	}
	virtual SWCSpectrum rho(const TsPack *tspack,
		BxDFType flags = BSDF_ALL) const { return SWCSpectrum(1.f); }
	virtual SWCSpectrum rho(const TsPack *tspack, const Vector &woW,
		BxDFType flags = BSDF_ALL) const { return SWCSpectrum(1.f); }

protected:
	// UniformAreaBSDF Private Methods
	virtual ~UniformAreaBSDF() { }
};

class  GonioAreaBSDF : public BSDF  {
public:
	// GonioBSDF Public Methods
	GonioAreaBSDF(const DifferentialGeometry &dgs, const Normal &ngeom,
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
	// GonioAreaBSDF Private Methods
	virtual ~GonioAreaBSDF() { }
	//GonioAreaBSDF Private Data
	const SampleableSphericalFunction *sf;
};

// AreaLight Method Definitions
AreaLight::AreaLight(const Transform &light2world,
	boost::shared_ptr<Texture<SWCSpectrum> > &le, float g, float pow,
	float e, SampleableSphericalFunction *ssf, u_int ns,
	const boost::shared_ptr<Primitive> &p)
	: Light(light2world, ns), Le(le), gain(g), power(pow), efficacy(e),
	func(ssf)
{
	if (p->CanIntersect() && p->CanSample()) {
		// Create a temporary to increase shared count
		// The assignment is just a swap
		boost::shared_ptr<Primitive> pr(p);
		prim = pr;
	} else {
		// Create _PrimitiveSet_ for _Primitive_
		vector<boost::shared_ptr<Primitive> > refinedPrims;
		PrimitiveRefinementHints refineHints(true);
		p->Refine(refinedPrims, refineHints, p);
		if (refinedPrims.size() == 1)
			prim = refinedPrims[0];
		else
			prim = boost::shared_ptr<Primitive>(new PrimitiveSet(refinedPrims));
	}
	area = prim->Area();
	Le->SetIlluminant(); // Illuminant must be set before calling Le->Y()
	const float gainFactor = power * efficacy /
		(area * M_PI * Le->Y());
	if (gainFactor > 0.f && !isinf(gainFactor))
		gain *= gainFactor;
}

AreaLight::~AreaLight()
{
	delete func;
}

SWCSpectrum AreaLight::L(const TsPack *tspack, const DifferentialGeometry &dg,
	const Vector& w) const
{
	if (Dot(dg.nn, w) > 0.f) {
		SWCSpectrum Ll(Le->Evaluate(tspack, dg) * gain);
		if (func) {
			// Transform to the local coordinate system around the point
			const Vector wLocal(Dot(dg.dpdu, w), Dot(dg.dpdv, w),
				Dot(dg.nn, w));
			Ll *= func->f(tspack, wLocal);
		}
		return Ll;
	}
	return SWCSpectrum(0.f);
}

float AreaLight::Power(const Scene *scene) const
{
	return gain * area * M_PI * Le->Y();
}

SWCSpectrum AreaLight::Sample_L(const TsPack *tspack, const Point &p,
	const Normal &n, float u1, float u2, float u3, Vector *wi, float *pdf,
	VisibilityTester *visibility) const
{
	DifferentialGeometry dg;
	dg.time = tspack->time;
	prim->Sample(tspack, p, u1, u2, u3, &dg);
	*wi = Normalize(dg.p - p);
	*pdf = prim->Pdf(p, *wi);
	visibility->SetSegment(p, dg.p, tspack->time);
	return L(tspack, dg, -*wi);
}
float AreaLight::Pdf(const TsPack *tspack, const Point &p, const Normal &N,
		const Vector &wi) const {
	return prim->Pdf(p, wi);
}
float AreaLight::Pdf(const TsPack *tspack, const Point &p, const Normal &N,
	const Point &po, const Normal &ns) const
{
	return prim->Pdf(p, po);
}
SWCSpectrum AreaLight::Sample_L(const TsPack *tspack, const Point &P,
		float u1, float u2, float u3, Vector *wo, float *pdf,
		VisibilityTester *visibility) const {
	DifferentialGeometry dg;
	dg.time = tspack->time;
	prim->Sample(tspack, P, u1, u2, u3, &dg);
	*wo = Normalize(dg.p - P);
	*pdf = prim->Pdf(P, *wo);
	visibility->SetSegment(P, dg.p, tspack->time);
	return L(tspack, dg, -*wo);
}
SWCSpectrum AreaLight::Sample_L(const TsPack *tspack, const Scene *scene, float u1,
		float u2, float u3, float u4,
		Ray *ray, float *pdf) const {
	DifferentialGeometry dg;
	dg.time = tspack->time;
	prim->Sample(u1, u2, tspack->rng->floatValue(), &dg); // TODO - REFACT - add passed value from sample
	ray->o = dg.p;
	ray->d = CosineSampleHemisphere(u3, u4);
	const float coso = ray->d.z;
	ray->d = ray->d.x * Normalize(dg.dpdu) + ray->d.y * Normalize(dg.dpdv) +
		ray->d.z * Vector(dg.nn);
	*pdf = prim->Pdf(ray->o) * coso * INV_PI;
	return L(tspack, dg, ray->d) * coso;
}
float AreaLight::Pdf(const TsPack *tspack, const Point &P, const Vector &w) const {
	return prim->Pdf(P, w);
}
bool AreaLight::Sample_L(const TsPack *tspack, const Scene *scene, float u1, float u2, float u3, BSDF **bsdf, float *pdf, SWCSpectrum *Le) const
{
	DifferentialGeometry dg;
	dg.time = tspack->time;
	prim->Sample(u1, u2, u3, &dg);
	if(func)
		*bsdf = ARENA_ALLOC(tspack->arena, GonioAreaBSDF)(dg, dg.nn,
			prim->GetExterior(), prim->GetInterior(), func);
	else
		*bsdf = ARENA_ALLOC(tspack->arena, UniformAreaBSDF)(dg, dg.nn,
			prim->GetExterior(), prim->GetInterior());
	*pdf = prim->Pdf(dg.p);
	if (*pdf > 0.f) {
		*Le = this->Le->Evaluate(tspack, dg) * (gain * M_PI);
		return true;
	}
	*Le = 0.f;
	return false;
}
bool AreaLight::Sample_L(const TsPack *tspack, const Scene *scene, const Point &p, const Normal &n,
	float u1, float u2, float u3, BSDF **bsdf, float *pdf, float *pdfDirect,
	VisibilityTester *visibility, SWCSpectrum *Le) const
{
	DifferentialGeometry dg;
	dg.time = tspack->time;
	prim->Sample(tspack, p, u1, u2, u3, &dg);
	Vector wo(Normalize(dg.p - p));
	*pdf = prim->Pdf(dg.p);
	*pdfDirect = prim->Pdf(p, dg.p);
	if (*pdfDirect > 0.f) {
		if(func)
			*bsdf = ARENA_ALLOC(tspack->arena, GonioAreaBSDF)(dg, dg.nn,
				prim->GetExterior(), prim->GetInterior(), func);
		else
			*bsdf = ARENA_ALLOC(tspack->arena, UniformAreaBSDF)(dg, dg.nn,
				prim->GetExterior(), prim->GetInterior());
		visibility->SetSegment(p, dg.p, tspack->time);
		*Le = this->Le->Evaluate(tspack, dg) * (gain * M_PI);
		return true;
	}
	*Le = 0.f;
	return false;
}
SWCSpectrum AreaLight::L(const TsPack *tspack, const Ray &ray, const DifferentialGeometry &dg, const Normal &n, BSDF **bsdf, float *pdf, float *pdfDirect) const
{
	if(func) {
		*bsdf = ARENA_ALLOC(tspack->arena, GonioAreaBSDF)(dg, dg.nn,
			prim->GetExterior(), prim->GetInterior(), func);
	} else {
		if (!(Dot(dg.nn, ray.d) < 0.f)) {
			*pdfDirect = *pdf = 0.f;
			*bsdf = NULL;
			return SWCSpectrum(tspack, 0.f);
		}
		*bsdf = ARENA_ALLOC(tspack->arena, UniformAreaBSDF)(dg, dg.nn,
			prim->GetExterior(), prim->GetInterior());
	}
	*pdf = prim->Pdf(dg.p);
	*pdfDirect = prim->Pdf(ray.o, dg.p);
	return Le->Evaluate(tspack, dg) * (gain * M_PI) * (*bsdf)->f(tspack, Vector(dg.nn), -ray.d);
}

class HemiSphereSphericalFunction : public SphericalFunction {
public:
	HemiSphereSphericalFunction(const boost::shared_ptr<const SphericalFunction> &aSF) : sf(aSF) {}
	SWCSpectrum f(const TsPack *tspack, float phi, float theta) const {
		return theta > 0.f ? sf->f(tspack, phi, theta) : 0.f;
	}
private:
	const boost::shared_ptr<const SphericalFunction> sf;
};

AreaLight* AreaLight::CreateAreaLight(const Transform &light2world,
	const ParamSet &paramSet, const boost::shared_ptr<Primitive> &prim)
{
	boost::shared_ptr<Texture<SWCSpectrum> > L(
		paramSet.GetSWCSpectrumTexture("L", RGBColor(1.f)));

	float g = paramSet.FindOneFloat("gain", 1.f);
	float p = paramSet.FindOneFloat("power", 100.f);		// Power/Lm in Watts
	float e = paramSet.FindOneFloat("efficacy", 17.f);		// Efficacy Lm per Watt

	boost::shared_ptr<const SphericalFunction> sf(CreateSphericalFunction(paramSet));
	SampleableSphericalFunction *ssf = NULL;
	if (sf) {
		boost::shared_ptr<const SphericalFunction> hf(new HemiSphereSphericalFunction(sf));
		ssf = new SampleableSphericalFunction(hf);
	}

	int nSamples = paramSet.FindOneInt("nsamples", 1);

	AreaLight *l = new AreaLight(light2world, L, g, p, e, ssf, nSamples, prim);
	l->hints.InitParam(paramSet);
	return l;
}

static DynamicLoader::RegisterAreaLight<AreaLight> r("area");

