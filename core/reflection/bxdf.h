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

#ifndef LUX_BXDF_H
#define LUX_BXDF_H
// bxdf.h*
#include "lux.h"
#include "geometry/raydifferential.h"
#include "spectrum.h"
#include "memory.h"

namespace lux
{

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
	BSDF(const DifferentialGeometry &dgs, const Normal &ngeom,
		float eta = 1.f);
	virtual int NumComponents() const = 0;
	virtual int NumComponents(BxDFType flags) const = 0;
	virtual inline void SetCompositingParams(CompositingParams *cp) {
		compParams = cp;
	}
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
	/**
	 * Samples the BSDF.
	 * Returns the result of the BSDF for the sampled direction in f.
	 */
	virtual bool Sample_f(const TsPack *tspack, const Vector &o, Vector *wi,
		float u1, float u2, float u3, SWCSpectrum *const f, float *pdf,
		BxDFType flags = BSDF_ALL, BxDFType *sampledType = NULL,
		float *pdfBack = NULL, bool reverse = false) const = 0;
	virtual float Pdf(const TsPack *tspack, const Vector &wo,
		const Vector &wi, BxDFType flags = BSDF_ALL) const = 0;
	virtual SWCSpectrum f(const TsPack *tspack, const Vector &woW,
		const Vector &wiW, BxDFType flags = BSDF_ALL) const = 0;
	virtual SWCSpectrum rho(const TsPack *tspack,
		BxDFType flags = BSDF_ALL) const = 0;
	virtual SWCSpectrum rho(const TsPack *tspack, const Vector &wo,
		BxDFType flags = BSDF_ALL) const = 0;

	static void *Alloc(const TsPack *tspack, u_int sz) {
		return tspack->arena->Alloc(sz);
	}
	static void FreeAll(const TsPack *tspack) { tspack->arena->FreeAll(); }

	// BSDF Public Data
	const DifferentialGeometry dgShading;
	const float eta;

	// Compositing Parameters pointer
	CompositingParams *compParams;
	
protected:
	// BSDF Private Methods
	virtual ~BSDF() { }
	// BSDF Private Data
	Normal nn, ng;
	Vector sn, tn;
};
#define BSDF_ALLOC(TSPACK,T)  new (BSDF::Alloc((TSPACK), sizeof(T))) T

// Single BxDF BSDF declaration
class  SingleBSDF : public BSDF  {
public:
	// StackedBSDF Public Methods
	SingleBSDF(const DifferentialGeometry &dgs, const Normal &ngeom,
		BxDF *b, float e = 1.f) : BSDF(dgs, ngeom, e), bxdf(b) { }
	virtual inline int NumComponents() const { return 1; }
	virtual inline int NumComponents(BxDFType flags) const;
	/**
	 * Samples the BSDF.
	 * Returns the result of the BSDF for the sampled direction in f.
	 */
	virtual bool Sample_f(const TsPack *tspack, const Vector &o, Vector *wi,
		float u1, float u2, float u3, SWCSpectrum *const f, float *pdf,
		BxDFType flags = BSDF_ALL, BxDFType *sampledType = NULL,
		float *pdfBack = NULL, bool reverse = false) const;
	virtual float Pdf(const TsPack *tspack, const Vector &wo,
		const Vector &wi, BxDFType flags = BSDF_ALL) const;
	virtual SWCSpectrum f(const TsPack *tspack, const Vector &woW,
		const Vector &wiW, BxDFType flags = BSDF_ALL) const;
	virtual SWCSpectrum rho(const TsPack *tspack,
		BxDFType flags = BSDF_ALL) const;
	virtual SWCSpectrum rho(const TsPack *tspack, const Vector &wo,
		BxDFType flags = BSDF_ALL) const;

protected:
	// SingleBSDF Private Methods
	virtual ~SingleBSDF() { }
	// SingleBSDF Private Data
	BxDF *bxdf;
};

