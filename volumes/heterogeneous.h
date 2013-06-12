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

// heterogeneous.h*
#include "volume.h"
#include "texture.h"
#include "sampling.h"
#include "luxrays/core/epsilon.h"

namespace lux
{

// HeterogeneousVolume Declarations
class HeterogeneousVolume : public Volume {
public:
	HeterogeneousVolume(const boost::shared_ptr<Texture<FresnelGeneral> > &fr,
		boost::shared_ptr<Texture<SWCSpectrum> > &a,
		boost::shared_ptr<Texture<SWCSpectrum> > &s,
		boost::shared_ptr<Texture<SWCSpectrum> > &g_,
		float ss) :
		Volume("HomogeneousVolume-"  + boost::lexical_cast<string>(this)),
		fresnel(fr), sigmaA(a), sigmaS(s), g(g_),
		primitive(&material, this, this), material(this, g_),
		stepSize(ss) { }
	virtual ~HeterogeneousVolume() { }
	virtual SWCSpectrum SigmaA(const SpectrumWavelengths &sw,
		const DifferentialGeometry &dg) const {
		return fresnel->Evaluate(sw, dg).SigmaA(sw) +
			sigmaA->Evaluate(sw, dg).Clamp();
	}
	virtual SWCSpectrum SigmaS(const SpectrumWavelengths &sw,
		const DifferentialGeometry &dg) const {
		return sigmaS->Evaluate(sw, dg);
	}
	virtual SWCSpectrum Lve(const SpectrumWavelengths &sw,
		const DifferentialGeometry &dg) const {
		return SWCSpectrum(0.f);
	}
	virtual float P(const SpectrumWavelengths &,
		const DifferentialGeometry &dg,
		const Vector &, const Vector &) const { return 0.f; }
	virtual SWCSpectrum SigmaT(const SpectrumWavelengths &sw,
		const DifferentialGeometry &dg) const {
		return SigmaA(sw, dg) + SigmaS(sw, dg);
	}
	virtual SWCSpectrum Tau(const SpectrumWavelengths &sw, const Ray &ray,
		float step = 1.f, float offset = .5f) const {
		// Evaluate the scattering at the path origin
		DifferentialGeometry dg;
		dg.p = ray(ray.mint);
		dg.nn = Normal(-ray.d);
		SWCSpectrum sigma(SigmaT(sw, dg));
		// Compute the number of steps to evaluate the volume
		// Integrates in steps of at most stepSize
		// unless stepSize is too small compared to the total length
		const float rl = (ray.maxt - ray.mint) * ray.d.Length();
		const u_int steps = Ceil2UInt(rl / max(MachineEpsilon::E(rl), stepSize));
		const float ss = rl / steps; // Effective step size
		SWCSpectrum tau(0.f);
		for (float s = 1; s <= steps; ++s) {
			// Compute the mean scattering over the current step
			dg.p = ray(ray.mint + s * ss);
			const SWCSpectrum sigma2(SigmaT(sw, dg));
			sigma = (sigma + sigma2) * .5f;
			// Skip the step if no scattering occurs
			if (sigma.Black()) {
				sigma = sigma2;
				continue;
			}
			for (u_int i = 0; i < WAVELENGTH_SAMPLES; i++) {
				// avoid NaNs by defining zero absorption coefficient as no absorption
				tau.c[i] += (sigma.c[i] > 0.f) ? sigma.c[i] * ss : 0.f;
			}
			sigma = sigma2;
		}
		return tau;
	}
	virtual FresnelGeneral Fresnel(const SpectrumWavelengths &sw,
		const DifferentialGeometry &dg) const {
		return fresnel->Evaluate(sw, dg);
	}
	bool Scatter(const Sample &sample, bool scatteredStart, const Ray &ray,
		float u, Intersection *isect, float *pdf, float *pdfBack,
		SWCSpectrum *L) const {
		const SpectrumWavelengths &sw = sample.swl;
		// Evaluate the scattering at the path origin
		DifferentialGeometry dg;
		dg.p = ray(ray.mint);
		dg.nn = Normal(-ray.d);
		float sigma = SigmaS(sw, dg).Filter(sw);
		// Compute the number of steps to evaluate the volume
		// Integrates in steps of at most stepSize
		// unless stepSize is too small compared to the total length
		const float rl = (ray.maxt - ray.mint) * ray.d.Length();
		const u_int steps = Ceil2UInt(rl / max(MachineEpsilon::E(rl), stepSize));
		const float ss = rl / steps; // Effective step size
		if (pdf)
			*pdf = 1.f;
		if (pdfBack)
			*pdfBack = 1.f;
		for (float s = 1; s <= steps; ++s) {
			// Compute the mean scattering over the current step
			dg.p = ray(ray.mint + s * ss);
			const float sigma2(SigmaS(sw, dg).Filter(sw));
			sigma = (sigma + sigma2) * .5f;
			// Skip the step if no scattering occurs
			if (!(sigma > 0.f)) {
				if (s == 1 && pdfBack)
					*pdfBack = 0.f;
				sigma = sigma2;
				continue;
			}
			// Determine scattering distance
			const float d = logf(1 - u) / sigma; //the real distance is ray.mint-d
			if (pdf) {
				*pdf *= expf(ss * sigma);
			}
			if (pdfBack) {
				*pdfBack *= expf(ss * sigma);
				if (s == 1 && scatteredStart)
					*pdfBack *= sigma;
			}
			const bool scatter = d > ray.mint - ray.maxt;
			if (!scatter) {
				sigma = sigma2;
				// Update the random variable to account for
				// the current step
				u -= (1.f - u) * (expf(sigma * ss) - 1.f);
				continue;
			}
			// The ray is scattered
			if (pdf)
				*pdf *= sigma;
			ray.maxt = ray.mint + (s - 1U) * ss - d;
			isect->dg.p = ray(ray.maxt);
			isect->dg.nn = Normal(-ray.d);
			isect->dg.scattered = true;
			CoordinateSystem(Vector(isect->dg.nn), &(isect->dg.dpdu), &(isect->dg.dpdv));
			isect->ObjectToWorld = Transform();
			isect->primitive = &primitive;
			isect->material = &material;
			isect->interior = this;
			isect->exterior = this;
			isect->arealight = NULL; // Update if volumetric emission
			if (L)
				*L *= SigmaT(sample.swl, isect->dg);
			break;
		}
		if (L)
			*L *= Exp(-Tau(sample.swl, ray));
		return isect->dg.scattered;
	}
	// HeterogeneousVolume Public Methods
	static Volume *CreateVolume(const Transform &volume2world, const ParamSet &params);
	static Region *CreateVolumeRegion(const Transform &volume2world, const ParamSet &params);
	// HeterogeneousVolume Private Data
private:
	boost::shared_ptr<Texture<FresnelGeneral> > fresnel;
	boost::shared_ptr<Texture<SWCSpectrum> > sigmaA, sigmaS, g;
	ScattererPrimitive primitive;
	VolumeScatterMaterial material;
	float stepSize;
};

}//namespace lux
