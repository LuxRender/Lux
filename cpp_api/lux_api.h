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

#ifndef LUX_CPP_API_H	// LUX_API_H already used by Lux core/api.h
#define LUX_CPP_API_H

#include <vector>

#include <boost/thread.hpp>

// LuxRender core includes
#include "context.h"
#include "queryable.h"
#include "paramset.h"

// CPP API Includes
#include "export_defs.h"
#include "lux_instance.h"
#include "lux_paramset.h"

CPP_EXPORT class CPP_API lux_wrapped_context : public lux_instance {
public:
	lux_wrapped_context(const char* _name);
	virtual ~lux_wrapped_context();

	// context metadata
	const char* version();
	const char* getName();

	// file parsing methods
	bool parse(const char* filename, bool async);
	bool parsePartial(const char* filename, bool async);
	bool parseSuccessful();
	
	// housekeeping
	void wait();
	void exit();
	void cleanup();

	// scene description methods
	void identity();
	void translate(float dx, float dy, float dz);
	void rotate(float angle, float ax, float ay, float az);
	void scale(float sx, float sy, float sz);
	void lookAt(float ex, float ey, float ez, float lx, float ly, float lz, float ux, float uy, float uz);
	void concatTransform(float tx[16]);
	void transform(float tx[16]);
	void coordinateSystem(const char *name);
	void coordSysTransform(const char *name);
	void renderer(const char *name, const lux_paramset* params);
	void film(const char *name, const lux_paramset* params);
	void lightSource(const char *name, const lux_paramset* params);
	void worldBegin();
	void worldEnd();

	// rendering control
	double statistics(const char* statName);
	const char* printableStatistics(const bool addTotal);
	unsigned int addThread();
	void removeThread();

private:
	const char* name;
	lux::Context* ctx;
	std::vector<boost::thread*> render_threads;
	void checkContext()
	{
		if (ctx == NULL)
			ctx = new lux::Context(name);

		lux::Context::SetActive(ctx);
	}
	void world_end_thread()
	{
		ctx->WorldEnd();
	}
};

CPP_EXPORT class CPP_API lux_wrapped_paramset : public lux_paramset {
public:
	lux_wrapped_paramset();
	virtual ~lux_wrapped_paramset();

	lux::ParamSet* GetParamSet() { return ps; };

	void AddFloat(const char*, const float *, u_int nItems = 1);
	void AddInt(const char*, const int *, u_int nItems = 1);
	void AddBool(const char*, const bool *, u_int nItems = 1);
	void AddPoint(const char*, const float *, u_int nItems = 1);
	void AddVector(const char*, const float *, u_int nItems = 1);
	void AddNormal(const char*, const float *, u_int nItems = 1);
	void AddRGBColor(const char*, const float *, u_int nItems = 1);
	void AddString(const char*, const char*, u_int nItems = 1);
	void AddTexture(const char*, const char*);

private:
	lux::ParamSet* ps;
};

// exported factory functions
CPP_EXPORT CPP_API lux_instance* CreateLuxInstance(const char* _name);
CPP_EXPORT CPP_API lux_paramset* CreateLuxParamSet();

#endif	// LUX_CPP_API_H
