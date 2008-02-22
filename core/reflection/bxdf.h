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

#ifndef LUX_BXDF_H
#define LUX_BXDF_H
// bxdf.h*
#include "lux.h"
#include "geometry.h"
#include "shape.h"
#include "spectrum.h"

namespace lux
{

// BSDF Inline Functions
inline float CosTheta(const Vector &w) { return w.z; }
inline float SinTheta(const Vector &w) {
	return sqrtf(max(0.f, 1.f - w.z*w.z));
}
inline float SinTheta2(const Vector &w) {
	return 1.f - CosTheta(w)*CosTheta(w);
}
inline float CosPhi(const Vector &w) {
	return w.x / SinTheta(w);
}
inline float SinPhi(const Vector &w) {
	return w.y / SinTheta(w);
}
inline bool SameHemisphere(const Vector &w,
                          const Vector &wp) {
	return w.z * wp.z > 0.f;
}
// BSDF Declarations
enum BxDFType {
	BSDF_REFLECTION   = 1<<0,
	BSDF_TRANSMISSION = 1<<1,
	BSDF_DIFFUSE      = 1<<2,
	BSDF_GLOSSY       = 1<<3,
	BSDF_SPECULAR     = 1<<4,
	BSDF_ALL_TYPES        = BSDF_DIFFUSE |
	                        BSDF_GLOSSY |
					        BSDF_SPECULAR,
	BSDF_ALL_REFLECTION   = BSDF_REFLECTION |
	                        BSDF_ALL_TYPES,
	BSDF_ALL_TRANSMISSION = BSDF_TRANSMISSION |
	                        BSDF_ALL_TYPES,
	BSDF_ALL              = BSDF_ALL_REFLECTION |
	                        BSDF_ALL_TRANSMISSION
};
class  BSDF {
public:
	// BSDF Public Methods
	SWCSpectrum Sample_f(const Vector &o, Vector *wi, float u1, float u2,
		float u3, float *pdf, BxDFType flags = BSDF_ALL,
		BxDFType *sampledType = NULL) const;
	SWCSpectrum Sample_f(const Vector &wo, Vector *wi,
		BxDFType flags = BSDF_ALL,
		BxDFType *sampledType = NULL) const;
	float Pdf(const Vector &wo,
	          const Vector &wi,
			  BxDFType flags = BSDF_ALL) const;
	BSDF(const DifferentialGeometry &dgs,
	     const Normal &ngeom,
		 float eta = 1.f);
	inline void Add(BxDF *bxdf);
	int NumComponents() const { return nBxDFs; }
	int NumComponents(BxDFType flags) const;
	bool HasShadingGeometry() const {
		return (nn.x != ng.x || nn.y != ng.y || nn.z != ng.z);
	}
	Vector WorldToLocal(const Vector &v) const {
		return Vector(Dot(v, sn), Dot(v, tn), Dot(v, nn));
	}
	Vector LocalToWorld(const Vector &v) const {
		return Vector(sn.x * v.x + tn.x * v.y + nn.x * v.z,
		              sn.y * v.x + tn.y * v.y + nn.y * v.z,
		              sn.z * v.x + tn.z * v.y + nn.z * v.z);
	}
	SWCSpectrum f(const Vector &woW, const Vector &wiW,
		BxDFType flags = BSDF_ALL) const;
	SWCSpectrum rho(BxDFType flags = BSDF_ALL) const;
	SWCSpectrum rho(const Vector &wo,
	             BxDFType flags = BSDF_ALL) const;
	static void *Alloc( u_int sz) { return arena->Alloc(sz); }		// TODO remove original memory arena
	static void FreeAll() { arena->FreeAll(); }
	// BSDF Public Data
	const DifferentialGeometry dgShading;
	const float eta;
	
	friend class RenderThread;
	
private:
	// BSDF Private Methods
	~BSDF() { }
	friend class NoSuchClass;
	// BSDF Private Data
	Normal nn, ng;
	Vector sn, tn;
	int nBxDFs;
	#define MAX_BxDFS 8
	BxDF * bxdfs[MAX_BxDFS];
	//static MemoryArena arena;
	static boost::thread_specific_ptr<MemoryArena> arena;
	
};
#define BSDF_ALLOC(T)  new (BSDF::Alloc(sizeof(T))) T
// BxDF Declarations
class  BxDF {
public:
	// BxDF Interface
	virtual ~BxDF() { }
	BxDF(BxDFType t) : type(t) { }
	bool MatchesFlags(BxDFType flags) const {
		return (type & flags) == type;
	}
	virtual SWCSpectrum f(const Vector &wo,
	                   const Vector &wi) const = 0;
	virtual SWCSpectrum Sample_f(const Vector &wo, Vector *wi,
		float u1, float u2, float *pdf) const;
	virtual SWCSpectrum rho(const Vector &wo,
	                     int nSamples = 16,
		                 float *samples = NULL) const;
	virtual SWCSpectrum rho(int nSamples = 16,
	                     float *samples = NULL) const;
	virtual float Pdf(const Vector &wi, const Vector &wo) const;
	// BxDF Public Data
	const BxDFType type;
};
class  BRDFToBTDF : public BxDF {
public:
	// BRDFToBTDF Public Methods
	BRDFToBTDF(BxDF *b)
		: BxDF(BxDFType(b->type ^
		               (BSDF_REFLECTION | BSDF_TRANSMISSION))) {
		brdf = b;
	}
	static Vector otherHemisphere(const Vector &w) {
		return Vector(w.x, w.y, -w.z);
	}
	SWCSpectrum rho(const Vector &w, int nSamples,
			float *samples) const {
		return brdf->rho(otherHemisphere(w), nSamples, samples);
	}
	SWCSpectrum rho(int nSamples, float *samples) const {
		return brdf->rho(nSamples, samples);
	}
	SWCSpectrum f(const Vector &wo, const Vector &wi) const;
	SWCSpectrum Sample_f(const Vector &wo, Vector *wi,
		float u1, float u2, float *pdf) const;
	float Pdf(const Vector &wo, const Vector &wi) const;
private:
	BxDF *brdf;
};

// BSDF Inline Method Definitions
inline void BSDF::Add(BxDF *b) {
	BOOST_ASSERT(nBxDFs < MAX_BxDFS);
	bxdfs[nBxDFs++] = b;
}
inline int BSDF::NumComponents(BxDFType flags) const {
	int num = 0;
	for (int i = 0; i < nBxDFs; ++i)
		if (bxdfs[i]->MatchesFlags(flags)) ++num;
	return num;
}

}//namespace lux

#endif // LUX_BXDF_H