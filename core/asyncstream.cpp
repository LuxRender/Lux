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

#include "asyncstream.h"
#include <boost/bind.hpp>

using namespace lux;


static void timer_handler(boost::system::error_code *result, boost::system::error_code ec)
{
	if (ec) {
		// ignore errors caused by timer being cancelled
		if (ec != boost::asio::error::operation_aborted)
			*result = ec;
	} else {
		// timer expired, so set result to timed_out
		*result = boost::system::errc::make_error_code(boost::system::errc::timed_out);
	}
}



struct transfer_result {

	transfer_result() : ec(), bytes_transferred(0) { }

	boost::system::error_code ec;
	std::size_t bytes_transferred;
};



static void transfer_handler(transfer_result *result, boost::system::error_code ec, std::size_t bytes_transferred)
{
	result->ec = ec;
	result->bytes_transferred = bytes_transferred;
}



std::streamsize socket_device::read(char* data, std::streamsize n)
{
	transfer_result read_result;

	if (!socket.is_open())
		return -1;

	socket.get_io_service().reset();

	boost::asio::deadline_timer timer(socket.get_io_service());
	if (duration_type() < timeout_duration) {
		// non-zero timeout, set up timeout timer
		timer.expires_from_now(timeout_duration);
		timer.async_wait(
			boost::bind(
				timer_handler, 
				&this->timer_error, 
				boost::asio::placeholders::error)
		);
	}

	// queue read
	boost::asio::async_read(
		socket,
		boost::asio::buffer(data, n),
		boost::asio::transfer_at_least(1),
		boost::bind( 
			transfer_handler, 
			&read_result, 
			boost::asio::placeholders::error, 
			boost::asio::placeholders::bytes_transferred )
	);
 
	while ( socket.get_io_service().run_one() ) {
		// first check if we received any data			
		if (read_result.bytes_transferred > 0) { 
			// if so we return and ignore any errors
			timer.cancel();
			break;
		} else if (read_result.ec) { // otherwise check for errors
			timer.cancel();
			// if error is eof, return -1
			if (read_result.ec == boost::asio::error::eof)
				return -1;
			//otherwise throw
			throw boost::system::system_error(read_result.ec);
		} else if (timer_error) { // last we check if we timed out
			// if so close connection to cancel the pending io
			// and make sure future io fails
			boost::system::error_code ignored_ec;
			socket.close(ignored_ec);
			throw boost::system::system_error(timer_error);
		} 
	}

	return static_cast<std::streamsize>(read_result.bytes_transferred);
}



std::streamsize socket_device::write(const char* data, std::streamsize n)
{
	transfer_result write_result;

	if (!socket.is_open())
		return -1;

	socket.get_io_service().reset();

	boost::asio::deadline_timer timer(socket.get_io_service());

	if (duration_type() < timeout_duration) {
		// non-zero timeout, set up timeout timer
		timer.expires_from_now(timeout_duration);
		timer.async_wait(
			boost::bind(
				timer_handler, 
				&this->timer_error, 
				boost::asio::placeholders::error)
		);
	}

	// queue write
	boost::asio::async_write(
		socket,
		boost::asio::buffer(data, n),
		boost::asio::transfer_at_least(1),
		boost::bind( 
			transfer_handler, 
			&write_result, 
			boost::asio::placeholders::error, 
			boost::asio::placeholders::bytes_transferred )
	);
 
	while ( socket.get_io_service().run_one() ) {
		// first check if we wrote any data			
		if (write_result.bytes_transferred > 0) { 
			// if so we return and ignore any errors
			timer.cancel();
			break;
		} else if (write_result.ec) { // otherwise check for errors
			timer.cancel();
			// unlike read, eof is a fatal error as well
			throw boost::system::system_error(write_result.ec);
		} else if (timer_error) { // last we check if we timed out
			// if so close connection to cancel the pending io
			// and make sure future io fails
			boost::system::error_code ignored_ec;
			socket.close(ignored_ec);
			throw boost::system::system_error(timer_error);
		} 
	}

	return static_cast<std::streamsize>(write_result.bytes_transferred);
}

boost::iostreams::stream_offset socket_device::seek(boost::iostreams::stream_offset off, std::ios_base::seekdir way) {
	if (way == std::ios_base::cur && off == 0) {
		// if seeking 0 chars from current, allow
		return 0;
	}
	// otherwise fail
	return -1;
}

void socket_device::close() {
	boost::system::error_code ignored_ec;
	socket.close(ignored_ec);
}