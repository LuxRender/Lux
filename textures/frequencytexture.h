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

// frequency.cpp*
#include "lux.h"
#include "texture.h"
#include "frequencyspd.h"
#include "paramset.h"

namespace lux
{

// FrequencyTexture Declarations
template <class T>
class FrequencyFloatTexture : public Texture<T> {
public:
	// FrequencyTexture Public Methods
	FrequencyFloatTexture(const T &v) { value = v; }
	T Evaluate(const TsPack *tspack, const DifferentialGeometry &) const {
		return value;
	}
private:
	T value;
};

template <class T>
class FrequencySpectrumTexture : public Texture<T> {
public:
	// FrequencyTexture Public Methods
	FrequencySpectrumTexture(const float &w, const float &p, const float &r) {
		FSPD = new FrequencySPD(w, p, r);
	}
	T Evaluate(const TsPack *tspack, const DifferentialGeometry &) const {
		return SWCSpectrum(tspack, FSPD);
	}
	void SetPower(float power, float area) {
		FSPD->Scale(power / (area * M_PI * FSPD->y()));
	}
private:
	FrequencySPD* FSPD;
};

class FrequencyTexture
{
public:
	static Texture<float> * CreateFloatTexture(const Transform &tex2world, const TextureParams &tp);
	static Texture<SWCSpectrum> * CreateSWCSpectrumTexture(const Transform &tex2world, const TextureParams &tp);
};

}//namespace lux

