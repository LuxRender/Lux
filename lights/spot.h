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

// spot.cpp*
#include "lux.h"
#include "light.h"
#include "texture.h"

namespace lux
{

// SpotLight Declarations
class SpotLight : public Light {
public:
	// SpotLight Public Methods
	SpotLight(const Transform &light2world,
		const boost::shared_ptr< Texture<SWCSpectrum> > &L, 
		float gain, float power, float efficacy,
		float width, float fall, bool sup);
	virtual ~SpotLight();
	virtual bool IsSupport() const { return support; }
	virtual bool IsDeltaLight() const { return true; }
	virtual bool IsEnvironmental() const { return false; }
	virtual float Power(const Scene &) const {
		return Lbase->Y() * gain * 2.f * M_PI *
			(1.f - .5f * (cosFalloffStart + cosTotalWidth));
	}
	virtual float Pdf(const Point &p, const DifferentialGeometry &dg) const;
	virtual bool SampleL(const Scene &scene, const Sample &sample,
		float u1, float u2, float u3, BSDF **bsdf, float *pdf,
		SWCSpectrum *Le) const;
	virtual bool SampleL(const Scene &scene, const Sample &sample,
		const Point &p, float u1, float u2, float u3, BSDF **bsdf,
		float *pdf, float *pdfDirect, SWCSpectrum *Le) const;
	
	static Light *CreateLight(const Transform &light2world,
		const ParamSet &paramSet);
private:
	// SpotLight Private Data
	float cosTotalWidth, cosFalloffStart;
	Point lightPos;
	boost::shared_ptr<Texture<SWCSpectrum> > Lbase;
	float gain;
	bool support;
};

}//namespace lux

