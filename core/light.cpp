/***************************************************************************
 *   Copyright (C) 1998-2007 by authors (see AUTHORS.txt )                 *
 *                                                                         *
 *   This file is part of Lux Renderer.                                    *
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
 *   Lux Renderer website : http://www.luxrender.org                       *
 ***************************************************************************/

// light.cpp*
#include "light.h"
#include "scene.h"
// Light Method Definitions
Light::~Light() {
}
bool VisibilityTester::Unoccluded(const Scene *scene) const {
	// Update shadow ray statistics
	// radiance - disabled for threading // static StatsCounter nShadowRays("Lights","Number of shadow rays traced");
	// radiance - disabled for threading // ++nShadowRays;
	return !scene->IntersectP(r);
}
Spectrum VisibilityTester::
	Transmittance(const Scene *scene) const {
	return scene->Transmittance(r);
}
Spectrum Light::Le(const RayDifferential &) const {
	return Spectrum(0.);
}

void Light::AddPortalShape(ShapePtr s) {
	ShapePtr PortalShape;

	if (s->CanIntersect())
		PortalShape = s;
	else {
		// Create _ShapeSet_ for _Shape_
		ShapePtr shapeSet = s;
		vector<ShapePtr > todo, done;
		todo.push_back(shapeSet);
		while (todo.size()) {
			ShapePtr sh = todo.back();
			todo.pop_back();
			if (sh->CanIntersect())
				done.push_back(sh);
			else
				sh->Refine(todo);
		}
		if (done.size() == 1) PortalShape = done[0];
		else {
			if (done.size() > 16)
				Warning("Portal light geometry turned into %d shapes; "
					"may be very inefficient.", (int)done.size());
			ShapePtr o (new ShapeSet(done, s->ObjectToWorld, s->reverseOrientation));
			PortalShape = o;
			havePortalShape = true; 
			// store
			PortalArea += PortalShape->Area();
			PortalShapes.push_back(PortalShape);
		}
	}

}
