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


/*
What's a 'QueryableAttribute' ? It's simply an object holding the attribute's type, the attribute's name,description, default values,...,...,and accessor methods :

Code: Select all
    //Accessors
       void setValue(float f);
       float getFloatValue();


Of course these methods check if the attribute has the good type, eg. calling 'getFloatValue' on a integer attribute will raise an error.
*/

class AttributeType {
public:
	enum DataType { None, Bool, Int, Float, Double, String };
};

class QueryableAttribute
{
protected:
	QueryableAttribute(const std::string &n, const std::string &d)
		: name(n), description(d)
	{

	}

public:
	QueryableAttribute()
	{
	}

	virtual AttributeType::DataType Type() const {
		throw std::runtime_error("QueryableAttribute::Type() not overridden");
	}
	std::string TypeStr() const {
		switch (Type()) {
			case AttributeType::None:
				return "none";
			case AttributeType::Bool:
				return "bool";
			case AttributeType::Int:
				return "int";
			case AttributeType::Float:
				return "float";
			case AttributeType::Double:
				return "double";
			case AttributeType::String:
				return "string";
			default:
				return "invalid";
		}
	}

	// assignment operators return void as it does not make sense 
	// to return a reference
	virtual void operator= (const bool &v) {
		throw std::runtime_error("Parameter type '" + TypeStr() + "' is not compatible with type 'bool'");
	}
	virtual void operator= (const int &v) {
		throw std::runtime_error("Parameter type '" + TypeStr() + "' is not compatible with type 'int'");
	}
	virtual void operator= (const float &v) {
		throw std::runtime_error("Parameter type '" + TypeStr() + "' is not compatible with type 'float'");
	}
	virtual void operator= (const double &v) {
		throw std::runtime_error("Parameter type '" + TypeStr() + "' is not compatible with type 'double'");
	}
	virtual void operator= (const std::string &v) {
		throw std::runtime_error("Parameter type '" + TypeStr() + "' is not compatible with type 'string'");
	}


	//Get accessors
	virtual std::string Value() const = 0;

	virtual bool BoolValue() const {
		throw std::runtime_error("Parameter type '" + TypeStr() + "' is not compatible with type 'bool'");
	}
	virtual int IntValue() const {
		throw std::runtime_error("Parameter type '" + TypeStr() + "' is not compatible with type 'int'");
	}
	virtual float FloatValue() const {
		throw std::runtime_error("Parameter type '" + TypeStr() + "' is not compatible with type 'float'");
	}
	virtual double DoubleValue() const {
		throw std::runtime_error("Parameter type '" + TypeStr() + "' is not compatible with type 'double'");
	}
	virtual const std::string& StringValue() const {
		throw std::runtime_error("Parameter type '" + TypeStr() + "' is not compatible with type 'string'");
	}

	virtual std::string DefaultValue() const = 0;

	virtual bool DefaultBoolValue() const {
		throw std::runtime_error("Parameter type '" + TypeStr() + "' is not compatible with type 'bool'");
	}
	virtual int DefaultIntValue() const {
		throw std::runtime_error("Parameter type '" + TypeStr() + "' is not compatible with type 'int'");
	}
	virtual float DefaultFloatValue() const {
		throw std::runtime_error("Parameter type '" + TypeStr() + "' is not compatible with type 'float'");
	}
	virtual double DefaultDoubleValue() const {
		throw std::runtime_error("Parameter type '" + TypeStr() + "' is not compatible with type 'double'");
	}
	virtual const std::string& DefaultStringValue() const {
		throw std::runtime_error("Parameter type '" + TypeStr() + "' is not compatible with type 'string'");
	}

	virtual bool HasMinValue() const {
		return false;
	}

	virtual int MinIntValue() const {
		throw std::runtime_error("Parameter type '" + TypeStr() + "' is not compatible with type 'int'");
	}
	virtual float MinFloatValue() const {
		throw std::runtime_error("Parameter type '" + TypeStr() + "' is not compatible with type 'float'");
	}
	virtual double MinDoubleValue() const {
		throw std::runtime_error("Parameter type '" + TypeStr() + "' is not compatible with type 'double'");
	}

	virtual bool HasMaxValue() const {
		return false;
	}

	virtual int MaxIntValue() const {
		throw std::runtime_error("Parameter type '" + TypeStr() + "' is not compatible with type 'int'");
	}
	virtual float MaxFloatValue() const {
		throw std::runtime_error("Parameter type '" + TypeStr() + "' is not compatible with type 'float'");
	}
	virtual double MaxDoubleValue() const {
		throw std::runtime_error("Parameter type '" + TypeStr() + "' is not compatible with type 'double'");
	}

//private:
	std::string name;
	std::string description;
};


class NullAttribute : public QueryableAttribute {
public:
	~NullAttribute() {}

	virtual AttributeType::DataType Type() const {
		return AttributeType::None;
	}

	virtual std::string Value() const {
		return "null";
	}

	virtual std::string DefaultValue() const {
		return "null";
	}
};

template <class D>
class GenericQueryableAttribute : public QueryableAttribute {
protected:
	GenericQueryableAttribute<D>(const std::string &_name, const std::string &_desc, D _defaultValue) 
		: QueryableAttribute(_name, _desc), 
		defaultValue(_defaultValue), 
		hasMinValue(false), minValue(_defaultValue), // initialize min/max in case they must be
		hasMaxValue(false), maxValue(_defaultValue) {
		// attributes are read-only by default
		setFunc = boost::bind(&GenericQueryableAttribute<D>::ReadOnlyError, boost::ref(*this), _1);
	}

