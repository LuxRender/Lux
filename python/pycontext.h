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

#ifndef LUX_PYCONTEXT_H
#define LUX_PYCONTEXT_H

#include <vector>

#include <boost/assert.hpp>
#include <boost/foreach.hpp>
#include <boost/pool/pool.hpp>
#include <boost/thread.hpp>
#include <boost/shared_array.hpp>

#include "context.h"
#include "queryable.h"
#include "api.h"

#include "pydoc_context.h"

#define	EXTRACT_PARAMETERS(_params) \
	std::vector<LuxToken> aTokens; \
	std::vector<LuxPointer> aValues; \
	int count = getParametersFromPython(_params, aTokens, aValues);

#define PASS_PARAMETERS \
	count, aTokens.size()>0?&aTokens[0]:0, aValues.size()>0?&aValues[0]:0

#define PASS_PARAMSET \
	ParamSet( count, name, aTokens.size()>0?&aTokens[0]:0, aValues.size()>0?&aValues[0]:0 )

namespace lux {

boost::once_flag luxInitFlag = BOOST_ONCE_INIT;

//The memory pool handles temporary allocations and is freed after each C API Call
boost::pool<> memoryPool(sizeof(char));

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
				LOG( LUX_SEVERE,LUX_CONSISTENCY)<< "Passing unrecognised data type '"<<first_item_classname<<"' in tuple to Python API for '"<<tokenString<<"' token.";
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
				LOG( LUX_SEVERE,LUX_CONSISTENCY)<< "Passing unrecognised data type '"<<first_item_classname<<"' in list to Python API for '"<<tokenString<<"' token.";
			}
		}
		else
		{
			//Unrecognised parameter type : we throw an error
			LOG( LUX_SEVERE,LUX_CONSISTENCY)<< "Passing unrecognised parameter type to Python API for '"<<tokenString<<"' token.";
		}

	}
	return(n);
}

int framebuffer_getbuffer(PyObject *exporter, Py_buffer *view, int flags) {
	return 0;
}

void framebuffer_releasebuffer(PyObject *exporter, Py_buffer *view) {
}

// for debugging
//#define PYLUX_USE_BUFFERS 1

struct shared_array_noop_deleter {
	void operator()(float *) {
	}
};

struct float_buffer {
	float_buffer(Context *ctx, float *buf, size_t nelms)
		: context(ctx), buffer_ptr(buf, shared_array_noop_deleter()), buffer_nelms(nelms), owned(false) {
	}

	float_buffer(Context *ctx, const boost::shared_array<float> &buf, size_t nelms)
		: context(ctx), buffer_ptr(buf), buffer_nelms(nelms), owned(true) {
	}

	float_buffer()
		: context(NULL), buffer_ptr(NULL), buffer_nelms(0) {
	}

private:
	Context *context;
	boost::shared_array<float> buffer_ptr;
	Py_ssize_t buffer_nelms;
	bool owned;

	friend int float_buffer_getbuffer(PyObject *, Py_buffer *, int);
	friend void float_buffer_releasebuffer(PyObject *, Py_buffer *);
	friend Py_ssize_t float_buffer_len(PyObject *);
	friend PyObject* float_buffer_item(PyObject *, Py_ssize_t);
};

int float_buffer_getbuffer(PyObject *exporter, Py_buffer *view, int flags) {

	boost::python::extract<float_buffer&> b(exporter);

	if (!b.check()) {
		PyErr_SetString(PyExc_BufferError, "Invalid buffer exporter instance");
		view->obj = NULL;
		return -1;
	}

	float_buffer& buf = b();

	if (!buf.context) {
		PyErr_SetString(PyExc_BufferError, "Buffer exporter not initialized");
		view->obj = NULL;
		return -1;	
	}

	if (!buf.buffer_ptr) {
		PyErr_SetString(PyExc_BufferError, "Invalid buffer in buffer exporter");
		view->obj = NULL;
		return -1;
	}

	// based on PyBuffer_FillInfo
    if (view == NULL) 
		return 0;
	const bool readonly = true;
    if (((flags & PyBUF_WRITABLE) == PyBUF_WRITABLE) &&
        (readonly)) {
        PyErr_SetString(PyExc_BufferError,
                        "Object is not writable.");
        return -1;
    }

    view->obj = exporter;
    if (view->obj)
        Py_INCREF(view->obj);
	view->buf = buf.buffer_ptr.get();
    view->len = sizeof(float) * buf.buffer_nelms;
    view->readonly = readonly;
    view->itemsize = sizeof(float);
    view->format = NULL;
    if ((flags & PyBUF_FORMAT) == PyBUF_FORMAT)
        view->format = "f";
    view->ndim = 1;
    view->shape = NULL;
    if ((flags & PyBUF_ND) == PyBUF_ND)
        view->shape = &buf.buffer_nelms;
    view->strides = NULL;
    if ((flags & PyBUF_STRIDES) == PyBUF_STRIDES)
        view->strides = &(view->itemsize);
    view->suboffsets = NULL;
    view->internal = NULL;
    return 0;
}

