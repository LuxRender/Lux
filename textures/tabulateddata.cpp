/***************************************************************************
 *   Copyright (C) 1998-2013 by authors (see AUTHORS.txt)                  *
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
#include "filedata.h"

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
	boost::smatch m;

	// wavelength: <unit> data: <tag>
	boost::regex header_expr("wavelength:\\s*(eV|um|cm-1|nm)(\\s*[,;]\\s*|\\s+)data:\\s*(\\S+)");

	// two floats separated by whitespace, comma or semicolon
	boost::regex sample_expr("^\\s*(\\d*\\.?\\d+(?:[eE]-?\\d+)?)(\\s*[,;]\\s*|\\s+)(-?\\d*\\.?\\d+(?:[eE]-?\\d+)?)");


	// used to convert file units to wavelength in nm
	float (*tolambda)(float) = NULL;


	vector<float> wl;
	vector<float> data;

	// Attempt decode of embedded data
	FileData::decode(tp, "filename");

	const string filename = AdjustFilename(tp.FindOneString("filename", ""));
	std::ifstream fs;
	fs.open(filename.c_str());
	string line;

	bool parsingdata = false;
	int n = 0;
	while(getline(fs, line).good()) {

		if (!parsingdata && boost::regex_search(line, m, header_expr)) {		
			const string wlunit = m[1];	
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
				LOG(LUX_ERROR, LUX_BADFILE) << "Unknown wavelength unit '" <<
					wlunit << "', assuming nm.";
				tolambda = &nmtolambda;
			}
			parsingdata = true;
			continue;
		}

		if (!boost::regex_search(line, m, sample_expr)) {
			if (parsingdata)
				break;
			else
				// ignore unparseable lines at start of file
				continue;
		}

		parsingdata = true;
		
		if (!tolambda) {
			// no header found
			LOG(LUX_INFO, LUX_NOERROR) << "Unknown wavelength unit in '" << filename << "', assuming nm.";
			tolambda = &nmtolambda;			
		}

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
	if (wl.front() > wl.back()) {
		std::reverse(wl.begin(), wl.end());
		std::reverse(data.begin(), data.end());
	}

	if (wl.front() > WAVELENGTH_END || wl.back() < WAVELENGTH_START) {
		LOG(LUX_ERROR, LUX_RANGE) << "Spectral data file '" << filename 
			<< "' does not contain data in the visible spectrum (" << WAVELENGTH_START << '-' << WAVELENGTH_END << "nm)";
		const float default_wl[] = {380.f, 720.f};
		const float default_data[] = {1.f, 1.f};
		return new IrregularDataTexture(2, default_wl, default_data);
	}

	if (wl.front() > WAVELENGTH_START || wl.back() < WAVELENGTH_END) {
		LOG(LUX_INFO, LUX_CONSISTENCY) << "Spectral data file '" << filename 
			<< "' does not cover the entire visible spectrum (" << WAVELENGTH_START << '-' << WAVELENGTH_END << "nm)"
			<< ": " << wl.front() << '-' << wl.back() << "nm"
			<< ", this may yield unintented results.";
	}

	return new IrregularDataTexture(n, &wl[0], &data[0]);
}

static DynamicLoader::RegisterSWCSpectrumTexture<TabulatedDataTexture> r("tabulateddata");
