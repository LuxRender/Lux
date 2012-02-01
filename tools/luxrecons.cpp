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

/*
exrnormalize out.exr out.n.exr
exrpptm out.n.exr out.pp.exr
exrnormalize out.pp.exr out.pp.n.exr
exrtopng out.pp.n.exr out.png
*/

#include <iomanip>
#include <fstream>
#include <string>
#include <sstream>
#include <exception>
#include <iostream>

#include "lux.h"
#include "error.h"
#include "samplefile.h"
#include "exrio.h"
#include "luxrecons/sampledatagrid.h"
#include "color.h"
#include "osfunc.h"
#include "sampling.h"

#include <boost/program_options.hpp>

#if defined(__GNUC__) && !defined(__CYGWIN__)
#include <execinfo.h>
#include <stdio.h>
#include <stdlib.h>
#include <typeinfo>
#include <cxxabi.h>

static string Demangle(const char *symbol) {
	size_t size;
	int status;
	char temp[128];
	char* result;

	if (1 == sscanf(symbol, "%*[^'(']%*[^'_']%[^')''+']", temp)) {
		if (NULL != (result = abi::__cxa_demangle(temp, NULL, &size, &status))) {
			string r = result;
			return r + " [" + symbol + "]";
		}
	}

	if (1 == sscanf(symbol, "%127s", temp))
		return temp;

	return symbol;
}

void MainTerminate(void) {
	std::cerr << "=========================================================" << std::endl;
	std::cerr << "Unhandled exception" << std::endl;

	void *array[32];
	size_t size = backtrace(array, 32);
	char **strings = backtrace_symbols(array, size);

	std::cerr << "Obtained " << size << " stack frames." << std::endl;

	for (size_t i = 0; i < size; i++)
		std::cerr << "  " << Demangle(strings[i]) << std::endl;

	free(strings);

	abort();
}
#endif

using namespace lux;
namespace po = boost::program_options;

enum ReconstructionTypes {
	TYPE_BOX = 0,
	TYPE_RPF = 1,
	TYPE_V1_NORMAL = 2,
	TYPE_V1_BSDF = 3
};

//------------------------------------------------------------------------------

void RPF_PresprocessSamples(const SampleDataGrid &sampleDataGrig,
		const int x, const int y,
		const int b, vector<size_t> N) {
	// Compute mean and standard deviation of all samples inside the pixel
	const vector<size_t> &P = sampleDataGrig.GetPixelList(x, y);

	// Scene features mean
	SampleData &sampleData(*sampleDataGrig.sampleData);
	const size_t sceneFeaturesSize = sampleData.sceneFeaturesCount;

	vector<float> sceneFeaturesMean(sceneFeaturesSize, 0.f);
	for (size_t i = 0; i < P.size(); ++i) {
		const float *sceneFeatures = sampleData.GetSceneFeatures(P[i]);

		for (size_t j = 0; j < sceneFeaturesSize; ++j)
			sceneFeaturesMean[j] += sceneFeatures[j];
	}

	for (size_t j = 0; j < sceneFeaturesSize; ++j)
		sceneFeaturesMean[j] /= P.size();

	// Scene features standard deviation
	vector<float> sceneFeaturesStandardDeviation(sceneFeaturesSize, 0.f);
	for (size_t i = 0; i < P.size(); ++i) {
		const float *sceneFeatures = sampleData.GetSceneFeatures(P[i]);

		for (size_t j = 0; j < sceneFeaturesSize; ++j) {
			const float v = sceneFeatures[j] - sceneFeaturesMean[j];
			sceneFeaturesStandardDeviation[j] = v * v;
		}
	}

	for (size_t j = 0; j < sceneFeaturesSize; ++j)
		sceneFeaturesStandardDeviation[j] = sqrtf(sceneFeaturesStandardDeviation[j] / P.size());

	// For all samples inside the filtering box
	const float filterWidth = b / 2.f;
	const int x0 = max(Ceil2Int(x - filterWidth), sampleDataGrig.xPixelStart);
	const int x1 = min(Floor2Int(x + filterWidth), sampleDataGrig.xPixelEnd);
	const int y0 = max(Ceil2Int(y - filterWidth), sampleDataGrig.yPixelStart);
	const int y1 = min(Floor2Int(y + filterWidth), sampleDataGrig.yPixelEnd);

	for (int yy = y0; yy <= y1; ++yy) {
		for (int xx = x0; xx <= x1; ++xx) {
			const vector<size_t> &NP = sampleDataGrig.GetPixelList(xx, yy);

			for (size_t i = 0; i < NP.size(); ++i) {
				// Check if it is a sample I can use
				const float *sceneFeatures = sampleData.GetSceneFeatures(NP[i]);

				bool valid = true;
				for (size_t j = 0; j < sceneFeaturesSize; ++j) {
					// World coordinates require a larger constant
					const float k = ((j % 9) <= 2) ? 30.f : 3.f;
					const float kk = k * sceneFeaturesStandardDeviation[j];
					const float fm = fabsf(sceneFeatures[j] - sceneFeaturesMean[j]);

					if ((fm > kk) && ((fm > .1f) || (kk > .1f))) {
						valid = false;
						break;
					}
				}

				if (valid)
					N.push_back(NP[i]);
			}
		}
	}

	//LOG(LUX_INFO, LUX_NOERROR) << "[" << x << ", " << y << "] " << N->size() << " + " << indexPixelList.size();
}

