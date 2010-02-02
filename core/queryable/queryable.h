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

/*
 *
How it works :
Each class that wants to expose one or more members has to inherit the 'Queryable' class :

Code: Select all
    class Sphere: public Shape, public Queryable


It should also provide get/set member functions for each exposed attribute :

Code: Select all
    void Sphere::setRadius(float rad) {radius=rad;} //WARNING this is wrong ; changing the radius needs recomputing (see constructor)
       float Sphere::getRadius() { return radius; }


And finally it should add one line for each attribute in the constructor:

Code: Select all
    Sphere::Sphere(const Transform &o2w, bool ro, float rad,  float z0, float z1, float pm): Shape(o2w, ro) {
       SET_FLOAT_ATTRIBUTE(Sphere,"sphereRadius",setRadius,getRadius);
           ....


That's all.

After that, the Queryable class provides this method :

Code: Select all
    std::vector<QueryableAttribute> getAttributes() { return attributes; }



So, a Queryable object can have one ore more 'QueryableAttributes'. What's a 'QueryableAttribute' ? It's simply an object holding the attribute's type, the attribute's name,description, default values,...,...,and accessor methods :

Code: Select all
    //Accessors
       void setValue(float f);
       float getFloatValue();


Of course these methods check if the attribute has the good type, eg. calling 'getFloatValue' on a integer attribute will raise an error.
 *
 *
 */


namespace lux
{
//All Queryable objects inherits from this class
class Queryable
{
public:
	Queryable(std::string _name);
	virtual ~Queryable();

	void addAttribute(QueryableAttribute attr)
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
			exit(1);
		}
	}

	const std::string getName()
	{
		return name;
	}




private:
	std::map<std::string, QueryableAttribute> attributes;
	std::string name;
};

}//namespace lux


//MACROS
#define SET_FLOAT_ATTRIBUTE(className,attributeName,setMemberFunction, getMemberFunction) \
	{ QueryableAttribute _tmpAttribute(attributeName,ATTRIBUTE_FLOAT); \
	_tmpAttribute.setFloatFunc=boost::bind(&className::setMemberFunction, boost::ref(*this), _1); \
	_tmpAttribute.getFloatFunc=boost::bind(&className::getMemberFunction, boost::ref(*this)); \
	addAttribute(_tmpAttribute);}

#define SET_FLOAT_ATTRIBUTE_READONLY(className,attributeName, getMemberFunction) \
	{ QueryableAttribute _tmpAttribute(attributeName,ATTRIBUTE_FLOAT); \
	_tmpAttribute.setFloatFunc=boost::bind(&QueryableAttribute::readOnlyFloatError, boost::ref(*this), _1); \
	_tmpAttribute.getFloatFunc=boost::bind(&className::getMemberFunction, boost::ref(*this)); \
	addAttribute(_tmpAttribute);}

#define SET_INT_ATTRIBUTE(className,attributeName,setMemberFunction, getMemberFunction) \
	{ QueryableAttribute _tmpAttribute(attributeName,ATTRIBUTE_INT); \
	_tmpAttribute.setIntFunc=boost::bind(&className::setMemberFunction, boost::ref(*this), _1); \
	_tmpAttribute.getIntFunc=boost::bind(&className::getMemberFunction, boost::ref(*this)); \
	addAttribute(_tmpAttribute);}


#endif // LUX_QUERYABLE_H