void float_buffer_releasebuffer(PyObject *exporter, Py_buffer *view) {
	boost::python::extract<float_buffer&> b(exporter);

	if (!b.check()) {
		PyErr_SetString(PyExc_BufferError, "Invalid buffer exporter instance");
		return;
	}

	float_buffer& buf = b();

	buf.buffer_ptr.reset();
}

Py_ssize_t float_buffer_len(PyObject *exporter) {   
	boost::python::extract<float_buffer&> b(exporter);
	
	if (!b.check()) {
		PyErr_SetString(PyExc_BufferError, "Invalid buffer exporter instance");
		return 0;
	}
	
	float_buffer& buf = b();
	
	return buf.buffer_nelms;
}

PyObject* float_buffer_item(PyObject *exporter, Py_ssize_t idx) {
	boost::python::extract<float_buffer&> b(exporter);
	
	if (!b.check()) {
		PyErr_SetString(PyExc_BufferError, "Invalid buffer exporter instance");
		return 0;
	}
	
	float_buffer& buf = b();
	
	return boost::python::incref((boost::python::object(buf.buffer_ptr[idx])).ptr());
}

/*
 * PyContext class
 */

class PyContext
{
public:
	PyContext(std::string _name)
	{
		//System wide init
		boost::call_once(&luxInit, luxInitFlag);

		//Here we create a new context
		name = _name;
		createNewContext(_name);
		// LOG(LUX_INFO,LUX_NOERROR)<<"Created new context : '"<<name<<"'";
	}

	~PyContext()
	{
		//destroy threads
		BOOST_FOREACH(boost::thread *t,pyLuxWorldEndThreads)
		{
			delete(t);
		}
		pyLuxWorldEndThreads.clear();

		// Close any active net connections
		boost::python::list server_list( getRenderingServersStatus() );
		for(boost::python::ssize_t n=0; n<boost::python::len(server_list); n++)
		{
			RenderingServerInfo RSI = boost::python::extract<RenderingServerInfo>(server_list[n]);
			context->RemoveServer( RSI );
		}

		// free the context
		delete context;
		context = NULL;
	}

	/**
	 * Return a useful string to represent Context objects:
	 *  <pylux.Context name>
	 */
	boost::python::str repr()
	{
		std::stringstream o(std::ios_base::out);
		o << "<pylux.Context " << name << ">";
		return boost::python::str(o.str().c_str());
	}

	bool parse(const char *filename, bool async)
	{
		checkActiveContext();
		if (async)
		{
			pyLuxWorldEndThreads.push_back(new boost::thread( boost::bind(luxParse, filename) ));
			return true;	// Real parse status can be checked later with parseSuccess()
		}
		else
		{
			return luxParse(filename);
		}
	}

	bool parsePartial(const char *filename, bool async)
	{
		checkActiveContext();
		if (async)
		{
			pyLuxWorldEndThreads.push_back(new boost::thread( boost::bind(luxParsePartial, filename) ));
			return true;	// Real parse status can be checked later with parseSuccess()
		}
		else
		{
			return luxParsePartial(filename);
		}
	}

	bool parseSuccessful()
	{
		checkActiveContext();
		return context->currentApiState != STATE_PARSE_FAIL;
	}

	void cleanup()
	{
		checkActiveContext();
		context->Cleanup();

		// Ensure the context memory is freed. Any pylux calls
		// after this will create a new Context with the same name.
		delete context;
		context = NULL;
	}

	void identity()
	{
		checkActiveContext();
		context->Identity();
	}

	void translate(float dx, float dy, float dz)
	{
		checkActiveContext();
		context->Translate(dx,dy,dz);
	}

	void rotate(float angle, float ax, float ay, float az)
	{
		checkActiveContext();
		context->Rotate(angle,ax,ay,az);
	}

	void scale(float sx, float sy, float sz)
	{
		checkActiveContext();
		context->Scale(sx,sy,sz);
	}

	void lookAt(float ex, float ey, float ez, float lx, float ly, float lz, float ux, float uy, float uz)
	{
		checkActiveContext();
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

		checkActiveContext();
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

		checkActiveContext();
		context->Transform(pFloat);
		memoryPool.purge_memory();
	}

