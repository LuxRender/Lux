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

// fresnelcolor.h*
#include "lux.h"
#include "texture.h"
#include "fresnelgeneral.h"
#include "paramset.h"

namespace lux
{

// CauchyTexture Declarations
class FresnelColorTexture : public Texture<FresnelGeneral> {
public:
	// FresnelColorTexture Public Methods
	FresnelColorTexture(const boost::shared_ptr<Texture<SWCSpectrum> > &c) :
		Texture("FresnelColorTexture-" + boost::lexical_cast<string>(this)),
		color(c) { }
	virtual ~FresnelColorTexture() { }
	virtual FresnelGeneral Evaluate(const SpectrumWavelengths &sw,
		const DifferentialGeometry &dg) const {
		SWCSpectrum c(color->Evaluate(sw, dg));
		return FresnelGeneral(FULL_FRESNEL,
			FresnelApproxEta(c), FresnelApproxK(c));
	}
	virtual float Y() const { return FresnelApproxEta(color->Filter()).c[0]; }
	virtual void GetDuv(const SpectrumWavelengths &sw,
		const DifferentialGeometry &dg, float delta,
		float *du, float *dv) const {
		color->GetDuv(sw, dg, delta, du, dv);
	}

	Texture<SWCSpectrum> *GetColorTexture() { return color.get(); }

	static Texture<FresnelGeneral> *CreateFresnelTexture(const Transform &tex2world, const ParamSet &tp);
private:
	const boost::shared_ptr<Texture<SWCSpectrum> > color;
};

}//namespace lux

