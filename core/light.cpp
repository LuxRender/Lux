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

// light.cpp*
#include "light.h"
#include "scene.h"
#include "reflection/bxdf.h"

using namespace lux;

// Light Method Definitions
Light::~Light() {
}
bool VisibilityTester::Unoccluded(const Scene *scene) const {
	// Update shadow ray statistics
	// radiance - disabled for threading // static StatsCounter nShadowRays("Lights","Number of shadow rays traced");
	// radiance - disabled for threading // ++nShadowRays;
	return !scene->IntersectP(r);
}

bool VisibilityTester::TestOcclusion(const TsPack *tspack, const Scene *scene, SWCSpectrum *f) const {
	*f = 1.f;
	RayDifferential ray(r);
	Intersection isect;
	while (true) {
		if (!scene->Intersect(ray, &isect))
			return true;
		BSDF *bsdf = isect.GetBSDF(tspack, ray, tspack->rng->floatValue());							// TODO - REFACT - remove and add random value from sample
		const float pdf = bsdf->Pdf(tspack, -ray.d, ray.d, BSDF_ALL_TRANSMISSION);
		if (!(pdf > 0.f))
			return false;
		*f *= AbsDot(Normalize(bsdf->dgShading.nn), Normalize(ray.d)) / pdf;

		*f *= bsdf->f(tspack, -ray.d, ray.d);
		if (f->Black())
			return false;

		// Dade - need to scale the RAY_EPSILON value because the ray direction
		// is not normalized (in order to avoid light leaks: bug #295)
		float epsilon = SHADOW_RAY_EPSILON / ray.d.Length();
		ray.mint = ray.maxt + epsilon;
		ray.maxt = r.maxt;
	}
}

void VisibilityTester::Transmittance(const TsPack *tspack, const Scene *scene, 
									 const Sample *sample, SWCSpectrum *const L) const {
	return scene->Transmittance(tspack, r, sample, L);
}
SWCSpectrum Light::Le(const TsPack *tspack, const RayDifferential &) const {
	return SWCSpectrum(0.);
}
SWCSpectrum Light::Le(const TsPack *tspack, const Scene *scene, const Ray &r,
	const Normal &n, BSDF **bsdf, float *pdf, float *pdfDirect) const
{
	return SWCSpectrum(0.f);
}

void Light::AddPortalShape(boost::shared_ptr<Primitive> s) {
	boost::shared_ptr<Primitive> PortalShape;

	if (s->CanIntersect() && s->CanSample())
		PortalShape = s;
	else {
		// Create _ShapeSet_ for _Shape_
		vector<boost::shared_ptr<Primitive> > done;
		PrimitiveRefinementHints refineHints(true);
		s->Refine(done, refineHints, s);
		if (done.size() == 1) PortalShape = done[0];
		else {
			boost::shared_ptr<Primitive> o (new PrimitiveSet(done));
			PortalShape = o;
		}
	}
	havePortalShape = true;
	// store
	PortalArea += PortalShape->Area();
	PortalShapes.push_back(PortalShape);
	++nrPortalShapes;
}