static inline float sqr(const float v) {
	return v * v;
}

void RPF_FilterColorSamples(const SampleDataGrid &sampleDataGrig,
		const vector<size_t> &P, const vector<size_t> &N,
		const float alpha, const float beta,
		const vector<XYZColor> &c1, vector<XYZColor> &c2) {
	//SampleData &sampleData(*sampleDataGrig.sampleData);

	for (size_t i = 0; i < P.size(); ++i)
		c2[P[i]] = c1[P[i]];

	// Filter the colors of samples in pixel P using bilateral filter
	/*for (size_t i = 0; i < P.size(); ++i) {
		const size_t Pi = P[i];
		c2[Pi] = 0.f;
		float w = 0.f;

		const float *sceneFeaturesPi = sampleData.GetSceneFeatures(P[i]);
		for (size_t j = 0; j < N.size(); ++j) {
			const float *sceneFeaturesNj = sampleData.GetSceneFeatures(N[j]);

			//
			float wij = expf(sqr(sceneFeaturesPi[0] - sceneFeaturesNj[0]) + sqr(sceneFeaturesPi[1] - sceneFeaturesNj[1]));
			c2[Pi] += wij * c1[Pi];
			w += wij;
		}

		c2[Pi] /= w;
	}*/
}

void Recons_RPF(SampleData *sampleData, const string &outputFileName) {
	LOG(LUX_INFO, LUX_NOERROR) << "Building sample data grid...";

	SampleDataGrid sampleDataGrig(sampleData);

	// Build the sample list
	vector<XYZColor> c1(sampleData->count);
	for (size_t i = 0; i < sampleData->count; ++i)
		c1[i] = *(sampleData->GetColor(i));
	vector<XYZColor> c2(sampleData->count);

	const int b = 7;
	LOG(LUX_INFO, LUX_NOERROR) << "RPF step, size: " << b;
	double lastPrintTime = osWallClockTime();
	for (int y = sampleDataGrig.yPixelStart; y <= sampleDataGrig.yPixelEnd; ++y) {
		if (osWallClockTime() - lastPrintTime > 5.0) {
			LOG(LUX_INFO, LUX_NOERROR) << "RPF line: " << (y - sampleDataGrig.yPixelStart + 1) << "/" << sampleDataGrig.yResolution;
			lastPrintTime = osWallClockTime();
		}

		for (int x = sampleDataGrig.xPixelStart; x <= sampleDataGrig.xPixelEnd; ++x) {
			const vector<size_t> &P = sampleDataGrig.GetPixelList(x, y);

			if (P.size() > 0) {
				// Preprocess the samples and cluster them
				vector<size_t> N;
				RPF_PresprocessSamples(sampleDataGrig, x, y, b, N);

				// Compute feature weight
				// TODO
				float alpha = 1.f;
				float beta = 1.f;

				// Filter color samples
				RPF_FilterColorSamples(sampleDataGrig, P, N, alpha, beta, c1, c2);
			}
		}
	}

	// Copy c'' in c'
	std::copy(c2.begin(), c2.end(), c1.begin());

	LOG(LUX_INFO, LUX_NOERROR) << "Writing EXR image: " << outputFileName;

	const float red[2] = {0.63f, 0.34f};
	const float green[2] = {0.31f, 0.595f};
	const float blue[2] = {0.155f, 0.07f};
	const float white[2] = {0.314275f, 0.329411f};
	ColorSystem colorSpace = ColorSystem(red[0], red[1], green[0], green[1], blue[0], blue[1], white[0], white[1], 1.f);

	vector<RGBColor> pixels(sampleDataGrig.xResolution * sampleDataGrig.yResolution);
	size_t rgbi = 0;
	for (int y = sampleDataGrig.yPixelStart; y <= sampleDataGrig.yPixelEnd; ++y) {
		for (int x = sampleDataGrig.xPixelStart; x <= sampleDataGrig.xPixelEnd; ++x) {
			const vector<size_t> &P = sampleDataGrig.GetPixelList(x, y);

			XYZColor c;
			if (P.size() > 0) {
				for (size_t i = 0; i < P.size(); ++i)
					c += c1[P[i]];

				c /= P.size();
			}
			//LOG(LUX_INFO, LUX_NOERROR) << "[" << x << ", " << y << "][" << indexPixelList.size() << "] = " << c;

			pixels[rgbi++] = colorSpace.ToRGBConstrained(c);
		}
	}

	vector<float> dummy;
	WriteOpenEXRImage(2, false, false, 0, outputFileName, pixels, dummy,
		sampleDataGrig.xResolution, sampleDataGrig.yResolution,
		sampleDataGrig.xResolution, sampleDataGrig.yResolution,
		0, 0,
		dummy);
}

