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

#include "lux.h"
#include "texture.h"
#include "rgbrefl.h"
#include "rgbillum.h"
#include "fresnelgeneral.h"
#include "paramset.h"
#include "primitive.h"

namespace lux
{

class HitPointAlphaTexture : public Texture<float> {
public:
	HitPointAlphaTexture() :
		Texture("HitPointAlphaTexture-" + boost::lexical_cast<string>(this)) { }
	virtual ~HitPointAlphaTexture() { }
	virtual float Evaluate(const SpectrumWavelengths &sw,
		const DifferentialGeometry &dgs) const {
		RGBColor color;
		float alpha;
		dgs.handle->GetShadingInformation(dgs, &color, &alpha);

		return alpha;
	}

	// The following methods don't make very much sense in this case. I have no
	// information about the color that will be delivered by DifferentialGeometry.
	virtual float Y() const { return 1.f; }
	virtual void GetDuv(const SpectrumWavelengths &sw,
		const DifferentialGeometry &dg, float delta,
		float *du, float *dv) const { *du = *dv = 0.f; }

	static Texture<float> *CreateFloatTexture(const Transform &tex2world, const ParamSet &tp);
};

class HitPointRGBColorTexture : public Texture<SWCSpectrum> {
public:
	HitPointRGBColorTexture() :
		Texture("HitPointRGBColorTexture-" + boost::lexical_cast<string>(this)) { }
	virtual ~HitPointRGBColorTexture() { }
	virtual SWCSpectrum Evaluate(const SpectrumWavelengths &sw,
		const DifferentialGeometry &dgs) const {
		RGBColor color;
		float alpha;
		dgs.handle->GetShadingInformation(dgs, &color, &alpha);

		return SWCSpectrum(sw, color);
	}

	// The following methods don't make very much sense in this case. I have no
	// information about the color that will be delivered by DifferentialGeometry.
	virtual float Y() const { return 1.f; }
	virtual float Filter() const { return 1.f; }
	virtual void GetDuv(const SpectrumWavelengths &sw,
		const DifferentialGeometry &dg, float delta,
		float *du, float *dv) const { *du = *dv = 0.f; }

	static Texture<SWCSpectrum> *CreateSWCSpectrumTexture(const Transform &tex2world, const ParamSet &tp);
};

}//namespace lux

