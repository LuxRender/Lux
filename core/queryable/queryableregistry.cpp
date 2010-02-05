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

// queryableregistry.cpp
#include <sstream>
#include <iostream>
#include <boost/foreach.hpp>
#include "queryable.h"
#include "queryableregistry.h"

namespace lux
{

void QueryableRegistry::Insert(Queryable* object)
{
	//TODO jromang - add assertion (not duplicate name)
	queryableObjects.insert(std::pair<std::string,Queryable*>(object->GetName(),object));
}

void QueryableRegistry::Erase(Queryable* object)
{
	//TODO jromang - add assertion (existing object)
	queryableObjects.erase(object->GetName());
}

std::string QueryableRegistry::GetContent() const
{
	std::stringstream XMLOutput;
	//ATTRIBUTE_NONE, ATTRIBUTE_INT,ATTRIBUTE_FLOAT,ATTRIBUTE_STRING
	static const char* typeString[]= {"none","int","float","string"};


	XMLOutput<<"<?xml version='1.0' encoding='utf-8'?>"<<std::endl;
	XMLOutput<<"<context>"<<std::endl;

	std::pair<std::string, Queryable*> pairQObject;
	BOOST_FOREACH( pairQObject, queryableObjects )
	{

		XMLOutput << "  <object>"<<std::endl;
		XMLOutput << "    <name>"<<pairQObject.first<<"</name>"<<std::endl;

		std::pair<std::string, QueryableAttribute> pairQAttribute;
		BOOST_FOREACH( pairQAttribute, *(pairQObject.second) )
		{
			XMLOutput<<"    <attribute>"<<std::endl;
			XMLOutput<<"      <name>"<< pairQAttribute.second.name <<"</name>"<<std::endl;
			XMLOutput<<"      <type>"<< typeString[pairQAttribute.second.type] <<"</type>"<<std::endl;
			XMLOutput<<"    </attribute>"<<std::endl;
		}

		XMLOutput << "  </object>"<<std::endl;
	}

	XMLOutput<<"</context>"<<std::endl;

	return XMLOutput.str();
}

}//namespace lux
