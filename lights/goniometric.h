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

// goniometric.cpp*
#include "lux.h"
#include "light.h"
#include "shape.h"
#include "scene.h"
#include "mipmap.h"
#include "sphericalfunction.h"

namespace lux
{

// GonioPhotometricLight Declarations
class GonioPhotometricLight : public Light {
public:
	// GonioPhotometricLight Public Methods
	GonioPhotometricLight(const Transform &light2world, 
		const RGBColor &intensity, float gain,
		const string &texname, const string &iesname);
	~GonioPhotometricLight();
	bool IsDeltaLight() const { return true; }
	SWCSpectrum Power(const TsPack *tspack, const Scene *) const;
	SWCSpectrum Sample_L(const TsPack *tspack, const Point &P, float u1, float u2, float u3,
		Vector *wo, float *pdf, VisibilityTester *visibility) const;
	SWCSpectrum Sample_L(const TsPack *tspack, const Scene *scene, float u1, float u2,
			float u3, float u4, Ray *ray, float *pdf) const;
	float Pdf(const Point &, const Vector &) const;
	
	static Light *CreateLight(const Transform &light2world,
		const ParamSet &paramSet);
private:
	SWCSpectrum L(const TsPack *tspack, const Vector& w) const;
	// GonioPhotometricLight Private Data
	Point lightPos;
	SPD *LSPD;
	SampleableSphericalFunction *func;
};

}//namespace lux

