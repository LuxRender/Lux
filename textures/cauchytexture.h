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

// cauchytexture.h*
#include "lux.h"
#include "texture.h"
#include "fresnelcauchy.h"
#include "paramset.h"

namespace lux
{

// CauchyTexture Declarations
class CauchyTexture : public Texture<const Fresnel *> {
public:
	// ConstantTexture Public Methods
	CauchyTexture(float cauchya, float cauchyb) :
		fresnel(cauchya, cauchyb, 0.f), index(cauchya + cauchyb * 1e6f /
		(WAVELENGTH_END * WAVELENGTH_START)) { }
	virtual ~CauchyTexture() { }
	virtual const Fresnel *Evaluate(const TsPack *tspack,
		const DifferentialGeometry &dg) const { return &fresnel; }
	virtual float Y() const { return index; }
	virtual void GetDuv(const TsPack *tspack,
		const DifferentialGeometry &dg, float delta,
		float *du, float *dv) const { *du = *dv = 0.f; }

	static Texture<const Fresnel *> *CreateFresnelTexture(const Transform &tex2world, const ParamSet &tp);
private:
	FresnelCauchy fresnel;
	float index;
};

}//namespace lux

