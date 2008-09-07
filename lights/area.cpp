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

// area.cpp*
#include "light.h"
#include "mc.h"
#include "primitive.h"
#include "paramset.h"
#include "rgbillum.h"
#include "blackbody.h"
#include "reflection/bxdf.h"
#include "reflection/bxdf/lambertian.h"
#include "dynload.h"

using namespace lux;

// AreaLight Method Definitions
AreaLight::AreaLight(const Transform &light2world,								// TODO - radiance - add portal implementation
		const RGBColor &le, float g, int ns,
		const boost::shared_ptr<Shape> &s)
	: Light(light2world, ns) {
	// Create SPD
	LSPD = new RGBIllumSPD(le);
	LSPD->Scale(g);

	if (s->CanIntersect())
		shape = s;
	else {
		// Create _ShapeSet_ for _Shape_
		boost::shared_ptr<Shape> shapeSet = s;
		vector<boost::shared_ptr<Shape> > todo, done;
		todo.push_back(shapeSet);
		while (todo.size()) {
			boost::shared_ptr<Shape> sh = todo.back();
			todo.pop_back();
			if (sh->CanIntersect())
				done.push_back(sh);
			else
				sh->Refine(todo);
		}
		if (done.size() == 1) shape = done[0];
		else {
			boost::shared_ptr<Shape> o (new ShapeSet(done, s->ObjectToWorld, s->reverseOrientation));
			shape = o;
		}
	}
	area = shape->Area();
}
SWCSpectrum AreaLight::Sample_L(const TsPack *tspack, const Point &p,
		const Normal &n, float u1, float u2, float u3,
		Vector *wi, float *pdf,
		VisibilityTester *visibility) const {
	Normal ns;
	Point ps = shape->Sample(p, u1, u2, tspack->rng->floatValue(), &ns); // TODO - REFACT - add passed value from sample
	*wi = Normalize(ps - p);
	*pdf = shape->Pdf(p, *wi);
	visibility->SetSegment(p, ps);
	return L(tspack, ps, ns, -*wi);
}
float AreaLight::Pdf(const Point &p, const Normal &N,
		const Vector &wi) const {
	return shape->Pdf(p, wi);
}
SWCSpectrum AreaLight::Sample_L(const TsPack *tspack, const Point &P,
		float u1, float u2, float u3, Vector *wo, float *pdf,
		VisibilityTester *visibility) const {
	Normal Ns;
	Point Ps = shape->Sample(P, u1, u2, tspack->rng->floatValue(), &Ns); // TODO - REFACT - add passed value from sample
	*wo = Normalize(Ps - P);
	*pdf = shape->Pdf(P, *wo);
	visibility->SetSegment(P, Ps);
	return L(tspack, Ps, Ns, -*wo);
}
SWCSpectrum AreaLight::Sample_L(const TsPack *tspack, const Scene *scene, float u1,
		float u2, float u3, float u4,
		Ray *ray, float *pdf) const {
	Normal ns;
	ray->o = shape->Sample(u1, u2, tspack->rng->floatValue(), &ns); // TODO - REFACT - add passed value from sample
	ray->d = UniformSampleSphere(u3, u4);
	if (Dot(ray->d, ns) < 0.) ray->d *= -1;
	*pdf = shape->Pdf(ray->o) * INV_TWOPI;
	return L(tspack, ray->o, ns, ray->d);
}
float AreaLight::Pdf(const Point &P, const Vector &w) const {
	return shape->Pdf(P, w);
}
SWCSpectrum AreaLight::Sample_L(const TsPack *tspack, const Scene *scene, float u1, float u2, BSDF **bsdf, float *pdf) const
{
	Normal ns;
	Point ps = shape->Sample(u1, u2, tspack->rng->floatValue(), &ns); // TODO - REFACT - add passed value from sample
	Vector dpdu, dpdv;
	CoordinateSystem(Vector(ns), &dpdu, &dpdv);
	DifferentialGeometry dg(ps, ns, dpdu, dpdv, Vector(0, 0, 0), Vector(0, 0, 0), 0, 0, NULL);
	*bsdf = BSDF_ALLOC(BSDF)(dg, ns);
	(*bsdf)->Add(BSDF_ALLOC(Lambertian)(SWCSpectrum(1.f)));
	*pdf = shape->Pdf(ps);
	return L(tspack, ps, ns, Vector(ns)) /** M_PI*/;
}
SWCSpectrum AreaLight::Sample_L(const TsPack *tspack, const Scene *scene, const Point &p, const Normal &n,
	float u1, float u2, float u3, BSDF **bsdf, float *pdf, float *pdfDirect,
	VisibilityTester *visibility) const
{
	Normal ns;
	Point ps = shape->Sample(p, u1, u2, tspack->rng->floatValue(), &ns); // TODO - REFACT - add passed value from sample
	Vector wo(Normalize(ps - p));
	*pdf = shape->Pdf(ps);
	*pdfDirect = shape->Pdf(p, wo) * AbsDot(wo, ns) / DistanceSquared(ps, p);
	Vector dpdu, dpdv;
	CoordinateSystem(Vector(ns), &dpdu, &dpdv);
	DifferentialGeometry dg(ps, ns, dpdu, dpdv, Vector(0, 0, 0), Vector(0, 0, 0), 0, 0, NULL);
	*bsdf = BSDF_ALLOC(BSDF)(dg, ns);
	(*bsdf)->Add(BSDF_ALLOC(Lambertian)(SWCSpectrum(1.f)));
	visibility->SetSegment(p, ps);
	return L(tspack, ps, ns, -wo) /** M_PI*/;
}
float AreaLight::Pdf(const Scene *scene, const Point &p) const
{
	return shape->Pdf(p);
}
SWCSpectrum AreaLight::L(const TsPack *tspack, const Ray &ray, const DifferentialGeometry &dg, const Normal &n, BSDF **bsdf, float *pdf, float *pdfDirect) const
{
	*bsdf = BSDF_ALLOC(BSDF)(dg, dg.nn);
	(*bsdf)->Add(BSDF_ALLOC(Lambertian)(SWCSpectrum(1.f)));
	*pdf = shape->Pdf(dg.p);
	*pdfDirect = shape->Pdf(ray.o, ray.d) * AbsDot(ray.d, n) / DistanceSquared(dg.p, ray.o);
	return L(tspack, dg.p, dg.nn, -ray.d) /** M_PI*/;
}

