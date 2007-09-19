
/*
 * pbrt source code Copyright(c) 1998-2007 Matt Pharr and Greg Humphreys
 *
 * All Rights Reserved.
 * For educational use only; commercial use expressly forbidden.
 * NO WARRANTY, express or implied, for this software.
 * (See file License.txt for complete license)
 */

// marble.cpp*
#include "pbrt.h"
#include "texture.h"
#include "paramset.h"
// MarbleTexture Declarations
class MarbleTexture : public Texture<Spectrum> {
public:
	// MarbleTexture Public Methods
	~MarbleTexture() {
		delete mapping;
	}
	MarbleTexture(int oct, float roughness, float sc, float var,
			TextureMapping3D *map) {
		omega = roughness;
		octaves = oct;
		mapping = map;
		scale = sc;
		variation = var;
	}
	Spectrum Evaluate(const DifferentialGeometry &dg) const {
		Vector dpdx, dpdy;
		Point P = mapping->Map(dg, &dpdx, &dpdy);
		P *= scale;
		float marble = P.y + variation * FBm(P, scale * dpdx,
			scale * dpdy, omega, octaves);
		float t = .5f + .5f * sinf(marble);
		// Evaluate marble spline at _t_
		static float c[][3] = { { .58f, .58f, .6f }, { .58f, .58f, .6f }, { .58f, .58f, .6f },
			{ .5f, .5f, .5f }, { .6f, .59f, .58f }, { .58f, .58f, .6f },
			{ .58f, .58f, .6f }, {.2f, .2f, .33f }, { .58f, .58f, .6f }, };
		#define NC  sizeof(c) / sizeof(c[0])
		#define NSEG (NC-3)
		int first = Floor2Int(t * NSEG);
		t = (t * NSEG - first);
		Spectrum c0(c[first]), c1(c[first+1]), c2(c[first+2]), c3(c[first+3]);
		// Bezier spline evaluated with de Castilejau's algorithm
		Spectrum s0 = (1.f - t) * c0 + t * c1;
		Spectrum s1 = (1.f - t) * c1 + t * c2;
		Spectrum s2 = (1.f - t) * c2 + t * c3;
		s0 = (1.f - t) * s0 + t * s1;
		s1 = (1.f - t) * s1 + t * s2;
		// Extra scale of 1.5 to increase variation among colors
		return 1.5f * ((1.f - t) * s0 + t * s1);
	}
private:
	// MarbleTexture Private Data
	int octaves;
	float omega, scale, variation;
	TextureMapping3D *mapping;
};
// MarbleTexture Method Definitions
extern "C" DLLEXPORT Texture<float> * CreateFloatTexture(const Transform &tex2world,
		const TextureParams &tp) {
	return NULL;
}

extern "C" DLLEXPORT Texture<Spectrum> * CreateSpectrumTexture(const Transform &tex2world,
		const TextureParams &tp) {
	// Initialize 3D texture mapping _map_ from _tp_
	TextureMapping3D *map = new IdentityMapping3D(tex2world);
	return new MarbleTexture(tp.FindInt("octaves", 8),
		tp.FindFloat("roughness", .5f),
		tp.FindFloat("scale", 1.f),
		tp.FindFloat("variation", .2f),
		map);
}
