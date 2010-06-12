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

#ifndef LUX_WARDISOTROPIC_H
#define LUX_WARDISOTROPIC_H
// wardisotropic.h*
#include "lux.h"
#include "microfacetdistribution.h"

namespace lux
{

// Kelemen and Szirmay-Kalos / Microfacet Based BRDF Model, Eurographics 2001
class  WardIsotropic : public MicrofacetDistribution {
public:
	WardIsotropic(float rms);
	virtual ~WardIsotropic() { }

	// WardIsotropic Public Methods
	virtual float D(const Vector &wh) const;
	virtual void SampleH(float u1, float u2, Vector *wh, float *d,
		float *pdf) const;
	virtual float Pdf(const Vector &wh) const;

private:
	float r;
};

}//namespace lux

#endif // LUX_WARDISOTROPIC_H

