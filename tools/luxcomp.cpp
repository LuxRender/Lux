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

#include <iomanip>
#include <fstream>
#include <string>
#include <sstream>
#include <exception>
#include <iostream>

#include <boost/program_options.hpp>
#include <boost/thread.hpp>
#include <boost/thread/xtime.hpp>
#include <boost/date_time/posix_time/posix_time.hpp>
#include <boost/filesystem/path.hpp>
#include <boost/filesystem/operations.hpp>
#include <boost/scoped_ptr.hpp>

#include "lux.h"
#include "api.h"
#include "context.h"
#include "fleximage.h"
#include "error.h"
#include "osfunc.h"

#if defined(WIN32) && !defined(__CYGWIN__) /* We need the following two to set stdout to binary */
#include <io.h>
#include <fcntl.h>
#endif

#if defined(WIN32) && !defined(__CYGWIN__)
#include "direct.h"
#define chdir _chdir
#endif

using namespace lux;
namespace po = boost::program_options;

void CheckFilePath(const std::string fileName) {
	boost::filesystem::path fullPath(boost::filesystem::initial_path());
	fullPath = boost::filesystem::system_complete(boost::filesystem::path(
			fileName, boost::filesystem::native));

	if (!boost::filesystem::exists(fullPath)) {
		std::stringstream ss;
		ss << "Unable to open file '" << fullPath.string() << "'";
		luxError(LUX_NOFILE, LUX_SEVERE, ss.str().c_str());
		exit(2);
	}
}

void PrintFilmInfo(const FlexImageFilm &film) {
	std::stringstream ss;

	ss << "Film Width x Height: " << film.GetXPixelCount() << "x" << film.GetYPixelCount();
	luxError(LUX_NOERROR, LUX_INFO, ss.str().c_str());

	u_int bufferCount = film.GetNumBufferConfigs();
	ss.str("");
	ss << "Buffer Config count: " << bufferCount;
	luxError(LUX_NOERROR, LUX_INFO, ss.str().c_str());

	for (u_int i = 0; i < bufferCount; i++) {
		const BufferConfig &bc = film.GetBufferConfig(i);

		ss.str("");
		ss << "Buffer Config index " << i << ": type=" << bc.type <<
				", output=" << bc.output <<
				", postfix=" << bc.postfix;
		luxError(LUX_NOERROR, LUX_INFO, ss.str().c_str());
	}

	u_int bufferGroupCount = film.GetNumBufferGroups();
	ss.str("");
	ss << "Buffer Group count: " << bufferGroupCount;
	luxError(LUX_NOERROR, LUX_INFO, ss.str().c_str());

	for (u_int i = 0; i < bufferGroupCount; i++) {
		ss.str("");
		ss << "Buffer Group index " << i << ": name=" << film.GetGroupName(i) <<
				", enable=" << film.GetGroupEnable(i) <<
				", scale=" << film.GetGroupScale(i) <<
				", RGBScale=(" << film.GetGroupRGBScale(i) << ")" <<
				", temperature=" << film.GetGroupTemperature(i);
		luxError(LUX_NOERROR, LUX_INFO, ss.str().c_str());
	}
}

static inline double sqr(double a) {
	return a * a;
}

bool CheckFilms(u_int bufferIndex, FlexImageFilm &refFilm, FlexImageFilm &testFilm) {
	std::stringstream ss;

	// Dade - there are several assumption here about the number of buffers, etc.
	Buffer *refBuf = refFilm.GetBufferGroup(0).getBuffer(bufferIndex);
	Buffer *testBuf = testFilm.GetBufferGroup(0).getBuffer(bufferIndex);

	if (refBuf->xPixelCount != testBuf->xPixelCount) {
		ss.str("");
		ss << "Wrong film width: " << refBuf->xPixelCount << "!=" << testBuf->xPixelCount;
		luxError(LUX_NOERROR, LUX_WARNING, ss.str().c_str());
		return false;
	}

	if (refBuf->yPixelCount != testBuf->yPixelCount) {
		ss.str("");
		ss << "Wrong film height: " << refBuf->yPixelCount << "!=" << testBuf->yPixelCount;
		luxError(LUX_NOERROR, LUX_WARNING, ss.str().c_str());
		return false;
	}

	return true;
}

