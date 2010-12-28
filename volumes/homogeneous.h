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

// homogeneous.cpp*
#include "volume.h"
#include "texture.h"
#include "sampling.h"

namespace lux
{

// HomogeneousVolume Declarations
class HomogeneousVolume : public Volume {
public:
	HomogeneousVolume(const boost::shared_ptr<Texture<FresnelGeneral> > &fr,
		boost::shared_ptr<Texture<SWCSpectrum> > &a,
		boost::shared_ptr<Texture<SWCSpectrum> > &s,
		boost::shared_ptr<Texture<SWCSpectrum> > &g_) :
		fresnel(fr), sigmaA(a), sigmaS(s), g(g_),
		primitive(&material, this, this), material(s, g_, ParamSet()) { }
	virtual ~HomogeneousVolume() { }
	virtual SWCSpectrum SigmaA(const SpectrumWavelengths &sw,
		const Point &p, const Vector &) const {
		DifferentialGeometry dg; //FIXME give it as argument
		dg.p = p;
		return fresnel->Evaluate(sw, dg).SigmaA(sw) +
			sigmaA->Evaluate(sw, dg);
	}
	virtual SWCSpectrum SigmaS(const SpectrumWavelengths &sw,
		const Point &p, const Vector &) const {
		DifferentialGeometry dg; //FIXME give it as argument
		dg.p = p;
		return sigmaS->Evaluate(sw, dg);
	}
	virtual SWCSpectrum Lve(const SpectrumWavelengths &sw, const Point &,
		const Vector &) const { return SWCSpectrum(0.f); }
	virtual float P(const SpectrumWavelengths &, const Point &,
		const Vector &, const Vector &) const { return 0.f; }
	virtual SWCSpectrum SigmaT(const SpectrumWavelengths &sw,
		const Point &p, const Vector &w) const {
		return SigmaA(sw, p, w) + SigmaS(sw, p, w);
	}
	virtual SWCSpectrum Tau(const SpectrumWavelengths &sw, const Ray &ray,
		float step = 1.f, float offset = .5f) const {
		const SWCSpectrum sigma(SigmaT(sw, ray.o, ray.d));
		if (sigma.Black())
			return SWCSpectrum(0.f);
		return sigma * ray.d.Length() * (ray.maxt - ray.mint);
	}
	virtual FresnelGeneral Fresnel(const SpectrumWavelengths &sw,
		const Point &p, const Vector &) const {
		DifferentialGeometry dg; //FIXME give it as argument
		dg.p = p;
		return fresnel->Evaluate(sw, dg);
	}
	bool Scatter(const Sample &sample, bool scatteredStart, const Ray &ray,
		float u, Intersection *isect, float *pdf, float *pdfBack,
		SWCSpectrum *L) const {
		// Determine scattering distance
		const float k = sigmaS->Filter();
		const float d = logf(1 - u) / k; //the real distance is ray.mint-d
		bool scatter = d > ray.mint - ray.maxt;
		if (scatter) {
			// The ray is scattered
			ray.maxt = ray.mint - d;
			isect->dg.p = ray(ray.maxt);
			isect->dg.nn = Normal(-ray.d);
			isect->dg.scattered = true;
			CoordinateSystem(Vector(isect->dg.nn), &(isect->dg.dpdu), &(isect->dg.dpdv));
			isect->WorldToObject = Transform();
			isect->primitive = &primitive;
			isect->material = &material;
			isect->interior = this;
			isect->exterior = this;
			isect->arealight = NULL; // Update if volumetric emission
			if (pdf)
				*pdf = k * expf(d * k); //d is negative
			if (pdfBack) {
				*pdfBack = expf(d * k);
				if (scatteredStart)
					*pdfBack *= k;
			}
		} else {
			if (pdf) {
				*pdf = expf((ray.mint - ray.maxt) * k);
				// if u==1 we are just checking connectivity
				// so we give the probability of scattering
				// at the end point so that we can estimate
				// the alternate path probability
				if (isect->dg.scattered && u == 1.f)
					*pdf *= k;
			}
			if (pdfBack) {
				*pdfBack = expf((ray.mint - ray.maxt) * k);
				if (scatteredStart)
					*pdfBack *= k;
			}
		}
		if (L)
			*L *= Exp(-Tau(sample.swl, ray));
		return scatter;
	}
	// HomogeneousVolume Public Methods
	static Volume *CreateVolume(const Transform &volume2world, const ParamSet &params);
	static Region *CreateVolumeRegion(const Transform &volume2world, const ParamSet &params);
	// HomogeneousVolume Private Data
private:
	boost::shared_ptr<Texture<FresnelGeneral> > fresnel;
	boost::shared_ptr<Texture<SWCSpectrum> > sigmaA, sigmaS, g;
	ScattererPrimitive primitive;
	ScatterMaterial material;
};

}//namespace lux
