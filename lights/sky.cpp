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

// sky.cpp*
#include "sky.h"

// SkyLight Method Definitions
SkyLight::~SkyLight() {
}

SkyLight::SkyLight(const Transform &light2world,
		                const Spectrum &L, int ns, Vector sd, float turb, bool atm)
	: Light(light2world, ns) {
	Lbase = L;
	sundir = sd;
	turbidity = turb;

	V = 4.0; // Junge's exponent.

    InitSunThetaPhi();
    toSun = Vector(cos(phiS)*sin(thetaS), sin(phiS)*sin(thetaS), cos(thetaS));

    sunSpectralRad =  ComputeAttenuatedSunlight(thetaS, turbidity);
    sunSolidAngle =  0.25*M_PI*1.39*1.39/(150*150);  // = 6.7443e-05

    float theta2 = thetaS*thetaS;
    float theta3 = theta2*thetaS;
    float T = turb;
    float T2 = turb*turb;

    float chi = (4.0/9.0 - T / 120.0) * (M_PI - 2 * thetaS);
    zenith_Y = (4.0453 * T - 4.9710) * tan(chi) - .2155 * T + 2.4192;
    zenith_Y *= 1000;  // conversion from kcd/m^2 to cd/m^2
    
    zenith_x =
	(+0.00165*theta3 - 0.00374*theta2 + 0.00208*thetaS + 0)          * T2 +
	(-0.02902*theta3 + 0.06377*theta2 - 0.03202*thetaS + 0.00394) * T +
	(+0.11693*theta3 - 0.21196*theta2 + 0.06052*thetaS + 0.25885);

    zenith_y =
	(+0.00275*theta3 - 0.00610*theta2 + 0.00316*thetaS  + 0)          * T2 +
	(-0.04214*theta3 + 0.08970*theta2 - 0.04153*thetaS  + 0.00515) * T +
	(+0.15346*theta3 - 0.26756*theta2 + 0.06669*thetaS  + 0.26688);
      
    perez_Y[1] =    0.17872 *T  - 1.46303;
    perez_Y[2] =   -0.35540 *T  + 0.42749;
    perez_Y[3] =   -0.02266 *T  + 5.32505;
    perez_Y[4] =    0.12064 *T  - 2.57705;
    perez_Y[5] =   -0.06696 *T  + 0.37027;
      
    perez_x[1] =   -0.01925 *T  - 0.25922;
    perez_x[2] =   -0.06651 *T  + 0.00081;
    perez_x[3] =   -0.00041 *T  + 0.21247;
    perez_x[4] =   -0.06409 *T  - 0.89887;
    perez_x[5] =   -0.00325 *T  + 0.04517;
      
    perez_y[1] =   -0.01669 *T  - 0.26078;
    perez_y[2] =   -0.09495 *T  + 0.00921;
    perez_y[3] =   -0.00792 *T  + 0.21023;
    perez_y[4] =   -0.04405 *T  - 1.65369;
    perez_y[5] =   -0.01092 *T  + 0.05291;

    if(atm) {
		// Atmospheric Perspective initialization
		CreateConstants();	
		InitA0();
    }
    atmInited = atm;
	printf("skyzenithrad luminance: %f\n", GetSkySpectralRadiance(.0, .0).y() );
}
Spectrum
	SkyLight::Le(const RayDifferential &r) const {
	Vector w = r.d;
	// Compute sky light radiance for direction
	Spectrum L = Lbase;

	Vector wh = Normalize(WorldToLight(w));
	float phi = SphericalPhi(wh);
	float theta = SphericalTheta(wh);
	L *= GetSkySpectralRadiance(theta, phi);

	return L;
}
Spectrum SkyLight::Sample_L(const Point &p,
		const Normal &n, float u1, float u2,
		Vector *wi, float *pdf,
		VisibilityTester *visibility) const {
	// Sample cosine-weighted direction on unit sphere
	float x, y, z;
	ConcentricSampleDisk(u1, u2, &x, &y);
	z = sqrtf(max(0.f, 1.f - x*x - y*y));
	if (RandomFloat() < .5) z *= -1;
	*wi = Vector(x, y, z);
	// Compute _pdf_ for cosine-weighted infinite light direction
	*pdf = fabsf(wi->z) * INV_TWOPI;
	// Transform direction to world space
	Vector v1, v2;
	CoordinateSystem(Normalize(Vector(n)), &v1, &v2);
	*wi = Vector(v1.x * wi->x + v2.x * wi->y + n.x * wi->z,
	             v1.y * wi->x + v2.y * wi->y + n.y * wi->z,
	             v1.z * wi->x + v2.z * wi->y + n.z * wi->z);
	visibility->SetRay(p, *wi);
	return Le(RayDifferential(p, *wi));
}
float SkyLight::Pdf(const Point &, const Normal &n,
		const Vector &wi) const {
	return AbsDot(n, wi) * INV_TWOPI;
}
Spectrum SkyLight::Sample_L(const Point &p,
		float u1, float u2, Vector *wi, float *pdf,
		VisibilityTester *visibility) const {
	*wi = UniformSampleSphere(u1, u2);
	*pdf = UniformSpherePdf();
	visibility->SetRay(p, *wi);
	return Le(RayDifferential(p, *wi));
}
float SkyLight::Pdf(const Point &, const Vector &) const {
	return 1.f / (4.f * M_PI);
}
Spectrum SkyLight::Sample_L(const Scene *scene,
		float u1, float u2, float u3, float u4,
		Ray *ray, float *pdf) const {
	// Choose two points _p1_ and _p2_ on scene bounding sphere
	Point worldCenter;
	float worldRadius;
	scene->WorldBound().BoundingSphere(&worldCenter,
	                                    &worldRadius);
	worldRadius *= 1.01f;
	Point p1 = worldCenter + worldRadius *
		UniformSampleSphere(u1, u2);
	Point p2 = worldCenter + worldRadius *
		UniformSampleSphere(u3, u4);
	// Construct ray between _p1_ and _p2_
	ray->o = p1;
	ray->d = Normalize(p2-p1);
	// Compute _SkyLight_ ray weight
	Vector to_center = Normalize(worldCenter - p1);
	float costheta = AbsDot(to_center,ray->d);
	*pdf =
		costheta / ((4.f * M_PI * worldRadius * worldRadius));
	return Le(RayDifferential(ray->o, -ray->d));
}
Spectrum SkyLight::Sample_L(const Point &p,
		Vector *wi, VisibilityTester *visibility) const {
	float pdf;
	Spectrum L = Sample_L(p, RandomFloat(), RandomFloat(),
		wi, &pdf, visibility);
	if (pdf == 0.f) return Spectrum(0.f);
	return L / pdf;
}
Light* SkyLight::CreateLight(const Transform &light2world,
		const ParamSet &paramSet) {
	Spectrum L = paramSet.FindOneSpectrum("L", Spectrum(1.0));			// Base color (gain) must be 1.f
	int nSamples = paramSet.FindOneInt("nsamples", 1);
	Vector sundir = paramSet.FindOneVector("sundir", Vector(0,0,-1));	// direction vector of the sun
	Normalize(sundir);
	float turb = paramSet.FindOneFloat("turbidity", 2.0f);				// [in] turb  Turbidity (1.0,30+) 2-6 are most useful for clear days.
	bool initAtmEffects = true;											// [in] initAtmEffects  (must be)
	return new SkyLight(light2world, L, nSamples, sundir, turb, initAtmEffects);
}

