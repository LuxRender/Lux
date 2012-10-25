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

// dots.cpp*
#include "lux.h"
#include "spectrum.h"
#include "texture.h"
#include "color.h"
#include "paramset.h"
#include "error.h"

namespace lux
{

// DotsTexture Declarations
class DotsTexture : public Texture<float> {
public:
	// DotsTexture Public Methods
	DotsTexture(TextureMapping2D *m, boost::shared_ptr<Texture<float> > &c1,
		boost::shared_ptr<Texture<float> > &c2) :
		outsideDot(c1), insideDot(c2), mapping(m) { }
	virtual ~DotsTexture() { delete mapping; }
	virtual float Evaluate(const SpectrumWavelengths &sw,
		const DifferentialGeometry &dg) const {
		// Compute cell indices for dots
		float s, t;
		mapping->Map(dg, &s, &t);
		const int sCell = Floor2Int(s + .5f);
		const int tCell = Floor2Int(t + .5f);
		// Return _insideDot_ result if point is inside dot
		if (Noise(sCell + .5f, tCell + .5f) > 0.f) {
			const float radius = .35f;
			const float maxShift = 0.5f - radius;
			const float sCenter = sCell + maxShift *
				Noise(sCell + 1.5f, tCell + 2.8f);
			const float tCenter = tCell + maxShift *
				Noise(sCell + 4.5f, tCell + 9.8f);
			const float ds = s - sCenter, dt = t - tCenter;
			if (ds * ds + dt * dt < radius * radius)
				return insideDot->Evaluate(sw, dg);
		}
		return outsideDot->Evaluate(sw, dg);
	}
	virtual float Y() const {
		return (insideDot->Y() + outsideDot->Y()) * .5f;
	}
	virtual float Filter() const {
		return (insideDot->Filter() + outsideDot->Filter()) * .5f; }
	virtual void GetDuv(const SpectrumWavelengths &sw,
		const DifferentialGeometry &dg, float delta,
		float *du, float *dv) const {
		// Compute cell indices for dots
		float s, t, dsdu, dtdu, dsdv, dtdv;
		mapping->MapDuv(dg, &s, &t, &dsdu, &dtdu, &dsdv, &dtdv);
		const int sCell = Floor2Int(s + .5f);
		const int tCell = Floor2Int(t + .5f);
		// Return _insideDot_ result if point is inside dot
		if (Noise(sCell + .5f, tCell + .5f) <= 0.f) {
			*du = *dv = 0.f;
			return;
		}
		const float radius = .35f;
		const float maxShift = 0.5f - radius;
		const float sCenter = sCell + maxShift *
			Noise(sCell + 1.5f, tCell + 2.8f);
		const float tCenter = tCell + maxShift *
			Noise(sCell + 4.5f, tCell + 9.8f);
		const float ds = s - sCenter, dt = t - tCenter;
		const float r2 = ds * ds + dt * dt;
		const float radius2 = radius * radius;
		const float dst = (fabsf(dsdu) + fabsf(dsdv) + fabsf(dtdu) +
			fabsf(dtdv)) * delta;
		if (ds < 0.f) {
			dsdu = -dsdu;
			dsdv = -dsdv;
		}
		if (dt < 0.f) {
			dtdu = -dtdu;
			dtdv = -dtdv;
		}
		const float dst2 = dst * dst * .25f;
		if (r2 < radius2) {
			insideDot->GetDuv(sw, dg, delta, du, dv);
			if (r2 > radius2 + dst2 - dst * radius) {
				const float d = (outsideDot->Evaluate(sw, dg) -
					insideDot->Evaluate(sw, dg)) /
					(sqrtf(r2) * delta);
				*du += d * (dsdu + dtdu);
				*dv += d * (dsdv + dtdv);
			}
		} else {
			outsideDot->GetDuv(sw, dg, delta, du, dv);
			if (r2 < radius2 + dst2 + dst * radius) {
				const float d = (outsideDot->Evaluate(sw, dg) -
					insideDot->Evaluate(sw, dg)) /
					(sqrtf(r2) * delta);
				*du -= d * (dsdu + dtdu);
				*dv -= d * (dsdv + dtdv);
			}
		}
	}
	virtual void GetMinMaxFloat(float *minValue, float *maxValue) const {
		float min1, min2;
		float max1, max2;
		insideDot->GetMinMaxFloat(&min1, &max1);
		outsideDot->GetMinMaxFloat(&min2, &max2);
		*minValue = min(min1, min2);
		*maxValue = max(max1, max2);
	}
	virtual void SetIlluminant() {
		// Update sub-textures
		outsideDot->SetIlluminant();
		insideDot->SetIlluminant();
	}
	static Texture<float> * CreateFloatTexture(const Transform &tex2world, const ParamSet &tp);
	
private:
	// DotsTexture Private Data
	boost::shared_ptr<Texture<float> > outsideDot, insideDot;
	TextureMapping2D *mapping;
};


// DotsTexture Method Definitions
inline Texture<float> * DotsTexture::CreateFloatTexture(const Transform &tex2world,
	const ParamSet &tp) {
	// Initialize 2D texture mapping _map_ from _tp_
	boost::shared_ptr<Texture<float> > in(tp.GetFloatTexture("inside", 1.f)),
		out(tp.GetFloatTexture("outside", 0.f));
	return new DotsTexture(TextureMapping2D::Create(tex2world, tp), in, out);
}

}//namespace lux

