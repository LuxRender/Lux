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

// bestcandidate.cpp*
#include "sampling.h"
#include "paramset.h"
#include "film.h"
// BestCandidate Sampling Constants
#define SQRT_SAMPLE_TABLE_SIZE 64
#define SAMPLE_TABLE_SIZE (SQRT_SAMPLE_TABLE_SIZE * \
                           SQRT_SAMPLE_TABLE_SIZE)
// BestCandidateSampler Declarations
class BestCandidateSampler : public Sampler {
public:
	// BestCandidateSampler Public Methods
	BestCandidateSampler(int xstart, int xend,
	                     int ystart, int yend,
						 int pixelsamples);
	~BestCandidateSampler() {
		delete[] strat2D;
		// so we leak on the individual elements of these arrays.  so it goes...
		delete[] oneDSamples;
		delete[] twoDSamples;
	}
	void setSeed( u_int s );
	int RoundSize(int size) const {
		int root = Ceil2Int(sqrtf((float)size - .5f));
		return root*root;
	}
	bool GetNextSample(Sample *sample, u_int *use_pos);
	virtual BestCandidateSampler* clone() const; // Lux (copy) constructor for multithreading

	static Sampler *CreateSampler(const ParamSet &params, const Film *film);
private:
	// BestCandidateSampler Private Data
	int tableOffset;
	float xTableCorner, yTableCorner, tableWidth;
	static const float sampleTable[SAMPLE_TABLE_SIZE][5];
	float **oneDSamples, **twoDSamples;
	int *strat2D;
	float sampleOffsets[3];
};
