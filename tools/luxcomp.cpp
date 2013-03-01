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

#include <iomanip>
#include <fstream>
#include <string>
#include <sstream>
#include <exception>
#include <iostream>
#include <limits>

#include "api.h"
#include "film/fleximage.h"

#include <boost/program_options.hpp>
#include <boost/thread.hpp>
#include <boost/date_time/posix_time/posix_time.hpp>
#include <boost/filesystem.hpp>
#include <boost/scoped_ptr.hpp>

#if defined(WIN32) && !defined(__CYGWIN__) /* We need the following two to set stdout to binary */
#include <io.h>
#include <fcntl.h>
#endif

using namespace lux;
namespace po = boost::program_options;

enum ComparisonTypes {
	TYPE_MSE = 0,
	TYPE_RMS = 1
};

static inline double sqr(double a) {
	return a * a;
}

void CheckFilePath(const std::string fileName) {
	boost::filesystem::path fullPath(boost::filesystem::system_complete(fileName));

	if (!boost::filesystem::exists(fullPath)) {
		LOG(LUX_SEVERE,LUX_NOFILE) << "Unable to open file '" << fullPath.string() << "'";
		exit(2);
	}
}

void PrintFilmInfo(const FlexImageFilm &film) {
	LOG(LUX_INFO,LUX_NOERROR) << "Film Width x Height: " << film.GetXPixelCount() << "x" << film.GetYPixelCount();

	u_int bufferCount = film.GetNumBufferConfigs();
	LOG( LUX_INFO,LUX_NOERROR) << "Buffer Config count: " << bufferCount;

	for (u_int i = 0; i < bufferCount; i++) {
		const BufferConfig &bc = film.GetBufferConfig(i);

		LOG( LUX_INFO,LUX_NOERROR) << "Buffer Config index " << i << ": type=" << bc.type <<
				", output=" << bc.output <<
				", postfix=" << bc.postfix;
	}

	u_int bufferGroupCount = film.GetNumBufferGroups();
	LOG(LUX_INFO,LUX_NOERROR) << "Buffer Group count: " << bufferGroupCount;

	for (u_int i = 0; i < bufferGroupCount; i++) {
		LOG(LUX_INFO,LUX_NOERROR) << "Buffer Group index " << i << ": name=" << film.GetGroupName(i) <<
				", enable=" << film.GetGroupEnable(i) <<
				", scale=" << film.GetGroupScale(i) <<
				", RGBScale=(" << film.GetGroupRGBScale(i) << ")" <<
				", temperature=" << film.GetGroupTemperature(i);
	}
}

bool CheckFilms(FlexImageFilm &refFilm, FlexImageFilm &testFilm) {

	if (refFilm.GetXPixelCount() != testFilm.GetXPixelCount()) {
		LOG( LUX_SEVERE, LUX_CONSISTENCY) << "Mismatch in reference and test film resolution.";
		LOG( LUX_SEVERE, LUX_CONSISTENCY) << "Wrong film width: " << refFilm.GetXPixelCount() << " != " << testFilm.GetXPixelCount() << ".";
		return false;
	}

	if (refFilm.GetYPixelCount() != testFilm.GetYPixelCount()) {
		LOG( LUX_SEVERE, LUX_CONSISTENCY) << "Mismatch in reference and test film resolution.";
		LOG( LUX_SEVERE, LUX_CONSISTENCY) << "Wrong film height: " << refFilm.GetYPixelCount() << " != " << testFilm.GetYPixelCount() << ".";
		return false;
	}

	return true;
}

