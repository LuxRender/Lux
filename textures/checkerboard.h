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
#include "texture.h"
#include "paramset.h"
#include "sampling.h"
#include "shape.h"
#include "error.h"

namespace lux
{
// Checkerboard2D Private Types
//enum { NONE, SUPERSAMPLE, CLOSEDFORM } aaMethod;
typedef enum { NONE, SUPERSAMPLE, CLOSEDFORM } MethodType;
extern MethodType aaMethod;

// CheckerboardTexture Declarations
template <class T> class Checkerboard2D : public Texture<T> {
public:
	// Checkerboard2D Public Methods
	Checkerboard2D(TextureMapping2D *m,
	               boost::shared_ptr<Texture<T> > c1,
			       boost::shared_ptr<Texture<T> > c2,
				   const string &aa) {
		mapping = m;
		tex1 = c1;
		tex2 = c2;
		// Select anti-aliasing method for _Checkerboard2D_
		if (aa == "none") aaMethod = NONE;
		else if (aa == "supersample") aaMethod = SUPERSAMPLE;
		else if (aa == "closedform") aaMethod = CLOSEDFORM;
		else {
			//Warning("Anti-aliasing mode \"%s\" not understood "
			//	"by Checkerboard2D, defaulting"
			//	"to \"supersample\"", aa.c_str());
			std::stringstream ss;
			ss<<"Anti-aliasing mode '"<<aa<<"' not understood by Checkerboard2D, defaulting to 'supersample'";
			luxError(LUX_BADTOKEN,LUX_WARNING,ss.str().c_str());
			aaMethod = SUPERSAMPLE;
		}
	}
	virtual ~Checkerboard2D() {
		delete mapping;
	}
	virtual T Evaluate(const TsPack *tspack, const DifferentialGeometry &dg) const {
		float s, t, dsdx, dtdx, dsdy, dtdy;
		mapping->Map(dg, &s, &t, &dsdx, &dtdx, &dsdy, &dtdy);
		if (aaMethod == CLOSEDFORM) {
			// Compute closed form box-filtered _Checkerboard2D_ value
			// Evaluate single check if filter is entirely inside one of them
			float ds = max(fabsf(dsdx), fabsf(dsdy));
			float dt = max(fabsf(dtdx), fabsf(dtdy));
			float s0 = s - ds, s1 = s + ds;
			float t0 = t - dt, t1 = t + dt;
			if (Floor2Int(s0) == Floor2Int(s1) &&
				Floor2Int(t0) == Floor2Int(t1)) {
				// Point sample _Checkerboard2D_
				if ((Floor2Int(s) + Floor2Int(t)) % 2 == 0)
					return tex1->Evaluate(tspack, dg);
				return tex2->Evaluate(tspack, dg);
			}
			// Apply box-filter to checkerboard region
			#define BUMPINT(x) \
				(Floor2Int((x)/2) + \
				 2.f * max((x/2)-Floor2Int(x/2) - .5f, 0.f))
			float sint = (BUMPINT(s1) - BUMPINT(s0)) / (2.f * ds);
			float tint = (BUMPINT(t1) - BUMPINT(t0)) / (2.f * dt);
			float area2 = sint + tint - 2.f * sint * tint;
			if (ds > 1.f || dt > 1.f)
				area2 = .5f;
			return (1.f - area2) * tex1->Evaluate(tspack, dg) +
				area2 * tex2->Evaluate(tspack, dg);
		}
		else if (aaMethod == SUPERSAMPLE) {
			// Supersample _Checkerboard2D_
			#define SQRT_SAMPLES 4
			#define N_SAMPLES (SQRT_SAMPLES * SQRT_SAMPLES)
			float samples[2*N_SAMPLES];
			StratifiedSample2D(tspack, samples, SQRT_SAMPLES, SQRT_SAMPLES);						// COCACOLA - fix
			T value = 0.;
			float filterSum = 0.;
			for (int i = 0; i < N_SAMPLES; ++i) {
				// Compute new differential geometry for supersample location
				float dx = samples[2*i]   - 0.5f;
				float dy = samples[2*i+1] - 0.5f;
				DifferentialGeometry dgs = dg;
				dgs.p += dx * dgs.dpdx + dy * dgs.dpdy;
				dgs.u += dx * dgs.dudx + dy * dgs.dudy;
				dgs.v += dx * dgs.dvdx + dy * dgs.dvdy;
				dgs.dudx /= N_SAMPLES;
				dgs.dudy /= N_SAMPLES;
				dgs.dvdx /= N_SAMPLES;
				dgs.dvdy /= N_SAMPLES;
				// Compute $(s,t)$ for supersample and evaluate sub-texture
				float ss, ts, dsdxs, dtdxs, dsdys, dtdys;
				mapping->Map(dgs, &ss, &ts, &dsdxs, &dtdxs, &dsdys, &dtdys);
				float wt = expf(-2.f * (dx*dx + dy*dy));
				filterSum += wt;
				if ((Floor2Int(ss) + Floor2Int(ts)) % 2 == 0)
					value += wt * tex1->Evaluate(tspack, dgs);
				else
					value += wt * tex2->Evaluate(tspack, dgs);
			}
			return value / filterSum;
			#undef N_SAMPLES // NOBOOK
		}
		// Point sample _Checkerboard2D_
		if ((Floor2Int(s) + Floor2Int(t)) % 2 == 0)
			return tex1->Evaluate(tspack, dg);
		return tex2->Evaluate(tspack, dg);
	}
	virtual float Y() const { return (tex1->Y() + tex2->Y()) / 2.f; }
	virtual void SetPower(float power, float area) {
		// Update sub-textures
		tex1->SetPower(power, area);
		tex2->SetPower(power, area);
	}
	virtual void SetIlluminant() {
		// Update sub-textures
		tex1->SetIlluminant();
		tex2->SetIlluminant();
	}
private:
	// Checkerboard2D Private Data
	boost::shared_ptr<Texture<T> > tex1, tex2;
	TextureMapping2D *mapping;
};
template <class T> class Checkerboard3D : public Texture<T> {
public:
	// Checkerboard3D Public Methods
	Checkerboard3D(TextureMapping3D *m,
	               boost::shared_ptr<Texture<T> > c1,
			       boost::shared_ptr<Texture<T> > c2) {
		mapping = m;
		tex1 = c1;
		tex2 = c2;
	}
	virtual ~Checkerboard3D() { delete mapping; }
	virtual T Evaluate(const TsPack *tspack, const DifferentialGeometry &dg) const {
		// Supersample _Checkerboard3D_
		#define N_SAMPLES 4
		float samples[2*N_SAMPLES*N_SAMPLES];
		StratifiedSample2D(tspack, samples, N_SAMPLES, N_SAMPLES);									// COCACOLA - fix
		T value = 0.;
		float filterSum = 0.;
		for (int i = 0; i < N_SAMPLES*N_SAMPLES; ++i) {
			// Compute new differential geometry for supersample location
			float dx = samples[2*i]   - 0.5f;
			float dy = samples[2*i+1] - 0.5f;
			DifferentialGeometry dgs = dg;
			dgs.p += dx * dgs.dpdx + dy * dgs.dpdy;
			dgs.u += dx * dgs.dudx + dy * dgs.dudy;
			dgs.v += dx * dgs.dvdx + dy * dgs.dvdy;
			dgs.dudx /= N_SAMPLES;
			dgs.dudy /= N_SAMPLES;
			dgs.dvdx /= N_SAMPLES;
			dgs.dvdy /= N_SAMPLES;
			// Compute 3D supersample position and evaluate sub-texture
			Vector dPPdx, dPPdy;
			Point PP = mapping->Map(dgs, &dPPdx, &dPPdy);
			float wt = expf(-2.f * (dx*dx + dy*dy));
			filterSum += wt;
			if ((Floor2Int(PP.x) + Floor2Int(PP.y) + Floor2Int(PP.z)) % 2 == 0)
				value += wt * tex1->Evaluate(tspack, dgs);
			else
				value += wt * tex2->Evaluate(tspack, dgs);
		}
		return value / filterSum;
	}
	virtual float Y() const { return (tex1->Y() + tex2->Y()) / 2.f; }
	virtual void SetPower(float power, float area) {
		// Update sub-textures
		tex1->SetPower(power, area);
		tex2->SetPower(power, area);
	}
	virtual void SetIlluminant() {
		// Update sub-textures
		tex1->SetIlluminant();
		tex2->SetIlluminant();
	}
private:
	// Checkerboard3D Private Data
	boost::shared_ptr<Texture<T> > tex1, tex2;
	TextureMapping3D *mapping;
};

class Checkerboard
{
public:
	static Texture<float> * CreateFloatTexture(const Transform &tex2world, const TextureParams &tp);
	static Texture<SWCSpectrum> * CreateSWCSpectrumTexture(const Transform &tex2world, const TextureParams &tp);
};

}//namespace lux

#endif

