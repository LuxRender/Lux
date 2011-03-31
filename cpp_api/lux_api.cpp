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
// FACTORY FUNCTIONS
// -----------------------------------------------------------------------

// Returns an instance of lux_wrapped_context, cast as a pure lux_instance interface
CPP_API lux_instance* CreateLuxInstance(const char* _name)
{
	return dynamic_cast<lux_instance*>(new lux_wrapped_context(_name));
}
// Returns an instance of lux_wrapped_paramset, cast as a pure lux_paramset interface
CPP_API lux_paramset* CreateLuxParamSet()
{
	return dynamic_cast<lux_paramset*>(new lux_wrapped_paramset());
}

// -----------------------------------------------------------------------
// CONTEXT WRAPPING
// -----------------------------------------------------------------------

#define UPCAST_PARAMSET *((lux_wrapped_paramset*)params)->GetParamSet()

// Initialise the Lux Core only once, when the first lux_wrapped_context is constructed
boost::once_flag luxInitFlag = BOOST_ONCE_INIT;

// This class is going to wrap lux::Context
lux_wrapped_context::lux_wrapped_context(const char* _name) : name(_name)
{
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

	delete(ctx);
	ctx = NULL;
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
	checkContext();
	return true;
}
bool lux_wrapped_context::parsePartial(const char* filename, bool async)
{
	checkContext();
	return true;
}
bool lux_wrapped_context::parseSuccessful()
{
	checkContext();
	return true;
}