	GenericQueryableAttribute<D>(const std::string &_name, const std::string &_desc, D _defaultValue,
								 bool _hasMin, D _minValue, bool _hasMax, D _maxValue)
		: QueryableAttribute(_name, _desc), 
		defaultValue(_defaultValue), 
		hasMinValue(_hasMin), minValue(_minValue), 
		hasMaxValue(_hasMax), maxValue(_maxValue) {
		// attributes are read-only by default
		setFunc = boost::bind(&GenericQueryableAttribute<D>::ReadOnlyError, boost::ref(*this), _1);
	}

public:
	~GenericQueryableAttribute<D>() {}

	virtual std::string Value() const {
		return boost::lexical_cast<std::string>(getFunc());
	}

	virtual std::string DefaultValue() const {
		return boost::lexical_cast<std::string>(defaultValue);
	}

	virtual bool HasMinValue() const {
		return hasMinValue;
	}

	virtual bool HasMaxValue() const {
		return hasMaxValue;
	}

	boost::function<void (D)> setFunc;
	boost::function<D (void)> getFunc;
	void ReadOnlyError(D v) {
		LOG(LUX_ERROR,LUX_BADTOKEN)<<"Queryable attributes: cannot change read-only attribute '" << name << "'";
	}

protected:
	D defaultValue;

	bool hasMinValue;
	D minValue;
	bool hasMaxValue;
	D maxValue;
};

class QueryableBoolAttribute : public GenericQueryableAttribute<bool> {
public:
	QueryableBoolAttribute(const std::string &_name, const std::string &_desc, bool _defaultValue) 
		: GenericQueryableAttribute<bool>(_name, _desc, _defaultValue) {
	}

	virtual AttributeType::DataType Type() const {
		return AttributeType::Bool;
	}

	virtual void operator= (const bool &v) {
		setFunc(v);
	}

	virtual bool BoolValue() const {
		return getFunc();
	}

	virtual bool DefaultBoolValue() const {
		return defaultValue;
	}
};

class QueryableIntAttribute : public GenericQueryableAttribute<int> {
public:
	QueryableIntAttribute(const std::string &_name, const std::string &_desc, int _defaultValue) 
		: GenericQueryableAttribute<int>(_name, _desc, _defaultValue) {
	}

	virtual AttributeType::DataType Type() const {
		return AttributeType::Int;
	}

	virtual void operator= (const int &v) {
		setFunc(v);
	}

	virtual int IntValue() const {
		return getFunc();
	}

	virtual int DefaultIntValue() const {
		return defaultValue;
	}

	virtual int MinIntValue() const {
		return minValue;
	}

	virtual int MaxIntValue() const {
		return maxValue;
	}
};

class QueryableFloatAttribute : public GenericQueryableAttribute<float> {
public:
	QueryableFloatAttribute(const std::string &_name, const std::string &_desc, float _defaultValue) 
		: GenericQueryableAttribute<float>(_name, _desc, _defaultValue) {
	}

	virtual AttributeType::DataType Type() const {
		return AttributeType::Float;
	}

	virtual void operator= (const float &v) {
		setFunc(v);
	}

	virtual float FloatValue() const {
		return getFunc();
	}

	virtual float DefaultFloatValue() const {
		return defaultValue;
	}

	virtual float MinFloatValue() const {
		return minValue;
	}

	virtual float MaxFloatValue() const {
		return maxValue;
	}
};

class QueryableDoubleAttribute : public GenericQueryableAttribute<double> {
public:
	QueryableDoubleAttribute(const std::string &_name, const std::string &_desc, double _defaultValue) 
		: GenericQueryableAttribute<double>(_name, _desc, _defaultValue) {
	}

	virtual AttributeType::DataType Type() const {
		return AttributeType::Double;
	}

	virtual void operator= (const float &v) {
		double d = v;
		setFunc(d);
	}

	virtual void operator= (const double &v) {
		setFunc(v);
	}

	virtual float FloatValue() const {
		return static_cast<float>(getFunc());
	}

	virtual double DoubleValue() const {
		return static_cast<double>(getFunc());
	}

	virtual float DefaultFloatValue() const {
		return static_cast<float>(defaultValue);
	}

	virtual double DefaultDoubleValue() const {
		return defaultValue;
	}

	virtual float MinFloatValue() const {
		return static_cast<float>(minValue);
	}

	virtual double MinDoubleValue() const {
		return minValue;
	}

	virtual float MaxFloatValue() const {
		return static_cast<float>(maxValue);
	}

	virtual double MaxDoubleValue() const {
		return maxValue;
	}
};


class QueryableStringAttribute : public GenericQueryableAttribute<std::string> {
public:
	QueryableStringAttribute(const std::string &_name, const std::string &_desc, std::string _defaultValue) 
		: GenericQueryableAttribute<std::string>(_name, _desc, _defaultValue) {
	}

	virtual AttributeType::DataType Type() const {
		return AttributeType::String;
	}

	virtual void operator= (const std::string &v) {
		setFunc(v);
	}

	virtual const std::string& StringValue() const {
		value = getFunc();
		return value;
	}

	virtual const std::string& DefaultStringValue() const {
		return defaultValue;
	}
private:
	// used to cache value, so a reference can be returned
	mutable std::string value;
};

}//namespace lux


#endif // LUX_QUERYABLE_ATTRIBUTE_H
