/***************************************************************************
 *   Copyright (C) 1998-2008 by authors (see AUTHORS.txt )                 *
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

#ifndef LUX_ERROR_H
#define LUX_ERROR_H

#include "api.h"

#include <sstream>
#include <string>

#define BOOST_ENABLE_ASSERT_HANDLER
#define BOOST_ENABLE_ASSERTS
//#define BOOST_DISABLE_ASSERTS
#include <boost/assert.hpp>

extern LuxErrorHandler luxError;

namespace boost
{
	inline void assertion_failed(char const *expr, char const *function, char const *file, long line)
	{
		std::ostringstream o;
		o<< "Assertion '"<<expr<<"' failed in function '"<<function<<"' (file:"<<file<<" line:"<<line<<")";
		luxError(LUX_BUG, LUX_SEVERE, const_cast<char *>(o.str().c_str()));
	}	
}


#endif //LUX_ERROR_H
