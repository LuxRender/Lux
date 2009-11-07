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

// distant.cpp*
#include "distant.h"
#include "mc.h"
#include "paramset.h"
#include "dynload.h"

using namespace lux;

// DistantLight Method Definitions
DistantLight::DistantLight(const Transform &light2world,
						   const boost::shared_ptr< Texture<SWCSpectrum> > L, 
						   float g, const Vector &dir)
	: Light(light2world) {
	lightDir = Normalize(LightToWorld(dir));
	Lbase = L;
	Lbase->SetIlluminant();
	gain = g;
}
DistantLight::~DistantLight()
{
}
SWCSpectrum DistantLight::Sample_L(const TsPack *tspack, const Point &p, float u1, float u2, float u3,
		Vector *wi, float *pdf, VisibilityTester *visibility) const {
	*pdf = 1.f;
	*wi = lightDir;
	visibility->SetRay(p, *wi, tspack->time);
	return Lbase->Evaluate(tspack, dummydg) * gain;
}
float DistantLight::Pdf(const TsPack *tspack, const Point &, const Vector &) const {
	return 0.f;
}
float DistantLight::Pdf(const TsPack *tspack, const Point &p, const Normal &N,
	const Point &po, const Normal &ns) const
{
	return 0.f;
}
SWCSpectrum DistantLight::Sample_L(const TsPack *tspack, const Scene *scene,
		float u1, float u2, float u3, float u4,
		Ray *ray, float *pdf) const {
	// Choose point on disk oriented toward infinite light direction
	Point worldCenter;
	float worldRadius;
	scene->WorldBound().BoundingSphere(&worldCenter,
	                                   &worldRadius);
	Vector v1, v2;
	CoordinateSystem(lightDir, &v1, &v2);
	float d1, d2;
	ConcentricSampleDisk(u1, u2, &d1, &d2);
	Point Pdisk =
		worldCenter + worldRadius * (d1 * v1 + d2 * v2);
	// Set ray origin and direction for infinite light ray
	ray->o = Pdisk + worldRadius * lightDir;
	ray->d = -lightDir;
	*pdf = 1.f / (M_PI * worldRadius * worldRadius);
	return Lbase->Evaluate(tspack, dummydg) * gain;
}
Light* DistantLight::CreateLight(const Transform &light2world,
		const ParamSet &paramSet, const TextureParams &tp) {
	boost::shared_ptr<Texture<SWCSpectrum> > L = tp.GetSWCSpectrumTexture("L", RGBColor(1.f));
	float g = paramSet.FindOneFloat("gain", 1.f);
	Point from = paramSet.FindOnePoint("from", Point(0,0,0));
	Point to = paramSet.FindOnePoint("to", Point(0,0,1));
	Vector dir = from-to;
	return new DistantLight(light2world, L, g, dir);
}

static DynamicLoader::RegisterLight<DistantLight> r("distant");

