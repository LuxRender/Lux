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

// torus.cpp*
#include "torus.h"
#include "paramset.h"
#include "dynload.h"

using namespace lux;

int signR(double Z)
{
	if (Z > 0.0) return 1;
	if (Z < 0.0) return -1;
	
	return 0;
}

double CBRT(double Z)
{
	double ret;
	const double THIRD = 1./3.;
	//define cubic root as statement function
	//SIGN has different meanings in both C and Fortran
	// Was unable to use the sign command of C, so wrote my own
	// that why a new variable needs to be introduced that keeps track of the sign of
	// SIGN is supposed to return a 1, -1 or 0 depending on what the sign of the argument is
	//ret = fabs(pow(fabs(Z),THIRD)) * signR(Z);
	ret = pow(fabs(Z),THIRD) * signR(Z);
	return ret;
}

/*-------------------- Global Function Description Block ----------------------
 *
 *     ***CUBIC************************************************08.11.1986
 *     Solution of a cubic equation
 *     Equations of lesser degree are solved by the appropriate formulas.
 *     The solutions are arranged in ascending order.
 *     Returns number of real roots found.
 *     NO WARRANTY, ALWAYS TEST THIS SUBROUTINE AFTER DOWNLOADING
 *     ******************************************************************
 *     A      (i)  vector containing the polynomial coefficients
 *     X      (o)  results
 *     ==================================================================
 *  	17-Oct-2004 / Raoul Rausch
 *		Conversion from Fortran to C
 *
 *     minor modifications by Lord Crc
 *
 *
 *-----------------------------------------------------------------------------
 */
int cubic(double A[4], double X[3])
{
	const double PI = 3.1415926535897932;
	const double THIRD = 1./3.;
	double W, P, Q, DIS, PHI;
	int L;

	//define cubic root as statement function
	// In C, the function is defined outside of the cubic fct

	// ====determine the degree of the polynomial ====

	if (A[3] != 0.0) {
		//cubic problem
		W = A[2]/A[3]*THIRD;
		double W2 = W*W;
		double P1 = -(A[1]/A[3]*THIRD - W2);
		P = P1*P1*P1;
		Q = -.5*(2.0*W2*W-(A[1]*W-A[0])/A[3]);
		DIS = Q*Q-P;
		if ( DIS < 0.0 ) {
			//three real solutions!
			//Confine the argument of ACOS to the interval [-1;1]!
			PHI = acosf(min(1.0,max(-1.0,Q/sqrt(P))));
			P = 2.0*sqrt(P1);
			for (int i = 0; i < 3; i++)
				X[i] = P*cosf((PHI+2*((double)i)*PI)*THIRD)-W;
			L = 3;
		}
		else {
			// only one real solution!
			DIS = sqrt(DIS);
			X[0] = CBRT(Q+DIS)+CBRT(Q-DIS)-W;
			L = 1;
		}
	}
	else if (A[2] != 0.0) {
		// quadratic problem
		P = 0.5*A[1]/A[2];
		DIS = P*P-A[0]/A[2];
		if (DIS > 0.0) {
			// 2 real solutions
			DIS = sqrt(DIS);
			X[0] = -P - DIS;
			X[1] = -P + DIS;
			L = 2;
		}
		else {
			// no real solution
			return 0;
		}
	}
	else if (A[1] != 0.0) {
		//linear equation
		X[0] = A[0]/A[1];
		L = 1;
	}
	else {
		//no equation
		return 0;
	}
 
	// perform one step of a newton iteration in order to minimize
	// round-off errors
	for (int i = 0; i < L; i++) {
		X[i] -= (A[0]+X[i]*(A[1]+X[i]*(A[2]+X[i]*A[3]))) / (A[1]+X[i]*(2.0*A[2]+X[i]*3.0*A[3]));
	}

	return L;
}


