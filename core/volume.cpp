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

// volume.cpp*
#include "volume.h"
#include "sampling.h"
#include "spectrum.h"

namespace lux
{
// Volume Scattering Definitions

float PhaseIsotropic(const Vector &, const Vector &) {
	return 1.f / (4.f * M_PI);
}
 float PhaseRayleigh(const Vector &w, const Vector &wp) {
	const float costheta = Dot(w, wp);
	return  3.f / (16.f * M_PI) * (1.f + costheta * costheta);
}
 float PhaseMieHazy(const Vector &w, const Vector &wp) {
	const float costheta = Dot(w, wp);
	return (0.5f + 4.5f * powf(0.5f * (1.f + costheta), 8.f)) / (4.f * M_PI);
}
 float PhaseMieMurky(const Vector &w, const Vector &wp) {
	const float costheta = Dot(w, wp);
	return (0.5f + 16.5f * powf(0.5f * (1.f + costheta), 32.f)) / (4.f * M_PI);
}

float PhaseHG(const Vector &w, const Vector &wp, float g) {
	const float costheta = Dot(w, wp);
	return 1.f / (4.f * M_PI) * (1.f - g * g) /
		powf(1.f + g * g - 2.f * g * costheta, 1.5f);
}

float PhaseSchlick(const Vector &w,
                   const Vector &wp, float g) {
	const float k = g * (1.55f - .55f * g * g);
	const float compkcostheta = 1.f - k * Dot(w, wp);
	return (1.f - k * k) / (4.f * M_PI * compkcostheta * compkcostheta);
}
bool RGBVolume::Scatter(const Sample &sample, bool scatteredStart,
	const Ray &ray, float u, Intersection *isect, float *pdf,
	float *pdfBack, SWCSpectrum *L) const
{
	// Determine scattering distance
	const float k = sigS.Filter();
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
		if (L)
			*L *= SigmaT(sample.swl, isect->dg);
	}
	if (pdf) {
		*pdf = expf((ray.mint - ray.maxt) * k);
		if (isect->dg.scattered)
			*pdf *= k;
	}
	if (pdfBack) {
		*pdfBack = expf((ray.mint - ray.maxt) * k);
			if (scatteredStart)
				*pdfBack *= k;
	}
	if (L)
		*L *= Exp(-Tau(sample.swl, ray));
	return scatter;
}
AggregateRegion::AggregateRegion(const vector<Region *> &r)
{
	regions = r;
	for (u_int i = 0; i < regions.size(); ++i)
		bound = Union(bound, regions[i]->WorldBound());
}
SWCSpectrum AggregateRegion::SigmaA(const SpectrumWavelengths &sw,
	const DifferentialGeometry &dg) const
{
	SWCSpectrum s(0.f);
	for (u_int i = 0; i < regions.size(); ++i)
		s += regions[i]->SigmaA(sw, dg);
	return s;
}
SWCSpectrum AggregateRegion::SigmaS(const SpectrumWavelengths &sw,
	const DifferentialGeometry &dg) const
{
	SWCSpectrum s(0.f);
	for (u_int i = 0; i < regions.size(); ++i)
		s += regions[i]->SigmaS(sw, dg);
	return s;
}
SWCSpectrum AggregateRegion::Lve(const SpectrumWavelengths &sw,
	const DifferentialGeometry &dg) const
{
	SWCSpectrum L(0.f);
	for (u_int i = 0; i < regions.size(); ++i)
		L += regions[i]->Lve(sw, dg);
	return L;
}
float AggregateRegion::P(const SpectrumWavelengths &sw,
	const DifferentialGeometry &dg, const Vector &w, const Vector &wp) const
{
	float ph = 0.f, sumWt = 0.f;
	for (u_int i = 0; i < regions.size(); ++i) {
		const float sigt = regions[i]->SigmaT(sw, dg).Filter(sw);
		if (sigt > 0.f) {
			const float wt = regions[i]->SigmaA(sw, dg).Filter(sw) /
				sigt;
			sumWt += wt;
			ph += wt * regions[i]->P(sw, dg, w, wp);
		}
	}
	return ph / sumWt;
}
SWCSpectrum AggregateRegion::SigmaT(const SpectrumWavelengths &sw,
	const DifferentialGeometry &dg) const
{
	SWCSpectrum s(0.f);
	for (u_int i = 0; i < regions.size(); ++i)
		s += regions[i]->SigmaT(sw, dg);
	return s;
}
SWCSpectrum AggregateRegion::Tau(const SpectrumWavelengths &sw, const Ray &ray,
	float step, float offset) const
{
	SWCSpectrum t(0.f);
	for (u_int i = 0; i < regions.size(); ++i)
		t += regions[i]->Tau(sw, ray, step, offset);
	return t;
}
bool AggregateRegion::IntersectP(const Ray &ray, float *t0, float *t1, bool null_shp_isect) const
{
	*t0 = INFINITY;
	*t1 = -INFINITY;
	for (u_int i = 0; i < regions.size(); ++i) {
		float tr0, tr1;
		if (regions[i]->IntersectP(ray, &tr0, &tr1, null_shp_isect)) {
			*t0 = min(*t0, tr0);
			*t1 = max(*t1, tr1);
		}
	}
	return (*t0 < *t1);
}
AggregateRegion::~AggregateRegion() {
	for (u_int i = 0; i < regions.size(); ++i)
		delete regions[i];
}
bool AggregateRegion::Scatter(const Sample &sample, bool scatteredStart,
	const Ray &ray, float u, Intersection *isect, float *pdf,
	float *pdfBack, SWCSpectrum *L) const
{
	bool scatter = false;
	for (u_int i = 0; i < regions.size(); ++i)
		scatter = scatter || regions[i]->Scatter(sample, scatteredStart,
			ray, u, isect, pdf, pdfBack, L);
	return scatter;
}

}//namespace lux

