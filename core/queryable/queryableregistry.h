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

#ifndef LUX_QUERYABLE_REGISTRY_H
#define LUX_QUERYABLE_REGISTRY_H

#include <map>
#include <string>
#include "error.h"
#include "queryable.h"

#include <boost/thread.hpp>

namespace lux
{

class QueryableRegistry
{
public:
	void Insert(Queryable* object);
	void Erase(Queryable* object);
	const char * GetContent();

	Queryable* operator[] (const std::string &s)
	{
		std::map<std::string, Queryable*>::iterator it=queryableObjects.find(s);
		if(it!=queryableObjects.end()) 
			return((*it).second);
		else
		{
			//LOG(LUX_SEVERE,LUX_BADTOKEN) << "Object '" << s << "' does not exist in registry";
			return(0);
		}
	}

private:
	std::map<std::string, Queryable*> queryableObjects;
	std::string XMLOptionsString;
	mutable boost::mutex classWideMutex;
};

}//namespace lux


#endif // LUX_QUERYABLE_REGISTRY_H
