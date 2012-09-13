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

#ifndef LUX_BSH_H
#define LUX_BSH_H
// bsh.h*
#include "lux.h"

namespace lux
{
// Bounding Sphere Hierarcy Definitions
template <class PointType, int MaxChilds>
class BSHNode {
public:
	typedef BSHNode<PointType, MaxChilds> NodeType;

	BSHNode(PointType c = PointType()) : parent(NULL), childCount(0),
		radius(0.f), radius2(0.f), center(c) {
		memset(child, 0, sizeof(child));
	}

	~BSHNode() {
		for (int i = 0; i < MaxChilds; i++)
			delete child[i];
	}

	bool Contains(const PointType &p) const {
		return DistanceSquared(center, p) < radius2;
	}

	void AddChild(NodeType *newNode) {
		// Don't add if node is full or new child doesn't exist
		if (!IsFull() && newNode) {
			child[childCount++] = newNode;
			newNode->parent = this;
		}
	}

	void Split() {
		if (!IsLeaf())
			return;
		NodeType *leftNode = child[0];
		float maxDist2 = 0.f;

		// find node furthest away from center
		for (int i = 0; i < childCount; ++i) {
			const float dist2 = DistanceSquared(center, child[i]->center);
			if (dist2 > maxDist2) {
				maxDist2 = dist2;
				leftNode = child[i];
			}
		}

		// find node furthest away from left node
		NodeType *rightNode = leftNode;
		maxDist2 = 0.f;
		for (int i = 0; i < childCount; ++i) {
			const float dist2 = DistanceSquared(leftNode->center, child[i]->center);
			if (dist2 > maxDist2) {
				maxDist2 = dist2;
				rightNode = child[i];
			}
		}

		// move centers so they just contain the left and right child nodes
		PointType leftCenter = leftNode->center + (rightNode->center - leftNode->center) * 0.25;
		PointType rightCenter = rightNode->center - (rightNode->center - leftNode->center) * 0.25;

		leftNode = new NodeType(leftCenter);
		if (!leftNode)
			return;
		leftNode->parent = this;

		rightNode = new NodeType(rightCenter);
		if (!rightNode) {
			delete leftNode;
			return;
		}
		rightNode->parent = this;

		// put other nodes in the child nodes they are closest to
		for (int i = 0; i < childCount; ++i) {
			const float leftDist2 = DistanceSquared(leftNode->center, child[i]->center);
			const float rightDist2 = DistanceSquared(rightNode->center, child[i]->center);
			if (leftDist2 < rightDist2)
				leftNode->AddChild(child[i]);
			else
				rightNode->AddChild(child[i]);
		}

		// clear child array and set left/right node
		memset(child, 0, sizeof(child));
		childCount = -1; // not a leaf
		child[0] = leftNode;
		child[1] = rightNode;

		leftNode->CalculateBounds();
		rightNode->CalculateBounds();
		CalculateBounds();
	}

	void CalculateBounds()
	{
		if (IsLeaf()) {
			radius = radius2 = 0.f;
			center = PointType();
			if (childCount < 1)
				return;

			for (int i = 0; i < childCount; ++i) {
				radius2 = max<float>(radius2, DistanceSquared(center, child[i]->center));
				center += child[i]->center;
			}

			radius = sqrtf(radius2);
			center *= (1.f / childCount);
		} else {
			// not a leaf
			center = (Left()->center + Right()->center) * 0.5f;
			radius = max<float>(Distance(center, Left()->center) + Left()->radius,
				Distance(center, Right()->center) + Right()->radius);
			radius2 = radius * radius;
		}
	}


	bool IsLeaf() const { return childCount >= 0; }

	bool IsFull() const { return childCount == MaxChilds; }

	NodeType *Left() const { return child[0]; }

	NodeType *Right() const { return child[1]; }

	NodeType *parent;
	NodeType *child[MaxChilds];
	int childCount;
	float radius, radius2;
	PointType center;
};

template <class PointType, class LookupProc, int MaxChilds = 4>
class BSH {
public:
	typedef BSHNode<PointType, MaxChilds> NodeType;

	BSH() : root(NULL), count(0) { }

	~BSH() { delete root; }

	void AddNode(PointType p) {
		NodeType *newNode = new NodeType(p);
		if (!newNode)
			return;

		if (!root) {
			root = new NodeType(p);
			if (root)
				root->AddChild(newNode);
			return;
		}

		NodeType *node = root;
		while (node) {
			if (node->IsLeaf()) {
				if (node->IsFull())
					// not leaf after split
					node->Split();
				else
					break;
				if (node->IsLeaf()) { // Split has failed
					node = NULL; // Can't add new node
					break;
				}
			}

			const float d1 = DistanceSquared(node->Left()->center, p);
			const float d2 = DistanceSquared(node->Right()->center, p);

			if (d1 < d2)
				node = node->Left();
			else
				node = node->Right();
		}
		if (!node) {
			delete newNode;
			return;
		}

		node->AddChild(newNode);
		node->CalculateBounds();

		while (node->parent) {
			node = node->parent;
			const float oldr2 = node->radius2;
			node->CalculateBounds();
			// no need to continue if bound was not increased
			if (oldr2 >= node->radius2)
				break;
		}

		++count;
	}

