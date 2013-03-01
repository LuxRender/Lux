/***************************************************************************
 *   Copyright (C) 1998-2013 by authors (see AUTHORS.txt)                  *
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

// glossy2.cpp*
#include "lux.h"
#include "material.h"

namespace lux
{

// Glossy Class Declarations
class GlossyCombined : public Material {
public:
	// GlossyCombined Public Methods
	GlossyCombined(boost::shared_ptr<Texture<SWCSpectrum> > &kd,
		boost::shared_ptr<Texture<SWCSpectrum> > &ks,
		boost::shared_ptr<Texture<SWCSpectrum> > &ka,
		boost::shared_ptr<Texture<float> > &i,
		boost::shared_ptr<Texture<float> > &d,
		boost::shared_ptr<Texture<float> > &u,
		boost::shared_ptr<Texture<float> > &v,
		bool mb,
		const ParamSet &mp) : Material("GlossyCombined-" + boost::lexical_cast<string>(this), mp),
		Kd(kd), Ks(ks), Ka(ka),
		depth(d), index(i), nu(u), nv(v), multibounce(mb) { }
	virtual ~GlossyCombined() { }
	virtual BSDF *GetBSDF(MemoryArena &arena, const SpectrumWavelengths &sw,
		const Intersection &isect,
		const DifferentialGeometry &dgShading) const;
	
private:
	// Glossy Private Data
	boost::shared_ptr<Texture<SWCSpectrum> > Kd, Ks, Ka;
	boost::shared_ptr<Texture<float> > depth, index;
	boost::shared_ptr<Texture<float> > nu, nv;
	bool multibounce;
};

class Glossy2 : public Material {
public:
	// Glossy Public Methods
	Glossy2(boost::shared_ptr<Texture<SWCSpectrum> > &kd,
		boost::shared_ptr<Texture<SWCSpectrum> > &ks,
		boost::shared_ptr<Texture<SWCSpectrum> > &ka,
		boost::shared_ptr<Texture<float> > &i,
		boost::shared_ptr<Texture<float> > &d,
		boost::shared_ptr<Texture<float> > &u,
		boost::shared_ptr<Texture<float> > &v,
		boost::shared_ptr<Texture<float> > &s,
		bool mb,
		const ParamSet &mp) : Material("Glossy2-" + boost::lexical_cast<string>(this), mp), Kd(kd), Ks(ks), Ka(ka),	
		depth(d), index(i), nu(u), nv(v), sigma(s), 
		multibounce(mb) { }
	virtual ~Glossy2() { }
	virtual BSDF *GetBSDF(MemoryArena &arena, const SpectrumWavelengths &sw,
		const Intersection &isect,
		const DifferentialGeometry &dgShading) const;

	Texture<SWCSpectrum> *GetKdTexture() { return Kd.get(); }
	Texture<SWCSpectrum> *GetKsTexture() { return Ks.get(); }
	Texture<SWCSpectrum> *GetKaTexture() { return Ka.get(); }
	Texture<float> *GetNuTexture() { return nu.get(); }
	Texture<float> *GetNvTexture() { return nv.get(); }
	Texture<float> *GetDepthTexture() { return depth.get(); }
	Texture<float> *GetIndexTexture() { return index.get(); }
	bool IsMultiBounce() const { return multibounce; }

	static Material * CreateMaterial(const Transform &xform,
		const ParamSet &mp);
private:
	// Glossy Private Data
	boost::shared_ptr<Texture<SWCSpectrum> > Kd, Ks, Ka;
	boost::shared_ptr<Texture<float> > depth, index;
	boost::shared_ptr<Texture<float> > nu, nv;
	boost::shared_ptr<Texture<float> > sigma;
	bool multibounce;
};

class GlossyCoating : public Material {
public:
	// GlossyCoating Public Methods
	GlossyCoating(boost::shared_ptr<Material> &bmat,
		boost::shared_ptr<Texture<SWCSpectrum> > &ks,
		boost::shared_ptr<Texture<SWCSpectrum> > &ka,
		boost::shared_ptr<Texture<float> > &i,
		boost::shared_ptr<Texture<float> > &d,
		boost::shared_ptr<Texture<float> > &u,
		boost::shared_ptr<Texture<float> > &v,
		bool mb,
		const ParamSet &mp) : Material("GlossyCoating-" + boost::lexical_cast<string>(this), mp),
		basemat(bmat), Ks(ks), Ka(ka), depth(d), index(i), nu(u), nv(v), multibounce(mb) { }
	virtual ~GlossyCoating() { }
	virtual BSDF *GetBSDF(MemoryArena &arena, const SpectrumWavelengths &sw,
		const Intersection &isect,
		const DifferentialGeometry &dgShading) const;
	
	static Material * CreateMaterial(const Transform &xform,
		const ParamSet &mp);
private:
	// GlossyCoating Private Data
	boost::shared_ptr<Material> basemat;
	boost::shared_ptr<Texture<SWCSpectrum> > Ks, Ka;
	boost::shared_ptr<Texture<float> > depth, index;
	boost::shared_ptr<Texture<float> > nu, nv;
	bool multibounce;
};

}//namespace lux
