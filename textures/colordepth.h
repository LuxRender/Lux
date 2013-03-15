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

// colordepth.cpp*
#include "lux.h"
#include "spectrum.h"
#include "texture.h"
#include "paramset.h"

namespace lux
{

// ColorDepthTexture Declarations
class ColorDepthTexture : public Texture<SWCSpectrum> {
public:
	// ColorDepthTexture Public Methods
	ColorDepthTexture(float t, boost::shared_ptr<Texture<SWCSpectrum> > &c) :
		Texture("ColorDepthTexture-" + boost::lexical_cast<string>(this)),
		d(-max(1e-3f, t)), color(c) { }
	virtual ~ColorDepthTexture() { }
	virtual SWCSpectrum Evaluate(const SpectrumWavelengths &sw,
		const DifferentialGeometry &dg) const {
		return Ln(color->Evaluate(sw, dg).Clamp(1e-9f, 1.f)) / d;
	}
	virtual float Y() const { return Filter(); }
	virtual float Filter() const { return logf(Clamp(color->Filter(), 1e-9f, 1.f)) / d; }
	virtual void GetDuv(const SpectrumWavelengths &sw,
		const DifferentialGeometry &dg, float delta,
		float *du, float *dv) const { *du = *dv = 0.f; }
	static Texture<SWCSpectrum> *CreateSWCSpectrumTexture(const Transform &tex2world, const ParamSet &tp);

private:
	float d;
	boost::shared_ptr<Texture<SWCSpectrum> > color;
};

}//namespace lux

