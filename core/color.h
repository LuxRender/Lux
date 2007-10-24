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

#ifndef LUX_COLOR_H
#define LUX_COLOR_H
// color.h*
#include "lux.h"
// Spectrum Declarations

#ifndef LUX_USE_SSE

class  Spectrum {
public:
	// Spectrum Public Methods
	Spectrum(float v = 0.f) {
		for (int i = 0; i < COLOR_SAMPLES; ++i)
			c[i] = v;
	}
	Spectrum(float cs[COLOR_SAMPLES]) {
		for (int i = 0; i < COLOR_SAMPLES; ++i)
			c[i] = cs[i];
	}
	friend ostream &operator<<(ostream &, const Spectrum &);
	Spectrum &operator+=(const Spectrum &s2) {
		for (int i = 0; i < COLOR_SAMPLES; ++i)
			c[i] += s2.c[i];
		return *this;
	}
	Spectrum operator+(const Spectrum &s2) const {
		Spectrum ret = *this;
		for (int i = 0; i < COLOR_SAMPLES; ++i)
			ret.c[i] += s2.c[i];
		return ret;
	}
	Spectrum operator-(const Spectrum &s2) const {
		Spectrum ret = *this;
		for (int i = 0; i < COLOR_SAMPLES; ++i)
			ret.c[i] -= s2.c[i];
		return ret;
	}
	Spectrum operator/(const Spectrum &s2) const {
		Spectrum ret = *this;
		for (int i = 0; i < COLOR_SAMPLES; ++i)
			ret.c[i] /= s2.c[i];
		return ret;
	}
	Spectrum operator*(const Spectrum &sp) const {
		Spectrum ret = *this;
		for (int i = 0; i < COLOR_SAMPLES; ++i)
			ret.c[i] *= sp.c[i];
		return ret;
	}
	Spectrum &operator*=(const Spectrum &sp) {
		for (int i = 0; i < COLOR_SAMPLES; ++i)
			c[i] *= sp.c[i];
		return *this;
	}
	Spectrum operator*(float a) const {
		Spectrum ret = *this;
		for (int i = 0; i < COLOR_SAMPLES; ++i)
			ret.c[i] *= a;
		return ret;
	}
	Spectrum &operator*=(float a) {
		for (int i = 0; i < COLOR_SAMPLES; ++i)
			c[i] *= a;
		return *this;
	}
	friend inline
	Spectrum operator*(float a, const Spectrum &s) {
		return s * a;
	}
	Spectrum operator/(float a) const {
		return *this * (1.f / a);
	}
	Spectrum &operator/=(float a) {
		float inv = 1.f / a;
		for (int i = 0; i < COLOR_SAMPLES; ++i)
			c[i] *= inv;
		return *this;
	}
	void AddWeighted(float w, const Spectrum &s) {
		for (int i = 0; i < COLOR_SAMPLES; ++i)
			c[i] += w * s.c[i];
	}
	bool operator==(const Spectrum &sp) const {
		for (int i = 0; i < COLOR_SAMPLES; ++i)
			if (c[i] != sp.c[i]) return false;
		return true;
	}
	bool operator!=(const Spectrum &sp) const {
		return !(*this == sp);
	}
	bool Black() const {
		for (int i = 0; i < COLOR_SAMPLES; ++i)
			if (c[i] != 0.) return false;
		return true;
	}
	Spectrum Sqrt() const {
		Spectrum ret;
		for (int i = 0; i < COLOR_SAMPLES; ++i)
			ret.c[i] = sqrtf(c[i]);
		return ret;
	}
	Spectrum Pow(const Spectrum &e) const {
		Spectrum ret;
		for (int i = 0; i < COLOR_SAMPLES; ++i)
			ret.c[i] = c[i] > 0 ? powf(c[i], e.c[i]) : 0.f;
		return ret;
	}
	Spectrum operator-() const {
		Spectrum ret;
		for (int i = 0; i < COLOR_SAMPLES; ++i)
			ret.c[i] = -c[i];
		return ret;
	}
	friend Spectrum Exp(const Spectrum &s) {
		Spectrum ret;
		for (int i = 0; i < COLOR_SAMPLES; ++i)
			ret.c[i] = expf(s.c[i]);
		return ret;
	}
	Spectrum Clamp(float low = 0.f,
	               float high = INFINITY) const {
		Spectrum ret;
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
	bool operator<(const Spectrum &s2) const {
		return y() < s2.y();
	}
	friend class ParamSet;
	
	// Spectrum Public Data
	float c[COLOR_SAMPLES];
	static const int CIEstart = 360;
	static const int CIEend = 830;
	static const int nCIE = CIEend-CIEstart+1;
	static const float CIE_X[nCIE];
	static const float CIE_Y[nCIE];
	static const float CIE_Z[nCIE];
	
protected:
	// Spectrum Private Data
	static float XWeight[COLOR_SAMPLES];
	static float YWeight[COLOR_SAMPLES];
	static float ZWeight[COLOR_SAMPLES];
	friend Spectrum FromXYZ(float x, float y, float z);
};

#else //LUX_USE_SSE

#define COLOR_VECTORS 1

class  _MM_ALIGN16 Spectrum {
public:
	// Spectrum Public Methods
	Spectrum(float v = 0.f) {
		for (int i = 0; i < COLOR_SAMPLES; ++i)
			c[i] = v;
	}
	Spectrum(float cs[COLOR_SAMPLES]) {
		for (int i = 0; i < COLOR_SAMPLES; ++i)
			c[i] = cs[i];
	}
	Spectrum(__m128 cv[COLOR_VECTORS])
    {
    	for (int i = 0; i < COLOR_VECTORS; ++i)
    		cvec[i]=cv[i];
    }
	