/*-------------------- Global Function Description Block ----------------------
 *
 *     ***QUARTIC************************************************25.03.98
 *     Solution of a quartic equation
 *     Returns number of real solutions.
 *     Modified to return real roots only, in ascending order. 
 *     ref.: J. E. Hacke, Amer. Math. Monthly, Vol. 48, 327-328, (1941)
 *     NO WARRANTY, ALWAYS TEST THIS SUBROUTINE AFTER DOWNLOADING
 *     ******************************************************************
 *     dd(0:4)     (i)  vector containing the polynomial coefficients
 *     sol(1:4)    (o)  results, real part
 *     ==================================================================
 *  	17-Oct-2004 / Raoul Rausch
 *		Conversion from Fortran to C
 *
 *     modifications by Lord Crc
 *
 *-----------------------------------------------------------------------------
 */
 int quartic(double dd[5], double sol[4])
 {
	double AA[4], z[3];
	double a, b, c, d, f, p, q, r, zsol, xK2, xL, xK, sqp, sqm;
	int ncube;
	int Nsol = 0;

	if (dd[4] == 0.0)
	{
		return 0;
	}

	a = dd[4];
	b = dd[3];
	c = dd[2];
	d = dd[1];
	f = dd[0];

	double aa = a*a;
	double bb = b*b;

	p = (-3.0*b*b + 8.0 *a*c)/(8.0*aa);
	q = (bb*b - 4.0*a*b*c + 8.0*d*aa) / (8.0*aa*a);
	r = (-3.0*bb*bb + 16.0*a*bb*c - 64.0*aa*b*d + 256.0*aa*a*f)/(256.0*aa*aa);
	
	// Solve cubic resolvent
	AA[3] = 8.0;
	AA[2] = -4.0*p;
	AA[1] = -8.0*r;
	AA[0] = 4.0*p*r - q*q;

	ncube = cubic(AA, z);
	
	zsol = -1.e99;
	for (int i = 0; i < ncube; i++)
		zsol = max(zsol, z[i]);

	xK2 = 2.0*zsol - p;
	xK = sqrt(xK2);
	xL = q/(2.0*xK);
	sqp = xK2 - 4.0 * (zsol + xL);
	sqm = xK2 - 4.0 * (zsol - xL);

	if (sqp >= 0.0) {
		sqp = sqrt(sqp);

		if (sqm >= 0.0) {
			sqm = sqrt(sqm);
			sol[3] = 0.5 * (xK + sqp);
			sol[2] = 0.5 * (xK - sqp);
			sol[1] = 0.5 * (-xK + sqm);
			sol[0] = 0.5 * (-xK - sqm);
			Nsol = 4;
		}
		else {
			sol[1] = 0.5 * (xK + sqp);
			sol[0] = 0.5 * (xK - sqp);
			Nsol = 2;
		}
	}
	else {
		if (sqm < 0.0)
			// no real roots
			return 0;

		sqm = sqrt(sqm);

		// if ( (sqp < 0.0) && (sqm >= 0.0))
		sol[1] = 0.5 * (-xK + sqm);
		sol[0] = 0.5 * (-xK - sqm);
		Nsol = 2;
	}
	
	for (int i = 0; i < Nsol; i++)
		sol[i] -= b/(4.0*a);

	return Nsol;
 }

 bool Torus::FindIntersection(const Ray &ray, float *tHit, Point *pHit, 
	 float *phiHit, float *thetaHit) const {
 
	// Compute quartic torus coefficients

	double r2 = minorRadius * minorRadius;
	double R2 = majorRadius * majorRadius;

	double dd = Dot(ray.d, ray.d);
	double pd = Dot(Vector(ray.o), ray.d);
	double pp = Dot(Vector(ray.o), Vector(ray.o));
	double prR = pp - r2 - R2;

	double coef[5];

	coef[4] = dd*dd;
	coef[3] = 4.0*dd*pd;
	coef[2] = 4.0*pd*pd + 2.0*dd*prR + 4.0*R2*ray.d.z*ray.d.z;
	coef[1] = 4.0*pd*prR + 8.0*R2*ray.o.z*ray.d.z;
	coef[0] = prR*prR + 4.0*R2*(ray.o.z*ray.o.z - r2);

	double t[4];
	int Nsol;

	// Solve quartic equation for _t_ values
	Nsol = quartic(coef, t);
	if (Nsol < 1)
		return false;

	// Compute intersection distance along ray
	double thit = t[0];
	int ti = 0;
	double tmax = t[Nsol-1];

	if (thit > ray.maxt || tmax < ray.mint)
		return false;

	// find first usable solution
	if (thit < ray.mint) {
		for (int i = 1; i < Nsol && thit < ray.mint; i++) {
			thit = t[i];
			ti = i;
		}
		if (thit > ray.maxt) 
			return false;
	}

	// Compute torus hit position, $\phi$ and $\tetha$
	Point phit;
	float phi, theta;
	while (true) {
		phit = ray(thit);
		// TODO - if spindle or horn torus, phit.x may be zero?
		phi = atan2f(phit.y, phit.x);
		if (phi < 0.) phi += 2.f*M_PI;
		
		// clamp in case of precision issues
		float costheta = Clamp(phit.z / minorRadius, -1.f, 1.f);
		theta = acosf(costheta);
		// adjust theta if hit lies on the inner half of the torus
		if (phit.x*phit.x + phit.y*phit.y < majorRadius*majorRadius)
			theta = 2*M_PI - theta;

		// Test torus intersection against clipping parameters
		if (!(theta < thetaMin || theta > thetaMax || phi > phiMax))
			break;

		// test failed, try next solution
		ti++;
		if (ti >= Nsol)
			return false;
		thit = t[ti];
		if (thit > ray.maxt) 
			return false;
	}

	*pHit = phit;
	*tHit = thit;
	*phiHit = phi;
	*thetaHit = theta;

	return true;
 }
 
// Torus Method Definitions
Torus::Torus(const Transform &o2w, bool ro, float marad, float mirad,
               float tmi, float tma, float pm)
	: Shape(o2w, ro) {
	majorRadius = marad;
	minorRadius = mirad;
	thetaMin = Radians(Clamp(min(tmi, tma), 0.f, 360.f));
	thetaMax = Radians(Clamp(max(tmi, tma), 0.f, 360.f));
	phiMax = Radians(Clamp(pm, 0.0f, 360.0f));
	// find zmin and zmax, used to tighten bounding box
	if (thetaMin < M_PI && thetaMax > M_PI)		
		zmin = -minorRadius;
	else
		zmin = minorRadius * min(cosf(thetaMin), cosf(thetaMax));
	zmax = minorRadius * max(cosf(thetaMin), cosf(thetaMax));
}

