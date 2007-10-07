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
	
private:
	// Spectrum Private Data
	static float XWeight[COLOR_SAMPLES];
	static float YWeight[COLOR_SAMPLES];
	static float ZWeight[COLOR_SAMPLES];
	friend Spectrum FromXYZ(float x, float y, float z);
};

class RegularSpectrum {
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
        return (1-dx) * wavelengths[b0] + dx * wavelengths[b1];
    }

	inline Spectrum toSpectrum() {
		float st[3];	// TODO Add color samples & cleanup
		st[2] = sample( 360 );
		st[1] = sample( 595 );
		st[0] = sample( 830 );
        Spectrum o = Spectrum( st );
		return o;
        /*for (int i = 0, w = WAVELENGTH_MIN; i < CIE_xbar.length; i++, w += WAVELENGTH_STEP) {
            float s = sample(w);
            X += s * CIE_xbar[i];
            Y += s * CIE_ybar[i];
            Z += s * CIE_zbar[i];
        }
        return new XYZColor(X, Y, Z).mul(WAVELENGTH_STEP); */
    }
};

class IrregularSpectrum {
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
  //      if(sWa != sAm)		// TODO cleanup
   //         return; // TODO add exception    
   //     for(int i = 1; i < sWa; i++)
   //         if(wavelengths[i-1] >= wavelengths[i])
   //             return; // TODO add exception    

    }

    inline float sample(float lambda)
	{
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
                return (1-dx) * amplitudes[i-1] + dx * amplitudes[i];
            }
        }
        return amplitudes[sWa-1];
    }

	inline Spectrum toSpectrum() {
		float st[3];	// // TODO Add color samples & cleanup
		st[2] = sample( 360 );
		st[1] = sample( 595 );
		st[0] = sample( 830 );
        Spectrum o = Spectrum( st );
		return o;
        /*for (int i = 0, w = WAVELENGTH_MIN; i < CIE_xbar.length; i++, w += WAVELENGTH_STEP) {
            float s = sample(w);
            X += s * CIE_xbar[i];
            Y += s * CIE_ybar[i];
            Z += s * CIE_zbar[i];
        }
        return new XYZColor(X, Y, Z).mul(WAVELENGTH_STEP); */
    }
};

#endif // LUX_COLOR_H
