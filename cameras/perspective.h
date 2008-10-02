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

// perspective.cpp*
#include "camera.h"
#include "spectrum.h"
#include "sampling.h"
#include "mc.h"

namespace lux
{

// PerspectiveCamera Declarations
class PerspectiveCamera : public ProjectiveCamera {
public:
	// PerspectiveCamera Public Methods
	PerspectiveCamera(const Transform &world2cam,
		const float Screen[4], float hither, float yon,
		float sopen, float sclose,
		float lensr, float focald, bool autofocus, float fov,
		Film *film);
	float GenerateRay(const Sample &sample, Ray *) const;
	SWCSpectrum Sample_W(const Scene *scene, float u1, float u2, BSDF **bsdf, float *pdf) const;
	SWCSpectrum Sample_W(const Scene *scene, const Point &p, const Normal &n, float u1, float u2, BSDF **bsdf, float *pdf, float *pdfDirect, VisibilityTester *visibility) const;
	float Pdf(const Point &p, const Normal &n, const Vector &wi) const;
	void GetSamplePosition(const Point &p, const Vector &wi, float *x, float *y) const;
	bool IsVisibleFromEyes(const Scene *scene, const Point &lenP, const Point &worldP, Sample* sample_gen, Ray *ray_gen) const;
	float GetConnectingFactor(const Point &lenP, const Point &worldP, const Vector &wo, const Normal &n) const;
	void GetFlux2RadianceFactors(Film *film, float *factors, int xPixelCount, int yPixelCount) const;
	bool IsDelta() const
	{
		return LensRadius==0.0f;
	}
	bool Intersect(const Ray &ray, Intersection *isect) const;
	void SamplePosition(float u1, float u2, float u3, Point *p, float *pdf) const;
	float EvalPositionPdf() const;
	void AutoFocus(Scene* scene);

	//float SampleDirection(const Sample &sample, Ray *ray)
	//{
	//	GenerateRay(sample,ray);
	//	//return EvalDirectionPdf((Film*)NULL, ray->d, sample, Point(0,0,0))			
	//	return -1.0f;
	//}
	//float EvalDirectionPdf(Film *film, const Vector& wo, const Sample &sample, const Point& p)
	//{
	//	float detaX = 0.5f * xWidth - sample.imageX * xPixelWidth;
	//	float detaY = 0.5f * yHeight - sample.imageY * yPixelHeight;
	//	float distPixel2 = detaX * detaX + detaY * detaY + R * R;
	//	float cosPixel = Dot(wo, normal);
	//	float pdf = 1 / Apixel * distPixel2/cosPixel;

	//	if ( LensRadius != 0.0f )
	//	{
	//		float cos1 = Dot(wo,normal);
	//		Vector ProjectedDir = wo-(Vector)(cos1*normal)+(p-pos);
	//		// TODO: FocalDistance or FocalDistance - ClipHither
	//		float cos2 = FocalDistance / sqrt(FocalDistance*FocalDistance+ProjectedDir.LengthSquared());
	//		float t = cos1/cos2;
	//		pdf *= t*t*t;
	//	}
	//	return pdf;
	//	
	//}
	//SWCSpectrum EvalValue()
	//{
	//	return SWCSpectrum(1.0f);
	//}

	static Camera *CreateCamera(const Transform &world2cam, const ParamSet &params, Film *film);

private:
	Point pos;
	Normal normal;
	float fov;
	float posPdf;
	float screen[4];
	float R,xWidth,yHeight,xPixelWidth,yPixelHeight,Apixel;

	// Dade - field used for autofocus feature
	bool autoFocus;

	boost::shared_ptr<Shape> lens;
};

}//namespace lux
