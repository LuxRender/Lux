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

// tabulateddata.cpp*
#include "tabulateddata.h"
#include "irregulardata.h"
#include "dynload.h"

#include <boost/lexical_cast.hpp>
#include <boost/regex.hpp>
#include <fstream>

using namespace lux;

// converts photon energy (in eV) to wavelength (in nm)
static float eVtolambda(float eV) {
	// lambda = hc / E, where 
	//  h is planck's constant in eV*s
	//  c is speed of light in m/s
	return (4.135667e-15f * 299792458.f * 1e9f) / eV;
}

// converts wavelength (in micrometer) to wavelength (in nm)
static float umtolambda(float um) {
	return um * 1000.f;
}

//converts wavelength (in cm-1) to wavelength (in nm)
static float invcmtolambda(float invcm)
{
	return 1e7f / invcm;
}

//converts wavelength (in nm) to wavelength (in nm)
static float nmtolambda(float nm)
{
	return nm;
}

// TabulatedDataTexture Method Definitions
Texture<SWCSpectrum> *TabulatedDataTexture::CreateSWCSpectrumTexture(const Transform &tex2world,
	const ParamSet &tp)
{
	// resampling resolution in nm
	const float resolution = tp.FindOneFloat("resolution", 5);

	const string wlunit = tp.FindOneString("wlunit", "nm");	

	// used to convert file units to wavelength in nm
	float (*tolambda)(float) = NULL;

	if (wlunit == "eV") {
		// lambda in eV
		// low eV -> high lambda
		tolambda = &eVtolambda;
	} else if (wlunit == "um") {
		// lambda in um
		tolambda = &umtolambda;
	} else if (wlunit == "cm-1") {
		// lambda in cm-1
		tolambda = &invcmtolambda;
	} else if (wlunit == "nm") {
		// lambda in nm
		tolambda = &nmtolambda;
	} else {
		LOG(LUX_ERROR, LUX_SYNTAX) << "Unknown wavelength unit '" <<
			wlunit << "', assuming nm.";
		tolambda = &nmtolambda;
	}

	//float lambda_start = tp.FindOneFloat("start", 380.f);
	//float lambda_end = tp.FindOneFloat("end", 720.f);

	boost::smatch m;

	// two floats separated by whitespace, comma or semicolon
	boost::regex sample_expr("\\s*(\\d*\\.?\\d+)(\\s*[,;]\\s*|\\s+)(\\d*\\.?\\d+)");

	vector<float> wl;
	vector<float> data;

	const string filename = tp.FindOneString("name", "");
	std::ifstream fs;
	fs.open(filename.c_str());
	string line;

	int n = 0;
	while(getline(fs, line).good()) {

		// ignore unparseable lines
		if (!boost::regex_search(line, m, sample_expr)) {
			continue;
		}

		// linearly interpolate units in file
		// then convert to wavelength in nm
		wl.push_back(tolambda(boost::lexical_cast<float>(m[1])));
		data.push_back(boost::lexical_cast<float>(m[3]));
		n++;
	}

	LOG(LUX_DEBUG, LUX_NOERROR) << "Read '" <<
		n << "' entries from '" << filename << "'";	

	// need at least two samples
	if (n < 2) {
		LOG(LUX_ERROR, LUX_BADFILE) << "Unable to read data file '" << filename << "' or insufficient data";
		const float default_wl[] = {380.f, 720.f};
		const float default_data[] = {1.f, 1.f};
		return new IrregularDataTexture(2, default_wl, default_data);
	}

	// possibly reorder data so wavelength is increasing
	if (wl[0] > wl[1]) {
		std::reverse(wl.begin(), wl.end());
		std::reverse(data.begin(), data.end());
	}

	return new IrregularDataTexture(n, &wl[0], &data[0], resolution);
}

static DynamicLoader::RegisterSWCSpectrumTexture<TabulatedDataTexture> r("tabulateddata");
