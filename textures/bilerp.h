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

// bilerp.cpp*
#include "lux.h"
#include "texture.h"
#include "paramset.h"
#include "error.h"

namespace lux
{

// BilerpTexture Declarations
class BilerpFloatTexture : public Texture<float> {
public:
	// BilerpTexture Public Methods
	BilerpFloatTexture(TextureMapping2D *m,
				  const float &t00, const float &t01,
			      const float &t10, const float &t11) {
		mapping = m;
		v00 = t00;
		v01 = t01;
		v10 = t10;
		v11 = t11;
	}
	virtual ~BilerpFloatTexture() {
		delete mapping;
	}
	virtual float Evaluate(const TsPack *tspack, const DifferentialGeometry &dg) const {
		float s, t, dsdx, dtdx, dsdy, dtdy;
		mapping->Map(dg, &s, &t, &dsdx, &dtdx, &dsdy, &dtdy);
		return (1-s)*(1-t) * v00 +
		       (1-s)*(  t) * v01 +
			   (  s)*(1-t) * v10 +
			   (  s)*(  t) * v11;
	}
	virtual float Y() const { return (v00 + v01 + v10 + v11) / 4.f; }
	
	static Texture<float> * CreateFloatTexture(const Transform &tex2world, const TextureParams &tp);
	
private:
	// BilerpTexture Private Data
	TextureMapping2D *mapping;
	float v00, v01, v10, v11;
};

class BilerpSpectrumTexture : public Texture<SWCSpectrum> {
public:
	// BilerpTexture Public Methods
	BilerpSpectrumTexture(TextureMapping2D *m,
				  const RGBColor &t00, const RGBColor &t01,
			      const RGBColor &t10, const RGBColor &t11) {
		mapping = m;
		v00 = t00;
		v01 = t01;
		v10 = t10;
		v11 = t11;
	}
	virtual ~BilerpSpectrumTexture() {
		delete mapping;
	}
	virtual SWCSpectrum Evaluate(const TsPack *tspack, const DifferentialGeometry &dg) const {
		float s, t, dsdx, dtdx, dsdy, dtdy;
		mapping->Map(dg, &s, &t, &dsdx, &dtdx, &dsdy, &dtdy);
		return SWCSpectrum(tspack, (1-s)*(1-t) * v00 +
		       (1-s)*(  t) * v01 +
			   (  s)*(1-t) * v10 +
			   (  s)*(  t) * v11 );
	}
	virtual float Y() const { return RGBColor(v00 + v01 + v10 + v11).Y() / 4.f; }
	
	static Texture<SWCSpectrum> * CreateSWCSpectrumTexture(const Transform &tex2world, const TextureParams &tp);
	
private:
	// BilerpTexture Private Data
	TextureMapping2D *mapping;
	RGBColor v00, v01, v10, v11;
};

}//namespace lux

