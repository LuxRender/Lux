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

#ifndef LUX_PRIMITIVE_H
#define LUX_PRIMITIVE_H
// primitive.h*
#include "lux.h"
#include "geometry/transform.h"

namespace lux
{

class PrimitiveRefinementHints;

// Primitive Declarations
class Primitive {
public:
	// Construction/Destruction
	virtual ~Primitive();

	// General util
	/**
	 * Returns the world bounds of this primitive.
	 */
	virtual BBox WorldBound() const = 0;
	/**
	 * Refines this primitive to a number of primitives that are intersectable and
	 * satisfy all the given hints if possible.
	 * If this primitive should not be deallocated after refinement, it must
	 * make sure that one of the refined primitives has a shared pointer to
	 * this primitive (i.e. a copy of thisPtr)
	 * @param refined     The destenation list for the result.
	 * @param refineHints The hints for the refinement.
	 * @param thisPtr     The shared pointer to this primitive.
	 */
	virtual void Refine(vector<boost::shared_ptr<Primitive> > &refined,
		const PrimitiveRefinementHints &refineHints,
		boost::shared_ptr<Primitive> thisPtr);

	// Intersection
	/**
	 * Returns whether this primitive can be intersected.
	 */
	virtual bool CanIntersect() const = 0;
	/**
	 * Intersects this primitive with the given ray.
	 * If an intersection is found, the ray will (i.e. r.tmax)
	 * and all fields in the intersection info will be updated.
	 * @param r  The ray to intersect with this primitive.
	 * @param in The destination of the intersection information.
	 * @return Whether an intersection was found.
	 */
	virtual bool Intersect(const Ray &r, Intersection *in) const;
	/**
	 * Tests for intersection of this primitive with the given ray.
	 * @param r  The ray to intersect with this primitive.
	 * @return Whether an intersection was found.
	 */
	virtual bool IntersectP(const Ray &r) const;

	// Material
	/**
	 * Calculates the shading geometry from the given intersection geometry.
	 * @param obj2world The object to world transformation to use.
	 * @param dg        The intersection geometry.
	 * @param dgShading The destination for the shading geometry.
	 */
	virtual void GetShadingGeometry(const Transform &obj2world,
			const DifferentialGeometry &dg, DifferentialGeometry *dgShading) const;

	// Sampling
	/**
	 * Returns whether this primitive can be sampled.
	 */
	virtual bool CanSample() const = 0;
	/**
	 * Returns the area of this primitive.
	 */
	virtual float Area() const;
	/**
	 * Samples a point on this primitive.
	 * @param u1 The point coordinate in the first dimension.
	 * @param u2 The point coordinate in the second dimension.
	 * @param u3 The subprimitive to sample.
	 * @param Ns The destination for the normal in the sampled point.
	 * @return The sampled point.
	 */
	virtual Point Sample(float u1, float u2, float u3, Normal *Ns) const;
	/**
	 * Returns the probablity density for sampling the given point
	 * (@see Primitive::Sample(float,float,float,Normal*) const).
	 * @param p The point that was sampled.
	 * @return The pdf value (w.r.t. surface area) for the given point.
	 */
	virtual float Pdf(const Point &p) const;
	/**
	 * Samples a point on this primitive that will be tested for visibility
	 * from a given point.
	 * @param p  The point that will be tested for visibility with the result.
	 * @param u1 The point coordinate in the first dimension.
	 * @param u2 The point coordinate in the second dimension.
	 * @param u3 The subprimitive to sample.
	 * @param Ns The destination for the normal in the sampled point.
	 * @return The sampled point.
	 */
	virtual Point Sample(const Point &p,
			float u1, float u2, float u3, Normal *Ns) const;
	/**
	 * Returns the probability density for sampling the given point.
	 * (@see Primitive::Sample(Point&,float,float,float,Normal*) const).
	 * @param p  The point that was to be tested for visibility with the result.
	 * @param wi The direction from the above point to the sampled point.
	 * @return The pdf value (w.r.t. solid angle) for the given point.
	 */
	virtual float Pdf(const Point &p, const Vector &wi) const;
};

class PrimitiveRefinementHints {
public:
	PrimitiveRefinementHints(bool isForSampling)
		: forSampling(isForSampling)
	{
	}

	// The refined primitives should always be intersectable
	// Whether the refined primitives should be sampleable or not
	const bool forSampling;
};

// DifferentialGeometry Declarations
class DifferentialGeometry {
public:
	DifferentialGeometry() { u = v = 0.; prim = NULL; }
	// DifferentialGeometry Public Methods
	DifferentialGeometry(
			const Point &P,
			const Vector &DPDU,	const Vector &DPDV,
			const Vector &DNDU, const Vector &DNDV,
			float uu, float vv,
			const Primitive *pr);
	DifferentialGeometry(
			const Point &P, const Normal &NN,
			const Vector &DPDU,	const Vector &DPDV,
			const Vector &DNDU, const Vector &DNDV,
			float uu, float vv,
			const Primitive *pr);
	void AdjustNormal(bool ro, bool swapsHandedness) {
		reverseOrientation = ro;
		transformSwapsHandedness = swapsHandedness;
		// Adjust normal based on orientation and handedness
		if (reverseOrientation ^ transformSwapsHandedness) {
			nn.x = -nn.x;
			nn.y = -nn.y;
			nn.z = -nn.z;
		}
	}
	void ComputeDifferentials(const RayDifferential &r) const;
	// DifferentialGeometry Public Data
	Point p;
	Normal nn;
	Vector dpdu, dpdv;
	Normal dndu, dndv;
	mutable Vector dpdx, dpdy;
	float u, v;
	const Primitive* prim;
	bool reverseOrientation;
	bool transformSwapsHandedness;
	mutable float dudx, dvdx, dudy, dvdy;

