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

#ifndef LUX_SAMPLEFILE_H
#define	LUX_SAMPLEFILE_H

#include <string>
#include <fstream>

#include <boost/thread/mutex.hpp>

namespace lux
{

class SampleFileReader;

class SampleData {
public:
	SampleData(float *data, const size_t count, const size_t infoSize);
	SampleData(float *data, SampleFileReader *reader);
	~SampleData();

	const float *GetImageXY(const size_t index) const {
		return GetSample(index);
	}

	const XYZColor *GetColor(const size_t index) const {
		const float *p = GetSample(index);
		return (XYZColor *)(&p[2 + randomParametersCount]);
	}

	const float *GetSceneFeatures(const size_t index) const {
		const float *p = GetSample(index);
		return &p[2 + 3 + randomParametersCount];
	}

	static SampleData *Merge(vector<SampleData *> samples);

	size_t count, infoSize;
	u_int randomParametersCount, sceneFeaturesCount;

private:
	const float *GetSample(const size_t index) const {
		return &data[(infoSize / sizeof(float)) * index];
	}

	float *data;
};

class SampleFileWriter {
public:
	SampleFileWriter(const string &sampleFileName);
	~SampleFileWriter();

	void WriteHeader(const u_int count) {
		file->write((const char *)&count, sizeof(u_int));
	}

	void Write(const void *data, const size_t size) {
		file->write((const char *)data, size);
	}

	boost::mutex fileMutex;
	string fileName;
	std::ofstream *file;
	bool headerWritten;
};

class SampleFileReader {
public:
	SampleFileReader(const string &sampleFileName);
	~SampleFileReader();

	SampleData *ReadAllSamples();

	size_t sampleInfoCount, sampleInfoSize;
	u_int randomParametersCount;

private:
	string fileName;
	std::ifstream *file;
};

}//namespace lux

#endif	/* LUX_SAMPLEFILE_H */
