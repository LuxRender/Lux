/***************************************************************************
 *   Copyright (C) 1998-2008 by authors (see AUTHORS.txt )                 *
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
	DistributedPath(LightStrategy st, bool da, int ds, bool dd, bool dg, bool ida, int ids, bool idd, bool idg,
								 int drd, int drs, int dtd, int dts, int grd, int grs, int gtd, int gts, int srd, int std);
	~DistributedPath() { }

	SWCSpectrum Li(const TsPack *tspack, const Scene *scene, const RayDifferential &ray, const Sample *sample,
		float *alpha) const;
	void RequestSamples(Sample *sample, const Scene *scene);
	void Preprocess(const TsPack *tspack, const Scene *scene);

	static SurfaceIntegrator *CreateSurfaceIntegrator(const ParamSet &params);

private:
	SWCSpectrum LiInternal(const TsPack *tspack, const Scene *scene, const RayDifferential &ray,
		const Sample *sample, float *alpha, int rayDepth, bool includeEmit, float &nrContribs) const;

	// DistributedPath Private Data
	LightStrategy lightStrategy;
	bool directAll, directDiffuse, directGlossy, 
		indirectAll, indirectDiffuse, indirectGlossy;
	int directSamples, indirectSamples;
	int diffusereflectDepth, diffusereflectSamples, diffuserefractDepth, diffuserefractSamples, glossyreflectDepth, glossyreflectSamples,
		glossyrefractDepth, glossyrefractSamples, specularreflectDepth, specularrefractDepth, maxDepth;

	// Declare sample parameters for light source sampling
	int sampleOffset, bufferId;
	int lightSampleOffset, lightNumOffset, bsdfSampleOffset, bsdfComponentOffset;
	int indirectlightSampleOffset, indirectlightNumOffset, indirectbsdfSampleOffset, indirectbsdfComponentOffset;
	int diffuse_reflectSampleOffset, diffuse_reflectComponentOffset, indirectdiffuse_reflectSampleOffset, indirectdiffuse_reflectComponentOffset;
	int diffuse_refractSampleOffset, diffuse_refractComponentOffset, indirectdiffuse_refractSampleOffset, indirectdiffuse_refractComponentOffset;
	int glossy_reflectSampleOffset, glossy_reflectComponentOffset, indirectglossy_reflectSampleOffset, indirectglossy_reflectComponentOffset;
	int glossy_refractSampleOffset, glossy_refractComponentOffset, indirectglossy_refractSampleOffset, indirectglossy_refractComponentOffset;
};

}//namespace lux
