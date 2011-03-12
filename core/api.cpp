/***************************************************************************
 *   Copyright (C) 1998-2009 by authors (see AUTHORS.txt )                 *
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

// api.cpp*

#include "api.h"
#include "context.h"
#include "paramset.h"
#include "error.h"

#include <boost/date_time/posix_time/posix_time.hpp>
#include <boost/thread/mutex.hpp>
#include <vector>
using std::vector;
#include <string>
using std::string;
#include <iostream>
using std::cerr;
using std::endl;
#include <cstdarg>
#include <cstdio>
using std::FILE;

#include <FreeImage.h>

using namespace lux;

#define	EXTRACT_PARAMETERS(_start) \
	va_list	pArgs; \
	va_start( pArgs, _start ); \
\
	vector<LuxToken> aTokens; \
	vector<LuxPointer> aValues; \
	unsigned int count = buildParameterList( pArgs, aTokens, aValues );

#define PASS_PARAMETERS \
	count, aTokens.size()>0?&aTokens[0]:0, aValues.size()>0?&aValues[0]:0

namespace lux
{

//----------------------------------------------------------------------
// BuildParameterList
// Helper function to build a parameter list to pass on to the V style functions.
// returns a parameter count.

static unsigned int buildParameterList( va_list pArgs, vector<LuxToken>& aTokens, vector<LuxPointer>& aValues )
{
    unsigned int count = 0;
    LuxToken pToken = va_arg( pArgs, LuxToken );
    LuxPointer pValue;
    aTokens.clear();
    aValues.clear();
    while ( pToken != 0 && pToken != LUX_NULL )          	// While not LUX_NULL
    {
        aTokens.push_back( pToken );
        pValue = va_arg( pArgs, LuxPointer );
        aValues.push_back( pValue );
        pToken = va_arg( pArgs, LuxToken );
        count++;
    }
    return ( count );
}

}

static bool initialized = false;


// API Function Definitions

extern "C" void luxAddServer(const char * name)
{
	Context::GetActive()->AddServer(string(name));
}

extern "C" void luxRemoveServer(const char * name)
{
	Context::GetActive()->RemoveServer(string(name));
}

extern "C" unsigned int luxGetServerCount()
{
	return Context::GetActive()->GetServerCount();
}

extern "C" unsigned int luxGetRenderingServersStatus(RenderingServerInfo *info,
	unsigned int maxInfoCount)
{
	return Context::GetActive()->GetRenderingServersStatus(info,
		maxInfoCount);
}

extern "C" void luxCleanup()
{
	// Context ::luxCleanup reinitializes the core
	// so we must NOT change initialized to false
	if (initialized == true) {
		Context::GetActive()->Cleanup();

		// TODO - find proper place for FreeImage_DeInitialise
		//FreeImage_DeInitialise();
	}
	else
		LOG(LUX_ERROR,LUX_NOTSTARTED)<<"luxCleanup() called without luxInit().";
}

extern "C" void luxIdentity()
{
	Context::GetActive()->Identity();
}
extern "C" void luxTranslate(float dx, float dy, float dz)
{
	Context::GetActive()->Translate(dx,dy,dz);
}
extern "C" void luxTransform(float tr[16])
{
	Context::GetActive()->Transform(tr);
}
extern "C" void luxConcatTransform(float tr[16]) {
	Context::GetActive()->ConcatTransform(tr);
}
extern "C" void luxRotate(float angle, float dx, float dy, float dz)
{
	Context::GetActive()->Rotate(angle,dx,dy,dz);
}
extern "C" void luxScale(float sx, float sy, float sz)
{
	Context::GetActive()->Scale(sx,sy,sz);
}
extern "C" void luxLookAt(float ex, float ey, float ez,
	float lx, float ly, float lz, float ux, float uy, float uz)
{
	Context::GetActive()->LookAt(ex, ey, ez, lx, ly, lz, ux, uy, uz);
}
extern "C" void luxCoordinateSystem(const char *name)
{
	Context::GetActive()->CoordinateSystem(string(name));
}
extern "C" void luxCoordSysTransform(const char *name)
{
	Context::GetActive()->CoordSysTransform(string(name));
}
extern "C" void luxPixelFilter(const char *name, ...)
{
	EXTRACT_PARAMETERS(name);
	luxPixelFilterV(name, PASS_PARAMETERS);
}

extern "C" void luxPixelFilterV(const char *name, unsigned int n,
	const LuxToken tokens[], const LuxPointer params[])
{
	Context::GetActive()->PixelFilter(name,
		ParamSet(n, name, tokens, params));
}

extern "C" void luxFilm(const char *name, ...)
{
	EXTRACT_PARAMETERS(name);
	luxFilmV(name, PASS_PARAMETERS);
}

extern "C" void luxFilmV(const char *name, unsigned int n,
	const LuxToken tokens[], const LuxPointer params[])
{
	Context::GetActive()->Film(name, ParamSet(n, name, tokens, params));
}

extern "C" void luxSampler(const char *name, ...)
{
	EXTRACT_PARAMETERS(name);
	luxSamplerV(name, PASS_PARAMETERS);
}

extern "C" void luxSamplerV(const char *name, unsigned int n,
	const LuxToken tokens[], const LuxPointer params[])
{
	Context::GetActive()->Sampler(name, ParamSet(n, name, tokens, params));
}

extern "C" void luxAccelerator(const char *name, ...)
{
	EXTRACT_PARAMETERS(name);
	luxAcceleratorV(name, PASS_PARAMETERS);
}

extern "C" void luxAcceleratorV(const char *name, unsigned int n,
	const LuxToken tokens[], const LuxPointer params[])
{
	Context::GetActive()->Accelerator(name,
		ParamSet(n, name, tokens, params));
}

extern "C" void luxSurfaceIntegrator(const char *name, ...)
{
	EXTRACT_PARAMETERS(name);
	luxSurfaceIntegratorV(name, PASS_PARAMETERS);
}

extern "C" void luxSurfaceIntegratorV(const char *name, unsigned int n,
	const LuxToken tokens[], const LuxPointer params[])
{
	Context::GetActive()->SurfaceIntegrator(name,
		ParamSet(n, name, tokens, params));
}

extern "C" void luxVolumeIntegrator(const char *name, ...)
{
	EXTRACT_PARAMETERS(name);
	luxVolumeIntegratorV(name, PASS_PARAMETERS);
}

extern "C" void luxVolumeIntegratorV(const char *name, unsigned int n,
	const LuxToken tokens[], const LuxPointer params[])
{
	Context::GetActive()->VolumeIntegrator(name,
		ParamSet(n, name, tokens, params));
}

extern "C" void luxCamera(const char *name, ...)
{
	EXTRACT_PARAMETERS(name);
	luxCameraV(name, PASS_PARAMETERS);
}

extern "C" void luxCameraV(const char *name, unsigned int n,
	const LuxToken tokens[], const LuxPointer params[])
{
	Context::GetActive()->Camera(name, ParamSet(n, name, tokens, params));
}

extern "C" void luxWorldBegin()
{
	Context::GetActive()->WorldBegin();
}

extern "C" void luxAttributeBegin()
{
	Context::GetActive()->AttributeBegin();
}

extern "C" void luxAttributeEnd()
{
	Context::GetActive()->AttributeEnd();
}

extern "C" void luxTransformBegin()
{
	Context::GetActive()->TransformBegin();
}

extern "C" void luxTransformEnd()
{
	Context::GetActive()->TransformEnd();
}

extern "C" void luxTexture(const char *name, const char *type,
	const char *texname, ...)
{
	EXTRACT_PARAMETERS(texname);
	luxTextureV(name, type, texname, PASS_PARAMETERS);
}

extern "C" void luxTextureV(const char *name, const char *type,
	const char *texname, unsigned int n, const LuxToken tokens[],
	const LuxPointer params[])
{
	Context::GetActive()->Texture(name, type, texname,
		ParamSet(n, name, tokens, params));
}

extern "C" void luxMaterial(const char *name, ...)
{
	EXTRACT_PARAMETERS(name);
	luxMaterialV(name, PASS_PARAMETERS);
}

extern "C" void luxMaterialV(const char *name, unsigned int n,
	const LuxToken tokens[], const LuxPointer params[])
{
	Context::GetActive()->Material(name, ParamSet(n, name, tokens,
		params));
}

extern "C" void luxMakeNamedMaterial(const char *name, ...)
{
	EXTRACT_PARAMETERS(name);
	luxMakeNamedMaterialV(name, PASS_PARAMETERS);
}

extern "C" void luxMakeNamedMaterialV(const char *name, unsigned int n,
	const LuxToken tokens[], const LuxPointer params[])
{
	ParamSet p(n, name, tokens, params);
	Context::GetActive()->MakeNamedMaterial(name,p);
}

extern "C" void luxNamedMaterial(const char *name)
{
	Context::GetActive()->NamedMaterial(name);
}

extern "C" void luxLightSource(const char *name, ...)
{
	EXTRACT_PARAMETERS(name);
	luxLightSourceV(name, PASS_PARAMETERS);
}

extern "C" void luxLightSourceV(const char *name, unsigned int n,
	const LuxToken tokens[], const LuxPointer params[])
{
	Context::GetActive()->LightSource(name,
		ParamSet(n, name, tokens, params));
}

extern "C" void luxAreaLightSource(const char *name, ...)
{
	EXTRACT_PARAMETERS(name)
	luxAreaLightSourceV(name, PASS_PARAMETERS);
}

extern "C" void luxAreaLightSourceV(const char *name, unsigned int n,
	const LuxToken tokens[], const LuxPointer params[])
{
	Context::GetActive()->AreaLightSource(name,
		ParamSet(n, name, tokens, params));
}

extern "C" void luxPortalShape(const char *name, ...)
{
	EXTRACT_PARAMETERS(name)
	luxPortalShapeV(name, PASS_PARAMETERS);
}

extern "C" void luxPortalShapeV(const char *name, unsigned int n,
	const LuxToken tokens[], const LuxPointer params[])
{
	Context::GetActive()->PortalShape(name,
		ParamSet(n, name, tokens, params));
}

extern "C" void luxShape(const char *name, ...)
{
	EXTRACT_PARAMETERS(name)
	luxShapeV(name, PASS_PARAMETERS);
}

extern "C" void luxShapeV(const char *name, unsigned int n,
	const LuxToken tokens[], const LuxPointer params[])
{
	Context::GetActive()->Shape(name, ParamSet(n, name, tokens, params));
}

extern "C" void luxRenderer(const char *name, ...)
{
	EXTRACT_PARAMETERS(name)
	luxRendererV(name, PASS_PARAMETERS);
}

extern "C" void luxRendererV(const char *name, unsigned int n,
	const LuxToken tokens[], const LuxPointer params[])
{
	Context::GetActive()->Renderer(name, ParamSet(n, name, tokens, params));
}

extern "C" void luxReverseOrientation() {
	Context::GetActive()->ReverseOrientation();
}

extern "C" void luxMakeNamedVolume(const char *id, const char *name, ...)
{
	EXTRACT_PARAMETERS(name);
	luxMakeNamedVolumeV(id, name, PASS_PARAMETERS);
}

extern "C" void luxMakeNamedVolumeV(const char *id, const char *name,
	unsigned int n, const LuxToken tokens[], const LuxPointer params[])
{
	ParamSet p(n, name, tokens, params);
	Context::GetActive()->MakeNamedVolume(id, name, p);
}
extern "C" void luxVolume(const char *name, ...)
{
	EXTRACT_PARAMETERS(name);
	luxVolumeV(name, PASS_PARAMETERS);
}

extern "C" void luxVolumeV(const char *name, unsigned int n,
	const LuxToken tokens[], const LuxPointer params[])
{
	Context::GetActive()->Volume(name, ParamSet(n, name, tokens, params));
}

extern "C" void luxExterior(const char *name)
{
	Context::GetActive()->Exterior(name);
}

extern "C" void luxInterior(const char *name)
{
	Context::GetActive()->Interior(name);
}

extern "C" void luxObjectBegin(const char *name)
{
	Context::GetActive()->ObjectBegin(string(name));
}
extern "C" void luxObjectEnd()
{
	Context::GetActive()->ObjectEnd();
}
extern "C" void luxObjectInstance(const char *name)
{
	Context::GetActive()->ObjectInstance(string(name));
}
extern "C" void luxPortalInstance(const char *name)
{
	Context::GetActive()->PortalInstance(string(name));
}
extern "C" void luxMotionInstance(const char *name, float startTime,
	float endTime, const char *toTransform)
{
	Context::GetActive()->MotionInstance(string(name), startTime,
		endTime, string(toTransform));
}
extern "C" void luxWorldEnd() {
	// initialize rand() number generator
	srand(time(NULL));
	Context::GetActive()->WorldEnd();
}

extern "C" const char *luxVersion()
{
	static const char version[] = LUX_VERSION_STRING;
	return version;
}

extern "C" void luxInit()
{
	// System-wide initialization

	// Make sure floating point unit's rounding stuff is set
	// as is expected by the fast FP-conversion routines.  In particular,
	// we want double precision on Linux, not extended precision!
#ifdef FAST_INT
#if defined(__linux__) && defined(__i386__)
	int cword = _FPU_MASK_DM | _FPU_MASK_ZM | _FPU_MASK_OM | _FPU_MASK_PM |
	_FPU_MASK_UM | _FPU_MASK_IM | _FPU_DOUBLE | _FPU_RC_NEAREST;
	_FPU_SETCW(cword);
#endif
#if defined(WIN32)
	_control87(_PC_53, MCW_PC);
#endif
#endif // FAST_INT

	// API Initialization
	if (initialized)
		{LOG(LUX_ERROR,LUX_ILLSTATE)<<"luxInit() has already been called.";}
	else
		Context::SetActive(new Context());

	FreeImage_Initialise(true);

	initialized = true;
}

bool parseFile(const char *filename) {
	//TODO jromang - add thread lock here (we can only parse in one context)
	extern FILE *yyin;
	extern int yyparse(void);
	extern void yyrestart( FILE *new_file );
	extern void include_clear();
	extern string currentFile;
	extern u_int lineNum;

	bool parse_success = false;

	if (strcmp(filename, "-") == 0)
		yyin = stdin;
	else
		yyin = fopen(filename, "r");
	if (yyin != NULL) {
		currentFile = filename;
		if (yyin == stdin)
			currentFile = "<standard input>";
		lineNum = 1;
		// make sure to flush any buffers
		// before parsing
		include_clear();
		yyrestart(yyin);
		parse_success = (yyparse() == 0);
		
		if (yyin != stdin)
			fclose(yyin);
	} else {
		LOG(LUX_SEVERE, LUX_NOFILE) << "Unable to read scenefile '" << filename << "'";
	}

	currentFile = "";
	lineNum = 0;
	return (yyin != NULL) && parse_success;
}

// Parsing Global Interface
int luxParse(const char *filename)
{
	bool parse_success = parseFile(filename);

	if (!parse_success) {
		// syntax error
		Context::GetActive()->Free();
		Context::GetActive()->Init();
		Context::GetActive()->currentApiState = STATE_PARSE_FAIL;
	} else if (Context::GetActive()->currentApiState == STATE_WORLD_BLOCK) {
		// file doesn't contain valid world block
		LOG(LUX_SEVERE, LUX_BADFILE) << "Missing WorldEnd in scenefile '" << filename << "'";
		Context::GetActive()->Free();
		Context::GetActive()->Init();
		Context::GetActive()->currentApiState = STATE_PARSE_FAIL;
		parse_success = false;
	}

	return parse_success;
}

int luxParsePartial(const char *filename)
{
	// caller does error handling
	return parseFile(filename);
}

// Load/save FLM file
extern "C" void luxLoadFLM(const char* name)
{
	Context::GetActive()->LoadFLM(string(name));
}

extern "C" void luxSaveFLM(const char* name)
{
	Context::GetActive()->SaveFLM(string(name));
}

extern "C" void luxOverrideResumeFLM(const char *name)
{
	Context::GetActive()->OverrideResumeFLM(string(name));
}

extern "C" void luxOverrideFilename(const char *name)
{
	Context::GetActive()->OverrideFilename(string(name));
}

// Write film to a floating point OpenEXR image
extern "C" void luxSaveEXR(const char* name, bool useHalfFloat, bool includeZBuffer, int compressionType, bool tonemapped)
{
	Context::GetActive()->SaveEXR(string(name), useHalfFloat, includeZBuffer, compressionType, tonemapped);
}

//interactive control

//CORE engine control

//user interactive thread functions
extern "C" void luxStart()
{
	Context::GetActive()->Resume();
}

extern "C" void luxPause()
{
	Context::GetActive()->Pause();
}

extern "C" void luxExit()
{
	Context::GetActive()->Exit();
}

extern "C" void luxAbort()
{
	Context::GetActive()->Abort();
}

extern "C" void luxWait()
{
	Context::GetActive()->Wait();
}

extern "C" void luxSetHaltSamplesPerPixel(int haltspp,
	bool haveEnoughSamplesPerPixel, bool suspendThreadsWhenDone)
{
	Context::GetActive()->SetHaltSamplesPerPixel(haltspp,
		haveEnoughSamplesPerPixel, suspendThreadsWhenDone);
}
//controlling number of threads
extern "C" unsigned int luxAddThread()
{
	return Context::GetActive()->AddThread();
}

extern "C" void luxRemoveThread()
{
	Context::GetActive()->RemoveThread();
}

//framebuffer access
extern "C" void luxUpdateFramebuffer()
{
	Context::GetActive()->UpdateFramebuffer();
}

extern "C" unsigned char* luxFramebuffer()
{
	return Context::GetActive()->Framebuffer();
}

//histogram access
extern "C" void luxGetHistogramImage(unsigned char *outPixels,
	unsigned int width, unsigned int height, int options)
{
	Context::GetActive()->GetHistogramImage(outPixels, width, height,
		options);
}

// Parameter Access functions
extern "C" void luxSetParameterValue(luxComponent comp,
	luxComponentParameters param, double value, unsigned int index)
{
	return Context::GetActive()->SetParameterValue(comp, param, value,
		index);
}
extern "C" double luxGetParameterValue(luxComponent comp,
	luxComponentParameters param, unsigned int index)
{
	return Context::GetActive()->GetParameterValue(comp, param, index);
}
extern "C" double luxGetDefaultParameterValue(luxComponent comp,
	luxComponentParameters param, unsigned int index)
{
	return Context::GetActive()->GetDefaultParameterValue(comp, param,
		index);
}

extern "C" void luxSetStringParameterValue(luxComponent comp,
	luxComponentParameters param, const char* value, unsigned int index)
{
	return Context::GetActive()->SetStringParameterValue(comp, param, value,
		index);
}
extern "C" unsigned int luxGetStringParameterValue(luxComponent comp,
	luxComponentParameters param, char* dst, unsigned int dstlen,
	unsigned int index)
{
	const string str = Context::GetActive()->GetStringParameterValue(comp,
		param, index);
	unsigned int nToCopy = str.length() < dstlen ?
		str.length() + 1 : dstlen;
	if (nToCopy > 0) {
		strncpy(dst, str.c_str(), nToCopy - 1);
		dst[nToCopy - 1] = '\0';
	}
	return str.length();
}
extern "C" unsigned int luxGetDefaultStringParameterValue(luxComponent comp,
	luxComponentParameters param, char* dst, unsigned int dstlen,
	unsigned int index)
{
	const string str = Context::GetActive()->GetDefaultStringParameterValue(comp, param, index);
	unsigned int nToCopy = str.length() < dstlen ?
		str.length() + 1 : dstlen;
	if (nToCopy > 0) {
		strncpy(dst, str.c_str(), nToCopy - 1);
		dst[nToCopy - 1] = '\0';
	}
	return str.length();
}

extern "C" double luxStatistics(const char *statName)
{
	if (initialized)
		return Context::GetActive()->Statistics(statName);
	LOG(LUX_SEVERE,LUX_NOTSTARTED)<<"luxInit() must be called before calling 'luxStatistics'. Ignoring.";
	return 0.;
}

extern "C" const char* luxPrintableStatistics(const bool add_total)
{
	if (initialized)
		return Context::GetActive()->PrintableStatistics(add_total);
	LOG(LUX_SEVERE,LUX_NOTSTARTED)<<"luxInit() must be called before calling 'luxStatistics'. Ignoring.";
	return "";
}

extern "C" const char* luxCustomStatistics(const char *template_string)
{
	if (initialized)
		return Context::GetActive()->CustomStatistics(template_string);
	LOG(LUX_SEVERE,LUX_NOTSTARTED)<<"luxInit() must be called before calling 'luxStatistics'. Ignoring.";
	return "";
}

extern "C" const char* luxGetAttributes()
{
	return Context::GetActive()->registry.GetContent();
}

extern "C" bool luxHasObject(const char * objectName)
{
	return Context::GetActive()->registry[objectName] != NULL;
}

extern "C" bool luxHasAttribute(const char * objectName, const char * attributeName)
{
	Queryable *object=Context::GetActive()->registry[objectName];
	if (object) {
		try {
			return (*object).HasAttribute(attributeName);
		} catch (std::runtime_error e) {
			LOG(LUX_ERROR,LUX_CONSISTENCY)<< e.what();
		}
	} else {
		LOG(LUX_ERROR,LUX_BADTOKEN)<<"Unknown object '"<<objectName<<"'";
	}
	return false;
}

extern "C" bool luxHasAttributeDefaultValue(const char * objectName, const char * attributeName)
{
	Queryable *object=Context::GetActive()->registry[objectName];
	if (object) {
		try {
			return (*object)[attributeName].HasDefaultValue();
		} catch (std::runtime_error e) {
			LOG(LUX_ERROR,LUX_CONSISTENCY)<< e.what();
		}
	} else {
		LOG(LUX_ERROR,LUX_BADTOKEN)<<"Unknown object '"<<objectName<<"'";
	}
	return false;
}

extern "C" const char* luxGetStringAttribute(const char * objectName, const char * attributeName)
{
	try { 
		Queryable *object=Context::GetActive()->registry[objectName];
		if (object) 
			return (*object)[attributeName].StringValue().c_str();
	} catch (std::runtime_error e) {
		LOG(LUX_ERROR,LUX_CONSISTENCY)<< e.what();
	}

	return 0;
}

extern "C" const char* luxGetStringAttributeDefault(const char * objectName, const char * attributeName)
{
	try { 
		Queryable *object=Context::GetActive()->registry[objectName];
		if (object) 
			return (*object)[attributeName].DefaultStringValue().c_str();
	} catch (std::runtime_error e) {
		LOG(LUX_ERROR,LUX_CONSISTENCY)<< e.what();
	}

	return 0;
}

extern "C" void luxSetStringAttribute(const char * objectName, const char * attributeName, const char * value)
{
	Queryable *object=Context::GetActive()->registry[objectName];
	if (object) {
		try {
			(*object)[attributeName] = std::string(value);
		} catch (std::runtime_error e) {
			LOG(LUX_ERROR,LUX_CONSISTENCY)<< e.what();
		}
	} else {
		LOG(LUX_ERROR,LUX_BADTOKEN)<<"Unknown object '"<<objectName<<"'";
	}
}

extern "C" float luxGetFloatAttribute(const char * objectName, const char * attributeName)
{
	try { 
		Queryable *object=Context::GetActive()->registry[objectName];
		if (object) 
			return (*object)[attributeName].FloatValue();
	} catch (std::runtime_error e) {
		LOG(LUX_ERROR,LUX_CONSISTENCY)<< e.what();
	}

	return 0;
}

extern "C" void luxSetFloatAttribute(const char * objectName, const char * attributeName, float value)
{
	Queryable *object=Context::GetActive()->registry[objectName];
	if (object) {
		try {
			(*object)[attributeName] = value;
		} catch (std::runtime_error e) {
			LOG(LUX_ERROR,LUX_CONSISTENCY)<< e.what();
		}
	} else {
		LOG(LUX_ERROR,LUX_BADTOKEN)<<"Unknown object '"<<objectName<<"'";
	}
}

extern "C" float luxGetFloatAttributeDefault(const char * objectName, const char * attributeName)
{
	try { 
		Queryable *object=Context::GetActive()->registry[objectName];
		if (object) 
			return (*object)[attributeName].DefaultFloatValue();
	} catch (std::runtime_error e) {
		LOG(LUX_ERROR,LUX_CONSISTENCY)<< e.what();
	}

	return 0;
}

extern "C" double luxGetDoubleAttribute(const char * objectName, const char * attributeName)
{
	try { 
		Queryable *object=Context::GetActive()->registry[objectName];
		if (object) 
			return (*object)[attributeName].DoubleValue();
	} catch (std::runtime_error e) {
		LOG(LUX_ERROR,LUX_CONSISTENCY)<< e.what();
	}

	return 0;
}

extern "C" double luxGetDoubleAttributeDefault(const char * objectName, const char * attributeName)
{
	try { 
		Queryable *object=Context::GetActive()->registry[objectName];
		if (object) 
			return (*object)[attributeName].DefaultDoubleValue();
	} catch (std::runtime_error e) {
		LOG(LUX_ERROR,LUX_CONSISTENCY)<< e.what();
	}

	return 0;
}

extern "C" int luxGetIntAttribute(const char * objectName, const char * attributeName)
{
	try { 
		Queryable *object=Context::GetActive()->registry[objectName];
		if (object) 
			return (*object)[attributeName].IntValue();
	} catch (std::runtime_error e) {
		LOG(LUX_ERROR,LUX_CONSISTENCY)<< e.what();
	}

	return 0;
}

extern "C" int luxGetIntAttributeDefault(const char * objectName, const char * attributeName)
{
	try { 
		Queryable *object=Context::GetActive()->registry[objectName];
		if (object) 
			return (*object)[attributeName].DefaultIntValue();
	} catch (std::runtime_error e) {
		LOG(LUX_ERROR,LUX_CONSISTENCY)<< e.what();
	}

	return 0;
}

extern "C" void luxSetIntAttribute(const char * objectName, const char * attributeName, int value)
{
	Queryable *object=Context::GetActive()->registry[objectName];
	if (object) {
		try {
			(*object)[attributeName] = value;
		} catch (std::runtime_error e) {
			LOG(LUX_ERROR,LUX_CONSISTENCY)<< e.what();
		}
	} else {
		LOG(LUX_ERROR,LUX_BADTOKEN)<<"Unknown object '"<<objectName<<"'";
	}
}

extern "C" bool luxGetBoolAttribute(const char * objectName, const char * attributeName)
{
	try { 
		Queryable *object=Context::GetActive()->registry[objectName];
		if (object) 
			return (*object)[attributeName].BoolValue();
	} catch (std::runtime_error e) {
		LOG(LUX_ERROR,LUX_CONSISTENCY)<< e.what();
	}

	return 0;
}

extern "C" bool luxGetBoolAttributeDefault(const char * objectName, const char * attributeName)
{
	try { 
		Queryable *object=Context::GetActive()->registry[objectName];
		if (object) 
			return (*object)[attributeName].DefaultBoolValue();
	} catch (std::runtime_error e) {
		LOG(LUX_ERROR,LUX_CONSISTENCY)<< e.what();
	}

	return 0;
}

extern "C" void luxSetBoolAttribute(const char * objectName, const char * attributeName, bool value)
{
	Queryable *object=Context::GetActive()->registry[objectName];
	if (object) {
		try {
			(*object)[attributeName] = value;
		} catch (std::runtime_error e) {
			LOG(LUX_ERROR,LUX_CONSISTENCY)<< e.what();
		}
	} else {
		LOG(LUX_ERROR,LUX_BADTOKEN)<<"Unknown object '"<<objectName<<"'";
	}
}

extern "C" void luxSetAttribute(const char * objectName, const char * attributeName, int n, void *values)
{
	Queryable *object=Context::GetActive()->registry[objectName];
	if (object) {
		QueryableAttribute &attribute=(*object)[attributeName];
		switch(attribute.Type())
		{
		case AttributeType::Bool :
			BOOST_ASSERT(n==1);
			attribute = (*((bool*)values));
			break;

		case AttributeType::Int :
			BOOST_ASSERT(n==1);
			attribute = (*((int*)values));
			break;

		case AttributeType::Float :
			BOOST_ASSERT(n==1);
			attribute = (*((float*)values));
			break;

		case AttributeType::Double :
			BOOST_ASSERT(n==1);
			attribute = (*((double*)values));
			break;

		case AttributeType::String :
			BOOST_ASSERT(n==1);
			attribute = std::string((char*)values);
			break;

		case AttributeType::None :
		default:
			LOG(LUX_ERROR,LUX_BUG)<<"Unknown attribute type for '"<<attributeName<<"' in object '"<<objectName<<"'";
		}
	}
	else {
		LOG(LUX_ERROR,LUX_BADTOKEN)<<"Unknown object '"<<objectName<<"'";
	}
}

extern "C" void luxEnableDebugMode()
{
	Context::GetActive()->EnableDebugMode();
}

extern "C" void luxDisableRandomMode()
{
	Context::GetActive()->DisableRandomMode();
}

extern "C" void luxUpdateFilmFromNetwork()
{
	Context::GetActive()->UpdateFilmFromNetwork();
}

extern "C" void luxSetNetworkServerUpdateInterval(int updateInterval)
{
	Context::GetActive()->SetNetworkServerUpdateInterval(updateInterval);
}

extern "C" int luxGetNetworkServerUpdateInterval()
{
	return Context::GetActive()->GetNetworkServerUpdateInterval();
}

//error handling
int luxLastError = LUX_NOERROR;

namespace lux
{
	int luxLogFilter = LUX_INFO; //jromang TODO : should set filter to DEBUG only if there is a 'DEBUG' compile time option, otherwise set to LUX_INFO
}

extern "C" void luxErrorFilter(int severity)
{
	lux::luxLogFilter=severity;
}


// The internal error handling function. It can be changed through the
// API and allows to perform filtering on the errors by using the
// 'LOG' macro defined in error.h
LuxErrorHandler luxError = luxErrorPrint;

extern "C" void luxErrorHandler(LuxErrorHandler handler)
{
	luxError = handler;
}

extern "C" void luxErrorAbort(int code, int severity, const char *message)
{
	luxErrorPrint(code, severity, message);
	if (severity >= LUX_ERROR)
		exit(code);
}

extern "C" void luxErrorIgnore(int code, int severity, const char *message)
{
	luxLastError = code;
}

boost::mutex stdout_mutex;

extern "C" void luxErrorPrint(int code, int severity, const char *message)
{
	boost::mutex::scoped_lock lock(stdout_mutex);

	luxLastError = code;
	cerr<<"[";
#if !defined(WIN32) || defined(__CYGWIN__) //windows does not support ANSI escape codes (but CYGWIN does) ...
	//set the color
	switch (severity) {
	case LUX_DEBUG:
		cerr<<"\033[0;34m";		// BLUE
		break;
	case LUX_INFO:
		cerr<<"\033[0;32m";		// GREEN
		break;
	case LUX_WARNING:
		cerr<<"\033[0;33m";		// YELLOW
		break;
	case LUX_ERROR:
		cerr<<"\033[0;31m";		// RED
		break;
	case LUX_SEVERE:
		cerr<<"\033[0;31m";		// RED
		break;
	}
#else // ... but it does have it's own console API
	switch (severity) {
	case LUX_DEBUG:
		w32util::ChangeConsoleColor( FOREGROUND_BLUE );
		break;
	case LUX_INFO:
		w32util::ChangeConsoleColor( FOREGROUND_GREEN );
		break;
	case LUX_WARNING:
		w32util::ChangeConsoleColor( FOREGROUND_YELLOW );
		break;
	case LUX_ERROR:
		w32util::ChangeConsoleColor( FOREGROUND_RED );
		break;
	case LUX_SEVERE:
		w32util::ChangeConsoleColor( FOREGROUND_RED );
		break;
	}
#endif
	cerr<<"Lux ";
	cerr<<boost::posix_time::second_clock::local_time()<<' ';
	switch (severity) {
	case LUX_DEBUG:
		cerr<<"DEBUG";
		break;
	case LUX_INFO:
		cerr<<"INFO";
		break;
	case LUX_WARNING:
		cerr<<"WARNING";
		break;
	case LUX_ERROR:
		cerr<<"ERROR";
		break;
	case LUX_SEVERE:
		cerr<<"SEVERE ERROR";
		break;
	}
	cerr<<" : "<<code;
#if !defined(WIN32) || defined(__CYGWIN__) // windows does not support ANSI escape codes (but CYGWIN does) ...
	cerr<<"\033[0m";
#else // ... but it does have it's own console API
	w32util::ChangeConsoleColor( FOREGROUND_WHITE );
#endif
	cerr<<"] "<<message<<endl;
}

extern "C" void luxSetEpsilon(const float minValue, const float maxValue)
{
	Context::GetActive()->SetEpsilon(minValue < 0.f ? DEFAULT_EPSILON_MIN : minValue, maxValue < 0.f ? DEFAULT_EPSILON_MAX : maxValue);
}
