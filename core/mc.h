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

#ifndef LUX_MC_H
#define LUX_MC_H
// mc.h*

#include "geometry/vector.h"

// MC Utility Declarations
namespace lux
{

	double normsinv(double p);
		
	void RejectionSampleDisk(float u1, float u2, float *x, float *y);
	Vector UniformSampleHemisphere(float u1, float u2);
	float  UniformHemispherePdf(float theta, float phi);
	Vector UniformSampleSphere(float u1, float u2);
	float  UniformSpherePdf();
	Vector UniformSampleCone(float u1, float u2, float costhetamax);
	Vector UniformSampleCone(float u1, float u2, float costhetamax,
		const Vector &x, const Vector &y, const Vector &z);
	float  UniformConePdf(float costhetamax);
	void UniformSampleDisk(float u1, float u2, float *x, float *y);
	void UniformSampleTriangle(float ud1, float ud2, float *u, float *v);
	Vector SampleHG(const Vector &w, float g, float u1, float u2);
	float HGPdf(const Vector &w, const Vector &wp, float g);
	void ComputeStep1dCDF(const float *f, u_int nValues, float *c, float *cdf);
	float SampleStep1d(const float *f, const float *cdf, float c, u_int nSteps, float u,
		float *weight);
	void ConcentricSampleDisk(float u1, float u2, float *dx, float *dy);
	float GaussianSampleDisk(float u1);
	float InverseGaussianSampleDisk(float u1);
	float ExponentialSampleDisk(float u1, int power);
	float InverseExponentialSampleDisk(float u1, int power);
	float TriangularSampleDisk(float u1);
	void HoneycombSampleDisk(float u1, float u2, float *dx, float *dy);

	// MC Inline Functions
	inline Vector CosineSampleHemisphere(float u1, float u2)
	{
		Vector ret;
		ConcentricSampleDisk(u1, u2, &ret.x, &ret.y);
		ret.z = sqrtf(max(0.f, 1.f - ret.x * ret.x - ret.y * ret.y));
		return ret;
	}
	inline float CosineHemispherePdf(float costheta, float phi) {
		return costheta * INV_PI;
	}
	inline float BalanceHeuristic(u_int nf, float fPdf, u_int ng, float gPdf)
	{
		return (nf * fPdf) / (nf * fPdf + ng * gPdf);
	}
	inline float PowerHeuristic(u_int nf, float fPdf, u_int ng, float gPdf)
	{
		const float f = nf * fPdf, g = ng * gPdf;
		return (f * f) / (f * f + g * g);
	}

}//namespace lux

#endif // LUX_MC_H
