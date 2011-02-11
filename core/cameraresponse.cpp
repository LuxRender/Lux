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
#include <boost/regex.hpp>
#include <boost/iterator.hpp>
#include <iostream>
#include <fstream>
#include <sstream>
#include <algorithm>

using namespace lux;

namespace lux {

// estimates gamma in y = x^gamma using Gauss-Newton iterations
float EstimateGamma(const vector<float> &x, const vector<float> &y, float *rmse) {
	
	const size_t n = x.size();

	double gamma = 1.0; // initial guess
	int iter = 0;

	double last_ssr = 1e30;

	while (iter < 100) {

		double ssr = 0;
		double JTr = 0.0;
		double JTJ = 0.0;

		size_t i;

		for (i = 0; i < n; i++) {

			// log() doesn't behave nice near zero
			if (x[i] < 1e-12)
				continue;

			// pow() has issues with small but negative values of gamma
			// this is more stable
			const double lx = log(x[i]);
			const double xg = exp(gamma * lx);
			
			// residual
			double r = y[i] - xg;
			ssr += r*r;
			
			// jacobian
			double J = lx * xg;

			JTr += J * r;
			JTJ += J * J;
		}

		if (rmse)
			*rmse = static_cast<float>(sqrt(ssr/i));

		if (fabs(ssr - last_ssr) < 1e-6)
			break;

		const double delta = JTr / JTJ;

		if (fabs(delta) < 1e-9)
			break;

		gamma = gamma + delta;

		last_ssr = ssr;

		iter++;
	}

	return static_cast<float>(gamma);
}

void AdjustGamma(const vector<float> &from, vector<float> &to, float gamma = 2.2f) {
	for (size_t i = 0; i < from.size(); i++) {
		to[i] = powf(to[i], gamma);
	}
}

CameraResponse::CameraResponse(const string &film)
{
	validFile = false;
	fileName = film;
	std::ifstream file(film.c_str());
	if (!file) {
		LOG(LUX_WARNING, LUX_NOFILE) << "Film file \"" << film << "\" not found! Camera Response Function will not be applied.";
		return;
	}

	string crfdata;

	{
		// read file into memory
		std::stringstream ss("");
		ss << file.rdbuf();
		crfdata = ss.str();
	}

	boost::regex func_expr("(?-s)(?<name>\\S+)(?<channel>Red|Green|Blue)\\s+graph.+\\s+I\\s*=\\s*(?<I>.+$)\\s+B\\s*=\\s*(?<B>.+$)");
	boost::regex float_expr("-?\\d*\\.?\\d+(?:[eE][-+]?\\d+)?");

	boost::sregex_iterator rit(crfdata.begin(), crfdata.end(), func_expr);
	boost::sregex_iterator rend;
	
	bool red, green, blue;
	red = green = blue = false;

	string crfname("");

	for (; rit != rend; ++rit) {
		const boost::smatch &m = *rit;

		const string name = m["name"].str();
		const string channel = m["channel"].str();
		const string Istr = m["I"].str();
		const string Bstr = m["B"].str();

		LOG(LUX_INFO, LUX_NOERROR) << name << " | " << channel << " | " << Istr.length() << " | " << Bstr.length();

		if (crfname == "") {
			crfname = name;
		} else if (crfname != name) {
			LOG(LUX_WARNING, LUX_BADFILE) << "Expected Camera Response Function name '" << crfname << "' but found '" << name << "', disabling";
			return;
		}

		vector<float> *I = NULL;
		vector<float> *B = NULL;
		bool *channel_flag = NULL;

		if (channel == "Red") {
			I = &RedI;
			B = &RedB;
			channel_flag = &red;
		} else if (channel == "Green") {
			I = &GreenI;
			B = &GreenB;
			channel_flag = &green;
		} else if (channel == "Blue") {
			I = &BlueI;
			B = &BlueB;
			channel_flag = &blue;
		} else {
			LOG(LUX_WARNING, LUX_BUG) << "Error parsing Camera Response file, CRF disabled";
			return;
		}

		if (*channel_flag) {
			LOG(LUX_WARNING, LUX_BADFILE) << channel << " channel already specified in '" << film << "', ignoring";
			continue;
		}

		// parse functions
		for (boost::sregex_iterator rit(Istr.begin(), Istr.end(), float_expr); rit != rend; ++rit) {
			I->push_back(boost::lexical_cast<float>(rit->str(0)));
		}

		for (boost::sregex_iterator rit(Bstr.begin(), Bstr.end(), float_expr); rit != rend; ++rit) {
			B->push_back(boost::lexical_cast<float>(rit->str(0)));
		}

		*channel_flag = I->size() == B->size();

		if (!(*channel_flag) || I->empty()) {
			LOG(LUX_WARNING, LUX_LIMIT) << "Inconsistent " << channel << " data for '" << film << "', ignoring";
			I->clear();
			B->clear();
			*channel_flag = false;
		}

		color = red && green && blue;
		if (color)
			break;
	}
	
	// need at least red for either monochrome or color
	if (!red) {
		LOG(LUX_WARNING, LUX_BADFILE) << "No valid Camera Response Functions in '" << film << "', disabling CRF";
		return;
	}

	// compensate for built-in gamma
	// use Y for gamma estimation
	vector<float> YI, YB;

	// use all unique input values
	YI.insert(YI.end(), RedI.begin(), RedI.end());
	YI.insert(YI.end(), GreenI.begin(), GreenI.end());
	YI.insert(YI.end(), BlueI.begin(), BlueI.end());

	const size_t n = std::unique(YI.begin(), YI.end()) - YI.begin();

	YI.resize(n);
	YB.resize(n);

	for (size_t i = 0; i < n; i++) {
		RGBColor c(YI[i]);

		// map handles color / monochrome and interpolation
		Map(c);		

		YB[i] = c.Y();
	}

	float source_gamma, rmse;

	source_gamma = EstimateGamma(YI, YB, &rmse);

	LOG(LUX_DEBUG, LUX_NOERROR) << "Camera Response Function gamma estimated to " << source_gamma << " with RMSE of " << rmse;

	// compensate for built-in gamma
	AdjustGamma(RedI, RedB, 1.f / source_gamma);
	AdjustGamma(GreenI, GreenB, 1.f / source_gamma);
	AdjustGamma(BlueI, BlueB, 1.f / source_gamma);

	validFile = true;
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
