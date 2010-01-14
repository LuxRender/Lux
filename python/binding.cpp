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

#include <boost/python.hpp>
#include "api.h"

char const* greet()
{
   return "Hello from pylux !";
}

BOOST_PYTHON_MODULE(pylux)
{
    using namespace boost::python;
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
    def("pixelFilter", luxPixelFilterV);
    def("film",luxFilmV);
    def("sampler",luxSamplerV);
    def("accelerator",luxAcceleratorV);
    def("surfaceIntegrator",luxSurfaceIntegratorV);
    def("volumeIntegrator",luxVolumeIntegratorV);
    def("camera",luxCameraV);
    def("worldBegin",luxWorldBegin);
    def("attributeBegin",luxAttributeBegin);
    def("attributeEnd",luxAttributeEnd);
    def("transformBegin",luxTransformBegin);
    def("transformEnd",luxTransformEnd);
    def("texture",luxTextureV);
    def("material",luxMaterialV);
    def("makeNamedMaterial",luxMakeNamedMaterialV);
    def("namedMaterial",luxNamedMaterialV);
    def("lightSource",luxLightSourceV);
    def("areaLightSource",luxAreaLightSourceV);
    def("portalShape",luxPortalShapeV);
    def("shape",luxShapeV);
    def("reverseOrientation",luxReverseOrientation);
    def("volume",luxVolumeV);
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

