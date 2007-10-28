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

// metropolis.cpp*
#include "metropolis.h"

float MetroSample::mutate (const float x) const {
	static const float s1 = 1/1024., s2 = 1/16.;
	float dx = s2 * exp(-log(s2/s1) * RandomFloat());
	if (RandomFloat() < 0.5) {
		float x1 = x + dx;
		return (x1 > 1) ? x1 - 1 : x1;
	} else {
		float x1 = x - dx;
		return (x1 < 0) ? x1 + 1 : x1;
	}
}

float MetroSample::value (const int i, float defval) const {
	if (i >= (int) values.size()) {
		int oldsize = values.size();
		int newsize = i + 1;
		values.resize(newsize);
		modify.resize(newsize);
		for (int j = oldsize; j < newsize; j++) {
			values[j] = defval;
			modify[j] = time;
		}
	}
	for (; modify[i] < time; modify[i]++)
		values[i] = mutate(values[i]);
	return values[i];
}

MetroSample MetroSample::next () const {
	MetroSample newsamp(*this);
	newsamp.time = time + 1;
	return newsamp;
}

void Metropolis::GetNext(float& bs1, float& bs2, float& bcs, int pathLength)
{
	float os1 = bs1, os2 = bs2, ocs = bcs;
	bs1 = newsamp.value(5+pathLength, os1);
	bs2 = newsamp.value(5+pathLength+1, os2);
	bcs = newsamp.value(5+pathLength+2, ocs);
}

bool Metropolis::GetNextSample(Sampler *sampler, Sample *sample, u_int *use_pos)
{
	large = (RandomFloat() < pLarge);
	if(large) {
		// large mutation
		newsamp = MetroSample();

		// fetch samples from sampler
		if(!sampler->GetNextSample(sample, use_pos))
			return false;

		newsamp.value(0, sample->imageX / xRes);
		newsamp.value(1, sample->imageY / yRes);
		newsamp.value(2, sample->lensU);
		newsamp.value(3, sample->lensV);
		newsamp.value(4, sample->time);
	} else {
		// small mutation
		newsamp = msamp.next();

		// mutate current sample
		sample->imageX = newsamp.value(0, 0.) * xRes;
		sample->imageY = newsamp.value(1, 0.) * yRes;
		sample->lensU = newsamp.value(2, 0.);
		sample->lensV = newsamp.value(3, 0.);
		sample->time = newsamp.value(4, 0.);
	}

	newsamp.i = 5;
    return true;
}

void Metropolis::AddSample(const Sample &sample, const Ray &ray,
			   const Spectrum &newL, float alpha, Film *film)
{
	if (large) {
		sum += newL.y();
		nmetro++;
	}
	float accprob = __min(1, newL.y()/L.y());

	if (L.y() > 0.f && L != Spectrum(0.f))
		film->AddSample(msamp.value(0, 0.)*xRes, 
			msamp.value(1, 0.)*yRes, L*(1/L.y())*(1-accprob), alpha);

	if (newL.y() > 0.f && newL != Spectrum(0.f))
		film->AddSample(newsamp.value(0, 0.)*xRes, 
			newsamp.value(1, 0.)*yRes, newL*(1/newL.y())*accprob, alpha);

	if (RandomFloat() < accprob || consec_rejects > maxReject) {
		msamp = newsamp;
		L = newL;
		consec_rejects = 0;
	} else {
		consec_rejects++;
	}
}