	void Lookup(const PointType &p, const LookupProc &process,
		float &maxDist) {
		visited = 0;

		if (!root)
			return;

		privateLookup(root, p, process, maxDist);
	}

	std::string GetTree() const {
		std::stringstream XMLOutput;

		XMLOutput<<"<?xml version='1.0' encoding='utf-8'?>"<<std::endl;

		if (root)
			genTree(XMLOutput, root);

		return XMLOutput.str();
	}

protected:
	void genTree(std::stringstream &ss, const NodeType *node) const {
		ss << "<node center=\"" << node->center[0] << ", " << node->center[1] << ", " << node->center[2] << "\">";
		if (node->IsLeaf()) {
			for (int i = 0; i < node->childCount; ++i) {
				NodeType *child = node->child[i];
				if (!child)
					break;
				ss << "<child point=\"" << child->center[0] << ", " << child->center[1] << ", " << child->center[2] << "\"/>";
			}
		} else {
			genTree(ss, node->Left());
			genTree(ss, node->Right());
		}
		ss << "</node>";
	}

	void privateLookup(const NodeType *node, const PointType &p,
		const LookupProc &process, float &maxDist2) {

		if (node->IsLeaf()) {
			for (int i = 0; i < node->childCount; ++i) {
				NodeType *child = node->child[i];

				float dist2 = DistanceSquared(child->center, p);
				if (dist2 < maxDist2) {
					++visited;
					process(child->center, dist2, maxDist2);
				}
			}
		} else {
			const float dist2l = DistanceSquared(node->Left()->center, p);
			// use slightly larger bound to avoid taking square root
			// (a+b)^2 <= a^2 + b^2 + 2*max(a,b)
			const float sep2l = maxDist2 + node->Left()->radius2 +
				2 * max(maxDist2, node->Left()->radius2);

			if (dist2l < sep2l)
				privateLookup(node->Left(), p, process, maxDist2);

			const float dist2r = DistanceSquared(node->Right()->center, p);
			// use slightly larger bound to avoid taking square root
			// (a+b)^2 <= a^2 + b^2 + 2*max(a,b)
			const float sep2r = maxDist2 + node->Right()->radius2 +
				2 * max(maxDist2, node->Right()->radius2);

			if (dist2r < sep2r)
				privateLookup(node->Right(), p, process, maxDist2);
		}
	}

	NodeType *root;
	int count;
	int visited;
};


template <class DataType> class ClosePoint {
public:
	ClosePoint(const DataType *d = NULL, float dist = INFINITY) {
		data = d;
		distance = dist;
	}

	bool operator<(const ClosePoint &p2) const {
		return distance == p2.distance ? (data < p2.data) :
				distance < p2.distance;
	}

	const DataType *data;
	float distance;
};

template <class DataType>
class NearSetPointProcess {
public:
	NearSetPointProcess(u_int k) {
		points = NULL;
		nLookup = k;
		foundPoints = 0;
	}

	void operator()(const DataType &data, float dist, float &maxDist) const {
		if (foundPoints == nLookup)
			std::pop_heap(points, points + nLookup);
		else
			++foundPoints;
		points[foundPoints - 1] = ClosePoint<DataType>(&data, dist);
		std::push_heap(points, points + foundPoints);
		maxDist = points[0].distance;
	}

	ClosePoint<DataType> *points;
	u_int nLookup;
	mutable u_int foundPoints;
};

// Basic N dimensional Point class
template <int N>
class PointN {
public:
	PointN() {
		for (int i = 0; i < N; i++)
			x[i] = 0.f;
	}

	PointN(const float _x[N]) {
		for (int i = 0; i < N; i++)
			x[i] = _x[i];
	}
	
	PointN<N> operator+(const PointN<N> &p) const {
		PointN<N> r;
		for (int i = 0; i < N; i++)
			r.x[i] = x[i] + p.x[i];
		
		return r;
	}
	
	PointN<N> &operator+=(const PointN<N> &p) {
		for (int i = 0; i < N; i++)
			x[i] += p.x[i];
		return *this;
	}
	PointN<N> operator-(const PointN<N> &p) const {
		PointN<N> r;
		for (int i = 0; i < N; i++)
			r.x[i] = x[i] - p.x[i];
		
		return r;
	}	
	PointN<N> operator* (float f) const {
		PointN<N> r;
		for (int i = 0; i < N; i++)
			r.x[i] = x[i] * f;
		return r;
	}
	PointN<N> &operator*=(float f) {
		for (int i = 0; i < N; i++)
			x[i] *= f;
		return *this;
	}

	float x[N];
};

template <int N>
float Distance(const PointN<N> &p1, const PointN<N> &p2) {
	return sqrtf(DistanceSquared(p1, p2));
}

template <int N>
float DistanceSquared(const PointN<N> &p1, const PointN<N> &p2) {
	float dist2 = 0.f;
	for (int i = 0; i < N; i++) {
		const float d = p1.x[i] - p2.x[i];
		dist2 += d*d;
	}
	return dist2;
}

}//namespace lux


#endif // LUX_BSH_H
