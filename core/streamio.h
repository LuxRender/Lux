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

#include <streambuf>
#include <boost/limits.hpp>
#include <boost/iostreams/stream.hpp>
#include <boost/utility/base_from_member.hpp>
#include <boost/ref.hpp>

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


template <
	typename Device,
	typename charT = typename Device::char_type,
	typename traits = std::char_traits<charT> >
class device_streambuf : public std::basic_streambuf<charT, traits>
{
public:
	typedef traits traits_type;
	typedef typename traits_type::int_type int_type;
	typedef typename traits_type::pos_type pos_type;
	typedef typename traits_type::off_type off_type;

	explicit device_streambuf(Device& d, int_type buf_size = default_buffer_size)
		: device(d)
	{
		buffer_size = std::max(buf_size, 0);

		init_buffers();
	}

	virtual ~device_streambuf() {
		sync();
	}

	Device& get_device() {
		return device;
	}

	void close() {
		device.close();
	}

    Device& operator*() { return *this->device; }
    Device* operator->() { return &*this->device; }

protected:

	virtual int_type underflow()
	{
		if (this->gptr() < this->egptr())
			// buffer not exhausted
			return traits_type::eof();

		// fill buffer
		std::streamsize bytes_transferred = device.read(&get_buffer[0] + putback_max, get_buffer.size() - putback_max);
		if (bytes_transferred < 0)
			return traits_type::eof();

		this->setg(&get_buffer[0], &get_buffer[0] + putback_max,
			&get_buffer[0] + putback_max + bytes_transferred);

		return traits_type::to_int_type(*this->gptr());
	}

	// This is called when the buffer needs to be flushed.
	virtual int sync()	{
		return traits_type::eq_int_type(this->overflow(traits_type::eof()),
			traits_type::eof()) ? -1 : 0;
	} 

	virtual int_type overflow(int_type c)
	{
		// Send all data in the output buffer.
		charT* pb = this->pbase();
		charT* pe = this->pptr();
		while (pb < pe)
		{
			std::streamsize bytes_transferred = device.write(pb, static_cast<std::streamsize>(pe - pb));

			if (bytes_transferred < 0)
				return traits_type::eof();

			pb += bytes_transferred;
		}
		this->setp(&put_buffer[0], &put_buffer[0] + put_buffer.size());

		if (traits_type::eq_int_type(c, traits_type::eof()))
			// new character is eof, don't add it to output buffer
			return traits_type::not_eof(c);

		*this->pptr() = traits_type::to_char_type(c);
		this->pbump(1);
		return c;
	}

private:
	void init_buffers() {
		get_buffer.resize(buffer_size + putback_max);
		put_buffer.resize(buffer_size);

		this->setg(&get_buffer[0],
			&get_buffer[0] + putback_max,
			&get_buffer[0] + putback_max);

		this->setp(&put_buffer[0], &put_buffer[0] + buffer_size - 1);
	}


	enum { putback_max = 8 };
	enum { default_buffer_size = 4096 };

	Device& device;

	int_type buffer_size;

	// Input buffer
	std::vector<charT> get_buffer;
	// Output buffer
	std::vector<charT> put_buffer;
};


template <
	typename Device,
	typename charT = typename Device::char_type, 
	typename traits = std::char_traits<charT> >
class device_iostream :
	private boost::base_from_member<device_streambuf<Device, charT, traits> >,
	public std::basic_iostream<charT> 
{
public:
	typedef traits traits_type;
	typedef typename traits_type::int_type int_type;
	typedef typename traits_type::pos_type pos_type;
	typedef typename traits_type::off_type off_type;

	typedef boost::base_from_member<device_streambuf<Device, charT, traits> > pbase_type;
	typedef std::basic_iostream<charT> base_type;

	explicit device_iostream(Device& d)
		: pbase_type(boost::ref(d)), base_type(&this->member)
	{
	}

	explicit device_iostream(Device& d, int_type buffer_size)
		: pbase_type(boost::ref(d), buffer_size), base_type(&this->member)
	{
	}

	void close() {
		rdbuf()->close();
	}

	device_streambuf<Device, charT, traits>* rdbuf() const {
		return const_cast<device_streambuf<Device, charT, traits>*>(&this->member);
	}

    Device& operator*() { return rdbuf()->get_device(); }
    Device* operator->() { return &rdbuf()->get_device(); }
};

} // namespace lux

#endif // LUX_STREAMIO_H
