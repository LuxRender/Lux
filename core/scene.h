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

#ifndef LUX_SCENE_H
#define LUX_SCENE_H
// scene.h*
#include "lux.h"
#include "api.h"
#include "primitive.h"
#include "transport.h"

#include <boost/thread/thread.hpp>
#include <boost/noncopyable.hpp>
#include <boost/thread/mutex.hpp>

#include "luxrays/core/dataset.h"

namespace lux {

// Scene Declarations
class  Scene {
public:
	// Scene Public Methods
	Scene(Camera *c, SurfaceIntegrator *in, VolumeIntegrator *vi,
		Sampler *s, vector<boost::shared_ptr<Primitive> >  prims,
		boost::shared_ptr<Primitive> &accel,
		const vector<Light *> &lts, const vector<string> &lg,
		Region *vr);
	Scene(Camera *c);
	~Scene();
	bool Intersect(const Ray &ray, Intersection *isect) const {
		return aggregate->Intersect(ray, isect);
	}
	bool Intersect(const luxrays::RayHit &rayHit, Intersection *isect) const {
		if (rayHit.Miss())
			return false;
		else {
			// Something was hit
			const unsigned int currentTriangleIndex = rayHit.index;
			const unsigned int currentPrimIndex = dataSet->GetMeshID(currentTriangleIndex);
			const unsigned int triIndex = dataSet->GetMeshTriangleID(currentTriangleIndex);

			tesselatedPrimitives[currentPrimIndex]->GetIntersection(rayHit, triIndex, isect);

			return true;
		}
	}
	bool Intersect(const Sample &sample, const Volume *volume,
		bool scatteredStart, const Ray &ray, float u,
		Intersection *isect, BSDF **bsdf, float *pdf, float *pdfBack,
		SWCSpectrum *f) const {
		return volumeIntegrator->Intersect(*this, sample, volume,
			scatteredStart, ray, u, isect, bsdf, pdf, pdfBack, f);
	}
	// Used to complete intersection data with LuxRays
	bool Intersect(const Sample &sample, const Volume *volume,
		bool scatteredStart, const Ray &ray,
		const luxrays::RayHit &rayHit, float u, Intersection *isect,
		BSDF **bsdf, float *pdf, float *pdfBack, SWCSpectrum *f) const {
		return volumeIntegrator->Intersect(*this, sample, volume,
			scatteredStart, ray, rayHit, u, isect, bsdf, pdf,
			pdfBack, f);
	}
	bool Connect(const Sample &sample, const Volume *volume,
		bool scatteredStart, bool scatteredEnd, const Point &p0,
		const Point &p1, bool clip, SWCSpectrum *f, float *pdf,
		float *pdfR) const {
		return volumeIntegrator->Connect(*this, sample, volume,
			scatteredStart, scatteredEnd, p0, p1, clip, f, pdf,
			pdfR);
	}
	// Used with LuxRays
	int Connect(const Sample &sample, const Volume **volume,
		bool scatteredStart, bool scatteredEnd, const Ray &ray,
		const luxrays::RayHit &rayHit, SWCSpectrum *f, float *pdf,
		float *pdfR) const {
		return volumeIntegrator->Connect(*this, sample, volume,
			scatteredStart, scatteredEnd, ray, rayHit, f, pdf,
			pdfR);
	}
	bool IntersectP(const Ray &ray) const {
		return aggregate->IntersectP(ray);
	}
	const BBox &WorldBound() const { return bound; }
	SWCSpectrum Li(const Ray &ray, const Sample &sample,
		float *alpha = NULL) const;
	// modulates the supplied SWCSpectrum with the transmittance along the ray
	void Transmittance(const Ray &ray, const Sample &sample,
		SWCSpectrum *const L) const;

	//framebuffer access
	void UpdateFramebuffer();
	unsigned char* GetFramebuffer();
	float* GetFloatFramebuffer();
	float* GetAlphaBuffer();
	float* GetZBuffer();
	void SaveFLM(const string& filename);
	void SaveEXR(const string& filename, bool useHalfFloat, bool includeZBuffer, int compressionType, bool tonemapped);

	//histogram access
	void GetHistogramImage(unsigned char *outPixels, u_int width,
		u_int height, int options);

	// Parameter Access functions
	void SetParameterValue(luxComponent comp, luxComponentParameters param,
		double value, u_int index);
	double GetParameterValue(luxComponent comp,
		luxComponentParameters param, u_int index);
	double GetDefaultParameterValue(luxComponent comp,
		luxComponentParameters param, u_int index);
	void SetStringParameterValue(luxComponent comp,
		luxComponentParameters param, const string& value, u_int index);
	string GetStringParameterValue(luxComponent comp,
		luxComponentParameters param, u_int index);
	string GetDefaultStringParameterValue(luxComponent comp,
		luxComponentParameters param, u_int index);

	int DisplayInterval();
	u_int FilmXres();
	u_int FilmYres();

	bool IsFilmOnly() const { return filmOnly; }

	// Scene Data
	boost::shared_ptr<Primitive> aggregate;
	vector<Light *> lights;
	vector<string> lightGroups;
	Camera *camera;
	Region *volumeRegion;
	SurfaceIntegrator *surfaceIntegrator;
	VolumeIntegrator *volumeIntegrator;
	Sampler *sampler;
	BBox bound;
	u_long seedBase;
	bool terminated; // rendering is terminated

	// The following data are used when tracing rays with LuxRays
	// The list of original primitives. It is required by LuxRays to build the DataSet.
	vector<boost::shared_ptr<Primitive> > primitives;
	vector<const Primitive *> tesselatedPrimitives;
	luxrays::DataSet *dataSet;

private:
	bool filmOnly; // whether this scene has entire scene (incl. geometry, ..) or only a film
};

}//namespace lux

#endif // LUX_SCENE_H
