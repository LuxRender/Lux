/***************************************************************************
 *   Copyright (C) 1998-2010 by authors (see AUTHORS.txt )                 *
 *                                                                         *
 *   This file is part of LuxRays.                                         *
 *                                                                         *
 *   LuxRays is free software; you can redistribute it and/or modify       *
 *   it under the terms of the GNU General Public License as published by  *
 *   the Free Software Foundation; either version 3 of the License, or     *
 *   (at your option) any later version.                                   *
 *                                                                         *
 *   LuxRays is distributed in the hope that it will be useful,            *
 *   but WITHOUT ANY WARRANTY; without even the implied warranty of        *
 *   MERCHANTABILITY or FITNESS FOR A PARTICULAR PURPOSE.  See the         *
 *   GNU General Public License for more details.                          *
 *                                                                         *
 *   You should have received a copy of the GNU General Public License     *
 *   along with this program.  If not, see <http://www.gnu.org/licenses/>. *
 *                                                                         *
 *   LuxRays website: http://www.luxrender.net                             *
 ***************************************************************************/

#ifndef LUX_HITPOINTS_H
#define	LUX_HITPOINTS_H

#include "lux.h"
#include "scene.h"
#include "sampling.h"

#include "lookupaccel.h"

namespace lux
{

//------------------------------------------------------------------------------
// Eye path hit points
//------------------------------------------------------------------------------

enum HitPointType {
	SURFACE, CONSTANT_COLOR
};

class HitPointLightGroup {
public:
	unsigned long long photonCount;
	XYZColor reflectedFlux;

	u_int accumPhotonCount;
	XYZColor accumReflectedFlux;
	XYZColor accumRadiance;

	// Debug code
	// Radiance Sum Square Error, used to compute Mean Square Error
	//float radianceSSE;
};

class HitPointEyePass {
public:
	HitPointType type;

	// Eye path data
	SWCSpectrum pathThroughput; // Used only for SURFACE type
	float alpha;
	float distance;

	// Used for SURFACE type
	Point position;
	Vector wo;
	Normal bsdfNG;
};

class HitPoint {
public:
	// Used to render eye pass n+1 while doing photon pass n
	HitPointEyePass eyePass[2];

	vector<HitPointLightGroup> lightGroupData;
	
	float accumPhotonRadius2;
};

class SPPMRenderer;

//------------------------------------------------------------------------------
// Halton Eye Sampler
//------------------------------------------------------------------------------

class HaltonEyeSampler : public Sampler {
public:
	class HaltonEyeSamplerData {
	public:
		HaltonEyeSamplerData(const Sample &sample, u_int sz) :
			size(sz), index(0), pathCount(0) {
			values = new float *[max<u_int>(1U, sample.n1D.size() +
				sample.n2D.size() + sample.nxD.size())];
			u_int n = 0;
			for (u_int i = 0; i < sample.n1D.size(); ++i)
				n += sample.n1D[i];
			for (u_int i = 0; i < sample.n2D.size(); ++i)
				n += 2 * sample.n2D[i];
			for (u_int i = 0; i < sample.nxD.size(); ++i)
				n += sample.dxD[i];
			// Reserve space for screen and lens samples
			float *buffer = new float[n + 4] + 4;
			values[0] = buffer;	// in case n == 0
			u_int offset = 0;
			for (u_int i = 0; i < sample.n1D.size(); ++i) {
				values[offset + i] = buffer;
				buffer += sample.n1D[i];
			}
			offset += sample.n1D.size();
			for (u_int i = 0; i < sample.n2D.size(); ++i) {
				values[offset + i] = buffer;
				buffer += 2 * sample.n2D[i];
			}
			offset += sample.n2D.size();
			for (u_int i = 0; i < sample.nxD.size(); ++i) {
				values[offset + i] = buffer;
				buffer += sample.dxD[i];
			}
		}
		~HaltonEyeSamplerData() {
			// Don't forget screen and lens samples space
			delete[] (values[0] - 4);
			delete[] values;
		}
		u_int size;
		u_int index;
		u_int pathCount;
		float **values;
	};
	HaltonEyeSampler(int x0, int x1, int y0, int y1, const string &ps);
	virtual ~HaltonEyeSampler() { }
	virtual u_int GetTotalSamplePos() { return nPixels; }
	virtual u_int RoundSize(u_int sz) const { return sz; }

