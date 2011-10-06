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
#include "photonsampler.h"


/*
   The flux stored inside accumReflectedFlux can be normalised by a radial
   symetrical kernel as stated by:

      "A Progressive Error Estimation Framework for Photon Density Estimation"
      T. Hachisuka, W. Jarosz, and H. W. Jensen
      ACM Transactions on Graphics (SIGGRAPH Asia 2010), 2010

      http://graphics.ucsd.edu/~toshiya/ee.pdf

      its improve smoothness of the result and makes the assumption done in PPM less strict
*/
inline float Ekernel(const float d2, float md2) {
	// Epanechnikov kernel normalised on a disk
	float s = 1.f - d2 / md2;

	return s / (M_PI * md2) * 2.f;
}

using namespace lux;

void HitPointsLookUpAccel::AddFluxToHitPoint(Sample &sample, HitPoint *hp,
		const Point &hitPoint, const Vector &wi,
		const SpectrumWavelengths &sw, const SWCSpectrum &photonFlux, u_int lightGroup) {
	HitPointEyePass &hpep(hp->eyePass);

	// Check distance
	const float dist2 = DistanceSquared(hpep.position, hitPoint);
	if ((dist2 >  hp->accumPhotonRadius2))
		return;

	BxDFType const flag = BxDFType(BSDF_DIFFUSE | BSDF_GLOSSY | BSDF_REFLECTION | BSDF_TRANSMISSION);
	const SWCSpectrum f = hpep.bsdf->F(sw, wi, hpep.wo, true, flag);
	if (f.Black())
		return;

	XYZColor flux = XYZColor(sw, photonFlux * f * hpep.pathThroughput) * Ekernel(dist2, hp->accumPhotonRadius2);

	dynamic_cast<PhotonSampler *>(sample.sampler)->AddSample(&sample, lightGroup, hp, flux);
}

void HashCell::AddFlux(Sample& sample, HitPointsLookUpAccel *accel, const Point &hitPoint,
		const Vector &wi, const SpectrumWavelengths &sw, const SWCSpectrum &photonFlux, const u_int lightGroup) {
	switch (type) {
		case HH_LIST: {
			std::list<HitPoint *>::iterator iter = list->begin();
			while (iter != list->end()) {
				HitPoint *hp = *iter++;
				accel->AddFluxToHitPoint(sample, hp, hitPoint, wi, sw, photonFlux, lightGroup);
			}
			break;
		}
		case HH_KD_TREE: {
			kdtree->AddFlux(sample, accel, hitPoint, wi, sw, photonFlux, lightGroup);
			break;
		}
		default:
			assert (false);
	}
}

void HashCell::TransformToKdTree() {
	assert (type == HH_LIST);

	std::list<HitPoint *> *hplist = list;
	kdtree = new HCKdTree(hplist, size);
	delete hplist;
	type = HH_KD_TREE;
}

HashCell::HCKdTree::HCKdTree(
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

	RecursiveBuild(0, 0, nNodes, buildNodes);
	assert (nNodes == nextFreeNode);
}

HashCell::HCKdTree::~HCKdTree() {
	delete[] nodes;
	delete[] nodeData;
}

bool HashCell::HCKdTree::CompareNode::operator ()(const HitPoint *d1, const HitPoint *d2) const {
	return (d1->eyePass.position[axis] == d2->eyePass.position[axis]) ? (d1 < d2) :
			(d1->eyePass.position[axis] < d2->eyePass.position[axis]);
}

void HashCell::HCKdTree::RecursiveBuild(
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
		bound = Union(bound, buildNodes[i]->eyePass.position);
	unsigned int splitAxis = bound.MaximumExtent();
	unsigned int splitPos = (start + end) / 2;

	std::nth_element(buildNodes.begin() + start, buildNodes.begin() + splitPos,
		buildNodes.begin() + end, CompareNode(splitAxis));

	// Allocate kd-tree node and continue recursively
	nodes[nodeNum].init(buildNodes[splitPos]->eyePass.position[splitAxis], splitAxis);
	nodeData[nodeNum] = buildNodes[splitPos];

	if (start < splitPos) {
		nodes[nodeNum].hasLeftChild = 1;
		const unsigned int childNum = nextFreeNode++;
		RecursiveBuild( childNum, start, splitPos, buildNodes);
	}

	if (splitPos + 1 < end) {
		nodes[nodeNum].rightChild = nextFreeNode++;
		RecursiveBuild( nodes[nodeNum].rightChild, splitPos + 1, end, buildNodes);
	}
}

void HashCell::HCKdTree::AddFlux(Sample& sample, HitPointsLookUpAccel *accel, const Point &p,
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
		accel->AddFluxToHitPoint(sample, hp, p, wi, sw, photonFlux, lightGroup);
	}
}
