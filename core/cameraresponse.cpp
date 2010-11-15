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

// cameraresponse.cpp*
// Original code by Daniel90
#include "cameraresponse.h"
#include "color.h"
#include "error.h"

#include <boost/algorithm/string.hpp> // contains split()
#include <boost/lexical_cast.hpp> // used to convert string to float
#include <iostream>
#include <fstream>

using namespace lux;

namespace lux {

void AdjustGamma(const vector<float> &from, vector<float> &to, float gamma = 2.2f) {
	for (int i = 0; i < from.size(); i++) {
		to[i] = powf(to[i], gamma);
	}
}

CameraResponse::CameraResponse(const string &film)
{
	fileName = film;
	color = true;
	std::ifstream file(film.c_str());
	if (!file) {
		LOG(LUX_WARNING, LUX_NOFILE) << "Film file \"" << film << "\" not found! Camera Response Function will not be applied.";
		validFile = false;
	}
	else {
		int row = 0;
		string buffer;
		vector<string> crfstrings;
		vector<float> *crfpoints = NULL;
		while (getline(file, buffer)) {
			if (buffer[buffer.length()-1] == '\r')
				buffer = buffer.substr(0, buffer.length()-1);
			++row;
			switch (row) {
			case 4:
				crfpoints = &RedI;
				break;
			case 6:
				crfpoints = &RedB;
				break;
			case 10:
				crfpoints = &GreenI;
				break;
			case 12:
				crfpoints = &GreenB;
				break;
			case 16:
				crfpoints = &BlueI;
				break;
			case 18:
				crfpoints = &BlueB;
				break;
			default:
				crfpoints = NULL;
				break;
			}
			if (!crfpoints)
				continue;
			boost::split(crfstrings, buffer, boost::is_space(),
				boost::token_compress_on);
			for (vector<string>::size_type j = 0; j != crfstrings.size(); ++j)
				crfpoints->push_back(boost::lexical_cast<float>(crfstrings[j]));
		}
		if (row < 18)
			color = false;
		if (RedI.empty() || RedB.size() != RedI.size()) {
			LOG(LUX_WARNING, LUX_LIMIT) << "Inconsistent Red data for \"" << film << "\"";
			RedI.clear();
			RedI.push_back(0.f);
			RedI.push_back(1.f);
			RedB.clear();
			RedB.push_back(0.f);
			RedB.push_back(1.f);
		}
		if (color) {
			if (GreenI.empty() || GreenB.size() != GreenI.size()) {
				LOG(LUX_WARNING, LUX_LIMIT) << "Inconsistent Green data for \"" << film << "\"";
				GreenI.clear();
				GreenI.push_back(0.f);
				GreenI.push_back(1.f);
				GreenB.clear();
				GreenB.push_back(0.f);
				GreenB.push_back(1.f);
			}
			if (BlueI.empty() || BlueB.size() != BlueI.size()) {
				LOG(LUX_WARNING, LUX_LIMIT) << "Inconsistent Blue data for \"" << film << "\"";
				BlueI.clear();
				BlueI.push_back(0.f);
				BlueI.push_back(1.f);
				BlueB.clear();
				BlueB.push_back(0.f);
				BlueB.push_back(1.f);
			}
		}
		validFile = true;
	}

	AdjustGamma(RedI, RedB);
	AdjustGamma(GreenI, GreenB);
	AdjustGamma(BlueI, BlueB);
}

void CameraResponse::Map(RGBColor &rgb) const
{
	if (color) {
		rgb.c[0] = ApplyCrf(rgb.c[0], RedI, RedB);
		rgb.c[1] = ApplyCrf(rgb.c[1], GreenI, GreenB);
		rgb.c[2] = ApplyCrf(rgb.c[2], BlueI, BlueB);
	} else {
		const float y = rgb.Y();
		rgb.c[0] = rgb.c[1] = rgb.c[2] = ApplyCrf(y, RedI, RedB);
	}
}

float CameraResponse::ApplyCrf(float point, const vector<float> &from, const vector<float> &to) const
{
	if (point <= from.front())
		return to.front();
	if (point >= from.back())
		return to.back();

	int index = std::upper_bound(from.begin(), from.end(), point) -
		from.begin();
	float x1 = from[index - 1];
	float x2 = from[index];
	float y1 = to[index - 1];
	float y2 = to[index];
	return Lerp((point - x1) / (x2 - x1), y1, y2);
}

} // namespace lux