	// Dade - shape specific data, useful to "transport" informatin between
	// shape intersection method and GetShadingGeometry()
	union {
		float triangleBaryCoords[3];
	};
};

class Intersection {
public:
	// Intersection Public Methods
	Intersection() : primitive(NULL), material(NULL), arealight(NULL) { }
	BSDF *GetBSDF(const TsPack *tspack, const RayDifferential &ray, float u) const;
	SWCSpectrum Le(const TsPack *tspack, const Vector &wo) const;
	SWCSpectrum Le(const TsPack *tspack, const Ray &ray, const Normal &n, BSDF **bsdf, float *pdf, float *pdfDirect) const;

	void Set(const Transform& world2object,
			const Primitive* prim, const Material* mat,
			const AreaLight* areal = NULL)
	{
		WorldToObject = world2object;
		primitive = prim;
		material = mat;
		arealight = areal;
	}

	DifferentialGeometry dg;
	Transform WorldToObject;
	const Primitive *primitive;
	const Material *material;
	const AreaLight *arealight;
};

/**
 * A decorator for primitives that are light sources.
 * This is achieved by setting the arealight field in the intersection info.
 */
class AreaLightPrimitive : public Primitive {
public:
	// AreaLightPrimitive Public Methods
	AreaLightPrimitive(boost::shared_ptr<Primitive> prim, AreaLight* arealight);

	BBox WorldBound() const { return prim->WorldBound(); };
	void Refine(vector<boost::shared_ptr<Primitive> > &refined,
			const PrimitiveRefinementHints& refineHints,
			boost::shared_ptr<Primitive> thisPtr);

	bool CanIntersect() const { return prim->CanIntersect(); }
	bool Intersect(const Ray &r, Intersection *in) const;
	bool IntersectP(const Ray &r) const { return prim->IntersectP(r); }

	bool CanSample() const { return prim->CanSample(); }
	float Area() const { return prim->Area(); }
	Point Sample(float u1, float u2, float u3, Normal *Ns) const  {
		return prim->Sample(u1, u2, u3, Ns);
	}
	float Pdf(const Point &p) const { return prim->Pdf(p); }
	Point Sample(const Point &P,
			float u1, float u2, float u3, Normal *Ns) const {
		return prim->Sample(P, u1, u2, u3, Ns);
	}
	float Pdf(const Point &p, const Vector &wi) const {
		return prim->Pdf(p, wi);
	}
private:
	// AreaLightPrimitive Private Data
	boost::shared_ptr<Primitive> prim;
	AreaLight *areaLight;
};

/**
 * A decorator for instances of primitives.
 * This is achieved by changing the Object-to-World transformation
 * in the result and other transforming all intersection info that
 * was calculated.
 */
class InstancePrimitive : public Primitive {
public:
	// InstancePrimitive Public Methods
	InstancePrimitive(boost::shared_ptr<Primitive> i,
	                  const Transform &i2w) {
		instance = i;
		InstanceToWorld = i2w;
		WorldToInstance = i2w.GetInverse();
	}

	BBox WorldBound() const  {
		return InstanceToWorld(instance->WorldBound());
	}

	bool CanIntersect() const { return instance->CanIntersect(); }
	bool Intersect(const Ray &r, Intersection *in) const;
	bool IntersectP(const Ray &r) const;

	bool CanSample() const { return instance->CanSample(); }
	float Area() const { return instance->Area(); }
	Point Sample(float u1, float u2, float u3, Normal *Ns) const  {
		return instance->Sample(u1, u2, u3, Ns);
	}
	float Pdf(const Point &p) const { return instance->Pdf(p); }
	Point Sample(const Point &P,
			float u1, float u2, float u3, Normal *Ns) const {
		return instance->Sample(P, u1, u2, u3, Ns);
	}
	float Pdf(const Point &p, const Vector &wi) const {
		return instance->Pdf(p, wi);
	}
private:
	// InstancePrimitive Private Data
	boost::shared_ptr<Primitive> instance;
	Transform InstanceToWorld, WorldToInstance;
};

class Aggregate : public Primitive {
public:
	// Aggregate Public Methods
	bool CanIntersect() const { return true; }
	bool CanSample() const { return false; }

	/**
	 * Gives all primitives in this aggregate.
	 * @param prims The destination list for the primitives.
	 */
	virtual void GetPrimitives(vector<boost::shared_ptr<Primitive> > &prims) = 0;
};

}//namespace lux

#endif // LUX_PRIMITIVE_H
