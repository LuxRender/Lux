/***************************************************************************
 *   Copyright (C) 1998-2013 by authors (see AUTHORS.txt)                  *
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

// Cloud.cpp*
#include "cloudvolume.h"
#include "paramset.h"
#include "dynload.h"

using namespace lux;

// Cloud Method Definitions
Region * Cloud::CreateVolumeRegion(const Transform &volume2world,
	const ParamSet &params)
{
	// Initialize common volume region parameters
	RGBColor sigma_a = params.FindOneRGBColor("sigma_a", 0.f);
	RGBColor sigma_s = params.FindOneRGBColor("sigma_s", 0.f);
	float g = params.FindOneFloat("g", 0.f);
	RGBColor Le = params.FindOneRGBColor("Le", 0.f);
	Point p0 = params.FindOnePoint("p0", Point(0,0,0));
	Point p1 = params.FindOnePoint("p1", Point(1,1,1));
	//Cloud parameters
	float radius = params.FindOneFloat("radius", 0.5f);
	float noiseScale = params.FindOneFloat("noisescale", 0.5f);
	float turbulence = params.FindOneFloat("turbulence", 0.01f);
	float sharpness = params.FindOneFloat("sharpness", 6.0f);
	float offSet = params.FindOneFloat("noiseoffset", 0.f);
	int numSpheres = params.FindOneInt("spheres", 0);
	int octaves = params.FindOneInt("octaves", 1);
	float omega = params.FindOneFloat("omega", 0.75f);
	float variability = params.FindOneFloat("variability", 0.9f);
	float baseflatness = params.FindOneFloat("baseflatness", 0.8f);
	float spheresize = params.FindOneFloat("spheresize", 0.15f);
	return new VolumeRegion<CloudVolume>(volume2world, BBox(p0, p1),
		CloudVolume(sigma_a, sigma_s, g, Le, BBox(p0, p1), radius,
		volume2world, noiseScale, turbulence, sharpness, variability,
		baseflatness, octaves, omega, offSet, numSpheres, spheresize));
}

static DynamicLoader::RegisterVolumeRegion<Cloud> r("cloud");
