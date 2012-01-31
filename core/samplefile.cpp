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

#include <string.h>

#include <boost/filesystem/operations.hpp>

#include "lux.h"
#include "error.h"
#include "samplefile.h"
#include "sampling.h"

using namespace lux;
namespace bf = boost::filesystem;

SampleData::SampleData(float *d, const size_t c, const size_t s) :
	count(c), infoSize(s), data(d) {
	randomParametersCount = (infoSize - 2 * sizeof(float) - sizeof(XYZColor) -
			sizeof(SamplePathInfo)) / sizeof(float);
	sceneFeaturesCount = sizeof(SamplePathInfo) / sizeof(float);
}

SampleData::SampleData(float *d, SampleFileReader *reader) :
	count(reader->sampleInfoCount), infoSize(reader->sampleInfoSize), data(d) {
	randomParametersCount = (infoSize - 2 * sizeof(float) - sizeof(XYZColor) -
			sizeof(SamplePathInfo)) / sizeof(float);
	sceneFeaturesCount = sizeof(SamplePathInfo) / sizeof(float);
}

SampleData::~SampleData() {
	delete[] data;
}

SampleData *SampleData::Merge(vector<SampleData *> samples) {
	if (samples.size() == 1)
		return samples[0];

	size_t size = 0;
	size_t count = 0;
	const size_t infoSize = samples[0]->infoSize;
	for (size_t i = 0; i < samples.size(); ++i) {
		LOG(LUX_DEBUG, LUX_NOERROR) << "SampleData Merge " << i << ": " <<
				samples[i]->infoSize << " [" << samples[i]->count << "]";

		if (samples[i]->infoSize != infoSize)
			throw std::runtime_error("Different infoSize in a SampleData Merge");

		size += samples[i]->infoSize * samples[i]->count;
		count += samples[i]->count;
	}

	float *data = new float[size];
	float *p = data;
	for (size_t i = 0; i < samples.size(); ++i) {
		const size_t delta = samples[i]->infoSize * samples[i]->count;
		memcpy(p, samples[i]->data, delta);
		p += delta;

		delete samples[i];
	}

	return new SampleData(data, count, infoSize);
}

//------------------------------------------------------------------------------

SampleFileWriter::SampleFileWriter(const string &sampleFileName) {
	fileName = sampleFileName;

	// Open the file
	LOG(LUX_DEBUG, LUX_NOERROR) << "Opening samples file: " << sampleFileName;
	file = new std::ofstream(fileName.c_str(), std::ios_base::out | std::ios_base::binary);

	if(!file)
		throw std::runtime_error("Cannot open sample file '" + fileName + "' for writting");
	file->exceptions(std::ifstream::eofbit | std::ifstream::failbit | std::ifstream::badbit);

	// Write file type tag
	file->write("SEPL", 4);
	headerWritten = false;
}

SampleFileWriter::~SampleFileWriter() {
	if (file) {
		file->close();
		delete file;
	}
}

//------------------------------------------------------------------------------

SampleFileReader::SampleFileReader(const string &sampleFileName) {
	fileName = sampleFileName;

	// Check the file size
	const bf::path filePath = bf::system_complete(bf::path(fileName));
	if (!bf::exists(filePath))
			throw std::runtime_error("Sample file '" + fileName + "' doesn't exists");
	if (!bf::is_regular(filePath))
			throw std::runtime_error("Sample file '" + fileName + "' isn't a regular file");
	const size_t fileSize = bf::file_size(filePath);
	LOG(LUX_DEBUG, LUX_NOERROR) << "Sample file size: " << fileSize << " bytes";

	// Open the file
	LOG(LUX_DEBUG, LUX_NOERROR) << "Opening samples file: " << sampleFileName;
	file = new std::ifstream(fileName.c_str(), std::ios_base::in | std::ios_base::binary);

	if(!file)
		throw std::runtime_error("Cannot open sample file '" + fileName + "' for reading");
	file->exceptions(std::ifstream::eofbit | std::ifstream::failbit | std::ifstream::badbit);

	// Read the tag
	char buf[4];
	file->read(buf, 4);
	LOG(LUX_DEBUG, LUX_NOERROR) << "Sample file tag: [" << buf[0] << buf[1] << buf[2] << buf[3] << "]";

	if ((buf[0] != 'S') || (buf[1] != 'E') || (buf[2] != 'P') || (buf[3] != 'L'))
		throw std::runtime_error("Wrong sample file tag");

	// Read the header information
	file->read((char *)&randomParametersCount, sizeof(u_int));
	LOG(LUX_DEBUG, LUX_NOERROR) << "Sample file random parameters count: " << randomParametersCount;

	sampleInfoSize =
			// Random variables
			(2 + randomParametersCount) * sizeof(float) +
			// XYZColor
			sizeof(XYZColor) +
			// SamplePathInfo
			sizeof(SamplePathInfo);
	LOG(LUX_DEBUG, LUX_NOERROR) << "Sample info size: " << sampleInfoSize << " bytes";
	sampleInfoCount = (fileSize - 4 - sizeof(u_int)) / sampleInfoSize;
	LOG(LUX_DEBUG, LUX_NOERROR) << "Sample info count: " << sampleInfoCount;
}

