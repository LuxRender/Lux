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

#include "sphericalfunction.h"
#include "mipmap.h"
#include "mcdistribution.h"

namespace lux {

	// MipMapSphericalFunction

	MipMapSphericalFunction::MipMapSphericalFunction() {
	}

	MipMapSphericalFunction::MipMapSphericalFunction( boost::shared_ptr< const MIPMap<RGBColor> > aMipMap ) {
		SetMipMap( aMipMap );
	}

	RGBColor MipMapSphericalFunction::f(float phi, float theta) const {
		return mipMap->Lookup( phi * INV_TWOPI, theta * INV_PI );
	}

	// SampleableSphericalFunction

	SampleableSphericalFunction::SampleableSphericalFunction(
		boost::shared_ptr<const SphericalFunction> aFunc, int xRes, int yRes)
	{
		func = aFunc;
		nVDistribs = xRes;
		
		// Compute scalar-valued image
		float *img = new float[xRes*yRes];
		for (int x = 0; x < xRes; ++x) {
			float xp = (float)(x + .5f) / (float)xRes;
			for (int y = 0; y < yRes; ++y) {
				float yp = (float)(y + .5f) / (float)yRes;
				img[y+x*yRes] = func->f(xp * 2.f * M_PI, yp * M_PI).y();
			}
		}
		// Initialize sampling PDFs
		int nu = xRes, nv = yRes;
		float *func = (float *)alloca(max(nu, nv) * sizeof(float));
		float *sinVals = (float *)alloca(nv * sizeof(float));
		for (int i = 0; i < nv; ++i)
			sinVals[i] = sin(M_PI * float(i + .5f)/float(nv));
		vDistribs = new Distribution1D *[nu];
		for (int u = 0; u < nu; ++u) {
			// Compute sampling distribution for column _u_
			for (int v = 0; v < nv; ++v)
				func[v] = img[u*nv+v] *= sinVals[v];
			vDistribs[u] = new Distribution1D(func, nv);
		}
		// Compute sampling distribution for columns of image
		for (int u = 0; u < nu; ++u)
			func[u] = vDistribs[u]->funcInt;
		uDistrib = new Distribution1D(func, nu);
		delete[] img;
	}
	SampleableSphericalFunction::~SampleableSphericalFunction() {
		delete uDistrib;
		for(int i = 0; i < nVDistribs; i++) {
			delete vDistribs[i];
		}
		delete[] vDistribs;
	}

	RGBColor SampleableSphericalFunction::f(float phi, float theta) const {
		return func->f(phi, theta);
	}

	RGBColor SampleableSphericalFunction::Sample_f(float u1, float u2, Vector *w, float *pdf) const {
		// Find floating-point $(u,v)$ sample coordinates
		float pdfs[2];
		float fu = uDistrib->Sample(u1, &pdfs[0]);
		int u = Float2Int(fu);
		float fv = vDistribs[u]->Sample(u2, &pdfs[1]);
		// Convert sample point to direction on the unit sphere
		float theta = fv * vDistribs[u]->invCount * M_PI;
		float phi = fu * uDistrib->invCount * 2.f * M_PI;
		float costheta = cos(theta), sintheta = sin(theta);
		float sinphi = sin(phi), cosphi = cos(phi);
		*w = Vector(sintheta * cosphi, sintheta * sinphi, costheta);
		// Compute PDF for sampled direction
		*pdf = (pdfs[0] * pdfs[1]) / (2.f * M_PI * M_PI * sintheta);
		// Return value for direction
		return f(phi, theta);
	}

	float SampleableSphericalFunction::Average_f() const {
		return uDistrib->funcInt;
	}

} //namespace lux
