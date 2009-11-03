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

// irregulardata.cpp*
#include "lux.h"
#include "texture.h"
#include "irregular.h"
#include "paramset.h"

namespace lux
{

// IrregularDataTexture Declarations
class IrregularDataTexture : public Texture<SWCSpectrum> {
public:
	// IrregularDataSpectrumTexture Public Methods
	IrregularDataTexture(u_int n, const float *wl, const float *data,
		float resolution = 5.f) : SPD(wl, data, n, resolution) { }
	virtual ~IrregularDataTexture() { }
	virtual SWCSpectrum Evaluate(const TsPack *tspack,
		const DifferentialGeometry &) const {
		return SWCSpectrum(tspack, &SPD);
	}
	virtual float Y() const { return SPD.Y(); }
	virtual void SetPower(float power, float area) {
		const float y = Y();
		if (!(y > 0.f))
			return;
		SPD.Scale(power / (area * M_PI * y));
	}
	static Texture<SWCSpectrum> *CreateSWCSpectrumTexture(const Transform &tex2world, const TextureParams &tp);

private:
	IrregularSPD SPD;
};

}//namespace lux

