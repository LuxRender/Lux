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

// CPP API Includes
#include "export_defs.h"
#include "lux_instance.h"
#include "lux_paramset.h"
#include "lux_api.h"

// -----------------------------------------------------------------------
// FACTORY FUNCTIONS
// -----------------------------------------------------------------------

// Returns an instance of lux_wrapped_context, cast as a pure lux_instance interface
CPP_API lux_instance* CreateLuxInstance(const char* _name)
{
	return dynamic_cast<lux_instance*>(new lux_wrapped_context(_name));
};

//Returns an instance of lux_wrapped_paramset, cast as a pure lux_paramset interface
CPP_API lux_paramset* CreateLuxParamSet()
{
	return dynamic_cast<lux_paramset*>(new lux_wrapped_paramset());
};
