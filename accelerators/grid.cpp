
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

// grid.cpp*
#include "grid.h"
#include "stats.h"
#include "memory.h"
#include "paramset.h"
#include "dynload.h"

using namespace lux;

//static StatsRatio rayTests("Grid Accelerator", "Intersection tests per ray"); // NOBOOK
//static StatsRatio rayHits("Grid Accelerator", "Intersections found per ray"); // NOBOOK

// GridAccel Method Definitions
GridAccel::GridAccel(const vector<boost::shared_ptr<Primitive> > &p,
		bool forRefined, bool refineImmediately)
	: gridForRefined(forRefined) {
	// Initialize _prims_ with primitives for grid
	vector<boost::shared_ptr<Primitive> > prims;
	PrimitiveRefinementHints refineHints(false);
	if (refineImmediately) {
		for (u_int i = 0; i < p.size(); ++i) {
			if(p[i]->CanIntersect())
				prims.push_back(p[i]);
			else
				p[i]->Refine(prims, refineHints, p[i]);
		}
	}
	else
		prims = p;
	// Initialize mailboxes for grid
	nMailboxes = prims.size();
	mailboxes = AllocAligned<GMailboxPrim>(nMailboxes);
	for (u_int i = 0; i < nMailboxes; ++i)
		new (&mailboxes[i]) GMailboxPrim(prims[i]);
	// Compute bounds and choose grid resolution
	for (u_int i = 0; i < prims.size(); ++i)
		bounds = Union(bounds, prims[i]->WorldBound());
	Vector delta = bounds.pMax - bounds.pMin;
	// Find _voxelsPerUnitDist_ for grid
	int maxAxis = bounds.MaximumExtent();
	float invMaxWidth = 1.f / delta[maxAxis];
	BOOST_ASSERT(invMaxWidth > 0.f); // NOBOOK
	float cubeRoot = 3.f * powf(float(prims.size()), 1.f/3.f);
	float voxelsPerUnitDist = cubeRoot * invMaxWidth;
	for (int axis = 0; axis < 3; ++axis) {
		NVoxels[axis] =
		     Round2Int(delta[axis] * voxelsPerUnitDist);
		NVoxels[axis] = Clamp(NVoxels[axis], 1, 64);
	}
	// Compute voxel widths and allocate voxels
	for (int axis = 0; axis < 3; ++axis) {
		Width[axis] = delta[axis] / NVoxels[axis];
		InvWidth[axis] =
		    (Width[axis] == 0.f) ? 0.f : 1.f / Width[axis];
	}
	int nVoxels = NVoxels[0] * NVoxels[1] * NVoxels[2];
	voxels = AllocAligned<Voxel *>(nVoxels);
	memset(voxels, 0, nVoxels * sizeof(Voxel *));
	// Add primitives to grid voxels
	for (u_int i = 0; i < prims.size(); ++i) {
		// Find voxel extent of primitive
		BBox pb = prims[i]->WorldBound();
		int vmin[3], vmax[3];
		for (int axis = 0; axis < 3; ++axis) {
			vmin[axis] = PosToVoxel(pb.pMin, axis);
			vmax[axis] = PosToVoxel(pb.pMax, axis);
		}
		// Add primitive to overlapping voxels
		for (int z = vmin[2]; z <= vmax[2]; ++z)
			for (int y = vmin[1]; y <= vmax[1]; ++y)
				for (int x = vmin[0]; x <= vmax[0]; ++x) {
					int offset = Offset(x, y, z);
					if (!voxels[offset]) {
						// Allocate new voxel and store primitive in it
						voxels[offset] = voxelArena.construct(&mailboxes[i]);//new (voxelArena) Voxel(&mailboxes[i]);
					}
					else {
						// Add primitive to already-allocated voxel
						voxels[offset]->AddPrimitive(&mailboxes[i]);
					}
				}
		static StatsRatio nPrimitiveVoxels("Grid Accelerator", "Voxels covered vs # / primitives"); // NOBOOK
		nPrimitiveVoxels.Add((1 + vmax[0]-vmin[0]) * (1 + vmax[1]-vmin[1]) * // NOBOOK
			(1 + vmax[2]-vmin[2]), 1); // NOBOOK
	}
	// Update grid statistics
	static StatsPercentage nEmptyVoxels("Grid Accelerator","Empty voxels");
	static StatsRatio avgPrimsInVoxel("Grid Accelerator","Average # of primitives in voxel");
	static StatsCounter maxPrimsInVoxel("Grid Accelerator","Max # of primitives in a grid voxel");
	nEmptyVoxels.Add(0, NVoxels[0] * NVoxels[1] * NVoxels[2]);
	avgPrimsInVoxel.Add(0,NVoxels[0] * NVoxels[1] * NVoxels[2]);
	for (int z = 0; z < NVoxels[2]; ++z)
		for (int y = 0; y < NVoxels[1]; ++y)
			for (int x = 0; x < NVoxels[0]; ++x) {
				int offset = Offset(x, y, z);
				if (!voxels[offset]) nEmptyVoxels.Add(1, 0);
				else {
				    int nPrims = voxels[offset]->nPrimitives;
					maxPrimsInVoxel.Max(nPrims);
					avgPrimsInVoxel.Add(nPrims, 0);
				}
			}
}
BBox GridAccel::WorldBound() const {
	return bounds;
}
GridAccel::~GridAccel() {
	for (u_int i = 0; i < nMailboxes; ++i)
		mailboxes[i].~GMailboxPrim();
	FreeAligned(mailboxes);
	for (int i = 0;
	     i < NVoxels[0]*NVoxels[1]*NVoxels[2];
		 ++i)
		if (voxels[i]) voxels[i]->~Voxel();
	FreeAligned(voxels);
}
bool GridAccel::Intersect(const Ray &ray,
                          Intersection *isect) const {
	if (!gridForRefined) { // NOBOOK
		//rayTests.Add(0, 1); // NOBOOK
		//rayHits.Add(0, 1); // NOBOOK
	} // NOBOOK
	// Check ray against overall grid bounds
	float rayT;
	if (bounds.Inside(ray(ray.mint)))
		rayT = ray.mint;
	else if (!bounds.IntersectP(ray, &rayT))
		return false;
	Point gridIntersect = ray(rayT);
	// Get ray mailbox id
	int rayId = ++curMailboxId;
	// Set up 3D DDA for ray
	float NextCrossingT[3], DeltaT[3];
	int Step[3], Out[3], Pos[3];
	for (int axis = 0; axis < 3; ++axis) {
		// Compute current voxel for axis
		Pos[axis] = PosToVoxel(gridIntersect, axis);
		if (ray.d[axis] >= 0) {
			// Handle ray with positive direction for voxel stepping
			NextCrossingT[axis] = rayT +
				(VoxelToPos(Pos[axis]+1, axis) - gridIntersect[axis]) /
					ray.d[axis];
			DeltaT[axis] = Width[axis] / ray.d[axis];
			Step[axis] = 1;
			Out[axis] = NVoxels[axis];
		}
		else {
			// Handle ray with negative direction for voxel stepping
			NextCrossingT[axis] = rayT +
				(VoxelToPos(Pos[axis], axis) - gridIntersect[axis]) /
					ray.d[axis];
			DeltaT[axis] = -Width[axis] / ray.d[axis];
			Step[axis] = -1;
			Out[axis] = -1;
		}
	}
	// Walk ray through voxel grid
	bool hitSomething = false;
	for (;;) {
		Voxel *voxel =
			voxels[Offset(Pos[0],	Pos[1], Pos[2])];
		if (voxel != NULL)
			hitSomething |= voxel->Intersect(ray, isect, rayId);
		// Advance to next voxel
		// Find _stepAxis_ for stepping to next voxel
		int bits = ((NextCrossingT[0] < NextCrossingT[1]) << 2) +
			((NextCrossingT[0] < NextCrossingT[2]) << 1) +
			((NextCrossingT[1] < NextCrossingT[2]));
		const int cmpToAxis[8] = { 2, 1, 2, 1, 2, 2, 0, 0 };
		int stepAxis = cmpToAxis[bits];
		if (ray.maxt < NextCrossingT[stepAxis])
			break;
		Pos[stepAxis] += Step[stepAxis];
		if (Pos[stepAxis] == Out[stepAxis])
			break;
		NextCrossingT[stepAxis] += DeltaT[stepAxis];
	}
	return hitSomething;
}
int GridAccel::curMailboxId = 0;
bool Voxel::Intersect(const Ray &ray,
                      Intersection *isect,
					  int rayId) {
	// Refine primitives in voxel if needed
	if (!allCanIntersect) {
		GMailboxPrim **mpp;
		if (nPrimitives == 1) mpp = &onePrimitive;
		else mpp = primitives;
		for (u_int i = 0; i < nPrimitives; ++i) {
			GMailboxPrim *mp = mpp[i];
			// Refine primitive in _mp_ if it's not intersectable
			if (!mp->primitive->CanIntersect()) {
				vector<boost::shared_ptr<Primitive> > p;
				PrimitiveRefinementHints refineHints(false);
				mp->primitive->Refine(p, refineHints, mp->primitive);
				BOOST_ASSERT(p.size() > 0); // NOBOOK
				if (p.size() == 1)
					mp->primitive = p[0];
				else {
					boost::shared_ptr<Primitive> o (new GridAccel(p, true, false));
					mp->primitive = o;
				}
			}
		}
		allCanIntersect = true;
	}
	// Loop over primitives in voxel and find intersections
	bool hitSomething = false;
	GMailboxPrim **mpp;
	if (nPrimitives == 1) mpp = &onePrimitive;
	else mpp = primitives;
	for (u_int i = 0; i < nPrimitives; ++i) {
		GMailboxPrim *mp = mpp[i];
		// Do mailbox check between ray and primitive
		if (mp->lastMailboxId == rayId)
			continue;
		// Check for ray--primitive intersection
		mp->lastMailboxId = rayId;
		//rayTests.Add(1, 0); // NOBOOK
		if (mp->primitive->Intersect(ray, isect)) {
			//rayHits.Add(1, 0); // NOBOOK
			hitSomething = true;
		}
	}
	return hitSomething;
}
bool GridAccel::IntersectP(const Ray &ray) const {
	// radiance - disabled for threading // if (!gridForRefined) { // NOBOOK
	// radiance - disabled for threading // 	//rayTests.Add(0, 1); // NOBOOK
	// radiance - disabled for threading // 	//rayHits.Add(0, 1); // NOBOOK
	// radiance - disabled for threading // } // NOBOOK
	int rayId = ++curMailboxId;
	// Check ray against overall grid bounds
	float rayT;
	if (bounds.Inside(ray(ray.mint)))
		rayT = ray.mint;
	else if (!bounds.IntersectP(ray, &rayT))
		return false;
	Point gridIntersect = ray(rayT);
	// Set up 3D DDA for ray
	float NextCrossingT[3], DeltaT[3];
	int Step[3], Out[3], Pos[3];
	for (int axis = 0; axis < 3; ++axis) {
		// Compute current voxel for axis
		Pos[axis] = PosToVoxel(gridIntersect, axis);
		if (ray.d[axis] >= 0) {
			// Handle ray with positive direction for voxel stepping
			NextCrossingT[axis] = rayT +
				(VoxelToPos(Pos[axis]+1, axis) - gridIntersect[axis]) /
					ray.d[axis];
			DeltaT[axis] = Width[axis] / ray.d[axis];
			Step[axis] = 1;
			Out[axis] = NVoxels[axis];
		}
		else {
			// Handle ray with negative direction for voxel stepping
			NextCrossingT[axis] = rayT +
				(VoxelToPos(Pos[axis], axis) - gridIntersect[axis]) /
					ray.d[axis];
			DeltaT[axis] = -Width[axis] / ray.d[axis];
			Step[axis] = -1;
			Out[axis] = -1;
		}
	}
	// Walk grid for shadow ray
	for (;;) {
		int offset = Offset(Pos[0], Pos[1], Pos[2]);
		Voxel *voxel = voxels[offset];
		if (voxel && voxel->IntersectP(ray, rayId))
			return true;
		// Advance to next voxel
		// Find _stepAxis_ for stepping to next voxel
		int bits = ((NextCrossingT[0] < NextCrossingT[1]) << 2) +
			((NextCrossingT[0] < NextCrossingT[2]) << 1) +
			((NextCrossingT[1] < NextCrossingT[2]));
		const int cmpToAxis[8] = { 2, 1, 2, 1, 2, 2, 0, 0 };
		int stepAxis = cmpToAxis[bits];
		if (ray.maxt < NextCrossingT[stepAxis])
			break;
		Pos[stepAxis] += Step[stepAxis];
		if (Pos[stepAxis] == Out[stepAxis])
			break;
		NextCrossingT[stepAxis] += DeltaT[stepAxis];
	}
	return false;
}
bool Voxel::IntersectP(const Ray &ray, int rayId) {
	// Refine primitives in voxel if needed
	if (!allCanIntersect) {
		GMailboxPrim **mpp;
		if (nPrimitives == 1) mpp = &onePrimitive;
		else mpp = primitives;
		for (u_int i = 0; i < nPrimitives; ++i) {
			GMailboxPrim *mp = mpp[i];
			// Refine primitive in _mp_ if it's not intersectable
			if (!mp->primitive->CanIntersect()) {
				vector<boost::shared_ptr<Primitive> > p;
				PrimitiveRefinementHints refineHints(false);
				mp->primitive->Refine(p, refineHints, mp->primitive);
				BOOST_ASSERT(p.size() > 0); // NOBOOK
				if (p.size() == 1)
					mp->primitive = p[0];
				else {
					boost::shared_ptr<Primitive> o (new GridAccel(p, true, false));
					mp->primitive = o;
				}
			}
		}
		allCanIntersect = true;
	}
	GMailboxPrim **mpp;
	if (nPrimitives == 1) mpp = &onePrimitive;
	else mpp = primitives;
	for (u_int i = 0; i < nPrimitives; ++i) {
		GMailboxPrim *mp = mpp[i];
		// Do mailbox check between ray and primitive
		if (mp->lastMailboxId == rayId)
			continue;
		// Check for ray--primitive intersection for shadow ray
		mp->lastMailboxId = rayId;
		// radiance - disabled for threading // //rayTests.Add(1, 0);
		if (mp->primitive->IntersectP(ray)) {
			// radiance - disabled for threading // //rayHits.Add(1, 0);
			return true;
		}
	}
	return false;
}
void GridAccel::GetPrimitives(vector<boost::shared_ptr<Primitive> > &primitives) {
	primitives.reserve(nMailboxes);
	for(u_int i=0; i < nMailboxes; i++) {
		primitives.push_back(mailboxes[i].primitive);
	}
}
Aggregate *GridAccel::CreateAccelerator(const vector<boost::shared_ptr<Primitive> > &prims,
		const ParamSet &ps) {
	bool refineImmediately = ps.FindOneBool("refineimmediately", false);
	return new GridAccel(prims, false, refineImmediately);
}

static DynamicLoader::RegisterAccelerator<GridAccel> r("grid");
