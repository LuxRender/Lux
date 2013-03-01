/***************************************************************************
 *   Copyright (C) 1998-2013 by authors (see AUTHORS.txt)                  *
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

#ifndef LUX_TABRECKDTREEACCEL_H
#define LUX_TABRECKDTREEACCEL_H

// tabreckdtree.cpp*
#include "lux.h"
#include "primitive.h"
#include "memory.h"

namespace lux
{

struct TaBRecKdAccelNode {
    // TaBRecKdAccelNode Methods
    void initLeaf(int *primNums, int np,
            boost::shared_ptr<Primitive> *prims, MemoryArena &arena) {
        // Update kd leaf node allocation statistics
        // radiance - disabled for threading // static StatsCounter numLeafMade("Kd-Tree Accelerator","Leaf kd-tree nodes made");
        // radiance - disabled for threading // static StatsCounter maxDepth("Kd-Tree Accelerator","Maximum kd-tree depth");
        // radiance - disabled for threading // static StatsCounter maxLeafPrims("Kd-Tree Accelerator","Maximum number of primitives in leaf node");
        // radiance - disabled for threading // ++numLeafMade;
        //maxDepth.Max(depth);
        // radiance - disabled for threading // maxLeafPrims.Max(np);
        // radiance - disabled for threading // static StatsRatio leafPrims("Kd-Tree Accelerator","Avg. number of primitives in leaf nodes");
        // radiance - disabled for threading // leafPrims.Add(np, 1);
        nPrims = np << 2;
        flags |= 3;
        // Store _Primitive *_s for leaf node
        if (np == 0)
            onePrimitive = NULL;
        else if (np == 1)
            onePrimitive = prims[primNums[0]].get();
        else {
            primitives = (Primitive **)arena.Alloc(np *
                    sizeof(Primitive **));
            for (int i = 0; i < np; ++i)
                primitives[i] = prims[primNums[i]].get();
        }
    }
    void initInterior(int axis, float s) {
        // radiance - disabled for threading // static StatsCounter nodesMade("Kd-Tree Accelerator", "Interior kd-tree nodes made"); // NOBOOK
        // radiance - disabled for threading // ++nodesMade; // NOBOOK
        split = s;
        flags &= ~3;
        flags |= axis;
    }
    float SplitPos() const { return split; }
    int nPrimitives() const { return nPrims >> 2; }
    int SplitAxis() const { return flags & 3; }
    bool IsLeaf() const { return (flags & 3) == 3; }

    // NOTE - radiance - applied bugfix for light leaks on planes (found by ratow)
    // moved flags outside of union
    //u_int flags;   // Both
    // Dade - placing flags inside the union was intended by PBRT authors. It is
    // used at the same time of split or nPrims in order to set the lower 2 bits
    // of the long word. Read the PBRT book (kdtree chapter, about at pag. 200)
    // for more details. If there is a light leak in some case, another kind
    // of fix must be find.

    union {
        u_int flags;   // Both
        float split;   // Interior
        u_int nPrims;  // Leaf
    };
    union {
        u_int aboveChild;         // Interior
        Primitive *onePrimitive;  // Leaf
        Primitive **primitives;   // Leaf
    };
};

struct TaBRecBoundEdge {
    // TaBRecBoundEdge Public Methods
    TaBRecBoundEdge() { }
    TaBRecBoundEdge(float tt, int pn, bool starting) {
        t = tt;
        primNum = pn;
        type = starting ? START : END;
    }
    bool operator<(const TaBRecBoundEdge &e) const {
        if (t == e.t)
            return (int)type < (int)e.type;
        else return t < e.t;
    }
    float t;
    int primNum;
    enum { START, END } type;
};

// Dade - inverse mailbox support. I use a ring buffer in order to
// store a list of already intersected primitives.

// Dade - implementation with hardcoded size
struct TaBRecInverseMailboxes {
    int indexFirstFree;
    Primitive *mailboxes[8];

    TaBRecInverseMailboxes() {
        indexFirstFree = 0;
	for (u_int i = 0; i < 8; ++i)
		mailboxes[i] = NULL;
    }

    void addChecked(Primitive *p) {
        mailboxes[indexFirstFree++] = p;
        indexFirstFree &= 0x7;
    }

    bool alreadyChecked(const Primitive *p) const {
	    for (u_int i = 0; i < 8; ++i)
		    if (mailboxes[i] == p)
			    return true;

        return false;
    }
};

// TaBRecKdTreeAccel Declarations
class  TaBRecKdTreeAccel : public Aggregate {
public:
    // TaBRecKdTreeAccel Public Methods
    TaBRecKdTreeAccel(const vector<boost::shared_ptr<Primitive> > &p,
            int icost, int scost,
            float ebonus, int maxp, int maxDepth);
    virtual BBox WorldBound() const { return bounds; }
    virtual bool CanIntersect() const { return true; }
    virtual ~TaBRecKdTreeAccel();
    virtual bool Intersect(const Ray &ray, Intersection *isect) const;
    virtual bool IntersectP(const Ray &ray) const;
	virtual Transform GetLocalToWorld(float time) const {
		return Transform();
	}

    virtual void GetPrimitives(vector<boost::shared_ptr<Primitive> > &prims) const;

    static Aggregate *CreateAccelerator(const vector<boost::shared_ptr<Primitive> > &prims, const ParamSet &ps);

private:
    void buildTree(int nodeNum, const BBox &bounds,
            const vector<BBox> &primBounds,
            int *primNums, int nprims, int depth,
            TaBRecBoundEdge *edges[3],
            int *prims0, int *prims1, int badRefines = 0);
    // TaBRecKdTreeAccel Private Data
    BBox bounds;
    int isectCost, traversalCost, maxPrims;
    float emptyBonus;

    u_int nPrims;
    boost::shared_ptr<Primitive> *prims;
    TaBRecKdAccelNode *nodes;
    int nAllocedNodes, nextFreeNode;

    MemoryArena arena;
};

struct TaBRecKdNodeStack {
    const TaBRecKdAccelNode *node;
    float t;
    // Dade - Entry/exit point
    Point pb;
    // Dade - index of the previous stack element
    int prev;
};

}//namespace lux

#endif

