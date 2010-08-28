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



/* This file defines the Python binding to the Lux API ("pylux") */

#include <iostream>
#include <vector>
#include <cstring>
#include <boost/python.hpp>
#include <boost/python/type_id.hpp>
#include <boost/python/object.hpp>
#include <boost/pool/pool.hpp>
#include <boost/thread.hpp>
#include <boost/thread/once.hpp>
#include <boost/foreach.hpp>
#include <boost/assert.hpp>
#include "api.h"
#include "context.h"
#include "queryable.h"

#include "pydoc.h"

#define	EXTRACT_PARAMETERS(_params) \
	std::vector<LuxToken> aTokens; \
	std::vector<LuxPointer> aValues; \
	int count = getParametersFromPython(_params, aTokens, aValues);

#define PASS_PARAMETERS \
	count, aTokens.size()>0?&aTokens[0]:0, aValues.size()>0?&aValues[0]:0

#define PASS_PARAMSET \
	ParamSet( count, name, aTokens.size()>0?&aTokens[0]:0, aValues.size()>0?&aValues[0]:0 )

namespace lux{

boost::once_flag luxInitFlag = BOOST_ONCE_INIT;


//The memory pool handles temporary allocations and is freed after each C API Call
boost::pool<> memoryPool(sizeof(char));

//Simple function to test if pylux module was successfully loaded in python
char const* greet()
{
   return "Hello from pylux !";
}

//Here we transform a python list to lux C API parameter lists
int getParametersFromPython(boost::python::list& pList, std::vector<LuxToken>& aTokens, std::vector<LuxPointer>& aValues )
{
	boost::python::ssize_t n = boost::python::len(pList);

	//Assert parameter vectors are empty
	BOOST_ASSERT(aTokens.empty());
	BOOST_ASSERT(aValues.empty());

	for(boost::python::ssize_t i=0;i<n;i++)
	{
		// extract any object from the list
		boost::python::extract<boost::python::object> objectExtractor(pList[i]);
		boost::python::object o=objectExtractor();

		// find out its class name
		std::string object_classname = boost::python::extract<std::string>(o.attr("__class__").attr("__name__"));
		//std::cout<<"this is an Object: "<<object_classname<<std::endl;

		std::string tokenString;
		boost::python::object parameter_value;

		// handle a python-defined ParamSetItem
		if (object_classname == "ParamSetItem")
		{
			// TODO: make type_name from .type and .name and remove extra code from python ParamSetItem definition
			tokenString = boost::python::extract<std::string>(o.attr("type_name"));
			parameter_value = boost::python::extract<boost::python::object>(o.attr("value"));
		}
		// ASSUMPTION: handle a simple tuple
		else
		{
			boost::python::tuple l=boost::python::extract<boost::python::tuple>(pList[i]);
			tokenString = boost::python::extract<std::string>(l[0]);
			parameter_value = boost::python::extract<boost::python::object>(l[1]);
		}

		char *tok=(char *)memoryPool.ordered_malloc(sizeof(char)*tokenString.length()+1);
		strcpy(tok,tokenString.c_str());
		aTokens.push_back(tok);
		//std::cout<<"We have a nice parameter : ["<<tokenString<<']'<<std::endl;

		// go ahead and detect the type of the given value
		// TODO: should check this correlates with the type specified in the tokenString ?
		boost::python::extract<int> intExtractor(parameter_value);
		boost::python::extract<float> floatExtractor(parameter_value);
		boost::python::extract<boost::python::tuple> tupleExtractor(parameter_value);
		boost::python::extract<boost::python::list> listExtractor(parameter_value);
		boost::python::extract<std::string> stringExtractor(parameter_value);

		//Automatic type detection
		if(intExtractor.check())
		{
			int *pInt=(int*)memoryPool.ordered_malloc(sizeof(int));
			*pInt=intExtractor();
			aValues.push_back((LuxPointer)pInt);
			//std::cout<<"this is an INT:"<<*pInt<<std::endl;
		}
		else if(floatExtractor.check())
		{
			float *pFloat=(float*)memoryPool.ordered_malloc(sizeof(float));
			*pFloat=floatExtractor();
			aValues.push_back((LuxPointer)pFloat);
			//std::cout<<"this is an FLOAT:"<<*pFloat<<std::endl;
		}
		else if(stringExtractor.check())
		{
			std::string s=stringExtractor();
			char *pString=(char*)memoryPool.ordered_malloc(sizeof(char)*s.length()+1);
			strcpy(pString,s.c_str());
			aValues.push_back((LuxPointer)pString);
			//std::cout<<"this is a STRING:"<<*pString<<std::endl;
		}
		else if(tupleExtractor.check())
		{
			boost::python::tuple t=tupleExtractor();
			boost::python::ssize_t data_length=boost::python::len(t);

			// check 1st item for the data type in this tuple
			boost::python::extract<boost::python::object> tupleItemExtractor(t[0]);
			boost::python::object first_item=tupleItemExtractor();

			// find out its class name
			std::string first_item_classname = boost::python::extract<std::string>(first_item.attr("__class__").attr("__name__"));

			if (first_item_classname == "float")
			{
				float *pFloat=(float *)memoryPool.ordered_malloc(sizeof(float)*data_length);
				for(boost::python::ssize_t j=0;j<data_length;j++)
				{
					boost::python::extract<float> dataFloatExtractor(t[j]);
					BOOST_ASSERT(dataFloatExtractor.check());
					pFloat[j]=dataFloatExtractor();
					//std::cout<<pFloat[j]<<';';
				}
				//std::cout<<std::endl;
				aValues.push_back((LuxPointer)pFloat);
			}
			else if (first_item_classname == "int")
			{
				int *pInt=(int *)memoryPool.ordered_malloc(sizeof(int)*data_length);
				for(boost::python::ssize_t j=0;j<data_length;j++)
				{
					boost::python::extract<int> dataIntExtractor(t[j]);
					BOOST_ASSERT(dataIntExtractor.check());
					pInt[j]=dataIntExtractor();
				}
				aValues.push_back((LuxPointer)pInt);
			}
			else
			{
				//Unrecognised data type : we throw an error
				std::ostringstream o;
				o<< "Passing unrecognised data type '"<<first_item_classname<<"' in tuple to Python API for '"<<tokenString<<"' token.";
				luxError(LUX_CONSISTENCY, LUX_SEVERE, const_cast<char *>(o.str().c_str()));
			}
		}
		else if(listExtractor.check())
		{
			boost::python::list t=listExtractor();
			boost::python::ssize_t data_length=boost::python::len(t);

			// check 1st item for the data type in this list
			boost::python::extract<boost::python::object> listItemExtractor(t[0]);
			boost::python::object first_item=listItemExtractor();

			// find out its class name
			std::string first_item_classname = boost::python::extract<std::string>(first_item.attr("__class__").attr("__name__"));

			if (first_item_classname == "float")
			{
				float *pFloat=(float *)memoryPool.ordered_malloc(sizeof(float)*data_length);
				for(boost::python::ssize_t j=0;j<data_length;j++)
				{
					boost::python::extract<float> dataFloatExtractor(t[j]);
					BOOST_ASSERT(dataFloatExtractor.check());
					pFloat[j]=dataFloatExtractor();
					//std::cout<<pFloat[j]<<';';
				}
				//std::cout<<std::endl;
				aValues.push_back((LuxPointer)pFloat);
			}
			else if (first_item_classname == "int")
			{
				int *pInt=(int *)memoryPool.ordered_malloc(sizeof(int)*data_length);
				for(boost::python::ssize_t j=0;j<data_length;j++)
				{
					boost::python::extract<int> dataIntExtractor(t[j]);
					BOOST_ASSERT(dataIntExtractor.check());
					pInt[j]=dataIntExtractor();
				}
				aValues.push_back((LuxPointer)pInt);
			}
			else
			{
				//Unrecognised data type : we throw an error
				std::ostringstream o;
				o<< "Passing unrecognised data type '"<<first_item_classname<<"' in list to Python API for '"<<tokenString<<"' token.";
				luxError(LUX_CONSISTENCY, LUX_SEVERE, const_cast<char *>(o.str().c_str()));
			}
		}
		else
		{
			//Unrecognised parameter type : we throw an error
			std::ostringstream o;
			o<< "Passing unrecognised parameter type to Python API for '"<<tokenString<<"' token.";
			luxError(LUX_CONSISTENCY, LUX_SEVERE, const_cast<char *>(o.str().c_str()));
		}

	}
	return(n);
}

//Error handling
boost::python::object pythonErrorHandler;

void luxErrorPython(int code, int severity, const char *message)
{
	pythonErrorHandler(code, severity, message);
}

void pyLuxErrorHandler(boost::python::object handler)
{
	//System wide init (usefull if you set the handler before any context is created)
	boost::call_once(&luxInit, luxInitFlag);

	pythonErrorHandler=handler;
	luxErrorHandler(luxErrorPython);
}

/*
import pylux
import threading
pylux.init()
pylux.lookAt(0,0,1,0,1,0,0,0,1)
pylux.worldBegin()
pylux.lightSource('infinite', [])
pylux.worldEnd()
*/

/*
 * PyContext class
 */

class PyContext
{
public:
	PyContext(std::string name)
	{
		//System wide init
		boost::call_once(&luxInit, luxInitFlag);

		//Here we create a new context
		context=new Context(name);
		LOG(LUX_INFO,LUX_NOERROR)<<"Created new context : '"<<name<<"'";
	}

