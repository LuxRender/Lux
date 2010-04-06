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

#ifndef LUX_LIGHT_H
#define LUX_LIGHT_H
// light.h*
#include "lux.h"
#include "geometry/transform.h"
#include "spectrum.h"
#include "error.h"
#include "renderinghints.h"
// Light Declarations

namespace lux
{

class  Light {
public:
	// Light Interface
	virtual ~Light() { }
	Light(const Transform &l2w, u_int ns = 1U)
		: nSamples(max(1U, ns)), LightToWorld(l2w),
		  WorldToLight(l2w.GetInverse()) {
		if (WorldToLight.HasScale())
			luxError(LUX_UNIMPLEMENT,LUX_DEBUG,"Scaling detected in world-to-light transformation! Some lights might not support it yet.");
		havePortalShape = false;
		nrPortalShapes = 0;
		PortalArea = 0;
	}
	virtual float Power(const Scene *scene) const = 0;
	virtual bool IsDeltaLight() const = 0;
	virtual bool IsEnvironmental() const = 0;
	virtual SWCSpectrum Le(const TsPack *tspack, const Scene *scene,
		const Ray &r, const Normal &n, BSDF **bsdf, float *pdf,
		float *pdfDirect) const;
	virtual float Pdf(const TsPack *tspack, const Point &p, const Normal &n,
		const Point &po, const Normal &ns) const = 0;
	virtual SWCSpectrum Sample_L(const TsPack *tspack, const Scene *scene,
		float u1, float u2, float u3, float u4, Ray *ray,
		float *pdf) const = 0;
	virtual bool Sample_L(const TsPack *tspack, const Scene *scene,
		float u1, float u2, float u3, BSDF **bsdf, float *pdf,
		SWCSpectrum *L) const = 0;
	virtual bool Sample_L(const TsPack *tspack, const Scene *scene,
		const Point &p, const Normal &n, float u1, float u2, float u3,
		BSDF **bsdf, float *pdf, float *pdfDirect,
		VisibilityTester *visibility, SWCSpectrum *L) const = 0;
	const LightRenderingHints *GetRenderingHints() const { return &hints; }

	void AddPortalShape(boost::shared_ptr<Primitive> &shape);

	// Light Public Data
	const u_int nSamples;
	u_int nrPortalShapes;
	vector<boost::shared_ptr<Primitive> > PortalShapes;
	float PortalArea;
	u_int group;
protected:
	// Light Protected Data
	const Transform LightToWorld, WorldToLight;
	LightRenderingHints hints;
public: // Put last for better data alignment
	bool havePortalShape;
};

struct VisibilityTester {
	// VisibilityTester Public Methods

	void SetSegment(const Point &p1, const Point & p2, float time,
		bool clip = false) {
		cameraClip = clip;
		// Dade - need to scale the RAY_EPSILON value because the ray direction
		// is not normalized (in order to avoid light leaks: bug #295)
		const Vector w = p2 - p1;
		const float length = w.Length();
		const float shadowRayEpsilon = min(length,
			max(MachineEpsilon::E(p1), MachineEpsilon::E(length)));
		r = Ray(p1, w / length, shadowRayEpsilon,
			length - shadowRayEpsilon);
		r.time = time;
		volume = NULL;
	}

	void SetRay(const Point &p, const Vector & w, float time) {
		r = Ray(p, Normalize(w));
		r.time = time;
		cameraClip = false;
		volume = NULL;
	}

	bool Unoccluded(const Scene * scene) const;
	bool TestOcclusion(const TsPack *tspack, const Scene *scene,
		SWCSpectrum *f, float *pdf = NULL, float *pdfR = NULL) const;
	// modulates the supplied SWCSpectrum with the transmittance along the ray
	void Transmittance(const TsPack *tspack, const Scene * scene,
		const Sample *sample, SWCSpectrum *const L) const;
	Ray r;
	bool cameraClip;
	const Volume *volume;
};

class AreaLight : public Light {
public:
	// AreaLight Interface
	AreaLight(const Transform &light2world,
		boost::shared_ptr<Texture<SWCSpectrum> > &Le, float g,
		float pow, float e, SampleableSphericalFunction *ssf,
		u_int ns, const boost::shared_ptr<Primitive> &prim);
	virtual ~AreaLight();
	virtual SWCSpectrum L(const TsPack *tspack, const Ray &ray,
		const DifferentialGeometry &dg, const Normal &n, BSDF **bsdf,
		float *pdf, float *pdfDirect) const;
	virtual float Power(const Scene *scene) const;
	virtual bool IsDeltaLight() const { return false; }
	virtual bool IsEnvironmental() const { return false; }
	virtual float Pdf(const TsPack *tspack, const Point &p, const Normal &n,
		const Point &po, const Normal &ns) const;
	virtual SWCSpectrum Sample_L(const TsPack *tspack, const Scene *scene,
		float u1, float u2, float u3, float u4, Ray *ray,
		float *pdf) const;
	virtual bool Sample_L(const TsPack *tspack, const Scene *scene,
		float u1, float u2, float u3, BSDF **bsdf, float *pdf,
		SWCSpectrum *Le) const;
	virtual bool Sample_L(const TsPack *tspack, const Scene *scene,
		const Point &p, const Normal &n, float u1, float u2, float u3,
		BSDF **bsdf, float *pdf, float *pdfDirect,
		VisibilityTester *visibility, SWCSpectrum *Le) const;
	static AreaLight *CreateAreaLight(const Transform &light2world,
		const ParamSet &paramSet,
		const boost::shared_ptr<Primitive> &prim);
protected:
	// AreaLight Protected Data
	boost::shared_ptr<Texture<SWCSpectrum> > Le;
	boost::shared_ptr<Primitive> prim;
	float gain, power, efficacy, area;
	SampleableSphericalFunction *func;
};

}//namespace lux

#endif // LUX_LIGHT_H
