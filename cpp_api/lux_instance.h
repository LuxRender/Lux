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

// This file defines the interface to a LuxRender rendering Context

#ifndef LUX_INSTANCE_H
#define LUX_INSTANCE_H

#include "export_defs.h"
#include "lux_paramset.h"

// This is the CPP API Interface for LuxRender
CPP_EXPORT class CPP_API lux_instance {
public:
	lux_instance() {};
	virtual ~lux_instance() {};

	// context metadata
	virtual const char* version() = 0;
	virtual const char* getName() = 0;

	// file parsing methods
	virtual bool parse(const char* filename, bool async) = 0;
	virtual bool parsePartial(const char* filename, bool async) = 0;
	virtual bool parseSuccessful() = 0;
	
	// housekeeping
	virtual void wait() = 0;
	virtual void exit() = 0;
	virtual void cleanup() = 0;

	// scene description methods
	virtual void identity() = 0;
	virtual void translate(float dx, float dy, float dz) = 0;
	virtual void rotate(float angle, float ax, float ay, float az) = 0;
	virtual void scale(float sx, float sy, float sz) = 0;
	virtual void lookAt(float ex, float ey, float ez, float lx, float ly, float lz, float ux, float uy, float uz) = 0;
	virtual void concatTransform(float tx[16]) = 0;
	virtual void transform(float tx[16]) = 0;
	virtual void coordinateSystem(const char *name) = 0;
	virtual void coordSysTransform(const char *name) = 0;
	virtual void renderer(const char *name, const lux_paramset* params) = 0;
	virtual void film(const char *name, const lux_paramset* params) = 0;
	virtual void lightSource(const char *name, const lux_paramset* params) = 0;
	virtual void worldBegin() = 0;
	virtual void worldEnd() = 0;
	
	// rendering control
	virtual double statistics(const char* statName) = 0;
	virtual const char* printableStatistics(const bool addTotal) = 0;
	virtual unsigned int addThread() = 0;
	virtual void removeThread() = 0;

};

// Pointer to lux_instance factory function
typedef lux_instance* (*CreateLuxInstancePtr)(const char* name);

#endif	// LUX_INSTANCE_H
