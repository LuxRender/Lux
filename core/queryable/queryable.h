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
#include <boost/shared_ptr.hpp>
#include "error.h"
#include "queryableattribute.h"


namespace lux
{

namespace queryable {
	// internal function for Queryable API
	// used to set field of an object
	template <class T, class D> void setfield(T &obj, D T::*field, D value) {
		obj.*field = value;
	}
}

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

	void AddAttribute(boost::shared_ptr<QueryableAttribute> attr)
	{
		attributes.insert ( std::pair<std::string,boost::shared_ptr<QueryableAttribute> >(attr->name,attr) );
	}

	//Access by iterators : we are simply redirecting the calls to the map
	/* Iterators of a map container point to elements of this value_type.
	 * Thus, for an iterator called it that points to an element of a map, its key and mapped value can be accessed respectively with:
	 * map<Key,T>::iterator it;
	 * (*it).first;             // the key value (of type Key)
	 * (*it).second;            // the mapped value (of type T)
	 *  (*it);                   // the "element value" (of type pair<const Key,T>)
	 */
	typedef std::map<std::string, boost::shared_ptr<QueryableAttribute> >::iterator iterator;
	typedef std::map<std::string, boost::shared_ptr<QueryableAttribute> >::const_iterator const_iterator;
	iterator begin() { return attributes.begin(); }
	const_iterator begin() const { return attributes.begin(); }
    iterator end() { return attributes.end(); }
    const_iterator end() const { return attributes.end(); }

    //If s matches the name of an attribute in this object, the function returns a reference to its QueryableAttribute.
    //Otherwise, it throws an error.
	QueryableAttribute& operator[] (const std::string &s)
	{
		iterator it=attributes.find(s);
		if (it != attributes.end())
			return(*it->second);

		LOG(LUX_SEVERE,LUX_BADTOKEN) << "Attribute '" << s << "' does not exist in Queryable object";
		return nullAttribute;		
	}

	const std::string GetName() const
	{
		return name;
	}

	enum AttributeAccess { ReadOnlyAccess, ReadWriteAccess };

	template<class T> friend void AddBoolAttribute(T &object,
		const std::string &name, const std::string &description,
		bool T::*b, AttributeAccess access = ReadOnlyAccess) {

		boost::shared_ptr<QueryableBoolAttribute> attribute(
			new QueryableBoolAttribute(name, description, false));

		if (access == ReadWriteAccess)
			attribute->setFunc = boost::bind(lux::queryable::setfield<T, bool>, boost::ref(object), b, _1);

		attribute->getFunc = boost::bind(b, boost::ref(object));
		object.AddAttribute(attribute);
	}
	template<class T> friend void AddBoolAttribute(T &object,
		const std::string &name, const std::string &description,
		bool (T::*get)(), void (T::*set)(bool) = NULL) {

		boost::shared_ptr<QueryableBoolAttribute> attribute(
			new QueryableBoolAttribute(name, description, false));

		if (set)
			attribute->setFunc = boost::bind(set, boost::ref(object), _1);

		attribute->getFunc = boost::bind(get, boost::ref(object));
		object.AddAttribute(attribute);
	}

	template<class T> friend void AddStringAttribute(T &object,
		const std::string &name, const std::string &description,
		std::string T::*s, AttributeAccess access = ReadOnlyAccess) {

		boost::shared_ptr<QueryableStringAttribute> attribute(
			new QueryableStringAttribute(name, description, ""));

		if (access == ReadWriteAccess)
			attribute->setFunc = boost::bind(lux::queryable::setfield<T, std::string>, boost::ref(object), s, _1);

		attribute->getFunc = boost::bind(s, boost::ref(object));
		object.AddAttribute(attribute);
	}
	template<class T> friend void AddStringAttribute(T &object,
		const std::string &name, const std::string &description,
		std::string (T::*get)(), void (T::*set)(std::string) = NULL) {

		boost::shared_ptr<QueryableStringAttribute> attribute(
			new QueryableStringAttribute(name, description, ""));

		if (set)
			attribute->setFunc = boost::bind(set, boost::ref(object), _1);

		attribute->getFunc = boost::bind(get, boost::ref(object));
		object.AddAttribute(attribute);
	}