/**********************************************************
// Atmospheric Perspective Constants
// 
// ********************************************************/

// density decreses as exp(-alpha*h) alpha in m^-1.
// _1 refers to Haze, _2 referes to molecules.

static float const Alpha_1 = 0.83331810215394e-3;
static float const Alpha_2 = 0.11360016092149e-3;

#define Nt 20 // Number of bins for theta
#define Np 20 // Number of bins for phi

#define radians(x) ((x)/180.0*M_PI)

static Spectrum AT0_1[Nt+1][Np+1];
static Spectrum AT0_2[Nt+1][Np+1];

inline Spectrum exp(const Spectrum &in)
{
    Spectrum out;
    for(int i = 0; i < COLOR_SAMPLES; i++)
      out.c[i] = exp(in.c[i]);
    return out;
}

/* All angles in radians, theta angles measured from normal */
inline float RiAngleBetween(float thetav, float phiv, float theta, float phi)
{
  float cospsi = sin(thetav) * sin(theta) * cos(phi-phiv) + cos(thetav) * cos(theta);
  if (cospsi > 1) return 0;
  if (cospsi < -1) return M_PI;
  return  acos(cospsi);
}

/**********************************************************
// South = x,  East = y, up = z
// All times in decimal form (6.25 = 6:15 AM)
// All angles in Radians
// ********************************************************/

void SkyLight::SunThetaPhi(float &stheta, float &sphi) const
{
    sphi = phiS;
    stheta = thetaS;
}

/**********************************************************
// South = x,  East = y, up = z
// All times in decimal form (6.25 = 6:15 AM)
// All angles in Radians
// From IES Lighting Handbook pg 361
// ********************************************************/

