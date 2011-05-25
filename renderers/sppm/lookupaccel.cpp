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
#include "reflection/bxdf.h"

using namespace lux;

bool HitPointsLookUpAccel::AddFluxToHitPoint(HitPoint *hp, const u_int passIndex,
		const BSDF &bsdf, const Point &hitPoint, const Vector &wi,
		const SpectrumWavelengths &sw, const SWCSpectrum &photonFlux, u_int lightGroup) {
	HitPointEyePass &hpep(hp->eyePass[passIndex]);

	// Check distance
	const float dist2 = DistanceSquared(hpep.position, hitPoint);
	if ((dist2 >  hp->accumPhotonRadius2))
		return false;

	// Side test to choose which bsdf component we must sample
	// Note: hpep.bsdfNG is oriented facing the hitpoint eye path.
	BxDFType flag = BxDFType(BSDF_DIFFUSE | BSDF_GLOSSY |
			((Dot(hpep.bsdfNG, wi) > 0.f ? BSDF_REFLECTION : BSDF_TRANSMISSION)));
	const SWCSpectrum f = bsdf.F(sw, hpep.wo, wi, false, flag);
	if (f.Black())
		return false;

	XYZColor flux = XYZColor(sw, photonFlux * f * hpep.pathThroughput);
	// TODO: it should be more something like:
	//XYZColor flux = XYZColor(sw, photonFlux * f) * XYZColor(hp->sample->swl, hp->eyeThroughput);
	osAtomicInc(&hp->lightGroupData[lightGroup].accumPhotonCount);
	XYZColorAtomicAdd(hp->lightGroupData[lightGroup].reflectedFlux, flux);

	return true;
}

bool HitPointsLookUpAccel::DoesAddFluxToHitPoint(HitPoint *hp, const u_int passIndex,
		const BSDF &bsdf, const Point &hitPoint, const Vector &wi,
		const SpectrumWavelengths &sw) {
	HitPointEyePass &hpep(hp->eyePass[passIndex]);

	// Check distance
	const float dist2 = DistanceSquared(hpep.position, hitPoint);
	if ((dist2 >  hp->accumPhotonRadius2))
		return false;

	// Side test to choose which bsdf component we must sample
	// Note: hpep.bsdfNG is oriented facing the hitpoint eye path.
	BxDFType flag = BxDFType(BSDF_DIFFUSE | BSDF_GLOSSY |
			((Dot(hpep.bsdfNG, wi) > 0.f ? BSDF_REFLECTION : BSDF_TRANSMISSION)));
	const SWCSpectrum f = bsdf.F(sw, hpep.wo, wi, false, flag);
	if (f.Black())
		return false;

	return true;
}

void HitPointsLookUpAccel::AddFluxToSplatList(SplatList *splatList, HitPoint *hp, const u_int passIndex,
		const BSDF &bsdf, const Point &hitPoint, const Vector &wi,
		const SpectrumWavelengths &sw, const SWCSpectrum &photonFlux, const u_int lightGroup) {
	HitPointEyePass &hpep(hp->eyePass[passIndex]);

	// Check distance
	const float dist2 = DistanceSquared(hpep.position, hitPoint);
	if ((dist2 >  hp->accumPhotonRadius2))
		return;

	// Side test to choose which bsdf component we must sample
	// Note: hpep.bsdfNG is oriented facing the hitpoint eye path.
	BxDFType flag = BxDFType(BSDF_DIFFUSE | BSDF_GLOSSY |
			((Dot(hpep.bsdfNG, wi) > 0.f ? BSDF_REFLECTION : BSDF_TRANSMISSION)));
	const SWCSpectrum f = bsdf.F(sw, hpep.wo, wi, false, flag);
	if (f.Black())
		return;

	XYZColor flux = XYZColor(sw, photonFlux * f * hpep.pathThroughput);

	splatList->nodes.push_back(SplatNode(lightGroup, flux, hp));
}

