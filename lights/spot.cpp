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

// spot.cpp*
#include "spot.h"
#include "mc.h"
#include "paramset.h"
#include "reflection/bxdf.h"
#include "dynload.h"

using namespace lux;

static float LocalFalloff(const Vector &w, float cosTotalWidth, float cosFalloffStart)
{
	if (CosTheta(w) < cosTotalWidth)
		return 0.f;
 	if (CosTheta(w) > cosFalloffStart)
		return 1.f;
	// Compute falloff inside spotlight cone
	const float delta = (CosTheta(w) - cosTotalWidth) /
		(cosFalloffStart - cosTotalWidth);
	return powf(delta, 4);
}
class SpotBxDF : public BxDF
{
public:
	SpotBxDF(float width, float fall) : BxDF(BxDFType(BSDF_REFLECTION | BSDF_DIFFUSE)), cosTotalWidth(width), cosFalloffStart(fall) {}
	virtual ~SpotBxDF() { }
	virtual void f(const TsPack *tspack, const Vector &wo, const Vector &wi, SWCSpectrum *const f) const {
		*f += LocalFalloff(wi, cosTotalWidth, cosFalloffStart);
	}
	virtual bool Sample_f(const TsPack *tspack, const Vector &wo, Vector *wi, float u1, float u2, SWCSpectrum *const f,float *pdf, float *pdfBack = NULL, bool reverse = false) const
	{
		*wi = UniformSampleCone(u1, u2, cosTotalWidth);
		*pdf = UniformConePdf(cosTotalWidth);
		if (pdfBack)
			*pdfBack = Pdf(*wi, wo);
		*f = LocalFalloff(*wi, cosTotalWidth, cosFalloffStart);
		return true;
	}
	virtual float Pdf(const Vector &wi, const Vector &wo) const
	{
		if (CosTheta(wo) < cosTotalWidth)
			return 0.f;
		else
			return UniformConePdf(cosTotalWidth);
	}
private:
	float cosTotalWidth, cosFalloffStart;
};

// SpotLight Method Definitions
SpotLight::SpotLight(const Transform &light2world,
		const boost::shared_ptr< Texture<SWCSpectrum> > L, 
		float g, float width, float fall)
	: Light(light2world) {
	lightPos = LightToWorld(Point(0,0,0));

	Lbase = L;
	Lbase->SetIlluminant();
	gain = g;

	cosTotalWidth = cosf(Radians(width));
	cosFalloffStart = cosf(Radians(fall));
}
SpotLight::~SpotLight() {}
float SpotLight::Falloff(const Vector &w) const {
	return LocalFalloff(Normalize(WorldToLight(w)), cosTotalWidth, cosFalloffStart);
}
SWCSpectrum SpotLight::Sample_L(const TsPack *tspack, const Point &p, float u1, float u2, float u3,
		Vector *wi, float *pdf, VisibilityTester *visibility) const {
	*pdf = 1.f;
	*wi = Normalize(lightPos - p);
	visibility->SetSegment(p, lightPos, tspack->time);
	return Lbase->Evaluate(tspack, dummydg) * gain * Falloff(-*wi) /
		DistanceSquared(lightPos, p);
}
float SpotLight::Pdf(const Point &, const Vector &) const {
	return 0.;
}
float SpotLight::Pdf(const Point &p, const Normal &n,
	const Point &po, const Normal &ns) const
{
	return AbsDot(Normalize(p - po), ns) / DistanceSquared(p, po);
}
SWCSpectrum SpotLight::Sample_L(const TsPack *tspack, const Scene *scene, float u1,
		float u2, float u3, float u4,
		Ray *ray, float *pdf) const {
	ray->o = lightPos;
	Vector v = UniformSampleCone(u1, u2, cosTotalWidth);
	ray->d = LightToWorld(v);
	*pdf = UniformConePdf(cosTotalWidth);
	return Lbase->Evaluate(tspack, dummydg) * gain * Falloff(ray->d);
}
bool SpotLight::Sample_L(const TsPack *tspack, const Scene *scene, float u1, float u2, float u3, BSDF **bsdf, float *pdf, SWCSpectrum *Le) const
{
	Normal ns = LightToWorld(Normal(0, 0, 1));
	Vector dpdu, dpdv;
	CoordinateSystem(Vector(ns), &dpdu, &dpdv);
	DifferentialGeometry dg(lightPos, ns, dpdu, dpdv, Normal(0, 0, 0), Normal(0, 0, 0), 0, 0, NULL);
	*bsdf = BSDF_ALLOC(tspack, SingleBSDF)(dg, ns,
		BSDF_ALLOC(tspack, SpotBxDF)(cosTotalWidth, cosFalloffStart));
	*pdf = 1.f;
	*Le = Lbase->Evaluate(tspack, dummydg) * gain;
	return true;
}
bool SpotLight::Sample_L(const TsPack *tspack, const Scene *scene, const Point &p, const Normal &n,
	float u1, float u2, float u3, BSDF **bsdf, float *pdf, float *pdfDirect,
	VisibilityTester *visibility, SWCSpectrum *Le) const
{
	const Vector w(p - lightPos);
	*pdfDirect = 1.f / w.LengthSquared();
	Normal ns = LightToWorld(Normal(0, 0, 1));
	*pdf = 1.f;
	Vector dpdu, dpdv;
	CoordinateSystem(Vector(ns), &dpdu, &dpdv);
	DifferentialGeometry dg(lightPos, ns, dpdu, dpdv, Normal(0, 0, 0), Normal(0, 0, 0), 0, 0, NULL);
	*bsdf = BSDF_ALLOC(tspack, SingleBSDF)(dg, ns,
		BSDF_ALLOC(tspack, SpotBxDF)(cosTotalWidth, cosFalloffStart));
	visibility->SetSegment(p, lightPos, tspack->time);
	*Le = Lbase->Evaluate(tspack, dummydg) * gain;
	return true;
}
SWCSpectrum SpotLight::Le(const TsPack *tspack, const Scene *scene, const Ray &r,
	const Normal &n, BSDF **bsdf, float *pdf, float *pdfDirect) const
{
	return SWCSpectrum(0.f);
}
Light* SpotLight::CreateLight(const Transform &l2w, const ParamSet &paramSet, const TextureParams &tp) {
	boost::shared_ptr<Texture<SWCSpectrum> > L = tp.GetSWCSpectrumTexture("L", RGBColor(1.f));
	float g = paramSet.FindOneFloat("gain", 1.f);
	float coneangle = paramSet.FindOneFloat("coneangle", 30.);
	float conedelta = paramSet.FindOneFloat("conedeltaangle", 5.);
	// Compute spotlight world to light transformation
	Point from = paramSet.FindOnePoint("from", Point(0,0,0));
	Point to = paramSet.FindOnePoint("to", Point(0,0,1));
	Vector dir = Normalize(to - from);
	Vector du, dv;
	CoordinateSystem(dir, &du, &dv);
	boost::shared_ptr<Matrix4x4> o (new Matrix4x4( du.x,  du.y,  du.z, 0.,
	                                 dv.x,  dv.y,  dv.z, 0.,
	                                dir.x, dir.y, dir.z, 0.,
	                                    0,     0,     0, 1.));
	Transform dirToZ = Transform(o);
	Transform light2world =
	l2w *
	Translate(Vector(from.x, from.y, from.z)) *
	dirToZ.GetInverse();
	return new SpotLight(light2world, L, g, coneangle,
		coneangle-conedelta);
}

static DynamicLoader::RegisterLight<SpotLight> r("spot");

