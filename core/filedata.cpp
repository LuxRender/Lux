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

// filedata.cpp*

#include <boost/iostreams/copy.hpp>
#include <boost/iostreams/device/file.hpp>
#include <boost/iostreams/filtering_stream.hpp>
#include <boost/iostreams/filter/zlib.hpp>
#include <string>

#include "error.h"
#include "lux.h"
#include "paramset.h"
#include "filedata.h"

namespace lux
{

// Base64 Decode from http://www.adp-gmbh.ch/cpp/common/base64.html
/*
   (parts of) base64.cpp and base64.h

   Copyright (C) 2004-2008 René Nyffenegger

   This source code is provided 'as-is', without any express or implied
   warranty. In no event will the author be held liable for any damages
   arising from the use of this software.

   Permission is granted to anyone to use this software for any purpose,
   including commercial applications, and to alter it and redistribute it
   freely, subject to the following restrictions:

   1. The origin of this source code must not be misrepresented; you must not
      claim that you wrote the original source code. If you use this source code
      in a product, an acknowledgment in the product documentation would be
      appreciated but is not required.

   2. Altered source versions must be plainly marked as such, and must not be
      misrepresented as being the original source code.

   3. This notice may not be removed or altered from any source distribution.

   René Nyffenegger rene.nyffenegger@adp-gmbh.ch

*/
static const std::string base64_chars =
	"ABCDEFGHIJKLMNOPQRSTUVWXYZ"
	"abcdefghijklmnopqrstuvwxyz"
	"0123456789+/";

static inline bool is_base64(unsigned char c) {
	return (isalnum(c) || (c == '+') || (c == '/'));
}

std::string base64_decode(std::string const& encoded_string) {
	int in_len = encoded_string.size();
	int i = 0;
	int j = 0;
	int in_ = 0;
	unsigned char char_array_4[4], char_array_3[3];
	std::string ret;

	while (in_len-- && ( encoded_string[in_] != '=') && is_base64(encoded_string[in_])) {
		char_array_4[i++] = encoded_string[in_]; in_++;
		if (i ==4) {
			for (i = 0; i <4; i++)
			char_array_4[i] = base64_chars.find(char_array_4[i]);

			char_array_3[0] = (char_array_4[0] << 2) + ((char_array_4[1] & 0x30) >> 4);
			char_array_3[1] = ((char_array_4[1] & 0xf) << 4) + ((char_array_4[2] & 0x3c) >> 2);
			char_array_3[2] = ((char_array_4[2] & 0x3) << 6) + char_array_4[3];

			for (i = 0; (i < 3); i++)
				ret += char_array_3[i];
			i = 0;
		}
	}

	if (i) {
		for (j = i; j <4; j++)
			char_array_4[j] = 0;

		for (j = 0; j <4; j++)
			char_array_4[j] = base64_chars.find(char_array_4[j]);

		char_array_3[0] = (char_array_4[0] << 2) + ((char_array_4[1] & 0x30) >> 4);
		char_array_3[1] = ((char_array_4[1] & 0xf) << 4) + ((char_array_4[2] & 0x3c) >> 2);
		char_array_3[2] = ((char_array_4[2] & 0x3) << 6) + char_array_4[3];

		for (j = 0; (j < i - 1); j++) ret += char_array_3[j];
	}

	return ret;
}

bool FileData::decode(const ParamSet &tp, const std::string param_name)
{
	u_int nlines = 0;
	std::stringstream b64_compressed_filedata_buf;
	std::string b64_compressed_filedata;
	const std::string* b64_compressed_lines = tp.FindString(param_name + "_data", &nlines);
	const std::string bencoded_filename = tp.FindOneString(param_name, "");
	
	// concantenate lines into b64_compressed_filedata_buf
	for(u_int i=0; i<nlines; i++)
	{
		b64_compressed_filedata_buf << b64_compressed_lines[i];
	}
	b64_compressed_filedata = b64_compressed_filedata_buf.str().c_str();
	b64_compressed_filedata_buf.str("");
	
	if (b64_compressed_filedata.size() > 0 && bencoded_filename != "")
	{
		LOG(LUX_DEBUG,LUX_NOERROR)<<"Decoding embedded filedata:";
		LOG(LUX_DEBUG,LUX_NOERROR)<<"\tinput b64 compressed data length: "<< b64_compressed_filedata.size();

		// base64 decode
		const std::string compressed_filedata = base64_decode(b64_compressed_filedata);
		LOG(LUX_DEBUG,LUX_NOERROR)<<"\tcompressed data length: "<< compressed_filedata.size();

		// set up a filter stack
		boost::iostreams::filtering_ostream file_out;

		// decompress
		file_out.push(boost::iostreams::zlib_decompressor());

		// write to file
		boost::iostreams::file_sink file_out_sink(bencoded_filename.c_str(), std::ios_base::out | std::ios_base::binary);
		file_out.push(file_out_sink);

		// send input through filter stack
		boost::iostreams::copy(boost::make_iterator_range(compressed_filedata), file_out);

		LOG(LUX_INFO,LUX_NOERROR)<< "Decoded file "<<bencoded_filename<<" to disk";
		return true;
	}
	return false;
}

} // namespace lux
