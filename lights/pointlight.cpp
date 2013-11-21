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

// point.cpp*
#include "pointlight.h"
#include "bxdf.h"
#include "singlebsdf.h"
#include "sphericalfunction.h"
#include "sampling.h"
#include "paramset.h"
#include "dynload.h"
#include "luxrays/utils/mc.h"

using namespace lux;
using namespace luxrays;

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
	virtual bool SampleF(const SpectrumWavelengths &sw, const Vector &woW,
		Vector *wiW, float u1, float u2, float u3,
		SWCSpectrum *const f_, float *pdf, BxDFType flags = BSDF_ALL,
		BxDFType *sampledType = NULL, float *pdfBack = NULL,
		bool reverse = false) const {
		if (reverse || NumComponents(flags) == 0)
			return false;
		*wiW = UniformSampleSphere(u1, u2);
		if (sampledType)
			*sampledType = BSDF_DIFFUSE;
		*pdf = .25f * INV_PI;
		if (pdfBack)
			*pdfBack = 0.f;
		*f_ = SWCSpectrum(1.f);
		return true;
	}
	virtual float Pdf(const SpectrumWavelengths &sw, const Vector &woW,
		const Vector &wiW, BxDFType flags = BSDF_ALL) const {
		if (NumComponents(flags) == 1)
			return .25f * INV_PI;
		return 0.f;
	}
	virtual SWCSpectrum F(const SpectrumWavelengths &sw, const Vector &woW,
		const Vector &wiW, bool reverse, BxDFType flags = BSDF_ALL) const {
		if (NumComponents(flags) == 1)
			return SWCSpectrum(.25f * INV_PI);
		return SWCSpectrum(0.f);
	}
	virtual SWCSpectrum rho(const SpectrumWavelengths &sw,
		BxDFType flags = BSDF_ALL) const { return SWCSpectrum(1.f); }
	virtual SWCSpectrum rho(const SpectrumWavelengths &sw,
		const Vector &woW, BxDFType flags = BSDF_ALL) const {
		return SWCSpectrum(1.f);
	}

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
	virtual bool SampleF(const SpectrumWavelengths &sw, const Vector &woW,
		Vector *wiW, float u1, float u2, float u3,
		SWCSpectrum *const f_, float *pdf, BxDFType flags = BSDF_ALL,
		BxDFType *sampledType = NULL, float *pdfBack = NULL,
		bool reverse = false) const {
		if (reverse || NumComponents(flags) == 0)
			return false;
		*f_ = sf->SampleF(sw, u1, u2, wiW, pdf);
		*wiW = Normalize(LocalToWorld(*wiW));
		*f_ /= sf->Average_f();
		if (sampledType)
			*sampledType = BSDF_DIFFUSE;
		if (pdfBack)
			*pdfBack = 0.f;
		return true;
	}
	virtual float Pdf(const SpectrumWavelengths &sw, const Vector &woW,
		const Vector &wiW, BxDFType flags = BSDF_ALL) const {
		if (NumComponents(flags) == 1)
			return sf->Pdf(WorldToLocal(wiW));
		return 0.f;
	}
	virtual SWCSpectrum F(const SpectrumWavelengths &sw, const Vector &woW,
		const Vector &wiW, bool reverse, BxDFType flags = BSDF_ALL) const {
		if (NumComponents(flags) == 1)
			return sf->f(sw, WorldToLocal(wiW)) / sf->Average_f();
		return SWCSpectrum(0.f);
	}
	virtual SWCSpectrum rho(const SpectrumWavelengths &sw,
		BxDFType flags = BSDF_ALL) const { return SWCSpectrum(1.f); }
	virtual SWCSpectrum rho(const SpectrumWavelengths &sw,
		const Vector &woW, BxDFType flags = BSDF_ALL) const {
		return SWCSpectrum(1.f);
	}

protected:
	// GonioBSDF Private Methods
	virtual ~GonioBSDF() { }
	//GonioBSDF Private Data
	const SampleableSphericalFunction *sf;
};

