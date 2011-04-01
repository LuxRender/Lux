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

// This file wraps the lux::Context and lux::ParamSet classes, which is enough
// to provide a complete interface to rendering.

#include <iostream>
#include <string>

#include <boost/assert.hpp>
#include <boost/foreach.hpp>
#include <boost/pool/pool.hpp>
#include <boost/thread.hpp>

// Lux headers
#include "api.h"
#include "color.h"

// CPP API headers
#include "lux_api.h"

// -----------------------------------------------------------------------
// CONTEXT WRAPPING
// -----------------------------------------------------------------------

#define UPCAST_PARAMSET *((lux_wrapped_paramset*)params)->GetParamSet()

// Initialise the Lux Core only once, when the first lux_wrapped_context is constructed
boost::once_flag luxInitFlag = BOOST_ONCE_INIT;

// The wrapper needs to hold this mutex for every context method call
// since the LuxRender core is fond is using a single Active Context
// for certain things
boost::mutex ctxMutex;

// This class is going to wrap lux::Context
lux_wrapped_context::lux_wrapped_context(const char* _name) : name(_name)
{
	luxErrorFilter(LUX_DEBUG);
	boost::call_once(&luxInit, luxInitFlag);
	ctx = new lux::Context(_name);
}
lux_wrapped_context::~lux_wrapped_context()
{
	BOOST_FOREACH(boost::thread* t, render_threads)
	{
		delete(t);
	}
	render_threads.clear();

	if (ctx)
	{
		delete(ctx);
		ctx = NULL;
	}
}

// context metadata
const char* lux_wrapped_context::version()
{
	return luxVersion();
}
const char* lux_wrapped_context::getName()
{
	return name;
}

// file parsing methods
bool lux_wrapped_context::parse(const char* filename, bool async)
{
	boost::mutex::scoped_lock lock(ctxMutex);
	checkContext();
	if (async)
	{
		render_threads.push_back(new boost::thread( boost::bind(luxParse, filename) ));
		return true;	// Real parse status can be checked later with parseSuccess()
	}
	else
	{
		return luxParse(filename);
	}
}
bool lux_wrapped_context::parsePartial(const char* filename, bool async)
{
	boost::mutex::scoped_lock lock(ctxMutex);
	checkContext();
	if (async)
	{
		render_threads.push_back(new boost::thread( boost::bind(luxParsePartial, filename) ));
		return true;	// Real parse status can be checked later with parseSuccess()
	}
	else
	{
		return luxParsePartial(filename);
	}
}
bool lux_wrapped_context::parseSuccessful()
{
	boost::mutex::scoped_lock lock(ctxMutex);
	checkContext();
	return ctx->currentApiState != STATE_PARSE_FAIL;
}

// rendering control
void lux_wrapped_context::pause()
{
	boost::mutex::scoped_lock lock(ctxMutex);
	checkContext();
	ctx->Pause();
}
void lux_wrapped_context::start()
{
	boost::mutex::scoped_lock lock(ctxMutex);
	checkContext();
	ctx->Resume();
}
void lux_wrapped_context::setHaltSamplesPerPixel(int haltspp, bool haveEnoughSamplesPerPixel, bool suspendThreadsWhenDone)
{
	boost::mutex::scoped_lock lock(ctxMutex);
	checkContext();
	ctx->SetHaltSamplesPerPixel(haltspp, haveEnoughSamplesPerPixel, suspendThreadsWhenDone);
}
unsigned int lux_wrapped_context::addThread()
{
	boost::mutex::scoped_lock lock(ctxMutex);
	checkContext();
	return ctx->AddThread();
}
void lux_wrapped_context::removeThread()
{
	boost::mutex::scoped_lock lock(ctxMutex);
	checkContext();
	ctx->RemoveThread();
}
void lux_wrapped_context::abort()
{
	boost::mutex::scoped_lock lock(ctxMutex);
	checkContext();
	ctx->Abort();
}
void lux_wrapped_context::wait()
{
	boost::mutex::scoped_lock lock(ctxMutex);
	checkContext();
	ctx->Wait();
}
void lux_wrapped_context::exit()
{
	boost::mutex::scoped_lock lock(ctxMutex);
	checkContext();
	ctx->Exit();
}
void lux_wrapped_context::cleanup()
{
	boost::mutex::scoped_lock lock(ctxMutex);
	checkContext();
	ctx->Cleanup();

	// Ensure the context memory is freed. Any API calls
	// after this will create a new Context with the same name.
	delete(ctx);
	ctx = NULL;
}

