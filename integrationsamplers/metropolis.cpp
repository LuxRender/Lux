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

// initial metropolis light transport sample integrator
// by radiance

// TODO: add scaling of output image samples

// metropolis.cpp*
#include "metropolis.h"

// mutate a value in the metrosample vector
float MetroSample::mutate (const float x) const {
	static const float s1 = 1/1024., s2 = 1/16.;
	float dx = s2 * exp(-log(s2/s1) * lux::random::floatValue());
	if (lux::random::floatValue() < 0.5) {
		float x1 = x + dx;
		return (x1 > 1) ? x1 - 1 : x1;
	} else {
		float x1 = x - dx;
		return (x1 < 0) ? x1 + 1 : x1;
	}
}

// retrieve a mutated value from the vector,
// or add one with defval or a random one with defval = -1.f
float MetroSample::value (const int i, float defval) const {
	if (i >= (int) values.size()) {
		int oldsize = values.size();
		int newsize = i + 1;
		values.resize(newsize);
		modify.resize(newsize);
		for (int j = oldsize; j < newsize; j++) {
			if(defval == -1.)
				values[j] = lux::random::floatValue();
			else
			    values[j] = defval;
			modify[j] = time;
		}
	}
	for (; modify[i] < time; modify[i]++)
		values[i] = mutate(values[i]);
	return values[i];
}

// reset the sample vector for the next mutated set
MetroSample MetroSample::next () const {
	MetroSample newsamp(*this);
	newsamp.time = time + 1;
	return newsamp;
}

// interface for retrieving from integrators
void Metropolis::GetNext(float& bs1, float& bs2, float& bcs, int pathLength)
{
	bs1 = newsamp.value(5+pathLength, bs1);
	bs2 = newsamp.value(5+pathLength+1, bs2);
	bcs = newsamp.value(5+pathLength+2, bcs);
}

// interface for new ray/samples from scene
bool Metropolis::GetNextSample(Sampler *sampler, Sample *sample, u_int *use_pos)
{
	large = (lux::random::floatValue() < pLarge);
	if(large) {
		// *** large mutation ***
		newsamp = MetroSample();

		// fetch samples from sampler
		if(!sampler->GetNextSample(sample, use_pos))
			return false;

		// store in first 5 positions
		newsamp.value(0, sample->imageX / xRes);
		newsamp.value(1, sample->imageY / yRes);
		newsamp.value(2, sample->lensU);
		newsamp.value(3, sample->lensV);
		newsamp.value(4, sample->time);
	} else {
		// *** small mutation ***
		newsamp = msamp.next();

		// mutate current sample
		sample->imageX = newsamp.value(0, -1.) * xRes;
		sample->imageY = newsamp.value(1, -1.) * yRes;
		sample->lensU = newsamp.value(2, -1.);
		sample->lensV = newsamp.value(3, -1.);
		sample->time = newsamp.value(4, -1.);
	}

	// set increment
	newsamp.i = 5;

    return true;
}

// interface for adding/accepting a new image sample.
void Metropolis::AddSample(const Sample &sample, const Ray &ray,
			   const Spectrum &newL, float alpha, Film *film)
{
	// calculate accept probability from old and new image sample
	float accprob = min(1.0f, newL.y()/L.y());

	// add old sample
	if (L.y() > 0.f && L != Spectrum(0.f))
		film->AddSample(msamp.value(0, -1.)*xRes, 
			msamp.value(1, -1.)*yRes, L*(1/L.y())*(1-accprob), alpha);

	// add new sample
	if (newL.y() > 0.f && newL != Spectrum(0.f))
		film->AddSample(newsamp.value(0, -1.)*xRes, 
			newsamp.value(1, -1.)*yRes, newL*(1/newL.y())*accprob, alpha);

	// try or force accepting of the new sample
	if (lux::random::floatValue() < accprob || consec_rejects > maxReject) {
		msamp = newsamp;
		L = newL;
		consec_rejects = 0;
	} else {
		consec_rejects++;
	}
}