	void coordinateSystem(const char *name)
	{
		checkActiveContext();
		context->CoordinateSystem(std::string(name));
	}

	void coordSysTransform(const char *name)
	{
		checkActiveContext();
		context->CoordSysTransform(std::string(name));
	}
	
	void renderer(const char *name, boost::python::list params)
	{
		EXTRACT_PARAMETERS(params);
		checkActiveContext();
		context->Renderer(name,PASS_PARAMSET);
		memoryPool.purge_memory();
	}

	void pixelFilter(const char *name, boost::python::list params)
	{
		EXTRACT_PARAMETERS(params);
		checkActiveContext();
		context->PixelFilter(name,PASS_PARAMSET);
		memoryPool.purge_memory();
	}

	void film(const char *name, boost::python::list params)
	{
		EXTRACT_PARAMETERS(params);
		checkActiveContext();
		context->Film(name, PASS_PARAMSET);
		memoryPool.purge_memory();
	}

	void sampler(const char *name, boost::python::list params)
	{
		EXTRACT_PARAMETERS(params);
		checkActiveContext();
		context->Sampler(name, PASS_PARAMSET);
		memoryPool.purge_memory();
	}

	void accelerator(const char *name, boost::python::list params)
	{
		EXTRACT_PARAMETERS(params);
		checkActiveContext();
		context->Accelerator(name, PASS_PARAMSET);
		memoryPool.purge_memory();
	}

	void surfaceIntegrator(const char *name, boost::python::list params)
	{
		EXTRACT_PARAMETERS(params);
		checkActiveContext();
		context->SurfaceIntegrator(name, PASS_PARAMSET);
		memoryPool.purge_memory();
	}

	void volumeIntegrator(const char *name, boost::python::list params)
	{
		EXTRACT_PARAMETERS(params);
		checkActiveContext();
		context->VolumeIntegrator(name,PASS_PARAMSET);
		memoryPool.purge_memory();
	}

	void camera(const char *name, boost::python::list params)
	{
		EXTRACT_PARAMETERS(params);
		checkActiveContext();
		context->Camera(name, PASS_PARAMSET);
		memoryPool.purge_memory();
	}

	void worldBegin()
	{
		checkActiveContext();
		context->WorldBegin();
	}

	void attributeBegin()
	{
		checkActiveContext();
		context->AttributeBegin();
	}

	void attributeEnd()
	{
		checkActiveContext();
		context->AttributeEnd();
	}

	void transformBegin()
	{
		checkActiveContext();
		context->TransformBegin();
	}

	void transformEnd()
	{
		checkActiveContext();
		context->TransformEnd();
	}

	void motionBegin(boost::python::list tx)
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

