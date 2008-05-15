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

// bidirectional.cpp*
#include "lux.h"
#include "reflection/bxdf.h"
#include "transport.h"
#include "scene.h"
#include "mc.h"

namespace lux
{

// Bidirectional Local Declarations
struct BidirVertex;
class BidirIntegrator : public SurfaceIntegrator {
public:
	BidirIntegrator(int ed, int ld) : maxEyeDepth(ed), maxLightDepth(ld) {}
	// BidirIntegrator Public Methods
	SWCSpectrum Li(const Scene *scene, const RayDifferential &ray, const Sample *sample, float *alpha) const;
	void RequestSamples(Sample *sample, const Scene *scene);
	static SurfaceIntegrator *CreateSurfaceIntegrator(const ParamSet &params);
private:
	// BidirIntegrator Private Methods
	int generatePath(const Scene *scene, const Ray &r, const Sample *sample,
		int sampleOffset,
		vector<BidirVertex> &vertices) const;
	float weightPath(vector<BidirVertex> &eye, int nEye, vector<BidirVertex> &light, int nLight, float pdfLight, bool directLight) const;
	SWCSpectrum evalPath(const Scene *scene, vector<BidirVertex> &eye, int nEye,
	vector<BidirVertex> &light, int nLight) const;
	static float G(const BidirVertex &v0, const BidirVertex &v1);
	static bool visible(const Scene *scene, const Point &P0, const Point &P1);
	// BidirIntegrator Data
	int maxEyeDepth, maxLightDepth;
	int lightNumOffset, lightPosOffset, lightDirOffset;
	int sampleEyeOffset, sampleLightOffset, sampleDirectOffset;
};
struct BidirVertex {
	BidirVertex() : bsdf(NULL), bsdfWeight(0.f), dAWeight(0.f),
		rrWeight(1.f), flags(BxDFType(0)), f(0.f), Le(0.f) {}
	BSDF *bsdf;
	Point p;
	Normal ng, ns;
	Vector wi, wo;
	float bsdfWeight, dAWeight, rrWeight, dARWeight;
	BxDFType flags;
	SWCSpectrum f, Le;
};

}//namespace lux

