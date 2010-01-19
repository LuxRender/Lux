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

/* This file is the Python binding to the Lux API */

#include <iostream>
#include <vector>
#include <cstring>
#include <boost/python.hpp>
#include <boost/python/type_id.hpp>
#include <boost/pool/pool.hpp>
#include "error.h"

#include "api.h"

#define	EXTRACT_PARAMETERS(_params) \
	std::vector<LuxToken> aTokens; \
	std::vector<LuxPointer> aValues; \
	int count = getParametersFromPython(_params, aTokens, aValues);

#define PASS_PARAMETERS \
	count, aTokens.size()>0?&aTokens[0]:0, aValues.size()>0?&aValues[0]:0

//API test in python
/*
import pylux as lux
lux.init()
lux.lookAt(0,10,100,0,-1,0,0,1,0)
lux.camera("perspective",[("fov",30.0)])
lux.pixelFilter("mitchell",[ ("xwidth",2.0), ("ywidth",2.0)  ])
lux.sampler("random",[])
lux.film("multiimage", [ ("filename","simple.png"), ("xresolution",200), ("yresolution",200) ])
lux.worldBegin()

lux.attributeBegin()
lux.coordSysTransform("camera")
lux.lightSource("distant", [ ("from",(0.0,0.0,0.0)), ("to",(0.0,0.0,1.0)) ])
lux.attributeEnd()

lux.attributeBegin()
lux.rotate(135.0,1.0,0.0,0.0)

lux.texture("checks","color","checkerboard",[ ("uscale",4.0), ("vscale",4.0), ("tex1",(1.0,0.0,0.0)), ("tex2",(0.0,0.0,1.0)) ])
lux.material("matte",[("Kd","checks")])
lux.shape("disk",[ ("radius",20.0),("height",-1.0) ])
lux.attributeEnd()
lux.worldEnd()
 */

/*

>>> import pylux as lux
>>> lux.init()
>>> def myPrettyCoolPythonErrorHandler(code,severity,message):
...     print "This error is printed from python :" +message
...     return
...
>>> lux.errorHandler(myPrettyCoolPythonErrorHandler)

This error is printed from python :Compiled scene size: 1KBytes
This error is printed from python :Parameters 'xsamples' and 'ysamples' are deprecated, use 'pixelsamples' instead
This error is printed from python :Building KDTree, primitives: 1
This error is printed from python :Preprocess thread uses seed: 1804289382
This error is printed from python :Thread 0 uses seed: 1804289383


 */

