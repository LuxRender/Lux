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

#ifndef LUX_OSFUNC_H
#define LUX_OSFUNC_H

#include <boost/cstdint.hpp>
using boost::int32_t;
using boost::uint32_t;
#include <istream>
#include <ostream>

#include <boost/interprocess/detail/atomic.hpp>

#if defined(__linux__) || defined(__APPLE__) || defined(__CYGWIN__)
#include <stddef.h>
#include <sys/time.h>
#elif defined (WIN32)
#include <windows.h>
#else
#error "Unsupported Platform !!!"
#endif

namespace lux
{

// Dade - used to check and swap bytes in the network rendering code and
// other places
extern bool osIsLittleEndian();
extern void osWriteLittleEndianFloat(bool isLittleEndian,
		std::basic_ostream<char> &os, float value);
extern float osReadLittleEndianFloat(bool isLittleEndian,
		std::basic_istream<char> &is);
extern void osWriteLittleEndianDouble(bool isLittleEndian,
		std::basic_ostream<char> &os, double value);
extern double osReadLittleEndianDouble(bool isLittleEndian,
		std::basic_istream<char> &is);
extern void osWriteLittleEndianInt(bool isLittleEndian,
		std::basic_ostream<char> &os, int32_t value);
extern int32_t osReadLittleEndianInt(bool isLittleEndian,
		std::basic_istream<char> &is);
extern void osWriteLittleEndianUInt(bool isLittleEndian,
		std::basic_ostream<char> &os, uint32_t value);
extern uint32_t osReadLittleEndianUInt(bool isLittleEndian,
		std::basic_istream<char> &is);

inline double osWallClockTime() {
#if defined(__linux__) || defined(__APPLE__) || defined(__CYGWIN__)
	struct timeval t;
	gettimeofday(&t, NULL);

	return t.tv_sec + t.tv_usec / 1000000.0;
#elif defined (WIN32)
	return GetTickCount() / 1000.0;
#else
#error "Unsupported Platform !!!"
#endif
}

//------------------------------------------------------------------------------
// Atomic ops
//------------------------------------------------------------------------------

inline void osAtomicAdd(float *val, const float delta) {
	union bits {
		float f;
		boost::uint32_t i;
	};

	bits oldVal, newVal;

	do {
#if (defined(__i386__) || defined(__amd64__))
		__asm__ __volatile__("pause\n");
#endif

		oldVal.f = *val;
		newVal.f = oldVal.f + delta;
	} while (boost::interprocess::detail::atomic_cas32(reinterpret_cast<boost::uint32_t*>(val), newVal.i, oldVal.i) != oldVal.i);
}

inline void osAtomicAdd(unsigned int *val, const unsigned int delta) {
#if defined(WIN32)
	boost::uint32_t oldVal, newVal;
	do
	{
#if (defined(__i386__) || defined(__amd64__))
		 __asm__ __volatile__("pause\n");
#endif
		oldVal = *val;
		newVal = oldVal + delta;
	} while (boost::interprocess::detail::atomic_cas32(reinterpret_cast<boost::uint32_t*>(val), newVal, oldVal) != oldVal);
#else
	boost::interprocess::detail::atomic_add32(((boost::uint32_t *)val), (boost::uint32_t)delta);
#endif
}

/**
 * Atomically increments a 32bit variable
 * @return Previous value, before increment
 */
inline unsigned int osAtomicInc(unsigned int *val) {
	return boost::interprocess::detail::atomic_inc32(reinterpret_cast<boost::uint32_t*>(val));
}

/**
 * Atomically reads a 32bit variable
 * @return Value read
 */
inline unsigned int osAtomicRead(unsigned int *val) {
	return boost::interprocess::detail::atomic_read32(reinterpret_cast<boost::uint32_t*>(val));
}

/**
 * Atomically writes a 32bit variable
 */
inline void osAtomicWrite(unsigned int *val, unsigned int newVal) {
	boost::interprocess::detail::atomic_write32(reinterpret_cast<boost::uint32_t*>(val), static_cast<boost::uint32_t>(newVal));
}

}//namespace lux

#endif // LUX_OSFUNC_H