		checkActiveContext();
		context->MotionBegin(static_cast<u_int>(listSize), pFloat);
		memoryPool.purge_memory();
	}

	void motionEnd()
	{
		checkActiveContext();
		context->MotionEnd();
	}

	void texture(const char *name, const char *type, const char *texname, boost::python::list params)
	{
		EXTRACT_PARAMETERS(params);
		checkActiveContext();
		context->Texture(name, type, texname, PASS_PARAMSET);
		memoryPool.purge_memory();
	}

	void material(const char *name, boost::python::list params)
	{
		EXTRACT_PARAMETERS(params);
		checkActiveContext();
		context->Material(name, PASS_PARAMSET);
		memoryPool.purge_memory();
	}

	void makeNamedMaterial(const char *name, boost::python::list params)
	{
		EXTRACT_PARAMETERS(params);
		checkActiveContext();
		context->MakeNamedMaterial(name, PASS_PARAMSET);
		memoryPool.purge_memory();
	}

	void namedMaterial(const char *name)
	{
		checkActiveContext();
		context->NamedMaterial(name);
	}

	void lightGroup(const char *name, boost::python::list params)
	{
		EXTRACT_PARAMETERS(params);
		checkActiveContext();
		context->LightGroup(name, PASS_PARAMSET);
		memoryPool.purge_memory();
	}

	void lightSource(const char *name, boost::python::list params)
	{
		EXTRACT_PARAMETERS(params);
		checkActiveContext();
		context->LightSource(name, PASS_PARAMSET);
		memoryPool.purge_memory();
	}

	void areaLightSource(const char *name, boost::python::list params)
	{
		EXTRACT_PARAMETERS(params);
		checkActiveContext();
		context->AreaLightSource(name, PASS_PARAMSET);
		memoryPool.purge_memory();
	}

	void portalShape(const char *name, boost::python::list params)
	{
		EXTRACT_PARAMETERS(params);
		checkActiveContext();
		context->PortalShape(name, PASS_PARAMSET);
		memoryPool.purge_memory();
	}

	void shape(const char *name, boost::python::list params)
	{
		EXTRACT_PARAMETERS(params);
		checkActiveContext();
		context->Shape(name, PASS_PARAMSET);
		memoryPool.purge_memory();
	}

	void reverseOrientation()
	{
		checkActiveContext();
		context->ReverseOrientation();
	}

	void makeNamedVolume(const char *id, const char *name, boost::python::list params)
	{
		EXTRACT_PARAMETERS(params);
		checkActiveContext();
		context->MakeNamedVolume(id, name, PASS_PARAMSET);
		memoryPool.purge_memory();
	}

	void volume(const char *name, boost::python::list params)
	{
		EXTRACT_PARAMETERS(params);
		checkActiveContext();
		context->Volume(name, PASS_PARAMSET);
		memoryPool.purge_memory();
	}

	void exterior(const char *name)
	{
		checkActiveContext();
		context->Exterior(name);
	}

	void interior(const char *name)
	{
		checkActiveContext();
		context->Interior(name);
	}

	void objectBegin(const char *name)
	{
		checkActiveContext();
		context->ObjectBegin(std::string(name));
	}

	void objectEnd()
	{
		checkActiveContext();
		context->ObjectEnd();
	}

	void objectInstance(const char *name)
	{
		checkActiveContext();
		context->ObjectInstance(std::string(name));
	}

	void portalInstance(const char *name)
	{
		checkActiveContext();
		context->PortalInstance(std::string(name));
	}

	void motionInstance(const char *name, float startTime, float endTime, const char *toTransform)
	{
		checkActiveContext();
		context->MotionInstance(std::string(name), startTime, endTime, std::string(toTransform));
	}

	void worldEnd() //launch luxWorldEnd() into a thread
	{
		checkActiveContext();
		pyLuxWorldEndThreads.push_back(new boost::thread( boost::bind(&PyContext::pyWorldEnd, this) ));
	}

	void loadFLM(const char* name)
	{
		checkActiveContext();
		context->LoadFLM(std::string(name));
	}

	void saveFLM(const char* name)
	{
		checkActiveContext();
		context->SaveFLM(std::string(name));
	}

	void saveEXR(const char *filename, bool useHalfFloat, bool includeZBuffer, bool tonemapped)
	{
		checkActiveContext();
		context->SaveEXR(filename, useHalfFloat, includeZBuffer, 2 /*ZIP_COMPRESSION*/, tonemapped);
	}

	void overrideResumeFLM(const char *name)
	{
		checkActiveContext();
		context->OverrideResumeFLM(string(name));
	}

	void start()
	{
		checkActiveContext();
		context->Resume();
	}

	void pause() 
	{
		checkActiveContext();
		context->Pause();
	}

	void exit()
	{
		checkActiveContext();
		context->Exit();
	}

	void wait()
	{
		checkActiveContext();
		context->Wait();
	}

	void setHaltSamplesPerPixel(int haltspp, bool haveEnoughSamplesPerPixel, bool suspendThreadsWhenDone)
	{
		checkActiveContext();
		context->SetHaltSamplesPerPixel(haltspp, haveEnoughSamplesPerPixel, suspendThreadsWhenDone);
	}

	unsigned int addThread()
	{
		checkActiveContext();
		return context->AddThread();
	}

	void removeThread()
	{
		checkActiveContext();
		context->RemoveThread();
	}

	void setEpsilon(const float minValue, const float maxValue)
	{
		checkActiveContext();
		context->SetEpsilon(minValue < 0.f ? DEFAULT_EPSILON_MIN : minValue, maxValue < 0.f ? DEFAULT_EPSILON_MAX : maxValue);
	}

	void updateFramebuffer()
	{
		checkActiveContext();
		context->UpdateFramebuffer();
	}

	boost::python::list framebuffer()
	{
		boost::python::list pyFrameBuffer;
		checkActiveContext();
		int nvalues=(luxGetIntAttribute("film", "xResolution")) * (luxGetIntAttribute("film", "yResolution")) * 3; //get the number of values to copy

		unsigned char* framebuffer = context->Framebuffer();
		//copy the values
		for(int i=0;i<nvalues;i++)
			pyFrameBuffer.append(framebuffer[i]);
		return pyFrameBuffer;
	}

	boost::python::list floatFramebuffer()
	{
		boost::python::list pyFrameBuffer;
		checkActiveContext();
		int nvalues=(luxGetIntAttribute("film", "xResolution")) * (luxGetIntAttribute("film", "yResolution")) * 3; //get the number of values to copy

		float* framebuffer = context->FloatFramebuffer();
		//copy the values
		for(int i=0;i<nvalues;i++)
			pyFrameBuffer.append(framebuffer[i]);
		return pyFrameBuffer;
	}

	boost::python::list alphaBuffer()
	{
		boost::python::list pyFrameBuffer;
		checkActiveContext();
		int nvalues=(luxGetIntAttribute("film", "xResolution")) * (luxGetIntAttribute("film", "yResolution"));

		float* framebuffer = context->AlphaBuffer();
		//copy the values
		for(int i=0;i<nvalues;i++)
			pyFrameBuffer.append(framebuffer[i]);
		return pyFrameBuffer;
	}

	boost::python::object zBuffer()
	{
		checkActiveContext();

		const int xres = luxGetIntAttribute("film", "xResolution");
		const int yres = luxGetIntAttribute("film", "yResolution");
		const int nelms = xres * yres;
		float* zbuffer = context->ZBuffer();

#ifdef PYLUX_USE_BUFFERS
		// for debugging buffer support

		float_buffer fb = float_buffer(context, zbuffer, nelms);

		boost::python::object zbuf(fb);

		if (PyObject_CheckBuffer(zbuf.ptr()) == 0) {
			PyErr_BadArgument();
			return boost::python::object();
		}

		return zbuf;
#else
		boost::python::list pyFrameBuffer;
		//copy the values
		for(int i=0;i<nelms;i++)
			pyFrameBuffer.append(zbuffer[i]);
		return pyFrameBuffer;
#endif		
	}

	boost::python::tuple blenderCombinedDepthBuffers()
	{
		checkActiveContext();
		updateFramebuffer();
		bool preMult = luxGetBoolAttribute("film", "premultiplyAlpha");

		const int xres = luxGetIntAttribute("film", "xResolution");
		const int yres = luxGetIntAttribute("film", "yResolution");
		const int nelms = xres * yres;

		const float *color = context->FloatFramebuffer();
		const float *alpha = context->AlphaBuffer();
		const float *z = context->ZBuffer();

		boost::shared_array<float> pb(new float[4*nelms]);
		float* pixel = pb.get();

		boost::shared_array<float> zb(new float[nelms]);
		float* zbuffer = zb.get();

		for(int y=yres-1; y>-1;y--)
		{
			for(int x=0; x<xres; x++)
			{
				const int i = (y*xres + x);
				const int j = i*3;
				*(pixel++) = color[j+0];
				*(pixel++) = color[j+1];
				*(pixel++) = color[j+2];

				*(pixel++) = preMult == true ? alpha[i] : 1.0;
				*(zbuffer++) = z[i];
			}
		}

		float_buffer pfb = float_buffer(context, pb, 4*nelms);
		boost::python::object pbuf(pfb);

		float_buffer zfb = float_buffer(context, zb, nelms);
		boost::python::object zbuf(zfb);

		if ((!PyObject_CheckBuffer(zbuf.ptr())) || (!PyObject_CheckBuffer(pbuf.ptr()))) {
			PyErr_BadArgument();
			return boost::python::make_tuple(boost::python::object(), boost::python::object());
		}

		return boost::python::make_tuple( pbuf, zbuf );
	}

	/**
	 * Format framebuffers into a bottom-up format required
	 * by Blender 2.66's RenderLayer type
	 * Blender expects now always premultiplied alpha
	 * for showing sky/bg, alpha 1.0 is needed then
	 * We reuse lux premultiplyAlpha flag for condition
	 */
	boost::python::tuple blenderCombinedDepthRects()
	{
		updateFramebuffer();
		boost::python::list combined;
		boost::python::list depth;
		bool preMult = luxGetBoolAttribute("film", "premultiplyAlpha");

		checkActiveContext();
		int xres = luxGetIntAttribute("film", "xResolution");
		int yres = luxGetIntAttribute("film", "yResolution");

		float *color = context->FloatFramebuffer();
		float *alpha = context->AlphaBuffer();
		float *z = context->ZBuffer();
		for(int y=yres-1; y>-1;y--)
		{
			for(int x=0; x<xres; x++)
			{
				int i = (y*xres + x);
				int j = i*3;
				boost::python::list rect_item;
				rect_item.append( color[j] );
				rect_item.append( color[j+1] );
				rect_item.append( color[j+2] );
				rect_item.append( preMult == true ? alpha[i] : 1.0 );
				combined.append( rect_item );
				boost::python::list depth_item;
				depth_item.append( z[i] );
				depth.append( depth_item );
			}
		}
		return boost::python::make_tuple( combined, depth );
	}

	boost::python::list getHistogramImage(unsigned int width, unsigned int height, int options)
	{
		boost::python::list pyHistogramImage;
		int nvalues=width*height;
		unsigned char* outPixels = new unsigned char[nvalues];

		checkActiveContext();
		context->GetHistogramImage(outPixels, width, height, options);

		for(int i=0;i<nvalues;i++)
			pyHistogramImage.append(outPixels[i]);
		delete[] outPixels;
		return pyHistogramImage;
	}

	void setParameterValue(luxComponent comp, luxComponentParameters param, double value, unsigned int index)
	{
		checkActiveContext();
		return context->SetParameterValue(comp, param, value, index);
	}

	double getParameterValue(luxComponent comp, luxComponentParameters param, unsigned int index)
	{
		checkActiveContext();
		return context->GetParameterValue(comp, param, index);
	}

	double getDefaultParameterValue(luxComponent comp, luxComponentParameters param, unsigned int index)
	{
		checkActiveContext();
		return context->GetDefaultParameterValue(comp, param, index);
	}

	void setStringParameterValue(luxComponent comp, luxComponentParameters param, const char* value, unsigned int index)
	{
		checkActiveContext();
		return context->SetStringParameterValue(comp, param, value, index);
	}

	unsigned int getStringParameterValue(luxComponent comp, luxComponentParameters param, char* dst, unsigned int dstlen, unsigned int index)
	{
		checkActiveContext();
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
		checkActiveContext();
		const string str = context->GetDefaultStringParameterValue(comp, param, index);
		unsigned int nToCopy = str.length() < dstlen ?
			str.length() + 1 : dstlen;
		if (nToCopy > 0) {
			strncpy(dst, str.c_str(), nToCopy - 1);
			dst[nToCopy - 1] = '\0';
		}
		return str.length();
	}

	const char* getAttributes()
	{
		checkActiveContext();
		return luxGetAttributes();
	}

	//Queryable objects
	//Here I do a special handling for python :
	//Python is dynamically typed, unlike C++ which is statically typed.
	//Python variables may hold an integer, a float, list, dict, tuple, str, long etc., among other things.
	//So we don't need a getINT, getFLOAT, getXXX in the python api
	//This function handles all types
	boost::python::object getAttribute(const char *objectName, const char *attributeName)
	{
		checkActiveContext();
		if (!luxHasObject(objectName) ||
			!luxHasAttribute(objectName, attributeName))
			return boost::python::object();
		switch (luxGetAttributeType(objectName, attributeName)) {
			case LUX_ATTRIBUTETYPE_BOOL:
				return boost::python::object(luxGetBoolAttribute(objectName, attributeName));
			case LUX_ATTRIBUTETYPE_INT:
				return boost::python::object(luxGetIntAttribute(objectName, attributeName));
			case LUX_ATTRIBUTETYPE_FLOAT:
				return boost::python::object(luxGetFloatAttribute(objectName, attributeName));
			case LUX_ATTRIBUTETYPE_DOUBLE:
				return boost::python::object(luxGetDoubleAttribute(objectName, attributeName));
			case LUX_ATTRIBUTETYPE_STRING: {
				std::vector<char> buf(1 << 16, '\0');
				if (luxGetStringAttribute(objectName, attributeName, &buf[0], static_cast<unsigned int>(buf.size())) > 0)
					return boost::python::object(std::string(&buf[0]));
				return boost::python::object();
			}
			case LUX_ATTRIBUTETYPE_NONE:
				break;
			default:
				LOG(LUX_ERROR,LUX_BUG)<<"Unknown attribute type in pyLuxGetOption";
		}
		return boost::python::object();
	}

	void setAttribute(const char * objectName, const char * attributeName, boost::python::object value)
	{
		//void luxSetAttribute(const char * objectName, const char * attributeName, int n, void *values); /* Sets an option value */
		checkActiveContext();
		if (!luxHasObject(objectName)) {
			LOG(LUX_ERROR,LUX_BADTOKEN)<<"Unknown object '"<<objectName<<"'";
			return;
		}
		if (!luxHasAttribute(objectName, attributeName)) {
			LOG(LUX_ERROR,LUX_BADTOKEN)<<"Unknown attribute '"<<attributeName<<"' in object '"<<objectName<<"'";
			return;
		}
		switch(luxGetAttributeType(objectName, attributeName))
		{
			case LUX_ATTRIBUTETYPE_BOOL:
				luxSetBoolAttribute(objectName, attributeName, boost::python::extract<bool>(value));
				break;

			case LUX_ATTRIBUTETYPE_INT:
				luxSetIntAttribute(objectName, attributeName, boost::python::extract<int>(value));
				break;

			case LUX_ATTRIBUTETYPE_FLOAT:
				luxSetFloatAttribute(objectName, attributeName, boost::python::extract<float>(value));
				break;

			case LUX_ATTRIBUTETYPE_DOUBLE:
				luxSetDoubleAttribute(objectName, attributeName, boost::python::extract<double>(value));
				break;

			case LUX_ATTRIBUTETYPE_STRING:
				luxSetStringAttribute(objectName, attributeName, std::string(boost::python::extract<std::string>(value)).c_str());
				break;

			case LUX_ATTRIBUTETYPE_NONE:
			default:
				LOG(LUX_ERROR,LUX_BUG)<<"Unknown attribute type for '"<<attributeName<<"' in object '"<<objectName<<"'";
		}
	}

	void addServer(const char * name)
	{
		checkActiveContext();
		context->AddServer(std::string(name));
	}

	void removeServer(const char * name)
	{
		checkActiveContext();
		context->RemoveServer(std::string(name));
	}

	void resetServer(const char * name, const char * password)
	{
		checkActiveContext();
		context->ResetServer(std::string(name), std::string(password));
	}

	unsigned int getServerCount()
	{
		checkActiveContext();
		return luxGetIntAttribute("render_farm", "slaveNodeCount");
	}

	void updateFilmFromNetwork()
	{
		checkActiveContext();
		context->UpdateFilmFromNetwork();
	}

	void setNetworkServerUpdateInterval(int updateInterval)
	{
		checkActiveContext();
		luxSetIntAttribute("render_farm", "pollingInterval", updateInterval);
	}

	int getNetworkServerUpdateInterval()
	{
		checkActiveContext();
		return luxGetIntAttribute("render_farm", "pollingInterval");
	}

	boost::python::tuple getRenderingServersStatus()
	{
		checkActiveContext();
		int nServers = luxGetIntAttribute("render_farm", "slaveNodeCount");

		RenderingServerInfo *pInfoList = new RenderingServerInfo[nServers];
		nServers = context->GetRenderingServersStatus( pInfoList, nServers );
		
		boost::python::list server_list;

		for( int n = 0; n < nServers; n++ ) {
			server_list.append( pInfoList[n] );
		}

		delete[] pInfoList;
		
		return boost::python::tuple( server_list );
	}

	double statistics(const char *statName)
	{
		checkActiveContext();
		return context->Statistics(statName);
	}

	// Deprecated
	const char* printableStatistics(const bool add_total)
	{
		checkActiveContext();
		return luxPrintableStatistics(add_total);
	}

	void updateStatisticsWindow()
	{
		checkActiveContext();
		context->UpdateStatisticsWindow();
	}

	void enableDebugMode()
	{
		checkActiveContext();
		context->EnableDebugMode();
	}

	void disableRandomMode()
	{
		checkActiveContext();
		context->DisableRandomMode();
	}

	std::string name;