	~PyContext()
	{
		//destroy threads
		BOOST_FOREACH(boost::thread *t,pyLuxWorldEndThreads)
		{
			delete(t);
		}
		pyLuxWorldEndThreads.clear();
	}

	void greet() { LOG(LUX_INFO,LUX_NOERROR)<<"Hello from context '"<<context->GetName()<<"' !"; }

	int parse(const char *filename, bool async)
	{
		//TODO jromang - add thread lock here (we can only parse in one context)
		Context::SetActive(context);
		if (async)
		{
			pyLuxWorldEndThreads.push_back(new boost::thread( boost::bind(luxParse, filename) ));
			return true;
		}
		else
		{
			return luxParse(filename);
		}
	}

	void cleanup() {
		Context::SetActive(context);
		context->Cleanup();
	}
	void identity() {
		Context::SetActive(context);
		context->Identity();
	}
	void translate(float dx, float dy, float dz) {
		Context::SetActive(context);
		context->Translate(dx,dy,dz);
	}
	void rotate(float angle, float ax, float ay, float az) {
		Context::SetActive(context);
		context->Rotate(angle,ax,ay,az);
	}
	void scale(float sx, float sy, float sz) {
		Context::SetActive(context);
		context->Scale(sx,sy,sz);
	}
	void lookAt(float ex, float ey, float ez, float lx, float ly, float lz, float ux, float uy, float uz) {
		Context::SetActive(context);
		context->LookAt(ex, ey, ez, lx, ly, lz, ux, uy, uz);
	}

	void concatTransform(boost::python::list tx)
	{

		boost::python::extract<boost::python::list> listExtractor(tx);

		//std::cout<<"this is a LIST - WARNING ASSUMING FLOATS :";
		boost::python::list t=listExtractor();
		boost::python::ssize_t listSize=boost::python::len(t);
		float *pFloat=(float *)memoryPool.ordered_malloc(sizeof(float)*listSize);
		for(boost::python::ssize_t j=0;j<listSize;j++)
		{
				boost::python::extract<float> listFloatExtractor(t[j]);
				//jromang - Assuming floats here, but do we only have floats in lists ?
				BOOST_ASSERT(listFloatExtractor.check());
				pFloat[j]=listFloatExtractor();
				//std::cout<<pFloat[j]<<';';
		}
		//std::cout<<std::endl;

		Context::SetActive(context);
		context->ConcatTransform(pFloat);
		memoryPool.purge_memory();
	}