namespace lux{

//The memory pool handles temporary allocations and is freed after each C API Call
boost::pool<> memoryPool(sizeof(char));

//Simple function to test if pylux module was successfully loaded in python
char const* greet()
{
   return "Hello from pylux !";
}

//jromang - TODO remove that :-) this just just for testing
void testCAPI()
{
	float fov=30;
	float size=2, radius=20, height=-1;
	int resolution=200;
	float from[3]= {0,0,0};
	float to[3]= {0,0,1};
	float color1[3]={1,0,0},color2[3]={0,0,1};
	float scale=4;
	const char *filename="simple.png";

	luxInit();
	luxLookAt(0,10,100,0,-1,0,0,1,0);
	luxCamera("perspective","fov",&fov,LUX_NULL);
	luxPixelFilter("mitchell","xwidth", &size, "ywidth" , &size,LUX_NULL);
	luxSampler("random",LUX_NULL);
	luxFilm("multiimage","filename",filename,"xresolution",&resolution,"yresolution",&resolution,LUX_NULL);
	luxWorldBegin();

	luxAttributeBegin();
	luxCoordSysTransform("camera");
	luxLightSource("distant" ,"from",from,"to",to,LUX_NULL);
	luxAttributeEnd();

	luxAttributeBegin();
	luxRotate(135,1,0,0);

	luxTexture("checks","color","checkerboard" ,"uscale",&scale,"vscale",&scale,"tex1",color1, "tex2",color2,LUX_NULL);
	luxMaterial("matte","Kd","checks",LUX_NULL);
	luxShape("disk","radius",&radius,"height",&height,LUX_NULL);
	luxAttributeEnd();
	luxWorldEnd();

}

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

void pyLuxPixelFilter(const char *name, boost::python::list params)
{
	EXTRACT_PARAMETERS(params);
	luxPixelFilterV(name,PASS_PARAMETERS);
	memoryPool.purge_memory();
}

void pyLuxFilm(const char *name, boost::python::list params)
{
	EXTRACT_PARAMETERS(params);
	luxFilmV(name, PASS_PARAMETERS);
	memoryPool.purge_memory();
}

void pyLuxSampler(const char *name, boost::python::list params)
{
	EXTRACT_PARAMETERS(params);
	luxSamplerV(name,PASS_PARAMETERS);
	memoryPool.purge_memory();
}

void pyLuxAccelerator(const char *name, boost::python::list params)
{
	EXTRACT_PARAMETERS(params);
	luxAcceleratorV(name, PASS_PARAMETERS);
	memoryPool.purge_memory();
}

void pyLuxSurfaceIntegrator(const char *name, boost::python::list params)
{
	EXTRACT_PARAMETERS(params);
	luxSurfaceIntegratorV(name, PASS_PARAMETERS);
	memoryPool.purge_memory();
}

void pyLuxVolumeIntegrator(const char *name, boost::python::list params)
{
	EXTRACT_PARAMETERS(params);
	luxVolumeIntegratorV(name, PASS_PARAMETERS);
	memoryPool.purge_memory();
}

void pyLuxCamera(const char *name, boost::python::list params)
{
	EXTRACT_PARAMETERS(params);
	luxCameraV(name, PASS_PARAMETERS);
	memoryPool.purge_memory();
}

void pyLuxTexture(const char *name, const char *type, const char *texname, boost::python::list params)
{
	EXTRACT_PARAMETERS(params);
	luxTextureV(name, type, texname, PASS_PARAMETERS);
	memoryPool.purge_memory();
}

void pyLuxMaterial(const char *name, boost::python::list params)
{
	EXTRACT_PARAMETERS(params);
	luxMaterialV(name, PASS_PARAMETERS);
	memoryPool.purge_memory();
}

void pyLuxMakeNamedMaterial(const char *name, boost::python::list params)
{
	EXTRACT_PARAMETERS(params);
	luxMakeNamedMaterialV(name, PASS_PARAMETERS);
	memoryPool.purge_memory();
}

void pyLuxNamedMaterial(const char *name, boost::python::list params)
{
	EXTRACT_PARAMETERS(params);
	luxNamedMaterialV(name, PASS_PARAMETERS);
	memoryPool.purge_memory();
}

void pyLuxLightSource(const char *name, boost::python::list params)
{
	EXTRACT_PARAMETERS(params);
	luxLightSourceV(name, PASS_PARAMETERS);
	memoryPool.purge_memory();
}

void pyLuxAreaLightSource(const char *name, boost::python::list params)
{
	EXTRACT_PARAMETERS(params);
	luxAreaLightSourceV(name, PASS_PARAMETERS);
	memoryPool.purge_memory();
}

void pyLuxPortalShape(const char *name, boost::python::list params)
{
	EXTRACT_PARAMETERS(params);
	luxPortalShapeV(name, PASS_PARAMETERS);
	memoryPool.purge_memory();
}

void pyLuxShape(const char *name, boost::python::list params)
{
	EXTRACT_PARAMETERS(params);
	luxShapeV(name, PASS_PARAMETERS);
	memoryPool.purge_memory();
}

void pyLuxVolume(const char *name, boost::python::list params)
{
	EXTRACT_PARAMETERS(params);
	luxVolumeV(name, PASS_PARAMETERS);
	memoryPool.purge_memory();
}

void pyLuxExterior(const char *name, boost::python::list params)
{
	EXTRACT_PARAMETERS(params);
	luxExteriorV(name, PASS_PARAMETERS);
	memoryPool.purge_memory();
}

void pyLuxInterior(const char *name, boost::python::list params)
{
	EXTRACT_PARAMETERS(params);
	luxInteriorV(name, PASS_PARAMETERS);
	memoryPool.purge_memory();
}

//typedef void (*LuxErrorHandler)(int code, int severity, const char *msg);
//extern void luxErrorHandler(LuxErrorHandler handler);
boost::python::object pythonErrorHandler;

void luxErrorPython(int code, int severity, const char *message)
{
	pythonErrorHandler(code, severity, message);
}

void pyLuxErrorHandler(boost::python::object handler)
{
	pythonErrorHandler=handler;
	luxErrorHandler(luxErrorPython);
}


}//namespace lux


