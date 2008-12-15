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

// TODO - Port SPD interfaces

// goniometric.cpp*
#include "goniometric.h"
#include "imagereader.h"
#include "mc.h"
#include "paramset.h"
#include "dynload.h"

using namespace lux;

// GonioPhotometricLight Method Definitions
GonioPhotometricLight::GonioPhotometricLight(
		const Transform &light2world,
		const RGBColor &intensity, const string &texname)
	: Light(light2world) {
	lightPos = LightToWorld(Point(0,0,0));
	Intensity = intensity;
	// Create _mipmap_ for _GonioPhotometricLight_
	auto_ptr<ImageData> imgdata(ReadImage(texname));
	if (imgdata.get()!=NULL) {
		mipmap = imgdata->createMIPMap<RGBColor>();
	}
	else 
		mipmap = NULL;
}
SWCSpectrum GonioPhotometricLight::Sample_L(const TsPack *tspack, const Point &P, float u1, float u2,
		float u3, Vector *wo, float *pdf,
		VisibilityTester *visibility) const {
	*wo = Normalize(lightPos - P);
	*pdf = 1.f;
	visibility->SetSegment(P, lightPos, tspack->time);
	return SWCSpectrum(tspack, Intensity * Scale(-*wo) / DistanceSquared(lightPos, P));
}
SWCSpectrum GonioPhotometricLight::Sample_L(const TsPack *tspack, const Scene *scene, float u1, float u2,
		float u3, float u4, Ray *ray, float *pdf) const {
	ray->o = lightPos;
	ray->d = UniformSampleSphere(u1, u2);
	*pdf = UniformSpherePdf();
	return SWCSpectrum(tspack, Intensity * Scale(ray->d));
}
float GonioPhotometricLight::Pdf(const Point &, const Vector &) const {
	return 0.;
}
Light* GonioPhotometricLight::CreateLight(const Transform &light2world,
		const ParamSet &paramSet) {
	RGBColor I = paramSet.FindOneRGBColor("I", RGBColor(1.0));
	string texname = paramSet.FindOneString("mapname", "");
	return new GonioPhotometricLight(light2world, I, texname);
}

static DynamicLoader::RegisterLight<GonioPhotometricLight> r("goniometric");

