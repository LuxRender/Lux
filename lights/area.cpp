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

using namespace lux;

// AreaLight Method Definitions
AreaLight::AreaLight(const Transform &light2world,								// TODO - radiance - add portal implementation
		const Spectrum &le, int ns,
		const boost::shared_ptr<Shape> &s)
	: Light(light2world, ns) {
	Lemit = le;
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
SWCSpectrum AreaLight::Sample_L(const Point &p,
		const Normal &n, float u1, float u2,
		Vector *wi, float *pdf,
		VisibilityTester *visibility) const {
	Normal ns;
	Point ps = shape->Sample(p, u1, u2, &ns);
	*wi = Normalize(ps - p);
	*pdf = shape->Pdf(p, *wi);
	visibility->SetSegment(p, ps);
	return L(ps, ns, -*wi);
}
float AreaLight::Pdf(const Point &p, const Normal &N,
		const Vector &wi) const {
	return shape->Pdf(p, wi);
}
SWCSpectrum AreaLight::Sample_L(const Point &P,
		float u1, float u2, Vector *wo, float *pdf,
		VisibilityTester *visibility) const {
	Normal Ns;
	Point Ps = shape->Sample(P, u1, u2, &Ns);
	*wo = Normalize(Ps - P);
	*pdf = shape->Pdf(P, *wo);
	visibility->SetSegment(P, Ps);
	return L(Ps, Ns, -*wo);
}
SWCSpectrum AreaLight::Sample_L(const Scene *scene, float u1,
		float u2, float u3, float u4,
		Ray *ray, float *pdf) const {
	Normal ns;
	ray->o = shape->Sample(u1, u2, &ns);
	ray->d = UniformSampleSphere(u3, u4);
	if (Dot(ray->d, ns) < 0.) ray->d *= -1;
	*pdf = shape->Pdf(ray->o) * INV_TWOPI;
	return L(ray->o, ns, ray->d);
}
float AreaLight::Pdf(const Point &P, const Vector &w) const {
	return shape->Pdf(P, w);
}
SWCSpectrum AreaLight::Sample_L(const Point &P, Vector *wo,
		VisibilityTester *visibility) const {
	Normal Ns;
	Point Ps = shape->Sample(P, lux::random::floatValue(), lux::random::floatValue(), &Ns);
	*wo = Normalize(Ps - P);
	visibility->SetSegment(P, Ps);
	float pdf = shape->Pdf(P, *wo);
	if (pdf == 0.f) return SWCSpectrum(0.f);
	return L(P, Ns, -*wo) /	pdf;
}

void AreaLight::SamplePosition(float u1, float u2, Point *p, Normal *n, float *pdf) const
{
	*p = shape->Sample(u1, u2, n);
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
SWCSpectrum AreaLight::Eval(const Normal &n, const Vector &w) const
{
	return Dot(n, w) > 0 ? Lemit : 0.;
}

AreaLight* AreaLight::CreateAreaLight(const Transform &light2world, const ParamSet &paramSet,
		const boost::shared_ptr<Shape> &shape) {
	Spectrum L = paramSet.FindOneSpectrum("L", Spectrum(1.0));
	int nSamples = paramSet.FindOneInt("nsamples", 1);
	return new AreaLight(light2world, L, nSamples, shape);
}
