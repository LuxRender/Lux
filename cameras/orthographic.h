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

// orthographic.cpp*
#include "camera.h"

namespace lux
{

// OrthographicCamera Declarations
class OrthoCamera : public ProjectiveCamera {
public:
	// OrthoCamera Public Methods
	OrthoCamera(const Transform &world2cam,
	            const float Screen[4],
		        float hither, float yon,
				float sopen, float sclose,
				float lensr, float focald, bool autofocus, Film *film);
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
	void AutoFocus(Scene* scene);

	//float SampleDirection(const Sample &sample, Ray *ray)
	//{
	//	Point Pras(sample.imageX, sample.imageY, 0);
	//	RasterToCamera(Pras, &(ray->o));
	//	ray->d = Vector(0,0,1);
	//	ray->mint = 0.;
	//	ray->maxt = ClipYon - ClipHither;
	//	CameraToWorld(*ray, ray);
	//	return 1.0f;
	//}
	//float EvalDirectionPdf(Film *film, const Vector& wo, const Sample &sample, const Point& p)
	//{
	//	return 1.0f;
	//}
	//SWCSpectrum EvalValue()
	//{
	//	return SWCSpectrum(1.0f);
	//}

	static Camera *CreateCamera(const ParamSet &params, const Transform &world2cam, Film *film);

private:	
	// Dade - field used for autofocus feature
	bool autoFocus;

	float screenDx,screenDy;
};

}//namespace lux