void SkyLight::InitSunThetaPhi()
{
	Vector wh = Normalize(sundir);
	phiS = SphericalPhi(wh);
	thetaS = SphericalTheta(wh);
}

Vector SkyLight::GetSunPosition() const
{
    return toSun;
}

/**********************************************************
// Solar Radiance
// 
// ********************************************************/

Spectrum SkyLight::GetSunSpectralRadiance() const
{
    return sunSpectralRad;
}

float SkyLight::GetSunSolidAngle() const
{
    return sunSolidAngle;
}

/**********************************************************
// Sky Radiance
// 
// ********************************************************/

Spectrum SkyLight::GetSkySpectralRadiance(const Vector &varg) const // Don't use for now - radiance
{
    float theta, phi;
    Vector v = varg;
    if (v.z < 0) return Spectrum(0.);
    if (v.z < 0.001) 
      v = Vector(v.x,v.y,.001 );
    
    theta = acos(v.z);
    if (fabs(theta)< 1e-5) phi = 0;
    else  phi = atan2(v.y,v.x);
    return GetSkySpectralRadiance(theta,phi);
}

inline float SkyLight::PerezFunction(const float *lam, float theta, float gamma, float lvz) const
{
   float den = ((1 + lam[1]*exp(lam[2])) *
		 (1 + lam[3]*exp(lam[4]*thetaS) + lam[5]*cos(thetaS)*cos(thetaS)));
                
   float num = ((1 + lam[1]*exp(lam[2]/cos(theta))) *
		 (1 + lam[3]*exp(lam[4]*gamma)  + lam[5]*cos(gamma)*cos(gamma)));

   return lvz* num/den;
}

Spectrum SkyLight::GetSkySpectralRadiance(float theta, float phi) const
{
	// add bottom half of hemisphere with horizon colour
	if( theta > (M_PI/2)-0.001 ) theta = M_PI/2-0.001;

    float gamma = RiAngleBetween(theta,phi,thetaS,phiS);
	// Compute xyY values
    float x = PerezFunction(perez_x, theta, gamma, zenith_x);
    float y = PerezFunction(perez_y, theta, gamma, zenith_y);
    float Y = PerezFunction(perez_Y, theta, gamma, zenith_Y);

    Spectrum spect = ChromaticityToSpectrum(x,y);
	Spectrum o = Y * spect / spect.y();

	// TODO - radiance - cleanup
	if (o.IsNaN()) {
		o = Spectrum(0.f);
	}
	else if (o.y() < -1e-5) {
		o = Spectrum(0.f);
	}
	else if (isinf(o.y())) {
		o = Spectrum(0.f);
	} 

    return o;
}


/**********************************************************
// Atmospheric perspective functions
// 
// ********************************************************/


/**********************************************************
// Initialization
// 
// ********************************************************/

void SkyLight::CalculateA0(float thetav, float phiv, Spectrum &A0_1, Spectrum &A0_2) const
{
    float psi;
    Spectrum skyRad;
    Spectrum beta_ang_1, beta_ang_2;
    float theta, phi, phiDelta = M_PI/20;
    float thetaDelta = M_PI/2/20;
    float thetaUpper;
  
    Spectrum skyAmb_1 = 0.;
    Spectrum skyAmb_2 = 0.;
  
    thetaUpper = M_PI / 2;

    for (theta = 0; theta < thetaUpper; theta += thetaDelta)
      for (phi = 0; phi < 2 * M_PI; phi += phiDelta)  {
	  skyRad = GetSkySpectralRadiance(theta, phi);
	  psi = RiAngleBetween(thetav, phiv, theta, phi);
	  
	  beta_ang_1 = beta_p_ang_prefix * GetNeta(psi, V);
	  beta_ang_2 = beta_m_ang_prefix * (1+0.9324*cos(psi)*cos(psi));
	  
	  skyAmb_1 += skyRad * beta_ang_1 * sin(theta) * thetaDelta * phiDelta;
	  skyAmb_2 += skyRad * beta_ang_2 * sin(theta) * thetaDelta * phiDelta;
      }
  
    /* Sun's ambience term*/
  
    psi = RiAngleBetween(thetav, phiv, thetaS, phiS);

    beta_ang_1 = beta_p_ang_prefix * GetNeta(psi, V);
    beta_ang_2 = beta_m_ang_prefix * (1+0.9324*cos(psi)*cos(psi));
  
    Spectrum sunAmb_1 = sunSpectralRad * beta_ang_1 * sunSolidAngle;
    Spectrum sunAmb_2 = sunSpectralRad * beta_ang_2 * sunSolidAngle;
    
	// Sum of sun and sky  (should probably add a ground ambient)
    A0_1 =  (sunAmb_1 + skyAmb_1);
    A0_2 =  (sunAmb_2 + skyAmb_2);
}

