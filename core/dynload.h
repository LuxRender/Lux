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

#ifndef LUX_DYNLOAD_H
#define LUX_DYNLOAD_H
// dynload.h*
#include "lux.h"
#include "texture.h"
// Runtime Loading Declarations
COREDLL void UpdatePluginPath(const string &newpath);
COREDLL ShapePtr MakeShape(const string &name,
	const Transform &object2world, bool reverseOrientation, const ParamSet &paramSet);
COREDLL MaterialPtr MakeMaterial(const string &name,
	const Transform &mtl2world, const TextureParams &mp);
COREDLL Texture<float>::TexturePtr MakeFloatTexture(const string &name,
	const Transform &tex2world, const TextureParams &tp);
COREDLL Texture<Spectrum>::TexturePtr MakeSpectrumTexture(const string &name,
	const Transform &tex2world, const TextureParams &tp);
COREDLL Light *MakeLight(const string &name,
	const Transform &light2world, const ParamSet &paramSet);
COREDLL AreaLight *MakeAreaLight(const string &name,
	const Transform &light2world,
	const ParamSet &paramSet, const ShapePtr &shape);
COREDLL VolumeRegion *MakeVolumeRegion(const string &name,
	const Transform &light2world, const ParamSet &paramSet);
COREDLL SurfaceIntegrator *MakeSurfaceIntegrator(const string &name,
		const ParamSet &paramSet);
COREDLL VolumeIntegrator *MakeVolumeIntegrator(const string &name,
		const ParamSet &paramSet);
COREDLL Primitive *MakeAccelerator(const string &name,
		const vector<Primitive* > &prims,
		const ParamSet &paramSet);
COREDLL Camera *MakeCamera(const string &name,
	const ParamSet &paramSet, const Transform &world2cam, Film *film);
COREDLL Sampler *MakeSampler(const string &name,
	const ParamSet &paramSet, const Film *film);
COREDLL Filter *MakeFilter(const string &name,
	const ParamSet &paramSet);
COREDLL ToneMap *MakeToneMap(const string &name,
	const ParamSet &paramSet);
COREDLL Film *MakeFilm(const string &name,
	const ParamSet &paramSet, Filter *filt);
#endif // LUX_DYNLOAD_H