private:
	Context *context;

	std::vector<boost::thread *> pyLuxWorldEndThreads; //hold pointers to the worldend threads
	void pyWorldEnd()
	{
		checkActiveContext();
		context->WorldEnd();
	}

	void checkActiveContext()
	{
		if (context == NULL)
		{
			createNewContext();
		}
		Context::SetActive(context);
	}

	void createNewContext(std::string _name = "PyLux context")
	{
		context = new Context(name);
		Context::SetActive(context);
		context->Init();
	}
};

} //namespace lux

static bool is_buffer(PyObject *obj) {
	return PyObject_CheckBuffer(obj);
}

static bool is_sequence(PyObject *obj) {
	return PySequence_Check(obj);
}

void export_float_buffer()
{
	using namespace boost::python;
	using namespace lux;

	// for debugging
	def("is_buffer", &is_buffer, args("obj"));
	def("is_sequence", &is_sequence, args("obj"));

	// register float_buffer
	class_<float_buffer>("float_buffer");	

	// update float_buffer type so it implements
	// the buffer and sequence protocols
	const converter::registration& fb_reg(converter::registry::lookup(type_id<float_buffer>()));
	PyTypeObject* fb_type = fb_reg.get_class_object();

	// buffer protocol
	static PyBufferProcs float_buffer_as_buffer = {
		(getbufferproc)float_buffer_getbuffer,
		(releasebufferproc)float_buffer_releasebuffer,
	};
	fb_type->tp_as_buffer = &float_buffer_as_buffer;

	// partial sequence protocol support
	static PySequenceMethods float_buffer_as_sequence = {
		float_buffer_len,       /* sq_length */   // needed for foreach_set
		0,						/* sq_concat */
		0,						/* sq_repeat */
		float_buffer_item       /* sq_item */     // needed for PySequence_Check to succeed
	};
	fb_type->tp_as_sequence = &float_buffer_as_sequence;
}

