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

// lineartonemap.h*
#include "lux.h"
#include "tonemap.h"
#include "color.h"
#include "context.h"
// #include <math.h>

namespace lux
{

// EVOp Declarations
class EVOp : public ToneMap {
public:
	// EVOp Public Methods
	// Applies a linear factor to the image, determined by the raw film's EV
	EVOp() { }
	virtual ~EVOp() { }
	
	virtual void Map(vector<XYZColor> &xyz, u_int xRes, u_int yRes, float maxDisplayY) const {
		// read data from film
		const float gamma = luxGetParameterValue(LUX_FILM, LUX_FILM_TORGB_GAMMA);
		const float Y = luxGetFloatAttribute("film", "averageLuminance");
		
		if (Y <= 0.f)
			return;

		/*
		(fstop * fstop) / exposure = Y*sensitivity/K

		take K = 12.5

		(fstop * fstop) / exposure = Y * sensitivity / 12.5

		exposure = 12.5*(fstop * fstop) / Y * sensitivity

		*/

		// linear tonemap operation
		//float factor = (exposure / (fstop * fstop) * sensitivity / 10.f * powf(118.f / 255.f, gamma));
		
		// substitute exposure, fstop and sensitivity cancel out; collect constants
		const float factor = (1.25f / Y * powf(118.f / 255.f, gamma));

		const u_int numPixels = xRes * yRes;
		for (u_int i = 0; i < numPixels; ++i)
			xyz[i] *= factor;
	}
	
	static ToneMap *CreateToneMap(const ParamSet &ps);
private:
};


// LinearOp Declarations
class LinearOp : public ToneMap {
public:
	// LinearOp Public Methods
	// The exposure index is given by:
	// H = q * L * t / N^2
	// where q is a filtering/conversion factor from cd.m-2 to lx,
	// L is the luminance in cd.m-2, t is the exposure duration in s,
	// N is the f-stop
	// The following relations determine the output:
	// Hsos = 10 / Ssos and Hsos maps to 118 in [0-255] at gamma=2.2
	// where Ssos is the ISO speed
	// q is set at 0.65 (should come from camera), so the final formula is:
	// R = L * 0.65 * t / N^2 * (118 / 255)^2.2 / (10 / Ssos)
	// This is all taken from the ISO speed article of wikipedia
	LinearOp(float sensitivity, float exposure, float fstop, float gamma) :
		factor(exposure / (fstop * fstop) * sensitivity * 0.65f / 10.f * powf(118.f / 255.f, gamma)) { }
	virtual ~LinearOp() { }
	virtual void Map(vector<XYZColor> &xyz, u_int xRes, u_int yRes, float maxDisplayY) const {
		const u_int numPixels = xRes * yRes;
		for (u_int i = 0; i < numPixels; ++i)
			xyz[i] *= factor;
	}
	
	static ToneMap *CreateToneMap(const ParamSet &ps);
private:
	// LinearOp Private Data
	const float factor;
};

}//namespace lux

