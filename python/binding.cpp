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
#include <cstring>
#include <boost/python.hpp>
#include <boost/python/type_id.hpp>

#include "api.h"

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

namespace lux{

char const* greet()
{
   return "Hello from pylux !";
}

//jromang - TODO bound checking
LuxToken pythonTokens[64];
LuxPointer pythonParams[64];


int getParametersFromPython(boost::python::list *pList)
{
	boost::python::ssize_t n = boost::python::len(*pList);
	for(boost::python::ssize_t i=0;i<n;i++)
	{
		//jromang - TODO bound checking
		//jromang - TODO : MEMORY LEAKS : have to use boost memory pool

		boost::python::tuple l=boost::python::extract<boost::python::tuple>((*pList)[i]);
		std::string tokenString=boost::python::extract<std::string>(l[0]);
		pythonTokens[i]=(LuxToken)malloc(sizeof(char)*tokenString.length()+1);
		std::cout<<"We have a nice parameter : ["<<tokenString<<']'<<std::endl;

		boost::python::extract<int> intExtractor(l[1]);
		boost::python::extract<float> floatExtractor(l[1]);
		boost::python::extract<boost::python::tuple> tupleExtractor(l[1]);
		boost::python::extract<std::string> stringExtractor(l[1]);

		if(intExtractor.check())
		{
			pythonParams[i]=(LuxPointer)malloc(sizeof(int));
			*((int*)pythonParams[i])=intExtractor();
			std::cout<<"this is an INT:"<<*((int*)pythonParams[i])<<std::endl;
		}
		else if(floatExtractor.check())
		{
			pythonParams[i]=(LuxPointer)malloc(sizeof(float));
			*((float*)pythonParams[i])=floatExtractor();
			std::cout<<"this is an FLOAT:"<<*((float*)pythonParams[i])<<std::endl;
		}
		else if(stringExtractor.check())
		{
			std::string s=stringExtractor();
			pythonParams[i]=(LuxPointer)malloc(sizeof(char)*s.length()+1);
			strcpy((char*)pythonParams[i],s.c_str());
			std::cout<<"this is a STRING:"<<s<<std::endl;
		}
		else if(tupleExtractor.check())
		{
			//jromang - TODO assuming floats here, but do we only have floats in tuples ?
			boost::python::tuple t=tupleExtractor();
			boost::python::ssize_t tupleSize=boost::python::len(t);
			pythonParams[i]=(LuxPointer)malloc(sizeof(float)*tupleSize);

			std::cout<<"this is a TUPLE - WARNING ASSUMING FLOATS :";

			for(boost::python::ssize_t j=0;j<tupleSize;j++)
			{
				*(((float*)pythonParams[i])+j)=boost::python::extract<float>(t[j]);
				std::cout<<*(((float*)pythonParams[i])+j)<<';';
			}
			std::cout<<std::endl;
		}
		else
		{
			//jromang - TODO throw a good formatted error
			std::cout<<"Error parameter type python"<<std::endl;
		}

	}
	return(n);
}

void pyLuxCamera(const char *name, boost::python::list params)
{
	int n=getParametersFromPython(&params);
	luxCameraV(name,n,pythonTokens,pythonParams);
}

void pyLuxPixelFilter(const char *name, boost::python::list params)
{
	int n=getParametersFromPython(&params);
	luxPixelFilterV(name,n,pythonTokens,pythonParams);
}

void pyLuxSampler(const char *name, boost::python::list params)
{
	int n=getParametersFromPython(&params);
	luxSamplerV(name,n,pythonTokens,pythonParams);
}

void pyLuxFilm(const char *name, boost::python::list params)
{
	int n=getParametersFromPython(&params);
	luxFilmV(name,n,pythonTokens,pythonParams);
}


void pyLuxLightSource(const char *name, boost::python::list params)
{
	int n=getParametersFromPython(&params);
	luxLightSourceV(name,n,pythonTokens,pythonParams);
}

void pyLuxTexture(const char *name, const char *type, const char *texname, boost::python::list params)
{
	int n=getParametersFromPython(&params);
	luxTextureV(name, type, texname, n, pythonTokens,pythonParams);
}

void pyLuxMaterial(const char *name, boost::python::list params)
{
	int n=getParametersFromPython(&params);
	luxMaterialV(name,n,pythonTokens,pythonParams);
}

void pyLuxShape(const char *name, boost::python::list params)
{
	int n=getParametersFromPython(&params);
	luxShapeV(name,n,pythonTokens,pythonParams);
}

}//namespace lux


BOOST_PYTHON_MODULE(pylux)
{
    using namespace boost::python;
    using namespace lux;

    //def("camera2", pyLuxCamera);

    def("greet", greet);
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
    def("accelerator",luxAcceleratorV);
    def("surfaceIntegrator",luxSurfaceIntegratorV);
    def("volumeIntegrator",luxVolumeIntegratorV);
    def("camera", pyLuxCamera); //wrapper
    def("worldBegin",luxWorldBegin);
    def("attributeBegin",luxAttributeBegin);
    def("attributeEnd",luxAttributeEnd);
    def("transformBegin",luxTransformBegin);
    def("transformEnd",luxTransformEnd);
    def("texture",pyLuxTexture); //wrapper
    def("material",pyLuxMaterial); //wrapper
    def("makeNamedMaterial",luxMakeNamedMaterialV);
    def("namedMaterial",luxNamedMaterialV);
    def("lightSource",pyLuxLightSource); //wrapper
    def("areaLightSource",luxAreaLightSourceV);
    def("portalShape",luxPortalShapeV);
    def("shape",pyLuxShape); //wrapper
    def("reverseOrientation",luxReverseOrientation);
    def("volume",luxVolumeV);
    def("exterior",luxExteriorV);
    def("interior",luxInteriorV);
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
}
