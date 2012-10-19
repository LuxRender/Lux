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
 *   Lux Renderer website : http://www.luxrender.org                       *
 ***************************************************************************/

// cloth.cpp*

/*
 * This code is adapted from Mitsuba renderer by Wenzel Jakob (http://www.mitsuba-renderer.org/)
 * by permission.
 *
 * This file implements the Irawan & Marschner BRDF,
 * a realistic model for rendering woven materials.
 * This spatially-varying reflectance model uses an explicit description
 * of the underlying weave pattern to create fine-scale texture and
 * realistic reflections across a wide range of different weave types.
 * To use the model, you must provide a special weave pattern
 * file---for an example of what these look like, see the
 * examples scenes available on the Mitsuba website.
 *
 * For reference, it is described in detail in the PhD thesis of
 * Piti Irawan ("The Appearance of Woven Cloth").
 *
 * The code in Mitsuba is a modified port of a previous Java implementation
 * by Piti.
 *
 */


#include "cloth.h"
#include "memory.h"
#include "singlebsdf.h"
#include "primitive.h"
#include "texture.h"
#include "color.h"
#include "paramset.h"
#include "dynload.h"
#include "randomgen.h"
#include "irawan.h"
#include "mc.h"

using namespace lux;

boost::shared_ptr<WeavePattern> DenimPattern(const float repeat_u, const float repeat_v) {
	boost::shared_ptr<WeavePattern> pattern(new WeavePattern((std::string)"Cotton denmim", 3, 6, 0.01f,4.0f, 0.0f, 0.5f, 5.0f, 1.0f, 3.0f, repeat_u,repeat_v, 0.0f,0.0f,0.0f,0.0f, 0.0f));

	int patterns[] = {
		1, 3, 8,
		1, 3, 5,
		1, 7, 5,
		1, 4, 5,
		6, 4, 5,
		2, 4, 5
	};

	for (u_int i = 0; i < pattern->tileHeight * pattern->tileWidth; i++)
		pattern->pattern.push_back(patterns[i]);

	pattern->yarns.push_back(new Yarn(Yarn::EWarp, -30, 12, 0, 1, 5, 0.1667, 0.75));
	pattern->yarns.push_back(new Yarn(Yarn::EWarp, -30, 12, 0, 1, 5, 0.1667, -0.25));
	pattern->yarns.push_back(new Yarn(Yarn::EWarp, -30, 12, 0, 1, 5, 0.5, 1.0833));
	pattern->yarns.push_back(new Yarn(Yarn::EWarp, -30, 12, 0, 1, 5, 0.5, 0.0833));
	pattern->yarns.push_back(new Yarn(Yarn::EWarp, -30, 12, 0, 1, 5, 0.8333, 0.4167));
	pattern->yarns.push_back(new Yarn(Yarn::EWeft, -30, 38, 0, 1, 1, 0.1667, 0.25));
	pattern->yarns.push_back(new Yarn(Yarn::EWeft, -30, 38, 0, 1, 1, 0.5, 0.5833));
	pattern->yarns.push_back(new Yarn(Yarn::EWeft, -30, 38, 0, 1, 1, 0.8333, 0.9167));

	return pattern;
}

