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

// goniometric.cpp*
#include "goniometric.h"
#include "imagereader.h"
#include "mc.h"
#include "paramset.h"
#include "dynload.h"
#include "exrio.h"
#include "spd.h"
#include "rgbillum.h"
#include "sphericalfunction_ies.h"

using namespace lux;

// GonioPhotometricLight Method Definitions
GonioPhotometricLight::GonioPhotometricLight(
		const Transform &light2world,
		const RGBColor &intensity,
		float gain,
		const string &texname,
		const string& iesname)
	: Light(light2world) {
	lightPos = LightToWorld(Point(0,0,0));
	// Create SPD
	LSPD = new RGBIllumSPD(intensity);
	LSPD->Scale(gain);
	// Create _mipmap_ for _GonioPhotometricLight_
	SphericalFunction *mipmapFunc = NULL;
	if( texname.length() > 0 ) {
		auto_ptr<ImageData> imgdata(ReadImage(texname));
		if (imgdata.get()!=NULL) {
			mipmapFunc = new MipMapSphericalFunction(
				boost::shared_ptr< MIPMap<RGBColor> >(imgdata->createMIPMap<RGBColor>())
			);
		}
	}
	// Create IES distribution
	SphericalFunction *iesFunc = NULL;
	if( iesname.length() > 0 ) {
		PhotometricDataIES data(iesname.c_str());
		if( data.IsValid() ) {
			iesFunc = new IESSphericalFunction( data );
		}
		else {
			stringstream ss;
			ss << "Invalid IES file: " << iesname;
			luxError( LUX_BADFILE, LUX_WARNING, ss.str().c_str() );
		}
	}
	SphericalFunction *distrSimple;
	if( !iesFunc && !mipmapFunc )
		distrSimple = new NoopSphericalFunction();
	else if( !iesFunc )
		distrSimple = mipmapFunc;
	else if( !mipmapFunc )
		distrSimple = iesFunc;
	else {
		CompositeSphericalFunction *compositeFunc = new CompositeSphericalFunction();
		compositeFunc->Add(
			boost::shared_ptr<const SphericalFunction>(mipmapFunc) );
		compositeFunc->Add(
			boost::shared_ptr<const SphericalFunction>(iesFunc) );
		distrSimple = compositeFunc;
	}
	func = new SampleableSphericalFunction(
		boost::shared_ptr<const SphericalFunction>(distrSimple),
		512, 256
	);

	/*float* data = new float[512*256*3];
	float* dataA = new float[512*256];
	for(int j=0; j<256; j++) {
		for(int i=0; i<512; i++) {
			RGBColor val = distr->f(
				((i + .5f) * 2.f * M_PI)/512.f,
				((j + .5f) * M_PI)/256.f
			);
			data[3 * (j*512 + i) + 0] = val.c[0];
			data[3 * (j*512 + i) + 1] = val.c[1];
			data[3 * (j*512 + i) + 2] = val.c[2];
			dataA[j*512 + i] = 1.f;
		}
	}
	WriteRGBAImage("ies_image.exr", data, dataA, 512, 256, 512, 256, 0, 0);*/
}
GonioPhotometricLight::~GonioPhotometricLight() {
	delete LSPD;
	delete func;
}
SWCSpectrum GonioPhotometricLight::Power(const TsPack *tspack, const Scene *) const {
	return SWCSpectrum(tspack, LSPD) * 4.f * M_PI * func->Average_f();
}
SWCSpectrum GonioPhotometricLight::Sample_L(const TsPack *tspack, const Point &P, float u1, float u2,
		float u3, Vector *wo, float *pdf,
		VisibilityTester *visibility) const {
	*wo = Normalize(lightPos - P);
	*pdf = 1.f;
	visibility->SetSegment(P, lightPos, tspack->time);
	return L(tspack, Normalize(WorldToLight(-*wo))) / DistanceSquared(lightPos, P);
}
SWCSpectrum GonioPhotometricLight::Sample_L(const TsPack *tspack, const Scene *scene, float u1, float u2,
		float u3, float u4, Ray *ray, float *pdf) const {
	ray->o = lightPos;
	Vector w;
	RGBColor f = func->Sample_f(u1, u2, &w, pdf);
	ray->d = Normalize(LightToWorld(w));
	return SWCSpectrum(tspack, LSPD) * SWCSpectrum(tspack, f);
}
float GonioPhotometricLight::Pdf(const Point &, const Vector &) const {
	return 0.;
}
SWCSpectrum GonioPhotometricLight::L(const TsPack *tspack, const Vector &w) const {
	return SWCSpectrum(tspack, LSPD) * SWCSpectrum(tspack, func->f(w));
}
Light* GonioPhotometricLight::CreateLight(const Transform &light2world,
		const ParamSet &paramSet) {
	RGBColor I = paramSet.FindOneRGBColor("I", RGBColor(1.0));
	float g = paramSet.FindOneFloat("gain", 1.f);
	string texname = paramSet.FindOneString("mapname", "");
	string iesname = paramSet.FindOneString("iesname", "");
	Point P = paramSet.FindOnePoint("from", Point(0,0,0));
	Transform l2w = Translate(Vector(P.x, P.y, P.z)) * light2world;
	return new GonioPhotometricLight(l2w, I, g, texname, iesname);
}

static DynamicLoader::RegisterLight<GonioPhotometricLight> r("goniometric");

