/***************************************************************************
 *   Copyright (C) 1998-2007 by authors (see AUTHORS.txt )                 *
 *                                                                         *
 *   This file is part of Lux Renderer.                                    *
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
 *   Lux Renderer website : http://www.luxrender.org                       *
 ***************************************************************************/

// bidirectional.cpp*
#include "lux.h"
#include "transport.h"

namespace lux
{

// Bidirectional Local Declarations
class BidirIntegrator : public SurfaceIntegrator {
public:
	// PathIntegrator types
	enum LightStrategy { SAMPLE_ALL_UNIFORM, SAMPLE_ONE_UNIFORM,
		SAMPLE_AUTOMATIC
	};
//	enum RRStrategy { RR_EFFICIENCY, RR_PROBABILITY, RR_NONE };

	BidirIntegrator(int ed, int ld, float et, float lt, LightStrategy ls,
		bool d) :
		maxEyeDepth(ed), maxLightDepth(ld),
		eyeThreshold(et), lightThreshold(lt),
		lightStrategy(ls), debug(d) {
		eyeBufferId = -1;
		lightBufferId = -1;
	}
	// BidirIntegrator Public Methods
	int Li(const TsPack *tspack, const Scene *scene, const RayDifferential &ray, const Sample *sample) const;
	void RequestSamples(Sample *sample, const Scene *scene);
	void Preprocess(const TsPack *tspack, const Scene *scene);
	static SurfaceIntegrator *CreateSurfaceIntegrator(const ParamSet &params);
	int maxEyeDepth, maxLightDepth;
	float eyeThreshold, lightThreshold;
	int sampleEyeOffset, sampleLightOffset;
	int eyeBufferId, lightBufferId;
private:
	// BidirIntegrator Data
	LightStrategy lightStrategy;
	int lightNumOffset, lightComponentOffset;
	int lightPosOffset, lightDirOffset, sampleDirectOffset;
	bool debug;
};

}//namespace lux