	virtual void InitSample(Sample *sample) const {
		sample->sampler = const_cast<HaltonEyeSampler *>(this);
		u_int size = 0;
		for (u_int i = 0; i < sample->n1D.size(); ++i)
			size += sample->n1D[i];
		for (u_int i = 0; i < sample->n2D.size(); ++i)
			size += 2 * sample->n2D[i];
		sample->samplerData = new HaltonEyeSamplerData(*sample, size);
	}
	virtual void FreeSample(Sample *sample) const {
		HaltonEyeSamplerData *data = static_cast<HaltonEyeSamplerData *>(sample->samplerData);
		delete data;
	}
	void InitHalton(Sample *sample)
	{
		HaltonEyeSamplerData *data = static_cast<HaltonEyeSamplerData *>(sample->samplerData);
		u_int size = 0;
		for (u_int i = 0; i < sample->n1D.size(); ++i)
			size += sample->n1D[i];
		for (u_int i = 0; i < sample->n2D.size(); ++i)
			size += 2 * sample->n2D[i];

		halton[data->index] = new PermutedHalton(size + 4, *(sample->rng));
		haltonOffset[data->index] = sample->rng->floatValue();
	}
	void FreeHalton(Sample *sample)
	{
		HaltonEyeSamplerData *data = static_cast<HaltonEyeSamplerData *>(sample->samplerData);
		delete halton[data->index];
		halton[data->index] = NULL;
	}
	virtual bool GetNextSample(Sample *sample) {
		HaltonEyeSamplerData *data = static_cast<HaltonEyeSamplerData *>(sample->samplerData);
		PermutedHalton *h = halton[data->index];
		h->Sample(data->pathCount,
			data->values[0] - 4);
		int x, y;
		pixelSampler->GetNextPixel(&x, &y, data->index);

		// Add an offset to the samples to avoid to start with 0.f values
		for (int i = -4; i < static_cast<int>(data->size); ++i) {
			const float v = data->values[0][i] + haltonOffset[data->index];
			data->values[0][i] = (v >= 1.f) ? (v - 1.f) : v;
		}
		sample->imageX = x + data->values[0][-4];
		sample->imageY = y + data->values[0][-3];
		sample->lensU = data->values[0][-2];
		sample->lensV = data->values[0][-1];
		return true;
	}
	virtual float GetOneD(const Sample &sample, u_int num, u_int pos) {
		HaltonEyeSamplerData *data = static_cast<HaltonEyeSamplerData *>(sample.samplerData);
		return data->values[num][pos];
	}
	virtual void GetTwoD(const Sample &sample, u_int num, u_int pos,
		float u[2]) {
		HaltonEyeSamplerData *data = static_cast<HaltonEyeSamplerData *>(sample.samplerData);
		u[0] = data->values[sample.n1D.size() + num][2 * pos];
		u[1] = data->values[sample.n1D.size() + num][2 * pos + 1];
	}
	virtual float *GetLazyValues(const Sample &sample, u_int num, u_int pos) {
		HaltonEyeSamplerData *data = static_cast<HaltonEyeSamplerData *>(sample.samplerData);
		float *result = data->values[sample.n1D.size() + sample.n2D.size() + num];
		for (u_int i = 0; i < sample.dxD[num]; ++i)
			result[i] = sample.rng->floatValue();
		return result;
	}
//	virtual void AddSample(const Sample &sample);
	PixelSampler *pixelSampler;
private:
	u_int nPixels;
	mutable u_int curIndex;
	vector<PermutedHalton *> halton;
	vector<float> haltonOffset;
};

class HitPoints {
public:
	HitPoints(SPPMRenderer *engine, RandomGenerator *rng);
	~HitPoints();

