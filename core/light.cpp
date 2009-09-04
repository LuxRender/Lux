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

// light.cpp*
#include "light.h"
#include "scene.h"
#include "shape.h"
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

bool VisibilityTester::TestOcclusion(const TsPack *tspack, const Scene *scene, SWCSpectrum *f, float *pdf, float *pdfR) const
{
	RayDifferential ray(r);
	ray.time = tspack->time;
	Vector d(Normalize(ray.d));
	// Dade - need to scale the RAY_EPSILON value because the ray direction
	// is not normalized (in order to avoid light leaks: bug #295)
	const float epsilon = SHADOW_RAY_EPSILON / ray.d.Length();
	Intersection isect;
	const BxDFType flags(BxDFType(BSDF_SPECULAR | BSDF_TRANSMISSION));
	// The for loop prevent an infinite sequence when the ray is almost
	// parrallel to the surface and is self shadowed
	for (u_int i = 0; i < 10000; ++i) {
		if (!scene->Intersect(ray, &isect))
			return true;
		BSDF *bsdf = isect.GetBSDF(tspack, ray);

		*f *= bsdf->f(tspack, -d, d, flags);
		if (f->Black())
			return false;
		*f *= AbsDot(bsdf->dgShading.nn, d);
		if (pdf)
			*pdf *= bsdf->Pdf(tspack, -d, d);
		if (pdfR)
			*pdfR *= bsdf->Pdf(tspack, d, -d);

		ray.mint = ray.maxt + epsilon;
		ray.maxt = r.maxt;
	}
	return false;
}

void VisibilityTester::Transmittance(const TsPack *tspack, const Scene *scene, 
	const Sample *sample, SWCSpectrum *const L) const {
	scene->Transmittance(tspack, r, sample, L);
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
