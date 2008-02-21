#include "lux.h"
#include "transport.h"
#include "scene.h"
#include "volume.h"
#include "stdio.h"
#include "MultipleScatteringPathTracing.h"

using namespace lux;

void MultipleScatteringPathIntegrator::RequestSamples(Sample *sample,const Scene *scene)
{
	vr = scene->volumeRegion;
	if (vr && !isHomogeneous)
	{
		float s = vr->GetSmallScaleSize();
		dMin = s<0 ? stepSize : s*stepMultiplier;
		dMax = dMin * stepRatio;
	}
}
Spectrum MultipleScatteringPathIntegrator::Transmittance(const Ray &r) const
{
	float length = r.d.Length();
	if (length == 0.f)
		return Spectrum(1.0f);
	Ray ray(r.o, r.d / length,
		r.mint * length,
		r.maxt * length);
	float t0, t1;
	if (!vr->IntersectP(ray, &t0, &t1))
		return Spectrum(1.0f);
	Spectrum tau(0.0f);
	if (isHomogeneous)
	{
		tau = vr->sigma_t(ray((t0+t1)*0.5f), Vector());
		tau *= t1-t0;
	}
	else
	{
		float d;
		Spectrum s0,s1;
		int zeroTimer = stepRatio;
		bool tag = true;

		s0 = vr->sigma_t(ray(t0),Vector());
		d = lux::random::floatValue() * dMin;
		t0 += d;

		while( t0<t1 )
		{
			s1 = vr->sigma_t(ray(t0),Vector());
			if (!s1.Black())
			{
				zeroTimer = stepRatio;
				if (d == dMax)
				{
					d = dMin;
					t0 += dMin - dMax;
					continue;
				}
			}
			else if (d != dMax)
			{
				zeroTimer--;
			}
			if (zeroTimer == 0)
				d = dMax;
			tau += (s0+s1) * 0.5f * d;
			if (tag)
			{	// use tag other than d<dMin, because it produces consistent behavior even if lux::random::floatValue()==1.0f.
				tag = false;
				d = dMin;
			}
			t0 += d;
			s0 = s1;
		}
		tau += s0 * 0.5f * d;
	}

	return Exp(-tau);
}