void HitPointsLookUpAccel::Splat(SplatList *splatList) {
	const size_t size = splatList->nodes.size();
	const u_int splatCount = splatList->splatCount;

	for (size_t i = 0 ; i < size; ++i) {
		SplatNode *sn = &splatList->nodes[i];
		HitPoint *hp = sn->hitPoints;
		u_int lightGroup = sn->lightGroup;

		osAtomicAdd(&hp->lightGroupData[lightGroup].accumPhotonCount, splatCount);

		XYZColor flux = splatCount * sn->flux;
		XYZColorAtomicAdd(hp->lightGroupData[lightGroup].reflectedFlux, flux);
	}
}

bool HashCell::AddFlux(HitPointsLookUpAccel *accel, const u_int passIndex, const Point &hitPoint, const BSDF &bsdf,
		const Vector &wi, const SpectrumWavelengths &sw, const SWCSpectrum &photonFlux, const u_int lightGroup) {
	switch (type) {
		case HH_LIST: {
			std::list<HitPoint *>::iterator iter = list->begin();
			bool isVisible = false;
			while (iter != list->end()) {
				HitPoint *hp = *iter++;
				isVisible |= accel->AddFluxToHitPoint(hp, passIndex, bsdf, hitPoint, wi, sw, photonFlux, lightGroup);
			}

			return isVisible;
			break;
		}
		case HH_KD_TREE: {
			return kdtree->AddFlux(accel, passIndex, bsdf, hitPoint, wi, sw, photonFlux, lightGroup);
			break;
		}
		default:
			assert (false);
	}

	return false;
}

bool HashCell::HitSomething(HitPointsLookUpAccel *accel, const u_int passIndex, const Point &hitPoint, const BSDF &bsdf,
		const Vector &wi, const SpectrumWavelengths &sw) {
	switch (type) {
		case HH_LIST: {
			std::list<HitPoint *>::iterator iter = list->begin();
			while (iter != list->end()) {
				HitPoint *hp = *iter++;
				if(accel->DoesAddFluxToHitPoint(hp, passIndex, bsdf, hitPoint, wi, sw))
					return true;
			}
			break;
		}
		case HH_KD_TREE: {
			if (kdtree->HitSomething(accel, passIndex, bsdf, hitPoint, wi, sw))
				return true;
			break;
		}
		default:
			assert (false);
	}

	return false;
}

void HashCell::AddFlux(SplatList *splatList, HitPointsLookUpAccel *accel, const u_int passIndex, const Point &hitPoint, const BSDF &bsdf,
		const Vector &wi, const SpectrumWavelengths &sw, const SWCSpectrum &photonFlux, const u_int lightGroup) {
	switch (type) {
		case HH_LIST: {
			std::list<HitPoint *>::iterator iter = list->begin();
			while (iter != list->end()) {
				HitPoint *hp = *iter++;
				accel->AddFluxToSplatList(splatList, hp, passIndex, bsdf, hitPoint, wi, sw, photonFlux, lightGroup);
			}
			break;
		}
		case HH_KD_TREE: {
			kdtree->AddFlux(splatList, accel, passIndex, bsdf, hitPoint, wi, sw, photonFlux, lightGroup);
			break;
		}
		default:
			assert (false);
	}
}

void HashCell::TransformToKdTree(const u_int passIndex) {
	assert (type == HH_LIST);

	std::list<HitPoint *> *hplist = list;
	kdtree = new HCKdTree(passIndex, hplist, size);
	delete hplist;
	type = HH_KD_TREE;
}

