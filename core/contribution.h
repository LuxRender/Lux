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

#ifndef LUX_CONTRIBUTION_H
#define LUX_CONTRIBUTION_H
// contribution.h*
#include "lux.h"
#include "memory.h"
#include "color.h"
#include "fastmutex.h"
#include "osfunc.h"

#include <boost/thread/mutex.hpp>
#include <boost/cstdint.hpp>

using boost::uint16_t;

namespace lux
{

// Size of a contribution buffer
// 4096 seems best.
#define CONTRIB_BUF_SIZE 4096u

// Minimum number of buffers to keep alive/reuse
// In practice twice this amount stays allocated
#define CONTRIB_BUF_KEEPALIVE 1

// Switch on to get feedback in the log about allocation
#define CONTRIB_DEBUG false

class Contribution {
public:
	Contribution(float x=0.f, float y=0.f, const XYZColor &c=0.f, float a=0.f, float zd=0.f,
		float v=0.f, u_int b=0, u_int g=0) :
		imageX(x), imageY(y), color(c), alpha(a), zdepth(zd), variance(v),
		buffer(static_cast<uint16_t>(b)), bufferGroup(static_cast<uint16_t>(g)) { }

	float imageX, imageY;
	XYZColor color;
	float alpha, zdepth, variance;
	uint16_t buffer, bufferGroup;
};

class ContributionBuffer {
	friend class ContributionPool;
	class Buffer {
	public:
		Buffer() : pos(0) {
			contribs = AllocAligned<Contribution>(CONTRIB_BUF_SIZE);
		}

		~Buffer() {
			FreeAligned(contribs);
		}

		bool Add(const Contribution &c, float weight=1.f) {
			u_int i = osAtomicInc(&pos);

			// ensure we stay within bounds
			if (i+1 >= CONTRIB_BUF_SIZE)
				return false;

			contribs[i] = c;
			contribs[i].variance = weight;

			return true;
		}

		bool Filled() const {
			return pos > CONTRIB_BUF_SIZE/2;
		}

		void Splat(Film *film);

	private:
		u_int pos;
		Contribution *contribs;
	};
public:
	ContributionBuffer(ContributionPool *p);

	~ContributionBuffer();

	inline void Add(const Contribution &c, float weight=1.f);

	void AddSampleCount(float c) {
		sampleCount += c;
	}

private:
	float sampleCount;
	vector<vector<Buffer *> > buffers;
	ContributionPool *pool;
};

class ContributionPool {
	friend class ContributionBuffer;
public:

	ContributionPool(Film *f);

	void End(ContributionBuffer *c);

	void Next(ContributionBuffer::Buffer **b, float sc, u_int bufferGroup,
		u_int buffer);

	// Flush() and Delete() are not thread safe,
	// they can only be called by Scene after rendering is finished.
	void Flush();
	void Delete();

	// I have to implement this method here in order
	// to acquire splattingMutex lock
	void CheckFilmWriteOuputInterval();

private:
	float sampleCount;
	vector<ContributionBuffer::Buffer*> CFree; // Emptied/available buffers
	vector<vector<vector<ContributionBuffer::Buffer*> > > CFull; // Full buffers
	vector<ContributionBuffer::Buffer*> CSplat; // Buffers being splat

	Film *film;
	fast_mutex poolMutex;
	boost::mutex splattingMutex;
};

inline void ContributionBuffer::Add(const Contribution &c, float weight)
{
	Buffer **buf = &(buffers[c.bufferGroup][c.buffer]);
	if (!(*buf)->Add(c, weight)) {
		pool->Next(buf, sampleCount, c.bufferGroup, c.buffer);
		(*buf)->Add(c, weight);
		sampleCount = 0.f;
	}
}

}//namespace lux

#endif // LUX_CONTRIBUTION_H
