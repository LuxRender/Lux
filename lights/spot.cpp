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

// spot.cpp*
#include "spot.h"
#include "mc.h"
#include "spd.h"
#include "rgbillum.h"
#include "paramset.h"
#include "dynload.h"

using namespace lux;

// SpotLight Method Definitions
SpotLight::SpotLight(const Transform &light2world,
		const RGBColor &intensity, float gain, float width, float fall)
	: Light(light2world) {
	lightPos = LightToWorld(Point(0,0,0));

	// Create SPD
	LSPD = new RGBIllumSPD(intensity);
	LSPD->Scale(gain);

	cosTotalWidth = cosf(Radians(width));
	cosFalloffStart = cosf(Radians(fall));
}
SpotLight::~SpotLight()
{
	delete LSPD;
}
float SpotLight::Falloff(const Vector &w) const {
	Vector wl = Normalize(WorldToLight(w));
	float costheta = wl.z;
	if (costheta < cosTotalWidth)
		return 0.;
 	if (costheta > cosFalloffStart)
		return 1.;
	// Compute falloff inside spotlight cone
	float delta = (costheta - cosTotalWidth) /
		(cosFalloffStart - cosTotalWidth);
	return delta*delta*delta*delta;
}
SWCSpectrum SpotLight::Sample_L(const TsPack *tspack, const Point &p, float u1, float u2, float u3,
		Vector *wi, float *pdf, VisibilityTester *visibility) const {
	*pdf = 1.f;
	*wi = Normalize(lightPos - p);
	visibility->SetSegment(p, lightPos);
	return SWCSpectrum(tspack, LSPD) * Falloff(-*wi) /
		DistanceSquared(lightPos, p);
}
float SpotLight::Pdf(const Point &, const Vector &) const {
	return 0.;
}
SWCSpectrum SpotLight::Sample_L(const TsPack *tspack, const Scene *scene, float u1,
		float u2, float u3, float u4,
		Ray *ray, float *pdf) const {
	ray->o = lightPos;
	Vector v = UniformSampleCone(u1, u2, cosTotalWidth);
	ray->d = LightToWorld(v);
	*pdf = UniformConePdf(cosTotalWidth);
	return SWCSpectrum(tspack, LSPD) * Falloff(ray->d);
}
Light* SpotLight::CreateLight(const Transform &l2w, const ParamSet &paramSet) {
	RGBColor I = paramSet.FindOneRGBColor("I", RGBColor(1.0));
	float g = paramSet.FindOneFloat("gain", 1.f);
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
	Transform light2world =
	l2w *
	Translate(Vector(from.x, from.y, from.z)) *
	dirToZ.GetInverse();
	return new SpotLight(light2world, I, g, coneangle,
		coneangle-conedelta);
}

static DynamicLoader::RegisterLight<SpotLight> r("spot");

