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

// stratified.cpp*
#include "stratified.h"
// Lux (copy) constructor
StratifiedSampler* StratifiedSampler::clone() const
 {
   return new StratifiedSampler(*this);
 }
// StratifiedSampler Method Definitions
StratifiedSampler::StratifiedSampler(int xstart, int xend,
		int ystart, int yend, int xs, int ys, bool jitter)
	: Sampler(xstart, xend, ystart, yend, xs * ys) {
	jitterSamples = jitter;
	xPos = xPixelStart;
	yPos = yPixelStart;
	xPixelSamples = xs;
	yPixelSamples = ys;
	// Allocate storage for a pixel's worth of stratified samples
	imageSamples = (float *)AllocAligned(5 * xPixelSamples *
		yPixelSamples * sizeof(float));
	lensSamples =
		imageSamples + 2 * xPixelSamples * yPixelSamples;
	timeSamples =
		lensSamples + 2 * xPixelSamples * yPixelSamples;
	// Generate stratified camera samples for (_xPos_,_yPos_)
	StratifiedSample2D(imageSamples,
		xPixelSamples, yPixelSamples,
		jitterSamples);
	StratifiedSample2D(lensSamples,
		xPixelSamples, yPixelSamples,
		jitterSamples);
	StratifiedSample1D(timeSamples, xPixelSamples*yPixelSamples,
		jitterSamples);
	// Shift stratified image samples to pixel coordinates
	for (int o = 0;
	     o < 2 * xPixelSamples * yPixelSamples;
		 o += 2) {
		imageSamples[o]   += xPos;
		imageSamples[o+1] += yPos;
	}
	// Decorrelate sample dimensions
	Shuffle(lensSamples, xPixelSamples*yPixelSamples, 2);
	Shuffle(timeSamples, xPixelSamples*yPixelSamples, 1);
	samplePos = 0;
}
void StratifiedSampler::setSeed( u_int s )
{
    //fs_scramble = s;
}
bool StratifiedSampler::GetNextSample(Sample *sample) {
	// Compute new set of samples if needed for next pixel
	if (samplePos == xPixelSamples * yPixelSamples) {
		// Advance to next pixel for stratified sampling
		if (++xPos == xPixelEnd) {
			xPos = xPixelStart;
			++yPos;
		}
		if (yPos == yPixelEnd)
			return false;
		// Generate stratified camera samples for (_xPos_,_yPos_)
		StratifiedSample2D(imageSamples,
			xPixelSamples, yPixelSamples,
			jitterSamples);
		StratifiedSample2D(lensSamples,
			xPixelSamples, yPixelSamples,
			jitterSamples);
		StratifiedSample1D(timeSamples, xPixelSamples*yPixelSamples,
			jitterSamples);
		// Shift stratified image samples to pixel coordinates
		for (int o = 0;
		     o < 2 * xPixelSamples * yPixelSamples;
			 o += 2) {
			imageSamples[o]   += xPos;
			imageSamples[o+1] += yPos;
		}
		// Decorrelate sample dimensions
		Shuffle(lensSamples, xPixelSamples*yPixelSamples, 2);
		Shuffle(timeSamples, xPixelSamples*yPixelSamples, 1);
		samplePos = 0;
	}
	// Return next _StratifiedSampler_ sample point
	sample->imageX = imageSamples[2*samplePos];
	sample->imageY = imageSamples[2*samplePos+1];
	sample->lensU = lensSamples[2*samplePos];
	sample->lensV = lensSamples[2*samplePos+1];
	sample->time = timeSamples[samplePos];
	// Generate stratified samples for integrators
	for (u_int i = 0; i < sample->n1D.size(); ++i)
		LatinHypercube(sample->oneD[i], sample->n1D[i], 1);
	for (u_int i = 0; i < sample->n2D.size(); ++i)
		LatinHypercube(sample->twoD[i], sample->n2D[i], 2);
	++samplePos;
	return true;
}
Sampler* StratifiedSampler::CreateSampler(const ParamSet &params, const Film *film) {
	bool jitter = params.FindOneBool("jitter", true);
	// Initialize common sampler parameters
	int xstart, xend, ystart, yend;
	film->GetSampleExtent(&xstart, &xend, &ystart, &yend);
	int xsamp = params.FindOneInt("xsamples", 2);
	int ysamp = params.FindOneInt("ysamples", 2);
	return new StratifiedSampler(xstart, xend, ystart, yend, xsamp, ysamp,
		jitter);
}
