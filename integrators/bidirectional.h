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
#include "transport.h"
#include "scene.h"
#include "mc.h"
// Bidirectional Local Declarations
struct BidirVertex;
class BidirIntegrator : public SurfaceIntegrator {
public:
	// BidirIntegrator Public Methods
	Spectrum Li(const Scene *scene, const RayDifferential &ray, const Sample *sample, float *alpha) const;
	void RequestSamples(Sample *sample, const Scene *scene);
	
	static SurfaceIntegrator *CreateSurfaceIntegrator(const ParamSet &params);
private:
	// BidirIntegrator Private Methods
	int generatePath(const Scene *scene, const Ray &r, const Sample *sample,
		const int *bsdfOffset, const int *bsdfCompOffset,
		BidirVertex *vertices, int maxVerts) const;
	float weightPath(BidirVertex *eye, int nEye, BidirVertex *light, int nLight) const;
	Spectrum evalPath(const Scene *scene, BidirVertex *eye, int nEye,
	BidirVertex *light, int nLight) const;
	static float G(const BidirVertex &v0, const BidirVertex &v1);
	static bool visible(const Scene *scene, const Point &P0, const Point &P1);
	// BidirIntegrator Data
	#define MAX_VERTS 4
	int eyeBSDFOffset[MAX_VERTS], eyeBSDFCompOffset[MAX_VERTS];
	int lightBSDFOffset[MAX_VERTS], lightBSDFCompOffset[MAX_VERTS];
	int directLightOffset[MAX_VERTS], directLightNumOffset[MAX_VERTS];
	int directBSDFOffset[MAX_VERTS], directBSDFCompOffset[MAX_VERTS];
	int lightNumOffset, lightPosOffset, lightDirOffset;
};
struct BidirVertex {
	BidirVertex() { bsdfWeight = dAWeight = 0.; rrWeight = 1.;
		flags = BxDFType(0); bsdf = NULL; }
	BSDF *bsdf;
	Point p;
	Normal ng, ns;
	Vector wi, wo;
	float bsdfWeight, dAWeight, rrWeight;
	BxDFType flags;
};
