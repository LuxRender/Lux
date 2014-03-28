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

#ifndef _TIGER_HASH_H_
#define _TIGER_HASH_H_
// tigerhash.h*

#include <iostream>
#include <fstream>
#include <string>
#ifdef __APPLE__
#include <tr1/cstdint>
#else
#include <cstdint>
#endif
#include <boost/array.hpp>
#include "error.h"

// TODO - Tiger hash implementation as not been verified on big endian platforms

namespace lux {

class tigerhash {
public:
	typedef boost::array<uint8_t, 3*8> digest_type;

	tigerhash();
	tigerhash(tigerhash &other);

	void restart();
	void update(const char* data, uint64_t length);
	digest_type end_message() const;
	
private:
	void compress(const uint8_t *str, uint64_t state[3]) const;

	uint64_t count;
	uint8_t buffer[64];
	uint64_t res[3];

	static uint64_t table[4*256];
};

std::string digest_string(const tigerhash::digest_type &d);

template <class HashAlgorithm>
class streamhasher {
public:
	typedef typename HashAlgorithm::digest_type result_type;

	streamhasher& operator<< (std::streambuf* sb) {
		boost::array<char, 256 * 1024> block;

		for( ; ; ) {
			std::streamsize r = sb->sgetn(&block[0], block.size());
			if (r < 1)
				break;
			h.update(&block[0], r);
		}

		return *this;
	}

	result_type operator()() const {
		return h.end_message();
	}
private:
	HashAlgorithm h;
};


template <class HashAlgorithm>
class stringhasher {
public:
	typedef typename HashAlgorithm::digest_type result_type;

	stringhasher& operator<< (const std::string& s) {
		h.update(s.c_str(), s.length());

		return *this;
	}

	result_type operator()() const {
		return h.end_message();
	}
private:
	HashAlgorithm h;
};

template <class HashAlgorithm>
typename HashAlgorithm::digest_type string_hash(const std::string &s) {
	stringhasher<HashAlgorithm> hasher;
	hasher << s;
	return hasher();
}

template <class HashAlgorithm>
typename HashAlgorithm::digest_type file_hash(const std::string &filename) {
	std::ifstream fs(filename.c_str(), std::ifstream::in | std::ifstream::binary);

	streamhasher<HashAlgorithm> hasher;

	hasher << fs.rdbuf();

	if (fs.fail())
		LOG( LUX_ERROR,LUX_SYSTEM) << "Error hashing file '" << filename << "'";

	return hasher();
}

} // namespace

std::ostream &operator<<(std::ostream &os, const lux::tigerhash::digest_type &d);


#endif