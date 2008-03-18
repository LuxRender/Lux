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

// path.cpp*
#include "pathn.h"
#include "bxdf.h"
#include "light.h"
#include "paramset.h"
#include "camera.h"	// for Film
#include "film.h"	// Film::RequestBuffer

using namespace lux;

// Lux (copy) constructor
PathnIntegrator* PathnIntegrator::clone() const
{
	PathnIntegrator *path = new PathnIntegrator(*this);
/*	path->lightPositionOffset = new int[maxDepth];
	path->lightNumOffset = new int[maxDepth];
	path->bsdfDirectionOffset = new int[maxDepth];
	path->bsdfComponentOffset = new int[maxDepth];
	path->continueOffset = new int[maxDepth];
	path->outgoingDirectionOffset = new int[maxDepth];
	path->outgoingComponentOffset = new int[maxDepth];
	for (int i = 0; i < maxDepth; ++i) {
		path->lightPositionOffset[i] = lightPositionOffset[i];
		path->lightNumOffset[i] = lightNumOffset[i];
		path->bsdfDirectionOffset[i] = bsdfDirectionOffset[i];
		path->bsdfComponentOffset[i] = bsdfComponentOffset[i];
		path->continueOffset[i] = continueOffset[i];
		path->outgoingDirectionOffset[i] = outgoingDirectionOffset[i];
		path->outgoingComponentOffset[i] = outgoingComponentOffset[i];
	}*/
	return path;
}
// PathnIntegrator Method Definitions
void PathnIntegrator::Preprocess(const Scene* scene)
{
	int i,id;
	char postfix[64];
	bufferIds.clear();
	BufferType type = BUF_TYPE_PER_PIXEL;
	sampler->GetBufferType(&type);
	for(i=0; i<=maxDepth; ++i)
	{
		sprintf(postfix,"_%02d",i);
		id = scene->camera->film->RequestBuffer(type,(BufferOutputConfig)(BUF_FRAMEBUFFER|BUF_STANDALONE),postfix);
		bufferIds.push_back(id);
	}
	scene->camera->film->CreateBuffers();
}
void PathnIntegrator::RequestSamples(Sample *sample, const Scene *scene)
{
	vector<u_int> structure;
	structure.push_back(2);
	structure.push_back(1);
	structure.push_back(2);
	structure.push_back(1);
	structure.push_back(1);
	structure.push_back(2);
	structure.push_back(1);
	sampleOffset = sample->AddxD(structure, maxDepth + 1);

	/*	for (i = 0; i < maxDepth; ++i) {
		lightPositionOffset[i] = sample->Add2D(1);
		lightNumOffset[i] = sample->Add1D(1);
		bsdfDirectionOffset[i] = sample->Add2D(1);
		bsdfComponentOffset[i] = sample->Add1D(1);
		continueOffset[i] = sample->Add1D(1);
		outgoingDirectionOffset[i] = sample->Add2D(1);
		outgoingComponentOffset[i] = sample->Add1D(1);
	}*/
}

