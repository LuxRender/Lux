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

	BSHNode(PointType c = PointType())
			: parent(NULL), childCount(0), radius(0.f), radius2(0.f), center(c) {
		memset(child, 0, sizeof(child));
	}
	
	bool Contains(const PointType &p) const {
		float dist2 = DistanceSquared(center, p);
		return dist2 < radius2;
	}

	void AddChild(NodeType *newNode) {
		child[childCount++] = newNode;
		newNode->parent = this;
	}

	void Split() {
		NodeType *leftNode = child[0];
		float maxDist2 = 0.f;

		// find node furthest away from center
		for (int i = 0; i < MaxChilds; i++) {
			NodeType *node = child[i];
			if (!node)
				break;

			const float dist2 = DistanceSquared(center, node->center);
			if (dist2 > maxDist2) {
				maxDist2 = dist2;
				leftNode = node;
			}
		}


		// find node furthest away from left node
		NodeType *rightNode = leftNode;
		maxDist2 = 0.f;
		for (int i = 0; i < MaxChilds; i++) {
			NodeType *node = child[i];
			if (!node)
				break;

			const float dist2 = DistanceSquared(leftNode->center, node->center);
			if (dist2 > maxDist2) {
				maxDist2 = dist2;
				rightNode = node;
			}
		}

		// move centers so they just contain the left and right child nodes
		PointType leftCenter = leftNode->center + (rightNode->center - leftNode->center) * 0.25;
		PointType rightCenter = rightNode->center - (rightNode->center - leftNode->center) * 0.25;

		leftNode = new NodeType(leftCenter);
		leftNode->parent = this;

		rightNode = new NodeType(rightCenter);
		rightNode->parent = this;

		// put other nodes in the child nodes they are closest to
		for (int i = 0; i < MaxChilds; i++) {
			NodeType *node = child[i];
			if (!node)
				break;

			const float leftDist2 = DistanceSquared(leftNode->center, node->center);
			const float rightDist2 = DistanceSquared(rightNode->center, node->center);
			if (leftDist2 < rightDist2)
				leftNode->AddChild(node);
			else
				rightNode->AddChild(node);
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
			int count = 0;

			for (int i = 0; i < MaxChilds; i++) {
				NodeType *node = child[i];
				if (!node)
					break;

				center += node->center;
				count++;
			}

			if (count < 1)
				return;

			center *= (1.0 / count);

			for (int i = 0; i < MaxChilds; i++) {
				NodeType *node = child[i];
				if (!node)
					break;

				radius2 = max<float>(radius2, DistanceSquared(center, node->center));
			}
			radius = sqrtf(radius2);
		} else {
			// not a leaf
			center = (Left()->center + Right()->center) * 0.5f;
			radius = max<float>(Distance(center, Left()->center) + Left()->radius,
				Distance(center, Right()->center) + Right()->radius);
			radius2 = radius*radius;
		}
	}


	bool IsLeaf() const {
		return childCount >= 0;
	}

	bool IsFull() const {
		return childCount == MaxChilds;
	}

	NodeType *Left() const {
		return child[0];
	}

	NodeType *Right() const {
		return child[1];
	}

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

	BSH() : root(NULL), count(0) {
    }

    void AddNode(PointType p) {
        NodeType *newNode = new NodeType(p);

        if (!root) {
            root = new NodeType(p);
            root->AddChild(newNode);
            return;
        }

        NodeType *node = root;
        while (true)
        {
            if (node->IsLeaf())
            {
                if (node->IsFull())
                    // not leaf after split
                    node->Split();
                else
                    break;
            }

            const float d1 = DistanceSquared(node->Left()->center, p);
            const float d2 = DistanceSquared(node->Right()->center, p);

            if (d1 < d2)
                node = node->Left();
            else
                node = node->Right();
        }

        node->AddChild(newNode);
        node->CalculateBounds();

		while ((node = node->parent) != NULL) {
			const float oldr2 = node->radius2;
            node->CalculateBounds();
			// no need to continue if bound was not increased
			if (oldr2 >= node->radius2)
				break;
		}

        count++;
    }

    void Lookup(const PointType &p, const LookupProc &process, float &maxDist)
    {
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
			for (int i = 0; i < MaxChilds; i++) {
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

	void privateLookup(const NodeType *node, const PointType &p, const LookupProc &process, float &maxDist2) {

        if (node->IsLeaf()) {
			for (int i = 0; i < MaxChilds; i++) {
				NodeType *child = node->child[i];
				if (!child)
					break;

                float dist2 = DistanceSquared(child->center, p);
				if (dist2 < maxDist2) {
					visited++;
					process(child->center, dist2, maxDist2);
				}
            }
        } else {
			if (node->Left()) {
				float dist2 = DistanceSquared(node->Left()->center, p);
				// use slightly larger bound to avoid taking square root
				// (a+b)^2 <= a^2 + b^2 + 2*max(a,b)
				float sep2 = maxDist2 + node->Left()->radius2 + 2*max(maxDist2, node->Left()->radius2);

				if (dist2 < sep2)
					privateLookup(node->Left(), p, process, maxDist2);
			}

			if (node->Right()) {
				float dist2 = DistanceSquared(node->Right()->center, p);
				// use slightly larger bound to avoid taking square root
				// (a+b)^2 <= a^2 + b^2 + 2*max(a,b)
				float sep2 = maxDist2 + node->Right()->radius2 + 2*max(maxDist2, node->Right()->radius2);

				if (dist2 < sep2)
					privateLookup(node->Right(), p, process, maxDist2);
			}
        }
    }

    NodeType *root;
    int count;
    int visited;
};


template <class DataType> class ClosePoint {
public:
	ClosePoint(const DataType *d = NULL,
			float dist = INFINITY) {
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
		// Do usual heap management
		if (foundPoints < nLookup) {
			// Add point to unordered array of photons
			points[foundPoints++] = ClosePoint<DataType>(&data, dist);
			if (foundPoints == nLookup) {
				std::make_heap(&points[0], &points[nLookup]);
				maxDist = points[0].distance;
			}
		} else {
			// Remove most distant point from heap and add new point
			std::pop_heap(&points[0], &points[nLookup]);
			points[nLookup - 1] = ClosePoint<DataType>(&data, dist);
			std::push_heap(&points[0], &points[nLookup]);
			maxDist = points[0].distance;
		}
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
	return sqrt(DistanceSquared(p1, p2));
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
