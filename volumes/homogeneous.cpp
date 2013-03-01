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

// homogeneous.cpp*
#include "homogeneous.h"
#include "paramset.h"
#include "dynload.h"

using namespace lux;

// HomogeneousVolume Method Definitions
Volume * HomogeneousVolume::CreateVolume(const Transform &volume2world,
	const ParamSet &params)
{
	// Initialize common volume region parameters
	boost::shared_ptr<Texture<FresnelGeneral> > fr(params.GetFresnelTexture("fresnel", 1.5f));
	boost::shared_ptr<Texture<SWCSpectrum> > sigma_a(params.GetSWCSpectrumTexture("sigma_a", RGBColor(0.f)));
	boost::shared_ptr<Texture<SWCSpectrum> > sigma_s(params.GetSWCSpectrumTexture("sigma_s", RGBColor(0.f)));
	boost::shared_ptr<Texture<SWCSpectrum> > g(params.GetSWCSpectrumTexture("g", RGBColor(0.f)));
//	RGBColor Le = params.FindOneRGBColor("Le", RGBColor(0.f));

	return new HomogeneousVolume(fr, sigma_a, sigma_s, g);
}
Region * HomogeneousVolume::CreateVolumeRegion(const Transform &volume2world,
	const ParamSet &params)
{
	// Initialize common volume region parameters
	RGBColor sigma_a = params.FindOneRGBColor("sigma_a", RGBColor(0.f));
	RGBColor sigma_s = params.FindOneRGBColor("sigma_s", RGBColor(0.f));
	float g = params.FindOneFloat("g", 0.f);
	RGBColor Le = params.FindOneRGBColor("Le", RGBColor(0.f));
	Point p0 = params.FindOnePoint("p0", Point(0,0,0));
	Point p1 = params.FindOnePoint("p1", Point(1,1,1));

	return new VolumeRegion<RGBVolume>(volume2world, BBox(p0, p1),
		RGBVolume(sigma_a, sigma_s, Le, g));
}

static DynamicLoader::RegisterVolume<HomogeneousVolume> r1("homogeneous");
static DynamicLoader::RegisterVolumeRegion<HomogeneousVolume> r2("homogeneous");