BOOST_PYTHON_MODULE(pylux)
{
    using namespace boost::python;
    using namespace lux;

    def("greet", greet);
    def("testCAPI",testCAPI); //jromang TODO : remove

    def("init", luxInit);
    def("cleanup", luxCleanup);
    def("identity", luxIdentity);
    def("translate", luxTranslate);
    def("rotate", luxRotate);
    def("scale", luxScale);
    def("lookAt", luxLookAt);
    def("concatTransform", luxConcatTransform);
    def("transform", luxTransform);
    def("coordinateSystem", luxCoordinateSystem);
    def("coordSysTransform", luxCoordSysTransform);
    def("pixelFilter", pyLuxPixelFilter); //wrapper
    def("film",pyLuxFilm); //wrapper
    def("sampler",pyLuxSampler); //wrapper
    def("accelerator",pyLuxAccelerator); //wrapper
    def("surfaceIntegrator",pyLuxSurfaceIntegrator); //wrapper
    def("volumeIntegrator",pyLuxVolumeIntegrator); //wrapper
    def("camera", pyLuxCamera); //wrapper
    def("worldBegin",luxWorldBegin);
    def("attributeBegin",luxAttributeBegin);
    def("attributeEnd",luxAttributeEnd);
    def("transformBegin",luxTransformBegin);
    def("transformEnd",luxTransformEnd);
    def("texture",pyLuxTexture); //wrapper
    def("material",pyLuxMaterial); //wrapper
    def("makeNamedMaterial",pyLuxMakeNamedMaterial); //wrapper
    def("namedMaterial",pyLuxNamedMaterial); //wrapper
    def("lightSource",pyLuxLightSource); //wrapper
    def("areaLightSource",pyLuxAreaLightSource); //wrapper
    def("portalShape",pyLuxPortalShape); //wrapper
    def("shape",pyLuxShape); //wrapper
    def("reverseOrientation",luxReverseOrientation);
    def("volume",pyLuxVolume); //wrapper
    def("exterior",pyLuxExterior); //wrapper
    def("interior",pyLuxInterior); //wrapper
    def("objectBegin",luxObjectBegin);
    def("objectEnd",luxObjectEnd);
    def("objectInstance",luxObjectInstance);
    def("motionInstance",luxMotionInstance);
    def("worldEnd",luxWorldEnd);
    def("loadFLM",luxLoadFLM);
    def("saveFLM",luxSaveFLM);
    def("overrideResumeFLM",luxOverrideResumeFLM);
    def("start",luxStart);
    def("pause",luxPause);
    def("exit",luxExit);
    def("wait",luxWait);
    def("setHaltSamplePerPixel",luxSetHaltSamplePerPixel);
    def("addThread",luxAddThread);
    def("removeThread",luxRemoveThread);
    def("setEpsilon", luxSetEpsilon);

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
    def("renderingThreadsStatus",luxGetRenderingThreadsStatus);

    // Framebuffer access
    void luxUpdateFramebuffer();
    unsigned char* luxFramebuffer();

    // Histogram access
    def("getHistogramImage",luxGetHistogramImage);

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
    // Parameter Access functions
    def("setParameterValue",luxSetParameterValue);
    def("getParameterValue",luxGetParameterValue);
    def("getDefaultParameterValue",luxGetDefaultParameterValue);
    def("setStringParameterValue",luxSetStringParameterValue);
    def("getStringParameterValue",luxGetStringParameterValue);
    def("getDefaultStringParameterValue",luxGetDefaultStringParameterValue);

    // Networking
    def("addServer",luxAddServer);
    def("removeServer",luxRemoveServer);
    def("getServerCount",luxGetServerCount);
    def("updateFilmFromNetwork",luxUpdateFilmFromNetwork);
    def("setNetworkServerUpdateInterval",luxSetNetworkServerUpdateInterval);
    def("getNetworkServerUpdateInterval",luxGetNetworkServerUpdateInterval);

    class_<RenderingServerInfo>("RenderingServerInfo")
				.def_readonly("serverIndex", &RenderingServerInfo::serverIndex)
    			.def_readonly("name", &RenderingServerInfo::name)
    			.def_readonly("port", &RenderingServerInfo::port)
				.def_readonly("sid", &RenderingServerInfo::sid)
				.def_readonly("numberOfSamplesReceived", &RenderingServerInfo::numberOfSamplesReceived)
				.def_readonly("secsSinceLastContact", &RenderingServerInfo::secsSinceLastContact)
            ;
    // information about the servers
    def("getRenderingServersStatus",luxGetRenderingServersStatus);

    // Informations and statistics
    def("statistics",luxStatistics);

    //Debug mode
    def("enableDebugMode",luxEnableDebugMode);
    def("disableRandomMode",luxDisableRandomMode);

    //TODO jromang : error handling in python
    def("errorHandler",pyLuxErrorHandler);
}