SWCSpectrum PathnIntegrator::Li(const Scene *scene,
		const RayDifferential &r, const Sample *sample,
		float *alpha) const
{
	int pathLength;
	SampleGuard guard(sampler);
	for (pathLength=0; pathLength<=maxDepth; ++pathLength)
		L[pathLength] = 0.0f;

	// Declare common path integration variables
	SWCSpectrum pathThroughput = 1.;
	RayDifferential ray(r);
	bool specularBounce = false;
	for (pathLength=0; ; ++pathLength) {
		// Find next vertex of path
		Intersection isect;
		if (!scene->Intersect(ray, &isect)) {
			// Stop path sampling since no intersection was found
			// Possibly add emitted light
			// NOTE - Added by radiance - adds horizon in render & reflections
			if (pathLength == 0 || specularBounce)
				for (u_int i = 0; i < scene->lights.size(); ++i)
					L[pathLength] += pathThroughput * scene->lights[i]->Le(ray); 
			// NOTE - MLJack - Initialize the variable alpha
			// (Sometimes alpha is a QNAN.)
			if (alpha) *alpha = 1.;
			// Set alpha channel NOTE - RADIANCE - disabled for now
			/*if (pathLength == 0 && alpha) {
				if (L != 0.) *alpha = 1.;
				else *alpha = 0.;
			} */
			break;
		}
		if (pathLength == 0) {
			r.maxt = ray.maxt;
			if (alpha) *alpha = 1.;
		}
		else
			pathThroughput *= scene->Transmittance(ray);
		// Possibly add emitted light at path vertex
		if (pathLength == 0 || specularBounce)
			L[pathLength] += pathThroughput * isect.Le(-ray.d);
		if (pathLength == maxDepth)
			break;
		// Evaluate BSDF at hit point
		BSDF *bsdf = isect.GetBSDF(ray);
		// Sample illumination from lights to find path contribution
		const Point &p = bsdf->dgShading.p;
		const Normal &n = bsdf->dgShading.nn;
		Vector wo = -ray.d;
		float *data = sample->sampler->GetLazyValues(const_cast<Sample *>(sample), sampleOffset, pathLength);
		if (pathLength < maxDepth)
			L[pathLength+1] += pathThroughput *
				UniformSampleOneLight(scene, p, n,
					wo, bsdf, sample,
					data, data + 2, data + 3, data + 5);
/*					lightPositionOffset[pathLength],
					lightNumOffset[pathLength],
					bsdfDirectionOffset[pathLength],
					bsdfComponentOffset[pathLength]);*/
		else 
			L[pathLength+1] += pathThroughput *
				UniformSampleOneLight(scene, p, n,
					wo, bsdf, sample); 

		// Possibly terminate the path
		if (pathLength > 3) {
			if (data[6]/*sample->oneD[continueOffset[pathLength]][0]*/ > continueProbability)
				break;
			// increase path contribution
			pathThroughput /= continueProbability;
		}

		// Sample BSDF to get new path direction
		// Get random numbers for sampling new direction, _bs1_, _bs2_, and _bcs_
/*		float bs1, bs2, bcs;
		bs1 = sample->twoD[outgoingDirectionOffset[pathLength]][0];
		bs2 = sample->twoD[outgoingDirectionOffset[pathLength]][1];
		bcs = sample->oneD[outgoingComponentOffset[pathLength]][0];*/
		Vector wi;
		float pdf;
		BxDFType flags;
		SWCSpectrum f = bsdf->Sample_f(wo, &wi, data[7]/*bs1*/, data[8]/*bs2*/, data[9]/*bcs*/,
			&pdf, BSDF_ALL, &flags);
		if (f.Black() || pdf == 0.)
			break;
		specularBounce = (flags & BSDF_SPECULAR) != 0;
		pathThroughput *= f * AbsDot(wi, n) / pdf;

		ray = RayDifferential(p, wi);
	}
	SWCSpectrum LL = 0.0f;
	for (pathLength=0; pathLength<=maxDepth; ++pathLength)
	{
		sampler->AddSample(sample->imageX,sample->imageY,*sample,r,L[pathLength].ToXYZ(),*alpha,bufferIds[pathLength]);
		//LL+=L[pathLength];
	}
	//sampler->AddSample(*sample,r,LL.ToXYZ(),*alpha,bufferIds[0]);

	return SWCSpectrum(-1.0f);
}
SurfaceIntegrator* PathnIntegrator::CreateSurfaceIntegrator(const ParamSet &params)
{
	// general
	int maxDepth = params.FindOneInt("maxdepth", 16);
	float RRcontinueProb = params.FindOneFloat("rrcontinueprob", .65f);			// continueprobability for RR (0.0-1.0)

	return new PathnIntegrator(maxDepth, RRcontinueProb);

}

