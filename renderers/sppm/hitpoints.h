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
#include "reflection/bxdf.h"

namespace lux
{

struct PhotonData
{
	Point p;
	Vector wi;
	SWCSpectrum alpha;
	uint lightGroup;
};


//------------------------------------------------------------------------------
// Eye path hit points
//------------------------------------------------------------------------------

class HitPointEyePass {
public:
	// Eye path data
	SWCSpectrum pathThroughput; // Used only for SURFACE type

	BSDF *bsdf;

	float alpha;
	float distance;

	Vector wo;
};

/*
Probabilistic approach TODO:

a) remove statistics if unneeded
b) align data structure in memory
*/

class HitPoint {
public:
	HitPointEyePass eyePass;

	// photons statistics
private:
	unsigned long long photonCount;
	u_int accumPhotonCount;
public:
	float accumPhotonRadius2;
	float imageX, imageY;

	Point GetPosition() const
	{
		return eyePass.bsdf->dgShading.p;
	}

	void SetConstant()
	{
		eyePass.bsdf = NULL;
	}
	void SetSurface()
	{
		// the fact to set something else that NULL in bsdf field set the
		// hitpoint as a surface
	}

	bool IsSurface() const
	{
		return eyePass.bsdf != NULL;
	}

	void IncPhoton()
	{
		osAtomicInc(&accumPhotonCount);
	}
	void InitStats()
	{
		photonCount = 0;
		accumPhotonCount = 0;
	}

	u_int GetPhotonCount() const
	{
		return photonCount;
	}
	void DoRadiusReduction(float const alpha, float const pass, bool useproba)
	{
		if(useproba)
		{
			accumPhotonRadius2 *= (pass + alpha) / (pass + 1.0f);
		}
		else if (accumPhotonCount > 0) {
			/*
			TODO: startK disable because incorrect
			u_int k = renderer->sppmi->photonStartK;
			if(k > 0 && photonCount == 0)
			{
				// This heuristic is triggered by hitpoint on the first pass
				// which gather photons.

				// If the pass gather more than k photons, and with the
				// assumption that photons are uniformly spread on the
				// hitpoint, we reduce the search radius.

				if(accumPhotonCount > k)
				{
					// We now suppose that we only gather k photons, and
					// reduce the radius accordingly.
					// Note: the flux is already normalised, so it does
					// not depends of the radius, no need to change it.
					accumPhotonRadius2 *= ((float) k) / ((float) accumPhotonCount);
					accumPhotonCount = k;
				}
			}
			*/
			const unsigned long long pcount = photonCount + accumPhotonCount;

			// Compute g and do radius reduction
			const float g = alpha * pcount / (photonCount * alpha + accumPhotonCount);

			// Radius reduction
			accumPhotonRadius2 *= g;
		}

		photonCount += accumPhotonCount;
		accumPhotonCount = 0;
	}
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
		boost::mutex::scoped_lock lock(initMutex);
		if (halton.size() == 0) {
			for (u_int i = 0; i < nPixels; ++i) {
				const_cast<vector<PermutedHalton *> &>(halton).push_back(new PermutedHalton(size + 4, *(sample->rng)));
				const_cast<vector<float> &>(haltonOffset).push_back(sample->rng->floatValue());
			}
		}
		lock.unlock();
		sample->samplerData = new HaltonEyeSamplerData(*sample, size);
	}
	virtual void FreeSample(Sample *sample) const {
		HaltonEyeSamplerData *data = static_cast<HaltonEyeSamplerData *>(sample->samplerData);
		delete data;
	}
	virtual bool GetNextSample(Sample *sample) {
		HaltonEyeSamplerData *data = static_cast<HaltonEyeSamplerData *>(sample->samplerData);
		halton[data->index]->Sample(data->pathCount,
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
	mutable boost::mutex initMutex;
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

	const BBox &GetBBox() const {
		return hitPointBBox;
	}

	float GetMaxPhotonRadius2() const { return maxHitPointRadius2; }

	void UpdatePointsInformation();
	const u_int GetPassCount() const { return currentPass; }
	void IncPass() {
		++currentPass;
		if (currentPass < wavelengthStratPasses) {
			const u_int i = currentPass + 1; // use 1-based counting
			const u_int Nsegments = 1 << Floor2UInt(Log2(i));
			const u_int j = (2*Nsegments - 1) - i; // reverse order seems better
			wavelengthSample = static_cast<float>(2*j + 1) / (2*Nsegments);
		} else
			wavelengthSample = Halton(currentPass - wavelengthStratPasses, wavelengthSampleScramble);
		timeSample = Halton(currentPass, timeSampleScramble);
	}

	const float GetWavelengthSample() { return wavelengthSample; }
	const float GetTimeSample() { return timeSample; }

	void AddFlux(Sample &sample, const PhotonData &photon)
	{
		lookUpAccel->AddFlux(sample, photon);
	}
	void AccumulateFlux(const u_int index, const u_int count);
	void SetHitPoints(Sample &sample, RandomGenerator *rng, const u_int index, const u_int count);

	void RefreshAccel(const u_int index, const u_int count, boost::barrier &barrier) {
		lookUpAccel->Refresh(index, count, barrier);
	}

private:
	void TraceEyePath(HitPoint *hp, const Sample &sample);

	SPPMRenderer *renderer;
public:
	Sampler *eyeSampler;

	BxDFType store_component, bounce_component;

private:
	// Hit points information
	float initialPhotonRadius;

	BBox hitPointBBox;
	float maxHitPointRadius2;
	std::vector<HitPoint> *hitPoints;
	HitPointsLookUpAccel *lookUpAccel;

	u_int currentPass;

	// Only a single set of wavelengths is sampled for each pass
	float wavelengthSample, timeSample;
	u_int wavelengthSampleScramble, timeSampleScramble;
	u_int wavelengthStratPasses;
};

}//namespace lux

#endif	/* LUX_HITPOINTS_H */