HashCell::HCKdTree::HCKdTree(const u_int passIndex,
		std::list<HitPoint *> *hps, const unsigned int count) {
	nNodes = count;
	nextFreeNode = 1;

	//std::cerr << "Building kD-Tree with " << nNodes << " nodes" << std::endl;

	nodes = new KdNode[nNodes];
	nodeData = new HitPoint*[nNodes];
	nextFreeNode = 1;

	// Begin the HHGKdTree building process
	std::vector<HitPoint *> buildNodes;
	buildNodes.reserve(nNodes);
	maxDistSquared = 0.f;
	std::list<HitPoint *>::iterator iter = hps->begin();
	for (unsigned int i = 0; i < nNodes; ++i)  {
		buildNodes.push_back(*iter++);
		maxDistSquared = max<float>(maxDistSquared, buildNodes[i]->accumPhotonRadius2);
	}
	//std::cerr << "kD-Tree search radius: " << sqrtf(maxDistSquared) << std::endl;

	RecursiveBuild(passIndex, 0, 0, nNodes, buildNodes);
	assert (nNodes == nextFreeNode);
}

HashCell::HCKdTree::~HCKdTree() {
	delete[] nodes;
	delete[] nodeData;
}

bool HashCell::HCKdTree::CompareNode::operator ()(const HitPoint *d1, const HitPoint *d2) const {
	return (d1->eyePass[passIndex].position[axis] == d2->eyePass[passIndex].position[axis]) ? (d1 < d2) :
			(d1->eyePass[passIndex].position[axis] < d2->eyePass[passIndex].position[axis]);
}

void HashCell::HCKdTree::RecursiveBuild(const u_int passIndex,
		const unsigned int nodeNum, const unsigned int start,
		const unsigned int end, std::vector<HitPoint *> &buildNodes) {
	assert (nodeNum >= 0);
	assert (start >= 0);
	assert (end >= 0);
	assert (nodeNum < nNodes);
	assert (start < nNodes);
	assert (end <= nNodes);
	assert (start < end);

	// Create leaf node of kd-tree if we've reached the bottom
	if (start + 1 == end) {
		nodes[nodeNum].initLeaf();
		nodeData[nodeNum] = buildNodes[start];
		return;
	}

	// Choose split direction and partition data
	// Compute bounds of data from start to end
	BBox bound;
	for (unsigned int i = start; i < end; ++i)
		bound = Union(bound, buildNodes[i]->eyePass[passIndex].position);
	unsigned int splitAxis = bound.MaximumExtent();
	unsigned int splitPos = (start + end) / 2;

	std::nth_element(buildNodes.begin() + start, buildNodes.begin() + splitPos,
		buildNodes.begin() + end, CompareNode(splitAxis, passIndex));

	// Allocate kd-tree node and continue recursively
	nodes[nodeNum].init(buildNodes[splitPos]->eyePass[passIndex].position[splitAxis], splitAxis);
	nodeData[nodeNum] = buildNodes[splitPos];

	if (start < splitPos) {
		nodes[nodeNum].hasLeftChild = 1;
		const unsigned int childNum = nextFreeNode++;
		RecursiveBuild(passIndex, childNum, start, splitPos, buildNodes);
	}

	if (splitPos + 1 < end) {
		nodes[nodeNum].rightChild = nextFreeNode++;
		RecursiveBuild(passIndex, nodes[nodeNum].rightChild, splitPos + 1, end, buildNodes);
	}
}

