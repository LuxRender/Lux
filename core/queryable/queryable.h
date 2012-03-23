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
	template <class D> D getvalue(const D &value) {
		return value;
	}
	template <class T, class E> int getenumfield(T &obj, E T::*field) {
		return obj.*field;
	}
	template <class T, class E> void setenumfield(T &obj, E T::*field, int value) {
		obj.*field = static_cast<E>(value);
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
class LUX_EXPORT Queryable
{
public:
	Queryable(std::string _name);
	virtual ~Queryable();

	void AddAttribute(boost::shared_ptr<QueryableAttribute> attr)
	{
		// replace any existing attribute with same name
		attributes.erase(attr->name);
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

	bool HasAttribute(const std::string attributeName)
	{
		iterator it = attributes.find(attributeName);
		return it != attributes.end();
	}

    //If s matches the name of an attribute in this object, the function returns a reference to its QueryableAttribute.
    //Otherwise, it throws an error.
	QueryableAttribute& operator[] (const std::string &attributeName)
	{
		iterator it = attributes.find(attributeName);
		if (it != attributes.end())
			return(*it->second);

		LOG(LUX_SEVERE,LUX_BADTOKEN) << "Attribute '" << attributeName << "' does not exist in Queryable object";
		return nullAttribute;		
	}

	const std::string GetName() const
	{
		return name;
	}

	enum AttributeAccess { ReadOnlyAccess, ReadWriteAccess };

	template<class T> friend void AddBoolConstant(T &object,
		const std::string &name, const std::string &description,
		bool b) {

		AddValueAttrib<QueryableBoolAttribute>(object, name, description, b);
	}
	template<class T> friend void AddBoolAttribute(T &object,
		const std::string &name, const std::string &description,
		bool T::*b, AttributeAccess access = ReadOnlyAccess) {

		AddFieldAttrib<QueryableBoolAttribute>(object, name, description, b, access);
	}
	template<class T> friend void AddBoolAttribute(T &object,
		const std::string &name, const std::string &description,
		bool defaultValue, bool T::*b,
		AttributeAccess access = ReadOnlyAccess) {

		AddFieldAttrib<QueryableBoolAttribute>(object, name, description, defaultValue, b, access);
	}
	template<class T> friend void AddBoolAttribute(T &object,
		const std::string &name, const std::string &description,
		bool (T::*get)(), void (T::*set)(bool) = NULL) {

		AddAttrib<QueryableBoolAttribute>(object, name, description, get, set);
	}

	template<class T> friend void AddStringConstant(T &object,
		const std::string &name, const std::string &description,
		const std::string &s) {

		AddValueAttrib<QueryableStringAttribute>(object, name, description, s);
	}
	template<class T> friend void AddStringAttribute(T &object,
		const std::string &name, const std::string &description,
		std::string T::*s, AttributeAccess access = ReadOnlyAccess) {

		AddFieldAttrib<QueryableStringAttribute>(object, name, description, s, access);
	}
	template<class T> friend void AddStringAttribute(T &object,
		const std::string &name, const std::string &description,
		std::string defaultValue, std::string T::*s,
		AttributeAccess access = ReadOnlyAccess) {

		AddFieldAttrib<QueryableStringAttribute>(object, name, description, defaultValue, s, access);
	}
	template<class T> friend void AddStringAttribute(T &object,
		const std::string &name, const std::string &description,
		std::string (T::*get)(), void (T::*set)(std::string) = NULL) {

		AddAttrib<QueryableStringAttribute>(object, name, description, get, set);
	}

	template<class T> friend void AddFloatConstant(T &object,
		const std::string &name, const std::string &description,
		float f) {

		AddValueAttrib<QueryableFloatAttribute>(object, name, description, f);
	}
	template<class T> friend void AddFloatAttribute(T &object,
		const std::string &name, const std::string &description,
		float T::*f, AttributeAccess access = ReadOnlyAccess) {

		AddFieldAttrib<QueryableFloatAttribute>(object, name, description, f, access);
	}
	template<class T> friend void AddFloatAttribute(T &object,
		const std::string &name, const std::string &description,
		float defaultValue, float T::*f,
		AttributeAccess access = ReadOnlyAccess) {

		AddFieldAttrib<QueryableFloatAttribute>(object, name, description, defaultValue, f, access);
	}
	template<class T> friend void AddFloatAttribute(T &object,
		const std::string &name, const std::string &description,
		float (T::*get)(), void (T::*set)(float) = NULL) {

		AddAttrib<QueryableFloatAttribute>(object, name, description, get, set);
	}

	template<class T> friend void AddDoubleConstant(T &object,
		const std::string &name, const std::string &description,
		double d) {

		AddValueAttrib<QueryableDoubleAttribute>(object, name, description, d);
	}
	template<class T> friend void AddDoubleAttribute(T &object,
		const std::string &name, const std::string &description,
		double T::*f, AttributeAccess access = ReadOnlyAccess) {

		AddFieldAttrib<QueryableDoubleAttribute>(object, name, description, f, access);
	}
	template<class T> friend void AddDoubleAttribute(T &object,
		const std::string &name, const std::string &description,
		double defaultValue, double T::*f,
		AttributeAccess access = ReadOnlyAccess) {

		AddFieldAttrib<QueryableDoubleAttribute>(object, name, description, defaultValue, f, access);
	}
	template<class T> friend void AddDoubleAttribute(T &object,
		const std::string &name, const std::string &description,
		double (T::*get)(), void (T::*set)(double) = NULL) {

		AddAttrib<QueryableDoubleAttribute>(object, name, description, get, set);
	}

	template<class T, class E> friend void AddIntEnumAttribute(T &object,
		const std::string &name, const std::string &description,
		E T::*e, AttributeAccess access = ReadOnlyAccess) {

		AddEnumFieldAttrib<QueryableIntAttribute>(object, name, description, e, access);
	}
	template<class T> friend void AddIntConstant(T &object,
		const std::string &name, const std::string &description,
		int i) {

		AddValueAttrib<QueryableIntAttribute>(object, name, description, i);
	}
	template<class T> friend void AddIntAttribute(T &object,
		const std::string &name, const std::string &description,
		int T::*i, AttributeAccess access = ReadOnlyAccess) {

		AddFieldAttrib<QueryableIntAttribute>(object, name, description, i, access);
	}
	template<class T> friend void AddIntAttribute(T &object,
		const std::string &name, const std::string &description,
		int defaultValue, int T::*i,
		AttributeAccess access = ReadOnlyAccess) {

		AddFieldAttrib<QueryableIntAttribute>(object, name, description, defaultValue, i, access);
	}
	template<class T> friend void AddIntAttribute(T &object,
		const std::string &name, const std::string &description,
		unsigned int (T::*get)(), void (T::*set)(unsigned int) = NULL) {

		AddAttrib<QueryableIntAttribute>(object, name, description, get, set);
	}
	template<class T> friend void AddIntAttribute(T &object,
		const std::string &name, const std::string &description,
		int (T::*get)(), void (T::*set)(int) = NULL) {

		AddAttrib<QueryableIntAttribute>(object, name, description, get, set);
	}
protected:
	template<class QA, class T, class D> static void AddValueAttrib(T &object,
		const std::string &name, const std::string &description,
		const D &value) {

		boost::shared_ptr<QA> attribute(
			new QA(name, description));

		attribute->getFunc = boost::bind(lux::queryable::getvalue<D>, value);
		object.AddAttribute(attribute);
	}
	template<class QA, class T, class E> static void AddEnumFieldAttrib(T &object,
		const std::string &name, const std::string &description,
		E T::*field, AttributeAccess access) {

		boost::shared_ptr<QA> attribute(
			new QA(name, description));

		if (access == ReadWriteAccess)
			attribute->setFunc = boost::bind(lux::queryable::setenumfield<T, E>, boost::ref(object), field, _1);

		attribute->getFunc = boost::bind(lux::queryable::getenumfield<T, E>, boost::ref(object), field);
		object.AddAttribute(attribute);
	}
	template<class QA, class T, class D> static void AddFieldAttrib(T &object,
		const std::string &name, const std::string &description,
		D T::*field, AttributeAccess access) {

		boost::shared_ptr<QA> attribute(
			new QA(name, description));

		if (access == ReadWriteAccess)
			attribute->setFunc = boost::bind(lux::queryable::setfield<T, D>, boost::ref(object), field, _1);

		attribute->getFunc = boost::bind(field, boost::ref(object));
		object.AddAttribute(attribute);
	}
	template<class QA, class T, class D> static void AddFieldAttrib(T &object,
		const std::string &name, const std::string &description,
		D defaultValue, D T::*field, AttributeAccess access) {

		boost::shared_ptr<QA> attribute(
			new QA(name, description, defaultValue));

		if (access == ReadWriteAccess)
			attribute->setFunc = boost::bind(lux::queryable::setfield<T, D>, boost::ref(object), field, _1);

		attribute->getFunc = boost::bind(field, boost::ref(object));
		object.AddAttribute(attribute);
	}
	template<class QA, class T, class D> static void AddAttrib(T &object,
		const std::string &name, const std::string &description,
		D (T::*get)(), void (T::*set)(D)) {

		boost::shared_ptr<QA> attribute(
			new QA(name, description));

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
