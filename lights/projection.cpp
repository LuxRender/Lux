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

// projection.cpp*
#include "projection.h"
#include "imagereader.h"
#include "bxdf.h"
#include "mc.h"
#include "paramset.h"
#include "dynload.h"
#include "epsilon.h"

using namespace lux;

class ProjectionBxDF : public BxDF
{
public:
	ProjectionBxDF(float A, const MIPMap<RGBColor> *map,
		const Transform &proj, float xS, float xE, float yS, float yE) :
		BxDF(BxDFType(BSDF_REFLECTION | BSDF_DIFFUSE)),
		xStart(xS), xEnd(xE), yStart(yS), yEnd(yE), Area(A),
		Projection(proj), projectionMap(map) { }
	virtual ~ProjectionBxDF() { }
	virtual void f(const TsPack *tspack, const Vector &wo, const Vector &wi, SWCSpectrum *const f) const
	{
		const float cos = wi.z;
		if (cos < 0.f)
			return;
		const float cos2 = cos * cos;
		const Point p0(Projection(Point(wi.x, wi.y, wi.z)));
		if (p0.x < xStart || p0.x >= xEnd || p0.y < yStart || p0.y >= yEnd)
			return;
		if (!projectionMap)
			*f += SWCSpectrum(1.f / (Area * cos2 * cos2));
		else {
			const float s = (p0.x - xStart) / (xEnd - xStart);
			const float t = (p0.y - yStart) / (yEnd - yStart);
			f->AddWeighted(1.f / (Area * cos2 * cos2),
				SWCSpectrum(tspack, projectionMap->Lookup(s, t)));
		}
	}
	virtual bool Sample_f(const TsPack *tspack, const Vector &wo, Vector *wi, float u1, float u2,
		SWCSpectrum *const f, float *pdf, float *pdfBack = NULL, bool reverse = false) const
	{
		const Point pS(Projection.GetInverse()(Point(u1 * (xEnd - xStart) + xStart, u2 * (yEnd - yStart) + yStart, 0.f)));
		*wi = Normalize(Vector(pS.x, pS.y, pS.z));
		const float cos = wi->z;
		const float cos2 = cos * cos;
		*pdf = 1.f / (Area * cos2 * cos);
		if (pdfBack)
			*pdfBack = 0.f;
		if (!projectionMap)
			*f = SWCSpectrum(1.f / (Area * cos2 * cos2));
		else
			*f = SWCSpectrum(tspack, projectionMap->Lookup(u1, u2)) /
				(Area * cos2 * cos2);
		return true;
	}
	virtual float Pdf(const TsPack *tspack, const Vector &wi, const Vector &wo) const
	{
		const float cos = wo.z;
		if (cos < 0.f)
			return 0.f;
		const float cos2 = cos * cos;
		const Point p0(Projection(Point(wo.x, wo.y, wo.z)));
		if (p0.x < xStart || p0.x >= xEnd || p0.y < yStart || p0.y >= yEnd)
			return 0.f;
		else 
			return 1.f / (Area * cos2 * cos);
	}
private:
	float xStart, xEnd, yStart, yEnd, Area;
	const Transform &Projection;
	const MIPMap<RGBColor> *projectionMap;
};

