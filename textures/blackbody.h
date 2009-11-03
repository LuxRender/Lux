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

// blackbody.cpp*
#include "lux.h"
#include "texture.h"
#include "blackbodyspd.h"
#include "paramset.h"

namespace lux
{

// BlackBodyTexture Declarations
class BlackBodyTexture : public Texture<SWCSpectrum> {
public:
	// BlackBodyTexture Public Methods
	BlackBodyTexture(float t) : BBSPD(t) { }
	virtual ~BlackBodyTexture() { }
	virtual SWCSpectrum Evaluate(const TsPack *tspack,
		const DifferentialGeometry &) const {
		return SWCSpectrum(tspack, &BBSPD);
	}
	virtual float Y() const { return BBSPD.Y(); }
	virtual void SetPower(float power, float area) {
		const float y = Y();
		if (!(y > 0.f))
			return;
		BBSPD.Scale(power / (area * M_PI * y));
	}
	static Texture<SWCSpectrum> *CreateSWCSpectrumTexture(const Transform &tex2world, const TextureParams &tp);

private:
	BlackbodySPD BBSPD;
};

}//namespace lux

