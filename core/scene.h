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

namespace lux {

// Scene Declarations
class  Scene {
public:
	// Scene Public Methods
	Scene(Camera *c, SurfaceIntegrator *in, VolumeIntegrator *vi,
		Sampler *s, boost::shared_ptr<Primitive> &accel,
		const vector<Light *> &lts, const vector<string> &lg,
		Region *vr);
	Scene(Camera *c);
	~Scene();
	bool Intersect(const Ray &ray, Intersection *isect) const {
		return aggregate->Intersect(ray, isect);
	}
	bool Intersect(const Sample &sample, const Volume *volume,
		const RayDifferential &ray, Intersection *isect, BSDF **bsdf,
		SWCSpectrum *f) const {
		return volumeIntegrator->Intersect(*this, sample, volume, ray,
			isect, bsdf, f);
	}
	bool Connect(const Sample &sample, const Volume *volume,
		const Point &p0, const Point &p1, bool clip,
		SWCSpectrum *f, float *pdf, float *pdfR) const {
		return volumeIntegrator->Connect(*this, sample, volume,
			p0, p1, clip, f, pdf, pdfR);
	}
	bool IntersectP(const Ray &ray) const {
		return aggregate->IntersectP(ray);
	}
	const BBox &WorldBound() const { return bound; }
	SWCSpectrum Li(const RayDifferential &ray, const Sample &sample,
		float *alpha = NULL) const;
	// modulates the supplied SWCSpectrum with the transmittance along the ray
	void Transmittance(const Ray &ray, const Sample &sample,
		SWCSpectrum *const L) const;

	//framebuffer access
	void UpdateFramebuffer();
	unsigned char* GetFramebuffer();
	void SaveFLM(const string& filename);

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

private:
	bool filmOnly; // whether this scene has entire scene (incl. geometry, ..) or only a film
};

}//namespace lux

#endif // LUX_SCENE_H
