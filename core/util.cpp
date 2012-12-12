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

// util.cpp*
#include "lux.h"
#include "api.h"
#include "error.h"
#include "streamio.h"

#include <map>
using std::map;
#include <iomanip>
using std::endl;
using std::setw;
using std::left;
using std::setfill;
using std::setprecision;
using std::flush;
using std::cout;

namespace lux
{

/* string hashing function
 * An algorithm produced by Professor Daniel J. Bernstein and shown first to the world on the usenet newsgroup comp.lang.c. It is one of the most efficient hash functions ever published.
 */
unsigned int DJBHash(const std::string& str)
{
   unsigned int hash = 5381;

   for(std::size_t i = 0; i < str.length(); i++)
   {
      hash = ((hash << 5) + hash) + static_cast<unsigned int>(str[i]);
   }

   return hash;
}

// multibuffer_device Method Definitions
std::streamsize multibuffer_device::read(char* s, std::streamsize n)
{
	namespace io = boost::iostreams;
	// Read up to n characters from the underlying data source
	// into the buffer s, returning the number of characters
	// read; return -1 to indicate EOF
	io::stream_offset amt = io::position_to_offset(end) - io::position_to_offset(pos);
	std::streamsize result = static_cast<std::streamsize>((min)(static_cast<io::stream_offset>(n), amt));
	if (result != 0) {
		std::size_t i = static_cast<size_t>(io::position_to_offset(pos) / buffer_capacity);

		std::streamsize cur = static_cast<std::streamsize>(io::position_to_offset(pos) - static_cast<io::stream_offset>(i) * buffer_capacity);

		std::streamsize remaining = result;
		while (remaining > 0) {
			const std::vector<char_type>& buf(buffers[i]);

			std::streamsize subsize = (min)(static_cast<std::streamsize>(buf.size() - cur), remaining);

			std::copy(buf.begin() + cur, 
						buf.begin() + cur + subsize, 
						s );

			pos += subsize;
			s += subsize;
			remaining -= subsize;
			cur = 0;
			i++;
		}

		return result;
	} else {
		return -1; // EOF
	}
}

std::streamsize multibuffer_device::write(const char* s, std::streamsize n)
{
	namespace io = boost::iostreams;
	// Write up to n characters to the underlying 
	// data sink into the buffer s, returning the 
	// number of characters written
	io::stream_offset start = io::position_to_offset(pos);
	try {
		std::size_t i = static_cast<size_t>(start / buffer_capacity);

		if (i >= buffers.size())
			grow();

		std::streamsize cur = static_cast<std::streamsize>(start - static_cast<io::stream_offset>(i) * buffer_capacity);

		// limit size of stream to capacity of std::streamsize
		io::stream_offset amt = (std::numeric_limits<std::streamsize>::max)() - io::position_to_offset(start);
		std::streamsize remaining = static_cast<std::streamsize>((min)(static_cast<io::stream_offset>(n), amt));

		while (remaining > 0) {
			std::vector<char_type>& buf(buffers[i]);

			std::streamsize subsize = (min)(static_cast<std::streamsize>(buf.capacity() - cur), remaining);

			if (static_cast<std::streamsize>(buf.size()) < cur + subsize)
				buf.resize(cur + subsize);

			std::copy(s, s + subsize, buf.begin() + cur);

			pos += subsize;
			end = (max)(end, pos);
			s += subsize;
			remaining -= subsize;
			if (remaining <= 0)
				break;

			cur = 0;
			i++;
			if (i >= buffers.size())
				grow();
		}

	} catch (std::bad_alloc &e) {
		// ignore
	}
	return static_cast<std::streamsize>(io::position_to_offset(pos) - start);
}

boost::iostreams::stream_offset multibuffer_device::seek(boost::iostreams::stream_offset off, std::ios_base::seekdir way)
{
	namespace io = boost::iostreams;
	// Seek to position off and return the new stream 
	// position. The argument way indicates how off is
	// interpretted:
	//    - std::ios_base::beg indicates an offset from the 
	//      sequence beginning 
	//    - std::ios_base::cur indicates an offset from the 
	//      current character position 
	//    - std::ios_base::end indicates an offset from the 
	//      sequence end 
	// Determine new value of pos
	io::stream_offset next;
	if (way == std::ios_base::beg) {
		next = off;
	} else if (way == std::ios_base::cur) {
		next = io::position_to_offset(pos) + off;
	} else if (way == std::ios_base::end) {
		next = io::position_to_offset(end) + off;
	} else {
		throw std::ios_base::failure("bad seek direction");
	}

	// Check for errors
	if (next < 0 || next > io::position_to_offset(end))
		throw std::ios_base::failure("bad seek offset");

	pos = io::offset_to_position(next);
	return pos;
}

LUX_EXPORT nullstream nullStream;

}