boost::shared_ptr<WeavePattern> SilkCharmeusePattern(const float repeat_u, const float repeat_v) {
	boost::shared_ptr<WeavePattern> pattern(new WeavePattern((std::string)"Silk charmeuse", 5, 10, 0.02f,7.3f, 0.5f, 0.5f, 9.0f, 1.0f, 3.0f, repeat_u,repeat_v, 0.0f,0.0f,0.0f,0.0f, 0.0f));

	int patterns[] = {
		10, 2,  4,  6,  8,
		1,  2,  4,  6,  8,
		1,  2,  4, 13,  8,
		1,  2,  4,  7,  8,
		1, 11,  4,  7,  8,
		1,  3,  4,  7,  8,
		1,  3,  4,  7, 14,
		1,  3,  4,  7,  9,
		1,  3, 12,  7,  9,
		1,  3,  5,  7,  9
	};

	for (u_int i = 0; i < pattern->tileHeight * pattern->tileWidth; i++)
		pattern->pattern.push_back(patterns[i]);

	pattern->yarns.push_back(new Yarn(Yarn::EWarp, 0, 40, 2, 1, 9, 0.1, 0.45));
	pattern->yarns.push_back(new Yarn(Yarn::EWarp, 0, 40, 2, 1, 9, 0.3, 1.05));
	pattern->yarns.push_back(new Yarn(Yarn::EWarp, 0, 40, 2, 1, 9, 0.3, 0.05));
	pattern->yarns.push_back(new Yarn(Yarn::EWarp, 0, 40, 2, 1, 9, 0.5, 0.65));
	pattern->yarns.push_back(new Yarn(Yarn::EWarp, 0, 40, 2, 1, 9, 0.5, -0.35));
	pattern->yarns.push_back(new Yarn(Yarn::EWarp, 0, 40, 2, 1, 9, 0.7, 1.25));
	pattern->yarns.push_back(new Yarn(Yarn::EWarp, 0, 40, 2, 1, 9, 0.7, 0.25));
	pattern->yarns.push_back(new Yarn(Yarn::EWarp, 0, 40, 2, 1, 9, 0.9, 0.85));
	pattern->yarns.push_back(new Yarn(Yarn::EWarp, 0, 40, 2, 1, 9, 0.9, -0.15));
	pattern->yarns.push_back(new Yarn(Yarn::EWeft, 0, 60, 0, 1, 1, 0.1, 0.95));
	pattern->yarns.push_back(new Yarn(Yarn::EWeft, 0, 60, 0, 1, 1, 0.3, 0.55));
	pattern->yarns.push_back(new Yarn(Yarn::EWeft, 0, 60, 0, 1, 1, 0.5, 0.15));
	pattern->yarns.push_back(new Yarn(Yarn::EWeft, 0, 60, 0, 1, 1, 0.7, 0.75));
	pattern->yarns.push_back(new Yarn(Yarn::EWeft, 0, 60, 0, 1, 1, 0.9, 0.35));

	return pattern;
}

boost::shared_ptr<WeavePattern> CottonTwillPattern(const float repeat_u, const float repeat_v) {
	boost::shared_ptr<WeavePattern> pattern(new WeavePattern((std::string)"Cotton twill", 4, 8, 0.01f,4.0f, 0.0f, 0.5f, 6.0f, 2.0f, 4.0f, repeat_u,repeat_v, 0.0f,0.0f,0.0f,0.0f, 0.0f));

	int patterns[] = {
		7, 2, 4, 6,
		7, 2, 4, 6,
		1, 8, 4, 6,
		1, 8, 4, 6,
		1, 3, 9, 6,
		1, 3, 9, 6,
		1, 3, 5, 10,
		1, 3, 5, 10
	};

	for (u_int i = 0; i < pattern->tileHeight * pattern->tileWidth; i++)
		pattern->pattern.push_back(patterns[i]);

	pattern->yarns.push_back(new Yarn(Yarn::EWarp, -30, 24, 0, 1, 6, 0.125, 0.375));
	pattern->yarns.push_back(new Yarn(Yarn::EWarp, -30, 24, 0, 1, 6, 0.375, 1.125));
	pattern->yarns.push_back(new Yarn(Yarn::EWarp, -30, 24, 0, 1, 6, 0.375, 0.125));
	pattern->yarns.push_back(new Yarn(Yarn::EWarp, -30, 24, 0, 1, 6, 0.625, 0.875));
	pattern->yarns.push_back(new Yarn(Yarn::EWarp, -30, 24, 0, 1, 6, 0.625, -0.125));
	pattern->yarns.push_back(new Yarn(Yarn::EWarp, -30, 24, 0, 1, 6, 0.875, 0.625));
	pattern->yarns.push_back(new Yarn(Yarn::EWeft, -30, 36, 0, 2, 1, 0.125, 0.875));
	pattern->yarns.push_back(new Yarn(Yarn::EWeft, -30, 36, 0, 2, 1, 0.375, 0.625));
	pattern->yarns.push_back(new Yarn(Yarn::EWeft, -30, 36, 0, 2, 1, 0.625, 0.375));
	pattern->yarns.push_back(new Yarn(Yarn::EWeft, -30, 36, 0, 2, 1, 0.875, 0.125));

	return pattern;
}