// Multiple BxDF BSDF declaration
class  MultiBSDF : public BSDF  {
public:
	// MultiBSDF Public Methods
	MultiBSDF(const DifferentialGeometry &dgs, const Normal &ngeom,
		float eta = 1.f);
	inline void Add(BxDF *bxdf);
	virtual inline int NumComponents() const { return nBxDFs; }
	virtual inline int NumComponents(BxDFType flags) const;
	/**
	 * Samples the BSDF.
	 * Returns the result of the BSDF for the sampled direction in f.
	 */
	virtual bool Sample_f(const TsPack *tspack, const Vector &o, Vector *wi,
		float u1, float u2, float u3, SWCSpectrum *const f, float *pdf,
		BxDFType flags = BSDF_ALL, BxDFType *sampledType = NULL,
		float *pdfBack = NULL, bool reverse = false) const;
	virtual float Pdf(const TsPack *tspack, const Vector &wo,
		const Vector &wi, BxDFType flags = BSDF_ALL) const;
	virtual SWCSpectrum f(const TsPack *tspack, const Vector &woW,
		const Vector &wiW, BxDFType flags = BSDF_ALL) const;
	virtual SWCSpectrum rho(const TsPack *tspack,
		BxDFType flags = BSDF_ALL) const;
	virtual SWCSpectrum rho(const TsPack *tspack, const Vector &wo,
		BxDFType flags = BSDF_ALL) const;

protected:
	// MultiBSDF Private Methods
	virtual ~MultiBSDF() { }
	// MultiBSDF Private Data
	int nBxDFs;
	#define MAX_BxDFS 8
	BxDF *bxdfs[MAX_BxDFS];
};

// Mix BSDF Declarations
class MixBSDF : public BSDF {
public:
	// MixBSDF Public Methods
	MixBSDF(const DifferentialGeometry &dgs, const Normal &ngeom);
	inline void Add(float weight, BSDF *bsdf);
	virtual inline int NumComponents() const;
	virtual inline int NumComponents(BxDFType flags) const;
	virtual inline void SetCompositingParams(CompositingParams *cp) {
		compParams = cp;
		for (int i = 0; i < nBSDFs; ++i)
			bsdfs[i]->SetCompositingParams(cp);
	}
	/**
	 * Samples the BSDF.
	 * Returns the result of the BSDF for the sampled direction in f.
	 */
	virtual bool Sample_f(const TsPack *tspack, const Vector &o, Vector *wi,
		float u1, float u2, float u3, SWCSpectrum *const f, float *pdf,
		BxDFType flags = BSDF_ALL, BxDFType *sampledType = NULL,
		float *pdfBack = NULL, bool reverse = false) const;
	virtual float Pdf(const TsPack *tspack, const Vector &wo,
		const Vector &wi, BxDFType flags = BSDF_ALL) const;
	virtual SWCSpectrum f(const TsPack *tspack, const Vector &woW,
		const Vector &wiW, BxDFType flags = BSDF_ALL) const;
	virtual SWCSpectrum rho(const TsPack *tspack,
		BxDFType flags = BSDF_ALL) const;
	virtual SWCSpectrum rho(const TsPack *tspack, const Vector &wo,
		BxDFType flags = BSDF_ALL) const;
private:
	// MixBSDF Private Methods
	virtual ~MixBSDF() { }
	// MixBSDF Private Data
	int nBSDFs;
	BSDF *bsdfs[MAX_BxDFS];
	float weights[MAX_BxDFS];
	float totalWeight;
};

