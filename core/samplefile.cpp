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

#include "lux.h"
#include "error.h"
#include "samplefile.h"

using namespace lux;

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
	file->read((char *)&randomParametersSize, sizeof(u_int));
	LOG(LUX_DEBUG, LUX_NOERROR) << "Sample file random parameters count: " << randomParametersSize;
}

SampleFileReader::~SampleFileReader() {
	if (file) {
		file->close();
		delete file;
	}
}
