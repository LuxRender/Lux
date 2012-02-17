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
#include <boost/ptr_container/ptr_vector.hpp>
#include <boost/cstdint.hpp>

using boost::uint16_t;

namespace lux
{

// Size of a contribution buffer
// 1024 seems better for tiled buffering
// TODO find best value
#define CONTRIB_BUF_SIZE 1024u

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
	float alpha, zdepth;
	mutable float variance; // set to negative value if contribution is invalid/rejected
	uint16_t buffer, bufferGroup;
};

class ContributionBuffer {
	friend class ContributionPool;
	class Buffer {
	public:
		Buffer();
		~Buffer();

		// Thread-safe way of adding a contribution to a buffer
		// Returns false if the buffer is full
		bool Add(const Contribution &c, float weight) {
			const u_int i = osAtomicInc(&pos);

			// ensure we stay within bounds
			if (!(i < CONTRIB_BUF_SIZE))
				return false;

			contribs[i] = c;
			contribs[i].variance = weight;

			return true;
		}

		void Splat(Film *film, u_int tileIndex);

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
	~ContributionPool();

	void End(ContributionBuffer *c);

	/*
	 * Takes a pointer to a full Buffer and swaps it with a pointer to an empty Buffer.
	 * This method is thread-safe, and a different thread may swap the Buffer pointer 
	 * instead of the calling thread. In this case Next() returns immediately.
	 * If the calling thread swaps the Buffer pointer, it will accumulate the supplied
	 * sample counter and reset it.
	 * 
	 * @param b Pointer to a Buffer pointer. The Buffer pointer will be replaced by a 
	 * pointer to an empty Buffer.
	 *
	 * @param sc Number of samples that the contributions in the full Buffer represents.
	 * If the Buffer pointer is swapped, this counter is reset to zero.
	 *
	 * @param tileIndex Index of the tile that the contributions in the Buffer should be
	 * accumulated to in the Film.
	 *
	 * @param bufferGroup The buffer group that the contributions in the Buffer belongs to.
	 */
	void Next(ContributionBuffer::Buffer* volatile *b, float *sc, u_int tileIndex,
		u_int bufferGroup);

	// Flush() and Delete() are not thread safe,
	// they can only be called by Scene after rendering is finished.
	void Flush();
	void Delete();

	// I have to implement this method here in order
	// to acquire splattingMutex lock
	void CheckFilmWriteOuputInterval();

	/**
	 * Get the indexes that the current contribution spans.
	 * Current implementation is limited to at most two tiles, ie the tiles are slabs.
	 * @param tileIndex0 First tile index, always set.
	 * @param tileIndex1 Second tile index if the contribution spans more than one tile, otherwise undefined.
	 * @return Number of tiles that the contribution spans, 1 or 2.
	 */
	u_int GetFilmTileIndexes(const Contribution &contrib, u_int *tileIndex0, u_int *tileIndex1) const;

private:
	typedef boost::mutex tile_mutex;
	//typedef fast_mutex tile_mutex;

	float sampleCount;
	vector<ContributionBuffer::Buffer*> CFree; // Emptied/available buffers
	vector<vector<vector<ContributionBuffer::Buffer*> > > CFull; // Full buffers
	vector<u_int> splattingTile;
	u_int splattingMisses;

	Film *film;
	fast_mutex poolMutex;
	boost::ptr_vector<tile_mutex> tileSplattingMutexes;
	boost::mutex mainSplattingMutex;
};

inline void ContributionBuffer::Add(const Contribution &c, float weight)
{

	u_int tileIndex0, tileIndex1;
	// Add the contribution to each tile that it spans.
	u_int num_tiles = pool->GetFilmTileIndexes(c, &tileIndex0, &tileIndex1);

	//if (num_tiles > 0) is always true
	{
		Buffer* volatile* const buf = &(buffers[tileIndex0][c.bufferGroup]);
		u_int i = 0;
		// Try adding contribution to the active buffer
		// if the buffer is full, try to get a fresh buffer.
		// The iteration count is a safeguard against an infinite 
		// loop in case something goes horribly wrong
		while (!((*buf)->Add(c, weight)) && (i++ < 10)) {
			// Get an empty buffer from the pool.
			// Next() will reset sampleCount if current thread 
			// swaps buffers.
			pool->Next(buf, &sampleCount, tileIndex0, c.bufferGroup);
			// Another thread may have swapped buf before we managed to.
			// Technically there's a chance we waited so long for the lock
			// in Next() that the buffer we got back has already been filled
			// thus we try to Add() again in a loop just to be sure.
		}
	}

	if (num_tiles > 1) {		
		Buffer* volatile* const buf = &(buffers[tileIndex1][c.bufferGroup]);
		u_int i = 0;
		while (!((*buf)->Add(c, weight)) && (i++ < 10)) {
			pool->Next(buf, &sampleCount, tileIndex0, c.bufferGroup);
		}
	}

}

}//namespace lux

#endif // LUX_CONTRIBUTION_H
