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

#ifndef LUX_API_H
#define LUX_API_H 1

#ifdef LUX_DLL
#	if defined(WIN32) || defined(__CYGWIN__)
#		ifdef LUX_INTERNAL
#			define LUX_EXPORT __declspec(dllexport)
#		else
#			define LUX_EXPORT __declspec(dllimport)
#		endif
#	else // unix
#		ifdef LUX_INTERNAL
#			define LUX_EXPORT __attribute__ ((visibility ("default")))
#		else
#			define LUX_EXPORT
#		endif
#	endif
#else
#	define LUX_EXPORT
#endif

#ifdef __cplusplus
extern "C" {
#endif

typedef const char *LuxToken;
typedef const char *LuxPointer;
#define LUX_NULL NULL

LUX_EXPORT const char *luxVersion();
LUX_EXPORT void luxInit();
LUX_EXPORT int luxParse(const char *filename);
/* allows for parsing of partial files, caller does error handling */
LUX_EXPORT int luxParsePartial(const char *filename);
LUX_EXPORT void luxCleanup();

/* Basic control flow, scoping, stacks */
LUX_EXPORT void luxIdentity();
LUX_EXPORT void luxTranslate(float dx, float dy, float dz);
LUX_EXPORT void luxRotate(float angle, float ax, float ay, float az);
LUX_EXPORT void luxScale(float sx, float sy, float sz);
LUX_EXPORT void luxLookAt(float ex, float ey, float ez, float lx, float ly, float lz, float ux, float uy, float uz);
LUX_EXPORT void luxConcatTransform(float transform[16]);
LUX_EXPORT void luxTransform(float transform[16]);
LUX_EXPORT void luxCoordinateSystem(const char *);
LUX_EXPORT void luxCoordSysTransform(const char *);
LUX_EXPORT void luxPixelFilter(const char *name, ...);
LUX_EXPORT void luxPixelFilterV(const char *name, unsigned int n, const LuxToken tokens[], const LuxPointer params[]);
LUX_EXPORT void luxFilm(const char *name, ...);
LUX_EXPORT void luxFilmV(const char *name, unsigned int n, const LuxToken tokens[], const LuxPointer params[]);
LUX_EXPORT void luxSampler(const char *name, ...);
LUX_EXPORT void luxSamplerV(const char *name, unsigned int n, const LuxToken tokens[], const LuxPointer params[]);
LUX_EXPORT void luxAccelerator(const char *name, ...);
LUX_EXPORT void luxAcceleratorV(const char *name, unsigned int n, const LuxToken tokens[], const LuxPointer params[]);
LUX_EXPORT void luxSurfaceIntegrator(const char *name, ...);
LUX_EXPORT void luxSurfaceIntegratorV(const char *name, unsigned int n, const LuxToken tokens[], const LuxPointer params[]);
LUX_EXPORT void luxVolumeIntegrator(const char *name, ...);
LUX_EXPORT void luxVolumeIntegratorV(const char *name, unsigned int n, const LuxToken tokens[], const LuxPointer params[]);
LUX_EXPORT void luxCamera(const char *name, ...);
LUX_EXPORT void luxCameraV(const char *name, unsigned int n, const LuxToken tokens[], const LuxPointer params[]);
LUX_EXPORT void luxWorldBegin();
LUX_EXPORT void luxAttributeBegin();
LUX_EXPORT void luxAttributeEnd();
LUX_EXPORT void luxTransformBegin();
LUX_EXPORT void luxTransformEnd();
LUX_EXPORT void luxMotionBegin(unsigned int n, float *t);
LUX_EXPORT void luxMotionEnd();
LUX_EXPORT void luxTexture(const char *name, const char *type, const char *texname, ...);
LUX_EXPORT void luxTextureV(const char *name, const char *type, const char *texname, unsigned int n, const LuxToken tokens[], const LuxPointer params[]);
LUX_EXPORT void luxMaterial(const char *name, ...);
LUX_EXPORT void luxMaterialV(const char *name, unsigned int n, const LuxToken tokens[], const LuxPointer params[]);
LUX_EXPORT void luxMakeNamedMaterial(const char *name, ...);
LUX_EXPORT void luxMakeNamedMaterialV(const char *name, unsigned int n, const LuxToken tokens[], const LuxPointer params[]);
LUX_EXPORT void luxNamedMaterial(const char *name);
LUX_EXPORT void luxLightSource(const char *name, ...);
LUX_EXPORT void luxLightSourceV(const char *name, unsigned int n, const LuxToken tokens[], const LuxPointer params[]);
LUX_EXPORT void luxAreaLightSource(const char *name, ...);
LUX_EXPORT void luxAreaLightSourceV(const char *name, unsigned int n, const LuxToken tokens[], const LuxPointer params[]);
LUX_EXPORT void luxPortalShape(const char *name, ...);
LUX_EXPORT void luxPortalShapeV(const char *name, unsigned int n, const LuxToken tokens[], const LuxPointer params[]);
LUX_EXPORT void luxShape(const char *name, ...);
LUX_EXPORT void luxShapeV(const char *name, unsigned int n, const LuxToken tokens[], const LuxPointer params[]);
LUX_EXPORT void luxRenderer(const char *name, ...);
LUX_EXPORT void luxRendererV(const char *name, unsigned int n, const LuxToken tokens[], const LuxPointer params[]);
LUX_EXPORT void luxReverseOrientation();
LUX_EXPORT void luxMakeNamedVolume(const char *id, const char *name, ...);
LUX_EXPORT void luxMakeNamedVolumeV(const char *id, const char *name, unsigned int n,
	const LuxToken tokens[], const LuxPointer params[]);
LUX_EXPORT void luxVolume(const char *name, ...);
LUX_EXPORT void luxVolumeV(const char *name, unsigned int n, const LuxToken tokens[], const LuxPointer params[]);
LUX_EXPORT void luxExterior(const char *name);
LUX_EXPORT void luxInterior(const char *name);
LUX_EXPORT void luxObjectBegin(const char *name);
LUX_EXPORT void luxObjectEnd();
LUX_EXPORT void luxObjectInstance(const char *name);
LUX_EXPORT void luxPortalInstance(const char *name);
LUX_EXPORT void luxMotionInstance(const char *name, float startTime, float endTime, const char *toTransform);
LUX_EXPORT void luxWorldEnd();

/* Load/Save FLM file */
LUX_EXPORT void luxLoadFLM(const char* name);
LUX_EXPORT void luxSaveFLM(const char* name);
/* Overrides the resume settings of the Film in the next scene to resume from the given FLM file, or use filename from scene if empty */
LUX_EXPORT void luxOverrideResumeFLM(const char *name);
/* Overrides the Film output filename */
LUX_EXPORT void luxOverrideFilename(const char *name);

/* Write film to a floating point OpenEXR image */
LUX_EXPORT void luxSaveEXR(const char* name, bool useHalfFloat, bool includeZBuffer, int compressionType, bool tonemapped);

/* User interactive thread functions */
LUX_EXPORT void luxStart();
LUX_EXPORT void luxPause();
LUX_EXPORT void luxExit();
LUX_EXPORT void luxAbort();
LUX_EXPORT void luxWait();

LUX_EXPORT void luxSetHaltSamplesPerPixel(int haltspp, bool haveEnoughSamplesPerPixel, bool suspendThreadsWhenDone);

/* Controlling number of threads */
LUX_EXPORT unsigned int luxAddThread();
LUX_EXPORT void luxRemoveThread();

/* Set the minimum and maximum value used for epsilon */
LUX_EXPORT void luxSetEpsilon(const float minValue, const float maxValue);

/* Framebuffer access */
LUX_EXPORT void luxUpdateFramebuffer();
LUX_EXPORT unsigned char* luxFramebuffer();
LUX_EXPORT float* luxFloatFramebuffer();
LUX_EXPORT float* luxAlphaBuffer();

/* User defined sampling */
LUX_EXPORT void luxSetUserSamplingMap(const float *map);
// NOTE: returns a copy of the map, it is up to the caller to free the allocated memory !
LUX_EXPORT float *luxGetUserSamplingMap();

/* Histogram access */
LUX_EXPORT void luxGetHistogramImage(unsigned char *outPixels, unsigned int width, unsigned int height, int options);
//histogram drawing options
#define    LUX_HISTOGRAM_RGB    	1
#define    LUX_HISTOGRAM_RGB_ADD	2
#define    LUX_HISTOGRAM_RED    	4
#define    LUX_HISTOGRAM_GREEN  	8
#define    LUX_HISTOGRAM_BLUE   	16
#define    LUX_HISTOGRAM_VALUE  	32
#define    LUX_HISTOGRAM_LOG    	64


/* Parameter access */
// Exposed Parameters

enum luxComponent {			LUX_FILM
};

enum luxComponentParameters {	LUX_FILM_TM_TONEMAPKERNEL,
				LUX_FILM_TM_REINHARD_PRESCALE,
				LUX_FILM_TM_REINHARD_POSTSCALE,
				LUX_FILM_TM_REINHARD_BURN,
				LUX_FILM_TM_LINEAR_SENSITIVITY,
				LUX_FILM_TM_LINEAR_EXPOSURE,
				LUX_FILM_TM_LINEAR_FSTOP,
				LUX_FILM_TM_LINEAR_GAMMA,
				LUX_FILM_TM_CONTRAST_YWA,
				LUX_FILM_TORGB_X_WHITE,
				LUX_FILM_TORGB_Y_WHITE,
				LUX_FILM_TORGB_X_RED,
				LUX_FILM_TORGB_Y_RED,
				LUX_FILM_TORGB_X_GREEN,
				LUX_FILM_TORGB_Y_GREEN,
				LUX_FILM_TORGB_X_BLUE,
				LUX_FILM_TORGB_Y_BLUE,
				LUX_FILM_TORGB_GAMMA,
				LUX_FILM_UPDATEBLOOMLAYER,
				LUX_FILM_DELETEBLOOMLAYER,
				LUX_FILM_BLOOMRADIUS,
				LUX_FILM_BLOOMWEIGHT,
				LUX_FILM_VIGNETTING_ENABLED,
				LUX_FILM_VIGNETTING_SCALE,
				LUX_FILM_ABERRATION_ENABLED,
				LUX_FILM_ABERRATION_AMOUNT,
				LUX_FILM_UPDATEGLARELAYER,
				LUX_FILM_DELETEGLARELAYER,
				LUX_FILM_GLARE_AMOUNT,
				LUX_FILM_GLARE_RADIUS,
				LUX_FILM_GLARE_BLADES,
				LUX_FILM_HISTOGRAM_ENABLED,
				LUX_FILM_NOISE_CHIU_ENABLED,
				LUX_FILM_NOISE_CHIU_RADIUS,
				LUX_FILM_NOISE_CHIU_INCLUDECENTER,
				LUX_FILM_NOISE_GREYC_ENABLED,
				LUX_FILM_NOISE_GREYC_AMPLITUDE,
				LUX_FILM_NOISE_GREYC_NBITER,
				LUX_FILM_NOISE_GREYC_SHARPNESS,
				LUX_FILM_NOISE_GREYC_ANISOTROPY,
				LUX_FILM_NOISE_GREYC_ALPHA,
				LUX_FILM_NOISE_GREYC_SIGMA,
				LUX_FILM_NOISE_GREYC_FASTAPPROX,
				LUX_FILM_NOISE_GREYC_GAUSSPREC,
				LUX_FILM_NOISE_GREYC_DL,
				LUX_FILM_NOISE_GREYC_DA,
				LUX_FILM_NOISE_GREYC_INTERP,
				LUX_FILM_NOISE_GREYC_TILE,
				LUX_FILM_NOISE_GREYC_BTILE,
				LUX_FILM_NOISE_GREYC_THREADS,
				LUX_FILM_LG_COUNT,
				LUX_FILM_LG_ENABLE,
				LUX_FILM_LG_NAME,
				LUX_FILM_LG_SCALE,
				LUX_FILM_LG_SCALE_RED,
				LUX_FILM_LG_SCALE_BLUE,
				LUX_FILM_LG_SCALE_GREEN,
				LUX_FILM_LG_TEMPERATURE,
				LUX_FILM_LG_SCALE_X,
				LUX_FILM_LG_SCALE_Y,
				LUX_FILM_LG_SCALE_Z,
				LUX_FILM_GLARE_THRESHOLD,
				LUX_FILM_CAMERA_RESPONSE_ENABLED,
				LUX_FILM_CAMERA_RESPONSE_FILE,
				LUX_FILM_LDR_CLAMP_METHOD,
				LUX_FILM_TM_FALSE_METHOD,
				LUX_FILM_TM_FALSE_COLORSCALE,
				LUX_FILM_TM_FALSE_MAX,
				LUX_FILM_TM_FALSE_MIN,
				LUX_FILM_TM_FALSE_MAXSAT,
				LUX_FILM_TM_FALSE_MINSAT,
				LUX_FILM_TM_FALSE_AVGLUM,
				LUX_FILM_TM_FALSE_AVGEMI,
				LUX_FILM_GLARE_MAP,
				LUX_FILM_GLARE_PUPIL,
				LUX_FILM_GLARE_LASHES
};

/* Parameter Access functions */
LUX_EXPORT void luxSetParameterValue(luxComponent comp, luxComponentParameters param, double value, unsigned int index = 0);
LUX_EXPORT double luxGetParameterValue(luxComponent comp, luxComponentParameters param, unsigned int index = 0);
LUX_EXPORT double luxGetDefaultParameterValue(luxComponent comp, luxComponentParameters param, unsigned int index = 0);
LUX_EXPORT void luxSetStringParameterValue(luxComponent comp, luxComponentParameters param, const char* value, unsigned int index = 0);
// an 0-terminated string is copied to dst (a buffer of at least dstlen chars), the length of the entire string is returned
LUX_EXPORT unsigned int luxGetStringParameterValue(luxComponent comp, luxComponentParameters param, char* dst, unsigned int dstlen, unsigned int index = 0);
LUX_EXPORT unsigned int luxGetDefaultStringParameterValue(luxComponent comp, luxComponentParameters param, char* dst, unsigned int dstlen, unsigned int index = 0);

/* Queryable objects */
enum luxAttributeType { // This should stay in sync with AttributeType::DataType
	LUX_ATTRIBUTETYPE_NONE,
	LUX_ATTRIBUTETYPE_BOOL,
	LUX_ATTRIBUTETYPE_INT,
	LUX_ATTRIBUTETYPE_FLOAT,
	LUX_ATTRIBUTETYPE_DOUBLE,
	LUX_ATTRIBUTETYPE_STRING
};
#define LUX_
LUX_EXPORT const char* luxGetAttributes(); /* Returns an XML string containing all queryable data of the current context */
LUX_EXPORT bool luxHasObject(const char * objectName); /* Returns true if the given object exists in the registry */
LUX_EXPORT bool luxHasAttribute(const char * objectName, const char * attributeName); /* Returns true if object has the given attribute */
LUX_EXPORT luxAttributeType luxGetAttributeType(const char *objectName, const char *attributeName); /* Returns the type of the attribute */
LUX_EXPORT unsigned int luxGetAttributeDescription(const char * objectName, const char * attributeName, char * dst, unsigned int dstlen); /* Gets the description of the attribute */
LUX_EXPORT bool luxHasAttributeDefaultValue(const char * objectName, const char * attributeName); /* Returns true if attribute has a default value */

LUX_EXPORT unsigned int luxGetStringAttribute(const char * objectName, const char * attributeName, char * dst, unsigned int dstlen);
LUX_EXPORT unsigned int luxGetStringAttributeDefault(const char * objectName, const char * attributeName, char * dst, unsigned int dstlen);
LUX_EXPORT void luxSetStringAttribute(const char * objectName, const char * attributeName, const char * value);
LUX_EXPORT float luxGetFloatAttribute(const char * objectName, const char * attributeName); /* Returns the value of a float attribute */
LUX_EXPORT float luxGetFloatAttributeDefault(const char * objectName, const char * attributeName); /* Returns the default value of a float attribute */
LUX_EXPORT void luxSetFloatAttribute(const char * objectName, const char * attributeName, float value); /* Sets an float attribute value */
LUX_EXPORT double luxGetDoubleAttribute(const char * objectName, const char * attributeName); /* Returns the value of a double attribute */
LUX_EXPORT double luxGetDoubleAttributeDefault(const char * objectName, const char * attributeName); /* Returns the default value of a double attribute */
LUX_EXPORT void luxSetDoubleAttribute(const char * objectName, const char * attributeName, double value); /* Sets an double attribute value */
LUX_EXPORT int luxGetIntAttribute(const char * objectName, const char * attributeName); /* Returns the value of an int attribute */
LUX_EXPORT int luxGetIntAttributeDefault(const char * objectName, const char * attributeName); /* Returns the default value of an int attribute */
LUX_EXPORT void luxSetIntAttribute(const char * objectName, const char * attributeName, int value); /* Sets an int attribute value */
LUX_EXPORT bool luxGetBoolAttribute(const char * objectName, const char * attributeName); /* Returns the value of a bool attribute */
LUX_EXPORT bool luxGetBoolAttributeDefault(const char * objectName, const char * attributeName); /* Returns the default value of a bool attribute */
LUX_EXPORT void luxSetBoolAttribute(const char * objectName, const char * attributeName, bool value); /* Sets a bool attribute value */
LUX_EXPORT void luxSetAttribute(const char * objectName, const char * attributeName, int n, void *values); /* Sets an attribute value */

/* Networking */
LUX_EXPORT void luxAddServer(const char * name);
LUX_EXPORT void luxRemoveServer(const char * name);
LUX_EXPORT void luxResetServer(const char * name, const char * password);
LUX_EXPORT unsigned int luxGetServerCount();	// deprecated
LUX_EXPORT void luxUpdateFilmFromNetwork();
LUX_EXPORT void luxUpdateLogFromNetwork();
LUX_EXPORT void luxSetNetworkServerUpdateInterval(int updateInterval);	// deprecated
LUX_EXPORT int luxGetNetworkServerUpdateInterval();	// deprecated

struct RenderingServerInfo {
	int serverIndex;

	// Dade - connection information
	const char *name; // Dade - name/ip address of the server
	const char *port; // Dade - tcp port of the server
	const char *sid; // Dade - session id for the server

	double numberOfSamplesReceived;
	double calculatedSamplesPerSecond;
	unsigned int secsSinceLastContact;
	unsigned int secsSinceLastSamples;
};
// Dade - return the number of rendering servers and fill the info buffer with
// information about the servers
LUX_EXPORT unsigned int luxGetRenderingServersStatus(RenderingServerInfo *info, unsigned int maxInfoCount);

/* Informations and statistics */
LUX_EXPORT double luxStatistics(const char *statName);
LUX_EXPORT const char* luxPrintableStatistics(const bool add_total);	// Deprecated
LUX_EXPORT void luxUpdateStatisticsWindow();

// Dade - enable debug mode
LUX_EXPORT void luxEnableDebugMode();
LUX_EXPORT void luxDisableRandomMode();

/* Error Handlers */
LUX_EXPORT extern int luxLastError; /*  Keeps track of the last error code */
LUX_EXPORT extern void luxErrorFilter(int severity); /* Sets the minimal level of severity to report */
LUX_EXPORT extern int luxGetErrorFilter(); /* Gets the minimal level of severity to report */
typedef void (*LuxErrorHandler)(int code, int severity, const char *msg);
LUX_EXPORT extern void luxErrorHandler(LuxErrorHandler handler);
LUX_EXPORT extern void luxErrorAbort(int code, int severity, const char *message);
LUX_EXPORT extern void luxErrorIgnore(int code, int severity, const char *message);
LUX_EXPORT extern void luxErrorPrint(int code, int severity, const char *message);
LUX_EXPORT extern LuxErrorHandler luxError;

/**
 * Reduce the magnitude on the input number by dividing into kilo- or Mega- or Giga- units
 * Used in conjuction with luxMagnitudePrefix
 */
LUX_EXPORT double luxMagnitudeReduce(double number);
/**
 * Return the magnitude prefix char for kilo- or Mega- or Giga-
 */
LUX_EXPORT const char* luxMagnitudePrefix(double number);

/*
 Error codes

 1 - 10     System and File errors
 11 - 20     Program Limitations
 21 - 40     State Errors
 41 - 60     Parameter and Protocol Errors
 61 - 80     Execution Errors
 */

#define LUX_NOERROR         0

#define LUX_NOMEM           1       /* Out of memory */
#define LUX_SYSTEM          2       /* Miscellaneous system error */
#define LUX_NOFILE          3       /* File nonexistant */
#define LUX_BADFILE         4       /* Bad file format */
#define LUX_BADVERSION      5       /* File version mismatch */
#define LUX_DISKFULL        6       /* Target disk is full */

#define LUX_UNIMPLEMENT    12       /* Unimplemented feature */
#define LUX_LIMIT          13       /* Arbitrary program limit */
#define LUX_BUG            14       /* Probably a bug in renderer */

#define LUX_NOTSTARTED     23       /* luxInit() not called */
#define LUX_NESTING        24       /* Bad begin-end nesting - jromang will be used in API v2 */
#define LUX_NOTOPTIONS     25       /* Invalid state for options - jromang will be used in API v2 */
#define LUX_NOTATTRIBS     26       /* Invalid state for attributes - jromang will be used in API v2 */
#define LUX_NOTPRIMS       27       /* Invalid state for primitives - jromang will be used in API v2 */
#define LUX_ILLSTATE       28       /* Other invalid state - jromang will be used in API v2 */
#define LUX_BADMOTION      29       /* Badly formed motion block - jromang will be used in API v2 */
#define LUX_BADSOLID       30       /* Badly formed solid block - jromang will be used in API v2 */

#define LUX_BADTOKEN       41       /* Invalid token for request */
#define LUX_RANGE          42       /* Parameter out of range */
#define LUX_CONSISTENCY    43       /* Parameters inconsistent */
#define LUX_BADHANDLE      44       /* Bad object/light handle */
#define LUX_NOPLUGIN       45       /* Can't load requested plugin */
#define LUX_MISSINGDATA    46       /* Required parameters not provided */
#define LUX_SYNTAX         47       /* Declare type syntax error */

#define LUX_MATH           61       /* Zerodivide, noninvert matrix, etc. */

/* Error severity levels */

#define LUX_DEBUG			-1		/* Debugging output */

#define LUX_INFO            0       /* Rendering stats & other info */
#define LUX_WARNING         1       /* Something seems wrong, maybe okay */
#define LUX_ERROR           2       /* Problem.  Results may be wrong */
#define LUX_SEVERE          3       /* So bad you should probably abort */

#ifdef __cplusplus
}
#endif

#endif /* LUX_API_H */
