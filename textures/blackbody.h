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

// blackbody.cpp*
#include "lux.h"
#include "texture.h"
#include "blackbodyspd.h"
#include "paramset.h"

namespace lux
{

// BlackBodyTexture Declarations
template <class T>
class BlackBodyFloatTexture : public Texture<T> {
public:
	// BlackBodyTexture Public Methods
	BlackBodyFloatTexture(const T &v) { value = v; }
	virtual ~BlackBodyFloatTexture() { }
	virtual T Evaluate(const TsPack *tspack, const DifferentialGeometry &) const {
		return value;
	}
private:
	T value;
};

template <class T>
class BlackBodySpectrumTexture : public Texture<T> {
public:
	// BlackBodyTexture Public Methods
	BlackBodySpectrumTexture(const float &t) {
		BBSPD = new BlackbodySPD(t);
	}
	virtual ~BlackBodySpectrumTexture() { }
	virtual T Evaluate(const TsPack *tspack, const DifferentialGeometry &) const {
		return SWCSpectrum(tspack, BBSPD);
	}
	virtual void SetPower(float power, float area) {
		BBSPD->Scale(power / (area * M_PI * BBSPD->y()));
	}

private:
	BlackbodySPD* BBSPD;
};

class BlackBodyTexture
{
public:
	static Texture<float> * CreateFloatTexture(const Transform &tex2world, const TextureParams &tp);
	static Texture<SWCSpectrum> * CreateSWCSpectrumTexture(const Transform &tex2world, const TextureParams &tp);
};

}//namespace lux

