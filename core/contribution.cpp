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

// contribution.cpp*
#include "lux.h"
#include "contribution.h"
#include "film.h"

namespace lux
{

void ContributionBuffer::Splat(Film *film)
{
	for (u_int i = 0; i < pos; ++i)
		film->AddSample(&contribs[i]);
	film->AddSampleCount(sampleCount);
	pos = 0;
	sampleCount = 0.f;
}

ContributionPool::ContributionPool() {
	total = 0;
	while (total < CONTRIB_BUF_KEEPALIVE) {
		// Add free buffers to both CFree and CSplat
		// so we can ping-pong between them.
		CFree.push_back(new ContributionBuffer());
		CSplat.push_back(new ContributionBuffer());
		++total;
	}
}

void ContributionPool::End(ContributionBuffer *c)
{
	fast_mutex::scoped_lock poolAction(poolMutex);

	if (c)
		CFull.push_back(c);

	// Any splatting not done by other threads 
	// will be done in Flush.
}

ContributionBuffer* ContributionPool::Next(ContributionBuffer *c)
{
	fast_mutex::scoped_lock poolAction(poolMutex);

	if (c)
		CFull.push_back(c);

	// If there are no free buffers, perform splat
	if (CFree.size() == 0) {
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
		CSplat.swap(CFull);
		CFull.swap(CFree);
		// CSplat now contains filled buffers,
		// CFull is empty and
		// CFree contains available buffers.

		// Store one free buffer for later, this way
		// we don't have to lock the pool lock again.
		ContributionBuffer *cold = CFree.back();
		CFree.pop_back();

		// release the pool lock
		poolAction.unlock();

		for(u_int i = 0; i < CSplat.size(); ++i)
			CSplat[i]->Splat(film);

		// return previously obtained buffer
		return cold;
	}

	ContributionBuffer *cold = CFree.back();
	CFree.pop_back();

	return cold;
}

void ContributionPool::Flush()
{
	for(u_int i = 0; i < CFull.size(); ++i)
		CSplat.push_back(CFull[i]);

	CFull.clear();

	for(u_int i = 0; i < CSplat.size(); ++i)
		CSplat[i]->Splat(film);
}

void ContributionPool::Delete()
{
	for(u_int i = 0; i < CFree.size(); ++i)
		delete CFree[i];
	for(u_int i = 0; i < CFull.size(); ++i)
		delete CFull[i];
	for(u_int i = 0; i < CSplat.size(); ++i)
		delete CSplat[i];
}

}
