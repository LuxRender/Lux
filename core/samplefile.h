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

class SampleData {
public:
	SampleData(float *data, const size_t count, const size_t infoSize);
	~SampleData();

	size_t GetCount() const { return count; }
	u_int GetRandomParametersCount()  const { return randomParametersCount; }

	static SampleData *Merge(vector<SampleData *> samples);

private:
	float *data;
	size_t count, infoSize;
	u_int randomParametersCount;
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
