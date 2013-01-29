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

// contribution.cpp*
#include "lux.h"
#include "contribution.h"
#include "film.h"

#include <core/partialcontribution.h>

namespace lux
{
	void PartialContribution::Splat(
			const SpectrumWavelengths &sw,
			const Sample &sample,
			float xl,
			float yl,
			float d,
			float alpha,
			u_int bufferId,
			float weight)
	{
		ContextSingle ctx(sw);

		sw.single = false;
		SplatW(sw, sample, vecNotSingle, xl, yl, d, alpha, bufferId, weight);
	}

	void PartialContribution::SplatW(
			const SpectrumWavelengths &sw,
			const Sample &sample,
			vector<contrib> &vec,
			float xl,
			float yl,
			float d,
			float alpha,
			u_int bufferId,
			float weight)

	{
		const u_int nGroups = vec.size();
		for (u_int i = 0; i < nGroups; ++i) {
			const SWCSpectrum L(vec[i].L.Clamp());
			if (!L.Black())
				vec[i].V /= L.Filter(sw);
			const XYZColor color(sw, L);
			sample.AddContribution(xl, yl,
				color * weight, alpha, d, vec[i].V,
				bufferId, i);
			vec[i].L = SWCSpectrum(0.f);
			vec[i].V = 0.f;
		}
	}
}