	void transform(boost::python::list tx)
	{
		boost::python::extract<boost::python::list> listExtractor(tx);

		//std::cout<<"this is a LIST - WARNING ASSUMING FLOATS :";
		boost::python::list t=listExtractor();
		boost::python::ssize_t listSize=boost::python::len(t);
		float *pFloat=(float *)memoryPool.ordered_malloc(sizeof(float)*listSize);
		for(boost::python::ssize_t j=0;j<listSize;j++)
		{
				boost::python::extract<float> listFloatExtractor(t[j]);
				//jromang - Assuming floats here, but do we only have floats in lists ?
				BOOST_ASSERT(listFloatExtractor.check());
				pFloat[j]=listFloatExtractor();
				//std::cout<<pFloat[j]<<';';
		}
		//std::cout<<std::endl;

		Context::SetActive(context);
		context->Transform(pFloat);
				memoryPool.purge_memory();
	}

	void coordinateSystem(const char *name) {
		Context::SetActive(context);
		context->CoordinateSystem(std::string(name));
	}
	void coordSysTransform(const char *name) {
		Context::SetActive(context);
		context->CoordSysTransform(std::string(name));
	}

	void pixelFilter(const char *name, boost::python::list params)
	{
		EXTRACT_PARAMETERS(params);
		Context::SetActive(context);
		context->PixelFilter(name,PASS_PARAMSET);
		memoryPool.purge_memory();
	}

	void film(const char *name, boost::python::list params)
	{
		EXTRACT_PARAMETERS(params);
		Context::SetActive(context);
		context->Film(name, PASS_PARAMSET);
		memoryPool.purge_memory();
	}

	void sampler(const char *name, boost::python::list params)
	{
		EXTRACT_PARAMETERS(params);
		Context::SetActive(context);
		context->Sampler(name, PASS_PARAMSET);
		memoryPool.purge_memory();
	}

	void accelerator(const char *name, boost::python::list params)
	{
		EXTRACT_PARAMETERS(params);
		Context::SetActive(context);
		context->Accelerator(name, PASS_PARAMSET);
		memoryPool.purge_memory();
	}

	void surfaceIntegrator(const char *name, boost::python::list params)
	{
		EXTRACT_PARAMETERS(params);
		Context::SetActive(context);
		context->SurfaceIntegrator(name, PASS_PARAMSET);
		memoryPool.purge_memory();
	}

	void volumeIntegrator(const char *name, boost::python::list params)
	{
		EXTRACT_PARAMETERS(params);
		Context::SetActive(context);
		context->VolumeIntegrator(name,PASS_PARAMSET);
		memoryPool.purge_memory();
	}

	void camera(const char *name, boost::python::list params)
	{
		EXTRACT_PARAMETERS(params);
		Context::SetActive(context);
		context->Camera(name, PASS_PARAMSET);
		memoryPool.purge_memory();
	}

	void worldBegin() {
		Context::SetActive(context);
		context->WorldBegin();
	}
	void attributeBegin() {
		Context::SetActive(context);
		context->AttributeBegin();
	}
	void attributeEnd() {
		Context::SetActive(context);
		context->AttributeEnd();
	}
	void transformBegin() {
		Context::SetActive(context);
		context->TransformBegin();
	}
	void transformEnd() {
		Context::SetActive(context);
		context->TransformEnd();
	}

	void texture(const char *name, const char *type, const char *texname, boost::python::list params)
	{
		EXTRACT_PARAMETERS(params);
		Context::SetActive(context);
		context->Texture(name, type, texname, PASS_PARAMSET);
		memoryPool.purge_memory();
	}

	void material(const char *name, boost::python::list params)
	{

		EXTRACT_PARAMETERS(params);
		Context::SetActive(context);
		context->Material(name, PASS_PARAMSET);
		memoryPool.purge_memory();
	}

	void makeNamedMaterial(const char *name, boost::python::list params)
	{
		EXTRACT_PARAMETERS(params);
		Context::SetActive(context);
		context->MakeNamedMaterial(name, PASS_PARAMSET);
		memoryPool.purge_memory();
	}

	void namedMaterial(const char *name) {
		Context::SetActive(context);
		context->NamedMaterial(name);
	}

	void lightGroup(const char *name, boost::python::list params)
	{
		EXTRACT_PARAMETERS(params);
		Context::SetActive(context);
		context->LightGroup(name, PASS_PARAMSET);
		memoryPool.purge_memory();
	}

	void lightSource(const char *name, boost::python::list params)
	{
		EXTRACT_PARAMETERS(params);
		Context::SetActive(context);
		context->LightSource(name, PASS_PARAMSET);
		memoryPool.purge_memory();
	}

	void areaLightSource(const char *name, boost::python::list params)
	{
		EXTRACT_PARAMETERS(params);
		Context::SetActive(context);
		context->AreaLightSource(name, PASS_PARAMSET);
		memoryPool.purge_memory();
	}

	void portalShape(const char *name, boost::python::list params)
	{
		EXTRACT_PARAMETERS(params);
		Context::SetActive(context);
		context->PortalShape(name, PASS_PARAMSET);
		memoryPool.purge_memory();
	}

	void shape(const char *name, boost::python::list params)
	{
		EXTRACT_PARAMETERS(params);
		Context::SetActive(context);
		context->Shape(name, PASS_PARAMSET);
		memoryPool.purge_memory();
	}

	void reverseOrientation() {
		Context::SetActive(context);
		context->ReverseOrientation();
	}

	void makeNamedVolume(const char *id, const char *name, boost::python::list params)
	{
		EXTRACT_PARAMETERS(params);
		Context::SetActive(context);
		context->MakeNamedVolume(id, name, PASS_PARAMSET);
		memoryPool.purge_memory();
	}