// rendering control
void lux_wrapped_context::pause()
{
	checkContext();
	ctx->Pause();
}
void lux_wrapped_context::start()
{
	checkContext();
	ctx->Resume();
}
void lux_wrapped_context::setHaltSamplesPerPixel(int haltspp, bool haveEnoughSamplesPerPixel, bool suspendThreadsWhenDone)
{
	checkContext();
	ctx->SetHaltSamplesPerPixel(haltspp, haveEnoughSamplesPerPixel, suspendThreadsWhenDone);
}
unsigned int lux_wrapped_context::addThread()
{
	checkContext();
	return ctx->AddThread();
}
void lux_wrapped_context::removeThread()
{
	checkContext();
	ctx->RemoveThread();
}
void lux_wrapped_context::wait()
{
	checkContext();
	ctx->Wait();
}
void lux_wrapped_context::exit()
{
	checkContext();
	ctx->Exit();
}
void lux_wrapped_context::cleanup()
{
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
	checkContext();
	ctx->Accelerator(aName, UPCAST_PARAMSET);
}
void lux_wrapped_context::areaLightSource(const char *aName, const lux_paramset* params)
{
	checkContext();
	ctx->AreaLightSource(aName, UPCAST_PARAMSET);
}
void lux_wrapped_context::attributeBegin()
{
	checkContext();
	ctx->AttributeBegin();
}
void lux_wrapped_context::attributeEnd()
{
	checkContext();
	ctx->AttributeEnd();
}
void lux_wrapped_context::camera(const char *cName, const lux_paramset* params)
{
	checkContext();
	ctx->Camera(cName, UPCAST_PARAMSET);
}
void lux_wrapped_context::concatTransform(float tx[16])
{
	checkContext();
	ctx->ConcatTransform(tx);
}
void lux_wrapped_context::coordinateSystem(const char *cnName)
{
	checkContext();
	ctx->CoordinateSystem(cnName);
}
void lux_wrapped_context::coordSysTransform(const char *cnName)
{
	checkContext();
	ctx->CoordSysTransform(cnName);
}
void lux_wrapped_context::exterior(const char *eName)
{
	checkContext();
	ctx->Exterior(eName);
}
void lux_wrapped_context::film(const char *fName, const lux_paramset* params)
{
	checkContext();
	ctx->Film(fName, UPCAST_PARAMSET);
}
void lux_wrapped_context::identity()
{
	checkContext();
	ctx->Identity();
}
void lux_wrapped_context::interior(const char *iName)
{
	checkContext();
	ctx->Interior(iName);
}
void lux_wrapped_context::lightGroup(const char *lName, const lux_paramset* params)
{
	checkContext();
	ctx->LightGroup(lName, UPCAST_PARAMSET);
}
void lux_wrapped_context::lightSource(const char *lName, const lux_paramset* params)
{
	checkContext();
	ctx->LightSource(lName, UPCAST_PARAMSET);
}
void lux_wrapped_context::lookAt(float ex, float ey, float ez, float lx, float ly, float lz, float ux, float uy, float uz)
{
	checkContext();
	ctx->LookAt(ex,ey,ez, lx,ly,lx, ux,uy,uz);
}
void lux_wrapped_context::makeNamedMaterial(const char *mName, const lux_paramset* params)
{
	checkContext();
	ctx->MakeNamedMaterial(mName, UPCAST_PARAMSET);
}
void lux_wrapped_context::makeNamedVolume(const char *vName, const char *vType, const lux_paramset* params)
{
	checkContext();
	ctx->MakeNamedVolume(vName, vType, UPCAST_PARAMSET);
}
void lux_wrapped_context::material(const char *mName, const lux_paramset* params)
{
	checkContext();
	ctx->Material(mName, UPCAST_PARAMSET);
}
void lux_wrapped_context::motionInstance(const char *mName, float startTime, float endTime, const char *toTransform)
{
	checkContext();
	ctx->MotionInstance(mName, startTime, endTime, toTransform);
}
void lux_wrapped_context::namedMaterial(const char *mName)
{
	checkContext();
	ctx->NamedMaterial(mName);
}
void lux_wrapped_context::objectBegin(const char *oName)
{
	checkContext();
	ctx->ObjectBegin(oName);
}
void lux_wrapped_context::objectEnd()
{
	checkContext();
	ctx->ObjectEnd();
}
void lux_wrapped_context::objectInstance(const char *oName)
{
	checkContext();
	ctx->ObjectInstance(oName);
}
void lux_wrapped_context::pixelFilter(const char *pName, const lux_paramset* params)
{
	checkContext();
	ctx->PixelFilter(pName, UPCAST_PARAMSET);
}
void lux_wrapped_context::portalInstance(const char *pName)
{
	checkContext();
	ctx->PortalInstance(pName);
}
void lux_wrapped_context::portalShape(const char *pName, const lux_paramset* params)
{
	checkContext();
	ctx->PortalShape(pName, UPCAST_PARAMSET);
}
void lux_wrapped_context::renderer(const char *rName, const lux_paramset* params)
{
	checkContext();
	ctx->Renderer(rName, UPCAST_PARAMSET);
}
void lux_wrapped_context::reverseOrientation()
{
	checkContext();
	ctx->ReverseOrientation();
}
void lux_wrapped_context::rotate(float angle, float ax, float ay, float az)
{
	checkContext();
	ctx->Rotate(angle, ax, ay, az);
}
void lux_wrapped_context::sampler(const char *sName, const lux_paramset* params)
{
	checkContext();
	ctx->Sampler(sName, UPCAST_PARAMSET);
}
void lux_wrapped_context::scale(float sx, float sy, float sz)
{
	checkContext();
	ctx->Scale(sx, sy, sz);
}
void lux_wrapped_context::shape(const char *sName, const lux_paramset* params)
{
	checkContext();
	ctx->Shape(sName, UPCAST_PARAMSET);
}
void lux_wrapped_context::surfaceIntegrator(const char *sName, const lux_paramset* params)
{
	checkContext();
	ctx->SurfaceIntegrator(sName, UPCAST_PARAMSET);
}
void lux_wrapped_context::texture(const char *tName, const char *tVariant, const char *tType, const lux_paramset* params)
{
	checkContext();
	ctx->Texture(tName, tVariant, tType, UPCAST_PARAMSET);
}
void lux_wrapped_context::transform(float tx[16])
{
	checkContext();
	ctx->Transform(tx);
}
void lux_wrapped_context::transformBegin()
{
	checkContext();
	ctx->TransformBegin();
}
void lux_wrapped_context::transformEnd()
{
	checkContext();
	ctx->TransformEnd();
}
void lux_wrapped_context::translate(float dx, float dy, float dz)
{
	checkContext();
	ctx->Translate(dx, dy, dz);
}
void lux_wrapped_context::volume(const char *vName, const lux_paramset* params)
{
	checkContext();
	ctx->Volume(vName, UPCAST_PARAMSET);
}	
void lux_wrapped_context::volumeIntegrator(const char *vName, const lux_paramset* params)
{
	checkContext();
	ctx->VolumeIntegrator(vName, UPCAST_PARAMSET);
}
void lux_wrapped_context::worldBegin()
{
	checkContext();
	ctx->WorldBegin();
}
void lux_wrapped_context::worldEnd()
{
	checkContext();
	// Run in a thread so that the calling code doesn't block; use wait() to block
	render_threads.push_back(new boost::thread( boost::bind(&lux_wrapped_context::world_end_thread, this) ));
}