// scene description methods
void lux_wrapped_context::accelerator(const char *aName, const lux_paramset* params)
{
	boost::mutex::scoped_lock lock(ctxMutex);
	checkContext();
	ctx->Accelerator(aName, UPCAST_PARAMSET);
}
void lux_wrapped_context::areaLightSource(const char *aName, const lux_paramset* params)
{
	boost::mutex::scoped_lock lock(ctxMutex);
	checkContext();
	ctx->AreaLightSource(aName, UPCAST_PARAMSET);
}
void lux_wrapped_context::attributeBegin()
{
	boost::mutex::scoped_lock lock(ctxMutex);
	checkContext();
	ctx->AttributeBegin();
}
void lux_wrapped_context::attributeEnd()
{
	boost::mutex::scoped_lock lock(ctxMutex);
	checkContext();
	ctx->AttributeEnd();
}
void lux_wrapped_context::camera(const char *cName, const lux_paramset* params)
{
	boost::mutex::scoped_lock lock(ctxMutex);
	checkContext();
	ctx->Camera(cName, UPCAST_PARAMSET);
}
void lux_wrapped_context::concatTransform(float tx[16])
{
	boost::mutex::scoped_lock lock(ctxMutex);
	checkContext();
	ctx->ConcatTransform(tx);
}
void lux_wrapped_context::coordinateSystem(const char *cnName)
{
	boost::mutex::scoped_lock lock(ctxMutex);
	checkContext();
	ctx->CoordinateSystem(cnName);
}
void lux_wrapped_context::coordSysTransform(const char *cnName)
{
	boost::mutex::scoped_lock lock(ctxMutex);
	checkContext();
	ctx->CoordSysTransform(cnName);
}
void lux_wrapped_context::exterior(const char *eName)
{
	boost::mutex::scoped_lock lock(ctxMutex);
	checkContext();
	ctx->Exterior(eName);
}
void lux_wrapped_context::film(const char *fName, const lux_paramset* params)
{
	boost::mutex::scoped_lock lock(ctxMutex);
	checkContext();
	ctx->Film(fName, UPCAST_PARAMSET);
}
void lux_wrapped_context::identity()
{
	boost::mutex::scoped_lock lock(ctxMutex);
	checkContext();
	ctx->Identity();
}
void lux_wrapped_context::interior(const char *iName)
{
	boost::mutex::scoped_lock lock(ctxMutex);
	checkContext();
	ctx->Interior(iName);
}
void lux_wrapped_context::lightGroup(const char *lName, const lux_paramset* params)
{
	boost::mutex::scoped_lock lock(ctxMutex);
	checkContext();
	ctx->LightGroup(lName, UPCAST_PARAMSET);
}
void lux_wrapped_context::lightSource(const char *lName, const lux_paramset* params)
{
	boost::mutex::scoped_lock lock(ctxMutex);
	checkContext();
	ctx->LightSource(lName, UPCAST_PARAMSET);
}
void lux_wrapped_context::lookAt(float ex, float ey, float ez, float lx, float ly, float lz, float ux, float uy, float uz)
{
	boost::mutex::scoped_lock lock(ctxMutex);
	checkContext();
	ctx->LookAt(ex,ey,ez, lx,ly,lx, ux,uy,uz);
}
void lux_wrapped_context::makeNamedMaterial(const char *mName, const lux_paramset* params)
{
	boost::mutex::scoped_lock lock(ctxMutex);
	checkContext();
	ctx->MakeNamedMaterial(mName, UPCAST_PARAMSET);
}
void lux_wrapped_context::makeNamedVolume(const char *vName, const char *vType, const lux_paramset* params)
{
	boost::mutex::scoped_lock lock(ctxMutex);
	checkContext();
	ctx->MakeNamedVolume(vName, vType, UPCAST_PARAMSET);
}
void lux_wrapped_context::material(const char *mName, const lux_paramset* params)
{
	boost::mutex::scoped_lock lock(ctxMutex);
	checkContext();
	ctx->Material(mName, UPCAST_PARAMSET);
}
void lux_wrapped_context::motionInstance(const char *mName, float startTime, float endTime, const char *toTransform)
{
	boost::mutex::scoped_lock lock(ctxMutex);
	checkContext();
	ctx->MotionInstance(mName, startTime, endTime, toTransform);
}
void lux_wrapped_context::namedMaterial(const char *mName)
{
	boost::mutex::scoped_lock lock(ctxMutex);
	checkContext();
	ctx->NamedMaterial(mName);
}
void lux_wrapped_context::objectBegin(const char *oName)
{
	boost::mutex::scoped_lock lock(ctxMutex);
	checkContext();
	ctx->ObjectBegin(oName);
}
void lux_wrapped_context::objectEnd()
{
	boost::mutex::scoped_lock lock(ctxMutex);
	checkContext();
	ctx->ObjectEnd();
}
void lux_wrapped_context::objectInstance(const char *oName)
{
	boost::mutex::scoped_lock lock(ctxMutex);
	checkContext();
	ctx->ObjectInstance(oName);
}
void lux_wrapped_context::pixelFilter(const char *pName, const lux_paramset* params)
{
	boost::mutex::scoped_lock lock(ctxMutex);
	checkContext();
	ctx->PixelFilter(pName, UPCAST_PARAMSET);
}
void lux_wrapped_context::portalInstance(const char *pName)
{
	boost::mutex::scoped_lock lock(ctxMutex);
	checkContext();
	ctx->PortalInstance(pName);
}
void lux_wrapped_context::portalShape(const char *pName, const lux_paramset* params)
{
	boost::mutex::scoped_lock lock(ctxMutex);
	checkContext();
	ctx->PortalShape(pName, UPCAST_PARAMSET);
}
void lux_wrapped_context::renderer(const char *rName, const lux_paramset* params)
{
	boost::mutex::scoped_lock lock(ctxMutex);
	checkContext();
	ctx->Renderer(rName, UPCAST_PARAMSET);
}
void lux_wrapped_context::reverseOrientation()
{
	boost::mutex::scoped_lock lock(ctxMutex);
	checkContext();
	ctx->ReverseOrientation();
}
void lux_wrapped_context::rotate(float angle, float ax, float ay, float az)
{
	boost::mutex::scoped_lock lock(ctxMutex);
	checkContext();
	ctx->Rotate(angle, ax, ay, az);
}
void lux_wrapped_context::sampler(const char *sName, const lux_paramset* params)
{
	boost::mutex::scoped_lock lock(ctxMutex);
	checkContext();
	ctx->Sampler(sName, UPCAST_PARAMSET);
}
void lux_wrapped_context::scale(float sx, float sy, float sz)
{
	boost::mutex::scoped_lock lock(ctxMutex);
	checkContext();
	ctx->Scale(sx, sy, sz);
}
void lux_wrapped_context::shape(const char *sName, const lux_paramset* params)
{
	boost::mutex::scoped_lock lock(ctxMutex);
	checkContext();
	ctx->Shape(sName, UPCAST_PARAMSET);
}
void lux_wrapped_context::surfaceIntegrator(const char *sName, const lux_paramset* params)
{
	boost::mutex::scoped_lock lock(ctxMutex);
	checkContext();
	ctx->SurfaceIntegrator(sName, UPCAST_PARAMSET);
}
void lux_wrapped_context::texture(const char *tName, const char *tVariant, const char *tType, const lux_paramset* params)
{
	boost::mutex::scoped_lock lock(ctxMutex);
	checkContext();
	ctx->Texture(tName, tVariant, tType, UPCAST_PARAMSET);
}
void lux_wrapped_context::transform(float tx[16])
{
	boost::mutex::scoped_lock lock(ctxMutex);
	checkContext();
	ctx->Transform(tx);
}
void lux_wrapped_context::transformBegin()
{
	boost::mutex::scoped_lock lock(ctxMutex);
	checkContext();
	ctx->TransformBegin();
}
void lux_wrapped_context::transformEnd()
{
	boost::mutex::scoped_lock lock(ctxMutex);
	checkContext();
	ctx->TransformEnd();
}
void lux_wrapped_context::translate(float dx, float dy, float dz)
{
	boost::mutex::scoped_lock lock(ctxMutex);
	checkContext();
	ctx->Translate(dx, dy, dz);
}
void lux_wrapped_context::volume(const char *vName, const lux_paramset* params)
{
	boost::mutex::scoped_lock lock(ctxMutex);
	checkContext();
	ctx->Volume(vName, UPCAST_PARAMSET);
}
void lux_wrapped_context::volumeIntegrator(const char *vName, const lux_paramset* params)
{
	boost::mutex::scoped_lock lock(ctxMutex);
	checkContext();
	ctx->VolumeIntegrator(vName, UPCAST_PARAMSET);
}
void lux_wrapped_context::worldBegin()
{
	boost::mutex::scoped_lock lock(ctxMutex);
	checkContext();
	ctx->WorldBegin();
}
void lux_wrapped_context::worldEnd()
{
	boost::mutex::scoped_lock lock(ctxMutex);
	checkContext();
	// Run in a thread so that the calling code doesn't block; use wait() to block
	render_threads.push_back(new boost::thread( boost::bind(&lux_wrapped_context::world_end_thread, this) ));
}

