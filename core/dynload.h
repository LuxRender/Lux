/***************************************************************************
 *   Copyright (C) 1998-2008 by authors (see AUTHORS.txt )                 *
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

#ifndef LUX_DYNLOAD_H
#define LUX_DYNLOAD_H
// dynload.h*
#include "lux.h"

namespace lux
{

// Runtime Loading Declarations
 void UpdatePluginPath(const string &newpath);
 boost::shared_ptr<Shape> MakeShape(const string &name,
	const Transform &object2world, bool reverseOrientation, const ParamSet &paramSet);
 boost::shared_ptr<Material> MakeMaterial(const string &name,
	const Transform &mtl2world, const TextureParams &mp);
 boost::shared_ptr<Texture<float> > MakeFloatTexture(const string &name,
	const Transform &tex2world, const TextureParams &tp);
 boost::shared_ptr<Texture<Spectrum> > MakeSpectrumTexture(const string &name,
	const Transform &tex2world, const TextureParams &tp);
 Light *MakeLight(const string &name,
	const Transform &light2world, const ParamSet &paramSet);
 AreaLight *MakeAreaLight(const string &name,
	const Transform &light2world,
	const ParamSet &paramSet, const boost::shared_ptr<Shape> &shape);
 VolumeRegion *MakeVolumeRegion(const string &name,
	const Transform &light2world, const ParamSet &paramSet);
 SurfaceIntegrator *MakeSurfaceIntegrator(const string &name,
		const ParamSet &paramSet);
 VolumeIntegrator *MakeVolumeIntegrator(const string &name,
		const ParamSet &paramSet);
 Primitive *MakeAccelerator(const string &name,
		const vector<Primitive* > &prims,
		const ParamSet &paramSet);
 Camera *MakeCamera(const string &name,
	const ParamSet &paramSet, const Transform &world2cam, Film *film);
 Sampler *MakeSampler(const string &name,
	const ParamSet &paramSet, const Film *film);
 Filter *MakeFilter(const string &name,
	const ParamSet &paramSet);
 ToneMap *MakeToneMap(const string &name,
	const ParamSet &paramSet);
 Film *MakeFilm(const string &name,
	const ParamSet &paramSet, Filter *filt);
 
}//namespace lux
 
#endif // LUX_DYNLOAD_H