	friend ostream &operator<<(ostream &, const Spectrum &);
	Spectrum &operator+=(const Spectrum &s2) {
		for (int i = 0; i < COLOR_SAMPLES; ++i)
			c[i] += s2.c[i];
		return *this;
	}
	Spectrum operator+(const Spectrum &s2) const {
		Spectrum ret;// = *this;
		for (int i = 0; i < COLOR_VECTORS; ++i)
			ret.cvec[i]=_mm_add_ps(cvec[i],s2.cvec[i]);
		
		//for (int i = 0; i < COLOR_SAMPLES; ++i)
		//	ret.c[i] += s2.c[i];
		return ret;
	}
	Spectrum operator-(const Spectrum &s2) const {
		Spectrum ret = *this;
		for (int i = 0; i < COLOR_SAMPLES; ++i)
			ret.c[i] -= s2.c[i];
		return ret;
	}
	Spectrum operator/(const Spectrum &s2) const {
		Spectrum ret;// = *this;
		for (int i = 0; i < COLOR_VECTORS; ++i)
			ret.cvec[i]=_mm_div_ps(cvec[i],s2.cvec[i]);
		//for (int i = 0; i < COLOR_SAMPLES; ++i)
		//	ret.c[i] /= s2.c[i];
		return ret;
	}
	Spectrum operator*(const Spectrum &sp) const {
		Spectrum ret;// = *this;
		for (int i = 0; i < COLOR_VECTORS; ++i)
			ret.cvec[i]=_mm_mul_ps(cvec[i],sp.cvec[i]);
		
		/*for (int i = 0; i < COLOR_SAMPLES; ++i)
			ret.c[i] *= sp.c[i];*/
		return ret;
	}
	Spectrum &operator*=(const Spectrum &sp) {
		//for (int i = 0; i < COLOR_SAMPLES; ++i)
		//	c[i] *= sp.c[i];
		
		for (int i = 0; i < COLOR_VECTORS; ++i)
			cvec[i]=_mm_mul_ps(cvec[i],sp.cvec[i]);
		
		return *this;
	}
	Spectrum operator*(float a) const {
		Spectrum ret = *this;
		for (int i = 0; i < COLOR_SAMPLES; ++i)
			ret.c[i] *= a;
		return ret;
	}
	Spectrum &operator*=(float a) {
		for (int i = 0; i < COLOR_SAMPLES; ++i)
			c[i] *= a;
		return *this;
	}
	friend inline
	Spectrum operator*(float a, const Spectrum &s) {
		return s * a;
	}
	Spectrum operator/(float a) const {
		return *this * (1.f / a);
	}
	Spectrum &operator/=(float a) {
		float inv = 1.f / a;
		for (int i = 0; i < COLOR_SAMPLES; ++i)
			c[i] *= inv;
		return *this;
	}
	void AddWeighted(float w, const Spectrum &s) {
		//for (int i = 0; i < COLOR_SAMPLES; ++i)
		//	c[i] += w * s.c[i];
		__m128 wvec=_mm_set_ps1(w);
		
		for (int i = 0; i < COLOR_VECTORS; ++i)
			cvec[i]=_mm_add_ps(cvec[i],_mm_mul_ps(s.cvec[i],wvec));
	}
	bool operator==(const Spectrum &sp) const {
		for (int i = 0; i < COLOR_SAMPLES; ++i)
			if (c[i] != sp.c[i]) return false;
		
		//for (int i = 0; i < COLOR_VECTORS; ++i)	
		//	if(_mm_cmpneq_ps(cvec[i],sp.cvec[i])) return false;
		return true;
	}
	bool operator!=(const Spectrum &sp) const {
		return !(*this == sp);
	}
	bool Black() const {
		for (int i = 0; i < COLOR_SAMPLES; ++i)
			if (c[i] != 0.) return false;
		return true;
	}
	Spectrum Sqrt() const {
		Spectrum ret;
		//for (int i = 0; i < COLOR_SAMPLES; ++i)
		//	ret.c[i] = sqrtf(c[i]);
			
		for (int i = 0; i < COLOR_VECTORS; ++i)	
			ret.cvec[i]=_mm_sqrt_ps(cvec[i]);
		return ret;
	}
	Spectrum Pow(const Spectrum &e) const {
		Spectrum ret;
		for (int i = 0; i < COLOR_SAMPLES; ++i)
			ret.c[i] = c[i] > 0 ? powf(c[i], e.c[i]) : 0.f;
		return ret;
	}
	Spectrum operator-() const {
		Spectrum ret;
		for (int i = 0; i < COLOR_SAMPLES; ++i)
			ret.c[i] = -c[i];
		return ret;
	}
	friend Spectrum Exp(const Spectrum &s) {
		Spectrum ret;
		for (int i = 0; i < COLOR_SAMPLES; ++i)
			ret.c[i] = expf(s.c[i]);
		return ret;
	}
	Spectrum Clamp(float low = 0.f,
	               float high = INFINITY) const {
		Spectrum ret;
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
	bool operator<(const Spectrum &s2) const {
		return y() < s2.y();
	}
	friend class ParamSet;
	
	// Spectrum Public Data
	//float c[COLOR_SAMPLES];
	
	union
    {
     	__m128  cvec[COLOR_VECTORS];
      float c[COLOR_SAMPLES];
    };
	
	
	static const int CIEstart = 360;
	static const int CIEend = 830;
	static const int nCIE = CIEend-CIEstart+1;
	static const float CIE_X[nCIE];
	static const float CIE_Y[nCIE];
	static const float CIE_Z[nCIE];

/*	
	void* operator new(size_t t) { return _mm_malloc(t,16); }
    void operator delete(void* ptr, size_t t) { _mm_free(ptr); }
    void* operator new[](size_t t) { return _mm_malloc(t,16); }
    void operator delete[] (void* ptr) { _mm_free(ptr); }
    void* operator new(long unsigned int i, Spectrum*) { return new Spectrum[i]; }
    */
	
protected:
	// Spectrum Private Data
	static float XWeight[COLOR_SAMPLES];
	static float YWeight[COLOR_SAMPLES];
	static float ZWeight[COLOR_SAMPLES];
	friend Spectrum FromXYZ(float x, float y, float z);
};

#endif

Spectrum FromXYZ(float x, float y, float z);

class RegularSpectrum : public Spectrum {
public:
	float *wavelengths;
    float lambdaMin, lambdaMax;
    float delta, invDelta;
	int sWa;
    
