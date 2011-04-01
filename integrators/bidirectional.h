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

	BidirIntegrator(u_int ed, u_int ld, float et, float lt, LightStrategy ls,
		bool d) :
		maxEyeDepth(ed), maxLightDepth(ld),
		eyeThreshold(et), lightThreshold(lt),
		lightStrategy(ls), debug(d) {
		eyeBufferId = 0;
		lightBufferId = 0;
	}
	virtual ~BidirIntegrator() { }
	// BidirIntegrator Public Methods
	virtual u_int Li(const TsPack *tspack, const Scene *scene, const Sample *sample) const;
	virtual void RequestSamples(Sample *sample, const Scene *scene);
	virtual void Preprocess(const TsPack *tspack, const Scene *scene);
	static SurfaceIntegrator *CreateSurfaceIntegrator(const ParamSet &params);
	u_int maxEyeDepth, maxLightDepth;
	float eyeThreshold, lightThreshold;
	u_int sampleEyeOffset, sampleLightOffset;
	u_int eyeBufferId, lightBufferId;
private:
	// BidirIntegrator Data
	LightStrategy lightStrategy;
	u_int lightNumOffset, lightComponentOffset;
	u_int lightPosOffset, lightDirOffset, sampleDirectOffset;
	bool debug;
};

}//namespace lux