bool HashCell::HCKdTree::AddFlux(HitPointsLookUpAccel *accel, const u_int passIndex, const BSDF &bsdf, const Point &p,
		const Vector &wi, const SpectrumWavelengths &sw, const SWCSpectrum &photonFlux, u_int lightGroup) {
	unsigned int nodeNumStack[64];
	// Start from the first node
	nodeNumStack[0] = 0;
	int stackIndex = 0;

	bool isVisible = false;
	while (stackIndex >= 0) {
		const unsigned int nodeNum = nodeNumStack[stackIndex--];
		KdNode *node = &nodes[nodeNum];

		const int axis = node->splitAxis;
		if (axis != 3) {
			const float dist = p[axis] - node->splitPos;
			const float dist2 = dist * dist;
			if (p[axis] <= node->splitPos) {
				if ((dist2 < maxDistSquared) && (node->rightChild < nNodes))
					nodeNumStack[++stackIndex] = node->rightChild;
				if (node->hasLeftChild)
					nodeNumStack[++stackIndex] = nodeNum + 1;
			} else {
				if (node->rightChild < nNodes)
					nodeNumStack[++stackIndex] = node->rightChild;
				if ((dist2 < maxDistSquared) && (node->hasLeftChild))
					nodeNumStack[++stackIndex] = nodeNum + 1;
			}
		}

		// Process the leaf
		HitPoint *hp = nodeData[nodeNum];
		isVisible |= accel->AddFluxToHitPoint(hp, passIndex, bsdf, p, wi, sw, photonFlux, lightGroup);
	}

	return isVisible;
}

bool HashCell::HCKdTree::HitSomething(HitPointsLookUpAccel *accel, const u_int passIndex, const BSDF &bsdf, const Point &p,
		const Vector &wi, const SpectrumWavelengths &sw) {
	unsigned int nodeNumStack[64];
	// Start from the first node
	nodeNumStack[0] = 0;
	int stackIndex = 0;

	while (stackIndex >= 0) {
		const unsigned int nodeNum = nodeNumStack[stackIndex--];
		KdNode *node = &nodes[nodeNum];

		const int axis = node->splitAxis;
		if (axis != 3) {
			const float dist = p[axis] - node->splitPos;
			const float dist2 = dist * dist;
			if (p[axis] <= node->splitPos) {
				if ((dist2 < maxDistSquared) && (node->rightChild < nNodes))
					nodeNumStack[++stackIndex] = node->rightChild;
				if (node->hasLeftChild)
					nodeNumStack[++stackIndex] = nodeNum + 1;
			} else {
				if (node->rightChild < nNodes)
					nodeNumStack[++stackIndex] = node->rightChild;
				if ((dist2 < maxDistSquared) && (node->hasLeftChild))
					nodeNumStack[++stackIndex] = nodeNum + 1;
			}
		}

		// Process the leaf
		HitPoint *hp = nodeData[nodeNum];
		if (accel->DoesAddFluxToHitPoint(hp, passIndex, bsdf, p, wi, sw))
			return true;
	}

	return false;
}

void HashCell::HCKdTree::AddFlux(SplatList *splatList, HitPointsLookUpAccel *accel, const u_int passIndex, const BSDF &bsdf, const Point &p,
		const Vector &wi, const SpectrumWavelengths &sw, const SWCSpectrum &photonFlux, u_int lightGroup) {
	unsigned int nodeNumStack[64];
	// Start from the first node
	nodeNumStack[0] = 0;
	int stackIndex = 0;

	while (stackIndex >= 0) {
		const unsigned int nodeNum = nodeNumStack[stackIndex--];
		KdNode *node = &nodes[nodeNum];

		const int axis = node->splitAxis;
		if (axis != 3) {
			const float dist = p[axis] - node->splitPos;
			const float dist2 = dist * dist;
			if (p[axis] <= node->splitPos) {
				if ((dist2 < maxDistSquared) && (node->rightChild < nNodes))
					nodeNumStack[++stackIndex] = node->rightChild;
				if (node->hasLeftChild)
					nodeNumStack[++stackIndex] = nodeNum + 1;
			} else {
				if (node->rightChild < nNodes)
					nodeNumStack[++stackIndex] = node->rightChild;
				if ((dist2 < maxDistSquared) && (node->hasLeftChild))
					nodeNumStack[++stackIndex] = nodeNum + 1;
			}
		}

		// Process the leaf
		HitPoint *hp = nodeData[nodeNum];
		accel->AddFluxToSplatList(splatList, hp, passIndex, bsdf, p, wi, sw, photonFlux, lightGroup);
	}
}
