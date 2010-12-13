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

namespace lux
{

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
	 * Compared to PBRT this is f*(ns.wo)
	 */
	virtual void F(const SpectrumWavelengths &sw, const Vector &wo,
		const Vector &wi, SWCSpectrum *const f) const = 0;
	/**
	 * Samples the BxDF.
	 * Returns the result of the BxDF for the sampled direction in f.
	 * Compared to PBRT this is f*(ns.wi)/pdf if reverse==true
	 * and f*(ns.wo)/pdf if reverse==true
	 */
	virtual bool SampleF(const SpectrumWavelengths &sw, const Vector &wo,
		Vector *wi, float u1, float u2, SWCSpectrum *const f,
		float *pdf, float *pdfBack = NULL, bool reverse = false) const;
	virtual SWCSpectrum rho(const SpectrumWavelengths &sw, const Vector &wo,
		u_int nSamples = 16, float *samples = NULL) const;
	virtual SWCSpectrum rho(const SpectrumWavelengths &sw,
		u_int nSamples = 16, float *samples = NULL) const;
	virtual float Weight(const SpectrumWavelengths &sw, const Vector &wo) const;
	virtual float Pdf(const SpectrumWavelengths &sw, const Vector &wi,
		const Vector &wo) const;
	// BxDF Public Data
	const BxDFType type;
};

