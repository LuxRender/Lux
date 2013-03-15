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

// light.cpp*
#include "light.h"
#include "scene.h"
#include "shape.h"
#include "camera.h"
#include "reflection/bxdf.h"
#include "sampling.h"

using namespace lux;

// Light Method Definitions

void Light::AddPortalShape(boost::shared_ptr<Primitive> &s)
{
	if (s->CanIntersect() && s->CanSample()) {
		PortalArea += s->Area();
		PortalShapes.push_back(s);
		++nrPortalShapes;
	} else {
		// Create _ShapeSet_ for _Shape_
		vector<boost::shared_ptr<Primitive> > done;
		PrimitiveRefinementHints refineHints(true);
		s->Refine(done, refineHints, s);
		for (u_int i = 0; i < done.size(); ++i) {
			PortalArea += done[i]->Area();
			PortalShapes.push_back(done[i]);
			++nrPortalShapes;
		}
	}
	havePortalShape = true;
}

bool InstanceLight::Le(const Scene &scene, const Sample &sample, const Ray &r,
	BSDF **bsdf, float *pdf, float *pdfDirect, SWCSpectrum *L) const
{
	if (!light->Le(scene, sample, Inverse(LightToWorld) * r, bsdf,
		pdf, pdfDirect, L))
		return false;
	float factor = (*bsdf)->dgShading.Volume();
	factor /= (*bsdf)->ApplyTransform(LightToWorld);
	if (pdf)
		*pdf *= factor;
	if (pdfDirect)
		*pdfDirect *= factor;
	return true;
}

bool InstanceLight::Le(const Scene &scene, const Sample &sample, const Point &p,
	BSDF **bsdf, float *pdf, float *pdfDirect, SWCSpectrum *L) const
{
	if (!light->Le(scene, sample, Inverse(LightToWorld) * p, bsdf,
		pdf, pdfDirect, L))
		return false;
	float factor = (*bsdf)->dgShading.Volume();
	factor /= (*bsdf)->ApplyTransform(LightToWorld);
	if (pdf)
		*pdf *= factor;
	if (pdfDirect)
		*pdfDirect *= factor;
	return true;
}

bool InstanceLight::SampleL(const Scene &scene, const Sample &sample,
	float u1, float u2, float u3, BSDF **bsdf, float *pdf,
	SWCSpectrum *L) const
{
	if (!light->SampleL(scene, sample, u1, u2, u3, bsdf, pdf, L))
		return false;
	float factor = (*bsdf)->dgShading.Volume();
	factor /= (*bsdf)->ApplyTransform(LightToWorld);
	*pdf *= factor;
	*L /= factor;
	return true;
}

bool InstanceLight::SampleL(const Scene &scene, const Sample &sample,
	const Point &p, float u1, float u2, float u3,
	BSDF **bsdf, float *pdf, float *pdfDirect, SWCSpectrum *L) const
{
	if (!light->SampleL(scene, sample, Inverse(LightToWorld) * p, u1, u2, u3,
		bsdf, pdf, pdfDirect, L))
		return false;
	float factor = (*bsdf)->dgShading.Volume();
	factor /= (*bsdf)->ApplyTransform(LightToWorld);
	if (pdf)
		*pdf *= factor;
	*pdfDirect *= factor;
	*L /= factor;
	return true;
}

bool InstanceLight::SampleL(const Scene &scene, const Sample &sample,
	const Point &p, const Normal &n, float u1, float u2, float u3,
	BSDF **bsdf, float *pdf, float *pdfDirect, SWCSpectrum *L) const
{
	if (!light->SampleL(scene, sample, Inverse(LightToWorld) * p, n, u1, u2, u3,
		bsdf, pdf, pdfDirect, L))
		return false;
	float factor = (*bsdf)->dgShading.Volume();
	factor /= (*bsdf)->ApplyTransform(LightToWorld);
	if (pdf)
		*pdf *= factor;
	*pdfDirect *= factor;
	*L /= factor;
	return true;
}

bool MotionLight::Le(const Scene &scene, const Sample &sample, const Ray &r,
	BSDF **bsdf, float *pdf, float *pdfDirect, SWCSpectrum *L) const
{
	const Transform LightToWorld(motionPath.Sample(sample.realTime));
	if (!light->Le(scene, sample, Inverse(LightToWorld) * r, bsdf,
		pdf, pdfDirect, L))
		return false;
	float factor = (*bsdf)->dgShading.Volume();
	factor /= (*bsdf)->ApplyTransform(LightToWorld);
	if (pdf)
		*pdf *= factor;
	if (pdfDirect)
		*pdfDirect *= factor;
	return true;
}

bool MotionLight::Le(const Scene &scene, const Sample &sample, const Point &p,
	BSDF **bsdf, float *pdf, float *pdfDirect, SWCSpectrum *L) const
{
	const Transform LightToWorld(motionPath.Sample(sample.realTime));
	if (!light->Le(scene, sample, Inverse(LightToWorld) * p, bsdf,
		pdf, pdfDirect, L))
		return false;
	float factor = (*bsdf)->dgShading.Volume();
	factor /= (*bsdf)->ApplyTransform(LightToWorld);
	if (pdf)
		*pdf *= factor;
	if (pdfDirect)
		*pdfDirect *= factor;
	return true;
}

bool MotionLight::SampleL(const Scene &scene, const Sample &sample,
	float u1, float u2, float u3, BSDF **bsdf, float *pdf,
	SWCSpectrum *L) const
{
	if (!light->SampleL(scene, sample, u1, u2, u3, bsdf, pdf, L))
		return false;
	const Transform LightToWorld(motionPath.Sample(sample.realTime));
	float factor = (*bsdf)->dgShading.Volume();
	factor /= (*bsdf)->ApplyTransform(LightToWorld);
	*pdf *= factor;
	*L /= factor;
	return true;
}

bool MotionLight::SampleL(const Scene &scene, const Sample &sample,
	const Point &p, float u1, float u2, float u3,
	BSDF **bsdf, float *pdf, float *pdfDirect, SWCSpectrum *L) const
{
	const Transform LightToWorld(motionPath.Sample(sample.realTime));
	if (!light->SampleL(scene, sample, Inverse(LightToWorld) * p, u1, u2, u3,
		bsdf, pdf, pdfDirect, L))
		return false;
	float factor = (*bsdf)->dgShading.Volume();
	factor /= (*bsdf)->ApplyTransform(LightToWorld);
	if (pdf)
		*pdf *= factor;
	*pdfDirect *= factor;
	*L /= factor;
	return true;
}

bool MotionLight::SampleL(const Scene &scene, const Sample &sample,
	const Point &p, const Normal &n, float u1, float u2, float u3,
	BSDF **bsdf, float *pdf, float *pdfDirect, SWCSpectrum *L) const
{
	const Transform LightToWorld(motionPath.Sample(sample.realTime));
	if (!light->SampleL(scene, sample, Inverse(LightToWorld) * p, n, u1, u2, u3,
		bsdf, pdf, pdfDirect, L))
		return false;
	float factor = (*bsdf)->dgShading.Volume();
	factor /= (*bsdf)->ApplyTransform(LightToWorld);
	if (pdf)
		*pdf *= factor;
	*pdfDirect *= factor;
	*L /= factor;
	return true;
}
