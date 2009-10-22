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

#ifndef LUX_SPD_H
#define LUX_SPD_H
// spd.h*

namespace lux
{

class SPD {
public:
	SPD() { 
		nSamples = 0U; 
		lambdaMin = lambdaMax = delta = invDelta = 0.f;
		samples = NULL;
	}
	virtual ~SPD() { FreeSamples(); }

	// samples the SPD by performing a linear interpolation on the data
	inline float sample(const float lambda) const {
		if (nSamples <= 1 || lambda < lambdaMin || lambda > lambdaMax)
			return 0.f;

		// interpolate the two closest samples linearly
		const float x = (lambda - lambdaMin) * invDelta;
		const u_int b0 = Floor2UInt(x);
		const u_int b1 = min(b0 + 1, nSamples - 1);
		const float dx = x - b0;
		return Lerp(dx, samples[b0], samples[b1]);
	}

	inline void sample(u_int n, const float lambda[], float *p) const {
		for (u_int i = 0; i < n; ++i) {
			if (nSamples <= 1 || lambda[i] < lambdaMin ||
				lambda[i] > lambdaMax) {
				p[i] = 0.f;
				continue;
			}

			// interpolate the two closest samples linearly
			const float x = (lambda[i] - lambdaMin) * invDelta;
			const u_int b0 = Floor2UInt(x);
			const u_int b1 = min(b0 + 1, nSamples - 1);
			const float dx = x - b0;
			p[i] = Lerp(dx, samples[b0], samples[b1]);
		}
	}

	float Y() const ;
	XYZColor ToXYZ() const;
	void AllocateSamples(u_int n);
	void FreeSamples();
	void Normalize();
	void Clamp();
	void Scale(float s);
	void Whitepoint(float temp);

protected:
	u_int nSamples;
	float lambdaMin, lambdaMax;
	float delta, invDelta;
	float *samples;

};

}//namespace lux

#endif // LUX_SPD_H