// I/O and imaging
void lux_wrapped_context::loadFLM(const char* fName)
{
	boost::mutex::scoped_lock lock(ctxMutex);
	checkContext();
	ctx->LoadFLM(fName);
}
void lux_wrapped_context::saveFLM(const char* fName)
{
	boost::mutex::scoped_lock lock(ctxMutex);
	checkContext();
	ctx->SaveFLM(fName);
}
void lux_wrapped_context::saveEXR(const char *filename, bool useHalfFloat, bool includeZBuffer, bool tonemapped)
{
	boost::mutex::scoped_lock lock(ctxMutex);
	checkContext();
	ctx->SaveEXR(filename, useHalfFloat, includeZBuffer, 2 /*ZIP_COMPRESSION*/, tonemapped);
}
void lux_wrapped_context::overrideResumeFLM(const char *fName)
{
	boost::mutex::scoped_lock lock(ctxMutex);
	checkContext();
	ctx->OverrideResumeFLM(fName);
}
void lux_wrapped_context::updateFramebuffer()
{
	boost::mutex::scoped_lock lock(ctxMutex);
	checkContext();
	ctx->UpdateFramebuffer();
}
const unsigned char* lux_wrapped_context::framebuffer()
{
	boost::mutex::scoped_lock lock(ctxMutex);
	checkContext();
	return ctx->Framebuffer();
}
const float* lux_wrapped_context::floatFramebuffer()
{
	boost::mutex::scoped_lock lock(ctxMutex);
	checkContext();
	return ctx->FloatFramebuffer();
}
const float* lux_wrapped_context::alphaBuffer()
{
	boost::mutex::scoped_lock lock(ctxMutex);
	checkContext();
	return ctx->AlphaBuffer();
}
const float* lux_wrapped_context::zBuffer()
{
	boost::mutex::scoped_lock lock(ctxMutex);
	checkContext();
	return ctx->ZBuffer();
}
const unsigned char* lux_wrapped_context::getHistogramImage(unsigned int width, unsigned int height, int options)
{
	boost::mutex::scoped_lock lock(ctxMutex);
	
	int nvalues=width*height;
	unsigned char* outPixels = new unsigned char[nvalues];
	checkContext();
	ctx->GetHistogramImage(outPixels, width, height, options);
	return outPixels;
}