// PointLight Method Definitions
PointLight::PointLight(const Transform &light2world,
	const boost::shared_ptr< Texture<SWCSpectrum> > &L,
	float g, float power, float efficacy,
	SampleableSphericalFunction *ssf) :
	Light("PointLight-" + boost::lexical_cast<string>(this), light2world),
	Lbase(L), gain(g), func(ssf)
{
	lightPos = LightToWorld * Point(0,0,0);
	Lbase->SetIlluminant(); // Illuminant must be set before calling Le->Y()
	const float gainFactor = power * efficacy / (4.f * M_PI * Lbase->Y());
	if (gainFactor > 0.f && !isinf(gainFactor))
		gain *= gainFactor;

	AddFloatAttribute(*this, "gain", "PointLight gain", &PointLight::gain);
}
PointLight::~PointLight() {
	delete func;
}
float PointLight::Power(const Scene &) const {
	return Lbase->Y() * gain * 4.f * M_PI;
}

float PointLight::Pdf(const Point &p, const PartialDifferentialGeometry &dg) const
{
	return 1.f;
}

bool PointLight::SampleL(const Scene &scene, const Sample &sample,
	float u1, float u2, float u3, BSDF **bsdf, float *pdf,
	SWCSpectrum *Le) const
{
	*pdf = 1.f;
	const Normal ns(Normalize(LightToWorld * Normal(0, 0, 1)));
	DifferentialGeometry dg(lightPos, ns,
		Normalize(LightToWorld * Vector(1, 0, 0)),
		Normalize(LightToWorld * Vector(0, 1, 0)),
		Normal(0, 0, 0), Normal(0, 0, 0), 0, 0, NULL);
	dg.time = sample.realTime;
	const Volume *v = GetVolume();
	if(func)
		*bsdf = ARENA_ALLOC(sample.arena, GonioBSDF)(dg, ns,
			v, v, func);
	else
		*bsdf = ARENA_ALLOC(sample.arena, UniformBSDF)(dg, ns,
			v, v);
	*Le = Lbase->Evaluate(sample.swl, dg) * (gain * 4.f * M_PI);
	return true;
}
bool PointLight::SampleL(const Scene &scene, const Sample &sample,
	const Point &p, float u1, float u2, float u3, BSDF **bsdf, float *pdf,
	float *pdfDirect, SWCSpectrum *Le) const
{
	const Normal ns(Normalize(LightToWorld * Normal(0, 0, 1)));
	DifferentialGeometry dg(lightPos, ns,
		Normalize(LightToWorld * Vector(1, 0, 0)),
		Normalize(LightToWorld * Vector(0, 1, 0)),
		Normal(0, 0, 0), Normal(0, 0, 0), 0, 0, NULL);
	dg.time = sample.realTime;
	*pdfDirect = 1.f;
	if (pdf)
		*pdf = 1.f;
	const Volume *v = GetVolume();
	if (func)
		*bsdf = ARENA_ALLOC(sample.arena, GonioBSDF)(dg, ns,
			v, v, func);
	else
		*bsdf = ARENA_ALLOC(sample.arena, UniformBSDF)(dg, ns,
			v, v);
	*Le = Lbase->Evaluate(sample.swl, dg) * (gain * 4.f * M_PI);
	return true;
}
Light* PointLight::CreateLight(const Transform &light2world,
		const ParamSet &paramSet) {
	boost::shared_ptr<Texture<SWCSpectrum> > L(paramSet.GetSWCSpectrumTexture("L", RGBColor(1.f)));
	float g = paramSet.FindOneFloat("gain", 1.f);
	float p = paramSet.FindOneFloat("power", 0.f);		// Power/Lm in Watts
	float e = paramSet.FindOneFloat("efficacy", 0.f);	// Efficacy Lm per Watt

	boost::shared_ptr<const SphericalFunction> sf(CreateSphericalFunction(paramSet));
	SampleableSphericalFunction *ssf = NULL;
	if(sf)
		ssf = new SampleableSphericalFunction(sf);

	Point P = paramSet.FindOnePoint("from", Point(0,0,0));
	Transform l2w = Translate(Vector(P.x, P.y, P.z)) * light2world;

	PointLight *l = new PointLight(l2w, L, g, p, e, ssf);
	l->hints.InitParam(paramSet);
	return l;
}

static DynamicLoader::RegisterLight<PointLight> r("point");
static DynamicLoader::RegisterLight<PointLight> rb("goniometric"); // Backwards compability for removed 'GonioPhotometricLight'

