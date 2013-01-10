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

#ifndef LUX_PARTIALCONTRIBUTION_H
#define LUX_PARTIALCONTRIBUTION_H

#include <vector>

#include "core/spectrumwavelengths.h"
#include "core/sampling.h"

namespace lux
{
struct ContextSingle
{
	ContextSingle(const SpectrumWavelengths &sw_)
		:sw(sw_),
		single(sw.single)
	{
	}
	~ContextSingle()
	{
		sw.single = single;
	}

	const SpectrumWavelengths& sw;
	bool single;
};

class PartialContribution {
	struct contrib
	{
		contrib() :L(0.f), V(0.f), active(false) { }

		SWCSpectrum L;
		float V;
		bool active;
	};

public:
	PartialContribution(const u_int nGroups) : vecNotSingle(nGroups) { }

	void Add(const SpectrumWavelengths &sw, SWCSpectrum L, u_int group, float weight)
	{
		AddUnFiltered(sw, L, group, L.Filter(sw) * weight, sw.single);
	}

	void Add(const SpectrumWavelengths &sw, SWCSpectrum L, u_int group, float weight, bool single)
	{
		AddUnFiltered(sw, L, group, L.Filter(sw) * weight, single);
	}

	void AddUnFiltered(const SpectrumWavelengths &sw, SWCSpectrum L, u_int group, float V)
	{
		AddUnFiltered(sw, L, group, V, sw.single);
	}

	void AddUnFiltered(const SpectrumWavelengths &sw, SWCSpectrum L, u_int group, float V, bool single)
	{
		if (!single) {
			vecNotSingle[group].L += L;
		} else {
			// Dispersion, only add the selected wavelength
			// Compensate for the wavelength selection probability
			vecNotSingle[group].L.c[sw.single_w] += L.c[sw.single_w] * WAVELENGTH_SAMPLES;
		}

		vecNotSingle[group].V += V;
		vecNotSingle[group].active = true;
	}

	void Splat(
			const SpectrumWavelengths &sw,
			const Sample &sample,
			float xl,
			float yl,
			float d,
			float alpha,
			u_int bufferId,
			float weight=1.f);

	static XYZColor toXYZColor(const SpectrumWavelengths &sw, SWCSpectrum &s, bool single)
	{
		ContextSingle ctx(sw);

		sw.single = single;
		return XYZColor(sw, s);
	}

private:
	void SplatW(
			const SpectrumWavelengths &sw,
			const Sample &sample,
			vector<struct contrib> &vec,
			float xl,
			float yl,
			float d,
			float alpha,
			u_int bufferId,
			float weight=1.f);

	std::vector<struct contrib> vecNotSingle;
};
}
#endif
