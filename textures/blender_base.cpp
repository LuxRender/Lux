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

#include "blender_base.h"
#include "geometry/raydifferential.h"
#include <sstream>

using namespace blender;
using std::stringstream;

namespace lux {

typedef map<string, short> mapstsh;

void BlenderTexture3D::GetDuv(const SpectrumWavelengths &sw,
	const DifferentialGeometry &dg, float delta, float *du, float *dv) const
{
	// Calculate values at intersection point (copy of Evaluate)
	Vector dpdu, dpdv;
	const Point P(mapping->MapDuv(dg, &dpdu, &dpdv));

	const float a = GetF(P);

	const float e = tex2->Evaluate(sw, dg) - tex1->Evaluate(sw, dg);
	float du1, dv1, du2, dv2;
	tex1->GetDuv(sw, dg, delta, &du1, &dv1);
	tex2->GetDuv(sw, dg, delta, &du2, &dv2);

	// Compute offset positions and evaluate displacement texture
	// Shift _P_ _du_ in the $u$ direction and calculate value
	const float uu = delta / dg.dpdu.Length();
	const Point Pu(P + dg.dpdu * uu);
	const float dau = (GetF(Pu) - a) / uu;

	// Shift _dgTemp_ _dv_ in the $v$ direction and calculate value
	const float vv = delta / dg.dpdv.Length();
	const Point Pv(P + dg.dpdv * vv);
	const float dav = (GetF(Pv) - a) / vv;

	*du = Lerp(a, du1, du2) + dau * e;
	*dv = Lerp(a, dv1, dv2) + dav * e;
}

static short GetValue(const mapstsh &m, const string &type, const string &name)
{
	mapstsh::const_iterator t = m.find(name);
	if (t != m.end())
		return (*t).second;
	LOG( LUX_ERROR,LUX_BADTOKEN) << "Unknown " << type << " '" << name << "'";
	return (*m.find("")).second;
}

static const mapstsh::value_type blendTypeInit[8] = {
	mapstsh::value_type("", TEX_LIN),
	mapstsh::value_type("lin", TEX_LIN),
	mapstsh::value_type("quad", TEX_QUAD),
	mapstsh::value_type("ease", TEX_EASE),
	mapstsh::value_type("diag", TEX_DIAG),
	mapstsh::value_type("sphere", TEX_SPHERE),
	mapstsh::value_type("halo", TEX_HALO),
	mapstsh::value_type("radial", TEX_RAD)
};
static const mapstsh blendType(blendTypeInit, blendTypeInit + 8);
short BlenderTexture3D::GetBlendType(const string &name)
{
	return GetValue(blendType, "blend type", name);
}

static const mapstsh::value_type cloudTypeInit[3] = {
	mapstsh::value_type("", TEX_DEFAULT),
	mapstsh::value_type("default", TEX_DEFAULT),
	mapstsh::value_type("color", TEX_COLOR)
};
static const mapstsh cloudType(cloudTypeInit, cloudTypeInit + 3);
short BlenderTexture3D::GetCloudType(const string &name)
{
	return GetValue(cloudType, "cloud type", name);
}

static const mapstsh::value_type marbleTypeInit[4] = {
	mapstsh::value_type("", TEX_SOFT),
	mapstsh::value_type("soft", TEX_SOFT),
	mapstsh::value_type("sharp", TEX_SHARP),
	mapstsh::value_type("sharper", TEX_SHARPER)
};
static const mapstsh marbleType(marbleTypeInit, marbleTypeInit + 4);
short BlenderTexture3D::GetMarbleType(const string &name)
{
	return GetValue(marbleType, "marble type", name);
}

static const mapstsh::value_type musgraveTypeInit[6] = {
	mapstsh::value_type("", TEX_MFRACTAL),
	mapstsh::value_type("multifractal", TEX_MFRACTAL),
	mapstsh::value_type("ridged_multifractal", TEX_RIDGEDMF),
	mapstsh::value_type("hybrid_multifractal", TEX_HYBRIDMF),
	mapstsh::value_type("hetero_terrain", TEX_HTERRAIN),
	mapstsh::value_type("fbm", TEX_FBM)
};
static const mapstsh musgraveType(musgraveTypeInit, musgraveTypeInit + 6);
short BlenderTexture3D::GetMusgraveType(const string &name)
{
	return GetValue(musgraveType, "musgrave type", name);
}

static const mapstsh::value_type stucciTypeInit[4] = {
	mapstsh::value_type("", TEX_PLASTIC),
	mapstsh::value_type("plastic", TEX_PLASTIC),
	mapstsh::value_type("wall_in", TEX_WALLIN),
	mapstsh::value_type("wall_out", TEX_WALLOUT)
};
static const mapstsh stucciType(stucciTypeInit, stucciTypeInit + 4);
short BlenderTexture3D::GetStucciType(const string &name)
{
	return GetValue(stucciType, "stucci type", name);
}

static const mapstsh::value_type voronoiTypeInit[8] = {
	mapstsh::value_type("", TEX_DISTANCE),
	mapstsh::value_type("actual_distance", TEX_DISTANCE),
	mapstsh::value_type("distance_squared", TEX_DISTANCE_SQUARED),
	mapstsh::value_type("manhattan", TEX_MANHATTAN),
	mapstsh::value_type("chebychev", TEX_CHEBYCHEV),
	mapstsh::value_type("minkovsky_half", TEX_MINKOVSKY_HALF),
	mapstsh::value_type("minkovsky_four", TEX_MINKOVSKY_FOUR),
	mapstsh::value_type("minkovsky", TEX_MINKOVSKY)
};
static const mapstsh voronoiType(voronoiTypeInit, voronoiTypeInit + 8);
short BlenderTexture3D::GetVoronoiType(const string &name)
{
	return GetValue(voronoiType, "voronoi distance", name);
}

static const mapstsh::value_type woodTypeInit[5] = {
	mapstsh::value_type("", TEX_BAND),
	mapstsh::value_type("bands", TEX_BAND),
	mapstsh::value_type("rings", TEX_RING),
	mapstsh::value_type("bandnoise", TEX_BANDNOISE),
	mapstsh::value_type("ringnoise", TEX_RINGNOISE)
};
static const mapstsh woodType(woodTypeInit, woodTypeInit + 5);
short BlenderTexture3D::GetWoodType(const string &name)
{
	return GetValue(woodType, "wood type", name);
}

static const mapstsh::value_type noiseTypeInit[3] = {
	mapstsh::value_type("", TEX_NOISESOFT),
	mapstsh::value_type("soft_noise", TEX_NOISESOFT),
	mapstsh::value_type("hard_noise", TEX_NOISEPERL)
};
static const mapstsh noiseType(noiseTypeInit, noiseTypeInit + 3);
short BlenderTexture3D::GetNoiseType(const string &name)
{
	return GetValue(noiseType, "noise type", name);
}

static const mapstsh::value_type noiseBasisInit[11] = {
	mapstsh::value_type("", TEX_BLENDER),
	mapstsh::value_type("blender_original", TEX_BLENDER),
	mapstsh::value_type("original_perlin", TEX_STDPERLIN),
	mapstsh::value_type("improved_perlin", TEX_NEWPERLIN),
	mapstsh::value_type("voronoi_f1", TEX_VORONOI_F1),
	mapstsh::value_type("voronoi_f2", TEX_VORONOI_F2),
	mapstsh::value_type("voronoi_f3", TEX_VORONOI_F3),
	mapstsh::value_type("voronoi_f4", TEX_VORONOI_F4),
	mapstsh::value_type("voronoi_f2_f1", TEX_VORONOI_F2_F1),
	mapstsh::value_type("voronoi_crackle", TEX_VORONOI_CRACKLE),
	mapstsh::value_type("cell_noise", TEX_CELLNOISE)
};
static const mapstsh noiseBasis(noiseBasisInit, noiseBasisInit + 11);
short BlenderTexture3D::GetNoiseBasis(const string &name)
{
	return GetValue(noiseBasis, "noise basis", name);
}

static const mapstsh::value_type noiseShapeInit[4] = {
	mapstsh::value_type("", TEX_SIN),
	mapstsh::value_type("sin", TEX_SIN),
	mapstsh::value_type("saw", TEX_SAW),
	mapstsh::value_type("tri", TEX_TRI)
};
static const mapstsh noiseShape(noiseShapeInit, noiseShapeInit + 4);
short BlenderTexture3D::GetNoiseShape(const string &name)
{
	return GetValue(noiseShape, "noise shape", name);
}

}