void  SkyLight::InitA0() const
{
    int i, j;
    float theta, phi;
  
    float delTheta = M_PI/Nt;
    float delPhi = 2*M_PI/Np;
  
    //cerr << "ggSunSky::Preprocessing: 0%\r";
    for (i=0,theta = 0; theta<=  M_PI; theta+= delTheta,i++) {
	for (j=0,phi=0; phi <= 2*M_PI; phi+= delPhi,j++)
	  CalculateA0(theta, phi,  AT0_1[i][j], AT0_2[i][j]);
	//cerr << "ggSunSky::Preprocessing: " << 100*(i*Np+j)/((Nt+1)*Np) <<"%  \r";
    }
    //cerr << "ggSunSky::Preprocessing:  100%   " << endl;
}      


/**********************************************************
// Evaluation
// 
// ********************************************************/

void SkyLight::GetAtmosphericEffects(const Vector &viewer, const Vector &source,
				     Spectrum &attenuation, Spectrum &inscatter ) const
{
    assert(atmInited);
				// Clean up the 1000 problem
    float h0 = viewer.z+1000;//1000 added to make sure ray doesnt
				//go below zero. 
    Vector direction = source - viewer; // was RiUnitVector3 TODO fix
    float thetav = acos(direction.z);
    float phiv = atan2(direction.y,direction.x);
    float s = (viewer - source).Length();


    // This should be changed so that we don't need to worry about it.
    if(h0+s*cos(thetav) <= 0)
    {
	attenuation = 1;
	inscatter = 0.;
	//cerr << "\nWarning: Ray going below earth's surface \n";
	return;
    }

    attenuation = AttenuationFactor(h0, thetav, s);
    inscatter   = InscatteredRadiance(h0, thetav, phiv, s);
}

inline float EvalFunc(float B, float x)
{
    if (fabs(B*x)< 0.01) return x;
    return (1-exp(-B*x))/B;
}

Spectrum SkyLight::AttenuationFactor(float h0, float theta, float s) const
{
    float costheta = cos(theta);
    float B_1 = Alpha_1 * costheta;
    float B_2 = Alpha_2 * costheta;
    float constTerm_1 = exp(-Alpha_1*h0) * EvalFunc(B_1, s);
    float constTerm_2 = exp(-Alpha_2*h0) * EvalFunc(B_2, s);
  
    return (exp(-beta_p * constTerm_1) *
	    exp(-beta_m * constTerm_2));
}

static void GetA0fromTable(float theta, float phi, Spectrum &A0_1, Spectrum &A0_2)
{
  float eps = 1e-4;
  if (phi < 0) phi += 2*M_PI; // convert phi from -pi..pi to 0..2pi
  theta = theta*Nt/M_PI - eps;
  phi = phi*Np/(2*M_PI) - eps;
  if (theta < 0) theta = 0;
  if (phi < 0) phi = 0;
  int i = (int) theta;
  float u = theta - i;
  int j = (int)phi;
  float v = phi - j;

  A0_1 = (1-u)*(1-v)*AT0_1[i][j] + u*(1-v)*AT0_1[i+1][j]
    + (1-u)*v*AT0_1[i][j+1] + u*v*AT0_1[i+1][j+1];
  A0_2 = (1-u)*(1-v)*AT0_2[i][j] + u*(1-v)*AT0_2[i+1][j]
    + (1-u)*v*AT0_2[i][j+1] + u*v*AT0_2[i+1][j+1];
}

inline float RiHelper1(float A, float B, float C, float D,
			float H, float K, float u)
{
    float t = exp(-K*(H-u));
    return (t/K)*((A*u*u*u + B*u*u + C*u +D) -
		  (3*A*u*u + 2*B*u + C)/K +
		  (6*A*u + 2*B)/(K*K) -
		  (6*A)/(K*K*K));
}

inline void RiCalculateABCD(float a, float b, float c, float d, float e,
			    float den, float &A, float &B, float &C, float &D)
{
    A = (-b*d -2 + 2*c + a*e - b*e + a*d)/den;
    B = -(2*a*a*e + a*a*d - 3*a - a*b*e +
	  3*a*c + a*b*d - 2*b*b*d - 3*b - b*b*e + 3*b*c)/den;
    C =( -b*b*b*d - 2*b*b*a*e -   b*b*a*d + a*a*b*e +
	2*a*a*b*d - 6*a*b     + 6*b*a*c   + a*a*a*e)/den;
    D = -(   b*b*b - b*b*b*a*d - b*b*a*a*e + b*b*a*a*d
	  -3*a*b*b + b*e*a*a*a - c*a*a*a + 3*c*b*a*a)/den;
}

