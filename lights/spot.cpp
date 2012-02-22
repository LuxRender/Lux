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

// spot.cpp*
#include "spot.h"
#include "memory.h"
#include "color.h"
#include "bxdf.h"
#include "singlebsdf.h"
#include "mc.h"
#include "sampling.h"
#include "paramset.h"
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
	virtual void F(const SpectrumWavelengths &sw, const Vector &wo,
		const Vector &wi, SWCSpectrum *const f) const {
		*f += LocalFalloff(wi, cosTotalWidth, cosFalloffStart) /
			fabsf(wi.z);
	}
	virtual bool SampleF(const SpectrumWavelengths &sw, const Vector &wo,
		Vector *wi, float u1, float u2, SWCSpectrum *const f,float *pdf,
		float *pdfBack = NULL, bool reverse = false) const {
		*wi = UniformSampleCone(u1, u2, cosTotalWidth);
		*pdf = UniformConePdf(cosTotalWidth);
		if (pdfBack)
			*pdfBack = Pdf(sw, *wi, wo);
		*f = LocalFalloff(*wi, cosTotalWidth, cosFalloffStart) /
			(fabsf(wi->z) * *pdf);
		return true;
	}
	virtual float Pdf(const SpectrumWavelengths &sw, const Vector &wi,
		const Vector &wo) const
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
	const boost::shared_ptr< Texture<SWCSpectrum> > &L, 
	float g, float power, float efficacy, float width, float fall)
	: Light(light2world), Lbase(L), gain(g)
{
	lightPos = LightToWorld(Point(0,0,0));

	cosTotalWidth = cosf(Radians(width));
	cosFalloffStart = cosf(Radians(fall));

	Lbase->SetIlluminant(); // Illuminant must be set before calling Le->Y()
	const float gainFactor = power * efficacy /
		(2.f * M_PI * Lbase->Y() *
		(1.f - .5f * (cosFalloffStart + cosTotalWidth)));
	if (gainFactor > 0.f && !isinf(gainFactor))
		gain *= gainFactor;
}
SpotLight::~SpotLight()
{
}

float SpotLight::Pdf(const Point &p, const PartialDifferentialGeometry &dg) const
{
	return 1.f;
}

bool SpotLight::SampleL(const Scene &scene, const Sample &sample,
	float u1, float u2, float u3, BSDF **bsdf, float *pdf,
	SWCSpectrum *Le) const
{
	Normal ns(Normalize(LightToWorld(Normal(0, 0, 1))));
	Vector dpdu, dpdv;
	CoordinateSystem(Vector(ns), &dpdu, &dpdv);
	DifferentialGeometry dg(lightPos, ns, dpdu, dpdv, Normal(0, 0, 0), Normal(0, 0, 0), 0, 0, NULL);
	dg.time = sample.realTime;
	const Volume *v = GetVolume();
	*bsdf = ARENA_ALLOC(sample.arena, SingleBSDF)(dg, ns,
		ARENA_ALLOC(sample.arena, SpotBxDF)(cosTotalWidth, cosFalloffStart), v, v);
	*pdf = 1.f;
	*Le = Lbase->Evaluate(sample.swl, dg) * gain;
	return true;
}
bool SpotLight::SampleL(const Scene &scene, const Sample &sample,
	const Point &p, float u1, float u2, float u3, BSDF **bsdf, float *pdf,
	float *pdfDirect, SWCSpectrum *Le) const
{
	const Vector w(p - lightPos);
	*pdfDirect = 1.f;
	Normal ns(Normalize(LightToWorld(Normal(0, 0, 1))));
	if (pdf)
		*pdf = 1.f;
	Vector dpdu, dpdv;
	CoordinateSystem(Vector(ns), &dpdu, &dpdv);
	DifferentialGeometry dg(lightPos, ns, dpdu, dpdv, Normal(0, 0, 0), Normal(0, 0, 0), 0, 0, NULL);
	dg.time = sample.realTime;
	const Volume *v = GetVolume();
	*bsdf = ARENA_ALLOC(sample.arena, SingleBSDF)(dg, ns,
		ARENA_ALLOC(sample.arena, SpotBxDF)(cosTotalWidth, cosFalloffStart), v, v);
	*Le = Lbase->Evaluate(sample.swl, dg) * gain;
	return true;
}
Light* SpotLight::CreateLight(const Transform &l2w, const ParamSet &paramSet)
{
	boost::shared_ptr<Texture<SWCSpectrum> > L(paramSet.GetSWCSpectrumTexture("L", RGBColor(1.f)));
	float g = paramSet.FindOneFloat("gain", 1.f);
	float p = paramSet.FindOneFloat("power", 0.f);		// Power/Lm in Watts
	float e = paramSet.FindOneFloat("efficacy", 0.f);	// Efficacy Lm per Watt
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
	Transform light2world = l2w *
		Translate(Vector(from.x, from.y, from.z)) *
		dirToZ.GetInverse();

	SpotLight *l = new SpotLight(light2world, L, g, p, e, coneangle,
		coneangle-conedelta);
	l->hints.InitParam(paramSet);
	return l;
}

static DynamicLoader::RegisterLight<SpotLight> r("spot");

