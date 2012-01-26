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
 
// lowdiscrepancy.cpp*
#include <fstream>

#include "lowdiscrepancy.h"
#include "error.h"
#include "scene.h"
#include "dynload.h"

using namespace lux;

LDSampler::LDData::LDData(const Sample &sample, int xPixelStart, int yPixelStart, u_int pixelSamples)
{
	xPos = xPixelStart - 1;
	yPos = yPixelStart;
	samplePos = pixelSamples - 1;

	// Allocate space for pixel's low-discrepancy sample tables
	imageSamples = new float[6 * pixelSamples];
	lensSamples = imageSamples + 2 * pixelSamples;
	timeSamples = imageSamples + 4 * pixelSamples;
	wavelengthsSamples = imageSamples + 5 * pixelSamples;
	oneDSamples = new float *[sample.n1D.size()];
	n1D = sample.n1D.size();
	for (u_int i = 0; i < sample.n1D.size(); ++i)
		oneDSamples[i] = new float[sample.n1D[i] * pixelSamples];
	twoDSamples = new float *[sample.n2D.size()];
	n2D = sample.n2D.size();
	for (u_int i = 0; i < sample.n2D.size(); ++i)
		twoDSamples[i] = new float[2 * sample.n2D[i] * pixelSamples];
	xDSamples = new float *[sample.nxD.size()];
	xD = new float *[sample.nxD.size()];
	nxD = sample.nxD.size();
	for (u_int i = 0; i < sample.nxD.size(); ++i) {
		xDSamples[i] = new float[sample.dxD[i] * sample.nxD[i] *
			pixelSamples];
		xD[i] = new float[sample.dxD[i]];
	}
}
LDSampler::LDData::~LDData()
{
	delete[] imageSamples;
	for (u_int i = 0; i < n1D; ++i)
		delete[] oneDSamples[i];
	for (u_int i = 0; i < n2D; ++i)
		delete[] twoDSamples[i];
	for (u_int i = 0; i < nxD; ++i)
		delete[] xDSamples[i];
	delete[] oneDSamples;
	delete[] twoDSamples;
	delete[] xDSamples;
}

// LDSampler Method Definitions
LDSampler::LDSampler(int xstart, int xend,
		int ystart, int yend, u_int ps, string pixelsampler,
		string *sampleFileName)
	: Sampler(xstart, xend, ystart, yend, RoundUpPow2(ps)) {
	// Initialize PixelSampler
	pixelSampler = MakePixelSampler(pixelsampler, xstart, xend, ystart, yend);

	totalPixels = pixelSampler->GetTotalPixels();

	// check/round pixelsamples to power of 2
	if (!IsPowerOf2(ps)) {
		LOG(LUX_WARNING,LUX_CONSISTENCY)<<"Pixel samples being rounded up to power of 2";
		pixelSamples = RoundUpPow2(ps);
	} else
		pixelSamples = ps;
	sampPixelPos = 0;

	if (sampleFileName)
		sampleFileWriter = new SampleFileWriter(*sampleFileName);
	else
		sampleFileWriter = NULL;
}

LDSampler::~LDSampler() {
	delete sampleFileWriter;
}

// return TotalPixels so scene shared thread increment knows total sample positions
u_int LDSampler::GetTotalSamplePos() {
	return totalPixels;
}

