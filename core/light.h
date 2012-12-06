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
#include "motionsystem.h"
#include "spectrum.h"
#include "error.h"
#include "renderinghints.h"
#include "queryable.h"
// Light Declarations

namespace lux
{

class  Light {
public:
	// Light Interface
	virtual ~Light() { }
	Light(const Transform &l2w, u_int ns = 1U)
		: nSamples(max(1U, ns)), LightToWorld(l2w) {
		if (LightToWorld.HasScale())
			LOG(LUX_DEBUG,LUX_UNIMPLEMENT)<< "Scaling detected in light-to-world transformation! Some lights might not support it yet.";
		havePortalShape = false;
		nrPortalShapes = 0;
		PortalArea = 0;
	}
	const Volume *GetVolume() const { return volume.get(); }
	void SetVolume(boost::shared_ptr<Volume> &v) {
		// Create a temporary to increase shared count
		// The assignment is just a swap
		boost::shared_ptr<Volume> vol(v);
		volume = vol;
	}
	virtual float Power(const Scene &scene) const = 0;
	virtual bool IsDeltaLight() const = 0;
	virtual bool IsEnvironmental() const = 0;
	virtual bool Le(const Scene &scene, const Sample &sample, const Ray &r,
		BSDF **bsdf, float *pdf, float *pdfDirect,
		SWCSpectrum *L) const { return false; }
	virtual float Pdf(const Point &p, const PartialDifferentialGeometry &dg) const = 0;
	virtual bool SampleL(const Scene &scene, const Sample &sample,
		float u1, float u2, float u3, BSDF **bsdf, float *pdf,
		SWCSpectrum *L) const = 0;
	virtual bool SampleL(const Scene &scene, const Sample &sample,
		const Point &p, float u1, float u2, float u3,
		BSDF **bsdf, float *pdf, float *pdfDirect,
		SWCSpectrum *L) const = 0;
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
	const Transform LightToWorld;
	LightRenderingHints hints;
public: // Put last for better data alignment
	bool havePortalShape;
protected:
	boost::shared_ptr<Volume> volume;
};

class AreaLight : public Light, public Queryable {
public:
	// AreaLight Interface
	AreaLight(const Transform &light2world,
		boost::shared_ptr<Texture<SWCSpectrum> > &Le, float g,
		float pow, float e, SampleableSphericalFunction *ssf,
		u_int ns, const boost::shared_ptr<Primitive> &prim);
	virtual ~AreaLight();
	virtual bool L(const Sample &sample, const Ray &ray,
		const DifferentialGeometry &dg, BSDF **bsdf, float *pdf,
		float *pdfDirect, SWCSpectrum *Le) const;
	virtual float Power(const Scene &scene) const;
	virtual bool IsDeltaLight() const { return false; }
	virtual bool IsEnvironmental() const { return false; }
	virtual float Pdf(const Point &p, const PartialDifferentialGeometry &dg) const;
	virtual bool SampleL(const Scene &scene, const Sample &sample,
		float u1, float u2, float u3, BSDF **bsdf, float *pdf,
		SWCSpectrum *Le) const;
	virtual bool SampleL(const Scene &scene, const Sample &sample,
		const Point &p, float u1, float u2, float u3,
		BSDF **bsdf, float *pdf, float *pdfDirect,
		SWCSpectrum *Le) const;
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

class  InstanceLight : public Light {
public:
	// Light Interface
	virtual ~InstanceLight() { }
	InstanceLight(const Transform &l2w, boost::shared_ptr<Light> &l)
		: Light(l2w, l->nSamples), light(l) { }
	virtual float Power(const Scene &scene) const {
		return light->Power(scene);
	}
	virtual bool IsDeltaLight() const { return light->IsDeltaLight(); }
	virtual bool IsEnvironmental() const {
		return light->IsEnvironmental();
	}
	virtual bool Le(const Scene &scene, const Sample &sample, const Ray &r,
		BSDF **bsdf, float *pdf, float *pdfDirect,
		SWCSpectrum *L) const;
	virtual float Pdf(const Point &p, const PartialDifferentialGeometry &dg) const {
		const PartialDifferentialGeometry dgi(Inverse(LightToWorld) *
			dg);
		const float factor = dgi.Volume() / dg.Volume();
		return light->Pdf(Inverse(LightToWorld) * p, dgi) * factor;
	}
	virtual bool SampleL(const Scene &scene, const Sample &sample,
		float u1, float u2, float u3, BSDF **bsdf, float *pdf,
		SWCSpectrum *L) const;
	virtual bool SampleL(const Scene &scene, const Sample &sample,
		const Point &p, float u1, float u2, float u3,
		BSDF **bsdf, float *pdf, float *pdfDirect,
		SWCSpectrum *L) const;

protected:
	boost::shared_ptr<Light> light;
};

class  MotionLight : public Light {
public:
	// Light Interface
	virtual ~MotionLight() { }
	MotionLight(const MotionSystem &mp, boost::shared_ptr<Light> &l)
		: Light(Transform(), l->nSamples), light(l), motionPath(mp) { }
	virtual float Power(const Scene &scene) const {
		return light->Power(scene);
	}
	virtual bool IsDeltaLight() const { return light->IsDeltaLight(); }
	virtual bool IsEnvironmental() const {
		return light->IsEnvironmental();
	}
	virtual bool Le(const Scene &scene, const Sample &sample, const Ray &r,
		BSDF **bsdf, float *pdf, float *pdfDirect,
		SWCSpectrum *L) const;
	virtual float Pdf(const Point &p, const PartialDifferentialGeometry &dg) const {
		const Transform LightToWorld(motionPath.Sample(dg.time));
		const PartialDifferentialGeometry dgi(Inverse(LightToWorld) *
			dg);
		const float factor = dgi.Volume() / dg.Volume();
		return light->Pdf(Inverse(LightToWorld) * p, dgi) * factor;
	}
	virtual bool SampleL(const Scene &scene, const Sample &sample,
		float u1, float u2, float u3, BSDF **bsdf, float *pdf,
		SWCSpectrum *L) const;
	virtual bool SampleL(const Scene &scene, const Sample &sample,
		const Point &p, float u1, float u2, float u3,
		BSDF **bsdf, float *pdf, float *pdfDirect,
		SWCSpectrum *L) const;

protected:
	boost::shared_ptr<Light> light;
	MotionSystem motionPath;
};

}//namespace lux

#endif // LUX_LIGHT_H
