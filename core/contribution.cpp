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

void ContributionPool::End(ContributionBuffer *c)
{
	boost::recursive_mutex::scoped_lock poolAction(poolMutex);
	if (c)
		CFull.push_back(c);

	// If there are too many full buffers and noone else is splatting: splat
	if (CFull.size() > CONTRIB_BUF_KEEPALIVE &&
		splatting.try_lock_upgrade()) {
		splatting.unlock_upgrade_and_lock();

		CSplat.insert(CSplat.end(), CFull.begin(), CFull.end());
		CFull.clear();

		poolAction.unlock();

		for(u_int i = 0; i < CSplat.size(); ++i)
			CSplat[i]->Splat(film);

		poolAction.lock();

		for(u_int i = 0; i < CSplat.size(); ++i)
			CFree.push_back(CSplat[i]);

		CSplat.clear();
		splatting.unlock();
	}
}

ContributionBuffer* ContributionPool::Next(ContributionBuffer *c)
{
	End(c);

	boost::recursive_mutex::scoped_lock poolAction(poolMutex);
	while (CFree.size() == 0) {
		// If no splatting is occuring, there really isn't any buffer
		if (splatting.try_lock_shared()) {
			ContributionBuffer *cnew = new ContributionBuffer();
			++total;

			if(CONTRIB_DEBUG) {
				std::stringstream ss;
				ss << "Allocating Contribution Buffer - Nr of buffers in pool: "
					<< total << " - Total mem used: " << total * CONTRIB_BUF_SIZE * sizeof(Contribution) << ".";
				luxError(LUX_NOERROR, LUX_INFO, ss.str().c_str());
			}
			splatting.unlock_shared();

			return cnew;
		}
		// Wait for the splatting to finish and check again
		// Unlock the poolMutex so that splatting can actually finish
		poolAction.unlock();
		splatting.lock_shared();
		splatting.unlock_shared();
		poolAction.lock();
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
