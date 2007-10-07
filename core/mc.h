/***************************************************************************
 *   Copyright (C) 1998-2007 by authors (see AUTHORS.txt )                 *
 *                                                                         *
 *   This file is part of Lux Renderer.                                    *
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
 *   Lux Renderer website : http://www.luxrender.org                       *
 ***************************************************************************/

#ifndef LUX_MC_H
#define LUX_MC_H
// mc.h*
// MC Utility Declarations
extern  void RejectionSampleDisk(float u1, float u2, float *x, float *y);
 Vector UniformSampleHemisphere(float u1, float u2);
 float  UniformHemispherePdf(float theta, float phi);
 Vector UniformSampleSphere(float u1, float u2);
 float  UniformSpherePdf();
 Vector UniformSampleCone(float u1, float u2, float costhetamax);
 Vector UniformSampleCone(float u1, float u2, float costhetamax,
	const Vector &x, const Vector &y, const Vector &z);
 float  UniformConePdf(float costhetamax);
 void UniformSampleDisk(float u1, float u2, float *x, float *y);
inline Vector CosineSampleHemisphere(float u1, float u2) {
	Vector ret;
	ConcentricSampleDisk(u1, u2, &ret.x, &ret.y);
	ret.z = sqrtf(max(0.f,
	                  1.f - ret.x*ret.x - ret.y*ret.y));
	return ret;
}
inline float CosineHemispherePdf(float costheta, float phi) {
	return costheta * INV_PI;
}
extern  Vector SampleHG(const Vector &w, float g, float u1, float u2);
extern  float HGPdf(const Vector &w, const Vector &wp, float g);
// MC Inline Functions
inline float BalanceHeuristic(int nf, float fPdf, int ng,
		float gPdf) {
	return (nf * fPdf) / (nf * fPdf + ng * gPdf);
}
inline float PowerHeuristic(int nf, float fPdf, int ng,
		float gPdf) {
	float f = nf * fPdf, g = ng * gPdf;
	return (f*f) / (f*f + g*g);
}
#endif // LUX_MC_H