void AreaLight::SamplePosition(float u1, float u2, float u3, Point *p, Normal *n, float *pdf) const
{
	*p = shape->Sample(u1, u2, u3, n);
	*pdf = shape->Pdf(*p);
}
void AreaLight::SampleDirection(float u1, float u2,const Normal &nn, Vector *wo, float *pdf) const
{
	//*wo = UniformSampleSphere(u1, u2);
	//if (Dot(*wo, nn) < 0.) *wo *= -1;
	//*pdf = INV_TWOPI;

	Vector v;
	v = CosineSampleHemisphere(u1, u2);
	TransformAccordingNormal(nn, v, wo);
	*pdf = INV_PI * AbsDot(*wo,nn);
}
float AreaLight::EvalPositionPdf(const Point p, const Normal &n, const Vector &w) const
{
	return Dot(n, w) > 0 ? shape->Pdf(p) : 0.;
}
float AreaLight::EvalDirectionPdf(const Point p, const Normal &n, const Vector &w) const
{
	return Dot(n, w) > 0 ? INV_PI * AbsDot(w,n) : 0.;
}
SWCSpectrum AreaLight::Eval(const TsPack *tspack, const Normal &n, const Vector &w) const
{
	return Dot(n, w) > 0 ? SWCSpectrum(tspack, LSPD) : 0.;
}

AreaLight* AreaLight::CreateAreaLight(const Transform &light2world, const ParamSet &paramSet,
		const boost::shared_ptr<Shape> &shape) {
	RGBColor L = paramSet.FindOneRGBColor("L", RGBColor(1.0));
	float g = paramSet.FindOneFloat("gain", 1.f);
	int nSamples = paramSet.FindOneInt("nsamples", 1);
	return new AreaLight(light2world, L, g, nSamples, shape);
}

static DynamicLoader::RegisterAreaLight<AreaLight> r("area");

