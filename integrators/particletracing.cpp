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

// particletracing.cpp*
#include "particletracing.h"
#include "lux.h"
#include "transport.h"
#include "scene.h"
#include "light.h"
#include "camera.h"
#include "film.h"
#include "bxdf.h"
#include "paramset.h"

using namespace lux;

// ParticleTracingIntegrator Method Definitions
void ParticleTracingIntegrator::RequestSamples(Sample *sample, const Scene *scene)
{
	// TODO: Add another 2 random numbers for Lens UV
	vector<u_int> structure;
	// TODO: Light sampling is done only once
	structure.push_back(1);	// Select a light
	structure.push_back(2);	// Select a position in the light
	structure.push_back(2);	// Select a direction in the light

	structure.push_back(2);	// Select a direction by BxDF
	structure.push_back(1);	// Select a BxDF
	sampleOffset = sample->AddxD(structure, maxDepth + 1);
}
void ParticleTracingIntegrator::Preprocess(const Scene *scene)
{
	int i,id;
	char postfix[64];
	bufferIds.clear();
	for(i=0; i<=maxDepth; ++i)
	{
		sprintf(postfix,"_%02d",i);
		id = scene->camera->film->RequestBuffer(BUF_TYPE_PER_SCREEN,(BufferOutputConfig)(BUF_FRAMEBUFFER|BUF_STANDALONE),postfix);
		bufferIds.push_back(id);
	}
	scene->camera->film->CreateBuffers();
	numOfLights = scene->lights.size();
}

SWCSpectrum ParticleTracingIntegrator::Li(const Scene *scene,
		const RayDifferential &r, const Sample *sample,
		float *alpha) const
{
	SampleGuard guard(sample->sampler, sample);
	Sample &sample_gen = const_cast<Sample &>(*sample);
	Ray ray_gen;

	SWCSpectrum pathThroughput(1.0f);
	Point p;
	Normal n;
	Vector wi;
	float *data = sample->sampler->GetLazyValues(const_cast<Sample *>(sample), sampleOffset, 0);

	// Choose light for bidirectional path
	int lightNum = Floor2Int(data[0] * numOfLights);
	lightNum = min(lightNum, numOfLights - 1);
	Light *light = scene->lights[lightNum];

	// Sample ray from light source to start light path
	SWCSpectrum F;
	double G,pdf,g;
	//Point lastPoint;
	Normal lastNormal;

	SWCSpectrum Le;
	Point lightPoint;
	Normal lightNormal;
	Vector lightDir;
	float lightPdf1,lightPdf2,lightPdf;

	light->SamplePosition(data[1], data[2], &lightPoint, &lightNormal, &lightPdf1);
	lightPdf = lightPdf1 / numOfLights;

	if (scene->camera->IsVisibleFromEyes(scene, lightPoint, &sample_gen, &ray_gen))
	{
		//Vector wi1 = Normalize(wi);
		Vector wo1 = Normalize(-ray_gen.d);
		Le = light->Eval(lightNormal,wo1);
		G = scene->camera->GetConnectingFactor(lightPoint,wo1,lightNormal);
		Le *= G / lightPdf;

		assert(!Le.IsNaN());
//		sampler->AddSample(sample_gen.imageX, sample_gen.imageY, *sample, ray_gen, Le.ToXYZ(), 1.0f, 0);
		sample->AddContribution(sample_gen.imageX, sample_gen.imageY, Le.ToXYZ(), 1.f, 0);
	}

	light->SampleDirection(data[3],data[4],lightNormal,&lightDir,&lightPdf2);
	lightPdf *= lightPdf2;
	if (lightPdf == 0.)
	{
		printf("P \n");
		return 0.f;
	}
	Le = light->Eval(lightNormal,lightDir);

	Ray lightRay(lightPoint,lightDir);

	RayDifferential ray(lightRay);
	Intersection isect;

	F = Le;
	//lastPoint = lightPoint;
	lastNormal = lightNormal;
	pdf = lightPdf;

	for (int pathLength = 0; pathLength < maxDepth; ++pathLength)
	{
		if (pathLength!=0)
			data = sample->sampler->GetLazyValues(const_cast<Sample *>(sample), sampleOffset, pathLength);
		
		// Find next vertex of path
		// Stop path sampling since no intersection was found
		if (!scene->Intersect(ray, &isect))
			break;
//		pathThroughput *= scene->Transmittance(ray);

		// Evaluate BSDF at hit point
		BSDF *bsdf = isect.GetBSDF(ray);
		// Sample illumination from lights to find path contribution
		p = bsdf->dgShading.p;
		n = bsdf->dgShading.nn;
		wi = -ray.d;

//		g = AbsDot(wi,n)/DistanceSquared(lastPoint,p);
		g = 1.0;
		G = AbsDot(wi,lastNormal)*g;
		pdf *= g;
		F *= G;

		if (scene->camera->IsVisibleFromEyes(scene, p, &sample_gen, &ray_gen))
		{
			Vector wo1 = Normalize(-ray_gen.d);
			Vector wi1 = Normalize(wi);
			SWCSpectrum F1 = F;
			G = scene->camera->GetConnectingFactor(lightPoint,wo1,n);
			F1 *= bsdf->f(wo1,wi1)*(G / pdf);
//			sampler->AddSample(sample_gen.imageX, sample_gen.imageY, *sample, ray_gen, F1.ToXYZ(), 1.0f, pathLength+1);
			sample->AddContribution(sample_gen.imageX, sample_gen.imageY, F1.ToXYZ(), 1.f, pathLength + 1);
		}

		Vector wo;
		float bsdf_pdf;
		BxDFType flags;
		SWCSpectrum f = bsdf->Sample_f(wi, &wo, data[5], data[6], data[7],
			&bsdf_pdf, BSDF_ALL, &flags);
		if (f.Black() || bsdf_pdf == 0.)
			break;
		F *= f;
		pdf *= bsdf_pdf;

		// Possibly terminate the path
//		if (pathLength > 3)
		{
			if (random::floatValue() > continueProbability)
			{
				break;
			}
			F /= continueProbability;
		}

		ray = RayDifferential(p, wo);

		//lastPoint = p;
		lastNormal = n;
	}

	return SWCSpectrum(-1.0f);
}
SurfaceIntegrator* ParticleTracingIntegrator::CreateSurfaceIntegrator(const ParamSet &params)
{
	int maxDepth = params.FindOneInt("maxdepth", 5);
	float RRcontinueProb = params.FindOneFloat("rrcontinueprob", .65f);			// continueprobability for RR (0.0-1.0)
	return new ParticleTracingIntegrator(maxDepth, RRcontinueProb);
}
