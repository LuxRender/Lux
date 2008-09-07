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

#ifndef LUX_RGBColor_H
#define LUX_RGBColor_H
// spectrum.h*
#include "lux.h"
#include "spd.h"
#include "color.h"

namespace lux
{

#define WAVELENGTH_SAMPLES 8
#define WAVELENGTH_START 380.
#define WAVELENGTH_END   720.
static const float inv_WAVELENGTH_SAMPLES = 1.f / WAVELENGTH_SAMPLES;


/*
// RGBColor Declarations
#ifndef DISABLED_LUX_USE_SSE

class  RGBColor {
	friend class boost::serialization::access;
public:
	// RGBColor Public Methods
	RGBColor(float v = 0.f) {
		for (int i = 0; i < COLOR_SAMPLES; ++i)
			c[i] = v;
	}
	
	RGBColor(const float cs[COLOR_SAMPLES]) {
		for (int i = 0; i < COLOR_SAMPLES; ++i)
			c[i] = cs[i];
	}
	friend ostream &operator<<(ostream &, const RGBColor &);
	RGBColor &operator+=(const RGBColor &s2) {
		for (int i = 0; i < COLOR_SAMPLES; ++i)
			c[i] += s2.c[i];
		return *this;
	}
	RGBColor &operator-=(const RGBColor &s2) {
		for (int i = 0; i < COLOR_SAMPLES; ++i)
			c[i] -= s2.c[i];
		return *this;
	}
	RGBColor operator+(const RGBColor &s2) const {
		RGBColor ret = *this;
		for (int i = 0; i < COLOR_SAMPLES; ++i)
			ret.c[i] += s2.c[i];
		return ret;
	}
	RGBColor operator-(const RGBColor &s2) const {
		RGBColor ret = *this;
		for (int i = 0; i < COLOR_SAMPLES; ++i)
			ret.c[i] -= s2.c[i];
		return ret;
	}
	RGBColor operator/(const RGBColor &s2) const {
		RGBColor ret = *this;
		for (int i = 0; i < COLOR_SAMPLES; ++i)
			ret.c[i] /= s2.c[i];
		return ret;
	}
	RGBColor operator*(const RGBColor &sp) const {
		RGBColor ret = *this;
		for (int i = 0; i < COLOR_SAMPLES; ++i)
			ret.c[i] *= sp.c[i];
		return ret;
	}
	RGBColor &operator*=(const RGBColor &sp) {
		for (int i = 0; i < COLOR_SAMPLES; ++i)
			c[i] *= sp.c[i];
		return *this;
	}
	RGBColor operator*(float a) const {
		RGBColor ret = *this;
		for (int i = 0; i < COLOR_SAMPLES; ++i)
			ret.c[i] *= a;
		return ret;
	}
	RGBColor &operator*=(float a) {
		for (int i = 0; i < COLOR_SAMPLES; ++i)
			c[i] *= a;
		return *this;
	}
	friend inline
	RGBColor operator*(float a, const RGBColor &s) {
		return s * a;
	}
	RGBColor operator/(float a) const {
		return *this * (1.f / a);
	}
	RGBColor &operator/=(float a) {
		float inv = 1.f / a;
		for (int i = 0; i < COLOR_SAMPLES; ++i)
			c[i] *= inv;
		return *this;
	}
	void AddWeighted(float w, const RGBColor &s) {
		for (int i = 0; i < COLOR_SAMPLES; ++i)
			c[i] += w * s.c[i];
	}
	bool operator==(const RGBColor &sp) const {
		for (int i = 0; i < COLOR_SAMPLES; ++i)
			if (c[i] != sp.c[i]) return false;
		return true;
	}
	bool operator!=(const RGBColor &sp) const {
		return !(*this == sp);
	}
	bool Black() const {
		for (int i = 0; i < COLOR_SAMPLES; ++i)
			if (c[i] != 0.) return false;
		return true;
	}
	RGBColor Sqrt() const {
		RGBColor ret;
		for (int i = 0; i < COLOR_SAMPLES; ++i)
			ret.c[i] = sqrtf(c[i]);
		return ret;
	}
	RGBColor Pow(const RGBColor &e) const {
		RGBColor ret;
		for (int i = 0; i < COLOR_SAMPLES; ++i)
			ret.c[i] = c[i] > 0 ? powf(c[i], e.c[i]) : 0.f;
		return ret;
	}
	RGBColor Pow(const float e) const {
		RGBColor ret;
		for (int i = 0; i < COLOR_SAMPLES; ++i)
			ret.c[i] = c[i] > 0 ? powf(c[i], e) : 0.f;
		return ret;
	}
	RGBColor operator-() const {
		RGBColor ret;
		for (int i = 0; i < COLOR_SAMPLES; ++i)
			ret.c[i] = -c[i];
		return ret;
	}
	friend RGBColor Exp(const RGBColor &s) {
		RGBColor ret;
		for (int i = 0; i < COLOR_SAMPLES; ++i)
			ret.c[i] = expf(s.c[i]);
		return ret;
	}
	RGBColor Clamp(float low = 0.f,
	               float high = INFINITY) const {
		RGBColor ret;
		for (int i = 0; i < COLOR_SAMPLES; ++i)
			ret.c[i] = ::Clamp(c[i], low, high);
		return ret;
	}
	bool IsNaN() const {
		for (int i = 0; i < COLOR_SAMPLES; ++i)
			if (isnan(c[i])) return true;
		return false;
	}
	void Print(FILE *f) const {
		for (int i = 0; i < COLOR_SAMPLES; ++i)
			fprintf(f, "%f ", c[i]);
	}
	XYZColor ToXYZ() const {
		float xyz[3];
		xyz[0] = xyz[1] = xyz[2] = 0.;
		for (int i = 0; i < COLOR_SAMPLES; ++i) {
			xyz[0] += XWeight[i] * c[i];
			xyz[1] += YWeight[i] * c[i];
			xyz[2] += ZWeight[i] * c[i];
		}
		return XYZColor(xyz);
	}
	float y() const {
		float v = 0.;
		for (int i = 0; i < COLOR_SAMPLES; ++i)
			v += YWeight[i] * c[i];
		return v;
	}
	bool operator<(const RGBColor &s2) const {
		return y() < s2.y();
	}
	friend class lux::ParamSet;
	
	// RGBColor Public Data
	float c[COLOR_SAMPLES];
	
protected:
	// RGBColor Private Data
	static float XWeight[COLOR_SAMPLES];
	static float YWeight[COLOR_SAMPLES];
	static float ZWeight[COLOR_SAMPLES];
	friend RGBColor FromXYZ(float x, float y, float z);
	
private:
	template<class Archive>
			void serialize(Archive & ar, const unsigned int version)
			{
				for (int i = 0; i < COLOR_SAMPLES; ++i)
					ar & c[i];
			}
};

#else //LUX_USE_SSE

#define COLOR_VECTORS 1

class  _MM_ALIGN16 RGBColor {
public:
	// RGBColor Public Methods
	RGBColor(float v = 0.f) {
		for (int i = 0; i < COLOR_SAMPLES; ++i)
			c[i] = v;
	}
	RGBColor(float cs[COLOR_SAMPLES]) {
		for (int i = 0; i < COLOR_SAMPLES; ++i)
			c[i] = cs[i];
	}
	RGBColor(__m128 cv[COLOR_VECTORS])
    {
    	for (int i = 0; i < COLOR_VECTORS; ++i)
    		cvec[i]=cv[i];
    }
	
	friend ostream &operator<<(ostream &, const RGBColor &);
	RGBColor &operator+=(const RGBColor &s2) {
		for (int i = 0; i < COLOR_SAMPLES; ++i)
			c[i] += s2.c[i];
		return *this;
	}
	RGBColor operator+(const RGBColor &s2) const {
		RGBColor ret;// = *this;
		for (int i = 0; i < COLOR_VECTORS; ++i)
			ret.cvec[i]=_mm_add_ps(cvec[i],s2.cvec[i]);
		
		//for (int i = 0; i < COLOR_SAMPLES; ++i)
		//	ret.c[i] += s2.c[i];
		return ret;
	}
	RGBColor operator-(const RGBColor &s2) const {
		RGBColor ret = *this;
		for (int i = 0; i < COLOR_SAMPLES; ++i)
			ret.c[i] -= s2.c[i];
		return ret;
	}
	RGBColor operator/(const RGBColor &s2) const {
		RGBColor ret;// = *this;
		for (int i = 0; i < COLOR_VECTORS; ++i)
			ret.cvec[i]=_mm_div_ps(cvec[i],s2.cvec[i]);
		//for (int i = 0; i < COLOR_SAMPLES; ++i)
		//	ret.c[i] /= s2.c[i];
		return ret;
	}
	RGBColor operator*(const RGBColor &sp) const {
		RGBColor ret;// = *this;
		for (int i = 0; i < COLOR_VECTORS; ++i)
			ret.cvec[i]=_mm_mul_ps(cvec[i],sp.cvec[i]);
		
		//for (int i = 0; i < COLOR_SAMPLES; ++i)
		//	ret.c[i] *= sp.c[i];
		return ret;
	}
	RGBColor &operator*=(const RGBColor &sp) {
		//for (int i = 0; i < COLOR_SAMPLES; ++i)
		//	c[i] *= sp.c[i];
		
		for (int i = 0; i < COLOR_VECTORS; ++i)
			cvec[i]=_mm_mul_ps(cvec[i],sp.cvec[i]);
		
		return *this;
	}
	RGBColor operator*(float a) const {
		RGBColor ret = *this;
		for (int i = 0; i < COLOR_SAMPLES; ++i)
			ret.c[i] *= a;
		return ret;
	}
	RGBColor &operator*=(float a) {
		for (int i = 0; i < COLOR_SAMPLES; ++i)
			c[i] *= a;
		return *this;
	}
	friend inline
	RGBColor operator*(float a, const RGBColor &s) {
		return s * a;
	}
	RGBColor operator/(float a) const {
		return *this * (1.f / a);
	}
	RGBColor &operator/=(float a) {
		float inv = 1.f / a;
		for (int i = 0; i < COLOR_SAMPLES; ++i)
			c[i] *= inv;
		return *this;
	}
	void AddWeighted(float w, const RGBColor &s) {
		//for (int i = 0; i < COLOR_SAMPLES; ++i)
		//	c[i] += w * s.c[i];
		__m128 wvec=_mm_set_ps1(w);
		
		for (int i = 0; i < COLOR_VECTORS; ++i)
			cvec[i]=_mm_add_ps(cvec[i],_mm_mul_ps(s.cvec[i],wvec));
	}
	bool operator==(const RGBColor &sp) const {
		for (int i = 0; i < COLOR_SAMPLES; ++i)
			if (c[i] != sp.c[i]) return false;
		
		//for (int i = 0; i < COLOR_VECTORS; ++i)	
		//	if(_mm_cmpneq_ps(cvec[i],sp.cvec[i])) return false;
		return true;
	}
	bool operator!=(const RGBColor &sp) const {
		return !(*this == sp);
	}
	bool Black() const {
		for (int i = 0; i < COLOR_SAMPLES; ++i)
			if (c[i] != 0.) return false;
		return true;
	}
	RGBColor Sqrt() const {
		RGBColor ret;
		//for (int i = 0; i < COLOR_SAMPLES; ++i)
		//	ret.c[i] = sqrtf(c[i]);
			
		for (int i = 0; i < COLOR_VECTORS; ++i)	
			ret.cvec[i]=_mm_sqrt_ps(cvec[i]);
		return ret;
	}
	RGBColor Pow(const RGBColor &e) const {
		RGBColor ret;
		for (int i = 0; i < COLOR_SAMPLES; ++i)
			ret.c[i] = c[i] > 0 ? powf(c[i], e.c[i]) : 0.f;
		return ret;
	}
	RGBColor operator-() const {
		RGBColor ret;
		for (int i = 0; i < COLOR_SAMPLES; ++i)
			ret.c[i] = -c[i];
		return ret;
	}
	friend RGBColor Exp(const RGBColor &s) {
		RGBColor ret;
		for (int i = 0; i < COLOR_SAMPLES; ++i)
			ret.c[i] = expf(s.c[i]);
		return ret;
	}
	RGBColor Clamp(float low = 0.f,
	               float high = INFINITY) const {
		RGBColor ret;
		for (int i = 0; i < COLOR_SAMPLES; ++i)
			ret.c[i] = ::Clamp(c[i], low, high);
		return ret;
	}
	bool IsNaN() const {
		for (int i = 0; i < COLOR_SAMPLES; ++i)
			if (isnan(c[i])) return true;
		return false;
	}
	void Print(FILE *f) const {
		for (int i = 0; i < COLOR_SAMPLES; ++i)
			fprintf(f, "%f ", c[i]);
	}
	void XYZ(float xyz[3]) const {
		xyz[0] = xyz[1] = xyz[2] = 0.;
		for (int i = 0; i < COLOR_SAMPLES; ++i) {
			xyz[0] += XWeight[i] * c[i];
			xyz[1] += YWeight[i] * c[i];
			xyz[2] += ZWeight[i] * c[i];
		}
	}
	float y() const {
		float v = 0.;
		for (int i = 0; i < COLOR_SAMPLES; ++i)
			v += YWeight[i] * c[i];
		return v;
	}
	bool operator<(const RGBColor &s2) const {
		return y() < s2.y();
	}
	friend class ParamSet;
	
	// RGBColor Public Data
	//float c[COLOR_SAMPLES];
	
	union
    {
     	__m128  cvec[COLOR_VECTORS];
      float c[COLOR_SAMPLES];
    };

	
//	void* operator new(size_t t) { return _mm_malloc(t,16); }
//    void operator delete(void* ptr, size_t t) { _mm_free(ptr); }
//    void* operator new[](size_t t) { return _mm_malloc(t,16); }
//    void operator delete[] (void* ptr) { _mm_free(ptr); }
//    void* operator new(long unsigned int i, RGBColor*) { return new RGBColor[i]; }
    
	
protected:
	// RGBColor Private Data
	static float XWeight[COLOR_SAMPLES];
	static float YWeight[COLOR_SAMPLES];
	static float ZWeight[COLOR_SAMPLES];
	friend RGBColor FromXYZ(float x, float y, float z);
};

#endif

inline RGBColor FromXYZ(float x, float y, float z) {
	float c[3];
	c[0] =  3.240479f * x + -1.537150f * y + -0.498535f * z;
	c[1] = -0.969256f * x +  1.875991f * y +  0.041556f * z;
	c[2] =  0.055648f * x + -0.204043f * y +  1.057311f * z;
	return RGBColor(c);
}

inline RGBColor FromXYZ(XYZColor c) {
	return FromXYZ(c.c[0], c.c[1], c.c[2]);
}
*/
#define Scalar float

class SWCSpectrum {
	friend class boost::serialization::access;
public:
	// RGBColor Public Methods
	SWCSpectrum(Scalar v = 0.f) {
		for (int i = 0; i < WAVELENGTH_SAMPLES; ++i)
			c[i] = v;
	}
	SWCSpectrum(const TsPack *tspack, RGBColor s);							// Note -radiance- - REFACT - can inline now.