// Add PyContext class to pylux module definition
void export_PyContext()
{
	using namespace boost::python;
	using namespace lux;

	export_float_buffer();

	class_<PyContext>(
		"Context",
		ds_pylux_Context,
		init<std::string>(args("Context", "name"), ds_pylux_Context_init)
		)
		.def_readonly("name",
			&PyContext::name
		)
		.def("__repr__",
			&PyContext::repr,
			args("Context")
		)
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
		.def("blenderCombinedDepthBuffers",
			&PyContext::blenderCombinedDepthBuffers,
			args("Context"),
			"Blender framebuffer fetcher method; returns combined Color+Alpha and Depth buffers in bottom-up format as Python buffers"
		)
		.def("blenderCombinedDepthRects",
			&PyContext::blenderCombinedDepthRects,
			args("Context"),
			"Blender framebuffer fetcher method; returns combined Color+Alpha and Depth buffers in bottom-up format"
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
		.def("floatFramebuffer",
			&PyContext::floatFramebuffer,
			args("Context"),
			ds_pylux_Context_floatframebuffer
		)
		.def("alphaBuffer",
			&PyContext::alphaBuffer,
			args("Context"),
			ds_pylux_Context_alphabuffer
		)
		.def("zBuffer",
			&PyContext::zBuffer,
			args("Context"),
			ds_pylux_Context_zbuffer
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
		.def("getAttribute",
			&PyContext::getAttribute,
			args("Context"),
			ds_pylux_Context_getAttribute
		)
		.def("getAttributes",
			&PyContext::getAttributes,
			args("Context"),
			ds_pylux_Context_getAttributes
		)
		.def("getParameterValue",
			&PyContext::getParameterValue,
			args("Context", "component", "parameter", "index"),
			ds_pylux_Context_getParameterValue
		)
		.def("getRenderingServersStatus",
			&PyContext::getRenderingServersStatus,
			args("Context"),
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
		.def("motionBegin",
			&PyContext::motionBegin,
			args("Context"),
			ds_pylux_Context_motionBegin
		)
		.def("motionEnd",
			&PyContext::motionEnd,
			args("Context"),
			ds_pylux_Context_motionEnd
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
		.def("parsePartial",
			&PyContext::parsePartial,
			args("Context", "filename", "asynchronous"),
			ds_pylux_Context_parsePartial
		)
		.def("parseSuccessful",
			&PyContext::parseSuccessful,
			args("Context")
			)
		.def("pause",
			&PyContext::pause,
			args("Context"),
			ds_pylux_Context_pause
		)
		.def("renderer",
			&PyContext::renderer,
			args("Context", "type", "ParamSet"),
			ds_pylux_Context_renderer
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
		.def("portalInstance",
			&PyContext::portalInstance,
			args("Context", "name"),
			ds_pylux_Context_portalInstance
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
		.def("resetServer",
			&PyContext::resetServer,
			args("Context", "address", "password"),
			ds_pylux_Context_resetServer
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
			args("Context", "type", "ParamSet"),
			ds_pylux_Context_sampler
		)
		.def("saveFLM",
			&PyContext::saveFLM,
			args("Context", "filename"),
			ds_pylux_Context_saveFLM
		)
		.def("saveEXR",
			&PyContext::saveEXR,
			args("Context", "filename", "useHalfFloat", "includeZBuffer", "tonemapped"),
			ds_pylux_Context_saveEXR
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
		.def("setHaltSamplesPerPixel",
			&PyContext::setHaltSamplesPerPixel,
			args("Context"),
			ds_pylux_Context_setHaltSamplesPerPixel
		)
		.def("setNetworkServerUpdateInterval",
			&PyContext::setNetworkServerUpdateInterval,
			args("Context"),
			ds_pylux_Context_setNetworkServerUpdateInterval
		)
		.def("setAttribute",
			&PyContext::setAttribute,
			args("Context"),
			ds_pylux_Context_setAttribute
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
		.def("printableStatistics",
			&PyContext::printableStatistics,
			args("Context", "add_total"),
			ds_pylux_Context_printable_statistics
		)
		.def("updateStatisticsWindow",
			&PyContext::updateStatisticsWindow,
			args("Context"),
			ds_pylux_Context_update_statistics_window
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

#endif	// LUX_PYCONTEXT_H
