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

namespace lux
{

// Dade - return the number of concurrent threads the hardware can run or
// 0 if the information is not avilable. The implementation come from Boost 1.35
extern int osHardwareConcurrency();

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

}//namespace lux

#endif // LUX_OSFUNC_H
