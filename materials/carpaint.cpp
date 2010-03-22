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

// carpaint.cpp*

// Simulate car paint - adopted from Gunther et al, "Effcient Acquisition and Realistic Rendering of Car Paint", 2005

#include "carpaint.h"
#include "memory.h"
#include "bxdf.h"
#include "blinn.h"
#include "fresnelslick.h"
#include "lambertian.h"
#include "microfacet.h"
#include "textures/constant.h"
#include "fresnelblend.h"
#include "paramset.h"
#include "dynload.h"

using namespace lux;

CarPaint::CarPaint(boost::shared_ptr<Texture<SWCSpectrum> > &kd,
	boost::shared_ptr<Texture<SWCSpectrum> > &ka,
	boost::shared_ptr<Texture<float> > &d,
	boost::shared_ptr<Texture<SWCSpectrum> > &ks1,
	boost::shared_ptr<Texture<SWCSpectrum> > &ks2,
	boost::shared_ptr<Texture<SWCSpectrum> > &ks3,
	boost::shared_ptr<Texture<float> > &r1,
	boost::shared_ptr<Texture<float> > &r2,
	boost::shared_ptr<Texture<float> > &r3,
	boost::shared_ptr<Texture<float> > &m1,
	boost::shared_ptr<Texture<float> > &m2,
	boost::shared_ptr<Texture<float> > &m3,
	boost::shared_ptr<Texture<float> > &bump,
	const CompositingParams &cp) :
	Kd(kd), Ka(ka), Ks1(ks1), Ks2(ks2), Ks3(ks3), depth(d), R1(r1), R2(r2),
	R3(r3), M1(m1), M2(m2), M3(m3), bumpMap(bump)
{
	compParams = new CompositingParams(cp);
}

// CarPaint Method Definitions
BSDF *CarPaint::GetBSDF(const TsPack *tspack,
	const DifferentialGeometry &dgGeom,
	const DifferentialGeometry &dgs,
	const Volume *exterior, const Volume *interior) const {

	// Allocate _BSDF_
	MultiBSDF *bsdf = ARENA_ALLOC(tspack->arena, MultiBSDF)(dgs, dgGeom.nn,
		exterior, interior);

	// The Carpaint BRDF is really a Multi-lobe Microfacet model with a Lambertian base
	// NOTE - lordcrc - changed clamping to 0..1 to avoid >1 reflection
	const SWCSpectrum kd(Kd->Evaluate(tspack, dgs).Clamp(0.f, 1.f));
	const SWCSpectrum ka(Ka->Evaluate(tspack, dgs).Clamp(0.f, 1.f));
	const float ld = depth->Evaluate(tspack, dgs);

	const SWCSpectrum ks1(Ks1->Evaluate(tspack, dgs).Clamp(0.f, 1.f));
	const float r1 = Clamp(R1->Evaluate(tspack, dgs), 0.f, 1.f);
	const float m1 = M1->Evaluate(tspack, dgs);
	if (ks1.Filter(tspack) > 0.f && m1 > 0.f) {
		MicrofacetDistribution *md1 = ARENA_ALLOC(tspack->arena, Blinn)((2.f * M_PI / (m1 * m1)) - 1.f);
		Fresnel *fr1 = ARENA_ALLOC(tspack->arena, FresnelSlick)(r1, 0.f);
		bsdf->Add(ARENA_ALLOC(tspack->arena, MicrofacetReflection)(ks1, fr1, md1));
	}

	const SWCSpectrum ks2(Ks2->Evaluate(tspack, dgs).Clamp(0.f, 1.f));
	const float r2 = Clamp(R2->Evaluate(tspack, dgs), 0.f, 1.f);
	const float m2 = M2->Evaluate(tspack, dgs);
	if (ks2.Filter(tspack) > 0.f && m2 > 0.f) {
		MicrofacetDistribution *md2 = ARENA_ALLOC(tspack->arena, Blinn)((2.f * M_PI / (m2 * m2)) - 1.f);
		Fresnel *fr2 = ARENA_ALLOC(tspack->arena, FresnelSlick)(r2, 0.f);
		bsdf->Add(ARENA_ALLOC(tspack->arena, MicrofacetReflection)(ks2, fr2, md2));
	}

	const SWCSpectrum ks3(Ks3->Evaluate(tspack, dgs).Clamp(0.f, 1.f));
	const float r3 = Clamp(R3->Evaluate(tspack, dgs), 0.f, 1.f);
	const float m3 = M3->Evaluate(tspack, dgs);
	if (ks3.Filter(tspack) > 0.f && m3 > 0.f) {
		MicrofacetDistribution *md3 = ARENA_ALLOC(tspack->arena, Blinn)((2.f * M_PI / (m3 * m3)) - 1.f);
		// The fresnel function is created by the FresnelBlend model
		//Fresnel *fr3 = ARENA_ALLOC(tspack->arena, FresnelSlick)(r3, 0.f);
		// Clear coat and lambertian base
		bsdf->Add(ARENA_ALLOC(tspack->arena, FresnelBlend)(kd, ks3 * r3, ka, ld, md3));
	} else {
		// Lambertian base only
		bsdf->Add(ARENA_ALLOC(tspack->arena, Lambertian)(kd));
	}

	// Add ptr to CompositingParams structure
	bsdf->SetCompositingParams(compParams);

	return bsdf;
}