Spectrum SkyLight::InscatteredRadiance(float h0, float theta, float phi, float s) const
{
    float costheta = cos(theta);
    float B_1 = Alpha_1*costheta;
    float B_2 = Alpha_2*costheta;
    Spectrum A0_1; 
    Spectrum A0_2;
    Spectrum result;
    int i;
    Spectrum I_1, I_2;
  
    GetA0fromTable(theta, phi, A0_1, A0_2);

    // approximation (< 0.3 for 1% accuracy)
    if (fabs(B_1*s) < 0.3) {
	float constTerm1 =  exp(-Alpha_1*h0);
	float constTerm2 =  exp(-Alpha_2*h0);

	Spectrum C_1 = beta_p * constTerm1;
	Spectrum C_2 = beta_m * constTerm2;
	    
	for(int i = 0; i < COLOR_SAMPLES; i++) {
	    I_1.c[i] =  (1-exp(-(B_1 + C_1.c[i] + C_2.c[i]) * s)) / (B_1 + C_1.c[i] + C_2.c[i]);
	    I_2.c[i] =  (1-exp(-(B_2 + C_1.c[i] + C_2.c[i]) * s)) / (B_2 + C_1.c[i] + C_2.c[i]);
	}

	return A0_1 * constTerm1 * I_1 + A0_2 * constTerm2 * I_2;
    }

    // Analytical approximation
    float A,B,C,D,H1,H2,K;
    float u_f1, u_i1,u_f2, u_i2, int_f, int_i, fs, fdashs, fdash0;
    float a1,b1,a2,b2;
    float den1, den2;

    b1 = u_f1 = exp(-Alpha_1*(h0+s*costheta));
    H1 = a1 = u_i1 = exp(-Alpha_1*h0);
    b2 = u_f2 = exp(-Alpha_2*(h0+s*costheta));
    H2 = a2 = u_i2 = exp(-Alpha_2*h0);
    den1 = (a1-b1)*(a1-b1)*(a1-b1);
    den2 = (a2-b2)*(a2-b2)*(a2-b2);
    
    for (i = 0; i < COLOR_SAMPLES; i++) {
	// for integral 1
	K = beta_p.c[i]/(Alpha_1*costheta);
	fdash0 = -beta_m.c[i]*H2;
	fs = exp(-beta_m.c[i]/(Alpha_2*costheta)*(u_i2 - u_f2));
	fdashs = -fs*beta_m.c[i]*u_f2;

	RiCalculateABCD(a1,b1,fs,fdash0,fdashs,den1,A,B,C,D);
	int_f = RiHelper1(A,B,C,D,H1,K, u_f1);
	int_i = RiHelper1(A,B,C,D,H1,K, u_i1);
	I_1.c[i] = (int_f - int_i)/(-Alpha_1*costheta);

	// for integral 2
	K = beta_m.c[i]/(Alpha_2*costheta);
	fdash0 = -beta_p.c[i]*H1;
	fs = exp(-beta_p.c[i]/(Alpha_1*costheta)*(u_i1 - u_f1));
	fdashs = -fs*beta_p.c[i]*u_f1;

	RiCalculateABCD(a2,b2,fs,fdash0, fdashs, den2, A,B,C,D);
	int_f = RiHelper1(A,B,C,D,H2,K, u_f2);
	int_i = RiHelper1(A,B,C,D,H2,K, u_i2);
	I_2.c[i] = (int_f - int_i)/(-Alpha_2*costheta);

    }
    return A0_1 * I_1 + A0_2 * I_2;
}

// -------------------------------------------------------------------------------------------------------------

//300-830 10nm
static float S0Amplitudes[54] = {
0.04,6.0,29.6,55.3,57.3,
61.8,61.5,68.8,63.4,65.8,
94.8,104.8,105.9,96.8,113.9,
125.6,125.5,121.3,121.3,113.5,
113.1,110.8,106.5,108.8,105.3,
104.4,100.0,96.0,95.1,89.1,
90.5,90.3,88.4,84.0,85.1,
81.9,82.6,84.9,81.3,71.9,
74.3,76.4,63.3,71.7,77.0,
65.2,47.7,68.6,65.0,66.0,
61.0,53.3,58.9,61.9
};

