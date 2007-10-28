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

#include "lux.h"
#include "transport.h"
#include "scene.h"

typedef enum {
	DEBUG_U, DEBUG_V,
	DEBUG_GEOM_NORMAL_X,
	DEBUG_GEOM_NORMAL_Y,
	DEBUG_GEOM_NORMAL_Z,
	DEBUG_SHAD_NORMAL_X,
	DEBUG_SHAD_NORMAL_Y,
	DEBUG_SHAD_NORMAL_Z,
	DEBUG_ONE,
	DEBUG_ZERO,
	DEBUG_HIT_SOMETHING
} DebugVariable;

class DebugIntegrator : public SurfaceIntegrator {
public:
	// DebugIntegrator Public Methods
	Spectrum Li(MemoryArena &arena, const Scene *scene, const RayDifferential &ray,
			const Sample *sample, float *alpha) const;
	DebugIntegrator( DebugVariable v[3] )
	{
		debug_variable[0] = v[0];
		debug_variable[1] = v[1];
		debug_variable[2] = v[2];
	}
	virtual DebugIntegrator* clone() const; // Lux (copy) constructor for multithreading
	IntegrationSampler* HasIntegrationSampler(IntegrationSampler *is) { return NULL; }; // Not implemented
	static SurfaceIntegrator *CreateSurfaceIntegrator(const ParamSet &params);
private:
	DebugVariable debug_variable[3];
};
