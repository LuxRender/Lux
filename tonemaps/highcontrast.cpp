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

// highcontrast.cpp*
#include "highcontrast.h"
#include "color.h"
#include "stats.h"
#include "dynload.h"

using namespace lux;

// HighContrastOp Method Definitions
void HighContrastOp::Map(vector<XYZColor> &xyz, u_int xRes, u_int yRes,
		float maxDisplayY) const {
	const u_int numPixels = xRes * yRes;
	TextureColor<float, 1> *lum = new TextureColor<float, 1>[numPixels];
	// Find minimum and maximum image luminances
	float minY = INFINITY, maxY = 0.f;
	for (u_int i = 0; i < numPixels; ++i) {
		lum[i].c[0] = xyz[i].Y() * 683.f;
		minY = min(minY, lum[i].c[0]);
		maxY = max(maxY, lum[i].c[0]);
	}
	float CYmin = C(minY), CYmax = C(maxY);
	// Build luminance image pyramid
	MIPMapFastImpl<TextureColor<float, 1> > pyramid(MIPMAP_EWA, xRes, yRes,
		lum, 4.f, TEXTURE_CLAMP);
	delete[] lum;
	// Apply high contrast tone mapping operator
	ProgressReporter progress(xRes * yRes, "Tone Mapping"); // NOBOOK
	for (u_int y = 0; y < yRes; ++y) {
		float yc = (float(y) + .5f) / float(yRes);
		for (u_int x = 0; x < xRes; ++x) {
			float xc = (float(x) + .5f) / float(xRes);
			// Compute local adaptation luminance at $(x,y)$
			float dwidth = 1.f / float(max(xRes, yRes));
			float maxWidth = 32.f / float(max(xRes, yRes));
			float width = dwidth, prevWidth = 0.f;
			float Yadapt;
			float prevlc = 0.f;
			const float maxLocalContrast = .5f;
			while (1) {
				// Compute local contrast at $(x,y)$
				float b0 = pyramid.LookupFloat(CHANNEL_MEAN,
					xc, yc, width, 0.f, 0.f, width);
				float b1 = pyramid.LookupFloat(CHANNEL_MEAN,
					xc, yc, 2.f * width, 0.f, 0.f, 2.f * width);
				float lc = fabsf((b0 - b1) / b0);
				// If maximum contrast is exceeded, compute adaptation luminance
				if (lc > maxLocalContrast) {
					float t = (maxLocalContrast - prevlc) / (lc - prevlc);
					float w = Lerp(t, prevWidth, width);
					Yadapt = pyramid.LookupFloat(CHANNEL_MEAN,
						xc, yc, w, 0.f, 0.f, w);
					break;
				}
				// Increase search region and prepare to compute contrast again
				prevlc = lc;
				prevWidth = width;
				width += dwidth;
				if (width >= maxWidth) {
					Yadapt = pyramid.LookupFloat(CHANNEL_MEAN,
						xc, yc, maxWidth, 0.f, 0.f, maxWidth);
					break;
				}
			}
			// Apply tone mapping based on local adaptation luminance
			xyz[x + y * xRes] *= T(Yadapt, CYmin, CYmax) * 683.f /
				Yadapt;
		}
		progress.Update(xRes); // NOBOOK
	}
	progress.Done(); // NOBOOK
}
ToneMap * HighContrastOp::CreateToneMap(const ParamSet &ps) {
	return new HighContrastOp;
}

static DynamicLoader::RegisterToneMap<HighContrastOp> r("highcontrast");
