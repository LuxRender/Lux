/***************************************************************************
 *   Copyright (C) 1998-2013 by authors (see AUTHORS.txt)                  *
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
using luxrays::SphericalDirection;
#include "sphericalfunction_ies.h"
#include "mipmap.h"
#include "paramset.h"
#include "imagereader.h"
#include "spectrumwavelengths.h"
#include "luxrays/utils/mcdistribution.h"

using namespace luxrays;

namespace lux {

// MipMapSphericalFunction

MipMapSphericalFunction::MipMapSphericalFunction()
{
}

MipMapSphericalFunction::MipMapSphericalFunction(boost::shared_ptr<const MIPMap> &aMipMap, bool flipZ )
{
	SetMipMap(aMipMap);
}

SWCSpectrum MipMapSphericalFunction::f(const SpectrumWavelengths &sw,
	float phi, float theta) const
{
	return mipMap->LookupSpectrum(sw, phi * INV_TWOPI, theta * INV_PI);
}

// SampleableSphericalFunction
SampleableSphericalFunction::SampleableSphericalFunction(
	boost::shared_ptr<const SphericalFunction> &aFunc,
	u_int xRes, u_int yRes) : func(aFunc)
{
	// Compute scalar-valued image
	SpectrumWavelengths swl;
	swl.Sample(.5f);
	float *img = new float[xRes * yRes];
	average = 0.f;
	float normalize = 0.f;
	for (u_int y = 0; y < yRes; ++y) {
		const float yp = M_PI * (y + .5f) / yRes;
		const float weight = sinf(yp);
		normalize += xRes * weight;
		for (u_int x = 0; x < xRes; ++x) {
			const float xp = 2.f * M_PI * (x + .5f) / xRes;
			const float value = func->f(swl, xp, yp).Filter(swl) *
				weight;
			average += value;
			img[x + y * xRes] = value;
		}
	}
	average *= 4.f * M_PI / normalize;
	// Initialize sampling PDFs
	uvDistrib = new Distribution2D(img, xRes, yRes);
	delete[] img;
}
SampleableSphericalFunction::~SampleableSphericalFunction() {
	delete uvDistrib;
}

SWCSpectrum SampleableSphericalFunction::f(const SpectrumWavelengths &sw,
	float phi, float theta) const
{
	return func->f(sw, phi, theta);
}

SWCSpectrum SampleableSphericalFunction::SampleF(const SpectrumWavelengths &sw,
	float u1, float u2, Vector *w, float *pdf) const
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
	return f(sw, phi, theta) / *pdf;
}

float SampleableSphericalFunction::Pdf(const Vector& w) const
{
	float theta = SphericalTheta(w), phi = SphericalPhi(w);
	return uvDistrib->Pdf(phi * INV_TWOPI, theta * INV_PI) /
		(2.f * M_PI * M_PI * sinf(theta));
}

float SampleableSphericalFunction::Average_f() const
{
	return average;
}

SphericalFunction *CreateSphericalFunction(const ParamSet &paramSet)
{
	bool flipZ = paramSet.FindOneBool("flipz", false);
	const string texname = paramSet.FindOneString("mapname", "");
	const string iesname = AdjustFilename(paramSet.FindOneString("iesname", ""));

	// Create _mipmap_ for _PointLight_
	SphericalFunction *mipmapFunc = NULL;
	if (texname.length() > 0) {
		std::auto_ptr<ImageData> imgdata(ReadImage(texname));
		if (imgdata.get() != NULL) {
			boost::shared_ptr<const MIPMap> mm(imgdata->createMIPMap());
			mipmapFunc = new MipMapSphericalFunction(mm, flipZ);
		}
	}
	// Create IES distribution
	SphericalFunction *iesFunc = NULL;
	if (iesname.length() > 0) {
		PhotometricDataIES data(iesname.c_str());
		if (data.IsValid()) {
			iesFunc = new IESSphericalFunction(data, flipZ);
		} else {
			LOG(LUX_WARNING, LUX_BADFILE) <<
				"Invalid IES file: " << iesname;
		}
	}

	if (iesFunc && mipmapFunc) {
		CompositeSphericalFunction *compositeFunc =
			new CompositeSphericalFunction();
		boost::shared_ptr<const SphericalFunction> mipmap(mipmapFunc);
		compositeFunc->Add(mipmap);
		boost::shared_ptr<const SphericalFunction> ies(iesFunc);
		compositeFunc->Add(ies);
		return compositeFunc;
	} else if (mipmapFunc)
		return mipmapFunc;
	else if (iesFunc)
		return iesFunc;
	else
		return NULL;
}

} //namespace lux