static float S1Amplitudes[54] = {
0.02,4.5,22.4,42.0,40.6,
41.6,38.0,42.4,38.5,35.0,
43.4,46.3,43.9,37.1,36.7,
35.9,32.6,27.9,24.3,20.1,
16.2,13.2,8.6,6.1,4.2,
1.9,0.0,-1.6,-3.5,-3.5,
-5.8,-7.2,-8.6,-9.5,-10.9,
-10.7,-12.0,-14.0,-13.6,-12.0,
-13.3,-12.9,-10.6,-11.6,-12.2,
-10.2,-7.8,-11.2,-10.4,-10.6,
-9.7,-8.3,-9.3,-9.8
};

static float S2Amplitudes[54] = {
0.0,2.0,4.0,8.5,7.8,
6.7,5.3,6.1,3.0,1.2,
-1.1,-0.5,-0.7,-1.2,-2.6,
-2.9,-2.8,-2.6,-2.6,-1.8,
-1.5,-1.3,-1.2,-1.0,-0.5,
-0.3,0.0,0.2,0.5,2.1,
3.2,4.1,4.7,5.1,6.7,
7.3,8.6,9.8,10.2,8.3,
9.6,8.5,7.0,7.6,8.0,
6.7,5.2,7.4,6.8,7.0,
6.4,5.5,6.1,6.5
};

static Spectrum riS0Spectrum, riS1Spectrum, riS2Spectrum;

void InitChromaticityToSpectrum()
{
	RegularSpectrum riS0(S0Amplitudes,300,830,54);
	RegularSpectrum riS1(S1Amplitudes,300,830,54);
	RegularSpectrum riS2(S2Amplitudes,300,830,54);

    riS0Spectrum = riS0.toSpectrum();
    riS1Spectrum = riS1.toSpectrum();
    riS2Spectrum = riS2.toSpectrum();
}

Spectrum SkyLight::ChromaticityToSpectrum(float x, float y) const
{
    static bool inited = false;
    if(!inited) {
	inited = true;
	InitChromaticityToSpectrum();
    }
    
    float M1 = (-1.3515 - 1.7703*x +  5.9114*y)/(0.0241 + 0.2562*x - 0.7341*y);
    float M2 = ( 0.03   -31.4424*x + 30.0717*y)/(0.0241 + 0.2562*x - 0.7341*y);

    return riS0Spectrum + M1 * riS1Spectrum + M2 * riS2Spectrum;   
}

// -------------------------------------------------------------------------------------------------------------

/* All data lifted from MI */
/* Units are either [] or cm^-1. refer when in doubt MI */

// k_o Spectrum table from pg 127, MI.
static float k_oWavelengths[64] = {
300, 305, 310, 315, 320,
325, 330, 335, 340, 345,
350, 355,

445, 450, 455, 460, 465,
470, 475, 480, 485, 490,
495,

500, 505, 510, 515, 520,
525, 530, 535, 540, 545,
550, 555, 560, 565, 570,
575, 580, 585, 590, 595,

600, 605, 610, 620, 630,
640, 650, 660, 670, 680,
690,

700, 710, 720, 730, 740,
750, 760, 770, 780, 790,
};

static float k_oAmplitudes[65] = {
  10.0,
  4.8,
  2.7,
  1.35,
  .8,
  .380,
  .160,
  .075,
  .04,
  .019,
  .007,
  .0,
  
  .003,
  .003,
  .004,
  .006,
  .008,
  .009,
  .012,
  .014,
  .017,
  .021,
  .025,

  .03,
  .035,
  .04,
  .045,
  .048,
  .057,
  .063,
  .07,
  .075,
  .08,
  .085,
  .095,
  .103,
  .110,
  .12,
  .122,
  .12,
  .118,
  .115,
  .12,

  .125,
  .130,
  .12,
  .105,
  .09,
  .079,
  .067,
  .057,
  .048,
  .036,
  .028,
  
  .023,
  .018,
  .014,
  .011,
  .010,
  .009,
  .007,
  .004,
  .0,
  .0
};


// k_g Spectrum table from pg 130, MI.
static float k_gWavelengths[4] = {
  759,
  760,
  770,
  771
};

static float k_gAmplitudes[4] = {
  0,
  3.0,
  0.210,
  0
};

// k_wa Spectrum table from pg 130, MI.
static float k_waWavelengths[13] = {
  689,
  690,
  700,
  710,
  720,
  730,
  740,
  750,
  760,
  770,
  780,
  790,
  800
};

static float k_waAmplitudes[13] = {
  0,
  0.160e-1,
  0.240e-1,
  0.125e-1,
  0.100e+1,
  0.870,
  0.610e-1,
  0.100e-2,
  0.100e-4,
  0.100e-4,
  0.600e-3,
  0.175e-1,
  0.360e-1
};