void DataFromName(const string name,
	boost::shared_ptr<Texture<SWCSpectrum> > *Kd,
	boost::shared_ptr<Texture<SWCSpectrum> > *Ks1,
	boost::shared_ptr<Texture<SWCSpectrum> > *Ks2,
	boost::shared_ptr<Texture<SWCSpectrum> > *Ks3,
	boost::shared_ptr<Texture<float> > *R1,
	boost::shared_ptr<Texture<float> > *R2,
	boost::shared_ptr<Texture<float> > *R3,
	boost::shared_ptr<Texture<float> > *M1,
	boost::shared_ptr<Texture<float> > *M2,
	boost::shared_ptr<Texture<float> > *M3)
{
	int numPaints = sizeof(carpaintdata) / sizeof(CarPaintData);

	// default (Ford F8)

	int i;

	for (i = 0; i < numPaints; i++) {
		if (name.compare(carpaintdata[i].name) == 0)
			break;
	}

	boost::shared_ptr<Texture<SWCSpectrum> > kd (new ConstantRGBColorTexture(carpaintdata[i].kd));
	boost::shared_ptr<Texture<SWCSpectrum> > ks1 (new ConstantRGBColorTexture(carpaintdata[i].ks1));
	boost::shared_ptr<Texture<SWCSpectrum> > ks2 (new ConstantRGBColorTexture(carpaintdata[i].ks2));
	boost::shared_ptr<Texture<SWCSpectrum> > ks3 (new ConstantRGBColorTexture(carpaintdata[i].ks3));
	boost::shared_ptr<Texture<float> > r1 (new ConstantFloatTexture(carpaintdata[i].r1));
	boost::shared_ptr<Texture<float> > r2 (new ConstantFloatTexture(carpaintdata[i].r2));
	boost::shared_ptr<Texture<float> > r3 (new ConstantFloatTexture(carpaintdata[i].r3));
	boost::shared_ptr<Texture<float> > m1 (new ConstantFloatTexture(carpaintdata[i].m1));
	boost::shared_ptr<Texture<float> > m2 (new ConstantFloatTexture(carpaintdata[i].m2));
	boost::shared_ptr<Texture<float> > m3 (new ConstantFloatTexture(carpaintdata[i].m3));

	*Kd = kd;
	*Ks1 = ks1;
	*Ks2 = ks2;
	*Ks3 = ks3;
	*R1 = r1;
	*R2 = r2;
	*R3 = r3;
	*M1 = m1;
	*M2 = m2;
	*M3 = m3;
}