	void Init();

	const double GetPhotonHitEfficency();

	HitPoint *GetHitPoint(const u_int index) {
		return &(*hitPoints)[index];
	}

	const u_int GetSize() const {
		return hitPoints->size();
	}

	const BBox &GetBBox(const u_int passIndex) const {
		return hitPointBBox[passIndex];
	}

	float GetMaxPhotonRadius2(const u_int passIndex) const { return maxHitPointRadius2[passIndex]; }

	void UpdatePointsInformation();
	const u_int GetEyePassCount() const { return currentEyePass; }
	const u_int GetPhotonPassCount() const { return currentPhotonPass; }
	void IncEyePass() {
		++currentEyePass;
		eyePassWavelengthSample = Halton(currentEyePass, wavelengthSampleScramble);
		eyePassTimeSample = Halton(currentEyePass, timeSampleScramble);
	}
	void IncPhotonPass() {
		++currentPhotonPass;
		photonPassWavelengthSample = Halton(currentPhotonPass, wavelengthSampleScramble);
		photonPassTimeSample = Halton(currentPhotonPass, timeSampleScramble);
	}

	const float GetPassWavelengthSample() { return eyePassWavelengthSample; }
	const float GetPassTimeSample() { return eyePassTimeSample; }
	const float GetPhotonPassWavelengthSample() { return photonPassWavelengthSample; }
	const float GetPhotonPassTimeSample() { return photonPassTimeSample; }

	void AddFlux(const Point &hitPoint, const BSDF &bsdf, const Vector &wi,
		const SpectrumWavelengths &sw, const SWCSpectrum &photonFlux, const u_int lightGroup) {
		const u_int passIndex = currentPhotonPass % 2;
		lookUpAccel[passIndex]->AddFlux(hitPoint, passIndex, bsdf, wi, sw, photonFlux, lightGroup);
	}
	void AddFlux(SplatList *splatList, const Point &hitPoint, const BSDF &bsdf, const Vector &wi,
		const SpectrumWavelengths &sw, const SWCSpectrum &photonFlux, const u_int lightGroup) {
		const u_int passIndex = currentPhotonPass % 2;
		return lookUpAccel[passIndex]->AddFlux(splatList, hitPoint, passIndex, bsdf, wi, sw, photonFlux, lightGroup);
	}

	void SplatFlux(SplatList *splatList) {
		const u_int passIndex = currentPhotonPass % 2;
		return lookUpAccel[passIndex]->Splat(splatList);
	}
	void AccumulateFlux(const float fluxScale, const u_int index, const u_int count);
	void SetHitPoints(RandomGenerator *rng,	const u_int index, const u_int count);

	void RefreshAccelMutex() {
		const u_int passIndex = currentEyePass % 2;
		lookUpAccel[passIndex]->RefreshMutex(passIndex);
	}

	void RefreshAccelParallel(const u_int index, const u_int count) {
		const u_int passIndex = currentEyePass % 2;
		lookUpAccel[passIndex]->RefreshParallel(passIndex, index, count);
	}

	void UpdateFilm(const unsigned long long totalPhotons);

private:
	void TraceEyePath(HitPoint *hp, const Sample &sample);

	SPPMRenderer *renderer;
	Sampler *eyeSampler;

	// Hit points information
	float initialPhotonRadius;
	// Used to render eye pass n+1 while doing photon pass n
	BBox hitPointBBox[2];
	float maxHitPointRadius2[2];
	std::vector<HitPoint> *hitPoints;
	HitPointsLookUpAccel *lookUpAccel[2];

	u_int currentEyePass;
	u_int currentPhotonPass;

	// Only a single set of wavelengths is sampled for each pass
	float eyePassWavelengthSample, photonPassWavelengthSample;
	float eyePassTimeSample, photonPassTimeSample;
	u_int wavelengthSampleScramble, timeSampleScramble;
};

}//namespace lux

#endif	/* LUX_HITPOINTS_H */