	void volume(const char *name, boost::python::list params)
	{
		EXTRACT_PARAMETERS(params);
		Context::SetActive(context);
		context->Volume(name, PASS_PARAMSET);
		memoryPool.purge_memory();
	}

	void exterior(const char *name)
	{
		Context::SetActive(context);
		context->Exterior(name);
	}

	void interior(const char *name)
	{
		Context::SetActive(context);
		context->Interior(name);
	}

	void objectBegin(const char *name) {
		Context::SetActive(context);
		context->ObjectBegin(std::string(name));
	}
	void objectEnd() {
		Context::SetActive(context);
		context->ObjectEnd();
	}
	void objectInstance(const char *name) {
		Context::SetActive(context);
		context->ObjectInstance(std::string(name));
	}

	void motionInstance(const char *name, float startTime, float endTime, const char *toTransform)
	{
		Context::SetActive(context);
		context->MotionInstance(std::string(name), startTime, endTime, std::string(toTransform));
	}

	void worldEnd() //launch luxWorldEnd() into a thread
	{
		pyLuxWorldEndThreads.push_back(new boost::thread( boost::bind(&PyContext::pyWorldEnd,this) ));
	}

	void loadFLM(const char* name) {
		Context::SetActive(context);
		context->LoadFLM(std::string(name));
	}
	void saveFLM(const char* name) {
		Context::SetActive(context);
		context->SaveFLM(std::string(name));
	}
	void overrideResumeFLM(const char *name) {
		Context::SetActive(context);
		context->OverrideResumeFLM(string(name));
	}
	void start() {
		Context::SetActive(context);
		context->Resume();
	}
	void pause() {
		Context::SetActive(context);
		context->Pause();
	}
	void exit() {
		Context::SetActive(context);
		context->Exit();
	}
	void wait() {
		Context::SetActive(context);
		context->Wait();
	}

	void setHaltSamplePerPixel(int haltspp, bool haveEnoughSamplePerPixel, bool suspendThreadsWhenDone)
	{
		Context::SetActive(context);
		context->SetHaltSamplePerPixel(haltspp, haveEnoughSamplePerPixel, suspendThreadsWhenDone);
	}

	unsigned int addThread() {
		Context::SetActive(context);
		return context->AddThread();
	}
	void removeThread() {
		Context::SetActive(context);
		context->RemoveThread();
	}

	void setEpsilon(const float minValue, const float maxValue)
	{
		Context::SetActive(context);
		context->SetEpsilon(minValue < 0.f ? DEFAULT_EPSILON_MIN : minValue, maxValue < 0.f ? DEFAULT_EPSILON_MAX : maxValue);
	}

	void updateFramebuffer() {
		Context::SetActive(context);
		context->UpdateFramebuffer();
	}

	boost::python::list framebuffer()
	{
		boost::python::list pyFrameBuffer;
		int nvalues=((int)luxStatistics("filmXres")) * ((int)luxStatistics("filmYres")) * 3; //get the number of values to copy

		Context::SetActive(context);
		unsigned char* framebuffer=luxFramebuffer(); //get the framebuffer
		//copy the values
		for(int i=0;i<nvalues;i++)
			pyFrameBuffer.append(framebuffer[i]);
		return pyFrameBuffer;
	}

	boost::python::list getHistogramImage(unsigned int width, unsigned int height, int options)
	{
		boost::python::list pyHistogramImage;
		int nvalues=width*height;
		unsigned char* outPixels = new unsigned char[nvalues];

		Context::SetActive(context);
		context->GetHistogramImage(outPixels, width, height, options);

		for(int i=0;i<nvalues;i++)
			pyHistogramImage.append(outPixels[i]);
		delete[] outPixels;
		return pyHistogramImage;
	}

	void setParameterValue(luxComponent comp, luxComponentParameters param, double value, unsigned int index)
	{
		Context::SetActive(context);
		return context->SetParameterValue(comp, param, value, index);
	}

	double getParameterValue(luxComponent comp, luxComponentParameters param, unsigned int index)
	{
		Context::SetActive(context);
		return context->GetParameterValue(comp, param, index);
	}

	double getDefaultParameterValue(luxComponent comp, luxComponentParameters param, unsigned int index)
	{
		Context::SetActive(context);
		return context->GetDefaultParameterValue(comp, param, index);
	}

	void setStringParameterValue(luxComponent comp, luxComponentParameters param, const char* value, unsigned int index)
	{
		Context::SetActive(context);
		return context->SetStringParameterValue(comp, param, value, index);
	}

	unsigned int getStringParameterValue(luxComponent comp, luxComponentParameters param, char* dst, unsigned int dstlen, unsigned int index)
	{
		Context::SetActive(context);
		const string str = context->GetStringParameterValue(comp, param, index);
		unsigned int nToCopy = str.length() < dstlen ?
			str.length() + 1 : dstlen;
		if (nToCopy > 0) {
			strncpy(dst, str.c_str(), nToCopy - 1);
			dst[nToCopy - 1] = '\0';
		}
		return str.length();
	}

	unsigned int getDefaultStringParameterValue(luxComponent comp, luxComponentParameters param, char* dst, unsigned int dstlen, unsigned int index)
	{
		Context::SetActive(context);
		const string str = context->GetDefaultStringParameterValue(comp, param, index);
		unsigned int nToCopy = str.length() < dstlen ?
			str.length() + 1 : dstlen;
		if (nToCopy > 0) {
			strncpy(dst, str.c_str(), nToCopy - 1);
			dst[nToCopy - 1] = '\0';
		}
		return str.length();
	}

	const char* getOptions() {
		Context::SetActive(context);
		return context->registry.GetContent();
	}