// BxDF Declarations
class  BxDF {
public:
	// BxDF Interface
	virtual ~BxDF() { }
	BxDF(BxDFType t) : type(t) { }
	bool MatchesFlags(BxDFType flags) const {
		return (type & flags) == type;
	}
	/**
	 * Evaluates the BxDF.
	 * Accumulates the result in the f parameter.
	 */
	virtual void f(const TsPack *tspack, const Vector &wo,
		const Vector &wi, SWCSpectrum *const f) const = 0;
	/**
	 * Samples the BxDF.
	 * Returns the result of the BxDF for the sampled direction in f.
	 */
	virtual bool Sample_f(const TsPack *tspack, const Vector &wo, Vector *wi,
		float u1, float u2, SWCSpectrum *const f, float *pdf, float *pdfBack = NULL,
		bool reverse = false) const;
	virtual SWCSpectrum rho(const TsPack *tspack, const Vector &wo,
	                     int nSamples = 16,
		                 float *samples = NULL) const;
	virtual SWCSpectrum rho(const TsPack *tspack, int nSamples = 16,
	                     float *samples = NULL) const;
	virtual float Weight(const TsPack *tspack, const Vector &wo) const;
	virtual float Pdf(const TsPack *tspack, const Vector &wi, const Vector &wo) const;
	// BxDF Public Data
	const BxDFType type;
};
class  BRDFToBTDF : public BxDF {
public:
	// BRDFToBTDF Public Methods
	BRDFToBTDF(BxDF *b, float ei = 1.f, float et = 1.f, float c = 0.f)
		: BxDF(BxDFType(b->type ^
			(BSDF_REFLECTION | BSDF_TRANSMISSION))),
		etai(ei), etat(et), cb(c), brdf(b) {}
	virtual ~BRDFToBTDF() { }
	static Vector otherHemisphere(const Vector &w) {
		return Vector(w.x, w.y, -w.z);
	}
	virtual SWCSpectrum rho(const TsPack *tspack, const Vector &w, int nSamples,
			float *samples) const {
		return brdf->rho(tspack, otherHemisphere(w), nSamples, samples);
	}
	virtual SWCSpectrum rho(const TsPack *tspack, int nSamples, float *samples) const {
		return brdf->rho(tspack, nSamples, samples);
	}
	virtual void f(const TsPack *tspack, const Vector &wo, const Vector &wi, SWCSpectrum *const f) const;
	virtual bool Sample_f(const TsPack *tspack, const Vector &wo, Vector *wi,
		float u1, float u2, SWCSpectrum *const f, float *pdf, float *pdfBack = NULL,
		bool reverse = false) const;
	virtual float Weight(const TsPack *tspack, const Vector &wo) const {
		return brdf->Weight(tspack, wo);
	}
	virtual float Pdf(const TsPack *tspack, const Vector &wo, const Vector &wi) const;
private:
	float etai, etat, cb;
	BxDF *brdf;
};

// BSDF Inline Method Definitions
inline int SingleBSDF::NumComponents(BxDFType flags) const
{
	return bxdf->MatchesFlags(flags) ? 1 : 0;
}
inline void MultiBSDF::Add(BxDF *b)
{
	BOOST_ASSERT(nBxDFs < MAX_BxDFS);
	bxdfs[nBxDFs++] = b;
}
inline int MultiBSDF::NumComponents(BxDFType flags) const
{
	int num = 0;
	for (int i = 0; i < nBxDFs; ++i)
		if (bxdfs[i]->MatchesFlags(flags)) ++num;
	return num;
}
inline void MixBSDF::Add(float weight, BSDF *bsdf)
{
	BOOST_ASSERT(nBSDFs < MAX_BxDFS);
	weights[nBSDFs] = weight;
	bsdfs[nBSDFs++] = bsdf;
	totalWeight += weight;
}
inline int MixBSDF::NumComponents() const
{
	int num = 0;
	for (int i = 0; i < nBSDFs; ++i)
		num += bsdfs[i]->NumComponents();
	return num;
}
inline int MixBSDF::NumComponents(BxDFType flags) const
{
	int num = 0;
	for (int i = 0; i < nBSDFs; ++i)
		num += bsdfs[i]->NumComponents(flags);
	return num;
}

}//namespace lux

#endif // LUX_BXDF_H
