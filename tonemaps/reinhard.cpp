/***************************************************************************
 *   Copyright (C) 1998-2007 by authors (see AUTHORS.txt )                 *
 *                                                                         *
 *   This file is part of Lux Renderer.                                    *
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
 *   Lux Renderer website : http://www.luxrender.org                       *
 ***************************************************************************/

/*
 * Reinhard Tonemapping (Indigo compatible) class
 *
 * Uses Indigo compatible parameters.
 * Adapted from Indigo Reinhard Tonemapper from Violet GPL sources.
 *
 * 30/09/07 - Radiance - Initial Version
 */

#include "reinhard.h"

// ReinhardOp Method Definitions
ReinhardOp::ReinhardOp(float prS, float poS, float b)
{
	pre_scale = prS;
	post_scale = poS;
	burn = b;
}

void ReinhardOp::Map(const float *y, int xRes, int yRes,
		float maxDisplayY, float *scale) const
{
	const float alpha = 0.1f;
	float Ywa = 0.;
	for (int i = 0; i < xRes * yRes; ++i)
		Ywa += y[i];
	const float av_lum = Ywa / (float) (xRes * yRes);
	const float iscale = pre_scale * alpha / av_lum;

	// find maximum luminance
	float maxY = 0.0f;
	for (int i = 0; i < xRes * yRes; ++i)
		if( maxY < y[i] )
			maxY = y[i];

	if(maxY == 0.0f) return;

	// use linear tonemapping if post_scale = 0
	if (post_scale == 0) 
	{
		for (int i = 0; i < xRes * yRes; ++i)
			scale[i] = pre_scale / av_lum;
		return;
	} 

	const float Y_white = pre_scale * alpha * burn;
	const float recip_Y_white2 = 1.0f / (Y_white * Y_white);

	// do tonemap
	for (int i = 0; i < xRes * yRes; ++i) {
		float lum = iscale * y[i];
		scale[i] = ( post_scale * (1.0f + lum*recip_Y_white2) / (1.0f + lum) ); //* maxDisplayY;
	}
}
ToneMap * ReinhardOp::CreateToneMap(const ParamSet &ps) {
	float pre_scale = ps.FindOneFloat("prescale", 1.f);
	float post_scale = ps.FindOneFloat("postscale", 1.f);
	float burn = ps.FindOneFloat("burn", 7.f);
	return new ReinhardOp(pre_scale, post_scale, burn);
}
