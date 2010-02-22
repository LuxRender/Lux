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
		boost::python::tuple l=boost::python::extract<boost::python::tuple>(pList[i]);
		std::string tokenString=boost::python::extract<std::string>(l[0]);
		char *tok=(char *)memoryPool.ordered_malloc(sizeof(char)*tokenString.length()+1);
		strcpy(tok,tokenString.c_str());
		aTokens.push_back(tok);
		//std::cout<<"We have a nice parameter : ["<<tokenString<<']'<<std::endl;

		boost::python::extract<int> intExtractor(l[1]);
		boost::python::extract<float> floatExtractor(l[1]);
		boost::python::extract<boost::python::tuple> tupleExtractor(l[1]);
		boost::python::extract<std::string> stringExtractor(l[1]);

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
			//std::cout<<"this is a TUPLE - WARNING ASSUMING FLOATS :";
			boost::python::tuple t=tupleExtractor();
			boost::python::ssize_t tupleSize=boost::python::len(t);
			float *pFloat=(float *)memoryPool.ordered_malloc(sizeof(float)*tupleSize);

			for(boost::python::ssize_t j=0;j<tupleSize;j++)
			{
				boost::python::extract<float> tupleFloatExtractor(t[j]);
				//jromang - Assuming floats here, but do we only have floats in tuples ?
				BOOST_ASSERT(tupleFloatExtractor.check());
				pFloat[j]=tupleFloatExtractor();
				//std::cout<<pFloat[j]<<';';
			}
			//std::cout<<std::endl;

			aValues.push_back((LuxPointer)pFloat);
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

	int parse(const char *filename)
	{
		//TODO jromang - add thread lock here (we can only parse in one context)
		Context::SetActive(context);
		return luxParse(filename);
	}

	void cleanup() { context->Cleanup(); }
	void identity() { context->Identity(); }
	void translate(float dx, float dy, float dz) { context->Translate(dx,dy,dz); }
	void rotate(float angle, float ax, float ay, float az) { context->Rotate(angle,ax,ay,az); }
	void scale(float sx, float sy, float sz) { context->Scale(sx,sy,sz); }
	void lookAt(float ex, float ey, float ez, float lx, float ly, float lz, float ux, float uy, float uz) { context->LookAt(ex, ey, ez, lx, ly, lz, ux, uy, uz); }
	void concatTransform(float transform[16]) { context->ConcatTransform(transform); }
	void transform(float transform[16]) { context->Transform(transform); }
	void coordinateSystem(const char *name) { context->CoordinateSystem(std::string(name)); }
	void coordSysTransform(const char *name) { context->CoordSysTransform(std::string(name)); }

	void pixelFilter(const char *name, boost::python::list params)
	{
		EXTRACT_PARAMETERS(params);
		context->PixelFilter(name,PASS_PARAMSET);
		memoryPool.purge_memory();
	}

	void film(const char *name, boost::python::list params)
	{
		EXTRACT_PARAMETERS(params);
		context->Film(name, PASS_PARAMSET);
		memoryPool.purge_memory();
	}

	void sampler(const char *name, boost::python::list params)
	{
		EXTRACT_PARAMETERS(params);
		context->Sampler(name, PASS_PARAMSET);
		memoryPool.purge_memory();
	}

	void accelerator(const char *name, boost::python::list params)
	{
		EXTRACT_PARAMETERS(params);
		context->Accelerator(name, PASS_PARAMSET);
		memoryPool.purge_memory();
	}

	void surfaceIntegrator(const char *name, boost::python::list params)
	{
		EXTRACT_PARAMETERS(params);
		context->SurfaceIntegrator(name, PASS_PARAMSET);
		memoryPool.purge_memory();
	}

	void volumeIntegrator(const char *name, boost::python::list params)
	{
		EXTRACT_PARAMETERS(params);
		context->VolumeIntegrator(name,PASS_PARAMSET);
		memoryPool.purge_memory();
	}

	void camera(const char *name, boost::python::list params)
	{
		EXTRACT_PARAMETERS(params);
		context->Camera(name, PASS_PARAMSET);
		memoryPool.purge_memory();
	}

	void worldBegin() { context->WorldBegin(); }
	void attributeBegin() { context->AttributeBegin(); }
	void attributeEnd() { context->AttributeEnd(); }
	void transformBegin() { context->TransformBegin(); }
	void transformEnd() { context->TransformEnd(); }

	void texture(const char *name, const char *type, const char *texname, boost::python::list params)
	{
		EXTRACT_PARAMETERS(params);
		context->Texture(name, type, texname, PASS_PARAMSET);
		memoryPool.purge_memory();
	}

	void material(const char *name, boost::python::list params)
	{
		EXTRACT_PARAMETERS(params);
		context->Material(name, PASS_PARAMSET);
		memoryPool.purge_memory();
	}

	void makeNamedMaterial(const char *name, boost::python::list params)
	{
		EXTRACT_PARAMETERS(params);
		context->MakeNamedMaterial(name, PASS_PARAMSET);
		memoryPool.purge_memory();
	}

	void namedMaterial(const char *name) { context->NamedMaterial(name); }

	void lightSource(const char *name, boost::python::list params)
	{
		EXTRACT_PARAMETERS(params);
		context->LightSource(name, PASS_PARAMSET);
		memoryPool.purge_memory();
	}

	void areaLightSource(const char *name, boost::python::list params)
	{
		EXTRACT_PARAMETERS(params);
		context->AreaLightSource(name, PASS_PARAMSET);
		memoryPool.purge_memory();
	}

	void portalShape(const char *name, boost::python::list params)
	{
		EXTRACT_PARAMETERS(params);
		context->PortalShape(name, PASS_PARAMSET);
		memoryPool.purge_memory();
	}

	void shape(const char *name, boost::python::list params)
	{
		EXTRACT_PARAMETERS(params);
		context->Shape(name, PASS_PARAMSET);
		memoryPool.purge_memory();
	}

	void reverseOrientation() { context->ReverseOrientation(); }

	void volume(const char *name, boost::python::list params)
	{
		EXTRACT_PARAMETERS(params);
		context->Volume(name, PASS_PARAMSET);
		memoryPool.purge_memory();
	}

	void exterior(const char *name, boost::python::list params)
	{
		EXTRACT_PARAMETERS(params);
		context->Exterior(name, PASS_PARAMSET);
		memoryPool.purge_memory();
	}

	void interior(const char *name, boost::python::list params)
	{
		EXTRACT_PARAMETERS(params);
		context->Interior(name, PASS_PARAMSET);
		memoryPool.purge_memory();
	}

	void objectBegin(const char *name) { context->ObjectBegin(std::string(name)); }
	void objectEnd() { context->ObjectEnd(); }
	void objectInstance(const char *name) { context->ObjectInstance(std::string(name)); }

	void motionInstance(const char *name, float startTime, float endTime, const char *toTransform)
	{
		context->MotionInstance(std::string(name), startTime, endTime, std::string(toTransform));
	}

	void worldEnd() //launch luxWorldEnd() into a thread
	{
		pyLuxWorldEndThreads.push_back(new boost::thread( boost::bind(&PyContext::pyWorldEnd,this) ));
	}

	void loadFLM(const char* name) { context->LoadFLM(std::string(name)); }
	void saveFLM(const char* name) { context->SaveFLM(std::string(name)); }
	void overrideResumeFLM(const char *name) { context->OverrideResumeFLM(string(name)); }
	void start() { context->Start(); }
	void pause() { context->Pause(); }
	void exit() { context->Exit(); }
	void wait() { context->Wait(); }

	void setHaltSamplePerPixel(int haltspp, bool haveEnoughSamplePerPixel, bool suspendThreadsWhenDone)
	{
		context->SetHaltSamplePerPixel(haltspp, haveEnoughSamplePerPixel, suspendThreadsWhenDone);
	}

	unsigned int addThread() { return context->AddThread(); }
	void removeThread() { context->RemoveThread(); }

	void setEpsilon(const float minValue, const float maxValue)
	{
		context->SetEpsilon(minValue < 0.f ? DEFAULT_EPSILON_MIN : minValue, maxValue < 0.f ? DEFAULT_EPSILON_MAX : maxValue);
	}

	unsigned int getRenderingThreadsStatus(RenderingThreadInfo *info, unsigned int maxInfoCount)
	{
		return context->GetRenderingThreadsStatus(info, maxInfoCount);
	}

	void updateFramebuffer() { context->UpdateFramebuffer(); }

	boost::python::list framebuffer()
	{
		boost::python::list pyFrameBuffer;
		int nvalues=((int)luxStatistics("filmXres")) * ((int)luxStatistics("filmYres")) * 3; //get the number of values to copy
		unsigned char* framebuffer=luxFramebuffer(); //get the framebuffer
		//copy the values
		for(int i=0;i<nvalues;i++)
			pyFrameBuffer.append(framebuffer[i]);
		return pyFrameBuffer;
	}

	void getHistogramImage(unsigned char *outPixels, unsigned int width, unsigned int height, int options)
	{
		context->GetHistogramImage(outPixels, width, height, options);
	}

	void setParameterValue(luxComponent comp, luxComponentParameters param, double value, unsigned int index)
	{
		return context->SetParameterValue(comp, param, value, index);
	}

	double getParameterValue(luxComponent comp, luxComponentParameters param, unsigned int index)
	{
		return context->GetParameterValue(comp, param, index);
	}

	double getDefaultParameterValue(luxComponent comp, luxComponentParameters param, unsigned int index)
	{
		return context->GetDefaultParameterValue(comp, param, index);
	}

	void setStringParameterValue(luxComponent comp, luxComponentParameters param, const char* value, unsigned int index)
	{
		return context->SetStringParameterValue(comp, param, value, index);
	}

	unsigned int getStringParameterValue(luxComponent comp, luxComponentParameters param, char* dst, unsigned int dstlen, unsigned int index)
	{
		const string str = context->GetStringParameterValue(comp,
			param, index);
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
		const string str = context->GetDefaultStringParameterValue(comp, param, index);
		unsigned int nToCopy = str.length() < dstlen ?
			str.length() + 1 : dstlen;
		if (nToCopy > 0) {
			strncpy(dst, str.c_str(), nToCopy - 1);
			dst[nToCopy - 1] = '\0';
		}
		return str.length();
	}

	const char* getOptions() { return context->registry.GetContent(); }

	//Queryable objects
	//Here I do a special handling for python :
	//Python is dynamically typed, unlike C++ which is statically typed.
	//Python variables may hold an integer, a float, list, dict, tuple, str, long etc., among other things.
	//So we don't need a getINT, getFLOAT, getXXX in the python api
	//This function handles all types
	boost::python::object getOption(const char *objectName, const char *attributeName)
	{
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
		Queryable *object=Context::GetActive()->registry[objectName];
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


	void addServer(const char * name) { context->AddServer(std::string(name)); }
	void removeServer(const char * name) { context->RemoveServer(std::string(name)); }
	unsigned int getServerCount() { return context->GetServerCount(); }
	void updateFilmFromNetwork() { context->UpdateFilmFromNetwork(); }
	void setNetworkServerUpdateInterval(int updateInterval) { context->SetNetworkServerUpdateInterval(updateInterval); }
	int getNetworkServerUpdateInterval() { return context->GetNetworkServerUpdateInterval(); }

	unsigned int getRenderingServersStatus(RenderingServerInfo *info, unsigned int maxInfoCount)
	{
		return context->GetRenderingServersStatus(info, maxInfoCount);
	}

	double statistics(const char *statName) { return context->Statistics(statName); }

	void enableDebugMode() { context->EnableDebugMode(); }
	void disableRandomMode() { context->DisableRandomMode(); }

private:
	Context *context;

	std::vector<boost::thread *> pyLuxWorldEndThreads; //hold pointers to the worldend threads
	void pyWorldEnd()
	{
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

    //Direct python module calls
    def("greet", greet); //Simple test function to check the module is imported
    def("version", luxVersion);

    // Information about the threads
    enum_<ThreadSignals>("ThreadSignals")
        .value("RUN", RUN)
        .value("PAUSE", PAUSE)
        .value("EXIT", EXIT)
        ;
    class_<RenderingThreadInfo>("RenderingThreadInfo")
			.def_readonly("threadIndex", &RenderingThreadInfo::threadIndex)
			.def_readonly("status", &RenderingThreadInfo::status);
        ;

    // Parameter access
    enum_<luxComponent>("luxComponent")
            .value("LUX_FILM", LUX_FILM)
            ;
    enum_<luxComponentParameters>("luxComponentParameters")
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

    class_<RenderingServerInfo>("RenderingServerInfo")
				.def_readonly("serverIndex", &RenderingServerInfo::serverIndex)
    			.def_readonly("name", &RenderingServerInfo::name)
    			.def_readonly("port", &RenderingServerInfo::port)
				.def_readonly("sid", &RenderingServerInfo::sid)
				.def_readonly("numberOfSamplesReceived", &RenderingServerInfo::numberOfSamplesReceived)
				.def_readonly("secsSinceLastContact", &RenderingServerInfo::secsSinceLastContact)
            ;

    //Error handling in python
    def("errorHandler",pyLuxErrorHandler);

    //PyContext class
    class_<PyContext>("Context", init<std::string>())
        .def("greet", &PyContext::greet)
        .def("parse", &PyContext::parse)
        .def("cleanup", &PyContext::cleanup)
        .def("identity", &PyContext::identity)
        .def("translate", &PyContext::translate)
        .def("rotate", &PyContext::rotate)
        .def("scale", &PyContext::scale)
        .def("lookAt", &PyContext::lookAt)
        .def("concatTransform", &PyContext::concatTransform)
        .def("transform", &PyContext::transform)
		.def("coordinateSystem", &PyContext::coordinateSystem)
		.def("coordSysTransform", &PyContext::coordSysTransform)
		.def("pixelFilter", &PyContext::pixelFilter)
		.def("film", &PyContext::film)
		.def("sampler", &PyContext::sampler)
		.def("accelerator", &PyContext::accelerator)
		.def("surfaceIntegrator", &PyContext::surfaceIntegrator)
		.def("volumeIntegrator", &PyContext::volumeIntegrator)
		.def("camera", &PyContext::camera)
		.def("worldBegin", &PyContext::worldBegin)
		.def("worldEnd", &PyContext::worldEnd)
		.def("attributeBegin", &PyContext::attributeBegin)
		.def("attributeEnd", &PyContext::attributeEnd)
		.def("transformBegin", &PyContext::transformBegin)
		.def("transformEnd", &PyContext::transformEnd)
		.def("texture", &PyContext::texture)
		.def("material", &PyContext::material)
		.def("makeNamedMaterial", &PyContext::makeNamedMaterial)
		.def("namedMaterial", &PyContext::namedMaterial)
		.def("lightSource", &PyContext::lightSource)
		.def("areaLightSource", &PyContext::areaLightSource)
		.def("portalShape", &PyContext::portalShape)
		.def("shape", &PyContext::shape)
		.def("reverseOrientation", &PyContext::reverseOrientation)
		.def("volume", &PyContext::volume)
		.def("exterior", &PyContext::exterior)
		.def("interior", &PyContext::interior)
		.def("objectBegin", &PyContext::objectBegin)
		.def("objectEnd", &PyContext::objectEnd)
		.def("objectInstance", &PyContext::objectInstance)
		.def("motionInstance", &PyContext::motionInstance)
		.def("loadFLM", &PyContext::loadFLM)
		.def("saveFLM", &PyContext::saveFLM)
		.def("overrideResumeFLM", &PyContext::overrideResumeFLM)
		.def("start", &PyContext::start)
		.def("pause", &PyContext::pause)
		.def("exit", &PyContext::exit)
		.def("wait", &PyContext::wait)
		.def("setHaltSamplePerPixel", &PyContext::setHaltSamplePerPixel)
		.def("addThread", &PyContext::addThread)
		.def("removeThread", &PyContext::removeThread)
		.def("setEpsilon", &PyContext::setEpsilon)
		.def("getRenderingThreadsStatus", &PyContext::getRenderingThreadsStatus)
		.def("updateFramebuffer", &PyContext::updateFramebuffer)
		.def("framebuffer", &PyContext::framebuffer)
		.def("getHistogramImage", &PyContext::getHistogramImage)
		.def("setParameterValue", &PyContext::setParameterValue)
		.def("getParameterValue", &PyContext::getParameterValue)
		.def("getDefaultParameterValue", &PyContext::getDefaultParameterValue)
		.def("setStringParameterValue", &PyContext::setStringParameterValue)
		.def("getStringParameterValue", &PyContext::getStringParameterValue)
		.def("getDefaultStringParameterValue", &PyContext::getDefaultStringParameterValue)
		.def("getOptions", &PyContext::getOptions)
		.def("getOption", &PyContext::getOption)
		.def("setOption", &PyContext::setOption)
		.def("addServer", &PyContext::addServer)
		.def("removeServer", &PyContext::removeServer)
		.def("getServerCount", &PyContext::getServerCount)
		.def("updateFilmFromNetwork", &PyContext::updateFilmFromNetwork)
		.def("setNetworkServerUpdateInterval", &PyContext::setNetworkServerUpdateInterval)
		.def("getNetworkServerUpdateInterval", &PyContext::getNetworkServerUpdateInterval)
		.def("getRenderingServersStatus", &PyContext::getRenderingServersStatus)
		.def("statistics", &PyContext::statistics)
		.def("enableDebugMode", &PyContext::enableDebugMode)
		.def("disableRandomMode", &PyContext::disableRandomMode)
		;
}