SampleFileReader::~SampleFileReader() {
	if (file) {
		file->close();
		delete file;
	}
}

SampleData *SampleFileReader::ReadAllSamples() {
	float *data = new float[sampleInfoCount * sampleInfoSize / sizeof(float)];

	file->read((char *)data, sampleInfoCount * sampleInfoSize);

	// Check data sanity
	size_t imageXY_nans = 0;
	size_t imageXY_infs = 0;
	size_t randomParameters_nans = 0;
	size_t randomParameters_infs = 0;
	size_t color_nans = 0;
	size_t color_infs = 0;
	size_t sceneFeatures_nans = 0;
	size_t sceneFeatures_infs = 0;

	float *p = data;
	for (size_t i = 0; i < sampleInfoCount; i++) {
		// Image X Y
		if (isnan(p[0]))
			++imageXY_nans;
		if (isinf(p[0]))
			++imageXY_infs;
		if (isnan(p[1]))
			++imageXY_nans;
		if (isinf(p[1]))
			++imageXY_infs;

		// Random parameters
		for (size_t j = 0; j < randomParametersCount; j++) {
			if (isnan(p[2 + j]))
				++randomParameters_nans;
			if (isinf(p[2 + j]))
				++randomParameters_infs;
		}

		// XYZColor
		if (isnan(p[2 + randomParametersCount]))
			++color_nans;
		if (isinf(p[2 + randomParametersCount]))
			++color_infs;
		if (isnan(p[2 + randomParametersCount + 1]))
			++color_nans;
		if (isinf(p[2 + randomParametersCount + 1]))
			++color_infs;
		if (isnan(p[2 + randomParametersCount + 2]))
			++color_nans;
		if (isinf(p[2 + randomParametersCount + 2]))
			++color_infs;

		// Scene features
		for (size_t j = 0; j < sizeof(SamplePathInfo) / sizeof(float); j++) {
			if (isnan(p[2 + randomParametersCount + 3 + j]))
				++sceneFeatures_nans;
			if (isinf(p[2 + randomParametersCount + 3 + j]))
				++sceneFeatures_infs;
		}

		p += sampleInfoSize / sizeof(float);
	}

	LOG(LUX_INFO, LUX_NOERROR) << "Sample info Image XY NaNs: " << imageXY_nans;
	LOG(LUX_INFO, LUX_NOERROR) << "Sample info Image XY Infs: " << imageXY_infs;
	LOG(LUX_INFO, LUX_NOERROR) << "Sample info Random parameters NaNs: " << randomParameters_nans;
	LOG(LUX_INFO, LUX_NOERROR) << "Sample info Random parameters Infs: " << randomParameters_infs;
	LOG(LUX_INFO, LUX_NOERROR) << "Sample info XYZ color NaNs: " << color_nans;
	LOG(LUX_INFO, LUX_NOERROR) << "Sample info XYZ color Infs: " << color_infs;
	LOG(LUX_INFO, LUX_NOERROR) << "Sample info Scene features color NaNs: " << sceneFeatures_nans;
	LOG(LUX_INFO, LUX_NOERROR) << "Sample info Scene features Infs: " << sceneFeatures_infs;

	return new SampleData(data, this);
}
