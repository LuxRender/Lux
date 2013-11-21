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

// colors.cpp*
#include "falsecolors.h"
#include "dynload.h"
#include "error.h"

using namespace lux;
using namespace luxrays;

void FalseColorsOp::Map(vector<XYZColor> &xyz, u_int xRes, u_int yRes, float maxDisplayY) const {
		//ColorSystem cs(0.63, 0.34, 0.31, 0.595, 0.155, 0.07, 0.314275, 0.329411, 1);
		ColorSystem cs(0.64, 0.33, 0.21, 0.71, 0.15, 0.06, 0.3127, 0.3290, 1); //adobeRGB

		const u_int numPixels = xRes * yRes;

		float luminance;
		RGBColor vcolor(0.f);
		for (u_int i = 0; i < numPixels; ++i) {
			luminance =  Clamp(xyz[i].Y(), minY, maxY);
			luminance = (luminance - minY) / (maxY - minY); //normalisation

			vcolor = ValuetoRGB(scaleColor, ValueScale(scaleMethod, luminance));
			xyz[i] = cs.ToXYZ(vcolor);
		}
}

// ColorsOp Method Definitions
ToneMap * FalseColorsOp::CreateToneMap(const ParamSet &ps) {
	float maxy = ps.FindOneFloat("max", 1.f);
	float miny = ps.FindOneFloat("min", 0.f);

	FalseScaleMethod method = Method_Linear;
	string falsemethodStr = ps.FindOneString("method", "linear");
	if (falsemethodStr == "linear")
		method = Method_Linear;
	else if (falsemethodStr == "log")
		method = Method_Log;
	else if (falsemethodStr == "log3")
		method = Method_Log3;
	else {
		LOG(LUX_WARNING,LUX_BADTOKEN) << "False color scaling method  '" << falsemethodStr << "' unknown. Using \"linear\".";
		method = Method_Linear;
	}

	FalseColorScale color = Scale_STD;
	string falsecolorStr = ps.FindOneString("colorscale", "std");
	if (falsecolorStr == "std")
		color = Scale_STD;
	else if (falsecolorStr == "lmk")
		color = Scale_LMK;
	else if (falsecolorStr == "red")
		color = Scale_RED;
	else if (falsecolorStr == "white")
		color = Scale_WHITE;
	else if (falsecolorStr == "yellow")
		color = Scale_YELLOW;
	else if (falsecolorStr == "speos")
		color = Scale_SPEOS;
	else {
		LOG(LUX_WARNING,LUX_BADTOKEN) << "False color scale  '" << falsecolorStr << "' unknown. Using \"std\".";
		color = Scale_STD;
	}

	return new FalseColorsOp(maxy, miny, method, color);
}

static DynamicLoader::RegisterToneMap<FalseColorsOp> r("falsecolors");