BBox Torus::ObjectBound() const {
	return BBox(Point(-majorRadius-minorRadius, -majorRadius-minorRadius, -minorRadius),
		Point(majorRadius+minorRadius, majorRadius+minorRadius, minorRadius));
}

bool Torus::Intersect(const Ray &r, float *tHit,
		DifferentialGeometry *dg) const {
	float phi, theta;
	float thit;
	Point phit;
	// Transform _Ray_ to object space
	Ray ray;
	WorldToObject(r, &ray);

	if (!FindIntersection(ray, &thit, &phit, &phi, &theta))
		return false;

	// Find parametric representation of torus hit
	// torus is parameterized as follows:
	// x = R*cos(phi) + r*cos(phi)*sin(theta)
	// y = R*sin(phi) + r*sin(phi)*sin(theta)
	// z = r*cos(theta)

	float u = phi / phiMax;
	float v = (theta - thetaMin) / (thetaMax - thetaMin);

	// Compute torus \dpdu and \dpdv
	float cosphi, sinphi, sintheta;
	Vector dpdu, dpdv;

	sintheta = sinf(theta);

	float zradius = sqrtf(phit.x*phit.x + phit.y*phit.y);
	if (zradius == 0)
	{
		// Handle hit at degenerate parameterization point
		// (spindle or horn torus)
		cosphi = 0;
		sinphi = 1;
		dpdv = (thetaMax-thetaMin) *
			Vector(phit.z * cosphi, phit.z * sinphi,
				-minorRadius * sintheta);
		Vector norm = Vector(phit);
		dpdu = Cross(dpdv, norm);
	}
	else
	{
		float invzradius = 1.f / zradius;
		cosphi = phit.x * invzradius;
		sinphi = phit.y * invzradius;
		dpdu = Vector(-phiMax * phit.y, phiMax * phit.x, 0);
		dpdv = (thetaMax-thetaMin) *
			Vector(phit.z * cosphi, phit.z * sinphi,
				-minorRadius * sintheta);
	}
	// Compute torus \dndu and \dndv
	Vector d2Pduu = -phiMax * phiMax * Vector(phit.x, phit.y, 0);
	Vector d2Pduv = (thetaMax - thetaMin) *
	                 phit.z * phiMax *
	                 Vector(-sinphi, cosphi, 0.);
	Vector d2Pdvv = -(thetaMax - thetaMin) *
	                 (thetaMax - thetaMin) *
	                 Vector(minorRadius*cosphi*sintheta, minorRadius*sinphi*sintheta, phit.z);
	// Compute coefficients for fundamental forms
	float E = Dot(dpdu, dpdu);
	float F = Dot(dpdu, dpdv);
	float G = Dot(dpdv, dpdv);
	Vector N = Normalize(Cross(dpdu, dpdv));
	float e = Dot(N, d2Pduu);
	float f = Dot(N, d2Pduv);
	float g = Dot(N, d2Pdvv);
	// Compute \dndu and \dndv from fundamental form coefficients
	float invEGF2 = 1.f / (E*G - F*F);
	Normal dndu((f*F - e*G) * invEGF2 * dpdu +
		(e*F - f*E) * invEGF2 * dpdv);
	Normal dndv((g*F - f*G) * invEGF2 * dpdu +
		(f*F - g*E) * invEGF2 * dpdv);
	// Initialize _DifferentialGeometry_ from parametric information
	*dg = DifferentialGeometry(ObjectToWorld(phit),
	                           ObjectToWorld(dpdu),
							   ObjectToWorld(dpdv),
	                           ObjectToWorld(dndu),
							   ObjectToWorld(dndv),
	                           u, v, this);
	// Update _tHit_
	*tHit = thit;
	return true;
}

bool Torus::IntersectP(const Ray &r) const {
	float phi, theta;
	float thit;
	Point phit;

	// Transform _Ray_ to object space
	Ray ray;
	WorldToObject(r, &ray);

	return FindIntersection(ray, &thit, &phit, &phi, &theta);
}

float Torus::Area() const {
	return phiMax * majorRadius * (thetaMax - thetaMin) * minorRadius;
}

Shape* Torus::CreateShape(const Transform &o2w,
					   bool reverseOrientation,
					   const ParamSet &params) {
	float majorRadius = params.FindOneFloat("majorradius", 1.f);
	float minorRadius = params.FindOneFloat("minorradius", .25f);
	float thetamin = params.FindOneFloat("thetamin", 0);
	float thetamax = params.FindOneFloat("thetamax", 360.f);
	float phimax = params.FindOneFloat("phimax", 360.f);
	return new Torus(o2w, reverseOrientation, majorRadius, minorRadius,
		thetamin, thetamax, phimax);
}

static DynamicLoader::RegisterShape<Torus> r("torus");
