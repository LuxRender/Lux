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
 *   Lux Renderer website : http://www.luxrender.org                       *
 ***************************************************************************/
#include "lux.h"
#include "imagereader.h"
#include "texturecolor.h"
#include "error.h"

#include <boost/filesystem.hpp>

namespace lux {

ImageData::~ImageData()
{
	switch (pixel_type_) {
	case UNSIGNED_CHAR_TYPE:
		if (noChannels_ == 1)
			delete[] (TextureColor<unsigned char, 1>*)data_;
		else if (noChannels_ == 3)
			delete[] (TextureColor<unsigned char, 3>*)data_;
		else if (noChannels_ == 4)
			delete[] (TextureColor<unsigned char, 4>*)data_;
		break;
	case UNSIGNED_SHORT_TYPE:
		if (noChannels_ == 1)
			delete[] (TextureColor<unsigned short, 1>*)data_;
		else if (noChannels_ == 3)
			delete[] (TextureColor<unsigned short, 3>*)data_;
		else if (noChannels_ == 4)
			delete[] (TextureColor<unsigned short, 4>*)data_;
		break;
	case FLOAT_TYPE:
		if (noChannels_ == 1)
			delete[] (TextureColor<float, 1>*)data_;
		else if (noChannels_ == 3)
			delete[] (TextureColor<float, 3>*)data_;
		else if (noChannels_ == 4)
			delete[] (TextureColor<float, 4>*)data_;
		break;
	}
}

void ImageData::data_scale()
{
	float sc[4];
	sc[0] = 0.0000114011578f;
	sc[1] = 0.0000144624098f;
	sc[2] = 0.0000149589504f;
	sc[3] = 1.f;
	if (noChannels_ == 1) {
		if (pixel_type_ == UNSIGNED_CHAR_TYPE) {
			TextureColor<unsigned char, 1> *ret = static_cast<TextureColor<unsigned char, 1> *>(data_);
			for ( int i = 0 ; i < width_*height_ ; i++ ) {
				for ( int j = 0 ; j < noChannels_ ; j++ )
					ret[i].c[j] = ret[i].c[j]*sc[j];
			}
			data_ = ret;
		} else if (pixel_type_ == FLOAT_TYPE) {
			TextureColor<float, 1> *ret = static_cast<TextureColor<float, 1> *>(data_);
			for ( int i = 0 ; i < width_*height_ ; i++ ) {
				for ( int j = 0 ; j < noChannels_ ; j++ )
					ret[i].c[j] = ret[i].c[j]*sc[j];
			}
			data_ = ret;
		} else if (pixel_type_ == UNSIGNED_SHORT_TYPE) {
			TextureColor<unsigned short, 1> *ret = static_cast<TextureColor<unsigned short, 1> *>(data_);
			for ( int i = 0 ; i < width_*height_ ; i++ ) {
				for ( int j = 0 ; j < noChannels_ ; j++ )
					ret[i].c[j] = ret[i].c[j]*sc[j];
			}
			data_ = ret;
		}
	} else if (noChannels_ == 3) {
		if (pixel_type_ == UNSIGNED_CHAR_TYPE) {
			TextureColor<unsigned char, 3> *ret = static_cast<TextureColor<unsigned char, 3> *>(data_);
			for ( int i = 0 ; i < width_*height_ ; i++ ) {
				for ( int j = 0 ; j < noChannels_ ; j++ )
					ret[i].c[j] = ret[i].c[j]*sc[j];
			}
			data_ = ret;
		} else if (pixel_type_ == FLOAT_TYPE) {
			TextureColor<float, 3u > *ret=static_cast<TextureColor<float, 3u >*>(data_);
			for ( int i = 0 ; i < width_*height_ ; i++ ) {
				for ( int j = 0 ; j < noChannels_ ; j++ )
					ret[i].c[j] = ret[i].c[j]*sc[j];
			}
			data_ = ret;
		} else if (pixel_type_ == UNSIGNED_SHORT_TYPE) {
			TextureColor<unsigned short, 3> *ret=static_cast<TextureColor<unsigned short, 3>*>(data_);
			for ( int i = 0 ; i < width_*height_ ; i++ ) {
				for ( int j = 0 ; j < noChannels_ ; j++ )
					ret[i].c[j] = ret[i].c[j]*sc[j];
			}
			data_ = ret;
		}
	} else if (noChannels_ == 4) {
		if (pixel_type_ == UNSIGNED_CHAR_TYPE) {
			TextureColor<unsigned char, 4> *ret = static_cast<TextureColor<unsigned char, 4> *>(data_);
			for ( int i = 0 ; i < width_*height_ ; i++ ) {
				for ( int j = 0 ; j < noChannels_ ; j++ )
					ret[i].c[j] = ret[i].c[j]*sc[j];
			}
			data_ = ret;
		} else if (pixel_type_ == FLOAT_TYPE) {
			TextureColor<float, 4> *ret = static_cast<TextureColor<float, 4> *>(data_);
			for ( int i = 0 ; i < width_*height_ ; i++ ) {
				for ( int j = 0 ; j < noChannels_ ; j++ )
					ret[i].c[j] = ret[i].c[j]*sc[j];
			}
			data_ = ret;
		} else if (pixel_type_ == UNSIGNED_SHORT_TYPE) {
			TextureColor<unsigned short, 4> *ret = static_cast<TextureColor<unsigned short, 4> *>(data_);
			for ( int i = 0 ; i < width_*height_ ; i++ ) {
				for ( int j = 0 ; j < noChannels_ ; j++ )
					ret[i].c[j] = ret[i].c[j]*sc[j];
			}
			data_ = ret;
		}
	} else {
		LOG(LUX_ERROR, LUX_SYSTEM) << "Unsupported channel count in ImageData::Scale_Data()";
	}

}

MIPMap *ImageData::createMIPMap(ImageTextureFilterType filterType,
	float maxAniso, ImageWrap wrapMode, float gain, float gamma)
{
	MIPMap *mipmap = NULL;

	// Dade - added support for 1 channel maps
	if (noChannels_ == 1) {
		if (pixel_type_ == UNSIGNED_CHAR_TYPE) {
			if ((gain == 1.0f) && (gamma == 1.0f))
				mipmap = new MIPMapFastImpl<TextureColor<unsigned char, 1> >(
					filterType, width_, height_,
					static_cast<TextureColor<unsigned char, 1> *>(data_),
					maxAniso, wrapMode);
			else
				mipmap = new MIPMapImpl<TextureColor<unsigned char, 1> >(
					filterType, width_, height_,
					static_cast<TextureColor<unsigned char, 1> *>(data_),
					maxAniso, wrapMode, gain, gamma);
		} else if (pixel_type_ == FLOAT_TYPE) {
			if ((gain == 1.0f) && (gamma == 1.0f))
				mipmap = new MIPMapFastImpl<TextureColor<float, 1> >(
					filterType, width_, height_,
					static_cast<TextureColor<float, 1> *>(data_),
					maxAniso, wrapMode);
			else
				mipmap = new MIPMapImpl<TextureColor<float, 1 > >(
					filterType, width_, height_,
					static_cast<TextureColor<float, 1> *>(data_),
					maxAniso, wrapMode, gain, gamma);
		} else if (pixel_type_ == UNSIGNED_SHORT_TYPE) {
			if ((gain == 1.0f) && (gamma == 1.0f))
				mipmap = new MIPMapFastImpl<TextureColor<unsigned short, 1> >(
					filterType, width_, height_,
					static_cast<TextureColor<unsigned short, 1> *>(data_),
					maxAniso, wrapMode);
			else
				mipmap = new MIPMapImpl<TextureColor<unsigned short, 1> >(
					filterType, width_, height_,
					static_cast<TextureColor<unsigned short, 1> *>(data_),
					maxAniso, wrapMode, gain, gamma);
		}
	} else if (noChannels_ == 3) {
		if (pixel_type_ == UNSIGNED_CHAR_TYPE) {
			if ((gain == 1.0f) && (gamma == 1.0f)) {
				mipmap = new MIPMapFastImpl<TextureColor<unsigned char, 3> >(
					filterType, width_, height_,
					static_cast<TextureColor<unsigned char, 3> *>(data_),
					maxAniso, wrapMode);
			} else
				mipmap = new MIPMapImpl<TextureColor<unsigned char, 3> >(
					filterType, width_, height_,
					static_cast<TextureColor<unsigned char, 3> *>(data_),
					maxAniso, wrapMode, gain, gamma);
		} else if (pixel_type_ == FLOAT_TYPE) {
			if ((gain == 1.0f) && (gamma == 1.0f))
				mipmap = new MIPMapFastImpl<TextureColor<float, 3> >(
					filterType, width_, height_,
					static_cast<TextureColor<float, 3> *>(data_),
					maxAniso, wrapMode);
			else
				mipmap = new MIPMapImpl<TextureColor<float, 3> >(
					filterType, width_, height_,
					static_cast<TextureColor<float, 3> *>(data_),
					maxAniso, wrapMode, gain, gamma);
		} else if (pixel_type_ == UNSIGNED_SHORT_TYPE) {
			if ((gain == 1.0f) && (gamma == 1.0f))
				mipmap = new MIPMapFastImpl<TextureColor<unsigned short, 3> >(
					filterType, width_, height_,
					static_cast<TextureColor<unsigned short, 3> *>(data_),
					maxAniso, wrapMode);
			else
				mipmap = new MIPMapImpl<TextureColor<unsigned short, 3> >(
					filterType, width_, height_,
					static_cast<TextureColor<unsigned short, 3> *>(data_),
					maxAniso, wrapMode, gain, gamma);
		}
	} else if (noChannels_ == 4) {
		if (pixel_type_ == UNSIGNED_CHAR_TYPE) {
			if ((gain == 1.0f) && (gamma == 1.0f))
				mipmap = new MIPMapFastImpl<TextureColor<unsigned char, 4> >(
					filterType, width_, height_,
					static_cast<TextureColor<unsigned char, 4> *>(data_),
					maxAniso, wrapMode);
			else
				mipmap = new MIPMapImpl<TextureColor<unsigned char, 4> >(
					filterType, width_, height_,
					static_cast<TextureColor<unsigned char, 4> *>(data_),
					maxAniso, wrapMode, gain, gamma);
		} else if (pixel_type_ == FLOAT_TYPE) {
			if ((gain == 1.0f) && (gamma == 1.0f))
				mipmap = new MIPMapFastImpl<TextureColor<float, 4> >(
					filterType, width_, height_,
					static_cast<TextureColor<float, 4> *>(data_),
					maxAniso, wrapMode);
			else
				mipmap = new MIPMapImpl<TextureColor<float, 4> >(
					filterType, width_, height_,
					static_cast<TextureColor<float, 4> *>(data_),
					maxAniso, wrapMode, gain, gamma);
		} else if (pixel_type_ == UNSIGNED_SHORT_TYPE) {
			if ((gain == 1.0f) && (gamma == 1.0f))
				mipmap = new MIPMapFastImpl<TextureColor<unsigned short, 4> >(
					filterType, width_, height_,
					static_cast<TextureColor<unsigned short, 4> *>(data_),
					maxAniso, wrapMode);
			else
				mipmap = new MIPMapImpl<TextureColor<unsigned short, 4> >(
					filterType, width_, height_,
					static_cast<TextureColor<unsigned short, 4> *>(data_),
					maxAniso, wrapMode, gain, gamma);
		}
	} else {
		LOG(LUX_ERROR, LUX_SYSTEM) << "Unsupported channel count in ImageData::createMIPMap()";
		return NULL;
	}

	return mipmap;
}

static bool FileExists(const boost::filesystem::path &path) {
	try {
		// boost::filesystem::exists() can throw an exception under Windows
		// if the drive in imagePath doesn't exist
		return boost::filesystem::exists(path);
	} catch (const boost::filesystem::filesystem_error &) {
		return false;
	}	
}

// converts filename to platform independent form
// and searches for file in current dir if it doesn't 
// exist in specified location
// can't be in util.cpp due to error.h conflict
string AdjustFilename(const string filename, bool silent) {

	boost::filesystem::path filePath(filename);
	string result = filePath.string();

	// boost::filesystem::exists() can throw an exception under Windows
	// if the drive in imagePath doesn't exist
	if (FileExists(filePath))
		return result;

	// file not found, try fallback
	if (FileExists(filePath.filename()))
		result = filePath.filename().string();
	else
		// we failed, just return the normalized name
		return result;

	if (!silent)
		LOG(LUX_INFO, LUX_NOERROR) << "Couldn't find file '" << filename << "', using '" << result << "' instead";

	return result;
}

} //namespace lux