//------------------------------------------------------------------------------

void Recons_BOX(SampleData *sampleData, const string &outputFileName) {
	LOG(LUX_INFO, LUX_NOERROR) << "Building sample data grid...";
	SampleDataGrid sampleDataGrig(sampleData);

	LOG(LUX_INFO, LUX_NOERROR) << "Box filtering...";
	const float red[2] = {0.63f, 0.34f};
	const float green[2] = {0.31f, 0.595f};
	const float blue[2] = {0.155f, 0.07f};
	const float white[2] = {0.314275f, 0.329411f};
	ColorSystem colorSpace = ColorSystem(red[0], red[1], green[0], green[1], blue[0], blue[1], white[0], white[1], 1.f);

	vector<RGBColor> pixels(sampleDataGrig.xResolution * sampleDataGrig.yResolution);
	size_t rgbi = 0;
	for (size_t y = 0; y < sampleDataGrig.yResolution; ++y) {
		for (size_t x = 0; x < sampleDataGrig.xResolution; ++x) {
			const vector<size_t> &index = sampleDataGrig.GetPixelList(x + sampleDataGrig.xPixelStart, y + sampleDataGrig.yPixelStart);

			XYZColor c;
			if (index.size() > 0) {
				for (size_t i = 0; i < index.size(); ++i)
					c += *(sampleData->GetColor(index[i]));

				c /= index.size();
			}
			//LOG(LUX_INFO, LUX_NOERROR) << "[" << x << ", " << y << "][" << index.size() << "] = " << c;

			pixels[rgbi++] = colorSpace.ToRGBConstrained(c);
		}
	}

	LOG(LUX_INFO, LUX_NOERROR) << "Writing EXR image: " << outputFileName;
	vector<float> dummy;
	WriteOpenEXRImage(2, false, false, 0, outputFileName, pixels, dummy,
		sampleDataGrig.xResolution, sampleDataGrig.yResolution,
		sampleDataGrig.xResolution, sampleDataGrig.yResolution,
		0, 0,
		dummy);
}