// Old-style parameter update interface
// TODO; see .h files

// Queryable interface
const char* lux_wrapped_context::getAttributes()
{
	boost::mutex::scoped_lock lock(ctxMutex);
	checkContext();
	return ctx->registry.GetContent();
}
// TODO; see .h files

// Networking interface
void lux_wrapped_context::addServer(const char *sName)
{
	boost::mutex::scoped_lock lock(ctxMutex);
	checkContext();
	ctx->AddServer(sName);
}
void lux_wrapped_context::removeServer(const char *sName)
{
	boost::mutex::scoped_lock lock(ctxMutex);
	checkContext();
	ctx->RemoveServer(sName);
}
unsigned int lux_wrapped_context::getServerCount()
{
	boost::mutex::scoped_lock lock(ctxMutex);
	checkContext();
	return ctx->GetServerCount();
}
void lux_wrapped_context::updateFilmFromNetwork()
{
	boost::mutex::scoped_lock lock(ctxMutex);
	checkContext();
	ctx->UpdateFilmFromNetwork();
}
void lux_wrapped_context::setNetworkServerUpdateInterval(int updateInterval)
{
	boost::mutex::scoped_lock lock(ctxMutex);
	checkContext();
	ctx->SetNetworkServerUpdateInterval(updateInterval);
}
int lux_wrapped_context::getNetworkServerUpdateInterval()
{
	boost::mutex::scoped_lock lock(ctxMutex);
	checkContext();
	return ctx->GetNetworkServerUpdateInterval();
}
// TODO; see .h files