Material* CarPaint::CreateMaterial(const Transform &xform, const ParamSet &mp)
{

	// Default values for missing parameters is from the Ford F8 dataset
	float def_kd[3], def_ks1[3], def_ks2[3], def_ks3[3], def_r[3], def_m[3];

	for (int i = 0; i < 3; i++) {
		def_kd[i] = carpaintdata[0].kd[i];
		def_ks1[i] = carpaintdata[0].ks1[i];
		def_ks2[i] = carpaintdata[0].ks2[i];
		def_ks3[i] = carpaintdata[0].ks3[i];
	}

	def_r[0] = carpaintdata[0].r1;
	def_r[1] = carpaintdata[0].r2;
	def_r[2] = carpaintdata[0].r3;

	def_m[0] = carpaintdata[0].m1;
	def_m[1] = carpaintdata[0].m2;
	def_m[2] = carpaintdata[0].m3;

	string paintname = mp.FindOneString("name", "");

	boost::shared_ptr<Texture<SWCSpectrum> > Ka(mp.GetSWCSpectrumTexture("Ka", RGBColor(0.f)));
	boost::shared_ptr<Texture<float> > d(mp.GetFloatTexture("d", 0.f));

	boost::shared_ptr<Texture<SWCSpectrum> > Kd;
	boost::shared_ptr<Texture<SWCSpectrum> > Ks1;
	boost::shared_ptr<Texture<SWCSpectrum> > Ks2;
	boost::shared_ptr<Texture<SWCSpectrum> > Ks3;

	boost::shared_ptr<Texture<float> > R1;
	boost::shared_ptr<Texture<float> > R2;
	boost::shared_ptr<Texture<float> > R3;

	boost::shared_ptr<Texture<float> > M1;
	boost::shared_ptr<Texture<float> > M2;
	boost::shared_ptr<Texture<float> > M3;
	if (paintname == "") {
		// we got no name, so try to read material properties directly
		boost::shared_ptr<Texture<SWCSpectrum> > kd(mp.GetSWCSpectrumTexture("Kd", RGBColor(def_kd)));
		boost::shared_ptr<Texture<SWCSpectrum> > ks1(mp.GetSWCSpectrumTexture("Ks1", RGBColor(def_ks1)));
		boost::shared_ptr<Texture<SWCSpectrum> > ks2(mp.GetSWCSpectrumTexture("Ks2", RGBColor(def_ks2)));
		boost::shared_ptr<Texture<SWCSpectrum> > ks3(mp.GetSWCSpectrumTexture("Ks3", RGBColor(def_ks3)));

		boost::shared_ptr<Texture<float> > r1(mp.GetFloatTexture("R1", def_r[0]));
		boost::shared_ptr<Texture<float> > r2(mp.GetFloatTexture("R2", def_r[1]));
		boost::shared_ptr<Texture<float> > r3(mp.GetFloatTexture("R3", def_r[2]));

		boost::shared_ptr<Texture<float> > m1(mp.GetFloatTexture("M1", def_m[0]));
		boost::shared_ptr<Texture<float> > m2(mp.GetFloatTexture("M2", def_m[1]));
		boost::shared_ptr<Texture<float> > m3(mp.GetFloatTexture("M3", def_m[2]));

		Kd = kd;
		Ks1 = ks1;
		Ks2 = ks2;
		Ks3 = ks3;
		R1 = r1;
		R2 = r2;
		R3 = r3;
		M1 = m1;
		M2 = m2;
		M3 = m3;
	} else
		// Pick from presets, fall back to the first if name not found
		DataFromName(paintname, &Kd, &Ks1, &Ks2, &Ks3, &R1, &R2, &R3, &M1, &M2, &M3);

	boost::shared_ptr<Texture<float> > bumpMap(mp.GetFloatTexture("bumpmap"));

	// Get Compositing Params
	CompositingParams cP;
	FindCompositingParams(mp, &cP);

	return new CarPaint(Kd, Ka, d, Ks1, Ks2, Ks3, R1, R2, R3, M1, M2, M3, bumpMap, cP);
}

static DynamicLoader::RegisterMaterial<CarPaint> r("carpaint");
