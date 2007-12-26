/***************************************************************************
 *   Copyright (C) 1998-2007 by authors (see AUTHORS.txt )                 *
 *                                                                         *
 *   This file is part of Lux Renderer.                                    *
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
 *   Lux Renderer website : http://www.luxrender.org                       *
 ***************************************************************************/

// api.cpp*

#include "api.h"
#include "paramset.h"
#include "color.h"
#include "scene.h"
#include "film.h"
#include "dynload.h"
#include "volume.h"
#include "../film/multiimage.h"
#include "error.h"
#include "renderfarm.h"
#include <iostream>
#include <fstream>
#include <vector>
#include <string>
#include <sstream>
#include <map>
#include <boost/date_time/posix_time/posix_time.hpp>
using std::map;
#if (_MSC_VER >= 1400) // NOBOOK
#include <stdio.h>     // NOBOOK
#define snprintf _snprintf // NOBOOK
#endif // NOBOOK
#include "../renderer/include/asio.hpp"
#include <boost/archive/text_oarchive.hpp>
#include <boost/archive/text_iarchive.hpp>
//#include <boost/archive/xml_oarchive.hpp>
//#include <boost/archive/xml_iarchive.hpp>
#include <boost/serialization/split_member.hpp>
#include <boost/serialization/vector.hpp>
#include <boost/serialization/string.hpp>


#include <boost/iostreams/filtering_stream.hpp>
#include <boost/iostreams/filtering_streambuf.hpp>
#include <boost/iostreams/copy.hpp>
#include <boost/iostreams/filter/zlib.hpp>

using asio::ip::tcp;
using namespace boost::iostreams;
using namespace lux;

#include "context.h"


static bool initialized=false;


// API Function Definitions

void luxAddServer(const char * name)
{
	Context::getActive()->addServer(std::string(name));
}

void luxCleanup()
{
	Context::getActive()->cleanup();
}

void luxIdentity()
{
	Context::getActive()->identity();
}
void luxTranslate(float dx, float dy, float dz)
{
	Context::getActive()->translate(dx,dy,dz);
}
void luxTransform(float tr[16])
{
	Context::getActive()->transform(tr);
}
void luxConcatTransform(float tr[16]) {
	Context::getActive()->concatTransform(tr);
}
void luxRotate(float angle, float dx, float dy, float dz)
{
	Context::getActive()->rotate(angle,dx,dy,dz);
}
void luxScale(float sx, float sy, float sz)
{
	Context::getActive()->scale(sx,sy,sz);
}
void luxLookAt(float ex, float ey, float ez, float lx, float ly, float lz,
		float ux, float uy, float uz)
{
	Context::getActive()->lookAt(ex, ey, ez, lx, ly, lz, ux, uy, uz);
}
void luxCoordinateSystem(const string &name)
{
	Context::getActive()->coordinateSystem(name);
}
void luxCoordSysTransform(const string &name)
{
	Context::getActive()->coordSysTransform(name);
}
void luxPixelFilter(const string &name, const ParamSet &params)
{
	Context::getActive()->pixelFilter(name,params);
}
void luxFilm(const string &type, const ParamSet &params)
{
	Context::getActive()->film(type,params);
}
void luxSampler(const string &name, const ParamSet &params)
{
	Context::getActive()->sampler(name,params);
}
void luxAccelerator(const string &name, const ParamSet &params)
{
	Context::getActive()->accelerator(name,params);
}
void luxSurfaceIntegrator(const string &name, const ParamSet &params)
{
	Context::getActive()->surfaceIntegrator(name,params);
}
void luxVolumeIntegrator(const string &name, const ParamSet &params)
{
	Context::getActive()->volumeIntegrator(name,params);
}
void luxCamera(const string &name, const ParamSet &params)
{
	Context::getActive()->camera(name,params);
}
void luxWorldBegin()
{
	Context::getActive()->worldBegin();
}
void luxAttributeBegin()
{
	Context::getActive()->attributeBegin();
}
void luxAttributeEnd()
{
	Context::getActive()->attributeEnd();
}
void luxTransformBegin()
{
	Context::getActive()->transformBegin();
}
void luxTransformEnd()
{
	Context::getActive()->transformEnd();
}
void luxTexture(const string &name, const string &type, const string &texname,
		const ParamSet &params)
{	
	Context::getActive()->texture(name, type, texname, params);
}
void luxMaterial(const string &name, const ParamSet &params)
{
	Context::getActive()->material(name,params);
}
void luxLightSource(const string &name, const ParamSet &params)
{
	Context::getActive()->lightSource(name,params);
}