// Stats
double lux_wrapped_context::statistics(const char* statName)
{
	boost::mutex::scoped_lock lock(ctxMutex);
	checkContext();
	return ctx->Statistics( std::string(statName) );
}
const char* lux_wrapped_context::printableStatistics(const bool addTotal)
{
	boost::mutex::scoped_lock lock(ctxMutex);
	checkContext();
	return ctx->PrintableStatistics(addTotal);
}
const char* lux_wrapped_context::customStatistics(const char* custom_template)
{
	boost::mutex::scoped_lock lock(ctxMutex);
	checkContext();
	return ctx->CustomStatistics(custom_template);
}

// Debugging interface
void lux_wrapped_context::enableDebugMode()
{
	boost::mutex::scoped_lock lock(ctxMutex);
	checkContext();
	ctx->EnableDebugMode();
}
void lux_wrapped_context::disableRandomMode()
{
	boost::mutex::scoped_lock lock(ctxMutex);
	checkContext();
	ctx->DisableRandomMode();
}
void lux_wrapped_context::setEpsilon(const float minValue, const float maxValue)
{
	boost::mutex::scoped_lock lock(ctxMutex);
	checkContext();
	ctx->SetEpsilon(minValue, maxValue);
}


// -----------------------------------------------------------------------
// PARAMSET WRAPPING
// -----------------------------------------------------------------------

// This class is going to wrap a lux::ParamSet
lux_wrapped_paramset::lux_wrapped_paramset()
{
	ps = new lux::ParamSet();
}
lux_wrapped_paramset::~lux_wrapped_paramset()
{
	delete(ps);
}
void lux_wrapped_paramset::AddFloat(const char* n, const float * v, unsigned int nItems)
{
	ps->AddFloat(n, v, nItems);
}
void lux_wrapped_paramset::AddInt(const char* n, const int * v, unsigned int nItems)
{
	ps->AddInt(n, v, nItems);
}
void lux_wrapped_paramset::AddBool(const char* n, const bool * v, unsigned int nItems)
{
	ps->AddBool(n, v, nItems);
}
void lux_wrapped_paramset::AddPoint(const char* n, const float * v, unsigned int nItems)
{
	lux::Point* p = new lux::Point(v[0],v[1],v[2]);
	ps->AddPoint(n, p, nItems);
}
void lux_wrapped_paramset::AddVector(const char* n, const float * v, unsigned int nItems)
{
	lux::Vector* vec = new lux::Vector(v[0],v[1],v[2]);
	ps->AddVector(n, vec, nItems);
}
void lux_wrapped_paramset::AddNormal(const char* n, const float * v, unsigned int nItems)
{
	lux::Normal* nor = new lux::Normal(v[0],v[1],v[2]);
	ps->AddNormal(n, nor, nItems);
}
void lux_wrapped_paramset::AddRGBColor(const char* n, const float * v, unsigned int nItems)
{
	lux::RGBColor* col = new lux::RGBColor(v[0],v[1],v[2]);
	ps->AddRGBColor(n, col, nItems);
}
void lux_wrapped_paramset::AddString(const char* n, const char* v, unsigned int nItems)
{
	std::string *str = new std::string(v);
	ps->AddString(n, str, nItems);
}
void lux_wrapped_paramset::AddTexture(const char* n, const char* v)
{
	ps->AddTexture(n, v);
}