// I/O and imaging
void lux_wrapped_context::loadFLM(const char* fName)
{
	checkContext();
	ctx->LoadFLM(fName);
}
void lux_wrapped_context::saveFLM(const char* fName)
{
	checkContext();
	ctx->SaveFLM(fName);
}
void lux_wrapped_context::saveEXR(const char *filename, bool useHalfFloat, bool includeZBuffer, bool tonemapped)
{
	checkContext();
	ctx->SaveEXR(filename, useHalfFloat, includeZBuffer, 2 /*ZIP_COMPRESSION*/, tonemapped);
}
void lux_wrapped_context::overrideResumeFLM(const char *fName)
{
	checkContext();
	ctx->OverrideResumeFLM(fName);
}
void lux_wrapped_context::updateFramebuffer()
{
	checkContext();
	ctx->UpdateFramebuffer();
}
const unsigned char* lux_wrapped_context::framebuffer()
{
	checkContext();
	return ctx->Framebuffer();
}
const float* lux_wrapped_context::floatFramebuffer()
{
	checkContext();
	return ctx->FloatFramebuffer();
}
const float* lux_wrapped_context::alphaBuffer()
{
	checkContext();
	return ctx->AlphaBuffer();
}
const float* lux_wrapped_context::zBuffer()
{
	checkContext();
	return ctx->ZBuffer();
}
const unsigned char* lux_wrapped_context::getHistogramImage(unsigned int width, unsigned int height, int options)
{
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
	checkContext();
	return ctx->registry.GetContent();
}
// TODO; see .h files

// Networking interface
void lux_wrapped_context::addServer(const char *sName)
{
	checkContext();
	ctx->AddServer(sName);
}
void lux_wrapped_context::removeServer(const char *sName)
{
	checkContext();
	ctx->RemoveServer(sName);
}
unsigned int lux_wrapped_context::getServerCount()
{
	checkContext();
	return ctx->GetServerCount();
}
void lux_wrapped_context::updateFilmFromNetwork()
{
	checkContext();
	ctx->UpdateFilmFromNetwork();
}
void lux_wrapped_context::setNetworkServerUpdateInterval(int updateInterval)
{
	checkContext();
	ctx->SetNetworkServerUpdateInterval(updateInterval);
}
int lux_wrapped_context::getNetworkServerUpdateInterval()
{
	checkContext();
	return ctx->GetNetworkServerUpdateInterval();
}
// TODO; see .h files

// Stats
double lux_wrapped_context::statistics(const char* statName)
{
	checkContext();
	return ctx->Statistics( std::string(statName) );
}
const char* lux_wrapped_context::printableStatistics(const bool addTotal)
{
	checkContext();
	return ctx->PrintableStatistics(addTotal);
}
const char* lux_wrapped_context::customStatistics(const char* custom_template)
{
	checkContext();
	return ctx->CustomStatistics(custom_template);
}

// Debugging interface
void lux_wrapped_context::enableDebugMode()
{
	checkContext();
	ctx->EnableDebugMode();
}
void lux_wrapped_context::disableRandomMode()
{
	checkContext();
	ctx->DisableRandomMode();
}
void lux_wrapped_context::setEpsilon(const float minValue, const float maxValue)
{
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
void lux_wrapped_paramset::AddFloat(const char* n, const float * v, u_int nItems)
{
	ps->AddFloat(n, v, nItems);
}
void lux_wrapped_paramset::AddInt(const char* n, const int * v, u_int nItems)
{
	ps->AddInt(n, v, nItems);
}
void lux_wrapped_paramset::AddBool(const char* n, const bool * v, u_int nItems)
{
	ps->AddBool(n, v, nItems);
}
void lux_wrapped_paramset::AddPoint(const char* n, const float * v, u_int nItems)
{
	lux::Point* p = new lux::Point(v[0],v[1],v[2]);
	ps->AddPoint(n, p, nItems);
}
void lux_wrapped_paramset::AddVector(const char* n, const float * v, u_int nItems)
{
	lux::Vector* vec = new lux::Vector(v[0],v[1],v[2]);
	ps->AddVector(n, vec, nItems);
}
void lux_wrapped_paramset::AddNormal(const char* n, const float * v, u_int nItems)
{
	lux::Normal* nor = new lux::Normal(v[0],v[1],v[2]);
	ps->AddNormal(n, nor, nItems);
}
void lux_wrapped_paramset::AddRGBColor(const char* n, const float * v, u_int nItems)
{
	lux::RGBColor* col = new lux::RGBColor(v[0],v[1],v[2]);
	ps->AddRGBColor(n, col, nItems);
}
void lux_wrapped_paramset::AddString(const char* n, const char* v, u_int nItems)
{
	std::string *str = new std::string(v);
	ps->AddString(n, str, nItems);
}
void lux_wrapped_paramset::AddTexture(const char* n, const char* v)
{
	ps->AddTexture(n, v);
}