// BSDF Declarations
class  BSDF {
public:
	// BSDF Public Methods
	BSDF(const DifferentialGeometry &dgs, const Normal &ngeom,
		const Volume *exterior, const Volume *interior);
	virtual u_int NumComponents() const = 0;
	virtual u_int NumComponents(BxDFType flags) const = 0;
	virtual inline void SetCompositingParams(const CompositingParams *cp) {
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
	const Volume *GetVolume(const Vector &w) const {
		return Dot(w, ng) > 0.f ? exterior : interior;
	}
	/**
	 * Samples the BSDF.
	 * Returns the result of the BSDF for the sampled direction in f.
	 * Compared to PBRT this is f*(ns.wi)/pdf if reverse==true
	 * and f*(ns.wo)*(ng.wi)/(ng.wo)/pdf if reverse==false
	 */
	virtual bool SampleF(const SpectrumWavelengths &sw, const Vector &wo,
		Vector *wi, float u1, float u2, float u3, SWCSpectrum *const f,
		float *pdf, BxDFType flags = BSDF_ALL,
		BxDFType *sampledType = NULL, float *pdfBack = NULL,
		bool reverse = false) const = 0;
	virtual float Pdf(const SpectrumWavelengths &sw, const Vector &wo,
		const Vector &wi, BxDFType flags = BSDF_ALL) const = 0;
	virtual SWCSpectrum F(const SpectrumWavelengths &sw, const Vector &woW,
		const Vector &wiW, bool reverse,
		BxDFType flags = BSDF_ALL) const = 0;
	virtual SWCSpectrum rho(const SpectrumWavelengths &sw,
		BxDFType flags = BSDF_ALL) const = 0;
	virtual SWCSpectrum rho(const SpectrumWavelengths &sw, const Vector &wo,
		BxDFType flags = BSDF_ALL) const = 0;

	// BSDF Public Data
	const Normal nn, ng;
	const DifferentialGeometry dgShading;
	const Volume *exterior, *interior;

	// Compositing Parameters pointer
	const CompositingParams *compParams;
	
protected:
	// BSDF Private Methods
	virtual ~BSDF() { }
	// BSDF Private Data
	Vector sn, tn;
};

// Single BxDF BSDF declaration
class  SingleBSDF : public BSDF  {
public:
	// StackedBSDF Public Methods
	SingleBSDF(const DifferentialGeometry &dgs, const Normal &ngeom,
		const BxDF *b, const Volume *exterior, const Volume *interior) :
		BSDF(dgs, ngeom, exterior, interior), bxdf(b) { }
	virtual inline u_int NumComponents() const { return 1; }
	virtual inline u_int NumComponents(BxDFType flags) const {
		return bxdf->MatchesFlags(flags) ? 1U : 0U;
	}
	/**
	 * Samples the BSDF.
	 * Returns the result of the BSDF for the sampled direction in f.
	 */
	virtual bool SampleF(const SpectrumWavelengths &sw, const Vector &woW,
		Vector *wiW, float u1, float u2, float u3,
		SWCSpectrum *const f_, float *pdf, BxDFType flags = BSDF_ALL,
		BxDFType *sampledType = NULL, float *pdfBack = NULL,
		bool reverse = false) const;
	virtual float Pdf(const SpectrumWavelengths &sw, const Vector &woW,
		const Vector &wiW, BxDFType flags = BSDF_ALL) const;
	virtual SWCSpectrum F(const SpectrumWavelengths &sw, const Vector &woW,
		const Vector &wiW, bool reverse,
		BxDFType flags = BSDF_ALL) const;
	virtual SWCSpectrum rho(const SpectrumWavelengths &sw,
		BxDFType flags = BSDF_ALL) const;
	virtual SWCSpectrum rho(const SpectrumWavelengths &sw,
		const Vector &woW, BxDFType flags = BSDF_ALL) const;

protected:
	// SingleBSDF Private Methods
	virtual ~SingleBSDF() { }
	// SingleBSDF Private Data
	const BxDF *bxdf;
};

// Multiple BxDF BSDF declaration
class  MultiBSDF : public BSDF  {
public:
	// MultiBSDF Public Methods
	MultiBSDF(const DifferentialGeometry &dgs, const Normal &ngeom,
		const Volume *exterior, const Volume *interior);
	inline void Add(BxDF *bxdf);
	virtual inline u_int NumComponents() const { return nBxDFs; }
	virtual inline u_int NumComponents(BxDFType flags) const;
	/**
	 * Samples the BSDF.
	 * Returns the result of the BSDF for the sampled direction in f.
	 */
	virtual bool SampleF(const SpectrumWavelengths &sw, const Vector &wo,
		Vector *wi, float u1, float u2, float u3, SWCSpectrum *const f,
		float *pdf, BxDFType flags = BSDF_ALL,
		BxDFType *sampledType = NULL, float *pdfBack = NULL,
		bool reverse = false) const;
	virtual float Pdf(const SpectrumWavelengths &sw, const Vector &wo,
		const Vector &wi, BxDFType flags = BSDF_ALL) const;
	virtual SWCSpectrum F(const SpectrumWavelengths &sw, const Vector &woW,
		const Vector &wiW, bool reverse,
		BxDFType flags = BSDF_ALL) const;
	virtual SWCSpectrum rho(const SpectrumWavelengths &sw,
		BxDFType flags = BSDF_ALL) const;
	virtual SWCSpectrum rho(const SpectrumWavelengths &sw, const Vector &wo,
		BxDFType flags = BSDF_ALL) const;

protected:
	// MultiBSDF Private Methods
	virtual ~MultiBSDF() { }
	// MultiBSDF Private Data
	u_int nBxDFs;
	#define MAX_BxDFS 8
	BxDF *bxdfs[MAX_BxDFS];
};

// Mix BSDF Declarations
class MixBSDF : public BSDF {
public:
	// MixBSDF Public Methods
	MixBSDF(const DifferentialGeometry &dgs, const Normal &ngeom,
		const Volume *exterior, const Volume *interior);
	inline void Add(float weight, BSDF *bsdf);
	virtual inline u_int NumComponents() const;
	virtual inline u_int NumComponents(BxDFType flags) const;
	virtual inline void SetCompositingParams(const CompositingParams *cp) {
		compParams = cp;
		for (u_int i = 0; i < nBSDFs; ++i)
			bsdfs[i]->SetCompositingParams(cp);
	}
	/**
	 * Samples the BSDF.
	 * Returns the result of the BSDF for the sampled direction in f.
	 */
	virtual bool SampleF(const SpectrumWavelengths &sw, const Vector &wo,
		Vector *wi, float u1, float u2, float u3, SWCSpectrum *const f,
		float *pdf, BxDFType flags = BSDF_ALL,
		BxDFType *sampledType = NULL, float *pdfBack = NULL,
		bool reverse = false) const;
	virtual float Pdf(const SpectrumWavelengths &sw, const Vector &wo,
		const Vector &wi, BxDFType flags = BSDF_ALL) const;
	virtual SWCSpectrum F(const SpectrumWavelengths &sw, const Vector &woW,
		const Vector &wiW, bool reverse,
		BxDFType flags = BSDF_ALL) const;
	virtual SWCSpectrum rho(const SpectrumWavelengths &sw,
		BxDFType flags = BSDF_ALL) const;
	virtual SWCSpectrum rho(const SpectrumWavelengths &sw, const Vector &wo,
		BxDFType flags = BSDF_ALL) const;
private:
	// MixBSDF Private Methods
	virtual ~MixBSDF() { }
	// MixBSDF Private Data
	u_int nBSDFs;
	BSDF *bsdfs[MAX_BxDFS];
	float weights[MAX_BxDFS];
	float totalWeight;
};

// BSDF Inline Method Definitions
inline void MultiBSDF::Add(BxDF *b)
{
	BOOST_ASSERT(nBxDFs < MAX_BxDFS);
	bxdfs[nBxDFs++] = b;
}
inline u_int MultiBSDF::NumComponents(BxDFType flags) const
{
	u_int num = 0;
	for (u_int i = 0; i < nBxDFs; ++i) {
		if (bxdfs[i]->MatchesFlags(flags))
			++num;
	}
	return num;
}
inline void MixBSDF::Add(float weight, BSDF *bsdf)
{
	if (!(weight > 0.f))
		return;
	BOOST_ASSERT(nBSDFs < MAX_BxDFS);
	// Special case since totalWeight = 1 when nBSDFs = 0
	if (nBSDFs == 0)
		totalWeight = weight;
	else
		totalWeight += weight;
	weights[nBSDFs] = weight;
	bsdfs[nBSDFs++] = bsdf;
}
inline u_int MixBSDF::NumComponents() const
{
	u_int num = 0;
	for (u_int i = 0; i < nBSDFs; ++i)
		num += bsdfs[i]->NumComponents();
	return num;
}
inline u_int MixBSDF::NumComponents(BxDFType flags) const
{
	u_int num = 0;
	for (u_int i = 0; i < nBSDFs; ++i)
		num += bsdfs[i]->NumComponents(flags);
	return num;
}

}//namespace lux

#endif // LUX_BXDF_H