	//Queryable objects
	//Here I do a special handling for python :
	//Python is dynamically typed, unlike C++ which is statically typed.
	//Python variables may hold an integer, a float, list, dict, tuple, str, long etc., among other things.
	//So we don't need a getINT, getFLOAT, getXXX in the python api
	//This function handles all types
	boost::python::object getOption(const char *objectName, const char *attributeName)
	{
		Context::SetActive(context);
		Queryable *object=context->registry[objectName];
		if(object!=0)
		{
			QueryableAttribute attr=(*object)[attributeName];
			if(attr.type==ATTRIBUTE_INT) return boost::python::object(attr.IntValue());
			if(attr.type==ATTRIBUTE_FLOAT) return boost::python::object(attr.FloatValue());
			if(attr.type==ATTRIBUTE_STRING) return boost::python::object(attr.Value());

			LOG(LUX_ERROR,LUX_BUG)<<"Unknown attribute type in pyLuxGetOption";
		}
		return boost::python::object(0);
	}

	void setOption(const char * objectName, const char * attributeName, boost::python::object value)
	{
		//void luxSetOption(const char * objectName, const char * attributeName, int n, void *values); /* Sets an option value */
		Context::SetActive(context);
		Queryable *object=context->registry[objectName];
		if(object!=0)
		{
			QueryableAttribute &attribute=(*object)[attributeName];
			//return (*object)[attributeName].IntValue();
			switch(attribute.type)
			{
			case ATTRIBUTE_INT :
				attribute.SetValue(boost::python::extract<int>(value));
				break;

			case ATTRIBUTE_FLOAT :
				attribute.SetValue(boost::python::extract<float>(value));
				break;

			case ATTRIBUTE_STRING :
				attribute.SetValue(boost::python::extract<std::string>(value));
				break;

			case ATTRIBUTE_NONE :
			default:
				LOG(LUX_ERROR,LUX_BUG)<<"Unknown attribute type for '"<<attributeName<<"' in object '"<<objectName<<"'";
			}
		}
		else
		{
			LOG(LUX_ERROR,LUX_BADTOKEN)<<"Unknown object '"<<objectName<<"'";
		}
	}


	void addServer(const char * name) {
		Context::SetActive(context);
		context->AddServer(std::string(name));
	}
	void removeServer(const char * name) {
		Context::SetActive(context);
		context->RemoveServer(std::string(name));
	}
	unsigned int getServerCount() {
		Context::SetActive(context);
		return context->GetServerCount();
	}
	void updateFilmFromNetwork() {
		Context::SetActive(context);
		context->UpdateFilmFromNetwork();
	}
	void setNetworkServerUpdateInterval(int updateInterval) {
		Context::SetActive(context);
		context->SetNetworkServerUpdateInterval(updateInterval);
	}
	int getNetworkServerUpdateInterval() {
		Context::SetActive(context);
		return context->GetNetworkServerUpdateInterval();
	}

	unsigned int getRenderingServersStatus(RenderingServerInfo *info, unsigned int maxInfoCount)
	{
		Context::SetActive(context);
		return context->GetRenderingServersStatus(info, maxInfoCount);
	}

	double statistics(const char *statName) {
		Context::SetActive(context);
		return context->Statistics(statName);
	}

	void enableDebugMode() {
		Context::SetActive(context);
		context->EnableDebugMode();
	}
	void disableRandomMode() {
		Context::SetActive(context);
		context->DisableRandomMode();
	}

private:
	Context *context;

	std::vector<boost::thread *> pyLuxWorldEndThreads; //hold pointers to the worldend threads
	void pyWorldEnd()
	{
		Context::SetActive(context);
		context->WorldEnd();
	}
};


}//namespace lux

/*
 *  Finally, we create the python module using boost/python !
 */
