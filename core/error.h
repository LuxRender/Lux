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

#ifndef LUX_ERROR_H
#define LUX_ERROR_H

#include "api.h"

#include <sstream>
#include <string>

#define BOOST_ENABLE_ASSERT_HANDLER
#define BOOST_ENABLE_ASSERTS
//#define BOOST_DISABLE_ASSERTS
#include <boost/assert.hpp>

namespace lux
{
	LUX_EXPORT extern int luxLogFilter;
	//Logging class to use when displaying infos and error, syntax :
	//  LOG(LUX_SEVERE,LUX_NOMEM)<<"one "<<23;

	class Log
	{
	public:
	   ~Log() { luxError(code, severity, os.str().c_str()); }

	   inline std::ostringstream& get(int _severity, int _code)
	   {
		  severity =_severity;
		  code=_code;
		  return os;
	   }

	private:
	   int severity, code;
	   std::ostringstream os;
	};

	struct nullstream : std::ostream
	{
		nullstream(): std::ios(0), std::ostream(0) {}
	};
	extern LUX_EXPORT nullstream nullStream;
}

//LOG macro. The filtering test uses the ?: operator instead of if{}else{}
//to avoid ambiguity when invoked in if{}/else{} constructs
#define LOG(severity,code) (severity<lux::luxLogFilter)?(lux::nullStream):lux::Log().get(severity, code)

//With this call you bypass the API filtering option and force output to the user
#define LOG_NO_FILTER(severity, code) Log().get(severity, code)

namespace boost
{
	inline void assertion_failed(char const *expr, char const *function, char const *file, long line)
	{
		LOG(LUX_SEVERE,LUX_BUG)<< "Assertion '"<<expr<<"' failed in function '"<<function<<"' (file:"<<file<<" line:"<<line<<")";
	}
}

#endif //LUX_ERROR_H