void luxAreaLightSource(const string &name, const ParamSet &params) {
	Context::getActive()->areaLightSource(name,params);
}

void luxPortalShape(const string &name, const ParamSet &params) {
	Context::getActive()->portalShape(name,params);
}

void luxShape(const string &name, const ParamSet &params) {
	Context::getActive()->shape(name,params);
}
void luxReverseOrientation() {
	Context::getActive()->reverseOrientation();
}
void luxVolume(const string &name, const ParamSet &params) {
	Context::getActive()->volume(name,params);
}
void luxObjectBegin(const string &name) {
	Context::getActive()->objectBegin(name);
}
void luxObjectEnd() {
	Context::getActive()->objectEnd();
}
void luxObjectInstance(const string &name) {
	Context::getActive()->objectInstance(name);
}
void luxWorldEnd() {
	Context::getActive()->worldEnd();
}


void luxInit() {

	// System-wide initialization
	//random number init
	lux::random::init();

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
void luxStart() {
	Context::getActive()->start();
	//luxCurrentScene->Start();
}

void luxPause() {
	Context::getActive()->pause();
	//luxCurrentScene->Pause();
}

void luxExit() {
	Context::getActive()->exit();
	//luxCurrentScene->Exit();
}

//controlling number of threads
int luxAddThread() {
	return Context::getActive()->addThread();
	//return luxCurrentScene->AddThread();
}

void luxRemoveThread() {
	Context::getActive()->removeThread();
	//luxCurrentScene->RemoveThread();
}

//framebuffer access
void luxUpdateFramebuffer() {
	Context::getActive()->updateFramebuffer();
	//luxCurrentScene->UpdateFramebuffer();
}

unsigned char* luxFramebuffer() {
	return Context::getActive()->framebuffer();
	//return luxCurrentScene->GetFramebuffer();
}


//TODO - jromang : remove & replace by luxstatistics
int luxDisplayInterval() {
	return Context::getActive()->displayInterval();
	//return luxCurrentScene->DisplayInterval();
}

//TODO - jromang : remove & replace by luxstatistics
int luxFilmXres() {
	return Context::getActive()->filmXres();
	//return luxCurrentScene->FilmXres();
}

//TODO - jromang : remove & replace by luxstatistics
int luxFilmYres() {
	return Context::getActive()->filmYres();
	//return luxCurrentScene->FilmYres();
}

double luxStatistics(char *statName) {
	if(initialized) return Context::getActive()->statistics(statName);
	else 
		{
			luxError(LUX_NOTSTARTED,LUX_SEVERE,"luxInit() must be called before calling  'luxStatistics'. Ignoring.");
			return 0;
		}
}


void luxUpdateFilmFromNetwork() {
	
	Context::getActive()->updateFilmFromNetwork();
}





//error handling
LuxErrorHandler luxError=luxErrorPrint;
int luxLastError=LUX_NOERROR;

void luxErrorHandler(LuxErrorHandler handler) {
	luxError=handler;
}

void luxErrorAbort(int code, int severity, const char *message) {
	luxErrorPrint(code, severity, message);
	if (severity!=LUX_INFO)
		exit(code);
}

void luxErrorIgnore(int code, int severity, const char *message) {
	luxLastError=code;
}

void luxErrorPrint(int code, int severity, const char *message) {
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

