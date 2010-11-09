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
 
// Modified GlossyTranslucent material based on glossy2 by paco

#include "lux.h"
#include "material.h"

namespace lux
{

// GlossyTranslucent Class Declarations
class GlossyTranslucent : public Material {
public:
	// GlossyTranslucent Public Methods
	GlossyTranslucent(boost::shared_ptr<Texture<SWCSpectrum> > &kd,
		boost::shared_ptr<Texture<SWCSpectrum> > &kt,
		boost::shared_ptr<Texture<SWCSpectrum> > &ks,
		boost::shared_ptr<Texture<SWCSpectrum> > &ks2,
		boost::shared_ptr<Texture<SWCSpectrum> > &ka,
		boost::shared_ptr<Texture<SWCSpectrum> > &ka2,
		boost::shared_ptr<Texture<float> > &i,
		boost::shared_ptr<Texture<float> > &i2,
		boost::shared_ptr<Texture<float> > &d,
		boost::shared_ptr<Texture<float> > &d2,
		boost::shared_ptr<Texture<float> > &u,
		boost::shared_ptr<Texture<float> > &u2,
		boost::shared_ptr<Texture<float> > &v,
		boost::shared_ptr<Texture<float> > &v2,
		bool mb,
		bool mb2,
		boost::shared_ptr<Texture<float> > &bump,
		const CompositingParams &cp) : Kd(kd), Kt(kt),
		Ks(ks), Ks_bf(ks2), Ka(ka), Ka_bf(ka2), depth(d), depth_bf(d2),
		index(i), index_bf(i2), nu(u), nu_bf(u2), nv(v), nv_bf(v2),
		bumpMap(bump), multibounce(mb), multibounce_bf(mb2) {
		compParams = new CompositingParams(cp);
	}
	virtual ~GlossyTranslucent() { }
	virtual void GetShadingGeometry(const SpectrumWavelengths &sw,
		const Normal &nGeom, DifferentialGeometry *dgBump) const {
		if (bumpMap)
			Bump(sw, bumpMap, nGeom, dgBump);
	}
	virtual BSDF *GetBSDF(MemoryArena &arena, const SpectrumWavelengths &sw,
		const DifferentialGeometry &dgGeom,
		const DifferentialGeometry &dgShading,
		const Volume *exterior, const Volume *interior) const;
	
	static Material * CreateMaterial(const Transform &xform,
		const ParamSet &mp);

private:
	// GlossyTranslucent Private Data
	boost::shared_ptr<Texture<SWCSpectrum> > Kd, Kt, Ks, Ks_bf, Ka, Ka_bf;
	boost::shared_ptr<Texture<float> > depth, depth_bf, index, index_bf;
	boost::shared_ptr<Texture<float> > nu, nu_bf, nv, nv_bf;
	boost::shared_ptr<Texture<float> > bumpMap;
	bool multibounce, multibounce_bf;
};

}//namespace lux
