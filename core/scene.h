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

#ifndef LUX_SCENE_H
#define LUX_SCENE_H
// scene.h*
#include "lux.h"
#include "primitive.h"
#include "transport.h"
// Scene Declarations
class COREDLL Scene {
public:
	// Scene Public Methods
	void Render();
	Scene(Camera *c, SurfaceIntegrator *in,
		VolumeIntegrator *vi, Sampler *s,
		Primitive *accel, const vector<Light *> &lts,
		VolumeRegion *vr);
	~Scene();
	bool Intersect(const Ray &ray, Intersection *isect) const {
		return aggregate->Intersect(ray, isect);
	}
	bool IntersectP(const Ray &ray) const {
		return aggregate->IntersectP(ray);
	}
	const BBox &WorldBound() const;
	Spectrum Li(const RayDifferential &ray, const Sample *sample,
		float *alpha = NULL) const;
	Spectrum Transmittance(const Ray &ray) const;
	// Scene Data
	Primitive *aggregate;
	vector<Light *> lights;
	Camera *camera;
	VolumeRegion *volumeRegion;
	SurfaceIntegrator *surfaceIntegrator;
	VolumeIntegrator *volumeIntegrator;
	Sampler *sampler;
	BBox bound;
};
#endif // LUX_SCENE_H
