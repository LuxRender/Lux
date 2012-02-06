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
#include <limits>

#include "lux.h"
#include "error.h"
#include "samplefile.h"
#include "exrio.h"
#include "luxrecons/sampledatagrid.h"
#include "color.h"
#include "osfunc.h"
#include "sampling.h"

#include <boost/program_options.hpp>
#include <boost/thread/thread.hpp>

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
	TYPE_V1_POS = 100,
	TYPE_V1_NORMAL = 101,
	TYPE_V1_BSDF = 102,
	TYPE_CLUSTER = 103
};

template<class T> inline T sqr(const T v) {
	return v * v;
}

//------------------------------------------------------------------------------

void RPF_PresprocessSamples(RandomGenerator &rng,
		const SampleDataGrid &sampleDataGrig,
		const int x, const int y,
		const int b, const size_t maxSamples, vector<size_t> &N) {
	const vector<size_t> &P = sampleDataGrig.GetPixelList(x, y);
	if (P.size() >= maxSamples) {
		N.insert(N.begin(), P.begin(), P.end());
		return;
	}

	const size_t maxNSize = maxSamples - P.size();

	// Compute mean and standard deviation of all samples inside the pixel

	// Scene features mean
	SampleData &sampleData(*sampleDataGrig.sampleData);
	const size_t sceneFeaturesCount = sampleData.sceneFeaturesCount;

	vector<float> sceneFeaturesMean(sceneFeaturesCount, 0.f);
	for (size_t i = 0; i < P.size(); ++i) {
		const float *sceneFeatures = sampleData.GetSceneFeatures(P[i]);

		for (size_t j = 0; j < sceneFeaturesCount; ++j)
			sceneFeaturesMean[j] += sceneFeatures[j];
	}

	for (size_t j = 0; j < sceneFeaturesCount; ++j)
		sceneFeaturesMean[j] /= P.size();

	// Scene features standard deviation
	vector<float> sceneFeaturesStandardDeviation(sceneFeaturesCount, 0.f);
	for (size_t i = 0; i < P.size(); ++i) {
		const float *sceneFeatures = sampleData.GetSceneFeatures(P[i]);

		for (size_t j = 0; j < sceneFeaturesCount; ++j)
			sceneFeaturesStandardDeviation[j] = sqr(sceneFeatures[j] - sceneFeaturesMean[j]);
	}

	for (size_t j = 0; j < sceneFeaturesCount; ++j)
		sceneFeaturesStandardDeviation[j] = sqrtf(sceneFeaturesStandardDeviation[j] / P.size());

	// For all samples inside the filtering box
	const float filterWidth = b / 2.f;
	const int x0 = max(Ceil2Int(x - filterWidth), sampleDataGrig.xPixelStart);
	const int x1 = min(Floor2Int(x + filterWidth), sampleDataGrig.xPixelEnd);
	const int y0 = max(Ceil2Int(y - filterWidth), sampleDataGrig.yPixelStart);
	const int y1 = min(Floor2Int(y + filterWidth), sampleDataGrig.yPixelEnd);

	for (int yy = y0; yy <= y1; ++yy) {
		for (int xx = x0; xx <= x1; ++xx) {
			// Skip Samples in P, already added
			if ((xx == x) && (yy == y))
				continue;

			const vector<size_t> &NP = sampleDataGrig.GetPixelList(xx, yy);

			for (size_t i = 0; i < NP.size(); ++i) {
				// Check if it is a sample I can use
				const float *sceneFeatures = sampleData.GetSceneFeatures(NP[i]);

				bool valid = true;
				for (size_t j = 0; j < sceneFeaturesCount; ++j) {
					// World coordinates require a larger constant
					const float k = ((j % (sizeof(SamplePathInfo) / sizeof(float))) <= 2) ? 30.f : 3.f;
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

	//LOG(LUX_INFO, LUX_NOERROR) << "[" << x << ", " << y << "] " << N.size();

	if (N.size() <= maxNSize) {
		N.insert(N.begin(), P.begin(), P.end());
		return;
	}

	// Select a random subset of N
	for (size_t i = 0; i < maxNSize; ++i) {
		const size_t size = N.size() - i - 1;
		const size_t index = min<size_t>(Floor2UInt(rng.floatValue() * size), size - 1);
		swap(N[i], N[i + index]);
	}

	N.resize(maxNSize);

	// Add all samples in P to N. Head insert because required by folowing code
	N.insert(N.begin(), P.begin(), P.end());
}

void RPF_FilterColorSamples(const SampleDataGrid &sampleDataGrig,
		const vector<size_t> &P, const vector<size_t> &N,
		const float alpha, const float beta,
		const vector<XYZColor> &c1, vector<XYZColor> &c2) {
	// NOTE: this code assume the first elements in N are the same of P

	SampleData &sampleData(*sampleDataGrig.sampleData);

	// Compute the mean
	float xyMean[2] = { 0.f, 0.f };
	XYZColor cMean;
	for (size_t i = 0; i < N.size(); ++i) {
		const size_t index = N[i];
		const float *imageXY = sampleData.GetImageXY(index);
		xyMean[0] += imageXY[0];
		xyMean[1] += imageXY[1];

		const XYZColor *c = sampleData.GetColor(index);
		cMean += *c;
	}

	xyMean[0] /= N.size();
	xyMean[1] /= N.size();
	cMean /= N.size();

	// Compute the standard deviation
	float xyStandardDeviation[2] = { 0.f, 0.f };
	XYZColor cStandardDeviation;
	for (size_t i = 0; i < N.size(); ++i) {
		const size_t index = N[i];
		const float *imageXY = sampleData.GetImageXY(index);
		xyStandardDeviation[0] += sqr(imageXY[0] - xyMean[0]);
		xyStandardDeviation[1] += sqr(imageXY[1] - xyMean[1]);

		const XYZColor *c = sampleData.GetColor(index);
		cStandardDeviation += sqr(*c - cMean);
	}

	xyStandardDeviation[0] = sqrtf(xyStandardDeviation[0] / N.size());
	if (xyStandardDeviation[0] == 0.f)
		xyStandardDeviation[0] = 1.f; // To avoid / 0.f
	xyStandardDeviation[1] = sqrtf(xyStandardDeviation[1] / N.size());
	if (xyStandardDeviation[1] == 0.f)
		xyStandardDeviation[1] = 1.f; // To avoid / 0.f
	cStandardDeviation = (cStandardDeviation / N.size()).Sqrt();
	if (cStandardDeviation.Black())
		cStandardDeviation = 1.f; // To avoid / 0.f

	// Compute the normalized values
	vector<float> xNormalized(N.size());
	vector<float> yNormalized(N.size());
	vector<XYZColor> cNormalized(N.size());

	for (size_t i = 0; i < N.size(); ++i) {
		const size_t index = N[i];
		const float *imageXY = sampleData.GetImageXY(index);
		xNormalized[i] = (imageXY[0] - xyMean[0]) / xyStandardDeviation[0];
		yNormalized[i] = (imageXY[1] - xyMean[1]) / xyStandardDeviation[1];

		const XYZColor *c = sampleData.GetColor(index);
		cNormalized[i] = (*c - cMean) / cStandardDeviation;
	}

	// Filter the colors of samples in pixel P using bilateral filter
	const float o_2_8 = .02f;
	const float o_2 = 8.f * o_2_8 / sampleDataGrig.avgSamplesPerPixel;

	for (size_t i = 0; i < P.size(); ++i) {
		// Calculate the filter weight
		const size_t Pi = P[i];
		c2[Pi] = 0.f;
		float w = 0.f;

		const float k = - 1.f / (2.f * o_2);
		for (size_t j = 0; j < N.size(); ++j) {
			// NOTE: this code assume the first elements in N are the same of P
			const float wij_xy = expf(k * (
					sqr(xNormalized[i] - xNormalized[j]) +
					sqr(yNormalized[i] - yNormalized[j])));
			const float wij_c = expf(k * (
					sqr(cNormalized[i].c[0] - cNormalized[j].c[0]) +
					sqr(cNormalized[i].c[1] - cNormalized[j].c[1]) +
					sqr(cNormalized[i].c[2] - cNormalized[j].c[2])));
			const float wij = wij_xy * wij_c;
			c2[Pi] += wij * c1[N[j]];
			w += wij;
		}

		/*if (c2[Pi].IsNaN()) {
			LOG(LUX_INFO, LUX_NOERROR) << "c2[Pi] = " << c2[Pi] << " w = " << w;
		}*/

		c2[Pi] /= w;
	}

	// Compute mean and standard deviation of filtered colors
	XYZColor c2Mean;
	for (size_t i = 0; i < P.size(); ++i)
		c2Mean += c2[P[i]];

	XYZColor c2StandardDeviation;
	for (size_t i = 0; i < P.size(); ++i) {
		c2StandardDeviation += sqr(c2[P[i]] - c2Mean);
	}
	c2StandardDeviation = (cStandardDeviation / P.size()).Sqrt();

	// Outlier rejection
	for (size_t i = 0; i < P.size(); ++i) {
		const size_t Pi = P[i];
		if (c2[Pi].c[0] - cMean.c[0] > c2StandardDeviation.c[0])
			c2[Pi].c[0] = cMean.c[0];

		if (c2[Pi].c[1] - cMean.c[1] > c2StandardDeviation.c[1])
			c2[Pi].c[1] = cMean.c[1];

		if (c2[Pi].c[2] - cMean.c[2] > c2StandardDeviation.c[2])
			c2[Pi].c[2] = cMean.c[2];
	}
}

void RPF_Thread(const SampleDataGrid *sampleDataGrig,
	const int b,
	const vector<XYZColor> *c1, vector<XYZColor> *c2,
	const size_t threadIndex, const size_t threadCount) {

	RandomGenerator rng(1337 + threadIndex);
	const size_t maxSamples = sqr(b) * sampleDataGrig->avgSamplesPerPixel / 5.f;
	LOG(LUX_INFO, LUX_NOERROR) << "[thread::" << threadIndex << "] RPF max. samples " << maxSamples << ", size " << b;

	double lastPrintTime = osWallClockTime();
	for (int y = sampleDataGrig->yPixelStart + threadIndex; y <= sampleDataGrig->yPixelEnd; y += threadCount) {
		if (osWallClockTime() - lastPrintTime > 5.0) {
			LOG(LUX_INFO, LUX_NOERROR) << "[thread::" << threadIndex << "] RPF line: " << (y - sampleDataGrig->yPixelStart + 1) << "/" << sampleDataGrig->yResolution;
			lastPrintTime = osWallClockTime();
		}

		for (int x = sampleDataGrig->xPixelStart; x <= sampleDataGrig->xPixelEnd; ++x) {
			const vector<size_t> &P = sampleDataGrig->GetPixelList(x, y);

			if (P.size() > 0) {
				// Preprocess the samples and cluster them
				vector<size_t> N;
				RPF_PresprocessSamples(rng, *sampleDataGrig, x, y, b, maxSamples, N);

				// Compute feature weight
				// TODO
				float alpha = 1.f;
				float beta = 1.f;

				// Filter color samples
				RPF_FilterColorSamples(*sampleDataGrig, P, N, alpha, beta, *c1, *c2);
			}
		}
	}
}

void Recons_RPF(SampleData *sampleData, const string &outputFileName) {
	const size_t threadCount = boost::thread::hardware_concurrency();
	LOG(LUX_INFO, LUX_NOERROR) << "Hardware concurrency: " << threadCount;

	LOG(LUX_INFO, LUX_NOERROR) << "Building sample data grid...";
	SampleDataGrid sampleDataGrig(sampleData);

	// Build the sample list
	vector<XYZColor> c1(sampleData->count);
	for (size_t i = 0; i < sampleData->count; ++i)
		c1[i] = *(sampleData->GetColor(i));
	vector<XYZColor> c2(sampleData->count);

	//const int bv[4] = { 55, 35, 17, 7 };
	//const int bv[2] = { 17, 7 };
	const int bv[1] = { 7 };
	for (int bi = 0; bi < 1; ++bi) {
		const int b = bv[bi];

		// Start all threads
		LOG(LUX_INFO, LUX_NOERROR) << "Starting " << threadCount << " threads...";
		vector<boost::thread *> threads(threadCount, NULL);
		for (size_t i = 0; i < threadCount; ++i)
			threads[i] = new boost::thread(boost::bind(RPF_Thread, &sampleDataGrig, b, &c1, &c2, i, threadCount));

		// Wait for all threads
		for (size_t i = 0; i < threadCount; ++i)
			threads[i]->join();

		// Copy c'' in c'
		std::copy(c2.begin(), c2.end(), c1.begin());
	}

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

			//LOG(LUX_INFO, LUX_NOERROR) << "[" << x << ", " << y << "] = " << c;

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

void Recons_V1_POS(SampleData *sampleData, const string &outputFileName) {
	LOG(LUX_INFO, LUX_NOERROR) << "Building sample data grid...";
	SampleDataGrid sampleDataGrig(sampleData);

	LOG(LUX_INFO, LUX_NOERROR) << "Path vertex 1 position output...";
	vector<RGBColor> pixels(sampleDataGrig.xResolution * sampleDataGrig.yResolution);

	Point offset(std::numeric_limits<float>::max());
	size_t rgbi = 0;
	for (size_t y = 0; y < sampleDataGrig.yResolution; ++y) {
		for (size_t x = 0; x < sampleDataGrig.xResolution; ++x) {
			const vector<size_t> &index = sampleDataGrig.GetPixelList(x + sampleDataGrig.xPixelStart, y + sampleDataGrig.yPixelStart);

			if (index.size() > 0) {
				for (size_t i = 0; i < index.size(); ++i) {

					const SamplePathInfo *pathInfo = (SamplePathInfo *)(sampleData->GetSceneFeatures(index[i]));

					offset.x = min(offset.x, pathInfo->v1Point.x);
					offset.y = min(offset.y, pathInfo->v1Point.y);
					offset.z = min(offset.z, pathInfo->v1Point.z);

					pixels[rgbi].c[0] += pathInfo->v1Point.x;
					pixels[rgbi].c[1] += pathInfo->v1Point.y;
					pixels[rgbi].c[2] += pathInfo->v1Point.z;
				}

				pixels[rgbi] /= index.size();
			} else {
				LOG(LUX_INFO, LUX_NOERROR) << "Missing positions in pixel [" << x << ", " << y <<"]";
			}

			++rgbi;
		}
	}

	for (size_t rgbi = 0; rgbi < pixels.size(); ++rgbi) {
		pixels[rgbi].c[0] -= offset.x;
		pixels[rgbi].c[1] -= offset.y;
		pixels[rgbi].c[2] -= offset.z;
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

			if (index.size() > 0) {
				for (size_t i = 0; i < index.size(); ++i) {

					const SamplePathInfo *pathInfo = (SamplePathInfo *)(sampleData->GetSceneFeatures(index[i]));

					// +1 for avoiding negative values
					pixels[rgbi].c[0] += pathInfo->v1Normal.x + 1.f;
					pixels[rgbi].c[1] += pathInfo->v1Normal.y + 1.f;
					pixels[rgbi].c[2] += pathInfo->v1Normal.z + 1.f;
				}

				pixels[rgbi] /= index.size();
			} else {
				LOG(LUX_INFO, LUX_NOERROR) << "Missing normals in pixel [" << x << ", " << y <<"]";
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
	vector<RGBColor> pixels(sampleDataGrig.xResolution * sampleDataGrig.yResolution);
	size_t rgbi = 0;
	for (size_t y = 0; y < sampleDataGrig.yResolution; ++y) {
		for (size_t x = 0; x < sampleDataGrig.xResolution; ++x) {
			const vector<size_t> &index = sampleDataGrig.GetPixelList(x + sampleDataGrig.xPixelStart, y + sampleDataGrig.yPixelStart);

			float c[3];
			for (size_t i = 0; i < 3; ++i)
				c[i] = 0.f;

			if (index.size() > 0) {
				for (size_t i = 0; i < index.size(); ++i) {
					const SamplePathInfo *pathInfo = (SamplePathInfo *)(sampleData->GetSceneFeatures(index[i]));

					for (size_t i = 0; i < 3; ++i)
						c[i] += pathInfo->v1SurfaceColor[i];
				}

				for (size_t i = 0; i < 3; ++i)
					c[i] /= index.size();
			} else {
				LOG(LUX_INFO, LUX_NOERROR) << "Missing BSDF in pixel [" << x << ", " << y <<"]";
			}

			pixels[rgbi].c[0] = c[0];
			pixels[rgbi].c[1] = c[1];
			pixels[rgbi++].c[2] = c[2];
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

void Recons_CLUSTER(SampleData *sampleData, const string &outputFileName) {
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

			pixels[rgbi++] = colorSpace.ToRGBConstrained(c).Y();
		}
	}

	const int b = 55;
	const size_t maxSamples = sqr(b) * sampleDataGrig.avgSamplesPerPixel / 5.f;
	LOG(LUX_INFO, LUX_NOERROR) << "Cluster max. samples " << maxSamples << ", size " << b;
	RandomGenerator rng(1337);
	double lastPrintTime = osWallClockTime();
	for (int y = sampleDataGrig.yPixelStart; y <= sampleDataGrig.yPixelEnd; y += b) {
		if (osWallClockTime() - lastPrintTime > 5.0) {
			LOG(LUX_INFO, LUX_NOERROR) << "Cluster line: " << (y - sampleDataGrig.yPixelStart + 1) << "/" << sampleDataGrig.yResolution;
			lastPrintTime = osWallClockTime();
		}

		for (int x = sampleDataGrig.xPixelStart; x <= sampleDataGrig.xPixelEnd; x += b) {
			const vector<size_t> &P = sampleDataGrig.GetPixelList(x, y);

			if (P.size() > 0) {
				// Preprocess the samples and cluster them
				vector<size_t> N;
				RPF_PresprocessSamples(rng, sampleDataGrig, x, y, b, maxSamples, N);

				for (size_t i = 0; i < N.size(); ++i) {
					const float *imageXY = sampleData->GetImageXY(N[i]);
					const int xx = int(imageXY[0]) - sampleDataGrig.xPixelStart;
					const int yy = int(imageXY[1]) - sampleDataGrig.yPixelStart;

					const int index = xx + yy * sampleDataGrig.xResolution;
					pixels[index].c[1] = 0.f;
					pixels[index].c[2] = 0.f;
				}

			}
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
			("type,t", po::value< int >(), "Select the type of reconstruction (0 = BOX, 1 = RPF, 100 = V1_POS, 101 = V1_NORMAL, 102 = V1_BSDF, 103 = CLUSTER)")
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
			case TYPE_V1_POS:
				Recons_V1_POS(sampleData, outputFileName);
				break;
			case TYPE_V1_NORMAL:
				Recons_V1_NORMAL(sampleData, outputFileName);
				break;
			case TYPE_V1_BSDF:
				Recons_V1_BSDF(sampleData, outputFileName);
				break;
			case TYPE_CLUSTER:
				Recons_CLUSTER(sampleData, outputFileName);
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
