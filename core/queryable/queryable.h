/***************************************************************************
 *   Copyright (C) 1998-2010 by authors (see AUTHORS.txt )                 *
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

#ifndef LUX_QUERYABLE_H
#define LUX_QUERYABLE_H

#include <vector>
#include <map>
#include <string>
#include "error.h"
#include "queryableattribute.h"


namespace lux
{

/*! \class Queryable
 * \brief Parent class of all Queryable objects in the core module
 * \author jromang
 *
 *  The rendering API allows external applications using lux to read or modify various parameters
 *  of the rendering engine or objects loaded in a context.
 *  To expose their attributes, objects allowing modifications via the API have to be 'Queryable' ; this
 *  is easily done in 3 steps :
 *
 *  1) Each class that wants to expose one or more members has to inherit the 'Queryable' class :
 *     'class Sphere: public Shape, public Queryable'
 *  2) It should also provide get/set member functions for each exposed attribute :
 *     'void Sphere::setRadius(float rad) {radius=rad;} //this is wrong ; changing the radius needs recomputing (see Sphere constructor)
 *     float Sphere::getRadius() { return radius; }'
 *  3) And finally it should add one line for each attribute in the constructor:
 *     'AddFloatAttribute(
 *     						"radius", boost::bind(&Sphere::getRadius, boost::ref(*this)),
 *     							      boost::bind(&Sphere::setRadius, boost::ref(*this))
 *      );'
 *
 */
class Queryable
{
public:
	Queryable(std::string _name);
	virtual ~Queryable();

	void AddAttribute(QueryableAttribute attr)
	{
		attributes.insert ( std::pair<std::string,QueryableAttribute>(attr.name,attr) );
	}

	//Access by iterators : we are simply redirecting the calls to the map
	/* Iterators of a map container point to elements of this value_type.
	 * Thus, for an iterator called it that points to an element of a map, its key and mapped value can be accessed respectively with:
	 * map<Key,T>::iterator it;
	 * (*it).first;             // the key value (of type Key)
	 * (*it).second;            // the mapped value (of type T)
	 *  (*it);                   // the "element value" (of type pair<const Key,T>)
	 */
	typedef std::map<std::string, QueryableAttribute>::iterator iterator;
	typedef std::map<std::string, QueryableAttribute>::const_iterator const_iterator;
	iterator begin() { return attributes.begin(); }
	const_iterator begin() const { return attributes.begin(); }
    iterator end() { return attributes.end(); }
    const_iterator end() const { return attributes.end(); }


    //If s matches the name of an attribute in this object, the function returns a reference to its QueryableAttribute.
    //Otherwise, it throws an error.
	QueryableAttribute& operator[] (const std::string &s)
	{
		iterator it=attributes.find(s);
		if(it!=attributes.end()) return((*it).second);
		else
		{
			LOG(LUX_SEVERE,LUX_BADTOKEN) << "Attribute '" << s << "' does not exist in Queryable object";
			//exit(1);
			return nullAttribute;
		}
	}

	const std::string GetName()
	{
		return name;
	}

	void AddFloatAttribute(std::string attributeName, boost::function<float (void)> getFunc, boost::function<void (float)> setFunc=boost::bind(&QueryableAttribute::ReadOnlyFloatError,_1))
	{
		QueryableAttribute tmpAttribute(attributeName,ATTRIBUTE_FLOAT);
		tmpAttribute.setFloatFunc=setFunc;
		tmpAttribute.getFloatFunc=getFunc;
		AddAttribute(tmpAttribute);
	}

	void AddIntAttribute(std::string attributeName, boost::function<int (void)> getFunc, boost::function<void (int)> setFunc=boost::bind(&QueryableAttribute::ReadOnlyIntError, _1))
	{
		QueryableAttribute tmpAttribute(attributeName,ATTRIBUTE_INT);
		tmpAttribute.setIntFunc=setFunc;
		tmpAttribute.getIntFunc=getFunc;
		AddAttribute(tmpAttribute);
	}



private:
	std::map<std::string, QueryableAttribute> attributes;
	std::string name;
	QueryableAttribute nullAttribute;
};

}//namespace lux




//MACROS
/*
#define SET_FLOAT_ATTRIBUTE(className,attributeName, getMemberFunction, setMemberFunction) \
	{ QueryableAttribute _tmpAttribute(attributeName,ATTRIBUTE_FLOAT); \
	_tmpAttribute.setFloatFunc=boost::bind(&className::setMemberFunction, boost::ref(*this), _1); \
	_tmpAttribute.getFloatFunc=boost::bind(&className::getMemberFunction, boost::ref(*this)); \
	AddAttribute(_tmpAttribute);}

#define SET_FLOAT_ATTRIBUTE_READONLY(className,attributeName, getMemberFunction) \
	{ QueryableAttribute _tmpAttribute(attributeName,ATTRIBUTE_FLOAT); \
	_tmpAttribute.setFloatFunc=boost::bind(&QueryableAttribute::ReadOnlyFloatError, _1); \
	_tmpAttribute.getFloatFunc=boost::bind(&className::getMemberFunction, boost::ref(*this)); \
	AddAttribute(_tmpAttribute);}

#define SET_INT_ATTRIBUTE(className,attributeName, getMemberFunction ,setMemberFunction) \
	{ QueryableAttribute _tmpAttribute(attributeName,ATTRIBUTE_INT); \
	_tmpAttribute.setIntFunc=boost::bind(&className::setMemberFunction, boost::ref(*this), _1); \
	_tmpAttribute.getIntFunc=boost::bind(&className::getMemberFunction, boost::ref(*this)); \
	AddAttribute(_tmpAttribute);}

#define SET_INT_ATTRIBUTE_READONLY(className,attributeName, getMemberFunction) \
	{ QueryableAttribute _tmpAttribute(attributeName,ATTRIBUTE_INT); \
	_tmpAttribute.setIntFunc=boost::bind(&QueryableAttribute::ReadOnlyIntError,_1); \
	_tmpAttribute.getIntFunc=boost::bind(&className::getMemberFunction, boost::ref(*this)); \
	AddAttribute(_tmpAttribute);}
*/


#endif // LUX_QUERYABLE_H
