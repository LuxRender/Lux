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

// environment.cpp*
#include "camera.h"

namespace lux
{

// EnvironmentCamera Declarations
class EnvironmentCamera : public Camera {
public:
	// EnvironmentCamera Public Methods
	EnvironmentCamera(const Transform &world2camStart, const Transform &world2camEnd, float hither,
		float yon, float sopen, float sclose, int sdist, Film *film);
	float GenerateRay(const Sample &sample, Ray *) const;
	bool IsVisibleFromEyes(const Scene *scene, const Point &lenP, const Point &worldP, Sample* sample_gen, Ray *ray_gen) const;
	float GetConnectingFactor(const Point &lenP, const Point &worldP, const Vector &wo, const Normal &n) const;
	void GetFlux2RadianceFactors(Film *film, float *factors, int xPixelCount, int yPixelCount) const;
	bool IsDelta() const
	{
		return true;
	}
	void SamplePosition(float u1, float u2, float u3, Point *p, float *pdf) const;
	float EvalPositionPdf() const;
	//float SampleDirection(const Sample &sample, Ray *ray)
	//{
	//	// FixMe: Duplicated code
	//	ray->o = rayOrigin;
	//	// Generate environment camera ray direction
	//	float theta = M_PI * sample.imageY / film->yResolution;
	//	float phi = 2 * M_PI * sample.imageX / film->xResolution;
	//	Vector dir(sinf(theta) * cosf(phi), cosf(theta),
	//		sinf(theta) * sinf(phi));
	//	CameraToWorld(dir, &ray->d);
	//	// Set ray time value
	//	ray->time = Lerp(sample.time, ShutterOpen, ShutterClose);
	//	ray->mint = ClipHither;
	//	ray->maxt = ClipYon;

	//	// R*R/(Apixel*nx*ny)
	//	// sub-pdf in Apixel is not correct
	//	return 1.0f/(2.0f*M_PI*M_PI*sinf(M_PI*((int)sample.imageY+0.5f)/film->yResolution));

	//}
	//float EvalDirectionPdf(Film *film, const Vector& wo, const Sample &sample, const Point& p)
	//{
	//	return 1.0f/(2.0f*M_PI*M_PI*sinf(M_PI*((int)sample.imageY+0.5f)/film->yResolution));
	//}
	//SWCSpectrum EvalValue()
	//{
	//	return SWCSpectrum(1.0f);
	//}

	EnvironmentCamera* Clone() const {
		return new EnvironmentCamera(*this);
	}

	static Camera *CreateCamera(const Transform &world2camStart, const Transform &world2camEnd, const ParamSet &params, Film *film);
private:
	bool GenerateSample(const Point &p, Sample *sample) const;
	// EnvironmentCamera Private Data
	//Point rayOrigin;
};

}//namespace lux