bool LDSampler::GetNextSample(Sample *sample) {
	LDData *data = (LDData *)(sample->samplerData);
	const RandomGenerator &rng(*(sample->rng));

	bool haveMoreSamples = true;
	++(data->samplePos);
	if (data->samplePos == pixelSamples) {
		u_int sampPixelPosToUse;
		// Move to the next pixel
		{
			fast_mutex::scoped_lock lock(sampPixelPosMutex);
			sampPixelPosToUse = sampPixelPos;
			sampPixelPos = (sampPixelPos + 1) % totalPixels;
		}

		// fetch next pixel from pixelsampler
		if(!pixelSampler->GetNextPixel(&data->xPos, &data->yPos, sampPixelPosToUse)) {
			// Dade - we are at a valid checkpoint where we can stop the
			// rendering. Check if we have enough samples per pixel in the film.
			if (film->enoughSamplesPerPixel) {
				// Dade - pixelSampler->renderingDone is shared among all rendering threads
				pixelSampler->renderingDone = true;
				haveMoreSamples = false;
			}
		} else
			haveMoreSamples = (!pixelSampler->renderingDone);

		data->samplePos = 0;
		// Generate low-discrepancy samples for pixel
		LDShuffleScrambled2D(rng, 1, pixelSamples, data->imageSamples);
		LDShuffleScrambled2D(rng, 1, pixelSamples, data->lensSamples);
		LDShuffleScrambled1D(rng, 1, pixelSamples, data->timeSamples);
		LDShuffleScrambled1D(rng, 1, pixelSamples, data->wavelengthsSamples);
		for (u_int i = 0; i < sample->n1D.size(); ++i)
			LDShuffleScrambled1D(rng, sample->n1D[i], pixelSamples,
				data->oneDSamples[i]);
		for (u_int i = 0; i < sample->n2D.size(); ++i)
			LDShuffleScrambled2D(rng, sample->n2D[i], pixelSamples,
				data->twoDSamples[i]);
		float *xDSamp;
		for (u_int i = 0; i < sample->nxD.size(); ++i) {
			xDSamp = data->xDSamples[i];
			for (u_int j = 0; j < sample->sxD[i].size(); ++j) {
				switch (sample->sxD[i][j]) {
				case 1: {
					LDShuffleScrambled1D(rng, sample->nxD[i],
						pixelSamples, xDSamp);
					xDSamp += sample->nxD[i] * pixelSamples;
					break; }
				case 2: {
					LDShuffleScrambled2D(rng, sample->nxD[i],
						pixelSamples, xDSamp);
					xDSamp += 2 * sample->nxD[i] * pixelSamples;
					break; }
				default:
					LOG(LUX_ERROR, LUX_LIMIT) << "Unsupported dimension";
					xDSamp += sample->sxD[i][j] * sample->nxD[i] * pixelSamples;
					break;
				}
			}
		}
	}

	// Copy low-discrepancy samples from tables
	sample->imageX = data->xPos + data->imageSamples[2 * data->samplePos];
	sample->imageY = data->yPos + data->imageSamples[2 * data->samplePos + 1];
	sample->lensU = data->lensSamples[2 * data->samplePos];
	sample->lensV = data->lensSamples[2 * data->samplePos + 1];
	sample->time = data->timeSamples[data->samplePos];
	sample->wavelengths = data->wavelengthsSamples[data->samplePos];

	return haveMoreSamples;
}

float LDSampler::GetOneD(const Sample &sample, u_int num, u_int pos)
{
	LDData *data = (LDData *)(sample.samplerData);
	return data->oneDSamples[num][sample.n1D[num] * data->samplePos + pos];
}

void LDSampler::GetTwoD(const Sample &sample, u_int num, u_int pos, float u[2])
{
	LDData *data = (LDData *)(sample.samplerData);
	const u_int startSamp = 2 * (sample.n2D[num] * data->samplePos + pos);
	u[0] = data->twoDSamples[num][startSamp];
	u[1] = data->twoDSamples[num][startSamp + 1];
}

float *LDSampler::GetLazyValues(const Sample &sample, u_int num, u_int pos)
{
	LDData *data = (LDData *)(sample.samplerData);
	float *sd = data->xD[num];
	float *xDSamp = data->xDSamples[num];
	u_int offset = 0;
	for (u_int i = 0; i < sample.sxD[num].size(); ++i) {
		if (sample.sxD[num][i] == 1) {
			sd[offset] = xDSamp[sample.nxD[num] * data->samplePos + pos];
		} else if (sample.sxD[num][i] == 2) {
			sd[offset] = xDSamp[2 * (sample.nxD[num] * data->samplePos + pos)];
			sd[offset + 1] = xDSamp[2 * (sample.nxD[num] * data->samplePos + pos) + 1];
		}
		xDSamp += sample.sxD[num][i] * sample.nxD[num] * pixelSamples;
		offset += sample.sxD[num][i];
	}
	return sd;
}

void LDSampler::AddSample(const Sample &sample)
{
	// Check if I have to write the sample information into the sample file
	if (sampleFileWriter) {
		boost::mutex::scoped_lock lock(sampleFileWriter->fileMutex);

		// Check if I have to write the header
		if (!sampleFileWriter->headerWritten) {
			WriteSampleInformationHeader(sample);
			sampleFileWriter->headerWritten = true;
		}

		// Write sample
		WriteSampleInformation(sample);
	}

	sample.contribBuffer->AddSampleCount(1.f);
	for (u_int i = 0; i < sample.contributions.size(); ++i)
		sample.contribBuffer->Add(sample.contributions[i], 1.f);
	sample.contributions.clear();
}

