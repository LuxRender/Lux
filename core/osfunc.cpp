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

#include "osfunc.h"

#ifdef WIN32
#include <windows.h>
#else

#ifdef __linux__
#include <sys/sysinfo.h>
#elif defined(__APPLE__) || defined(__FreeBSD__)
#include <sys/types.h>
#include <sys/sysctl.h>
#elif defined(__sun)
#include <unistd.h>
#endif

#endif //WIN32

namespace lux
{

// Dade - used to check and swap bytes in the network rendering code and
// other places
bool osIsLittleEndian() {
    union ShortBytes {
        short shortValue;
        unsigned char bytes[2];
    };

    ShortBytes shortTest;
    shortTest.shortValue = 1;

    return (shortTest.bytes[0] == 1);
}

template<class T> void osWriteLittleEndian(bool isLittleEndian,
	std::basic_ostream<char> &os, T value)
{
	if (isLittleEndian)
		os.write(reinterpret_cast<char *>(&value), sizeof(T));
	else {
		union ValueBytes {
			T value;
			char bytes[sizeof(T)];
		} f;
		f.value = value;

		for (const char *c = f.bytes + sizeof(T) - 1; c >= f.bytes; --c)
			os.write(c, 1);
	}
}

template<class T> T osReadLittleEndian(bool isLittleEndian,
	std::basic_istream<char> &is)
{
	union ValueBytes {
		T value;
		char bytes[sizeof(T)];
	} f;
	if (isLittleEndian) {
		is.read(reinterpret_cast<char *>(&f.value), sizeof(T));
	} else {
		for (char *c = f.bytes + sizeof(T) - 1; c >= f.bytes; --c)
			is.read(c, 1);
	}
	return f.value;
}

void osWriteLittleEndianFloat(bool isLittleEndian,
	std::basic_ostream<char> &os, float value)
{
	osWriteLittleEndian<float>(isLittleEndian, os, value);
}

float osReadLittleEndianFloat(bool isLittleEndian,
	std::basic_istream<char> &is)
{
	return osReadLittleEndian<float>(isLittleEndian, is);
}

void osWriteLittleEndianDouble(bool isLittleEndian,
	std::basic_ostream<char> &os, double value)
{
	osWriteLittleEndian<double>(isLittleEndian, os, value);
}

double osReadLittleEndianDouble(bool isLittleEndian,
	std::basic_istream<char> &is)
{
	return osReadLittleEndian<double>(isLittleEndian, is);
}

void osWriteLittleEndianInt(bool isLittleEndian,
	std::basic_ostream<char> &os, int32_t value)
{
	osWriteLittleEndian<int32_t>(isLittleEndian, os, value);
}

int32_t osReadLittleEndianInt(bool isLittleEndian,
	std::basic_istream<char> &is)
{
	return osReadLittleEndian<int32_t>(isLittleEndian, is);
}

void osWriteLittleEndianUInt(bool isLittleEndian,
	std::basic_ostream<char> &os, uint32_t value)
{
	osWriteLittleEndian<uint32_t>(isLittleEndian, os, value);
}

uint32_t osReadLittleEndianUInt(bool isLittleEndian,
	std::basic_istream<char> &is)
{
	return osReadLittleEndian<uint32_t>(isLittleEndian, is);
}

}//namespace lux
