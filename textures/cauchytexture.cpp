/***************************************************************************
 *   Copyright (C) 1998-2013 by authors (see AUTHORS.txt)                  *
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

// cauchytexture.cpp*
#include "cauchytexture.h"
#include "dynload.h"

using namespace lux;

// CauchyTexture Method Definitions
Texture<FresnelGeneral> *CauchyTexture::CreateFresnelTexture(const Transform &tex2world,
	const ParamSet &tp)
{
	const float cauchyb = tp.FindOneFloat("cauchyb", 0.f);
	const float index = tp.FindOneFloat("index", -1.f);
	float cauchya;
	if (index > 0.f)
		cauchya = tp.FindOneFloat("cauchya", index - cauchyb * 1e6f /
			(WAVELENGTH_END * WAVELENGTH_START));
	else
		cauchya = tp.FindOneFloat("cauchya", 1.5f);
	return new CauchyTexture(cauchya, cauchyb);
}

// AbbeTexture Method Definitions
Texture<FresnelGeneral> *AbbeTexture::CreateFresnelTexture(const Transform &tex2world,
	const ParamSet &tp)
{
	// which lines to use
	// d, D, e or custom
	string type = tp.FindOneString("type", "d");
	
	// default is d type
	float lambda_D = 587.5618f;
	float lambda_F = 486.13f;
	float lambda_C = 656.28f;
	if (type == "D") {
		lambda_D = 589.29f;
	} else if (type == "e") {
		lambda_D = 546.07f;
		lambda_F = 479.99f;
		lambda_C = 643.85f;
	} else if (type == "custom") {
		lambda_D = tp.FindOneFloat("lambda_D", lambda_D);
		lambda_F = tp.FindOneFloat("lambda_F", lambda_F);
		lambda_C = tp.FindOneFloat("lambda_C", lambda_C);
	}

	// default is V_d for BK7
	const float V = tp.FindOneFloat("V", 64.17f);
	const float n = tp.FindOneFloat("n", 1.51680f);

	// convert to Cauchy coefficients

	// convert wavelengths from nm to um
	lambda_D *= 1e-3f;
	lambda_F *= 1e-3f;
	lambda_C *= 1e-3f;
	
	// and square
	lambda_D *= lambda_D;
	lambda_F *= lambda_F;
	lambda_C *= lambda_C;

	const float cauchy_B = (n - 1.f) * (lambda_C * lambda_F) / (V * (lambda_C - lambda_F));
	const float cauchy_A = n - cauchy_B / lambda_D;

	return new CauchyTexture(cauchy_A, cauchy_B);
}

static DynamicLoader::RegisterFresnelTexture<CauchyTexture> r("cauchy");
static DynamicLoader::RegisterFresnelTexture<AbbeTexture> r2("abbe");
