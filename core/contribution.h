/***************************************************************************
 *   Copyright (C) 1998-2008 by authors (see AUTHORS.txt )                 *
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

#ifndef LUX_CONTRIBUTION_H
#define LUX_CONTRIBUTION_H
// contribution.h*
#include "lux.h"
#include "film.h"
#include "color.h"

#include <boost/thread/recursive_mutex.hpp>

namespace lux
{

// Size of a contribution buffer
// 4096 seems best.
#define CONTRIB_BUF_SIZE 4096

// Minimum number of buffers to keep alive/reuse
// In practice usually up to twice this amount stays allocated
#define CONTRIB_BUF_KEEPALIVE 8

// Switch on to get feedback in the log about allocation
#define CONTRIB_DEBUG false

class Contribution {
public:
	Contribution(float x=0.f, float y=0.f, const XYZColor &c=0.f, float a=0.f,
		float v=0.f, int b=0, int g=0) :
		imageX(x), imageY(y), color(c), alpha(a), variance(v),
		buffer(b), bufferGroup(g) { }

	float imageX, imageY;
	XYZColor color;
	float alpha, variance;
	int buffer, bufferGroup;
};

class ContributionBuffer {
public:
	ContributionBuffer() : pos(0), sampleCount(0.f) {
		contribs = (Contribution *)AllocAligned(CONTRIB_BUF_SIZE * sizeof(Contribution));
	}

	~ContributionBuffer() {
		FreeAligned(contribs);
	}

	bool Add(Contribution* c, float weight=1.f) {
		if(pos == CONTRIB_BUF_SIZE)
			return false;

		contribs[pos] = *c;
		if(weight != 1.f)
			contribs[pos].color *= weight;

		pos++;

		return true;
	}

	void AddSampleCount(float c) {
		sampleCount += c;
	}

	void Splat(Film *film) {
		for (short int i=0; i<pos; i++)
			film->AddSample(&contribs[i]);
		film->AddSampleCount(sampleCount);
	}

	void Reset() { pos = 0; sampleCount = 0.f; }

private:
	unsigned short int pos;
	float sampleCount;
	Contribution *contribs;
};

class ContributionPool {
public:

	ContributionPool() : total(0), splatting(false) { }

	void SetFilm(Film *f) { film = f; }

	ContributionBuffer* Get() { // Do not use Get() directly, use Next() below instead, it's thread-safe.
		u_int freepos = CFree.size();
		if(freepos == 0) {
			ContributionBuffer *cnew = new ContributionBuffer();
			total++;

			if(CONTRIB_DEBUG) {
				std::stringstream ss;
				ss << "Allocating Contribution Buffer - Nr of buffers in pool: "
					<< total << " - Total mem used: " << total * CONTRIB_BUF_SIZE * sizeof(Contribution) << ".";
				luxError(LUX_NOERROR, LUX_INFO, ss.str().c_str());
			}

			return cnew;
		}
		ContributionBuffer *cold = CFree[freepos-1];
		CFree.pop_back();
		cold->Reset();
		return cold;
	}

	void End(ContributionBuffer *c) {
		boost::recursive_mutex::scoped_lock PoolAction(PoolMutex);
		if(c) CFull.push_back(c);
	}

	ContributionBuffer* Next(ContributionBuffer *c) {
		bool doSplat = false;

		boost::recursive_mutex::scoped_lock PoolAction(PoolMutex);

		if(c) CFull.push_back(c);
		ContributionBuffer *cnew = Get();

		if(CFull.size() > CONTRIB_BUF_KEEPALIVE && splatting == false) {
			splatting = doSplat = true;

			for(u_int i=0; i<CFull.size(); i++)
				CSplat.push_back(CFull[i]);

			CFull.clear();
		}

		PoolAction.unlock();

		if(doSplat) {
			for(u_int i=0; i<CSplat.size(); i++) {
				CSplat[i]->Splat(film);
				CSplat[i]->Reset();
			}

			PoolAction.lock();

			for(u_int i=0; i<CSplat.size(); i++)
				CFree.push_back(CSplat[i]);

			CSplat.clear();
			splatting = false;

			PoolAction.unlock();
		}

		return cnew;
	}

	// Flush() and Delete() are not renderthread safe,
	// they can only be called by Scene after rendering is finished.
	void Flush() {
		for(u_int i=0; i<CFull.size(); i++)
			CSplat.push_back(CFull[i]);

		CFull.clear();

		for(u_int i=0; i<CSplat.size(); i++) {
			CSplat[i]->Splat(film);
			CSplat[i]->Reset();
		}
	}

	void Delete() {
		for(u_int i=0; i<CFree.size(); i++)
			delete CFree[i];
		for(u_int i=0; i<CFull.size(); i++)
			delete CFull[i];
		for(u_int i=0; i<CSplat.size(); i++)
			delete CSplat[i];
	}

private:
	unsigned int total;
	bool splatting;
	vector<ContributionBuffer*> CFree; // Emptied/available buffers
	vector<ContributionBuffer*> CFull; // Full buffers
	vector<ContributionBuffer*> CSplat; // Buffers being splat

	Film *film;
	boost::recursive_mutex PoolMutex;
};

}//namespace lux

#endif // LUX_CONTRIBUTION_H
