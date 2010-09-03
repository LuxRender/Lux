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

// distributedpath.cpp*
#include "lux.h"
#include "light.h"
#include "transport.h"
#include "scene.h"

namespace lux
{

// DistributedPath Declarations

class DistributedPath : public SurfaceIntegrator {
public:
	// DistributedPath types
	enum LightStrategy { SAMPLE_ALL_UNIFORM, SAMPLE_ONE_UNIFORM,
		SAMPLE_AUTOMATIC
	};

	// DistributedPath Public Methods
	DistributedPath(LightStrategy st, bool da, u_int ds, bool dd, bool dg,
		bool ida, u_int ids, bool idd, bool idg, u_int drd, u_int drs,
		u_int dtd, u_int dts, u_int grd, u_int grs, u_int gtd,
		u_int gts, u_int srd, u_int std, bool drer, float drert,
		bool drfr, float drfrt,  bool grer, float grert, bool grfr,
		float grfrt);
	virtual ~DistributedPath() { }

	virtual u_int Li(const Scene &scene, const Sample &sample) const;
	virtual void RequestSamples(Sample *sample, const Scene &scene);
	virtual void Preprocess(const RandomGenerator &rng, const Scene &scene);
	static SurfaceIntegrator *CreateSurfaceIntegrator(const ParamSet &params);

private:
	void LiInternal(const Scene &scene, const Sample &sample,
		const Volume *volume, const Ray &ray,
		vector<SWCSpectrum> &L, float *alpha, float *zdepth,
		u_int rayDepth, bool includeEmit, u_int &nrContribs) const;
	void Reject(const SpectrumWavelengths &sw,
		vector< vector<SWCSpectrum> > &LL, vector<SWCSpectrum> &L,
		float rejectrange) const;

	// DistributedPath Private Data
	LightStrategy lightStrategy;
	bool directAll, directDiffuse, directGlossy, 
		indirectAll, indirectDiffuse, indirectGlossy;
	u_int directSamples, indirectSamples;
	u_int diffusereflectDepth, diffusereflectSamples, diffuserefractDepth,
		diffuserefractSamples, glossyreflectDepth, glossyreflectSamples,
		glossyrefractDepth, glossyrefractSamples, specularreflectDepth,
		specularrefractDepth, maxDepth;

	// Declare sample parameters for light source sampling
	u_int sampleOffset, bufferId;
	u_int lightSampleOffset, lightNumOffset, bsdfSampleOffset,
	      bsdfComponentOffset;
	u_int indirectlightSampleOffset, indirectlightNumOffset,
	      indirectbsdfSampleOffset, indirectbsdfComponentOffset;
	u_int diffuse_reflectSampleOffset, diffuse_reflectComponentOffset,
	      indirectdiffuse_reflectSampleOffset,
	      indirectdiffuse_reflectComponentOffset;
	u_int diffuse_refractSampleOffset, diffuse_refractComponentOffset,
	      indirectdiffuse_refractSampleOffset,
	      indirectdiffuse_refractComponentOffset;
	u_int glossy_reflectSampleOffset, glossy_reflectComponentOffset,
	      indirectglossy_reflectSampleOffset,
	      indirectglossy_reflectComponentOffset;
	u_int glossy_refractSampleOffset, glossy_refractComponentOffset,
	      indirectglossy_refractSampleOffset,
	      indirectglossy_refractComponentOffset;

	bool diffusereflectReject, diffuserefractReject, glossyreflectReject,
	     glossyrefractReject;
	float diffusereflectReject_thr, diffuserefractReject_thr,
	      glossyreflectReject_thr, glossyrefractReject_thr;
};

}//namespace lux
