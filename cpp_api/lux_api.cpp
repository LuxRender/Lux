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

// Initialise the Lux Core only once, when the first lux_wrapped_context is constructed
boost::once_flag luxInitFlag = BOOST_ONCE_INIT;

// This class is going to wrap lux::Context
lux_wrapped_context::lux_wrapped_context(const char* _name) : name(_name)
{
	boost::call_once(&luxInit, luxInitFlag);
	ctx = new lux::Context( _name );
}
lux_wrapped_context::~lux_wrapped_context()
{
	for each(boost::thread* t in render_threads)
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

// housekeeping
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
void lux_wrapped_context::identity()
{
	checkContext();
}
void lux_wrapped_context::translate(float dx, float dy, float dz)
{
	checkContext();
}
void lux_wrapped_context::rotate(float angle, float ax, float ay, float az)
{
	checkContext();
}
void lux_wrapped_context::scale(float sx, float sy, float sz)
{
	checkContext();
}
void lux_wrapped_context::lookAt(float ex, float ey, float ez, float lx, float ly, float lz, float ux, float uy, float uz)
{
	checkContext();
}
void lux_wrapped_context::concatTransform(float tx[16])
{
	checkContext();
}
void lux_wrapped_context::transform(float tx[16])
{
	checkContext();
}
void lux_wrapped_context::coordinateSystem(const char *name)
{
	checkContext();
}
void lux_wrapped_context::coordSysTransform(const char *name)
{
	checkContext();
}
void lux_wrapped_context::renderer(const char *name, const lux_paramset* params)
{
	checkContext();
}
void lux_wrapped_context::film(const char *name, const lux_paramset* params)
{
	checkContext();
	ctx->Film(name, *((lux_wrapped_paramset*)params)->GetParamSet());
}
void lux_wrapped_context::lightSource(const char *name, const lux_paramset* params)
{
	checkContext();
	ctx->LightSource(name, *((lux_wrapped_paramset*)params)->GetParamSet());
}
void lux_wrapped_context::worldBegin()
{
	checkContext();
	ctx->WorldBegin();
}
void lux_wrapped_context::worldEnd()
{
	checkContext();
	render_threads.push_back(new boost::thread( boost::bind(&lux_wrapped_context::world_end_thread, this) ));
}

// rendering control
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
