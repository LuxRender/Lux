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
#include "motionsystem.h"
#include "geometry/raydifferential.h"
#include "spectrum.h"

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
	 * Samples a point on this primitive. Only the p, nn, dpdu, dpdv, u and v
	 * need to be calculated.
	 *
	 * @param u1 The point coordinate in the first dimension.
	 * @param u2 The point coordinate in the second dimension.
	 * @param u3 The subprimitive to sample.
	 * @param dg The destination to store the sampled point in.
	 */
	virtual void Sample(float u1, float u2, float u3, DifferentialGeometry *dg) const;
	/**
	 * Returns the probablity density for sampling the given point
	 * (@see Primitive::Sample(float,float,float,Normal*) const).
	 * @param p The point that was sampled.
	 * @return The pdf value (w.r.t. surface area) for the given point.
	 */
	virtual float Pdf(const Point &p) const;
	/**
	 * Samples a point on this primitive that will be tested for visibility
	 * from a given point. Only the p, nn, dpdu, dpdv, u and v need to be
	 * calculated.
	 *
	 * @param p  The point that will be tested for visibility with the result.
	 * @param u1 The point coordinate in the first dimension.
	 * @param u2 The point coordinate in the second dimension.
	 * @param u3 The subprimitive to sample.
	 * @param dg The destination to store the sampled point in.
	 */
	virtual void Sample(const Point &p,
			float u1, float u2, float u3, DifferentialGeometry *dg) const;
	/**
	 * Returns the probability density for sampling the given point.
	 * (@see Primitive::Sample(Point&,float,float,float,Normal*) const).
	 * @param p  The point that was to be tested for visibility with the result.
	 * @param wi The direction from the above point to the sampled point.
	 * @return The pdf value (w.r.t. solid angle) for the given point.
	 */
	virtual float Pdf(const Point &p, const Vector &wi) const;
	/**
	 * Returns the probability density for sampling the given point.
	 * (@see Primitive::Sample(Point&,float,float,float,Normal*) const).
	 * No visibility test is done here.
	 * @param p  The point that was to be tested for visibility with the result.
	 * @param po The point that was sampled.
	 * @return The pdf value (w.r.t. surface area) for the given point.
	 */
	virtual float Pdf(const Point &p, const Point &po) const;
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
	virtual ~AreaLightPrimitive() { }

	virtual BBox WorldBound() const { return prim->WorldBound(); };
	virtual void Refine(vector<boost::shared_ptr<Primitive> > &refined,
			const PrimitiveRefinementHints& refineHints,
			boost::shared_ptr<Primitive> thisPtr);

	virtual bool CanIntersect() const { return prim->CanIntersect(); }
	virtual bool Intersect(const Ray &r, Intersection *in) const;
	virtual bool IntersectP(const Ray &r) const { return prim->IntersectP(r); }

	virtual void GetShadingGeometry(const Transform &obj2world,
		const DifferentialGeometry &dg, DifferentialGeometry *dgShading) const {
			prim->GetShadingGeometry(obj2world, dg, dgShading);
	}

	virtual bool CanSample() const { return prim->CanSample(); }
	virtual float Area() const { return prim->Area(); }
	virtual void Sample(float u1, float u2, float u3, DifferentialGeometry *dg) const  {
		prim->Sample(u1, u2, u3, dg);
	}
	virtual float Pdf(const Point &p) const { return prim->Pdf(p); }
	virtual void Sample(const Point &P,
			float u1, float u2, float u3, DifferentialGeometry *dg) const {
		prim->Sample(P, u1, u2, u3, dg);
	}
	virtual float Pdf(const Point &p, const Vector &wi) const {
		return prim->Pdf(p, wi);
	}
	virtual float Pdf(const Point &p, const Point &po) const {
		return prim->Pdf(p, po);
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
 * was calculated. Optionally the material can be changed too.
 */
class InstancePrimitive : public Primitive {
public:
	// InstancePrimitive Public Methods
	/**
	 * Creates a new instance from the given primitive.
	 *
	 * @param i   The primitive to instance.
	 * @param i2w The instance to world transformation.
	 * @param mat The material this instance or NULL to use the
	 *            instanced primitive's material.
	 */
	InstancePrimitive(boost::shared_ptr<Primitive> i,
	                  const Transform &i2w,
					  boost::shared_ptr<Material> mat) {
		instance = i;
		InstanceToWorld = i2w;
		WorldToInstance = i2w.GetInverse();
		material = mat;
	}
	virtual ~InstancePrimitive() { }

	virtual BBox WorldBound() const  {
		return InstanceToWorld(instance->WorldBound());
	}

	virtual bool CanIntersect() const { return instance->CanIntersect(); }
	virtual bool Intersect(const Ray &r, Intersection *in) const;
	virtual bool IntersectP(const Ray &r) const;

	virtual bool CanSample() const { return instance->CanSample(); }
	virtual float Area() const { return instance->Area(); }
	virtual void Sample(float u1, float u2, float u3, DifferentialGeometry *dg) const  {
		instance->Sample(u1, u2, u3, dg);
	}
	virtual float Pdf(const Point &p) const { return instance->Pdf(p); }
	virtual void Sample(const Point &P,
			float u1, float u2, float u3, DifferentialGeometry *dg) const {
		instance->Sample(P, u1, u2, u3, dg);
	}
	virtual float Pdf(const Point &p, const Vector &wi) const {
		return instance->Pdf(p, wi);
	}
	virtual float Pdf(const Point &p, const Point &po) const {
		return instance->Pdf(p, po);
	}
private:
	// InstancePrimitive Private Data
	boost::shared_ptr<Primitive> instance;
	Transform InstanceToWorld, WorldToInstance;
	boost::shared_ptr<Material> material;
};

class Aggregate : public Primitive {
public:
	// Aggregate Public Methods
	virtual ~Aggregate() { }
	virtual bool CanIntersect() const { return true; }
	virtual bool CanSample() const { return false; }

	/**
	 * Gives all primitives in this aggregate.
	 * @param prims The destination list for the primitives.
	 */
	virtual void GetPrimitives(vector<boost::shared_ptr<Primitive> > &prims) = 0;
};


/**
 * A decorator for instances of primitives with interpolated matrices for motion blur.
 * This is achieved by changing the Object-to-World transformation
 * in the result and other transforming all intersection info that
 * was calculated, by interpolating between 2 matrices using the ray time.
 */
class MotionPrimitive : public Primitive {
public:
	// MotionPrimitive Public Methods
	/**
     * Creates a new instance from the given primitive.
     *
     * @param i   The primitive to instance.
     * @param i2ws The instance to world transformation at start time.
     * @param i2we The instance to world transformation at end time.
	 * @param s   The time at start.
     * @param e   The time at end.
     */
	MotionPrimitive(boost::shared_ptr<Primitive> i,
		const Transform &i2ws,
		const Transform &i2we,
		float s, float e )
	{
		instance = i;
		motionSystem = new MotionSystem(s, e, i2ws, i2we);
	}
	virtual ~MotionPrimitive() { delete motionSystem; }

    virtual BBox WorldBound() const;

    virtual bool CanIntersect() const { return instance->CanIntersect(); }
    virtual bool Intersect(const Ray &r, Intersection *in) const;
    virtual bool IntersectP(const Ray &r) const;

    virtual bool CanSample() const { return instance->CanSample(); }
    virtual float Area() const { return instance->Area(); }
    virtual void Sample(float u1, float u2, float u3, DifferentialGeometry *dg) const  {
            return instance->Sample(u1, u2, u3, dg);
    }
    virtual float Pdf(const Point &p) const { return instance->Pdf(p); }
    virtual void Sample(const Point &P, float u1, float u2, float u3, DifferentialGeometry *dg) const {
		instance->Sample(P, u1, u2, u3, dg);
	}
    virtual float Pdf(const Point &p, const Vector &wi) const {
        return instance->Pdf(p, wi);
     }
    virtual float Pdf(const Point &p, const Point &po) const {
        return instance->Pdf(p, po);
     }
private:
    // MotionPrimitive Private Data
    boost::shared_ptr<Primitive> instance;
	MotionSystem *motionSystem;
};


}//namespace lux

#endif // LUX_PRIMITIVE_H
