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

// film.cpp*
#include "film.h"
#include "dynload.h"
#include "paramset.h"
#include "tonemap.h"
#include "stats.h"

namespace lux
{

// Image Pipeline Function Definitions
void ApplyImagingPipeline(vector<Color> &pixels,
	int xResolution, int yResolution, ColorSystem &colorSpace,
	float bloomRadius, float bloomWeight,
	const char *toneMapName, const ParamSet *toneMapParams,
	float gamma, float dither)
{
	const int nPix = xResolution * yResolution ;
	// Possibly apply bloom effect to image
	if (bloomRadius > 0.f && bloomWeight > 0.f) {
		// Compute image-space extent of bloom effect
		const int bloomSupport = Float2Int(bloomRadius *
			max(xResolution, yResolution));
		const int bloomWidth = bloomSupport / 2;
		// Initialize bloom filter table
		vector<float> bloomFilter(bloomWidth * bloomWidth);
		for (int i = 0; i < bloomWidth * bloomWidth; ++i) {
			float dist = sqrtf(float(i)) / float(bloomWidth);
			bloomFilter[i] = powf(max(0.f, 1.f - dist), 4.f);
		}
		// Apply bloom filter to image pixels
		vector<Color> bloomImage(nPix);
		ProgressReporter prog(yResolution, "Bloom filter"); //NOBOOK
		for (int y = 0; y < yResolution; ++y) {
			for (int x = 0; x < xResolution; ++x) {
				// Compute bloom for pixel _(x,y)_
				// Compute extent of pixels contributing bloom
				int x0 = max(0, x - bloomWidth);
				int x1 = min(x + bloomWidth, xResolution - 1);
				int y0 = max(0, y - bloomWidth);
				int y1 = min(y + bloomWidth, yResolution - 1);
				int offset = y * xResolution + x;
				float sumWt = 0.;
				for (int by = y0; by <= y1; ++by) {
					for (int bx = x0; bx <= x1; ++bx) {
						// Accumulate bloom from pixel $(bx,by)$
						int dx = x - bx, dy = y - by;
						if (dx == 0 && dy == 0) continue;
						int dist2 = dx*dx + dy*dy;
						if (dist2 < bloomWidth * bloomWidth) {
							int bloomOffset = bx + by * xResolution;
							float wt = bloomFilter[dist2];
							sumWt += wt;
							bloomImage[offset].AddWeighted(wt, pixels[bloomOffset]);
						}
					}
				}
				bloomImage[offset] /= sumWt;
			}
			prog.Update(); //NOBOOK
		}
		prog.Done(); //NOBOOK
		// Mix bloom effect into each pixel
		for (int i = 0; i < nPix; ++i)
			pixels[i] = Lerp(bloomWeight, pixels[i], bloomImage[i]);
	}
	// Apply tone reproduction to image
	ToneMap *toneMap = NULL;
	if (toneMapName) {
		toneMap = MakeToneMap(toneMapName,
			toneMapParams ? *toneMapParams : ParamSet());
		if (toneMap) {
			toneMap->Map(pixels, xResolution, yResolution, 100.f);
			delete toneMap;
		}
	}
	// Convert to RGB
	const float invGamma = 1.f / gamma;
	for (int i = 0; i < nPix; ++i) {
		pixels[i] = colorSpace.ToRGBConstrained(XYZColor(pixels[i].c));
		// Do gamma correction
		pixels[i] = pixels[i].Pow(invGamma);
	}
	// Dither image
	if (dither > 0.f)
		for (int i = 0; i < nPix; ++i)
			pixels[i] += 2.f * dither * (lux::random::floatValueP() - .5f);
}

}//namespace lux

