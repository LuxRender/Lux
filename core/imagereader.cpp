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
#include "error.h"

#include <boost/filesystem/path.hpp>
#include <boost/filesystem/convenience.hpp>

namespace lux {

static bool FileExists(const boost::filesystem::path &path) {
	try {
		// boost::filesystem::exists() can throw an exception under Windows
		// if the drive in imagePath doesn't exist
		return boost::filesystem::exists(path);
	} catch (const boost::filesystem::filesystem_error &) {
		return false;
	}	
}

static bool FileExists(const string filename) {
	return FileExists(boost::filesystem::path(filename));
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
		result = filePath.filename();
	else
		// we failed, just return the normalized name
		return result;

	if (!silent)
		LOG(LUX_INFO, LUX_NOERROR) << "Couldn't find file '" << filename << "', using '" << result << "' instead";

	return result;
}

} //namespace lux