boost::shared_ptr<WeavePattern> WoolGabardinePattern(const float repeat_u, const float repeat_v) {
	boost::shared_ptr<WeavePattern> pattern(new WeavePattern((std::string)"Wool gabardine", 6, 9, 0.01f,4.0f, 0.0f, 0.5f, 12.0f, 6.0f, 0.0f, repeat_u,repeat_v, 0.0f,0.0f,0.0f,0.0f, 0.0f));

	int patterns[] = {
		1, 1, 2, 2, 7, 7,
		1, 1, 2, 2, 7, 7,
		1, 1, 2, 2, 7, 7,
		1, 1, 6, 6, 4, 4,
		1, 1, 6, 6, 4, 4,
		1, 1, 6, 6, 4, 4,
		5, 5, 3, 3, 4, 4,
		5, 5, 3, 3, 4, 4,
		5, 5, 3, 3, 4, 4
	};

	for (u_int i = 0; i < pattern->tileHeight * pattern->tileWidth; i++)
		pattern->pattern.push_back(patterns[i]);

	pattern->yarns.push_back(new Yarn(Yarn::EWarp, 30, 30, 0, 2, 6, 0.167, 0.667));
	pattern->yarns.push_back(new Yarn(Yarn::EWarp, 30, 30, 0, 2, 6, 0.5, 1.0));
	pattern->yarns.push_back(new Yarn(Yarn::EWarp, 30, 30, 0, 2, 6, 0.5, 0.0));
	pattern->yarns.push_back(new Yarn(Yarn::EWarp, 30, 30, 0, 2, 6, 0.833, 0.333));
	pattern->yarns.push_back(new Yarn(Yarn::EWeft, 30, 30, 0, 3, 2, 0.167, 0.167));
	pattern->yarns.push_back(new Yarn(Yarn::EWeft, 30, 30, 0, 3, 2, 0.5, 0.5));
	pattern->yarns.push_back(new Yarn(Yarn::EWeft, 30, 30, 0, 3, 2, 0.833, 0.833));

	return pattern;
}

boost::shared_ptr<WeavePattern> PolyesterLiningClothPattern(const float repeat_u, const float repeat_v) {
	boost::shared_ptr<WeavePattern> pattern(new WeavePattern((std::string)"Polyester lining cloth", 2, 2, 0.015f,4.0f, 0.5f, 0.5f, 1.0f, 1.0f, 0.0f, repeat_u,repeat_v, 8.0f,8.0f,6.0f,6.0f, 50.0f));

	int patterns[] = {
		3, 2,
		1, 4
	};

	for (u_int i = 0; i < pattern->tileHeight * pattern->tileWidth; i++)
		pattern->pattern.push_back(patterns[i]);

	pattern->yarns.push_back(new Yarn(Yarn::EWarp, 0, 22, -0.7, 1, 1, 0.25, 0.25));
	pattern->yarns.push_back(new Yarn(Yarn::EWarp, 0, 22, -0.7, 1, 1, 0.75, 0.75));
	pattern->yarns.push_back(new Yarn(Yarn::EWeft, 0, 16, -0.7, 1, 1, 0.25, 0.75));
	pattern->yarns.push_back(new Yarn(Yarn::EWeft, 0, 16, -0.7, 1, 1, 0.75, 0.25));

	return pattern;
}

boost::shared_ptr<WeavePattern> SilkShantungPattern(const float repeat_u, const float repeat_v) {
	boost::shared_ptr<WeavePattern> pattern(new WeavePattern((std::string)"Polyester lining cloth", 6, 8, 0.02f,1.5f, 0.5f, 0.5f, 8.0f, 16.0f, 0.0f, repeat_u,repeat_v, 20.0f,20.0f,10.0f,10.0f, 500.0f));

	int patterns[] = {
		3, 3, 3, 3, 2, 2,
		3, 3, 3, 3, 2, 2,
		3, 3, 3, 3, 2, 2,
		3, 3, 3, 3, 2, 2,
		4, 1, 1, 5, 5, 5,
		4, 1, 1, 5, 5, 5,
		4, 1, 1, 5, 5, 5,
		4, 1, 1, 5, 5, 5
	};

	for (u_int i = 0; i < pattern->tileHeight * pattern->tileWidth; i++)
		pattern->pattern.push_back(patterns[i]);

	pattern->yarns.push_back(new Yarn(Yarn::EWarp, 0, 50, -0.5, 2, 4, 0.3333, 0.25));
	pattern->yarns.push_back(new Yarn(Yarn::EWarp, 0, 50, -0.5, 2, 4, 0.8333, 0.75));
	pattern->yarns.push_back(new Yarn(Yarn::EWeft, 0, 23, -0.3, 4, 4, 0.3333, 0.75));
	pattern->yarns.push_back(new Yarn(Yarn::EWeft, 0, 23, -0.3, 4, 4, -0.1667, 0.25));
	pattern->yarns.push_back(new Yarn(Yarn::EWeft, 0, 23, -0.3, 4, 4, 0.8333, 0.25));

	return pattern;
}

