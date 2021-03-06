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
	boost::mutex::scoped_lock lock(classWideMutex);
	std::map<std::string, Queryable*>::iterator it = queryableObjects.find(object->GetName());
	if (it != queryableObjects.end()) {
		LOG(LUX_ERROR, LUX_BUG) << "Duplicate registration of Queryable object '" << object->GetName() << "' detected";
		queryableObjects.erase(it);
	}
	queryableObjects.insert(std::pair<std::string,Queryable*>(object->GetName(),object));
}

void QueryableRegistry::Erase(Queryable* object)
{
	boost::mutex::scoped_lock lock(classWideMutex);
	if (!queryableObjects.erase(object->GetName())) {
		LOG(LUX_ERROR, LUX_BUG) << "Deregistration of non-existing Queryable object '" << object->GetName() << "' detected";
	}
}

const char * QueryableRegistry::GetContent()
{
	std::stringstream XMLOutput;
	//ATTRIBUTE_NONE, ATTRIBUTE_INT,ATTRIBUTE_FLOAT,ATTRIBUTE_DOUBLE,ATTRIBUTE_STRING
	//static const char* typeString[]= {"none","int","float","double","string"};


	XMLOutput<<"<?xml version='1.0' encoding='utf-8'?>"<<std::endl;
	XMLOutput<<"<context>"<<std::endl;

	std::pair<std::string, Queryable*> pairQObject;
	BOOST_FOREACH( pairQObject, queryableObjects )
	{

		XMLOutput << "  <object>"<<std::endl;
		XMLOutput << "    <name>"<<pairQObject.first<<"</name>"<<std::endl;

		std::pair<std::string, boost::shared_ptr<QueryableAttribute> > pairQAttribute;
		BOOST_FOREACH( pairQAttribute, *(pairQObject.second) )
		{
			XMLOutput<<"    <attribute>"<<std::endl;
			XMLOutput<<"      <name>"<< pairQAttribute.second->name <<"</name>"<<std::endl;
			XMLOutput<<"      <type>"<< pairQAttribute.second->TypeStr() <<"</type>"<<std::endl;
			XMLOutput<<"      <description>"<< pairQAttribute.second->description <<"</description>"<<std::endl;
			XMLOutput<<"      <value>"<< pairQAttribute.second->Value() <<"</value>"<<std::endl;
			if (pairQAttribute.second->HasDefaultValue())
				XMLOutput<<"      <default>"<< pairQAttribute.second->DefaultValue() <<"</default>"<<std::endl;
			if (pairQAttribute.second->HasMinValue())
				XMLOutput<<"      <min>"<< pairQAttribute.second->MinFloatValue() <<"</min>"<<std::endl;
			if (pairQAttribute.second->HasMaxValue())
				XMLOutput<<"      <max>"<< pairQAttribute.second->MaxFloatValue() <<"</max>"<<std::endl;
			XMLOutput<<"    </attribute>"<<std::endl;
		}

		XMLOutput << "  </object>"<<std::endl;
	}

	XMLOutput<<"</context>"<<std::endl;

	XMLOptionsString=XMLOutput.str();
	return XMLOptionsString.c_str();
}

}//namespace lux
