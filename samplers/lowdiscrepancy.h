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
 
// lowdiscrepancy.cpp*
#include "sampling.h"
#include "paramset.h"
#include "film.h"
// LDSampler Declarations
class LDSampler : public Sampler {
public:
	// LDSampler Public Methods
	LDSampler(int xstart, int xend,
	          int ystart, int yend,
			  int nsamp);
	~LDSampler() {
		delete[] imageSamples;
		for (int i = 0; i < n1D; ++i)
			delete[] oneDSamples[i];
		for (int i = 0; i < n2D; ++i)
			delete[] twoDSamples[i];
		delete[] oneDSamples;
		delete[] twoDSamples;
	}
	int RoundSize(int size) const {
		return RoundUpPow2(size);
	}
	bool GetNextSample(Sample *sample);
	
	static Sampler *CreateSampler(const ParamSet &params, const Film *film);
private:
	// LDSampler Private Data
	int xPos, yPos, pixelSamples;
	int samplePos;
	float *imageSamples, *lensSamples, *timeSamples;
	float **oneDSamples, **twoDSamples;
	int n1D, n2D;
};