BOOST_PYTHON_MODULE(pylux)
{
	using namespace boost::python;
	using namespace lux;
	docstring_options doc_options(
		true	/* show user defined docstrings */,
		true	/* show python signatures */,
		false	/* show c++ signatures */
	);

	scope().attr("__doc__") = ds_pylux;

	//Direct python module calls
	// def("greet", greet); //Simple test function to check the module is imported
	def("version", luxVersion, ds_pylux_version);

	// Parameter access
	enum_<luxComponent>("luxComponent", ds_pylux_luxComponent)
		.value("LUX_FILM", LUX_FILM)
		;

	enum_<luxComponentParameters>("luxComponentParameters", ds_pylux_luxComponentParameters)
		.value("LUX_FILM_TM_TONEMAPKERNEL",LUX_FILM_TM_TONEMAPKERNEL)
		.value("LUX_FILM_TM_REINHARD_PRESCALE",LUX_FILM_TM_REINHARD_PRESCALE)
		.value("LUX_FILM_TM_REINHARD_POSTSCALE",LUX_FILM_TM_REINHARD_POSTSCALE)
		.value("LUX_FILM_TM_REINHARD_BURN",LUX_FILM_TM_REINHARD_BURN)
		.value("LUX_FILM_TM_LINEAR_SENSITIVITY",LUX_FILM_TM_LINEAR_SENSITIVITY)
		.value("LUX_FILM_TM_LINEAR_EXPOSURE",LUX_FILM_TM_LINEAR_EXPOSURE)
		.value("LUX_FILM_TM_LINEAR_FSTOP",LUX_FILM_TM_LINEAR_FSTOP)
		.value("LUX_FILM_TM_LINEAR_GAMMA",LUX_FILM_TM_LINEAR_GAMMA)
		.value("LUX_FILM_TM_CONTRAST_YWA",LUX_FILM_TM_CONTRAST_YWA)
		.value("LUX_FILM_TORGB_X_WHITE",LUX_FILM_TORGB_X_WHITE)
		.value("LUX_FILM_TORGB_Y_WHITE",LUX_FILM_TORGB_Y_WHITE)
		.value("LUX_FILM_TORGB_X_RED",LUX_FILM_TORGB_X_RED)
		.value("LUX_FILM_TORGB_Y_RED",LUX_FILM_TORGB_Y_RED)
		.value("LUX_FILM_TORGB_X_GREEN",LUX_FILM_TORGB_X_GREEN)
		.value("LUX_FILM_TORGB_Y_GREEN",LUX_FILM_TORGB_Y_GREEN)
		.value("LUX_FILM_TORGB_X_BLUE",LUX_FILM_TORGB_X_BLUE)
		.value("LUX_FILM_TORGB_Y_BLUE",LUX_FILM_TORGB_Y_BLUE)
		.value("LUX_FILM_TORGB_GAMMA",LUX_FILM_TORGB_GAMMA)
		.value("LUX_FILM_UPDATEBLOOMLAYER",LUX_FILM_UPDATEBLOOMLAYER)
		.value("LUX_FILM_DELETEBLOOMLAYER",LUX_FILM_DELETEBLOOMLAYER)
		.value("LUX_FILM_BLOOMRADIUS",LUX_FILM_BLOOMRADIUS)
		.value("LUX_FILM_BLOOMWEIGHT",LUX_FILM_BLOOMWEIGHT)
		.value("LUX_FILM_VIGNETTING_ENABLED",LUX_FILM_VIGNETTING_ENABLED)
		.value("LUX_FILM_VIGNETTING_SCALE",LUX_FILM_VIGNETTING_SCALE)
		.value("LUX_FILM_ABERRATION_ENABLED",LUX_FILM_ABERRATION_ENABLED)
		.value("LUX_FILM_ABERRATION_AMOUNT",LUX_FILM_ABERRATION_AMOUNT)
		.value("LUX_FILM_UPDATEGLARELAYER",LUX_FILM_UPDATEGLARELAYER)
		.value("LUX_FILM_DELETEGLARELAYER",LUX_FILM_DELETEGLARELAYER)
		.value("LUX_FILM_GLARE_AMOUNT",LUX_FILM_GLARE_AMOUNT)
		.value("LUX_FILM_GLARE_RADIUS",LUX_FILM_GLARE_RADIUS)
		.value("LUX_FILM_GLARE_BLADES",LUX_FILM_GLARE_BLADES)
		.value("LUX_FILM_GLARE_THRESHOLD",LUX_FILM_GLARE_THRESHOLD)
		.value("LUX_FILM_HISTOGRAM_ENABLED",LUX_FILM_HISTOGRAM_ENABLED)
		.value("LUX_FILM_NOISE_CHIU_ENABLED",LUX_FILM_NOISE_CHIU_ENABLED)
		.value("LUX_FILM_NOISE_CHIU_RADIUS",LUX_FILM_NOISE_CHIU_RADIUS)
		.value("LUX_FILM_NOISE_CHIU_INCLUDECENTER",LUX_FILM_NOISE_CHIU_INCLUDECENTER)
		.value("LUX_FILM_NOISE_GREYC_ENABLED",LUX_FILM_NOISE_GREYC_ENABLED)
		.value("LUX_FILM_NOISE_GREYC_AMPLITUDE",LUX_FILM_NOISE_GREYC_AMPLITUDE)
		.value("LUX_FILM_NOISE_GREYC_NBITER",LUX_FILM_NOISE_GREYC_NBITER)
		.value("LUX_FILM_NOISE_GREYC_SHARPNESS",LUX_FILM_NOISE_GREYC_SHARPNESS)
		.value("LUX_FILM_NOISE_GREYC_ANISOTROPY",LUX_FILM_NOISE_GREYC_ANISOTROPY)
		.value("LUX_FILM_NOISE_GREYC_ALPHA",LUX_FILM_NOISE_GREYC_ALPHA)
		.value("LUX_FILM_NOISE_GREYC_SIGMA",LUX_FILM_NOISE_GREYC_SIGMA)
		.value("LUX_FILM_NOISE_GREYC_FASTAPPROX",LUX_FILM_NOISE_GREYC_FASTAPPROX)
		.value("LUX_FILM_NOISE_GREYC_GAUSSPREC",LUX_FILM_NOISE_GREYC_GAUSSPREC)
		.value("LUX_FILM_NOISE_GREYC_DL",LUX_FILM_NOISE_GREYC_DL)
		.value("LUX_FILM_NOISE_GREYC_DA",LUX_FILM_NOISE_GREYC_DA)
		.value("LUX_FILM_NOISE_GREYC_INTERP",LUX_FILM_NOISE_GREYC_INTERP)
		.value("LUX_FILM_NOISE_GREYC_TILE",LUX_FILM_NOISE_GREYC_TILE)
		.value("LUX_FILM_NOISE_GREYC_BTILE",LUX_FILM_NOISE_GREYC_BTILE)
		.value("LUX_FILM_NOISE_GREYC_THREADS",LUX_FILM_NOISE_GREYC_THREADS)
		.value("LUX_FILM_LG_COUNT",LUX_FILM_LG_COUNT)
		.value("LUX_FILM_LG_ENABLE",LUX_FILM_LG_ENABLE)
		.value("LUX_FILM_LG_NAME",LUX_FILM_LG_NAME)
		.value("LUX_FILM_LG_SCALE",LUX_FILM_LG_SCALE)
		.value("LUX_FILM_LG_SCALE_RED",LUX_FILM_LG_SCALE_RED)
		.value("LUX_FILM_LG_SCALE_BLUE",LUX_FILM_LG_SCALE_BLUE)
		.value("LUX_FILM_LG_SCALE_GREEN",LUX_FILM_LG_SCALE_GREEN)
		.value("LUX_FILM_LG_TEMPERATURE",LUX_FILM_LG_TEMPERATURE)
		.value("LUX_FILM_LG_SCALE_X",LUX_FILM_LG_SCALE_X)
		.value("LUX_FILM_LG_SCALE_Y",LUX_FILM_LG_SCALE_Y)
		.value("LUX_FILM_LG_SCALE_Z",LUX_FILM_LG_SCALE_Z)
		;

	class_<RenderingServerInfo>("RenderingServerInfo", ds_pylux_RenderingServerInfo)
		.def_readonly("serverIndex", &RenderingServerInfo::serverIndex)
		.def_readonly("name", &RenderingServerInfo::name)
		.def_readonly("port", &RenderingServerInfo::port)
		.def_readonly("sid", &RenderingServerInfo::sid)
		.def_readonly("numberOfSamplesReceived", &RenderingServerInfo::numberOfSamplesReceived)
		.def_readonly("secsSinceLastContact", &RenderingServerInfo::secsSinceLastContact)
		;

	//Error handling in python
	def("errorHandler",
		pyLuxErrorHandler,
		args("function"),
		ds_pylux_errorHandler
	);

	//PyContext class
	class_<PyContext>(
		"Context",
		ds_pylux_Context,
		init<std::string>(args("Context", "name"), ds_pylux_Context_init)
		)
		//.def("greet", &PyContext::greet)
		.def("accelerator",
			&PyContext::accelerator,
			args("Context", "type", "ParamSet"),
			ds_pylux_Context_accelerator
		)
		.def("addServer",
			&PyContext::addServer,
			args("Context", "address"),
			ds_pylux_Context_addServer
		)
		.def("addThread",
			&PyContext::addThread,
			args("Context"),
			ds_pylux_Context_addThread
		)
		.def("areaLightSource",
			&PyContext::areaLightSource,
			args("Context", "type", "ParamSet"),
			ds_pylux_Context_areaLightSource
		)
		.def("attributeBegin",
			&PyContext::attributeBegin,
			args("Context"),
			ds_pylux_Context_attributeBegin
		)
		.def("attributeEnd",
			&PyContext::attributeEnd,
			args("Context"),
			ds_pylux_Context_attributeEnd
		)
		.def("camera",
			&PyContext::camera,
			args("Context", "type", "ParamSet"),
			ds_pylux_Context_camera
		)
		.def("cleanup",
			&PyContext::cleanup,
			args("Context"),
			ds_pylux_Context_cleanup
		)
		.def("concatTransform",
			&PyContext::concatTransform,
			args("Context", "transform"),
			ds_pylux_Context_concatTransform
		)
		.def("coordSysTransform",
			&PyContext::coordSysTransform,
			args("Context", "name"),
			ds_pylux_Context_coordSysTransform
		)
		.def("coordinateSystem",
			&PyContext::coordinateSystem,
			args("Context", "name"),
			ds_pylux_Context_coordinateSystem
		)
		.def("disableRandomMode",
			&PyContext::disableRandomMode,
			args("Context"),
			ds_pylux_Context_disableRandomMode
		)
		.def("enableDebugMode",
			&PyContext::enableDebugMode,
			args("Context"),
			ds_pylux_Context_enableDebugMode
		)
		.def("exit",
			&PyContext::exit,
			args("Context"),
			ds_pylux_Context_exit
		)
		.def("exterior",
			&PyContext::exterior,
			args("Context", "VolumeName"),
			ds_pylux_Context_exterior
		)
		.def("film",
			&PyContext::film,
			args("Context", "type", "ParamSet"),
			ds_pylux_Context_film
		)
		.def("framebuffer",
			&PyContext::framebuffer,
			args("Context"),
			ds_pylux_Context_framebuffer
		)
		.def("getDefaultParameterValue",
			&PyContext::getDefaultParameterValue,
			args("Context", "component", "parameter", "index"),
			ds_pylux_Context_getDefaultParameterValue
		)
		.def("getDefaultStringParameterValue",
			&PyContext::getDefaultStringParameterValue,
			args("Context"),
			ds_pylux_Context_getDefaultStringParameterValue
		)
		.def("getHistogramImage",
			&PyContext::getHistogramImage,
			args("Context", "width", "height", "options"),
			ds_pylux_Context_getHistogramImage
		)
		.def("getNetworkServerUpdateInterval",
			&PyContext::getNetworkServerUpdateInterval,
			args("Context"),
			ds_pylux_Context_getNetworkServerUpdateInterval
		)
		.def("getOption",
			&PyContext::getOption,
			args("Context"),
			ds_pylux_Context_getOption
		)
		.def("getOptions",
			&PyContext::getOptions,
			args("Context"),
			ds_pylux_Context_getOptions
		)
		.def("getParameterValue",
			&PyContext::getParameterValue,
			args("Context", "component", "parameter", "index"),
			ds_pylux_Context_getParameterValue
		)
		.def("getRenderingServersStatus",
			&PyContext::getRenderingServersStatus,
			args("Context", "ServerStatusObject", "index"),
			ds_pylux_Context_getRenderingServersStatus
		)
		.def("getServerCount",
			&PyContext::getServerCount,
			args("Context"),
			ds_pylux_Context_getServerCount
		)
		.def("getStringParameterValue",
			&PyContext::getStringParameterValue,
			args("Context"),
			ds_pylux_Context_getStringParameterValue
		)
		.def("identity",
			&PyContext::identity,
			args("Context"),
			ds_pylux_Context_identity
		)
		.def("interior",
			&PyContext::interior,
			args("Context", "VolumeName"),
			ds_pylux_Context_interior
		)
		.def("lightGroup",
			&PyContext::lightGroup,
			args("Context", "name", "ParamSet"),
			ds_pylux_Context_lightGroup
		)
		.def("lightSource",
			&PyContext::lightSource,
			args("Context", "type", "ParamSet"),
			ds_pylux_Context_lightSource
		)
		.def("loadFLM",
			&PyContext::loadFLM,
			args("Context", "filename"),
			ds_pylux_Context_loadFLM
		)
		.def("lookAt",
			&PyContext::lookAt,
			args("Context", "pos0", "pos1", "pos2", "trg0", "trg1", "trg2", "up0", "up1", "up2"),
			ds_pylux_Context_lookAt
		)
		.def("makeNamedMaterial",
			&PyContext::makeNamedMaterial,
			args("Context", "name", "ParamSet"),
			ds_pylux_Context_makeNamedMaterial
		)
		.def("makeNamedVolume",
			&PyContext::makeNamedVolume,
			args("Context", "name", "type", "ParamSet"),
			ds_pylux_Context_makeNamedVolume
		)
		.def("material",
			&PyContext::material,
			args("Context", "type", "ParamSet"),
			ds_pylux_Context_material
		)
		.def("motionInstance",
			&PyContext::motionInstance,
			args("Context"),
			ds_pylux_Context_motionInstance
		)
		.def("namedMaterial",
			&PyContext::namedMaterial,
			args("Context", "name"),
			ds_pylux_Context_namedMaterial
		)
		.def("objectBegin",
			&PyContext::objectBegin,
			args("Context", "name"),
			ds_pylux_Context_objectBegin
		)
		.def("objectEnd",
			&PyContext::objectEnd,
			args("Context"),
			ds_pylux_Context_objectEnd
		)
		.def("objectInstance",
			&PyContext::objectInstance,
			args("Context", "name"),
			ds_pylux_Context_objectInstance
		)
		.def("overrideResumeFLM",
			&PyContext::overrideResumeFLM,
			args("Context"),
			ds_pylux_Context_overrideResumeFLM
		)
		.def("parse",
			&PyContext::parse,
			args("Context", "filename", "asynchronous"),
			ds_pylux_Context_parse
			)
		.def("pause",
			&PyContext::pause,
			args("Context"),
			ds_pylux_Context_pause
		)
		.def("pixelFilter",
			&PyContext::pixelFilter,
			args("Context", "type", "ParamSet"),
			ds_pylux_Context_pixelFilter
		)
		.def("portalShape",
			&PyContext::portalShape,
			args("Context", "type", "ParamSet"),
			ds_pylux_Context_portalShape
		)
		.def("removeServer",
			&PyContext::removeServer,
			args("Context", "address"),
			ds_pylux_Context_removeServer
		)
		.def("removeThread",
			&PyContext::removeThread,
			args("Context"),
			ds_pylux_Context_removeThread
		)
		.def("reverseOrientation",
			&PyContext::reverseOrientation,
			args("Context"),
			ds_pylux_Context_reverseOrientation
		)
		.def("rotate",
			&PyContext::rotate,
			args("Context", "degrees", "x", "y", "z"),
			ds_pylux_Context_rotate
		)
		.def("sampler",
			&PyContext::sampler,
			args("Context", "type", "ParaSet"),
			ds_pylux_Context_sampler
		)
		.def("saveFLM",
			&PyContext::saveFLM,
			args("Context", "filename"),
			ds_pylux_Context_saveFLM
		)
		.def("scale",
			&PyContext::scale,
			args("Context", "x", "y", "z"),
			ds_pylux_Context_scale
		)
		.def("setEpsilon",
			&PyContext::setEpsilon,
			args("Context"),
			ds_pylux_Context_setEpsilon
		)
		.def("setHaltSamplePerPixel",
			&PyContext::setHaltSamplePerPixel,
			args("Context"),
			ds_pylux_Context_setHaltSamplePerPixel
		)
		.def("setNetworkServerUpdateInterval",
			&PyContext::setNetworkServerUpdateInterval,
			args("Context"),
			ds_pylux_Context_setNetworkServerUpdateInterval
		)
		.def("setOption",
			&PyContext::setOption,
			args("Context"),
			ds_pylux_Context_setOption
		)
		.def("setParameterValue",
			&PyContext::setParameterValue,
			args("Context"),
			ds_pylux_Context_setParameterValue
		)
		.def("setStringParameterValue",
			&PyContext::setStringParameterValue,
			args("Context"),
			ds_pylux_Context_setStringParameterValue
		)
		.def("shape",
			&PyContext::shape,
			args("Context", "type", "ParamSet"),
			ds_pylux_Context_shape
		)
		.def("start",
			&PyContext::start,
			args("Context"),
			ds_pylux_Context_start
		)
		.def("statistics",
			&PyContext::statistics,
			args("Context", "name"),
			ds_pylux_Context_statistics
		)
		.def("surfaceIntegrator",
			&PyContext::surfaceIntegrator,
			args("Context", "type", "ParamSet"),
			ds_pylux_Context_surfaceIntegrator
		)
		.def("texture",
			&PyContext::texture,
			args("Context", "name", "variant", "type", "ParamSet"),
			ds_pylux_Context_texture
		)
		.def("transform",
			&PyContext::transform,
			args("Context", "transform"),
			ds_pylux_Context_transform
		)
		.def("transformBegin",
			&PyContext::transformBegin,
			args("Context"),
			ds_pylux_Context_transformBegin
		)
		.def("transformEnd",
			&PyContext::transformEnd,
			args("Context"),
			ds_pylux_Context_transformEnd
		)
		.def("translate",
			&PyContext::translate,
			args("Context", "x", "y", "z"),
			ds_pylux_Context_translate
		)
		.def("updateFilmFromNetwork",
			&PyContext::updateFilmFromNetwork,
			args("Context"),
			ds_pylux_Context_updateFilmFromNetwork
		)
		.def("updateFramebuffer",
			&PyContext::updateFramebuffer,
			args("Context"),
			ds_pylux_Context_updateFramebuffer
		)
		.def("volume",
			&PyContext::volume,
			args("Context", "type", "ParamSet"),
			ds_pylux_Context_volume
		)
		.def("volumeIntegrator",
			&PyContext::volumeIntegrator,
			args("Context", "type", "ParamSet"),
			ds_pylux_Context_volumeIntegrator
		)
		.def("wait",
			&PyContext::wait,
			args("Context"),
			ds_pylux_Context_wait
		)
		.def("worldBegin",
			&PyContext::worldBegin,
			args("Context"),
			ds_pylux_Context_worldBegin
		)
		.def("worldEnd",
			&PyContext::worldEnd,
			args("Context"),
			ds_pylux_Context_worldEnd
		)
		;
}
