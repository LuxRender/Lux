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

// contribution.cpp*
#include "lux.h"
#include "contribution.h"
#include "film.h"

#include <boost/thread/locks.hpp>

namespace lux
{

ContributionBuffer::Buffer::Buffer() : pos(0) {
	contribs = AllocAligned<Contribution>(CONTRIB_BUF_SIZE);
}

ContributionBuffer::Buffer::~Buffer() {
	FreeAligned(contribs);
}


void ContributionBuffer::Buffer::Splat(Film *film, u_int tileIndex)
{
	const u_int num_contribs = min(pos, CONTRIB_BUF_SIZE);
	film->AddTileSamples(contribs, num_contribs, tileIndex);
	pos = 0;
}

ContributionBuffer::ContributionBuffer(ContributionPool *p) :
	sampleCount(0.f), pool(p)
{
	buffers.resize(pool->CFull.size());
	for (u_int i = 0; i < buffers.size(); ++i) {
		buffers[i].resize(pool->CFull[i].size());
		for (u_int j = 0; j < buffers[i].size(); ++j)
			buffers[i][j] = new Buffer();
	}
}

ContributionBuffer::~ContributionBuffer()
{
	pool->End(this);
	// Since End gives a reference to the buffers to the pool,
	// buffers freeing is going to be handled by the pool
}

ScopedPoolLock::ScopedPoolLock(ContributionPool* pool) : lock(pool->mainSplattingMutex) {
}

void ScopedPoolLock::unlock() {
	lock.unlock();
}

ContributionPool::ContributionPool(Film *f) : sampleCount(0.f), film(f)
{
	CFull.resize(film->GetTileCount());
	for (u_int i = 0; i < CFull.size(); ++i)
		CFull[i].resize(film->GetNumBufferGroups());
	for (u_int i = 0; i < CFull.size(); ++i)
		tileSplattingMutexes.push_back(new tile_mutex);
	splattingTile.resize(CFull.size());
	for (u_int total = 0; total < CONTRIB_BUF_KEEPALIVE; ++total) {
		CFree.push_back(new ContributionBuffer::Buffer());
	}
}

ContributionPool::~ContributionPool() {
}

void ContributionPool::End(ContributionBuffer *c)
{
	fast_mutex::scoped_lock poolAction(poolMutex);

	for (u_int i = 0; i < c->buffers.size(); ++i) {
		for (u_int j = 0; j < c->buffers[i].size(); ++j)
			CFull[i][j].push_back(c->buffers[i][j]);
	}
	sampleCount = c->sampleCount;
	c->sampleCount = 0.f;

	// Any splatting not done by other threads 
	// will be done in Flush.
}

void ContributionPool::Next(ContributionBuffer::Buffer* volatile *b, float *sc,
	u_int tileIndex, u_int bufferGroup)
{
	// store the current Buffer pointer for later comparison
	ContributionBuffer::Buffer* const buf = *b;

	fast_mutex::scoped_lock pool_lock(poolMutex);

	// If the Buffer* pointed to by b has changed
	// while we waited for the lock then another thread 
	// already swapped the buffer while we waited.
	// The new buffer should be empty so just return.
	if ((*b) != buf)
		return;

	vector<vector<ContributionBuffer::Buffer*> > &full_buffers(CFull[tileIndex]);

	// Accumulate sample count and reset the ContributionBuffer's count.
	sampleCount += *sc;
	*sc = 0.f;
	full_buffers[bufferGroup].push_back(buf); // use buf here since *b is volatile

	// isSplattingTile is 0 if no splatting of that tile is going on.
	// Roll-over just means we have to wait for the tile lock (in which case it's probably a good thing!)
	u_int isSplattingTile = osAtomicInc(&splattingTile[tileIndex]);
	if (isSplattingTile > 0) {
		// Another thread is splatting this tile, so
		// get a free buffer
		if (!CFree.empty()) {
			*b = CFree.back();
			CFree.pop_back();
			return;
		}
		// No free buffers, try allocating a new one
		// but make sure we don't allocate too many new buffers.
		const u_int maxBufferMisses = CFull.size() * 32; // TODO less arbitrary limit
		u_int bufferMisses = ++splattingMisses;
		if (bufferMisses < maxBufferMisses) {
			*b = new ContributionBuffer::Buffer();
			return;
		} 
		if (bufferMisses > 1000000) {
			// reset to avoid overflow
			splattingMisses = maxBufferMisses;
		}
	}

	// No splatting going on or we couldn't get a free buffer.
	// Either way, perform splatting

	vector<ContributionBuffer::Buffer*> splat_buffers;
	for (u_int j = 0; j < full_buffers.size(); ++j) {
		splat_buffers.insert(splat_buffers.end(),
			full_buffers[j].begin(), full_buffers[j].end());
		full_buffers[j].clear();
	}
	// splat_buffers contains filled buffers and
	// CFull[tileIndex] is empty

	// Since we're still holding the pool lock, 
	// no other thread will perform the above test
	// until CFree is filled with free buffers again.
	// This prevents a thread from trying to splat
	// prematurely.
	boost::mutex::scoped_lock main_splatting_lock(mainSplattingMutex);

	const float count = sampleCount;
	sampleCount = 0.f;

	// release the pool lock
	pool_lock.unlock();

	film->AddSampleCount(count);

	{
		// aquire tile splatting lock
		tile_mutex::scoped_lock tile_splatting_lock(tileSplattingMutexes[tileIndex]);

		// release main splatting lock
		main_splatting_lock.unlock();

		for(u_int i = 0; i < splat_buffers.size(); ++i)
			splat_buffers[i]->Splat(film, tileIndex);

		// indicate we're done splatting this tile
		osAtomicWrite(&splattingTile[tileIndex], 0);
	}

	// get buffer from the now free buffers
	*b = splat_buffers.back();
	splat_buffers.pop_back();

	{
		// reaquire pool lock
		fast_mutex::scoped_lock pool_lock_end(poolMutex);

		// put splatted buffers back
		CFree.insert(CFree.end(), splat_buffers.begin(), splat_buffers.end());
	}
}

void ContributionPool::Flush()
{
	for (u_int tileIndex = 0; tileIndex < CFull.size(); ++tileIndex) {
		for (u_int j = 0; j < CFull[tileIndex].size(); ++j) {
			for (u_int k = 0; k < CFull[tileIndex][j].size(); ++k)
				CFull[tileIndex][j][k]->Splat(film, tileIndex);
			CFree.insert(CFree.end(),
				CFull[tileIndex][j].begin(), CFull[tileIndex][j].end());
			CFull[tileIndex][j].clear();
		}
	}
}

void ContributionPool::Delete()
{
	Flush();
	// At this point CFull doesn't hold any buffer
	for(u_int i = 0; i < CFree.size(); ++i)
		delete CFree[i];
}

u_int ContributionPool::GetFilmTileIndexes(const Contribution &contrib, u_int *tileIndex0, u_int *tileIndex1) const {
	return film->GetTileIndexes(contrib, tileIndex0, tileIndex1);
}

}
