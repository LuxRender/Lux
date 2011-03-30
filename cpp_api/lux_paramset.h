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

// This file defines the interface to a LuxRender ParamSet, used for passing
// data into a LuxRender rendering Context

#ifndef LUX_CPP_PARAMSET_H	// LUX_PARAMSET_H already used in Lux core/paramset.h
#define LUX_CPP_PARAMSET_H

#include "export_defs.h"

// This is the CPP API Interface for a LuxRender ParamSet
CPP_EXPORT class CPP_API lux_paramset {
public:
	lux_paramset() {};
	virtual ~lux_paramset() {};

	virtual void AddFloat(const char*, const float *, u_int nItems = 1) = 0;
	virtual void AddInt(const char*, const int *, u_int nItems = 1) = 0;
	virtual void AddBool(const char*, const bool *, u_int nItems = 1) = 0;
	virtual void AddPoint(const char*, const float *, u_int nItems = 1) = 0;
	virtual void AddVector(const char*, const float *, u_int nItems = 1) = 0;
	virtual void AddNormal(const char*, const float *, u_int nItems = 1) = 0;
	virtual void AddRGBColor(const char*, const float *, u_int nItems = 1) = 0;
	virtual void AddString(const char*, const char*, u_int nItems = 1) = 0;
	virtual void AddTexture(const char*, const char*) = 0;

};

// Pointer to lux_paramset factory function
typedef lux_paramset* (*CreateLuxParamSetPtr)();

#endif	// LUX_CPP_PARAMSET_H
