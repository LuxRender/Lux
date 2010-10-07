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

// debug.cpp*
// Debug integrator by Greg Humphreys

// debug.cpp*

#include "debug.h"

using namespace lux;

// Lux (copy) constructor
DebugIntegrator* DebugIntegrator::clone() const
 {
   return new DebugIntegrator(*this);
 }
Spectrum DebugIntegrator::Li(const Scene *scene,
		const RayDifferential &ray, const Sample *sample,
		float *alpha) const {
	Intersection isect;
	Spectrum L(0.);
	bool hitSomething;
	BSDF *bsdf = NULL;

	if (alpha) *alpha = 1;

	hitSomething = scene->Intersect(ray, &isect);
	if (hitSomething) {
		bsdf = isect.GetBSDF( ray);
	}

	float color[3] = {0,0,0};

	int i;
	for (i = 0 ; i < 3 ; i++)
	{
		if (debug_variable[i] == DEBUG_HIT_SOMETHING)
		{
			color[i] = hitSomething ? 1 : 0;
		}
		else if (debug_variable[i] == DEBUG_ONE)
		{
			color[i] = 1;
		}
		else if (debug_variable[i] == DEBUG_ZERO)
		{
			color[i] = 0;
		}
		else if (hitSomething)
		{
			switch( debug_variable[i] )
			{
				case DEBUG_U:
					color[i] = isect.dg.u;
					break;
				case DEBUG_V:
					color[i] = isect.dg.v;
					break;
				case DEBUG_GEOM_NORMAL_X:
					color[i] = fabsf(isect.dg.nn.x);
					break;
				case DEBUG_GEOM_NORMAL_Y:
					color[i] = fabsf(isect.dg.nn.y);
					break;
				case DEBUG_GEOM_NORMAL_Z:
					color[i] = fabsf(isect.dg.nn.z);
					break;
				case DEBUG_SHAD_NORMAL_X:
					color[i] = fabsf(bsdf->dgShading.nn.x);
					break;
				case DEBUG_SHAD_NORMAL_Y:
					color[i] = fabsf(bsdf->dgShading.nn.y);
					break;
				case DEBUG_SHAD_NORMAL_Z:
					color[i] = fabsf(bsdf->dgShading.nn.z);
					break;
				default:
					break;
			}
		}
		if (color[i] < 0)
		{
			//Severe( "i = %d\ndebug_variable[i] = %d\nhit_something = %d\ncolor[i] = %f", i, debug_variable[i], hitSomething, color[i] );
			LOG(LUX_CONSISTENCY,LUX_SEVERE)<<"i = "<<i<<" debug_variable[i] = "<<debug_variable[i]<<" hit_something = "<<hitSomething<<" color[i] = "<<color[i];
		}
	}

	L = Spectrum(color);
	return L;
}

SurfaceIntegrator* DebugIntegrator::CreateSurfaceIntegrator(const ParamSet &params)
{
	string strs[3];
	strs[0] = params.FindOneString( "red", "u" );
	strs[1] = params.FindOneString( "green", "v" );
	strs[2] = params.FindOneString( "blue", "zero" );

	DebugVariable vars[3];

	int i;
	for (i = 0 ; i < 3 ; i++)
	{
		if (strs[i] == "u") { vars[i] = DEBUG_U; }
		else if (strs[i] == "v") { vars[i] = DEBUG_V; }
		else if (strs[i] == "nx") { vars[i] = DEBUG_GEOM_NORMAL_X; }
		else if (strs[i] == "ny") { vars[i] = DEBUG_GEOM_NORMAL_Y; }
		else if (strs[i] == "nz") { vars[i] = DEBUG_GEOM_NORMAL_Z; }
		else if (strs[i] == "snx") { vars[i] = DEBUG_SHAD_NORMAL_X; }
		else if (strs[i] == "sny") { vars[i] = DEBUG_SHAD_NORMAL_Y; }
		else if (strs[i] == "snz") { vars[i] = DEBUG_SHAD_NORMAL_Z; }
		else if (strs[i] == "hit") { vars[i] = DEBUG_HIT_SOMETHING; }
		else if (strs[i] == "zero") { vars[i] = DEBUG_ZERO; }
		else if (strs[i] == "one") { vars[i] = DEBUG_ONE; }
		else
		{
			//Error( "Unknown debug type \"%s\", defaulting to zero.", strs[i].c_str() );
			LOG(LUX_BADTOKEN,LUX_ERROR)<<"Unknown debug type '"<<strs[i]<<"' defaulting to zero.";
			vars[i] = DEBUG_ZERO;
		}
	}

	return new DebugIntegrator(vars);
}
