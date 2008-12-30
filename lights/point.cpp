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

// point.cpp*
#include "point.h"
#include "mc.h"
#include "reflection/bxdf.h"
#include "paramset.h"
#include "dynload.h"

using namespace lux;

class GonioBxDF : public BxDF {
public:
	GonioBxDF(const Normal &ns, const Vector &du, const Vector &dv, const SampleableSphericalFunction *func) :
		BxDF(BxDFType(BSDF_REFLECTION | BSDF_DIFFUSE)), x(du), y(dv), z(Vector(ns)), sf(func) {}
	void f(const TsPack *tspack, const Vector &wo, const Vector &wi, SWCSpectrum *const F) const {
		const Vector wL(wi.x * x.x + wi.y * y.x + wi.z * z.x,
				wi.x * x.y + wi.y * y.y + wi.z * z.y,
				wi.x * x.z + wi.y * y.z + wi.z * z.z);
		*F += SWCSpectrum(tspack, sf->f(wL));
	}
private:
	Vector x, y, z;
	const SampleableSphericalFunction *sf;
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
	delete func;
}
SWCSpectrum PointLight::Power(const TsPack *tspack, const Scene *) const {
	return Lbase->Evaluate(tspack, dummydg) * gain * 4.f * M_PI * func->Average_f();
}
SWCSpectrum PointLight::Sample_L(const TsPack *tspack, const Point &P, float u1, float u2,
		float u3, Vector *wo, float *pdf,
		VisibilityTester *visibility) const {
	*wo = Normalize(lightPos - P);
	*pdf = 1.f;
	visibility->SetSegment(P, lightPos, tspack->time);
	return L(tspack, Normalize(WorldToLight(-*wo))) / DistanceSquared(lightPos, P);
}
SWCSpectrum PointLight::Sample_L(const TsPack *tspack, const Scene *scene, float u1, float u2,
		float u3, float u4, Ray *ray, float *pdf) const {
	ray->o = lightPos;
	Vector w;
	RGBColor f = func->Sample_f(u1, u2, &w, pdf);
	ray->d = Normalize(LightToWorld(w));
	return Lbase->Evaluate(tspack, dummydg) * gain * SWCSpectrum(tspack, f);
}
float PointLight::Pdf(const Point &, const Vector &) const {
	return 0.;
}
SWCSpectrum PointLight::L(const TsPack *tspack, const Vector &w) const {
	return Lbase->Evaluate(tspack, dummydg) * gain * SWCSpectrum(tspack, func->f(w));
}
bool PointLight::Sample_L(const TsPack *tspack, const Scene *scene, float u1, float u2, float u3, BSDF **bsdf, float *pdf, SWCSpectrum *Le) const
{
	Normal ns = Normal(UniformSampleSphere(u1, u2));
	Vector dpdu, dpdv;
	CoordinateSystem(Vector(ns), &dpdu, &dpdv);
	DifferentialGeometry dg(lightPos, ns, dpdu, dpdv, Vector(0, 0, 0), Vector(0, 0, 0), 0, 0, NULL);
	*bsdf = BSDF_ALLOC(tspack, BSDF)(dg, ns);
	(*bsdf)->Add(BSDF_ALLOC(tspack, GonioBxDF)(WorldToLight(ns), WorldToLight(dpdu), WorldToLight(dpdv), func));
	*pdf = .25f * INV_PI;
	*Le = Lbase->Evaluate(tspack, dummydg) * gain;
	return true;
}
bool PointLight::Sample_L(const TsPack *tspack, const Scene *scene, const Point &p, const Normal &n,
	float u1, float u2, float u3, BSDF **bsdf, float *pdf, float *pdfDirect,
	VisibilityTester *visibility, SWCSpectrum *Le) const
{
	const Vector w(p - lightPos);
	Normal ns = Normal(w / sqrtf(w.LengthSquared()));
	*pdfDirect = 1.f;
	*pdf = .25f * INV_PI;
	Vector dpdu, dpdv;
	CoordinateSystem(Vector(ns), &dpdu, &dpdv);
	DifferentialGeometry dg(lightPos, ns, dpdu, dpdv, Vector(0, 0, 0), Vector(0, 0, 0), 0, 0, NULL);
	*bsdf = BSDF_ALLOC(tspack, BSDF)(dg, ns);
	(*bsdf)->Add(BSDF_ALLOC(tspack, GonioBxDF)(WorldToLight(ns), WorldToLight(dpdu), WorldToLight(dpdv), func));
	visibility->SetSegment(p, lightPos, tspack->time);
	*Le = Lbase->Evaluate(tspack, dummydg) * gain;
	return true;
}
SWCSpectrum PointLight::Le(const TsPack *tspack, const Scene *scene, const Ray &r,
	const Normal &n, BSDF **bsdf, float *pdf, float *pdfDirect) const
{
	return SWCSpectrum(0.f);
}
Light* PointLight::CreateLight(const Transform &light2world,
		const ParamSet &paramSet, const TextureParams &tp) {
	boost::shared_ptr<Texture<SWCSpectrum> > L = tp.GetSWCSpectrumTexture("L", RGBColor(1.f));
	float g = paramSet.FindOneFloat("gain", 1.f);

	SampleableSphericalFunction *ssf = 
		SampleableSphericalFunction::Create(paramSet, tp);

	Point P = paramSet.FindOnePoint("from", Point(0,0,0));
	Transform l2w = Translate(Vector(P.x, P.y, P.z)) * light2world;
	return new PointLight(l2w, L, g, ssf);
}

static DynamicLoader::RegisterLight<PointLight> r("point");
static DynamicLoader::RegisterLight<PointLight> rb("goniometric"); // Backwards compability for removed 'GonioPhotometricLight'

