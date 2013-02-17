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

#ifndef LUX_CHECKERBOARD_H
#define LUX_CHECKERBOARD_H

// checkerboard.cpp*
#include "lux.h"
#include "spectrum.h"
#include "texture.h"
#include "paramset.h"
#include "sampling.h"
#include "shape.h"
#include "error.h"

namespace lux
{

// CheckerboardTexture Declarations
class Checkerboard2D : public Texture<float> {
public:
	// Checkerboard2D Public Methods
	Checkerboard2D(TextureMapping2D *m,
		boost::shared_ptr<Texture<float> > &c1,
		boost::shared_ptr<Texture<float> > &c2, const string &aa) :
		Texture("Checkerboard2D-" + boost::lexical_cast<string>(this)),
		tex1(c1), tex2(c2), mapping(m) {
		// Select anti-aliasing method for _Checkerboard2D_
		if (aa == "none")
			aaMethod = NONE;
		else if (aa == "supersample") {
			LOG(LUX_WARNING, LUX_BADTOKEN) <<
				"Anti-aliasing mode 'supersample' is deprecated, using 'none' instead";
			aaMethod = NONE;
		} else if (aa == "closedform") {
			LOG(LUX_WARNING, LUX_BADTOKEN) <<
				"Anti-aliasing mode 'closedform' is deprecated, using 'none' instead";
			aaMethod = NONE;
		} else {
			LOG(LUX_WARNING, LUX_BADTOKEN) <<
				"Anti-aliasing mode '" << aa <<
				"' not understood by Checkerboard2D, defaulting to 'none'";
			aaMethod = NONE;
		}
	}
	virtual ~Checkerboard2D() {
		delete mapping;
	}
	virtual float Evaluate(const SpectrumWavelengths &sw,
		const DifferentialGeometry &dg) const {
		switch (aaMethod) {
/*			case CLOSEDFORM: {
				float s, t, dsdx, dtdx, dsdy, dtdy;
				mapping->MapDxy(dg, &s, &t, &dsdx, &dtdx,
					&dsdy, &dtdy);
				// Compute closed form box-filtered
				// _Checkerboard2D_ value
				// Evaluate single check if filter is entirely
				// inside one of them
				const float ds = max(fabsf(dsdx), fabsf(dsdy));
				const float dt = max(fabsf(dtdx), fabsf(dtdy));
				const float s0 = s - ds, s1 = s + ds;
				const float t0 = t - dt, t1 = t + dt;
				if (Floor2Int(s0) == Floor2Int(s1) &&
					Floor2Int(t0) == Floor2Int(t1)) {
					// Point sample _Checkerboard2D_
					if ((Floor2Int(s) + Floor2Int(t)) %
						2 == 0)
						return tex1->Evaluate(sw, dg);
					return tex2->Evaluate(sw, dg);
				}
				// Apply box-filter to checkerboard region
#define BUMPINT(x) \
	(Floor2Int((x) / 2) + \
	2.f * max(((x) / 2) - Floor2Int((x) / 2) - .5f, 0.f))
				const float sint = (BUMPINT(s1) - BUMPINT(s0)) /
					(2.f * ds);
				const float tint = (BUMPINT(t1) - BUMPINT(t0)) /
					(2.f * dt);
#undef BUMPINT
				float area2 = sint + tint - 2.f * sint * tint;
				if (ds > 1.f || dt > 1.f)
					area2 = .5f;
				return Lerp(area2, tex1->Evaluate(sw, dg),
					tex2->Evaluate(sw, dg));
			}
			case SUPERSAMPLE: {
				// Supersample _Checkerboard2D_
#define SQRT_SAMPLES 4
#define N_SAMPLES (SQRT_SAMPLES * SQRT_SAMPLES)
				float samples[2 * N_SAMPLES];
				StratifiedSample2D(tspack, samples,
					SQRT_SAMPLES, SQRT_SAMPLES);
				float value = 0.f;
				float filterSum = 0.f;
				for (u_int i = 0; i < N_SAMPLES; ++i) {
					// Compute new differential geometry
					// for supersample location
					float dx = samples[2 * i] - 0.5f;
					float dy = samples[2 * i + 1] - 0.5f;
					DifferentialGeometry dgs = dg;
					dgs.p += dx * dgs.dpdx + dy * dgs.dpdy;
					dgs.u += dx * dgs.dudx + dy * dgs.dudy;
					dgs.v += dx * dgs.dvdx + dy * dgs.dvdy;
					dgs.dudx /= N_SAMPLES;
					dgs.dudy /= N_SAMPLES;
					dgs.dvdx /= N_SAMPLES;
					dgs.dvdy /= N_SAMPLES;
					// Compute $(s,t)$ for supersample and
					// evaluate sub-texture
					float s, t;
					mapping->Map(dgs, &s, &t);
					float wt = expf(-2.f * (dx*dx + dy*dy));
					filterSum += wt;
					if ((Floor2Int(s) + Floor2Int(t)) %
						2 == 0)
						value += wt * tex1->Evaluate(sw, dgs);
					else
						value += wt * tex2->Evaluate(sw, dgs);
				}
				return value / filterSum;
#undef N_SAMPLES // NOBOOK
			}*/
			case NONE:
			default: {
				// Point sample _Checkerboard2D_
				float s, t;
				mapping->Map(dg, &s, &t);
				if ((Floor2Int(s) + Floor2Int(t)) % 2 == 0)
					return tex1->Evaluate(sw, dg);
				return tex2->Evaluate(sw, dg);
			}
		}
	}
	virtual float Y() const { return (tex1->Y() + tex2->Y()) * .5f; }
	virtual float Filter() const {
		return (tex1->Filter() + tex2->Filter()) * .5f;
	}
	virtual void GetDuv(const SpectrumWavelengths &sw,
		const DifferentialGeometry &dg,
		float delta, float *du, float *dv) const {
		float s, t, dsdu, dtdu, dsdv, dtdv;
		mapping->MapDuv(dg, &s, &t, &dsdu, &dtdu, &dsdv, &dtdv);
		const int is = Floor2Int(s), it = Floor2Int(t);
		const bool first = (is + it) % 2 == 0;
		if (first)
			tex1->GetDuv(sw, dg, delta, du, dv);
		else
			tex2->GetDuv(sw, dg, delta, du, dv);
		const float ds = delta * (dsdu + dsdv);
		const float dt = delta * (dtdu + dtdv);
		const bool bs = s - is < ds / 2.f ||
			s - is > 1.f - ds / 2.f;
		const bool bt = t - it < dt / 2.f ||
			t - it > 1.f - dt / 2.f;
		const float d = (tex2->Evaluate(sw, dg) -
			tex1->Evaluate(sw, dg)) / delta;
		if (bs) {
			if (first ^ (s - is < .5f)) {
				*du += d * dsdu;
				*dv += d * dsdv;
			} else {
				*du -= d * dsdu;
				*dv -= d * dsdv;
			}
		}
		if (bt) {
			if (first ^ (t - it < .5f)) {
				*du += d * dtdu;
				*dv += d * dtdv;
			} else {
				*du -= d * dtdu;
				*dv -= d * dtdv;
			}
		}
	}
	virtual void GetMinMaxFloat(float *minValue, float *maxValue) const {
		float min1, min2;
		float max1, max2;
		tex1->GetMinMaxFloat(&min1, &max1);
		tex2->GetMinMaxFloat(&min2, &max2);
		*minValue = min(min1, min2);
		*maxValue = max(max1, max2);
	}
	virtual void SetIlluminant() {
		// Update sub-textures
		tex1->SetIlluminant();
		tex2->SetIlluminant();
	}

