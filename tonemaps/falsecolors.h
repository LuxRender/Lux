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


#ifndef LUX_FALSECOLORS_H
#define LUX_FALSECOLORS_H
// falsecolors.h*
#include "lux.h"
#include "tonemap.h"
#include "luxrays/core/color/color.h"
#include "paramset.h"


namespace lux
{

	enum FalseColorScale {	Scale_STD=0, Scale_LMK, Scale_RED, Scale_WHITE, Scale_YELLOW, Scale_SPEOS };
	enum FalseScaleMethod { Method_Linear = 0, Method_Log, Method_Log3 };


// FalseColorsOp Declarations
class FalseColorsOp : public ToneMap {
public:
	// ColorsOp Public Methods
	FalseColorsOp(float maxy, float miny, FalseScaleMethod method, FalseColorScale color)
	{
		maxY = maxy; 
		minY = miny;
		scaleMethod = method;
		scaleColor = color;
	}
	virtual ~FalseColorsOp() { }
	virtual void Map(vector<XYZColor> &xyz, u_int xRes, u_int yRes, float maxDisplayY) const;
	static ToneMap *CreateToneMap(const ParamSet &ps);

private:
	float maxY;
	float minY;
	FalseScaleMethod scaleMethod;
	FalseColorScale scaleColor;
};


inline RGBColor FalsetoRGB_STD(float falseValue)
{
	RGBColor res;

	int value = luxrays::Clamp(falseValue, 0.f, 1.f) * 1023;

	int i = value / 128; //halfstep
	int v = value % 128;

	if (value == 0) { //for easiest debugging
		res.c[0] = 0;
		res.c[1] = 0;
		res.c[2] = 128;
	} else {
		switch (i) {
		case 0: //Dark blue to blue
			res.c[0] = 0;
			res.c[1] = 0;
			res.c[2] = 128 + v;
			break;
		case 1: // blue to cyan part1
			res.c[0] = 0;
			res.c[1] = v;
			res.c[2] = 255;
			break;
		case 2: // blue to cyan prt2
			res.c[0] = 0;
			res.c[1] = 127 + v;
			res.c[2] = 255;
			break;
		case 3: //cyan to light green
			res.c[0] = v;
			res.c[1] = 255;
			res.c[2] = 255 - v;
			break;
		case 4: //light green to yellow
			res.c[0] = 127 + v;
			res.c[1] = 255;
			res.c[2] = 127 - v;
			break;
		case 5: //yellow to orange
			res.c[0] = 255;
			res.c[1] = 255 - v;
			res.c[2] = 0;
			break;
		case 6: //orange to red
			res.c[0] = 255;
			res.c[1] = 128 - v;
			res.c[2] = 0;
			break;
		case 7: //red to dark red
			res.c[0] = 255 - v;
			res.c[1] = 0;
			res.c[2] = 0;
			break;
		default: //should not be used !
			res.c[0] = 0;
			res.c[1] = 0;
			res.c[2] = 0;
		}
	}
	
	return res / 255.f;
}

inline RGBColor FalsetoRGB_LMK(float falseValue)
{
	RGBColor res;

	int value = luxrays::Clamp(falseValue, 0.f, 1.f) * 1279;

	int i = value / 256;
	int v = value % 256;

	if (value == 0) { //for easiest debugging
		res.c[0] = 0;
		res.c[1] = 0;
		res.c[2] = 0;
	} else {
		switch (i) {
		case 0: //Black to Blue
			res.c[0] = 0;
			res.c[1] = 0;
			res.c[2] = v;
			break;
		case 1: //Blue to Green
			res.c[0] = 0;
			res.c[1] = v;
			res.c[2] = 255 - v;
			break;
		case 2: //Green to Red
			res.c[0] = v;
			res.c[1] = 255 - v;
			res.c[2] = 0;
			break;
		case 3: //Red to Yellow
			res.c[0] = 255;
			res.c[1] = v;
			res.c[2] = 0;
			break;
		case 4: //Yellow to white
			res.c[0] = 255;
			res.c[1] = 255;
			res.c[2] = v;
			break;
		default: //should not be used !
			res.c[0] = 0;
			res.c[1] = 0;
			res.c[2] = 0;
		}
	}
	
	return res / 255.f;
}

inline RGBColor FalsetoRGB_SPEOS(float falseValue)
{
	RGBColor res;

	int value = luxrays::Clamp(falseValue, 0.f, 1.f) * 1535;

	int i = value / 256;
	int v = value % 256;

	if (value == 0) { //for easiest debugging
		res.c[0] = 0;
		res.c[1] = 0;
		res.c[2] = 0;
	} else {
		switch (i) {
		case 0: //Black to blue
			res.c[0] = 0;
			res.c[1] = 0;
			res.c[2] = v;
			break;
		case 1: // blue to cyan
			res.c[0] = 0;
			res.c[1] = v;
			res.c[2] = 255;
			break;
		case 2: //cyan to green
			res.c[0] = 0;
			res.c[1] = 255;
			res.c[2] = 255 - v;
			break;
		case 3: //green to yellow
			res.c[0] = v;
			res.c[1] = 255;
			res.c[2] = 0;
			break;
		case 4: //yellow to red
			res.c[0] = 255;
			res.c[1] = 255 - v;
			res.c[2] = 0;
			break;
		case 5: //red to white
			res.c[0] = 255;
			res.c[1] = v;
			res.c[2] = v;
			break;
		default: //should not be used !
			res.c[0] = 0;
			res.c[1] = 0;
			res.c[2] = 0;
		}
	}
	
	return res / 255.f;
}

inline RGBColor FalsetoRGB_RED(float falseValue)
{
	RGBColor res;

	float value = luxrays::Clamp(falseValue, 0.f, 1.f);

	if (value == 0) { //for easiest debugging
		res.c[0] = 0;
		res.c[1] = 0;
		res.c[2] = 0;
	} else {
		res.c[0] = value;
		res.c[1] = 0;
		res.c[2] = 0;
	}

	return res;
}

inline RGBColor FalsetoRGB_WHITE(float falseValue)
{
	RGBColor res;

	float value = luxrays::Clamp(falseValue, 0.f, 1.f);

	if (value == 0) { //for easiest debugging
		res.c[0] = 0;
		res.c[1] = 0;
		res.c[2] = 0;
	} else {
		res.c[0] = value;
		res.c[1] = value;
		res.c[2] = value;
	}

	return res;
}

inline RGBColor FalsetoRGB_YELLOW(float falseValue)
{
	RGBColor res;

	float value = luxrays::Clamp(falseValue, 0.f, 1.f);

	if (value == 0) { //for easiest debugging
		res.c[0] = 0;
		res.c[1] = 0;
		res.c[2] = 0;
	} else {
		res.c[0] = value;
		res.c[1] = 0.287245165f * value;
		res.c[2] = 0;
	}

	return res;
}

inline float ValueScale(FalseScaleMethod scalemethod, float value)
{
	float result;

	switch (scalemethod) {
	case Method_Linear: //linear
		result = value;
		break;
	case Method_Log: //log
		result = log10f(1.f + 9.f * value); //logarythme curve
		break;
	case Method_Log3: //log3
		result = log10f(1.f + 9.f * value); //logarythme curve
		result = powf(result, 1.f / 3.f); // power of 3
		break;
	default:
		//LOG(LUX_ERROR,LUX_BADTOKEN) << "False color tonemapping method unknown.";
		result = value;
	}
	return result;
}

inline RGBColor ValuetoRGB(FalseColorScale colorscale, float value)
{
	RGBColor color;

	switch (colorscale) {
	case Scale_STD:
		color = FalsetoRGB_STD(value);
		break;
	case Scale_LMK:
		color = FalsetoRGB_LMK(value);
		break;
	case Scale_RED:
		color = FalsetoRGB_RED(value);
		break;
	case Scale_WHITE:
		color = FalsetoRGB_WHITE(value);
		break;
	case Scale_YELLOW:
		color = FalsetoRGB_YELLOW(value);
		break;
	case Scale_SPEOS:
		color = FalsetoRGB_SPEOS(value);
		break;
	default:
		//LOG(LUX_ERROR,LUX_BADTOKEN) << "False color tonemapping color scale unknown.";
		color = RGBColor(value);
	}

	return color;
}


}//namespace lux

#endif // LUX_FALSECOLORS_H