	template<class T> friend void AddFloatAttribute(T &object,
		const std::string &name, const std::string &description,
		float T::*f, AttributeAccess access = ReadOnlyAccess) {

		boost::shared_ptr<QueryableFloatAttribute> attribute(
			new QueryableFloatAttribute(name, description, 0.f));

		if (access == ReadWriteAccess)
			attribute->setFunc = boost::bind(lux::queryable::setfield<T, float>, boost::ref(object), f, _1);

		attribute->getFunc = boost::bind(f, boost::ref(object));
		object.AddAttribute(attribute);
	}
	template<class T> friend void AddFloatAttribute(T &object,
		const std::string &name, const std::string &description,
		float (T::*get)(), void (T::*set)(float) = NULL) {

		boost::shared_ptr<QueryableFloatAttribute> attribute(
			new QueryableFloatAttribute(name, description, 0.f));

		if (set)
			attribute->setFunc = boost::bind(set, boost::ref(object), _1);

		attribute->getFunc = boost::bind(get, boost::ref(object));
		object.AddAttribute(attribute);
	}

	template<class T> friend void AddDoubleAttribute(T &object,
		const std::string &name, const std::string &description,
		double T::*f, AttributeAccess access = ReadOnlyAccess) {

		boost::shared_ptr<QueryableDoubleAttribute> attribute(
			new QueryableDoubleAttribute(name, description, 0.0));

		if (access == ReadWriteAccess)
			attribute->setFunc = boost::bind(lux::queryable::setfield<T, double>, boost::ref(object), f, _1);

		attribute->getFunc = boost::bind(f, boost::ref(object));
		object.AddAttribute(attribute);
	}
	template<class T> friend void AddDoubleAttribute(T &object,
		const std::string &name, const std::string &description,
		double (T::*get)(), void (T::*set)(double) = NULL) {

		boost::shared_ptr<QueryableDoubleAttribute> attribute(
			new QueryableDoubleAttribute(name, description, 0.0));

		if (set)
			attribute->setFunc = boost::bind(set, boost::ref(object), _1);

		attribute->getFunc = boost::bind(get, boost::ref(object));
		object.AddAttribute(attribute);
	}

	template<class T> friend void AddIntAttribute(T &object,
		const std::string &name, const std::string &description,
		int T::*i, AttributeAccess access = ReadOnlyAccess) {

		boost::shared_ptr<QueryableIntAttribute> attribute(
			new QueryableIntAttribute(name, description, 0));

		if (access == ReadWriteAccess)
			attribute->setFunc = boost::bind(lux::queryable::setfield<T, int>, boost::ref(object), i, _1);

		attribute->getFunc = boost::bind(i, boost::ref(object));
		object.AddAttribute(attribute);
	}
	template<class T> friend void AddIntAttribute(T &object,
		const std::string &name, const std::string &description,
		unsigned int (T::*get)(), void (T::*set)(unsigned int) = NULL) {

		boost::shared_ptr<QueryableIntAttribute> attribute(
			new QueryableIntAttribute(name, description, 0));

		if (set)
			attribute->setFunc = boost::bind(set, boost::ref(object), _1);

		attribute->getFunc = boost::bind(get, boost::ref(object));
		object.AddAttribute(attribute);
	}
	template<class T> friend void AddIntAttribute(T &object,
		const std::string &name, const std::string &description,
		int (T::*get)(), void (T::*set)(int) = NULL) {

		boost::shared_ptr<QueryableIntAttribute> attribute(
			new QueryableIntAttribute(name, description, 0));

		if (set)
			attribute->setFunc = boost::bind(set, boost::ref(object), _1);

		attribute->getFunc = boost::bind(get, boost::ref(object));
		object.AddAttribute(attribute);
	}

private:
	std::map<std::string, boost::shared_ptr<QueryableAttribute> > attributes;
	std::string name;
	NullAttribute nullAttribute;
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
