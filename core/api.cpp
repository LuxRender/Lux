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

// api.cpp*

#include "api.h"
#include "context.h"
#include "paramset.h"

#include <boost/date_time/posix_time/posix_time.hpp>
#include <vector>
#include <cstdarg>

using namespace lux;

#define	EXTRACT_PARAMETERS(_start) \
	va_list	pArgs; \
	va_start( pArgs, _start ); \
\
	std::vector<LuxToken> aTokens; \
	std::vector<LuxPointer> aValues; \
	int count = buildParameterList( pArgs, aTokens, aValues );

#define PASS_PARAMETERS \
	count, aTokens.size()>0?&aTokens[0]:0, aValues.size()>0?&aValues[0]:0

namespace lux
{

//----------------------------------------------------------------------
// BuildParameterList
// Helper function to build a parameter list to pass on to the V style functions.
// returns a parameter count.

int buildParameterList( va_list pArgs, std::vector<LuxToken>& aTokens, std::vector<LuxPointer>& aValues )
{
    int count = 0;
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
	Context::luxAddServer(std::string(name));
}

extern "C" int luxGetRenderingServersStatus(RenderingServerInfo *info, int maxInfoCount) {
	return Context::luxGetRenderingServersStatus(info, maxInfoCount);
}

extern "C" void luxCleanup()
{
	Context::luxCleanup();
}

extern "C" void luxIdentity()
{
	Context::luxIdentity();
}
extern "C" void luxTranslate(float dx, float dy, float dz)
{
	Context::luxTranslate(dx,dy,dz);
}
extern "C" void luxTransform(float tr[16])
{
	Context::luxTransform(tr);
}
extern "C" void luxConcatTransform(float tr[16]) {
	Context::luxConcatTransform(tr);
}
extern "C" void luxRotate(float angle, float dx, float dy, float dz)
{
	Context::luxRotate(angle,dx,dy,dz);
}
extern "C" void luxScale(float sx, float sy, float sz)
{
	Context::luxScale(sx,sy,sz);
}
extern "C" void luxLookAt(float ex, float ey, float ez, float lx, float ly, float lz,
		float ux, float uy, float uz)
{
	Context::luxLookAt(ex, ey, ez, lx, ly, lz, ux, uy, uz);
}
extern "C" void luxCoordinateSystem(const char *name)
{
	Context::luxCoordinateSystem(std::string(name));
}
extern "C" void luxCoordSysTransform(const char *name)
{
	Context::luxCoordSysTransform(std::string(name));
}
extern "C" void luxPixelFilter(const char *name, ...) //
{
	EXTRACT_PARAMETERS( name )
	luxPixelFilterV( name, PASS_PARAMETERS );		
}

extern "C" void luxPixelFilterV (const char *name, int n, LuxToken tokens[], LuxPointer params[])
{
	//TODO - jromang : construct the paramset
	//Context::luxpixelFilter(name,params);
	Context::luxPixelFilter(name,ParamSet(n,name,tokens,params));
}

/*
void luxFilm(const char *type, const ParamSet &params)
{
	Context::luxFilm(std::string(type),params);
}*/
extern "C" void luxFilm(const char *name, ...)
{
	EXTRACT_PARAMETERS( name )
	luxFilmV( name, PASS_PARAMETERS );
}

extern "C" void luxFilmV(const char *name, int n, LuxToken tokens[], LuxPointer params[])
{
	Context::luxFilm(name,ParamSet(n,name,tokens,params));
}


/*
void luxSampler(const char *name, const ParamSet &params)
{
	Context::luxSampler(std::string(name),params);
}*/

extern "C" void luxSampler(const char *name, ...)
{
	EXTRACT_PARAMETERS( name )
	luxSamplerV( name, PASS_PARAMETERS );
}

extern "C" void luxSamplerV(const char *name, int n, LuxToken tokens[], LuxPointer params[])
{
	Context::luxSampler(name,ParamSet(n,name,tokens,params));
}

/*
void luxAccelerator(const char *name, const ParamSet &params)
{
	Context::luxAccelerator(std::string(name),params);
}*/
extern "C" void luxAccelerator(const char *name, ...)
{
	EXTRACT_PARAMETERS( name )
	luxAcceleratorV( name, PASS_PARAMETERS );
}

extern "C" void luxAcceleratorV(const char *name, int n, LuxToken tokens[], LuxPointer params[])
{
	Context::luxAccelerator(name,ParamSet(n,name,tokens,params));
}
/*
void luxSurfaceIntegrator(const char *name, const ParamSet &params)
{
	Context::luxSurfaceIntegrator(std::string(name),params);
}*/
extern "C" void luxSurfaceIntegrator(const char *name, ...)
{
	EXTRACT_PARAMETERS( name )
	luxSurfaceIntegratorV( name, PASS_PARAMETERS );
}

extern "C" void luxSurfaceIntegratorV(const char *name, int n, LuxToken tokens[], LuxPointer params[])
{
	Context::luxSurfaceIntegrator(name,ParamSet(n,name,tokens,params));
}

/*
void luxVolumeIntegrator(const char *name, const ParamSet &params)
{
	Context::luxVolumeIntegrator(std::string(name),params);
}*/

extern "C" void luxVolumeIntegrator(const char *name, ...)
{
	EXTRACT_PARAMETERS( name )
	luxVolumeIntegratorV( name, PASS_PARAMETERS );
}

extern "C" void luxVolumeIntegratorV(const char *name, int n, LuxToken tokens[], LuxPointer params[])
{
	Context::luxVolumeIntegrator(name,ParamSet(n,name,tokens,params));
}

/*
void luxCamera(const char *name, const ParamSet &params)
{
	Context::luxCamera(std::string(name),params);
}*/

extern "C" void luxCamera(const char *name, ...)
{
	EXTRACT_PARAMETERS( name )
	luxCameraV( name, PASS_PARAMETERS );
}

extern "C" void luxCameraV(const char *name, int n, LuxToken tokens[], LuxPointer params[])
{
	Context::luxCamera(name,ParamSet(n,name,tokens,params));
}

extern "C" void luxWorldBegin()
{
	Context::luxWorldBegin();
}
extern "C" void luxAttributeBegin()
{
	Context::luxAttributeBegin();
}
extern "C" void luxAttributeEnd()
{
	Context::luxAttributeEnd();
}
extern "C" void luxTransformBegin()
{
	Context::luxTransformBegin();
}
extern "C" void luxTransformEnd()
{
	Context::luxTransformEnd();
}
/*
void luxTexture(const char *name, const char *type, const char *texname,
		const ParamSet &params)
{	
	Context::luxTexture(std::string(name), std::string(type), std::string(texname), params);
}*/
extern "C" void luxTexture(const char *name, const char *type, const char *texname, ...)
{
	EXTRACT_PARAMETERS( texname )
	luxTextureV( name, type, texname, PASS_PARAMETERS );
}

extern "C" void luxTextureV(const char *name, const char *type, const char *texname, int n, LuxToken tokens[], LuxPointer params[])
{
	Context::luxTexture(name,type,texname,ParamSet(n,name,tokens,params));
}
/*
void luxMaterial(const char *name, const ParamSet &params)
{
	Context::luxMaterial(name,params);
}*/
extern "C" void luxMaterial(const char *name, ...)
{
	EXTRACT_PARAMETERS( name )
	luxMaterialV( name, PASS_PARAMETERS );
}

extern "C" void luxMaterialV(const char *name, int n, LuxToken tokens[], LuxPointer params[])
{
	Context::luxMaterial(name,ParamSet(n,name,tokens,params));
}

extern "C" void luxMakeNamedMaterial(const char *name, ...)
{
	EXTRACT_PARAMETERS( name )
	luxMakeNamedMaterialV( name, PASS_PARAMETERS );
}

extern "C" void luxMakeNamedMaterialV(const char *name, int n, LuxToken tokens[], LuxPointer params[])
{
	Context::luxMakeNamedMaterial(name,ParamSet(n,name,tokens,params));
}

extern "C" void luxNamedMaterial(const char *name, ...)
{
	EXTRACT_PARAMETERS( name )
	luxNamedMaterialV( name, PASS_PARAMETERS );
}

extern "C" void luxNamedMaterialV(const char *name, int n, LuxToken tokens[], LuxPointer params[])
{
	Context::luxNamedMaterial(name,ParamSet(n,name,tokens,params));
}
/*
void luxLightSource(const char *name, const ParamSet &params)
{
	Context::luxLightSource(std::string(name),params);
}*/
extern "C" void luxLightSource(const char *name, ...)
{
	EXTRACT_PARAMETERS( name )
	luxLightSourceV( name, PASS_PARAMETERS );
}

extern "C" void luxLightSourceV(const char *name, int n, LuxToken tokens[], LuxPointer params[])
{
	Context::luxLightSource(name,ParamSet(n,name,tokens,params));
}

/*
void luxAreaLightSource(const char *name, const ParamSet &params) {
	Context::luxAreaLightSource(std::string(name),params);
}*/
extern "C" void luxAreaLightSource(const char *name, ...)
{
	EXTRACT_PARAMETERS( name )
	luxAreaLightSourceV( name, PASS_PARAMETERS );
}

extern "C" void luxAreaLightSourceV(const char *name, int n, LuxToken tokens[], LuxPointer params[])
{
	Context::luxAreaLightSource(name,ParamSet(n,name,tokens,params));
}

/*
void luxPortalShape(const char *name, const ParamSet &params) {
	Context::luxPortalShape(std::string(name),params);
}*/
extern "C" void luxPortalShape(const char *name, ...)
{
	EXTRACT_PARAMETERS( name )
	luxPortalShapeV( name, PASS_PARAMETERS );
}

extern "C" void luxPortalShapeV(const char *name, int n, LuxToken tokens[], LuxPointer params[])
{
	Context::luxPortalShape(name,ParamSet(n,name,tokens,params));
}

/*
void luxShape(const char *name, const ParamSet &params) {
	Context::luxShape(std::string(name),params);
}*/
extern "C" void luxShape(const char *name, ...)
{
	EXTRACT_PARAMETERS( name )
	luxShapeV( name, PASS_PARAMETERS );
}

extern "C" void luxShapeV(const char *name, int n, LuxToken tokens[], LuxPointer params[])
{
	Context::luxShape(name,ParamSet(n,name,tokens,params));
}

extern "C" void luxReverseOrientation() {
	Context::luxReverseOrientation();
}
/*
void luxVolume(const char *name, const ParamSet &params) {
	Context::luxVolume(std::string(name),params);
}*/
extern "C" void luxVolume(const char *name, ...)
{
	EXTRACT_PARAMETERS( name )
	luxVolumeV( name, PASS_PARAMETERS );
}

extern "C" void luxVolumeV(const char *name, int n, LuxToken tokens[], LuxPointer params[])
{
	Context::luxVolume(name,ParamSet(n,name,tokens,params));
}

extern "C" void luxObjectBegin(const char *name) {
	Context::luxObjectBegin(std::string(name));
}
extern "C" void luxObjectEnd() {
	Context::luxObjectEnd();
}
extern "C" void luxObjectInstance(const char *name) {
	Context::luxObjectInstance(std::string(name));
}
extern "C" void luxWorldEnd() {
	Context::luxWorldEnd();
}


extern "C" void luxInit() {

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
		luxError(LUX_ILLSTATE,LUX_ERROR,"luxInit() has already been called.");
	else
		Context::setActive(new Context());
	
	initialized=true;
}

//interactive control

//CORE engine control


//user interactive thread functions
extern "C" void luxStart() {
	Context::luxStart();
}

extern "C" void luxPause() {
	Context::luxPause();
}

extern "C" void luxExit() {
	Context::luxExit();
}

extern "C" void luxWait() {
	Context::luxWait();
}

extern "C" void luxSetHaltSamplePerPixel(int haltspp, bool haveEnoughSamplePerPixel,
		bool suspendThreadsWhenDone) {
	Context::luxSetHaltSamplePerPixel(haltspp, haveEnoughSamplePerPixel, suspendThreadsWhenDone);
}
//controlling number of threads
extern "C" int luxAddThread() {
	return Context::luxAddThread();
	//return luxCurrentScene->AddThread();
}

extern "C" void luxRemoveThread() {
	Context::luxRemoveThread();
	//luxCurrentScene->RemoveThread();
}

extern "C" int luxGetRenderingThreadsStatus(RenderingThreadInfo *info, int maxInfoCount) {
	return Context::luxGetRenderingThreadsStatus(info, maxInfoCount);
}

//framebuffer access
extern "C" void luxUpdateFramebuffer() {
	Context::luxUpdateFramebuffer();
	//luxCurrentScene->UpdateFramebuffer();
}

extern "C" unsigned char* luxFramebuffer() {
	return Context::luxFramebuffer();
	//return luxCurrentScene->GetFramebuffer();
}

/*
//TODO - jromang : remove & replace by luxstatistics
extern "C" int luxDisplayInterval() {
	return Context::luxDisplayInterval();
	//return luxCurrentScene->DisplayInterval();
}


//TODO - jromang : remove & replace by luxstatistics
extern "C" int luxFilmXres() {
	return Context::luxFilmXres();
	//return luxCurrentScene->FilmXres();
}

//TODO - jromang : remove & replace by luxstatistics
extern "C" int luxFilmYres() {
	return Context::luxFilmYres();
	//return luxCurrentScene->FilmYres();
}*/

extern "C" double luxStatistics(const char *statName) {
	if(initialized) return Context::luxStatistics(statName);
	else 
		{
			luxError(LUX_NOTSTARTED,LUX_SEVERE,"luxInit() must be called before calling  'luxStatistics'. Ignoring.");
			return 0;
		}
}

extern "C" void luxEnableDebugMode() {	
	Context::luxEnableDebugMode();
}

extern "C" void luxUpdateFilmFromNetwork() {
	
	Context::luxUpdateFilmFromNetwork();
}

extern "C" void luxSetNetworkServerUpdateInterval(int updateInterval) {
	
	Context::luxSetNetworkServerUpdateInterval(updateInterval);
}

extern "C" int luxGetNetworkServerUpdateInterval() {
	
	return Context::luxGetNetworkServerUpdateInterval();
}

//error handling
LuxErrorHandler luxError=luxErrorPrint;
int luxLastError=LUX_NOERROR;

extern "C" void luxErrorHandler(LuxErrorHandler handler) {
	luxError=handler;
}

extern "C" void luxErrorAbort(int code, int severity, const char *message) {
	luxErrorPrint(code, severity, message);
	if (severity!=LUX_INFO)
		exit(code);
}

extern "C" void luxErrorIgnore(int code, int severity, const char *message) {
	luxLastError=code;
}

extern "C" void luxErrorPrint(int code, int severity, const char *message) {
	luxLastError=code;
	std::cerr<<"[";
#ifndef WIN32 //windows does not support ANSI escape codes
	//set the color
	switch (severity) {
	case LUX_INFO:
		std::cerr<<"\033[0;32m";
		break;
	case LUX_WARNING:
		std::cerr<<"\033[0;33m";
		break;
	case LUX_ERROR:
		std::cerr<<"\033[0;31m";
		break;
	case LUX_SEVERE:
		std::cerr<<"\033[0;31m";
		break;
	}
#endif
	std::cerr<<"Lux ";
	std::cerr<<boost::posix_time::second_clock::local_time()<<' ';
	switch (severity) {
	case LUX_INFO:
		std::cerr<<"INFO";
		break;
	case LUX_WARNING:
		std::cerr<<"WARNING";
		break;
	case LUX_ERROR:
		std::cerr<<"ERROR";
		break;
	case LUX_SEVERE:
		std::cerr<<"SEVERE ERROR";
		break;
	}
	std::cerr<<" : "<<code;
#ifndef WIN32 //windows does not support ANSI escape codes
	std::cerr<<"\033[0m";
#endif
	std::cerr<<"] "<<message<<std::endl;
}

