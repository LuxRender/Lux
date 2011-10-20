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

#ifndef LUX_LAYERED_H
#define LUX_LAYERED_H
// layeredbsdf.h*
#include "lux.h"
#include "bxdf.h"
#include "geometry/raydifferential.h"
#include "spectrum.h"
#include "randomgen.h"
#include <boost/thread/mutex.hpp>

namespace lux
{


// LayeredBSDF declaration
class  LayeredBSDF : public BSDF  {
public:
	// LayeredBSDF Public Methods
	LayeredBSDF(const DifferentialGeometry &dgs, const Normal &ngeom,
		const Volume *exterior, const Volume *interior);
	inline void Add(BSDF *bsdf, float opacity);
	virtual inline u_int NumComponents() const { return 2; } // reflection/transmission
	virtual inline u_int NumComponents(BxDFType flags) const;
	/**
	 * Samples the BSDF.
	 * Returns the result of the BSDF for the sampled direction in f.
	 */
	virtual bool SampleF(const SpectrumWavelengths &sw, const Vector &woW,
		Vector *wiW, float u1, float u2, float u3,
		SWCSpectrum *const f_, float *pdf, BxDFType flags = BSDF_ALL,
		BxDFType *sampledType = NULL, float *pdfBack = NULL,
		bool reverse = false) const ;
	virtual float Pdf(const SpectrumWavelengths &sw, const Vector &woW,
		const Vector &wiW, BxDFType flags = BSDF_ALL) const;
	virtual SWCSpectrum F(const SpectrumWavelengths &sw, const Vector &woW,
		const Vector &wiW, bool reverse,
		BxDFType flags = BSDF_ALL) const;
	virtual SWCSpectrum rho(const SpectrumWavelengths &sw,
		BxDFType flags = BSDF_ALL) const;
	virtual SWCSpectrum rho(const SpectrumWavelengths &sw,
		const Vector &woW, BxDFType flags = BSDF_ALL) const;

	Vector ReflectNg(const Vector &v) const {
		return v - (2.0f*Dot(geom_norm,v)) * geom_norm;
	}

	Vector LocalToWorldGeom(const Vector &v) const {
		return Vector(sn_geom.x * v.x + tn_geom.x * v.y + ng.x * v.z,
		              sn_geom.y * v.x + tn_geom.y * v.y + ng.y * v.z,
		              sn_geom.z * v.x + tn_geom.z * v.y + ng.z * v.z);
	}

	// for sampleF: F=f*(ns.wo)*(ng.wi)/(ng.wo)/pdf (reverse=false)
	float sample_Ftof_revfalse(const Vector &knownW,const Vector &sampledW, float pdf) const{
		return fabs( pdf/Dot(nn,sampledW) * Dot(ng,knownW) / Dot(nn,knownW) *Dot(nn,sampledW) / Dot(ng,sampledW) );
	}

	float sample_Ftof_revtrue(const Vector &sampledW, float pdf) const{
		return fabs (pdf/Dot(nn,sampledW) );
	}

	// for F:  F = f*(ns.wo) (at least for reverse=false. is reverse irrelevant? )
	float ftoF_rev_false(const Vector &outgoingW,const Vector &incomingW) const{
		return fabs( Dot(nn,outgoingW) * Dot(ng,incomingW) / Dot(ng,outgoingW) );
	}

	float ftoF_rev_true(const Vector &outgoingW,const Vector &incomingW) const{
		return AbsDot(nn,outgoingW);
	}

	float Ftof_rev_false(const Vector &outgoingW,const Vector &incomingW) const{
		return fabs ( Dot(ng,outgoingW) / Dot(ng,incomingW) / Dot(nn,outgoingW) );
	}

	bool matchesFlags(BxDFType flags) const { return (flags&BSDF_GLOSSY) ? true: false;}

	unsigned int getRandSeed() const;

protected:
	// LayeredBSDF Private Methods
	virtual ~LayeredBSDF() { }
	// LayeredBSDF Private Data
	u_int nBSDFs;
	#define MAX_BSDFS 8
	BSDF *bsdfs[MAX_BSDFS];
	float opacity[MAX_BSDFS];

	int max_num_bounces;
	int num_f_samples;
	Vector geom_norm,sn_geom, tn_geom;

	//boost::mutex seed_mutex;

	//RandomGenerator rng_seed;
	//unsigned int randseed;
	
};
// LayeredBSDF Inline Method Definitions
inline void LayeredBSDF::Add(BSDF *b, float op)
{
	BOOST_ASSERT(nBSDFs < MAX_BSDFS);
	bsdfs[nBSDFs] = b;
	opacity[nBSDFs++]=op;
	max_num_bounces=nBSDFs*4;

}

inline u_int LayeredBSDF::NumComponents(BxDFType flags) const
{
	if ((flags & BSDF_GLOSSY)>0) { return 1;}	// one for reflection/transmission
	if ((flags & BSDF_SPECULAR)>0) { return 1;}	// one for reflection/transmission
	return 0;
}

}//namespace lux



#endif // LUX_LAYERED_H
