/***************************************************************************
 *   Copyright (C) 1998-2013 by authors (see AUTHORS.txt)                  *
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

#include "hitpoints.h"
#include "lookupaccel.h"
#include "bxdf.h"

using namespace lux;

ParallelHashGrid::ParallelHashGrid(HitPoints *hps, float gridCoef):HitPointsLookUpAccel(hps) {
	gridSize = gridCoef * hitPoints->GetSize();
	jumpSize = hitPoints->GetSize();

	grid = new unsigned int[gridSize];
	jump_list = new unsigned int[jumpSize];
}

ParallelHashGrid::~ParallelHashGrid() {
	delete[] grid;
	delete[] jump_list;
}

void ParallelHashGrid::JumpInsert(unsigned int hv, unsigned int i)
{
	hv = atomic_cas32(reinterpret_cast<volatile boost::uint32_t*>(grid + hv), i, ~0u);

	if(hv == ~0u)
		return;

	do
	{
		hv = atomic_cas32(reinterpret_cast<volatile boost::uint32_t*>(jump_list + hv), i, ~0u);
	} while(hv != ~0u);
}

void ParallelHashGrid::ResetGrid(scheduling::Range *range, unsigned *data)
{
	for(unsigned int i = range->begin(); i != range->end(); i = range->next())
		data[i] = ~0u;
}

void ParallelHashGrid::Fill(scheduling::Range *range)
{
	for(unsigned int i = range->begin(); i != range->end(); i = range->next()) {
		HitPoint *hp = hitPoints->GetHitPoint(i);

		if (hp->IsSurface()) {
			const Point pos = hp->GetPosition() * invCellSize;
			JumpInsert(Hash(pos.x, pos.y, pos.z), i);
		}
	}
}

void ParallelHashGrid::Refresh(scheduling::Scheduler *scheduler)
{
	const float maxPhotonRadius2 = hitPoints->GetMaxPhotonRadius2();
	const float cellSize = sqrtf(maxPhotonRadius2) * 2.f;

	invCellSize = 1.f / cellSize;
	LOG(LUX_DEBUG, LUX_NOERROR) << "Building hit points hash grid";

	// Reset grid
	scheduler->Launch(boost::bind(&ParallelHashGrid::ResetGrid, this, _1, grid), 0, gridSize);
	// Reset jump
	scheduler->Launch(boost::bind(&ParallelHashGrid::ResetGrid, this, _1, jump_list), 0, jumpSize);

	scheduler->Launch(boost::bind(&ParallelHashGrid::Fill, this, _1), 0, hitPoints->GetSize());
}

void ParallelHashGrid::AddFlux(Sample &sample, const PhotonData &photon) {
	const float maxPhotonRadius = sqrtf(hitPoints->GetMaxPhotonRadius2());
	const Vector rad(maxPhotonRadius, maxPhotonRadius, maxPhotonRadius);

	// Look for eye path hit points near the current hit point
	const Point p1 = ((photon.p - rad)) * invCellSize;
	const Point p2 = ((photon.p + rad)) * invCellSize;

	const int xMin = p1.x;
	const int xMax = p2.x;
	const int yMin = p1.y;
	const int yMax = p2.y;
	const int zMin = p1.z;
	const int zMax = p2.z;

	for (int iz = zMin; iz <= zMax; ++iz) {
		for (int iy = yMin; iy <= yMax; ++iy) {
			for (int ix = xMin; ix <= xMax; ++ix) {
				unsigned int hv = Hash(ix, iy, iz);

				// jumpLookAt
				unsigned int hp_index = grid[hv];

				if(grid[hv] == ~0u)
					continue;

				do
				{
					AddFluxToHitPoint(sample, hitPoints->GetHitPoint(hp_index), photon);
					hp_index = jump_list[hp_index];
				}
				while(hp_index != ~0u);
			}
		}
	}
}
