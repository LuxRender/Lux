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

// projection.cpp*
#include "projection.h"
#include "imagereader.h"
#include "mc.h"
#include "paramset.h"

using namespace lux;

// ProjectionLight Method Definitions
ProjectionLight::
	ProjectionLight(const Transform &light2world,
		const Spectrum &intensity, const string &texname,
		float fov)
	: Light(light2world) {
	lightPos = LightToWorld(Point(0,0,0));
	Intensity = intensity;
	// Create _ProjectionLight_ MIP-map
	int width = 0, height = 0;
	auto_ptr<ImageData> imgdata(ReadImage(texname));
	if (imgdata.get()!=NULL) {
		width=imgdata->getWidth();
		height=imgdata->getHeight();
		projectionMap =imgdata->createMIPMap<Spectrum>();
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
	hither = RAY_EPSILON;
	yon = 1e30f;
	lightProjection = Perspective(fov, hither, yon);
	// Compute cosine of cone surrounding projection directions
	float opposite = tanf(Radians(fov) / 2.f);
	float tanDiag = opposite * sqrtf(1.f + 1.f/(aspect*aspect));
	cosTotalWidth = cosf(atanf(tanDiag));
}
ProjectionLight::~ProjectionLight() { delete projectionMap; }
SWCSpectrum ProjectionLight::Sample_L(const Point &p, Vector *wi,
	 	VisibilityTester *visibility) const {
	*wi = Normalize(lightPos - p);
	visibility->SetSegment(p, lightPos);
	return Intensity * Projection(-*wi) /
		DistanceSquared(lightPos, p);
}
Spectrum ProjectionLight::Projection(const Vector &w) const {
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
SWCSpectrum ProjectionLight::Sample_L(const Point &p, float u1, float u2,
		Vector *wi, float *pdf,
		VisibilityTester *visibility) const {
	*wi = Normalize(lightPos - p);
	*pdf = 1.f;
	visibility->SetSegment(p, lightPos);
	return Intensity * Projection(-*wi) / DistanceSquared(lightPos, p);
}
SWCSpectrum ProjectionLight::Sample_L(const Scene *scene, float u1, float u2,
		float u3, float u4, Ray *ray, float *pdf) const {
	ray->o = lightPos;
	Vector v = UniformSampleCone(u1, u2, cosTotalWidth);
	ray->d = LightToWorld(v);
	*pdf = UniformConePdf(cosTotalWidth);
	return Intensity * Projection(ray->d);
}
float ProjectionLight::Pdf(const Point &, const Vector &) const {
	return 0.;
}
Light* ProjectionLight::CreateLight(const Transform &light2world,
		const ParamSet &paramSet) {
	Spectrum I = paramSet.FindOneSpectrum("I", Spectrum(1.0));
	float fov = paramSet.FindOneFloat("fov", 45.);
	string texname = paramSet.FindOneString("mapname", "");
	return new ProjectionLight(light2world, I, texname, fov);
}
