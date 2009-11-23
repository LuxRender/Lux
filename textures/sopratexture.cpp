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

// sopratexture.cpp*
#include "sopratexture.h"
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

// CauchyTexture Method Definitions
Texture<ConcreteFresnel> *SopraTexture::CreateFresnelTexture(const Transform &tex2world,
	const TextureParams &tp)
{
	const string filename = tp.FindString("name");
	std::ifstream fs;
	fs.open(filename.c_str());
	string line;
	if (!getline(fs, line).good())
		return NULL;

	boost::smatch m;

	// read initial line, containing metadata
	boost::regex header_expr("(\\d+)\\s+(\\d*\\.?\\d+)\\s+(\\d*\\.?\\d+)\\s+(\\d+)");

	if (!boost::regex_search(line, m, header_expr))
		return NULL;

	// used to convert file units to wavelength in nm
	float (*tolambda)(float) = NULL;

	float lambda_first, lambda_last;

	if (m[1] == "1") {
		// lambda in eV
		// low eV -> high lambda
		lambda_last = boost::lexical_cast<float>(m[2]);
		lambda_first = boost::lexical_cast<float>(m[3]);
		tolambda = &eVtolambda;
	} else if (m[1] == "2") {
		// lambda in um
		lambda_first = boost::lexical_cast<float>(m[2]);
		lambda_last = boost::lexical_cast<float>(m[3]);
		tolambda = &umtolambda;
	} else if (m[1] == "3") {
		// lambda in cm-1
		lambda_last = boost::lexical_cast<float>(m[2]);
		lambda_first = boost::lexical_cast<float>(m[3]);
		tolambda = &invcmtolambda;
	} else if (m[1] == "4") {
		// lambda in nm
		lambda_first = boost::lexical_cast<float>(m[2]);
		lambda_last = boost::lexical_cast<float>(m[3]);
		tolambda = &nmtolambda;
	} else
		return NULL;

	// number of lines of nk data
	const int count = boost::lexical_cast<int>(m[4]);  

	// read nk data
	boost::regex sample_expr("(\\d*\\.?\\d+)\\s+(\\d*\\.?\\d+)");

	vector<float> wl(count);
	vector<float> n(count);
	vector<float> k(count);

	// we want lambda to go from low to high
	// so reverse direction
	const float delta = (lambda_last - lambda_first) / count;
	for (int i = count - 1; i >= 0; --i) {

		if (!getline(fs, line).good())
			return NULL;

		if (!boost::regex_search(line, m, sample_expr))
			return NULL;

		// linearly interpolate units in file
		// then convert to wavelength in nm
		wl[i] = tolambda(lambda_first + delta * i);
		n[i] = boost::lexical_cast<float>(m[1]);
		k[i] = boost::lexical_cast<float>(m[2]);
	}

	return new SopraTexture(wl, n, k);
}

static DynamicLoader::RegisterFresnelTexture<SopraTexture> r("sopra");
