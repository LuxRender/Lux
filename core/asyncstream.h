/***************************************************************************
 *   Copyright (C) 1998-2013 by authors (see AUTHORS.txt)                  *
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

#ifndef LUX_ASYNCSTREAM_H
#define LUX_ASYNCSTREAM_H
// asyncstream.h*

#include "lux.h"

#include <boost/asio.hpp>
#include <boost/iostreams/stream.hpp>

namespace lux 
{

class socket_device {
public:
	typedef char  char_type;
	typedef boost::iostreams::seekable_device_tag category;
	typedef boost::asio::ip::tcp::socket socket_type;
	typedef boost::posix_time::time_duration duration_type;

	socket_device(socket_type& socket_, duration_type timeout_)
			: socket(socket_), timeout_duration(timeout_) {
	}

	socket_device(socket_type& socket_)
			: socket(socket_), timeout_duration() {
	}

	~socket_device() {
		close();
	}

	socket_type& get_socket() {
		return socket;
	}

	duration_type timeout() const {
		return timeout_duration;
	}

	void timeout(const duration_type& duration) {
		timeout_duration = duration;
	}

	std::streamsize read(char* s, std::streamsize n);

	std::streamsize write(const char* s, std::streamsize n);

	boost::iostreams::stream_offset seek(boost::iostreams::stream_offset off, std::ios_base::seekdir way);

	void close();

private:

	socket_type& socket;
	duration_type timeout_duration;
	boost::system::error_code timer_error;
};


} // namespace lux

#endif // LUX_ASYNCSTREAM