//------------------------------------------------------------------------------

void Recons_V1_NORMAL(SampleData *sampleData, const string &outputFileName) {
	LOG(LUX_INFO, LUX_NOERROR) << "Building sample data grid...";
	SampleDataGrid sampleDataGrig(sampleData);

	LOG(LUX_INFO, LUX_NOERROR) << "Path vertex 1 normal output...";
	vector<RGBColor> pixels(sampleDataGrig.xResolution * sampleDataGrig.yResolution);
	size_t rgbi = 0;
	for (size_t y = 0; y < sampleDataGrig.yResolution; ++y) {
		for (size_t x = 0; x < sampleDataGrig.xResolution; ++x) {
			const vector<size_t> &index = sampleDataGrig.GetPixelList(x + sampleDataGrig.xPixelStart, y + sampleDataGrig.yPixelStart);

			pixels[rgbi] = RGBColor();
			if (index.size() > 0) {
				for (size_t i = 0; i < index.size(); ++i) {

					const SamplePathInfo *pathInfo = (SamplePathInfo *)(sampleData->GetSceneFeatures(index[i]));

					pixels[rgbi].c[0] += pathInfo->v1Normal.x;
					pixels[rgbi].c[1] += pathInfo->v1Normal.y;
					pixels[rgbi].c[2] += pathInfo->v1Normal.z;
				}

				pixels[rgbi] /= index.size();
			}

			++rgbi;
		}
	}

	LOG(LUX_INFO, LUX_NOERROR) << "Writing EXR image: " << outputFileName;
	vector<float> dummy;
	WriteOpenEXRImage(2, false, false, 0, outputFileName, pixels, dummy,
		sampleDataGrig.xResolution, sampleDataGrig.yResolution,
		sampleDataGrig.xResolution, sampleDataGrig.yResolution,
		0, 0,
		dummy);
}

//------------------------------------------------------------------------------

void Recons_V1_BSDF(SampleData *sampleData, const string &outputFileName) {
	LOG(LUX_INFO, LUX_NOERROR) << "Building sample data grid...";
	SampleDataGrid sampleDataGrig(sampleData);

	LOG(LUX_INFO, LUX_NOERROR) << "Path vertex 1 BSDF colour output...";
	const float red[2] = {0.63f, 0.34f};
	const float green[2] = {0.31f, 0.595f};
	const float blue[2] = {0.155f, 0.07f};
	const float white[2] = {0.314275f, 0.329411f};
	ColorSystem colorSpace = ColorSystem(red[0], red[1], green[0], green[1], blue[0], blue[1], white[0], white[1], 1.f);

	vector<RGBColor> pixels(sampleDataGrig.xResolution * sampleDataGrig.yResolution);
	size_t rgbi = 0;
	for (size_t y = 0; y < sampleDataGrig.yResolution; ++y) {
		for (size_t x = 0; x < sampleDataGrig.xResolution; ++x) {
			const vector<size_t> &index = sampleDataGrig.GetPixelList(x + sampleDataGrig.xPixelStart, y + sampleDataGrig.yPixelStart);

			XYZColor c;
			if (index.size() > 0) {
				for (size_t i = 0; i < index.size(); ++i) {
					const SamplePathInfo *pathInfo = (SamplePathInfo *)(sampleData->GetSceneFeatures(index[i]));

					c += pathInfo->v1Bsdf;
				}

				c /= index.size();
			}

			pixels[rgbi++] = colorSpace.ToRGBConstrained(c);
		}
	}

	LOG(LUX_INFO, LUX_NOERROR) << "Writing EXR image: " << outputFileName;
	vector<float> dummy;
	WriteOpenEXRImage(2, false, false, 0, outputFileName, pixels, dummy,
		sampleDataGrig.xResolution, sampleDataGrig.yResolution,
		sampleDataGrig.xResolution, sampleDataGrig.yResolution,
		0, 0,
		dummy);
}