Spectrum MultipleScatteringPathIntegrator::Li(const Scene *scene,
		const RayDifferential &r, const Sample *sample,
		float *alpha) const {
	Spectrum pathThroughput(1.0f), L(0.0f);
	Intersection isect;
	Spectrum transmittance;
	Point pointScattered;
	Vector wo,wi;
	float fTauSample,pdfSelect;
	float t0, t1;
	float spdf;
	bool isIntersect;
	bool specularBounce = false;
	bool isScatteredInMedium;
	int pathLength;
	RayDifferential ray(r);
	*alpha = 1.0f;	// TODO NULL assert

	// Set new tracing direction in the increment expression
	// to protect the ray differential created by Scene::Render()
	for( pathLength=0; pathLength <= maxDepth;
		pathLength++, ray = RayDifferential(pointScattered, wi) ) 
	{
		// Russian Roulette
		if (pathLength >= 3)
		{
			if (lux::random::floatValue() > rrPdf)
				break;
			pathThroughput /= rrPdf;
		}

		isScatteredInMedium = false;
		wo = -ray.d;
		isIntersect = scene->Intersect(ray, &isect);
		if (vr && vr->IntersectP(ray, &t0, &t1))
		{
			Spectrum stepTau(0.0f),lastTau;
			Point p = ray(t0),pPrev;
			// Random selection for scattering and transmitting by ray marching
			fTauSample = -log(1 - lux::random::floatValue());
			if (isHomogeneous)
			{
				lastTau = vr->sigma_t(ray((t0+t1)*0.5f), Vector());
				float d = fTauSample / lastTau.y();
				if ((t1-t0)>d)
				{
					isScatteredInMedium = true;
					stepTau = lastTau * d;
					p = ray(t0+d);
				}
			}
			else
			{
				float d;
				Spectrum s0,s1;
				int zeroTimer = stepRatio;
				bool tag = true;

				s0 = vr->sigma_t(ray(t0),Vector());
				d = lux::random::floatValue() * dMin;
				t0 += d;
				while(t0<t1)
				{
					s1 = vr->sigma_t(ray(t0),Vector());
					if (!s1.Black())
					{
						zeroTimer = stepRatio;
						if (d==dMax)
						{
							d = dMin;
							t0 += dMin - dMax;
							continue;
						}
					}
					else if (d != dMax)
					{
						zeroTimer--;
					}
					if (zeroTimer==0)
						d = dMax;
					stepTau += (s0+s1) * 0.5f * d;
					if ( stepTau.y() >= fTauSample)
					{
						isScatteredInMedium = true;
						lastTau = (s0+s1) * 0.5f;
						p = ray(t0);
						break;
					}
					if (tag)
					{
						tag = false;
						d = dMin;
					}
					t0 += d;
					s0 = s1;
				}
			}
			// [TODO]: compute emission during ray marching
			//L += pathThroughput * vr->Lve(pointScattered,wo);

			if (isScatteredInMedium)
			{	// Scattered by medium
				pdfSelect = expf(-stepTau.y()) * lastTau.y();
				transmittance = Exp(-stepTau);
				pathThroughput *= transmittance / pdfSelect;
				pointScattered = p;

				// [TODO] Multiple Importance Sampling
				// Next event estimator in medium
				Spectrum ss = vr->sigma_s(pointScattered, wo);
				if (!ss.Black()) {
					int nLights = scene->lights.size();
					int lightNum = min(Floor2Int(lux::random::floatValue() * nLights), nLights-1);
					Light *light = scene->lights[lightNum];
					// Add contribution of _light_ due to scattering at _p_
					float pdf;
					VisibilityTester vis;
					Spectrum Ln = light->Sample_L(pointScattered, lux::random::floatValue(), lux::random::floatValue(), &wi, &pdf, &vis);
					if (!Ln.Black() && pdf > 0.f && vis.Unoccluded(scene)) {
						Spectrum Ld = Ln * Transmittance(vis.r);
						L += pathThroughput * ss * PhaseHG(wo,-wi,g) * Ld * float(nLights) / pdf;
					}
				}
				// [TODO] Remove the hardcoded phase function sampling and evaluation.
				// [TODO] Add a BSDF-styled structure for AggregateVolume.
				// Sample phase function
				wi = -SampleHG(wo,g,lux::random::floatValue(),lux::random::floatValue());
				spdf = HGPdf(wo,-wi,g);
				float f = PhaseHG(wo,-wi,g);
				pathThroughput *= f * ss / spdf;
				specularBounce = false;
				continue;
			}
			else
			{	// Reflected by surface
				pdfSelect = expf(-stepTau.y());
				transmittance = Exp(-stepTau);
				pathThroughput *= transmittance / pdfSelect;
			}
		}

		if (!isIntersect)
			break;

		if (pathLength == 0 || specularBounce)
			L += pathThroughput * isect.Le(-ray.d);

		BSDF *bsdf = isect.GetBSDF(ray);
		pointScattered = bsdf->dgShading.p;
		const Normal &n = bsdf->dgShading.nn;

		// Next event estimator in surface
		L += pathThroughput * UniformSampleOneLight(scene, pointScattered, n, wo, bsdf, sample);

		// Sample BSDF
		BxDFType flags;
		Spectrum f = bsdf->Sample_f(wo, &wi, lux::random::floatValue(), lux::random::floatValue(), lux::random::floatValue(), &spdf, BSDF_ALL, &flags);
		if (f.Black() || spdf == 0.)
			break;
		specularBounce = (flags & BSDF_SPECULAR) != 0;
		pathThroughput *= f * AbsDot(wi, n) / spdf;
	}

	return L;
}

SurfaceIntegrator* MultipleScatteringPathIntegrator::CreateSurfaceIntegrator(const ParamSet &params) {
	int maxDepth = params.FindOneInt("maxdepth", 8);
	float rrPdf = params.FindOneFloat("rrPdf", 0.8f);
	float g  = params.FindOneFloat("g", 0.0f);
	bool isHomogeneous  = params.FindOneBool("isHomogeneous", false);
	int stepRatio  = params.FindOneInt("stepRatio", 4);
	float stepMultiplier  = params.FindOneFloat("stepMultiplier", 0.5f);
	float stepSize  = params.FindOneFloat("stepsize", 1.0f);
	return new MultipleScatteringPathIntegrator(maxDepth, rrPdf, g, isHomogeneous, stepRatio, stepMultiplier, stepSize);
}
