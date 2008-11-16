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


// scale.cpp*
#include "lux.h"
#include "texture.h"
#include "paramset.h"

namespace lux
{

// ScaleTexture Declarations
template <class T1, class T2>
class ScaleTexture : public Texture<T2> {
public:
	// ScaleTexture Public Methods
	ScaleTexture(boost::shared_ptr<Texture<T1> > t1,
			boost::shared_ptr<Texture<T2> > t2) {
		tex1 = t1;
		tex2 = t2;
	}
	T2 Evaluate(const TsPack *tspack, const DifferentialGeometry &dg) const {
		return tex1->Evaluate(tspack, dg) * tex2->Evaluate(tspack, dg);
	}
	
	static Texture<float> * CreateFloatTexture(const Transform &tex2world, const TextureParams &tp);
	static Texture<SWCSpectrum> * CreateSWCSpectrumTexture(const Transform &tex2world, const TextureParams &tp);
private:
	boost::shared_ptr<Texture<T1> > tex1;
	boost::shared_ptr<Texture<T2> > tex2;
};

// ScaleTexture Method Definitions
template <class T, class U> inline Texture<float> * ScaleTexture<T,U>::CreateFloatTexture(const Transform &tex2world,
		const TextureParams &tp) {
	return new ScaleTexture<float, float>(tp.GetFloatTexture("tex1", 1.f),
		tp.GetFloatTexture("tex2", 1.f));
}

template <class T,class U> inline Texture<SWCSpectrum> * ScaleTexture<T,U>::CreateSWCSpectrumTexture(const Transform &tex2world,
		const TextureParams &tp) {
	return new ScaleTexture<SWCSpectrum, SWCSpectrum>(
		tp.GetSWCSpectrumTexture("tex1", RGBColor(1.f)),
		tp.GetSWCSpectrumTexture("tex2", RGBColor(1.f)));
}

}//namespace lux