	const Texture<float> *GetTex1() const { return tex1.get(); }
	const Texture<float> *GetTex2() const { return tex2.get(); }
	const TextureMapping2D *GetTextureMapping2D() const { return mapping; }

private:
	// Checkerboard2D Private Types
	typedef enum { NONE, SUPERSAMPLE, CLOSEDFORM } MethodType;

	// Checkerboard2D Private Data
	boost::shared_ptr<Texture<float> > tex1, tex2;
	TextureMapping2D *mapping;
	MethodType aaMethod;
};
class Checkerboard3D : public Texture<float> {
public:
	// Checkerboard3D Public Methods
	Checkerboard3D(TextureMapping3D *m,
		boost::shared_ptr<Texture<float> > &c1,
		boost::shared_ptr<Texture<float> > &c2) :
		Texture("Checkerboard3D-" + boost::lexical_cast<string>(this)),
		tex1(c1), tex2(c2), mapping(m) { }
	virtual ~Checkerboard3D() { delete mapping; }
	virtual float Evaluate(const SpectrumWavelengths &sw,
		const DifferentialGeometry &dg) const {
		const Point p(mapping->Map(dg));
		if ((Floor2Int(p.x) + Floor2Int(p.y) + Floor2Int(p.z)) % 2 == 0)
			return tex1->Evaluate(sw, dg);
		return tex2->Evaluate(sw, dg);
/*		Vector dpdx, dpdy;
		const Point p(mapping->MapDxy(dg, &dpdx, &dpdy));
		// Compute closed form box-filtered _Checkerboard3D_ value
		// Evaluate single check if filter is entirely
		// inside one of them
		const float dx = max(fabsf(dpdx.x), fabsf(dpdy.x));
		const float dy = max(fabsf(dpdx.y), fabsf(dpdy.y));
		const float dz = max(fabsf(dpdx.z), fabsf(dpdy.z));
		const float x0 = p.x - dx, x1 = p.x + dx;
		const float y0 = p.y - dy, y1 = p.y + dy;
		const float z0 = p.z - dz, z1 = p.z + dz;
		if (Floor2Int(x0) == Floor2Int(x1) &&
			Floor2Int(y0) == Floor2Int(y1) &&
			Floor2Int(z0) == Floor2Int(z1)) {
			// Point sample _Checkerboard3D_
			if ((Floor2Int(p.x) + Floor2Int(p.y) + Floor2Int(p.z)) %
				2 == 0)
				return tex1->Evaluate(sw, dg);
			return tex2->Evaluate(sw, dg);
		}
		// Apply box-filter to checkerboard region
#define BUMPINT(x) \
	(Floor2Int((x) / 2) + \
	2.f * max(((x) / 2) - Floor2Int((x) / 2) - .5f, 0.f))
		const float xint = (BUMPINT(x1) - BUMPINT(x0)) / (2.f * dx);
		const float yint = (BUMPINT(y1) - BUMPINT(y0)) / (2.f * dy);
		const float zint = (BUMPINT(z1) - BUMPINT(z0)) / (2.f * dz);
#undef BUMPINT
		float vol = xint + yint + zint - xint * (yint + zint) -
			yint * zint;
		if (dx > 1.f || dy > 1.f || dz > 1.f)
			vol = .5f;
		return Lerp(vol, tex1->Evaluate(sw, dg),
			tex2->Evaluate(sw, dg));*/
/*		// Supersample _Checkerboard3D_
#define N_SAMPLES 4
		float samples[2 * N_SAMPLES * N_SAMPLES];
		StratifiedSample2D(tspack, samples, N_SAMPLES, N_SAMPLES);
		float value = 0.f;
		float filterSum = 0.f;
		for (u_int i = 0; i < N_SAMPLES * N_SAMPLES; ++i) {
			// Compute new differential geometry for supersample location
			float dx = samples[2 * i] - 0.5f;
			float dy = samples[2 * i + 1] - 0.5f;
			DifferentialGeometry dgs = dg;
			dgs.p += dx * dgs.dpdx + dy * dgs.dpdy;
			dgs.u += dx * dgs.dudx + dy * dgs.dudy;
			dgs.v += dx * dgs.dvdx + dy * dgs.dvdy;
			dgs.dudx /= N_SAMPLES;
			dgs.dudy /= N_SAMPLES;
			dgs.dvdx /= N_SAMPLES;
			dgs.dvdy /= N_SAMPLES;
			// Compute 3D supersample position and evaluate sub-texture
			Point PP = mapping->Map(dgs);
			float wt = expf(-2.f * (dx * dx + dy * dy));
			filterSum += wt;
			if ((Floor2Int(PP.x) + Floor2Int(PP.y) +
				Floor2Int(PP.z)) % 2 == 0)
				value += wt * tex1->Evaluate(sw, dgs);
			else
				value += wt * tex2->Evaluate(sw, dgs);
		}
		return value / filterSum;*/
	}
	virtual float Y() const { return (tex1->Y() + tex2->Y()) * .5f; }
	virtual float Filter() const {
		return (tex1->Filter() + tex2->Filter()) * .5f;
	}
	virtual void GetDuv(const SpectrumWavelengths &sw,
		const DifferentialGeometry &dg,
		float delta, float *du, float *dv) const {
		Vector dpdu, dpdv;
		const Point p(mapping->MapDuv(dg, &dpdu, &dpdv));
		const int ix = Floor2Int(p.x), iy = Floor2Int(p.y),
			iz = Floor2Int(p.z);
		const bool first = (ix + iy + iz) % 2 == 0;
		if (first)
			tex1->GetDuv(sw, dg, delta, du, dv);
		else
			tex2->GetDuv(sw, dg, delta, du, dv);
		const float dx = delta * (dpdu.x + dpdv.x);
		const float dy = delta * (dpdu.y + dpdv.y);
		const float dz = delta * (dpdu.z + dpdv.z);
		const bool bx = p.x - iy < dx / 2.f ||
			p.x - ix > 1.f - dx / 2.f;
		const bool by = p.y - iy < dy / 2.f ||
			p.y - iy > 1.f - dy / 2.f;
		const bool bz = p.z - iz < dz / 2.f ||
			p.z - iz > 1.f - dz / 2.f;
		const float d = (tex2->Evaluate(sw, dg) -
			tex1->Evaluate(sw, dg)) / delta;
		if (bx) {
			if (first ^ (p.x - ix < .5f)) {
				*du += d * dpdu.x;
				*dv += d * dpdv.x;
			} else {
				*du -= d * dpdu.x;
				*dv -= d * dpdv.x;
			}
		}
		if (by) {
			if (first ^ (p.y - iy < .5f)) {
				*du += d * dpdu.y;
				*dv += d * dpdv.y;
			} else {
				*du -= d * dpdu.y;
				*dv -= d * dpdv.y;
			}
		}
		if (bz) {
			if (first ^ (p.z - iz < .5f)) {
				*du += d * dpdu.z;
				*dv += d * dpdv.z;
			} else {
				*du -= d * dpdu.z;
				*dv -= d * dpdv.z;
			}
		}
	}
	virtual void GetMinMaxFloat(float *minValue, float *maxValue) const {
		float min1, min2;
		float max1, max2;
		tex1->GetMinMaxFloat(&min1, &max1);
		tex2->GetMinMaxFloat(&min2, &max2);
		*minValue = min(min1, min2);
		*maxValue = max(max1, max2);
	}
	virtual void SetIlluminant() {
		// Update sub-textures
		tex1->SetIlluminant();
		tex2->SetIlluminant();
	}

	const Texture<float> *GetTex1() const { return tex1.get(); }
	const Texture<float> *GetTex2() const { return tex2.get(); }
	const TextureMapping3D *GetTextureMapping3D() const { return mapping; }

private:
	// Checkerboard3D Private Data
	boost::shared_ptr<Texture<float> > tex1, tex2;
	TextureMapping3D *mapping;
};

class Checkerboard
{
public:
	static Texture<float> * CreateFloatTexture(const Transform &tex2world, const ParamSet &tp);
};

}//namespace lux

#endif