// zsolnai - compares the buffers returning the chosen error metric given in compType:
// TYPE_MSE - 0 (Mean Square Error) aka 1/n*sum_{i=1}^n (reference-measured)^2
// TYPE_RMS - 1 (Root Mean Square Error) aka sqrt(mse) or 1/sqrt(n)*sum_{i=1}^n sqrt((reference-measured)^2) if you will
double CompareFilmWith(u_int bufferIndex, FlexImageFilm &refFilm, FlexImageFilm &testFilm, ComparisonTypes compType) {
	if (!CheckFilms(refFilm, testFilm))
		return INFINITY;

	// Dade - there are several assumption here about the number of buffers, etc.
	Buffer *refBuf = refFilm.GetBufferGroup(0).getBuffer(bufferIndex);
	Buffer *testBuf = testFilm.GetBufferGroup(0).getBuffer(bufferIndex);

	// Dade - some metric here was copied exrdiff from PBRT 2.0
	double mse = 0.0;
	double rms = 0.0;
	double sum1 = 0.0;
	double sum2 = 0.0;
	int smallDiff = 0;
	int medDiff = 0;
	int bigDiff = 0;
	double maxDiff = -std::numeric_limits<double>::max();
	double minDiff = std::numeric_limits<double>::max();
	XYZColor refXYZ, testXYZ;
	float refAlpha, testAlpha;

	for (u_int y = 0; y < refBuf->yPixelCount; y++) {
		for (u_int x = 0; x < refBuf->xPixelCount; x++) {
			refBuf->GetData(x, y, &refXYZ, &refAlpha);
			testBuf->GetData(x, y, &testXYZ, &testAlpha);

			for (int i = 0; i < 3; i++) {
				sum1 += refXYZ.c[i];
				sum2 += testXYZ.c[i];
				mse += sqr(refXYZ.c[i] - testXYZ.c[i]);

				double d;
				if (refXYZ.c[i] == 0.0)
					d = fabs(refXYZ.c[i] - testXYZ.c[i]);
				else
					d = fabs(refXYZ.c[i] - testXYZ.c[i]) / refXYZ.c[i];

				if(d>maxDiff)
					maxDiff = d;
				if(d<minDiff)
					minDiff = d;

				if(d <= 0.1)
					smallDiff++;
				else if(d > 0.1 && d <= 0.5)
					medDiff++;
				else if (d > 0.5)
					bigDiff++;
			}
		}
	}

	const u_int pixelCount = refBuf->xPixelCount * refBuf->yPixelCount;
	const double avg1 = sum1 / pixelCount;
    const double avg2 = sum2 / pixelCount;
    double avgDelta = 100.0 * (avg1 - avg2) / min(avg1, avg2);
	const u_int compCount = 3 * pixelCount;
	mse /= compCount;
	rms = sqrt(mse);

	LOG(LUX_INFO,LUX_NOERROR) << "Small diff.: " << smallDiff << " (" << (100.0 * smallDiff / compCount) << "%)";
	LOG(LUX_INFO,LUX_NOERROR) << "Medium diff.: " << medDiff << " (" << (100.0 * medDiff / compCount) << "%)";
	LOG(LUX_INFO,LUX_NOERROR) << "Big diff.: " << bigDiff << " (" << (100.0 * bigDiff / compCount) << "%)";
	LOG(LUX_INFO,LUX_NOERROR) << "Min diff.: " << minDiff;
	LOG(LUX_INFO,LUX_NOERROR) << "Max diff.: " << maxDiff;
	LOG(LUX_INFO,LUX_NOERROR) << "Avg. reference: " << avg1;
	LOG(LUX_INFO,LUX_NOERROR) << "Avg. test: " << avg2;
	LOG(LUX_INFO,LUX_NOERROR) << "Avg. delta: " << avgDelta;
	LOG(LUX_INFO,LUX_NOERROR) << "Avg. |reference-test|: " << fabs(avg1-avg2);
	LOG(LUX_INFO,LUX_NOERROR) << "MSE: " << mse;
	LOG(LUX_INFO,LUX_NOERROR) << "RMS: " << rms;

	if(compType == TYPE_MSE)
		return mse;
	else if(compType == TYPE_RMS)
		return rms;

	LOG(LUX_ERROR,LUX_BUG) << "Invalid compare type: " << compType;
	return 0.0;
}

// zsolnai - compares the image framebuffers returning the chosen error metric given in compType (same as above).
double CompareFramebufferWith(FlexImageFilm &refFilm, FlexImageFilm &testFilm, ComparisonTypes compType=TYPE_RMS) {
	refFilm.WriteImage(IMAGE_FRAMEBUFFER);
	testFilm.WriteImage(IMAGE_FRAMEBUFFER);

	if (!CheckFilms(refFilm, testFilm))
		return INFINITY;
	
	const u_int pixelCount = refFilm.GetXPixelCount()*refFilm.GetYPixelCount();
	const u_int compCount = 3 * pixelCount;
	double mse = 0.0;
	double rms = 0.0;

	const float* p_ref = refFilm.getFloatFrameBuffer();
	const float* p_test = testFilm.getFloatFrameBuffer();

	for(u_int i=0;i<compCount;i++) { // note: this is on (0,pixelCount*3)
		mse += sqr(p_ref[i] - p_test[i]);
	}

	mse /= compCount;
	rms = sqrt(mse);

	LOG(LUX_INFO,LUX_NOERROR) << "MSE: " << mse;
	LOG(LUX_INFO,LUX_NOERROR) << "RMS: " << rms;

	if(compType == TYPE_MSE)
		return mse;
	else if(compType == TYPE_RMS)
		return rms;

	LOG(LUX_ERROR,LUX_BUG) << "Invalid compare type: " << compType;
	return 0.0;
}

