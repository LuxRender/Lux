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

#ifndef LUX_QUERYABLE_ATTRIBUTE_H
#define LUX_QUERYABLE_ATTRIBUTE_H

#include <string>
#include <boost/function.hpp>
#include <boost/bind.hpp>
#include <boost/lexical_cast.hpp>
#include "error.h"


namespace lux
{

typedef enum {ATTRIBUTE_NONE, ATTRIBUTE_INT,ATTRIBUTE_FLOAT,ATTRIBUTE_STRING} AttributeType;

/*
What's a 'QueryableAttribute' ? It's simply an object holding the attribute's type, the attribute's name,description, default values,...,...,and accessor methods :

Code: Select all
    //Accessors
       void setValue(float f);
       float getFloatValue();


Of course these methods check if the attribute has the good type, eg. calling 'getFloatValue' on a integer attribute will raise an error.
*/
class QueryableAttribute
{
public:
	QueryableAttribute()
	{
		type=ATTRIBUTE_NONE;
	}

	QueryableAttribute(const std::string &n, AttributeType t)
		: name(n), type(t)
	{

	}

	//Get accessors
	std::string Value()
	{
		switch(type)
		{
			case ATTRIBUTE_FLOAT: return boost::lexical_cast<std::string>(getFloatFunc());
			case ATTRIBUTE_INT: return boost::lexical_cast<std::string>(getIntFunc());
			default: luxError(LUX_BUG, LUX_SEVERE, "QueryableAttribute has invalid attribute type");
		}
	}

	float FloatValue()
	{
		BOOST_ASSERT(type==ATTRIBUTE_FLOAT);
		return getFloatFunc();
	}

	int IntValue()
	{
		BOOST_ASSERT(type==ATTRIBUTE_INT);
		return getIntFunc();
	}


	//Set accessors
	void SetValue(const std::string &value)
	{
		switch(type)
		{
			case ATTRIBUTE_FLOAT: setFloatFunc(boost::lexical_cast<float>(value));
			case ATTRIBUTE_INT: setIntFunc(boost::lexical_cast<int>(value));
			default: luxError(LUX_BUG, LUX_SEVERE, "QueryableAttribute has invalid attribute type");
		}
	}

	void SetValue(float f)
	{
		BOOST_ASSERT(type==ATTRIBUTE_FLOAT);
		setFloatFunc(f);
	}

	void SetValue(int i)
	{
		BOOST_ASSERT(type==ATTRIBUTE_INT);
		setIntFunc(i);
	}

	//functions pointing to the object's methods
	boost::function<void (int)> setIntFunc;
	boost::function<int (void)> getIntFunc;
	boost::function<void (float)> setFloatFunc;
	boost::function<float (void)> getFloatFunc;
	//...TODO add all types here

	//Default error functions for readonly attributes
	static void ReadOnlyFloatError(float f);
	static void ReadOnlyIntError(int i);

//private:
	std::string name;
	AttributeType type;

};


}//namespace lux


#endif // LUX_QUERYABLE_ATTRIBUTE_H