// Dade - compare 2 buffers returning the Mean Square Error
double CompareFilmWithMSE(u_int bufferIndex, FlexImageFilm &refFilm, FlexImageFilm &testFilm) {
	if (!CheckFilms(bufferIndex, refFilm, testFilm))
		return INFINITY;

	// Dade - there are several assumption here about the number of buffers, etc.
	Buffer *refBuf = refFilm.GetBufferGroup(0).getBuffer(bufferIndex);
	Buffer *testBuf = testFilm.GetBufferGroup(0).getBuffer(bufferIndex);

	// Dade - some metric here was copied exrdiff from PBRT 2.0

	double mse = 0.0;
	double sum1 = 0.0;
	double sum2 = 0.0;
    int smallDiff = 0;
	int bigDiff = 0;
	XYZColor refXYZ, testXYZ;
	float refAlpha, testAlpha;
	for (int y = 0; y < refBuf->yPixelCount; y++) {
		for (int x = 0; x < refBuf->xPixelCount; x++) {
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

				if (d > 0.05)
					smallDiff++;
				if (d > 0.5)
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

	std::stringstream ss;
	ss << "Big diff.: " << bigDiff << " (" << (100.0 * bigDiff / compCount) << "%)";
	luxError(LUX_NOERROR, LUX_INFO, ss.str().c_str());
	ss.str("");
	ss << "Small diff.: " << smallDiff << " (" << (100.0 * smallDiff / compCount) << "%)";
	luxError(LUX_NOERROR, LUX_INFO, ss.str().c_str());
	ss.str("");
	ss << "Avg. reference: " << avg1;
	luxError(LUX_NOERROR, LUX_INFO, ss.str().c_str());
	ss.str("");
	ss << "Avg. test: " << avg2;
	luxError(LUX_NOERROR, LUX_INFO, ss.str().c_str());
	ss.str("");
	ss << "Avg. delta: " << avgDelta;
	luxError(LUX_NOERROR, LUX_INFO, ss.str().c_str());
	ss.str("");
	ss << "MSE: " << mse;
	luxError(LUX_NOERROR, LUX_INFO, ss.str().c_str());

	return mse;
}

enum ComparisonTypes {
	TYPE_MSE = 0
};

int main(int ac, char *av[]) {

	try {
		std::stringstream ss;

		// Declare a group of options that will be
		// allowed only on command line
		po::options_description generic("Generic options");
		generic.add_options()
				("version,v", "Print version string")
				("help,h", "Produce help message")
				("verbosity,V", po::value< int >(), "Log output verbosity")
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
			ss.str("");
			ss << "Usage: luxcomp [options] <reference film file> <test film file>\n" << visible;
			luxError(LUX_SYSTEM, LUX_ERROR, ss.str().c_str());
			return 0;
		}

		if (vm.count("verbosity"))
			luxLogFilter = vm["verbosity"].as<int>();

		ss.str("");
		ss << "Lux version " << LUX_VERSION << " of " << __DATE__ << " at " << __TIME__;
		luxError(LUX_NOERROR, LUX_INFO, ss.str().c_str());
		if (vm.count("version"))
			return 0;

		ComparisonTypes compType = TYPE_MSE;
		if (vm.count("type"))
			compType = (ComparisonTypes)vm["type"].as<int>();
		ss.str("");
		ss << "Comparison type: " << compType;
		luxError(LUX_NOERROR, LUX_INFO, ss.str().c_str());

		if (vm.count("input-file")) {
			const std::vector<std::string> &v = vm["input-file"].as < vector<string> > ();

			if ((v.size() == 1)  || (v.size() == 2)) {
				luxError(LUX_NOERROR, LUX_INFO, "-------------------------------");

				// Dade - read the reference film
				std::string refFileName = v[0];
				ss.str("");
				ss << "Reference file name: '" << refFileName << "'";
				luxError(LUX_NOERROR, LUX_INFO, ss.str().c_str());

				// Dade - check if the file exist
				CheckFilePath(refFileName);

				// Dade - read the reference film from the file
				boost::scoped_ptr<FlexImageFilm> refFilm;
				refFilm.reset((FlexImageFilm *)FlexImageFilm::CreateFilmFromFLM(refFileName));
				if (!refFilm) {
					ss.str("");
					ss << "Error reading reference FLM file '" << refFileName << "'";
					luxError(LUX_NOFILE, LUX_SEVERE, ss.str().c_str());
					return 3;
				}

				PrintFilmInfo(*refFilm);

				if (v.size() == 2) {
					luxError(LUX_NOERROR, LUX_INFO, "-------------------------------");

					// Dade - read the film to test
					std::string testFileName = v[1];
					ss.str("");
					ss << "Test file name: '" << testFileName << "'";
					luxError(LUX_NOERROR, LUX_INFO, ss.str().c_str());

					// Dade - check if the file exist
					CheckFilePath(testFileName);

					// Dade - read the test film from the file
					boost::scoped_ptr<FlexImageFilm> testFilm;
					testFilm.reset((FlexImageFilm *)FlexImageFilm::CreateFilmFromFLM(testFileName));
					if (!testFilm) {
						ss.str("");
						ss << "Error reading test FLM file '" << testFilm << "'";
						luxError(LUX_NOFILE, LUX_SEVERE, ss.str().c_str());
						return 3;
					}

					PrintFilmInfo(*testFilm);

					luxError(LUX_NOERROR, LUX_INFO, "-------------------------------");

					float result = 0.0f;
					for (u_int i = 0; i < refFilm->GetNumBufferConfigs(); i++) {
						float bufResult = 0.0f;

						switch (compType) {
							default:
							case TYPE_MSE:
								bufResult = CompareFilmWithMSE(i, *refFilm, *testFilm);
								break;
						}

						result = max(bufResult, result);
						ss.str("");
						ss << "Result buffer index " << i << ": " << bufResult <<
								" (" << result << ")";
						luxError(LUX_NOERROR, LUX_INFO, ss.str().c_str());

					}
					std::cout << result << std::endl;

					if (vm.count("error")) {
						double treshold = vm["error"].as<double>();
						if (result >= treshold) {
							ss.str("");
							ss << "luxcomp: error above the treshold";
							luxError(LUX_SYSTEM, LUX_ERROR, ss.str().c_str());
							return 10;
						}
					}
				}
			} else {
				ss.str("");
				ss << "luxcomp: wrong input files count";
				luxError(LUX_SYSTEM, LUX_ERROR, ss.str().c_str());
				return 1;
			}
		} else {
			ss.str("");
			ss << "luxcomp: missing input files";
			luxError(LUX_SYSTEM, LUX_ERROR, ss.str().c_str());
			return 1;
		}
	} catch (std::exception & e) {
		std::stringstream ss;
		ss << "Command line argument parsing failed with error '" << e.what() << "', please use the --help option to view the allowed syntax.";
		luxError(LUX_SYNTAX, LUX_SEVERE, ss.str().c_str());
		return 1;
	}

	return 0;
}