int main(int ac, char *av[]) {

	try {
		std::stringstream ss;

		// Declare a group of options that will be
		// allowed only on command line
		po::options_description generic("Generic options");
		generic.add_options()
				("version,v", "Print version string")
				("help,h", "Produce help message")
				("verbose,V", "Increase output verbosity (show DEBUG messages)")
				("quiet,q", "Reduce output verbosity (hide INFO messages)") // (give once for WARNING only, twice for ERROR only)")
				("type,t", po::value< int >(), "Select the type of comparison")
				("error,e", po::value< double >(), "Error treshold for a failure")
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
			LOG( LUX_ERROR,LUX_SYSTEM) << "Usage: luxcomp [options] <reference film file> <test film file>\n" << visible;
			return 0;
		}

		LOG(LUX_INFO,LUX_NOERROR) << "Lux version " << luxVersion() << " of " << __DATE__ << " at " << __TIME__;
		
		if (vm.count("version"))
			return 0;

		if (vm.count("verbose")) {
			luxErrorFilter(LUX_DEBUG);
		}

		if (vm.count("quiet")) {
			luxErrorFilter(LUX_WARNING);
		}

		ComparisonTypes compType = TYPE_MSE;
		if (vm.count("type"))
			compType = (ComparisonTypes)vm["type"].as<int>();
		
		LOG( LUX_INFO,LUX_NOERROR) << "Comparison type: " << compType;
		
		if (vm.count("input-file")) {
			const std::vector<std::string> &v = vm["input-file"].as < vector<string> > ();

			if ((v.size() == 1)  || (v.size() == 2)) {

				luxInit();

				LOG( LUX_INFO,LUX_NOERROR)<< "-------------------------------";

				// Dade - read the reference film
				std::string refFileName = v[0];
				LOG( LUX_INFO,LUX_NOERROR) << "Reference file name: '" << refFileName << "'";

				// Dade - check if the file exist
				CheckFilePath(refFileName);

				// Dade - read the reference film from the file
				boost::scoped_ptr<FlexImageFilm> refFilm;
				refFilm.reset((FlexImageFilm *)FlexImageFilm::CreateFilmFromFLM(refFileName));
				if (!refFilm) {
					LOG( LUX_SEVERE,LUX_NOFILE) << "Error reading reference FLM file '" << refFileName << "'";
					return 3;
				}

				PrintFilmInfo(*refFilm);

				if (v.size() == 2) {
					LOG( LUX_INFO,LUX_NOERROR)<< "-------------------------------";

					// Dade - read the film to test
					std::string testFileName = v[1];
					LOG( LUX_INFO,LUX_NOERROR) << "Test file name: '" << testFileName << "'";

					// Dade - check if the file exist
					CheckFilePath(testFileName);

					// Dade - read the test film from the file
					boost::scoped_ptr<FlexImageFilm> testFilm;
					testFilm.reset((FlexImageFilm *)FlexImageFilm::CreateFilmFromFLM(testFileName));
					if (!testFilm) {
						LOG( LUX_SEVERE,LUX_NOFILE) << "Error reading test FLM file '" << testFilm << "'";
						return 3;
					}

					PrintFilmInfo(*testFilm);

					LOG( LUX_INFO,LUX_NOERROR)<< "-------------------------------";

					float result = 0.0f;
					for (u_int i = 0; i < refFilm->GetNumBufferConfigs(); i++) {
						float bufResult = 0.0f;

						switch (compType) {
							default:
							case TYPE_MSE:
								bufResult = CompareFilmWith(i, *refFilm, *testFilm, TYPE_MSE);
								break;
							case TYPE_RMS:
								bufResult = CompareFilmWith(i, *refFilm, *testFilm, TYPE_RMS);
								break;
						}

						result = max(bufResult, result);
						LOG( LUX_INFO,LUX_NOERROR) << "Result buffer index " << i << ": " << bufResult <<
								" (" << result << ")";
					}
					LOG( LUX_INFO,LUX_NOERROR)<< "-------------------------------";
					LOG( LUX_INFO,LUX_NOERROR)<< "Image comparison error metrics:";
					CompareFramebufferWith(*refFilm, *testFilm);

					if (vm.count("error")) {
						double treshold = vm["error"].as<double>();
						if (result >= treshold) {
							LOG( LUX_ERROR,LUX_SYSTEM) << "luxcomp: error above the treshold";
							return 10;
						}
					}
				}
			} else {
				LOG( LUX_ERROR,LUX_SYSTEM) << "luxcomp: wrong input files count";
				return 1;
			}
		} else {
			LOG( LUX_ERROR,LUX_SYSTEM) << "luxcomp: missing input files";
			return 1;
		}
	} catch (std::exception & e) {
		LOG(LUX_SEVERE,LUX_SYNTAX) << "Command line argument parsing failed with error '" << e.what() << "', please use the --help option to view the allowed syntax.";
		return 1;
	}

	return 0;
}
