/***************************************************************************
 *   Copyright (C) 1998-2010 by authors (see AUTHORS.txt )                 *
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

ParallelHashGrid::ParallelHashGrid(HitPoints *hps, float gridCoef) {
	hitPoints = hps;
	gridSize = gridCoef * hitPoints->GetSize();
	jumpSize = hitPoints->GetSize();

	grid = new unsigned int[gridSize];
	jump_list = new unsigned int[jumpSize];
}

ParallelHashGrid::~ParallelHashGrid() {
	delete[] grid;
	delete[] jump_list;
}

static void workSize(const u_int index, const u_int count, const unsigned int size, unsigned int * const first, unsigned int * const last)
{
	// Calculate the index of work this thread has to do
	const unsigned int workSize = size / count;
	*first = workSize * index;
	*last = (index == count - 1) ? size : (*first + workSize);
}

void ParallelHashGrid::JumpInsert(unsigned int hv, unsigned int i)
{
	hv = boost::interprocess::detail::atomic_cas32(reinterpret_cast<volatile boost::uint32_t*>(grid + hv), i, ~0u);

	if(hv == ~0u)
		return;

	do
	{
		hv = boost::interprocess::detail::atomic_cas32(reinterpret_cast<volatile boost::uint32_t*>(jump_list + hv), i, ~0u);
	} while(hv != ~0u);
}

void ParallelHashGrid::Refresh( const u_int index, const u_int count, boost::barrier &barrier)
{
	u_int first, last;

	const float maxPhotonRadius2 = hitPoints->GetMaxPhotonRadius2();
	const float cellSize = sqrtf(maxPhotonRadius2) * 2.f;

	if(index == 0)
	{
		invCellSize = 1.f / cellSize;
		LOG(LUX_DEBUG, LUX_NOERROR) << "Building hit points hash grid";
	}

	// reset grid
	workSize(index, count, gridSize, &first, &last);
	for(unsigned int i = first; i < last; ++i)
		grid[i] = ~0u;

	// first and last are used later, so don't change them
	workSize(index, count, jumpSize, &first, &last);
	for(unsigned int i = first; i < last; ++i)
		jump_list[i] = ~0u;

	// wait for the init of both list and invCellSize
	barrier.wait();

	// first and last are already computed by the init of jump_list
	for(unsigned int i = first; i < last; ++i)
	{
		HitPoint *hp = hitPoints->GetHitPoint(i);

		if (hp->IsSurface()) {
			const Point pos = hp->GetPosition() * invCellSize;
			JumpInsert(Hash(pos.x, pos.y, pos.z), i);
		}
	}

	barrier.wait();
}

void ParallelHashGrid::AddFlux(Sample &sample, const Point &hitPoint, const Vector &wi,
		const SpectrumWavelengths &sw, const SWCSpectrum &photonFlux, const u_int lightGroup) {

	const float maxPhotonRadius = sqrtf(hitPoints->GetMaxPhotonRadius2());
	const Vector rad(maxPhotonRadius, maxPhotonRadius, maxPhotonRadius);

	// Look for eye path hit points near the current hit point
	const Point p1 = ((hitPoint - rad)) * invCellSize;
	const Point p2 = ((hitPoint + rad)) * invCellSize;

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
					AddFluxToHitPoint(sample, hitPoints->GetHitPoint(hp_index), hitPoint, wi, sw, photonFlux, lightGroup);
					hp_index = jump_list[hp_index];
				}
				while(hp_index != ~0u);
			}
		}
	}
}
