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

#ifndef LUX_FRESNELGENERAL_H
#define LUX_FRESNELGENERAL_H
// fresnelgeneral.h*
#include "lux.h"
#include "fresnel.h"
#include "spectrumwavelengths.h"

namespace lux
{

typedef enum { AUTO_FRESNEL, DIELECTRIC_FRESNEL, CONDUCTOR_FRESNEL, FULL_FRESNEL } FresnelModel;

class  FresnelGeneral : public Fresnel {
public:
	// FresnelGeneral Public Methods
	FresnelGeneral(FresnelModel m, const SWCSpectrum &e,
		const SWCSpectrum &kk) : eta(e), k(kk) {
		if (m == AUTO_FRESNEL)
			model = CheckModel(e, kk);
		else
			model = m;
	}
	FresnelGeneral(FresnelModel m,
		const SWCSpectrum &ei, const SWCSpectrum &ki,
		const SWCSpectrum &et, const SWCSpectrum &kt) {
		SWCSpectrum norm(ei * ei + ki * ki);
		eta = (ei * et + ki * kt) / norm;
		k = (ei * kt - et * ki) / norm;
		if (m == AUTO_FRESNEL)
			model = CheckModel(eta, k);
		else
			model = m;
	}
	FresnelGeneral(float f) {
		eta = f;
		k = 0.f;
		model = CheckModel(eta, k);
	}
	virtual ~FresnelGeneral() { }
	virtual void Evaluate(const SpectrumWavelengths &sw, float cosi,
			SWCSpectrum *const f) const;
	virtual float Index(const SpectrumWavelengths &sw) const {
		return eta.Filter(sw);
	}
	virtual SWCSpectrum SigmaA(const SpectrumWavelengths &sw) const {
		// The 4e-9*Pi comes from Beer law (4*Pi) and unit conversion
		// of w from nm to m
		return k / SWCSpectrum(sw.w) * (4e-9f * M_PI);
	}
	virtual void ComplexEvaluate(const SpectrumWavelengths &sw,
		SWCSpectrum *fr, SWCSpectrum *fi) const {
		*fr = eta;
		*fi = k;
	}
	FresnelGeneral operator+(const FresnelGeneral &f) const {
		return FresnelGeneral(model, eta + f.eta, k + f.k);
	}
	FresnelGeneral &operator+=(const FresnelGeneral &f) {
		eta += f.eta;
		k += f.k;
		return *this;
	}
	FresnelGeneral operator-(const FresnelGeneral &f) const {
		return FresnelGeneral(model, eta - f.eta, k - f.k);
	}
	FresnelGeneral operator*(float f) const {
		return FresnelGeneral(model, eta * f, k * f);
	}
	friend FresnelGeneral operator*(float f, const FresnelGeneral &fg) {
		return FresnelGeneral(fg.model, fg.eta * f, fg.k * f);
	}
private:
	static FresnelModel CheckModel(const SWCSpectrum &e,
		const SWCSpectrum &kk) {
		bool dielectric = true, conductor = true;
		for (u_int i = 0; i < WAVELENGTH_SAMPLES; ++i) {
			if (e.c[i] <= 10.f * kk.c[i])
				dielectric = false;
			if (e.c[i] > kk.c[i])
				conductor = false;
		}
		if (dielectric)
			return DIELECTRIC_FRESNEL;
		else if (conductor)
			return CONDUCTOR_FRESNEL;
		return FULL_FRESNEL;
	}
	// FresnelGeneral Private Data
	SWCSpectrum eta, k;
	FresnelModel model;
};

}//namespace lux

#endif // LUX_FRESNELCONDUCTOR_H