void LDSampler::WriteSampleInformationHeader(const Sample &sample) {
	LOG(LUX_DEBUG, LUX_NOERROR) << "Sample n1D size: " << sample.n1D.size();
	LOG(LUX_DEBUG, LUX_NOERROR) << "Sample n2D size: " << sample.n2D.size();
	LOG(LUX_DEBUG, LUX_NOERROR) << "Sample nxD size: " << sample.nxD.size();

	// 4 for lensU, lensV, time, wavelengths
	u_int count = 4;
	for (size_t i = 0; i < sample.n1D.size(); ++i)
		count += sample.n1D[i];
	for (size_t i = 0; i < sample.n2D.size(); ++i)
		count += 2 * sample.n2D[i];

	for (size_t i = 0; i < sample.nxD.size(); ++i) { // Only the first 2 path bounces
		count += min<u_int>(sample.nxD[i], 2) * sample.dxD[i];
		LOG(LUX_DEBUG, LUX_NOERROR) << "Sample nxD[" << i << "] size: " << sample.nxD[i] <<
				" (capped to: " << min<u_int>(sample.nxD[i], 2) << ")";
		LOG(LUX_DEBUG, LUX_NOERROR) << "Sample sxD[" << i << "] size: " << sample.dxD[i];
	}

	sampleFileWriter->WriteHeader(count);
}

void LDSampler::WriteSampleInformation(const Sample &sample) {
	/*if (sample.contributions.size() == 0)
		return;*/

	// Write screen position
	sampleFileWriter->Write(&sample.imageX, sizeof(float));
	sampleFileWriter->Write(&sample.imageY, sizeof(float));

	// Write random parameters
	sampleFileWriter->Write(&sample.lensU, sizeof(float));
	sampleFileWriter->Write(&sample.lensV, sizeof(float));
	sampleFileWriter->Write(&sample.time, sizeof(float));
	sampleFileWriter->Write(&sample.wavelengths, sizeof(float));

	for (size_t i = 0; i < sample.n1D.size(); ++i) {
		for (size_t j = 0; j < sample.n1D[i]; ++j) {
			float v = GetOneD(sample, i, j);
			sampleFileWriter->Write(&v, sizeof(float));
		}
	}

	for (size_t i = 0; i < sample.n2D.size(); ++i) {
		for (size_t j = 0; j < sample.n2D.size(); ++j) {
			float uv[2];
			GetTwoD(sample, i, j, uv);
			sampleFileWriter->Write(&uv, sizeof(float[2]));
		}
	}

	for (size_t i = 0; i < sample.nxD.size(); ++i) {
		for (size_t j = 0; j < min<u_int>(sample.nxD[i], 2); ++j) { // Only the first 2 path bounces
			float *data = GetLazyValues(sample, i, j);
			sampleFileWriter->Write(&data, sizeof(float) * sample.dxD[i]);
		}
	}

	// Write sample color

	// TODO: I assume all Contributions have the same imageX, imageY, etc.
	// TODO: this will work only with some integrator (i.e. path, not with bidir)

	XYZColor c;
	for (u_int i = 0; i < sample.contributions.size(); ++i)
		c += sample.contributions[i].color;
	sampleFileWriter->Write(c.c, sizeof(float[3]));

	// Write scene features
	sampleFileWriter->Write(sample.pathInfo, sizeof(SamplePathInfo));
}

Sampler* LDSampler::CreateSampler(const ParamSet &params, const Film *film) {
	// Initialize common sampler parameters
	int xstart, xend, ystart, yend;
	film->GetSampleExtent(&xstart, &xend, &ystart, &yend);
	string pixelsampler = params.FindOneString("pixelsampler", "vegas");
	int nsamp = params.FindOneInt("pixelsamples", 4);

	bool writeSampleFile = params.FindOneBool("write_sample_file", false);
	string sampleFileName = params.FindOneString("sample_filename", "samples.spl");

	return new LDSampler(xstart, xend, ystart, yend, max(nsamp, 0), pixelsampler,
			writeSampleFile ? &sampleFileName : NULL);
}

static DynamicLoader::RegisterSampler<LDSampler> r("lowdiscrepancy");