Cloth::Cloth(boost::shared_ptr<Texture<SWCSpectrum> > &warp_kd,
			boost::shared_ptr<Texture<SWCSpectrum> > &warp_ks,
			boost::shared_ptr<Texture<SWCSpectrum> > &weft_kd,
			boost::shared_ptr<Texture<SWCSpectrum> > &weft_ks,
			boost::shared_ptr<WeavePattern> &pattern,
			const ParamSet &mp) : Material(mp), warp_Kd(warp_kd), warp_Ks(warp_ks), weft_Kd(weft_kd), weft_Ks(weft_ks),
			Pattern(pattern) {

		/* Estimate the average reflectance under diffuse illumination and use it to normalize the specular component */

		RandomGenerator *random = new RandomGenerator();
		u_int nSamples = 100000;

		SWCSpectrum s,result(0.0f);

		Irawan *irawan = new Irawan(s,s,s,s,random->floatValue(), random->floatValue(), 0.0, Pattern);

		for (u_int i=0; i < nSamples; ++i) {
			SWCSpectrum f;
			const Vector wi = CosineSampleHemisphere(random->floatValue(), random->floatValue());
			const Vector wo = CosineSampleHemisphere(random->floatValue(), random->floatValue());

			irawan->eval(wo, wi, random->floatValue(), random->floatValue(), &f, true);
			result += f * (1.0 / CosTheta(wo));
		}

		if (result.Black())
			specularNormalization = 0;
		else
			specularNormalization = nSamples / (result.Max() * M_PI);

		delete irawan;
		delete random;
}

// CarPaint Method Definitions
BSDF *Cloth::GetBSDF(MemoryArena &arena, const SpectrumWavelengths &sw,
	const Intersection &isect, const DifferentialGeometry &dgs) const
{
	SWCSpectrum warp_kd = warp_Kd->Evaluate(sw, dgs).Clamp(0.f, 1.f);
	SWCSpectrum warp_ks = warp_Ks->Evaluate(sw, dgs).Clamp(0.f, 1.f);
	SWCSpectrum weft_kd = weft_Kd->Evaluate(sw, dgs).Clamp(0.f, 1.f);
	SWCSpectrum weft_ks = weft_Ks->Evaluate(sw, dgs).Clamp(0.f, 1.f);

	// Allocate _BSDF_
	BxDF *bxdf = ARENA_ALLOC(arena, Irawan)(warp_kd, warp_ks, weft_kd, weft_ks, dgs.u, dgs.v, specularNormalization, Pattern);

	SingleBSDF *bsdf = ARENA_ALLOC(arena, SingleBSDF)(dgs, isect.dg.nn, bxdf, isect.exterior, isect.interior);

	// Add ptr to CompositingParams structure
	bsdf->SetCompositingParams(&compParams);

	//return bsdf;
	return bsdf;
}

Material* Cloth::CreateMaterial(const Transform &xform, const ParamSet &mp)
{
	string presetname = mp.FindOneString("presetname", "denim");

	boost::shared_ptr<Texture<SWCSpectrum> > warp_kd(mp.GetSWCSpectrumTexture("warp_Kd", RGBColor(0.f)));
	boost::shared_ptr<Texture<SWCSpectrum> > warp_ks(mp.GetSWCSpectrumTexture("warp_Ks", RGBColor(0.f)));
	boost::shared_ptr<Texture<SWCSpectrum> > weft_kd(mp.GetSWCSpectrumTexture("weft_Kd", RGBColor(0.f)));
	boost::shared_ptr<Texture<SWCSpectrum> > weft_ks(mp.GetSWCSpectrumTexture("weft_Ks", RGBColor(0.f)));
	float repeat_u = mp.FindOneFloat("repeat_u", 100.f);
	float repeat_v = mp.FindOneFloat("repeat_v", 100.f);

	boost::shared_ptr<WeavePattern> pattern;
	if (presetname == "silk_charmeuse")
		pattern = SilkCharmeusePattern(repeat_u, repeat_v);
	else if (presetname == "denim")
		pattern = DenimPattern(repeat_u, repeat_v);
	else if (presetname == "cotton_twill")
		pattern = CottonTwillPattern(repeat_u, repeat_v);
	else if (presetname == "wool_gabardine")
		pattern = WoolGabardinePattern(repeat_u, repeat_v);
	else if (presetname == "polyester_lining_cloth")
		pattern = PolyesterLiningClothPattern(repeat_u, repeat_v);
	else if (presetname == "silk_shantung")
		pattern = SilkShantungPattern(repeat_u, repeat_v);
	else
		pattern = DenimPattern(repeat_u, repeat_v);

	return new Cloth(warp_kd, warp_ks, weft_kd, weft_ks, pattern, mp);
}

static DynamicLoader::RegisterMaterial<Cloth> r("cloth");

