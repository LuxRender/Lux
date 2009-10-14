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

#ifndef LUX_RGBColor_H
#define LUX_RGBColor_H
// spectrum.h*
#include "lux.h"
#include "color.h"

namespace lux
{

#define WAVELENGTH_SAMPLES 4
#define WAVELENGTH_START 380.f
#define WAVELENGTH_END   720.f
static const float inv_WAVELENGTH_SAMPLES = 1.f / WAVELENGTH_SAMPLES;

#define Scalar float

class SWCSpectrum {
	friend class boost::serialization::access;
public:
	// RGBColor Public Methods
	SWCSpectrum(Scalar v = 0.f) {
		for (int i = 0; i < WAVELENGTH_SAMPLES; ++i)
			c[i] = v;
	}
	SWCSpectrum(const TsPack *tspack, const RGBColor &s);							// Note -radiance- - REFACT - can inline now.

	SWCSpectrum(const TsPack *tspack, const SPD *s);

	SWCSpectrum(const float cs[WAVELENGTH_SAMPLES]) {
		for (int i = 0; i < WAVELENGTH_SAMPLES; ++i)
			c[i] = cs[i];
	}
	friend ostream &operator<<(ostream &, const SWCSpectrum &);
	SWCSpectrum operator+(const SWCSpectrum &s2) const {
		SWCSpectrum ret = *this;
		for (int i = 0; i < WAVELENGTH_SAMPLES; ++i)
			ret.c[i] += s2.c[i];
		return ret;
	}
	SWCSpectrum &operator+=(const SWCSpectrum &s2) {
		for (int i = 0; i < WAVELENGTH_SAMPLES; ++i)
			c[i] += s2.c[i];
		return *this;
	}
	SWCSpectrum operator-(const SWCSpectrum &s2) const {
		SWCSpectrum ret = *this;
		for (int i = 0; i < WAVELENGTH_SAMPLES; ++i)
			ret.c[i] -= s2.c[i];
		return ret;
	}
	SWCSpectrum &operator-=(const SWCSpectrum &s2) {
		for (int i = 0; i < WAVELENGTH_SAMPLES; ++i)
			c[i] -= s2.c[i];
		return *this;
	}
	SWCSpectrum operator/(const SWCSpectrum &s2) const {
		SWCSpectrum ret = *this;
		for (int i = 0; i < WAVELENGTH_SAMPLES; ++i)
			ret.c[i] /= s2.c[i];
		return ret;
	}
	SWCSpectrum &operator/=(const SWCSpectrum &sp) {
		for (int i = 0; i < WAVELENGTH_SAMPLES; ++i)
			c[i] /= sp.c[i];
		return *this;
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
		return *this *= (1.f / a);
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
			ret.c[i] = c[i] > 0.f ? powf(c[i], e.c[i]) : 0.f;
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
	Scalar Y(const TsPack *tspack) const;
	Scalar Filter(const TsPack *tspack) const;

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