	SWCSpectrum(const TsPack *tspack, const SPD *s);

	SWCSpectrum(float cs[WAVELENGTH_SAMPLES]) {
		for (int i = 0; i < WAVELENGTH_SAMPLES; ++i)
			c[i] = cs[i];
	}
	friend ostream &operator<<(ostream &, const SWCSpectrum &);
	SWCSpectrum &operator+=(const SWCSpectrum &s2) {
		for (int i = 0; i < WAVELENGTH_SAMPLES; ++i)
			c[i] += s2.c[i];
		return *this;
	}
	SWCSpectrum operator+(const SWCSpectrum &s2) const {
		SWCSpectrum ret = *this;
		for (int i = 0; i < WAVELENGTH_SAMPLES; ++i)
			ret.c[i] += s2.c[i];
		return ret;
	}
	SWCSpectrum operator-(const SWCSpectrum &s2) const {
		SWCSpectrum ret = *this;
		for (int i = 0; i < WAVELENGTH_SAMPLES; ++i)
			ret.c[i] -= s2.c[i];
		return ret;
	}
	SWCSpectrum operator/(const SWCSpectrum &s2) const {
		SWCSpectrum ret = *this;
		for (int i = 0; i < WAVELENGTH_SAMPLES; ++i)
			ret.c[i] /= s2.c[i];
		return ret;
	}
	SWCSpectrum operator*(const SWCSpectrum &sp) const {
		SWCSpectrum ret = *this;
		for (int i = 0; i < WAVELENGTH_SAMPLES; ++i)
			ret.c[i] *= sp.c[i];
		return ret;
	}
	SWCSpectrum &operator*=(const SWCSpectrum &sp) {
		for (int i = 0; i < WAVELENGTH_SAMPLES; ++i)
			c[i] *= sp.c[i];
		return *this;
	}
	SWCSpectrum operator*(Scalar a) const {
		SWCSpectrum ret = *this;
		for (int i = 0; i < WAVELENGTH_SAMPLES; ++i)
			ret.c[i] *= a;
		return ret;
	}
	SWCSpectrum &operator*=(Scalar a) {
		for (int i = 0; i < WAVELENGTH_SAMPLES; ++i)
			c[i] *= a;
		return *this;
	}
	friend inline
	SWCSpectrum operator*(Scalar a, const SWCSpectrum &s) {
		return s * a;
	}
	SWCSpectrum operator/(Scalar a) const {
		return *this * (1.f / a);
	}
	SWCSpectrum &operator/=(Scalar a) {
		Scalar inv = 1.f / a;
		for (int i = 0; i < WAVELENGTH_SAMPLES; ++i)
			c[i] *= inv;
		return *this;
	}
	void AddWeighted(Scalar w, const SWCSpectrum &s) {
		for (int i = 0; i < WAVELENGTH_SAMPLES; ++i)
			c[i] += w * s.c[i];
	}
	bool operator==(const SWCSpectrum &sp) const {
		for (int i = 0; i < WAVELENGTH_SAMPLES; ++i)
			if (c[i] != sp.c[i]) return false;
		return true;
	}
	bool operator!=(const SWCSpectrum &sp) const {
		return !(*this == sp);
	}
	bool Black() const {
		for (int i = 0; i < WAVELENGTH_SAMPLES; ++i)
			if (c[i] != 0.f) return false;
		return true;
	}
	SWCSpectrum Sqrt() const {
		SWCSpectrum ret;
		for (int i = 0; i < WAVELENGTH_SAMPLES; ++i)
			ret.c[i] = sqrtf(c[i]);
		return ret;
	}
	SWCSpectrum Pow(const SWCSpectrum &e) const {
		SWCSpectrum ret;
		for (int i = 0; i < WAVELENGTH_SAMPLES; ++i)
			ret.c[i] = c[i] > 0 ? powf(c[i], e.c[i]) : 0.f;
		return ret;
	}
	SWCSpectrum operator-() const {
		SWCSpectrum ret;
		for (int i = 0; i < WAVELENGTH_SAMPLES; ++i)
			ret.c[i] = -c[i];
		return ret;
	}
	friend SWCSpectrum Exp(const SWCSpectrum &s) {
		SWCSpectrum ret;
		for (int i = 0; i < WAVELENGTH_SAMPLES; ++i)
			ret.c[i] = expf(s.c[i]);
		return ret;
	}
	SWCSpectrum Clamp(Scalar low = 0.f,
	               Scalar high = INFINITY) const {
		SWCSpectrum ret;
		for (int i = 0; i < WAVELENGTH_SAMPLES; ++i)
			ret.c[i] = ::Clamp(c[i], low, high);
		return ret;
	}
	bool IsNaN() const {
		for (int i = 0; i < WAVELENGTH_SAMPLES; ++i)
			if (isnan(c[i])) return true;
		return false;
	}
	void Print(FILE *f) const {
		for (int i = 0; i < WAVELENGTH_SAMPLES; ++i)
			fprintf(f, "%f ", c[i]);
	}
	XYZColor ToXYZ(const TsPack *tspack) const;
	Scalar y(const TsPack *tspack) const;
	Scalar filter(const TsPack *tspack) const;

//	bool operator<(const SWCSpectrum &s2) const {
//		return y() < s2.y();												// Note - radiance - REFACT - need to rewrite without use of Spectrumwavelengths
//		return false;
//	}
	friend class lux::ParamSet;
	
	// SWCSpectrum Public Data
	Scalar c[WAVELENGTH_SAMPLES];
	
private:
	template<class Archive>
			void serialize(Archive & ar, const unsigned int version)
			{
				for (int i = 0; i < WAVELENGTH_SAMPLES; ++i)
					ar & c[i];
			}
};

}//namespace lux

#endif // LUX_RGBColor_H