    RegularSpectrum(float *wl, float lMin, float lMax, int n) {
        lambdaMin = lMin;
        lambdaMax = lMax;
        wavelengths = wl;
		sWa = n;
        delta = (lambdaMax - lambdaMin) / (sWa-1);
        invDelta = 1 / delta;
    }
    
    inline float sample(float lambda) {
        // reject wavelengths outside the valid range
        if (lambda < lambdaMin || lambda > lambdaMax)
            return 0.;
        // interpolate the two closest samples linearly
        float x = (lambda - lambdaMin) * invDelta;
        int b0 = (int) x;
        int b1 = min(b0+1, sWa-1);
        float dx = x - b0;
        return (1. -dx) * wavelengths[b0] + dx * wavelengths[b1];
    }

	inline Spectrum toSpectrum() {
		float X = 0, Y = 0, Z = 0;
		for(int i=0,w=CIEstart; i < nCIE; i++, w++) {
			float s = sample(w);
			X += s * CIE_X[i];
			Y += s * CIE_Y[i];
			Z += s * CIE_Z[i];
		}
		return FromXYZ(X,Y,Z);
	}
};

class IrregularSpectrum : public Spectrum {
public:
    float *wavelengths;
    float *amplitudes;
	int sWa, sAm;

    IrregularSpectrum(float *wl, float *am, int n)
	{
        wavelengths = wl;
        amplitudes = am;
		sWa = n;
		sAm = n;
    }

    inline float sample(float lambda)
	{
		if (lambda < wavelengths[0])
			return amplitudes[0];
		if (lambda > wavelengths[sWa-1])
			return amplitudes[sWa - 1];
 
		int index = 0;
		for (; index < sWa; ++index)
			if (wavelengths[index] >= lambda)
				break;
 
		if (wavelengths[index] == lambda)
			return amplitudes[index];
 
		float intervalWidth = wavelengths[index] - wavelengths[index - 1];
		float u = (lambda - wavelengths[index - 1]) / intervalWidth;
		return ((1. - u) * amplitudes[index - 1]) + (u * amplitudes[index]);




/*

        if(sWa == 0)
            return 0; // no data
        if(sWa == 1 || lambda <= wavelengths[0])
            return amplitudes[0];
        if(lambda >= wavelengths[sWa-1])
            return amplitudes[sWa-1];
        for(int i=1; i < sWa; i++) {
            if(lambda < wavelengths[i]) {
                float dx = (lambda - wavelengths[i-1]) /
					(wavelengths[i] - wavelengths[i-1]);
                return (1. -dx) * amplitudes[i-1] + dx * amplitudes[i];
            }
        }
        return amplitudes[sWa-1]; */
    }

	inline Spectrum toSpectrum() {
		float X = 0, Y = 0, Z = 0;
		for(int i=0,w=CIEstart; i < nCIE; i++, w++) {
			float s = sample(w);
			X += s * CIE_X[i];
			Y += s * CIE_Y[i];
			Z += s * CIE_Z[i];
		}
		return FromXYZ(X,Y,Z);
	}
};

#endif // LUX_COLOR_H
