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

#include "sphericalfunction.h"
#include "sphericalfunction_ies.h"
#include "mipmap.h"
#include "mcdistribution.h"
#include "paramset.h"
#include "imagereader.h"

namespace lux {

	// MipMapSphericalFunction

	MipMapSphericalFunction::MipMapSphericalFunction() {
	}

	MipMapSphericalFunction::MipMapSphericalFunction( boost::shared_ptr< const MIPMap<RGBColor> > aMipMap, bool flipZ ) {
		SetMipMap( aMipMap );
	}

	RGBColor MipMapSphericalFunction::f(float phi, float theta) const {
		return mipMap->Lookup( phi * INV_TWOPI, theta * INV_PI );
	}

// SampleableSphericalFunction
SampleableSphericalFunction::SampleableSphericalFunction(
	boost::shared_ptr<const SphericalFunction> aFunc,
	u_int xRes, u_int yRes)
{
	func = aFunc;

	// Compute scalar-valued image
	float *img = new float[xRes * yRes];
	for (u_int y = 0; y < yRes; ++y) {
		float yp = M_PI * (y + .5f) / yRes;
		for (u_int x = 0; x < xRes; ++x) {
			float xp = 2.f * M_PI * (x + .5f) / xRes;
			img[x + y * xRes] = func->f(xp, yp).Y() * sin(yp);
		}
	}
	// Initialize sampling PDFs
	uvDistrib = new Distribution2D(img, xRes, yRes);
	delete[] img;
}
SampleableSphericalFunction::~SampleableSphericalFunction() {
	delete uvDistrib;
}

RGBColor SampleableSphericalFunction::f(float phi, float theta) const
{
	return func->f(phi, theta);
}

RGBColor SampleableSphericalFunction::Sample_f(float u1, float u2, Vector *w,
	float *pdf) const
{
	// Find floating-point $(u,v)$ sample coordinates
	float uv[2];
	uvDistrib->SampleContinuous(u1, u2, uv, pdf);
	// Convert sample point to direction on the unit sphere
	const float theta = uv[1] * M_PI;
	const float phi = uv[0] * 2.f * M_PI;
	const float costheta = cosf(theta), sintheta = sinf(theta);
	*w = SphericalDirection(sintheta, costheta, phi);
	// Compute PDF for sampled direction
	*pdf /= 2.f * M_PI * M_PI * sintheta;
	// Return value for direction
	return f(phi, theta);
}

float SampleableSphericalFunction::Pdf(const Vector& w) const
{
	float theta = SphericalTheta(w), phi = SphericalPhi(w);
	return uvDistrib->Pdf(phi * INV_TWOPI, theta * INV_PI) /
		(2.f * M_PI * M_PI * sinf(theta));
}

float SampleableSphericalFunction::Average_f() const {
	return uvDistrib->Average();
}

	SphericalFunction *CreateSphericalFunction(const ParamSet &paramSet, const TextureParams &tp) {
		bool flipZ = paramSet.FindOneBool("flipz", false);
		string texname = paramSet.FindOneString("mapname", "");
		string iesname = paramSet.FindOneString("iesname", "");

		// Create _mipmap_ for _PointLight_
		SphericalFunction *mipmapFunc = NULL;
		if( texname.length() > 0 ) {
			auto_ptr<ImageData> imgdata(ReadImage(texname));
			if (imgdata.get()!=NULL) {
				mipmapFunc = new MipMapSphericalFunction(
					boost::shared_ptr< MIPMap<RGBColor> >(imgdata->createMIPMap<RGBColor>()), flipZ
				);
			}
		}
		// Create IES distribution
		SphericalFunction *iesFunc = NULL;
		if( iesname.length() > 0 ) {
			PhotometricDataIES data(iesname.c_str());
			if( data.IsValid() ) {
				iesFunc = new IESSphericalFunction( data, flipZ );
			}
			else {
				stringstream ss;
				ss << "Invalid IES file: " << iesname;
				luxError( LUX_BADFILE, LUX_WARNING, ss.str().c_str() );
			}
		}

		if( !iesFunc && !mipmapFunc )
			return NULL;
		else if( !iesFunc )
			return mipmapFunc;
		else if( !mipmapFunc )
			return iesFunc;
		else {
			CompositeSphericalFunction *compositeFunc = new CompositeSphericalFunction();
			compositeFunc->Add(
				boost::shared_ptr<const SphericalFunction>(mipmapFunc) );
			compositeFunc->Add(
				boost::shared_ptr<const SphericalFunction>(iesFunc) );
			return compositeFunc;
		}
	}

} //namespace lux