// 380-750 by 10nm
static float solAmplitudes[38] = {
    165.5, 162.3, 211.2, 258.8, 258.2,
    242.3, 267.6, 296.6, 305.4, 300.6,
    306.6, 288.3, 287.1, 278.2, 271.0,
    272.3, 263.6, 255.0, 250.6, 253.1,
    253.5, 251.3, 246.3, 241.7, 236.8,
    232.1, 228.2, 223.4, 219.7, 215.3,
    211.0, 207.3, 202.4, 198.7, 194.3,
    190.7, 186.3, 182.6
};


/**********************************************************
// Sunlight Transmittance Functions
// 
// ********************************************************/

/* Most units not in SI system - For units, refer MI */
Spectrum SkyLight::ComputeAttenuatedSunlight(float theta, float turbidity)
{
    IrregularSpectrum k_oCurve(k_oAmplitudes, k_oWavelengths, 65);
    IrregularSpectrum k_gCurve(k_gAmplitudes, k_gWavelengths, 4);
    IrregularSpectrum k_waCurve(k_waAmplitudes, k_waWavelengths, 13);
    RegularSpectrum   solCurve(solAmplitudes, 380, 750, 38);  // every 10 nm  IN WRONG UNITS
				                                     // Need a factor of 100 (done below)
    float  data[91];  // (800 - 350) / 5  + 1

    float beta = 0.04608365822050 * turbidity - 0.04586025928522;
    float tauR, tauA, tauO, tauG, tauWA;

    float m = 1.0/(cos(theta) + 0.15*pow(93.885-theta/M_PI*180.0,-1.253));  // Relative Optical Mass
				// equivalent  
//    RiReal m = 1.0/(cos(theta) + 0.000940 * pow(1.6386 - theta,-1.253));  // Relative Optical Mass

    int i;
    float lambda;
    for(i = 0, lambda = 350; i < 91; i++, lambda+=5) {
				// Rayleigh Scattering
				// Results agree with the graph (pg 115, MI) */
		tauR = exp( -m * 0.008735 * pow(lambda/1000, float(-4.08)));

				// Aerosal (water + dust) attenuation
				// beta - amount of aerosols present 
				// alpha - ratio of small to large particle sizes. (0:4,usually 1.3)
				// Results agree with the graph (pg 121, MI) 
	const float alpha = 1.3;
	tauA = exp(-m * beta * pow(lambda/1000, -alpha));  // lambda should be in um

				// Attenuation due to ozone absorption  
				// lOzone - amount of ozone in cm(NTP) 
				// Results agree with the graph (pg 128, MI) 
	const float lOzone = .35;
	tauO = exp(-m * k_oCurve.sample(lambda) * lOzone);

				// Attenuation due to mixed gases absorption  
				// Results agree with the graph (pg 131, MI)
	tauG = exp(-1.41 * k_gCurve.sample(lambda) * m / pow(1 + 118.93 * k_gCurve.sample(lambda) * m, 0.45));

				// Attenuation due to water vapor absorbtion  
				// w - precipitable water vapor in centimeters (standard = 2) 
				// Results agree with the graph (pg 132, MI)
	const float w = 2.0;
	tauWA = exp(-0.2385 * k_waCurve.sample(lambda) * w * m /
		    pow(1 + 20.07 * k_waCurve.sample(lambda) * w * m, 0.45));

	data[i] = 100 * solCurve.sample(lambda) * tauR * tauA * tauO * tauG * tauWA;  // 100 comes from solCurve being
																				// in wrong units. 
    }
    RegularSpectrum oSC(data, 350,800,91);
    return oSC.toSpectrum();  // Converts to Spectrum
}

// -------------------------------------------------------------------------------------------------------------

// 300-800 at 10 nm 
static float n2_1Amplitudes[51] = {
  3.4012e-7, 3.3702e-7, 3.3425e-7, 3.3177e-7, 3.2955e-7,
  3.2754e-7, 3.2572e-7, 3.2406e-7, 3.2255e-7, 3.2116e-7,
  
  3.1989e-7, 3.1873e-7, 3.1765e-7, 3.1665e-7, 3.1572e-7,
  3.1486e-7, 3.1407e-7, 3.1332e-7, 3.1263e-7, 3.1198e-7,
  
  3.1137e-7, 3.1080e-7, 3.1026e-7, 3.0976e-7, 3.0928e-7,
  3.0884e-7, 3.0841e-7, 3.0801e-7, 3.0763e-7, 3.0728e-7,
  
  3.0694e-7, 3.0661e-7, 3.0631e-7, 3.0602e-7, 3.0574e-7,
  3.0547e-7, 3.0522e-7, 3.0498e-7, 3.0475e-7, 3.0453e-7,
  
  3.0432e-7, 3.0412e-7, 3.0393e-7, 3.0375e-7, 3.0357e-7,
  3.0340e-7, 3.0324e-7, 3.0309e-7, 3.0293e-7, 3.0279e-7,
  
  3.0265e-7
};

