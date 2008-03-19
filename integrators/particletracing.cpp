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
	SampleGuard guard(sampler);

	SWCSpectrum pathThroughput(1.0f);
	Point p;
	Normal n;
	Vector wi;
	// Choose light for bidirectional path
	int lightNum = Floor2Int(random::floatValue() * numOfLights);
	lightNum = min(lightNum, numOfLights - 1);
	Light *light = scene->lights[lightNum];
	// Sample ray from light source to start light path
	float u[4];
	u[0] = random::floatValue();
	u[1] = random::floatValue();
	u[2] = random::floatValue();
	u[3] = random::floatValue();

	SWCSpectrum F;
	double G,pdf,g;
	Point lastPoint;
	Normal lastNormal;

	SWCSpectrum Le;
	Point lightPoint;
	Normal lightNormal;
	Vector lightDir;
	float lightPdf1,lightPdf2,lightPdf;

	light->SamplePosition(u[0], u[1], &lightPoint, &lightNormal, &lightPdf1);
	lightPdf = lightPdf1 / numOfLights;

	if (scene->camera->IsVisibleFromEyes(scene, lightPoint, &sample_gen, &ray_gen))
	{
		//Vector wi1 = Normalize(wi);
		Vector wo1 = Normalize(-ray_gen.d);
		Le = light->Eval(lightNormal,wo1);
		G = scene->camera->GetConnectingFactor(lightPoint,wo1,lightNormal);
		Le *= G / lightPdf;

		assert(!Le.IsNaN());
		sampler->AddSample(sample_gen.imageX, sample_gen.imageY, *sample, ray_gen, Le.ToXYZ(), 1.0f, 0);
	}

	light->SampleDirection(u[2],u[3],lightNormal,&lightDir,&lightPdf2);
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
	lastPoint = lightPoint;
	lastNormal = lightNormal;
	pdf = lightPdf;

	for (int pathLength = 0; pathLength < maxDepth; ++pathLength)
	{
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

		// Sample BSDF to get new path direction
		// Get random numbers for sampling new direction, _bs1_, _bs2_, and _bcs_
		float bs1, bs2, bcs;
		bs1 = random::floatValue();
		bs2 = random::floatValue();
		bcs = random::floatValue();

		if (scene->camera->IsVisibleFromEyes(scene, p, &sample_gen, &ray_gen))
		{
			Vector wo1 = Normalize(-ray_gen.d);
			Vector wi1 = Normalize(wi);
			SWCSpectrum F1 = F;
			G = scene->camera->GetConnectingFactor(lightPoint,wo1,n);
			F1 *= bsdf->f(wo1,wi1)*(G / pdf);
			sampler->AddSample(sample_gen.imageX, sample_gen.imageY, *sample, ray_gen, F1.ToXYZ(), 1.0f, pathLength+1);
		}

		Vector wo;
		float bsdf_pdf;
		BxDFType flags;
		SWCSpectrum f = bsdf->Sample_f(wi, &wo, bs1, bs2, bcs,
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

		lastPoint = p;
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