//------------------------------------------------------------------------------

int main(int ac, char *av[]) {
#if defined(__GNUC__) && !defined(__CYGWIN__)
	std::set_terminate(MainTerminate);
#endif

	// Declare a group of options that will be
	// allowed only on command line
	po::options_description generic("Generic options");
	generic.add_options()
			("version,v", "Print version string")
			("help,h", "Produce help message")
			("output,o", po::value< std::string >()->default_value("out.exr"), "Output file")
			("verbose,V", "Increase output verbosity (show DEBUG messages)")
			("quiet,q", "Reduce output verbosity (hide INFO messages)") // (give once for WARNING only, twice for ERROR only)")
			("type,t", po::value< int >(), "Select the type of reconstruction (0 = BOX, 1 = RPF, 2 = V1_NORMAL, 3 = V1_BSDF)")
			;

	// Hidden options, will be allowed both on command line and
	// in config file, but will not be shown to the user.
	po::options_description hidden("Hidden options");
	hidden.add_options()
			("input-file", po::value< vector<string> >(), "input file")
			;

	po::options_description cmdline_options;
	cmdline_options.add(generic).add(hidden);

	po::options_description visible("Allowed options");
	visible.add(generic);

	po::positional_options_description p;

	p.add("input-file", -1);

	po::variables_map vm;
	store(po::command_line_parser(ac, av).
			options(cmdline_options).positional(p).run(), vm);

	if (vm.count("help")) {
		LOG(LUX_ERROR, LUX_SYSTEM) << "Usage: luxrecons [options] <samples file1> [<samples fileN>] <exr file>\n" << visible;
		return 0;
	}

	LOG(LUX_INFO, LUX_NOERROR) << "Lux version " << luxVersion() << " of " << __DATE__ << " at " << __TIME__;

	if (vm.count("version"))
		return 0;

	if (vm.count("verbose")) {
		luxErrorFilter(LUX_DEBUG);
	}

	if (vm.count("quiet")) {
		luxErrorFilter(LUX_WARNING);
	}

	ReconstructionTypes reconType = TYPE_RPF;
	if (vm.count("type"))
		reconType = (ReconstructionTypes)vm["type"].as<int>();

	LOG(LUX_INFO, LUX_NOERROR) << "Reconstruction type: " << reconType;

	if (vm.count("input-file")) {
		const std::vector<std::string> &v = vm["input-file"].as < vector<string> > ();
		const string outputFileName = vm["output"].as<string>();

		vector<SampleData *> samples;
		for (size_t i = 0; i < v.size(); i++) {
			SampleFileReader sfr(v[i]);
			SampleData *sd = sfr.ReadAllSamples();
			if (sd->count > 0)
				samples.push_back(sd);
			else
				LOG(LUX_WARNING, LUX_MISSINGDATA) << "No samples in file: " << v[i];
		}

		SampleData *sampleData = SampleData::Merge(samples);

		switch(reconType) {
			case TYPE_BOX:
				Recons_BOX(sampleData, outputFileName);
				break;
			case TYPE_RPF:
				Recons_RPF(sampleData, outputFileName);
				break;
			case TYPE_V1_NORMAL:
				Recons_V1_NORMAL(sampleData, outputFileName);
				break;
			case TYPE_V1_BSDF:
				Recons_V1_BSDF(sampleData, outputFileName);
				break;
			default:
				throw std::runtime_error("Unknown reconstruction type");
				break;
		}

		delete sampleData;
	} else {
		LOG(LUX_ERROR, LUX_SYSTEM) << "luxrecons: missing input/output files";
		return 1;
	}

	return 0;
}