// ProjectionLight Method Definitions
ProjectionLight::
	ProjectionLight(const Transform &light2world,
		const boost::shared_ptr< Texture<SWCSpectrum> > L, 
		float g, const string &texname,
		float fov)
	: Light(light2world) {
	lightPos = LightToWorld(Point(0,0,0));
	Lbase = L;
	Lbase->SetIlluminant();
	gain = g;
	// Create _ProjectionLight_ MIP-map
	int width = 0, height = 0;
	auto_ptr<ImageData> imgdata(ReadImage(texname));
	if (imgdata.get()!=NULL) {
		width=imgdata->getWidth();
		height=imgdata->getHeight();
		projectionMap =imgdata->createMIPMap<RGBColor>();
	}
	else 
		projectionMap = NULL;

	// Initialize _ProjectionLight_ projection matrix
	float aspect = float(width) / float(height);
	if (aspect > 1.f)  {
		screenX0 = -aspect;  screenX1 =  aspect;
		screenY0 = -1.f;     screenY1 =  1.f;
	}
	else {
		screenX0 = -1.f;            screenX1 =  1.f;
		screenY0 = -1.f / aspect;   screenY1 =  1.f / aspect;
	}
	hither = DEFAULT_EPSILON_STATIC;
	yon = 1e30f;
	lightProjection = Perspective(fov, hither, yon);
	// Compute cosine of cone surrounding projection directions
	float opposite = tanf(Radians(fov) / 2.f);
	float tanDiag = opposite * sqrtf(1.f + 1.f/(aspect*aspect));
	cosTotalWidth = cosf(atanf(tanDiag));
	area = 4.f * opposite * opposite / aspect;
}
ProjectionLight::~ProjectionLight() { delete projectionMap; }
RGBColor ProjectionLight::Projection(const Vector &w) const {
	Vector wl = WorldToLight(w);
	// Discard directions behind projection light
	if (wl.z < hither) return 0.;
	// Project point on to projection plane and compute light
	Point Pl = lightProjection(Point(wl.x, wl.y, wl.z));
	if (Pl.x < screenX0 || Pl.x > screenX1 ||
		Pl.y < screenY0 || Pl.y > screenY1) return 0.;
	if (!projectionMap) return 1;
	float s = (Pl.x - screenX0) / (screenX1 - screenX0);
	float t = (Pl.y - screenY0) / (screenY1 - screenY0);
	return projectionMap->Lookup(s, t);
}
SWCSpectrum ProjectionLight::Sample_L(const TsPack *tspack, const Point &p, float u1, float u2,
		float u3, Vector *wi, float *pdf,
		VisibilityTester *visibility) const {
	*wi = Normalize(lightPos - p);
	*pdf = 1.f;
	visibility->SetSegment(p, lightPos, tspack->time);
	return Lbase->Evaluate(tspack, dummydg) * gain * SWCSpectrum(tspack, Projection(-*wi)) / DistanceSquared(lightPos, p);
}
SWCSpectrum ProjectionLight::Sample_L(const TsPack *tspack, const Scene *scene, float u1, float u2,
		float u3, float u4, Ray *ray, float *pdf) const {
	ray->o = lightPos;
	Vector v = UniformSampleCone(u1, u2, cosTotalWidth);
	ray->d = LightToWorld(v);
	*pdf = UniformConePdf(cosTotalWidth);
	return Lbase->Evaluate(tspack, dummydg) * gain * SWCSpectrum(tspack, Projection(ray->d));
}
float ProjectionLight::Pdf(const TsPack *, const Point &, const Vector &) const {
	return 0.;
}
float ProjectionLight::Pdf(const TsPack *tspack, const Point &p, const Normal &n,
	const Point &po, const Normal &ns) const
{
	return 1.f;
}
bool ProjectionLight::Sample_L(const TsPack *tspack, const Scene *scene, float u1, float u2, float u3, BSDF **bsdf, float *pdf, SWCSpectrum *Le) const
{
	Normal ns = LightToWorld(Normal(0, 0, 1));
	Vector dpdu, dpdv;
	CoordinateSystem(Vector(ns), &dpdu, &dpdv);
	DifferentialGeometry dg(lightPos, ns, dpdu, dpdv, Normal(0, 0, 0), Normal(0, 0, 0), 0, 0, NULL);
	*bsdf = ARENA_ALLOC(tspack->arena, SingleBSDF)(dg, ns,
		ARENA_ALLOC(tspack->arena, ProjectionBxDF)(area, projectionMap,
			lightProjection, screenX0, screenX1, screenY0, screenY1));
	*pdf = 1.f;
	*Le = Lbase->Evaluate(tspack, dg) * gain;
	return true;
}
bool ProjectionLight::Sample_L(const TsPack *tspack, const Scene *scene, const Point &p, const Normal &n,
	float u1, float u2, float u3, BSDF **bsdf, float *pdf, float *pdfDirect,
	VisibilityTester *visibility, SWCSpectrum *Le) const
{
	const Vector w(p - lightPos);
	*pdfDirect = 1.f;
	Normal ns = LightToWorld(Normal(0, 0, 1));
	*pdf = 1.f;
	Vector dpdu, dpdv;
	CoordinateSystem(Vector(ns), &dpdu, &dpdv);
	DifferentialGeometry dg(lightPos, ns, dpdu, dpdv, Normal(0, 0, 0), Normal(0, 0, 0), 0, 0, NULL);
	*bsdf = ARENA_ALLOC(tspack->arena, SingleBSDF)(dg, ns,
		ARENA_ALLOC(tspack->arena, ProjectionBxDF)(area, projectionMap,
			lightProjection, screenX0, screenX1, screenY0, screenY1));
	visibility->SetSegment(p, lightPos, tspack->time);
	*Le = Lbase->Evaluate(tspack, dg) * gain;
	return true;
}
SWCSpectrum ProjectionLight::Le(const TsPack *tspack, const Scene *scene, const Ray &r,
	const Normal &n, BSDF **bsdf, float *pdf, float *pdfDirect) const
{
	return SWCSpectrum(0.f);
}

Light* ProjectionLight::CreateLight(const Transform &light2world,
		const ParamSet &paramSet) {
	boost::shared_ptr<Texture<SWCSpectrum> > L = paramSet.GetSWCSpectrumTexture("L", RGBColor(1.f));
	float g = paramSet.FindOneFloat("gain", 1.f);
	float fov = paramSet.FindOneFloat("fov", 45.);
	string texname = paramSet.FindOneString("mapname", "");

	ProjectionLight *l = new ProjectionLight(light2world, L, g, texname, fov);
	l->hints.InitParam(paramSet);
	return l;
}

static DynamicLoader::RegisterLight<ProjectionLight> r("projection");

