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

#ifndef LUX_STREAMIO_H
#define LUX_STREAMIO_H
// streamio.h*

#include "lux.h"

#include <boost/limits.hpp>
#include <boost/iostreams/stream.hpp>

namespace lux
{

class multibuffer_device {
public:
    typedef char  char_type;
	typedef boost::iostreams::seekable_device_tag  category;

	multibuffer_device(size_t _buffer_capacity = 32*1024*1024) 
			: buffer_capacity(_buffer_capacity), end(0), pos(0) {
	}

	~multibuffer_device() {
	}

    std::streamsize read(char* s, std::streamsize n);

    std::streamsize write(const char* s, std::streamsize n);

	boost::iostreams::stream_offset seek(boost::iostreams::stream_offset off, std::ios_base::seekdir way);

private:
	void grow() {
		std::vector<char_type> v;
		buffers.push_back(v);
		buffers.back().reserve(buffer_capacity);
	}

	size_t buffer_capacity;
	std::vector<std::vector<char_type> > buffers;
	std::streampos end;
	std::streampos pos;
};

} // namespace lux

#endif // LUX_STREAMIO_H
