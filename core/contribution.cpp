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

namespace lux
{

void ContributionBuffer::Buffer::Splat(Film *film)
{
	const u_int num_contribs = min(pos, CONTRIB_BUF_SIZE);
	for (u_int i = 0; i < num_contribs; ++i)
		film->AddSample(&contribs[i]);
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

ContributionPool::ContributionPool(Film *f) : sampleCount(0.f), film(f)
{
	for (u_int total = 0; total < CONTRIB_BUF_KEEPALIVE; ++total) {
		// Add free buffers to both CFree and CSplat
		// so we can ping-pong between them.
		CFree.push_back(new ContributionBuffer::Buffer());
		CSplat.push_back(new ContributionBuffer::Buffer());
	}
	CFull.resize(film->GetNumBufferGroups());
	for (u_int i = 0; i < CFull.size(); ++i)
		CFull[i].resize(film->GetBufferGroup(i).buffers.size());
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

void ContributionPool::Next(ContributionBuffer::Buffer* volatile *b, float sc,
	u_int bufferGroup, u_int buffer)
{
	ContributionBuffer::Buffer* const buf = *b;

	fast_mutex::scoped_lock poolAction(poolMutex);

	// other thread swapped the buffer while current thread
	// waited for the lock, all is good
	if ((*b) != buf)
		return;

	sampleCount += sc;
	CFull[bufferGroup][buffer].push_back(buf);

	if (!CFree.empty()) {
		*b = CFree.back();
		CFree.pop_back();
		return;
	}

	// If there are no free buffers, perform splat
	// Since we're still holding the pool lock, 
	// no other thread will perform the above test
	// until CFree is filled with free buffers again.
	// This prevents a thread from trying to splat
	// prematurely.
	boost::mutex::scoped_lock splattingAction(splattingMutex);

	// CSplat contains available buffers
	// from last splatting.
	// CFull contains filled buffers ready for splatting.
	// CFree is empty.
	CSplat.swap(CFree);
	for (u_int i = 0; i < CFull.size(); ++i) {
		for (u_int j = 0; j < CFull[i].size(); ++j) {
			CSplat.insert(CSplat.end(),
				CFull[i][j].begin(), CFull[i][j].end());
			CFull[i][j].clear();
		}
	}
	// CSplat now contains filled buffers,
	// CFull is empty and
	// CFree contains available buffers.

	// Dade - Bug 582 fix: allocate a new buffer if CFree is empty.
	if (CFree.empty())
		*b = new ContributionBuffer::Buffer();
	else {
		// Store one free buffer for later, this way
		// we don't have to lock the pool lock again.
		*b = CFree.back();
		CFree.pop_back();
	}

	const float count = sampleCount;
	sampleCount = 0.f;

	// release the pool lock
	poolAction.unlock();

	film->AddSampleCount(count);
	for(u_int i = 0; i < CSplat.size(); ++i)
		CSplat[i]->Splat(film);

	film->CheckWriteOuputInterval();
}

void ContributionPool::Flush()
{
	for (u_int i = 0; i < CFull.size(); ++i) {
		for (u_int j = 0; j < CFull[i].size(); ++j) {
			for (u_int k = 0; k < CFull[i][j].size(); ++k)
				CFull[i][j][k]->Splat(film);
			CFree.insert(CFree.end(),
				CFull[i][j].begin(), CFull[i][j].end());
			CFull[i][j].clear();
		}
	}
}

void ContributionPool::Delete()
{
	Flush();
	// At this point CFull doesn't hold any buffer
	for(u_int i = 0; i < CFree.size(); ++i)
		delete CFree[i];
	for(u_int i = 0; i < CSplat.size(); ++i)
		delete CSplat[i];
}

void ContributionPool::CheckFilmWriteOuputInterval()
{
	boost::mutex::scoped_lock splattingAction(splattingMutex);
	film->CheckWriteOuputInterval();
}

}