// 300-800 at 1nm
static float KAmplitudes[501] = {
#include "sunsky/K.spectrum"
};

static float K25Amplitudes[501] = {
#include "sunsky/K25.spectrum"
};

//300- 790 at 10nm  (for each of 1801 tenths of a degree
static float netaLambdaTable[50][1801] = {
#include "sunsky/Neta.Table"
};

static Spectrum netaTable[1801];

static float const Anis_cor = 1.06;  // Correction for molecular anisotropy.
static float const N = 2.545e25; // Number of molecules per unit volume.

void InitNetaTable()
{
    float data[50];
    for(int i = 0; i < 1801; i++) {
	for(int j = 0; j < 50; j++) 
	  data[j] = netaLambdaTable[j][i];

	RegularSpectrum oSC(data,300,790,50);
	netaTable[i] = oSC.toSpectrum();
    }
}
	    
/* might need to be converted because the above functions use a
   different system ... */
void  SkyLight::CreateConstants()
{
    InitNetaTable();
    
    float lambda;
    float b_m[101], b_p[101];
    float b_m_ang_prefix[101];
    float b_p_ang_prefix[101];

    int i;
    float lambdasi, Nlambda4;
    float n2_1;
    float K;
    float c = (0.06544204545455*turbidity -0.06509886363636)*1e-15;
    RegularSpectrum KCurve(KAmplitudes, 300,800,501);
    RegularSpectrum K25Curve(K25Amplitudes, 300,800,501);
    RegularSpectrum n2_1Curve(n2_1Amplitudes, 300,800,51);
  
    for (lambda = 300,i=0; lambda <= 800; lambda+=5,i++)
    {
	lambdasi = lambda*1e-9;  /* Converstion to SI units */
    
	Nlambda4 = N*lambdasi*lambdasi*lambdasi*lambdasi;
                        
	/* Rayleigh total scattering coefficient */
	/* (4-48), p 199, MC */
	n2_1 = n2_1Curve.sample(lambda); 
	b_m[i] = 8*M_PI*M_PI*M_PI*n2_1*Anis_cor/
	    (3*Nlambda4);

    
	/* Mie total scattering coefficient */
	/* (6-30). p 280; MC */
	if (V > 3.9) K = KCurve.sample(lambda); // V = 4.0;
	else K = K25Curve.sample(lambda);       // V = 2.5

	b_p[i] = 0.434*M_PI*c*pow(float(2*M_PI/lambdasi),V-2) * K *
	    pow(float(0.01),V-3);  /* Last term is unit correction for c */
    
	/* Rayleigh Angular scattering coefficient */
	/*  (4-55), p 200; MC */
	/* Needs to be multiplied by (1+0.9324cos^2(theta)) to get exact
	   angular scattering constant */
	b_m_ang_prefix[i] = 2*M_PI*M_PI*n2_1*Anis_cor*0.7629/
	    (3*Nlambda4);
    

	/* Mie Angular scattering coefficient */
	/* (6-24), p 271; MC */
	/* Needs to be multiplied by neta(theta,lambda) to get exact
	   angular scattering constant term */
	b_p_ang_prefix[i] = 0.217*c*pow(float(2*M_PI/lambdasi),V-2)*
	    pow(float(0.01),V-3);  /* Last term is unit correction for c */
    }
  
	RegularSpectrum Bm(b_m, 300,800,101);
	RegularSpectrum Bp(b_p, 300,800,101);
	RegularSpectrum Bma(b_m_ang_prefix, 300,800,101);
	RegularSpectrum Bmp(b_p_ang_prefix, 300,800,101);

    beta_m = Bm.toSpectrum();
    beta_p = Bp.toSpectrum();
    beta_m_ang_prefix = Bma.toSpectrum();
    beta_p_ang_prefix = Bmp.toSpectrum();
}

Spectrum SkyLight::GetNeta(float theta, float v) const
{
    theta = theta*180.0/M_PI * 10;
    float u = theta - (int)theta;
    if((theta < 0)||(theta > 1801)) {
	//cerr << "Error:getNeta() [theta outside range] theta=" << theta/10
	//     <<"degrees" << endl;
	return 0.;
    }
    if (theta > 1800) theta=1800;

    if ((int)theta==1800)
      return  netaTable[1800];
   
    return (1-u) * netaTable[(int)theta] + u * netaTable[(int)theta+1];

}
