#include "lux.h"
#include "transport.h"
#include "scene.h"
#include "volume.h"
#include "stdio.h"

namespace lux
{

class MultipleScatteringPathIntegrator : public SurfaceIntegrator {
public:
	MultipleScatteringPathIntegrator(int md, float rr, float gg, bool isH, int ra, float mul, float ss)
	{
		maxDepth = md;
		rrPdf = rr;
		g = gg;
		isHomogeneous = isH;
		stepRatio = ra;
		stepMultiplier = mul;
		stepSize = ss;
	}
	Spectrum Li(const Scene *scene, const RayDifferential &ray, const Sample *sample, float *alpha) const;
	void RequestSamples(Sample *sample, const Scene *scene);
	Spectrum Transmittance(const Ray &ray) const;

	virtual MultipleScatteringPathIntegrator* clone() const // Lux (copy) constructor for multithreading
	{
		return new MultipleScatteringPathIntegrator(*this);
	}
	IntegrationSampler* HasIntegrationSampler(IntegrationSampler *is) { return NULL; }; // Not implemented
	static SurfaceIntegrator *CreateSurfaceIntegrator(const ParamSet &params);
private:
	int maxDepth;
	float rrPdf;
	float g;
	bool isHomogeneous;
	int stepRatio;
	float stepMultiplier;
	float dMin;
	float dMax;
	float stepSize;
	VolumeRegion *vr;
};

}