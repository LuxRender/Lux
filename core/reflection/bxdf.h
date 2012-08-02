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

#ifndef LUX_BXDF_H
#define LUX_BXDF_H
// bxdf.h*
#include "lux.h"
#include "geometry/raydifferential.h"
#include "geometry/transform.h"
#include "spectrum.h"

namespace lux
{

enum BxDFType {
	BSDF_REFLECTION   = 1<<0,
	BSDF_TRANSMISSION = 1<<1,
	BSDF_DIFFUSE      = 1<<2,
	BSDF_GLOSSY       = 1<<3,
	BSDF_SPECULAR     = 1<<4,
	BSDF_ALL_TYPES        = BSDF_DIFFUSE |
	                        BSDF_GLOSSY |
					        BSDF_SPECULAR,
	BSDF_ALL_REFLECTION   = BSDF_REFLECTION |
	                        BSDF_ALL_TYPES,
	BSDF_ALL_TRANSMISSION = BSDF_TRANSMISSION |
	                        BSDF_ALL_TYPES,
	BSDF_ALL              = BSDF_ALL_REFLECTION |
	                        BSDF_ALL_TRANSMISSION
};

/**
 * The BxDF abstract class represents a simple bidirectional scattering function.
 * BxDF objects will be composed to form more complex BSDF.
 * All vectors are in shaded surface local geometry, ie the z coordinate is
 * always along the shading normal.
 */
class  BxDF {
public:
	// BxDF Interface
	virtual ~BxDF() { }
	/**
	 * The BxDF constructor only takes the scattering type as argument.
	 * The type should have exactly one of BSDF_DIFFUSE, BSDF_GLOSSY or
	 * BSDF_SPECULAR and at most one of BSDF_REFLECTION or BSDF_TRANSMISSION.
	 * In most cases, BSDF_REFLECTION or BSDF_TRANSMISSION will be specified
	 * but for some special BxDF, this doesn't make sense so they won't
	 * specify any and will thus be able to scatter light in all directions.
	 * This can be the case for volumetric scattering BxDF or for some light
	 * or camera BxDF.
	 */
	BxDF(BxDFType t) : type(t) { }
	bool MatchesFlags(BxDFType flags) const {
		return (type & flags) == type;
	}
	/**
	 * Evaluates the BxDF.
	 * Accumulates the result in the f parameter.
	 * Compared to the commonly defined value (PBRT definition),
	 * the result is multiplied by Dot(ns, wl) so it is not symetrical!
	 * The rational is that this is always what ends up being done and in
	 * some cases it allows to avoid some computations and have a more
	 * stable result.
	 * @param sw The spectral values sampled
	 * @param wl The direction towards the light (mandatory)
	 * @param we The direction towards the eye (mandatory)
	 * @param f The accumulator for the contribution,
	 * don't forget to initialize it before the first call to BxDF::F
	 */
	virtual void F(const SpectrumWavelengths &sw, const Vector &wl,
		const Vector &we, SWCSpectrum *const f) const = 0;
	/**
	 * Samples a new direction according to the BxDF.
	 * Returns the result of the BxDF for the sampled direction in f.
	 * Compared to the commonly defined value (PBRT definition),
	 * the result is multiplied by Dot(ns, wi)/pdf if reverse==true
	 * and by Dot(ns. wo)/pdf if reverse==false.
	 * The rational is that this is always what ends up being done and in
	 * most cases it simplifies computaions and gives a more stable result,
	 * if the pdf correctly matches the BxDF the result won't depend anymore
	 * on the sampled direction.
	 * @param sw The spectral values sampled
	 * @param wo The known direction
	 * @param wi A non NULL pointer to store the sampled direction
	 * @param u1 First random variable in [0,1) to sample the direction
	 * @param u2 Second random variable in [0,1) to sample the direction
	 * @param f A non NULL pointer to store the BxDF value in the sampled
	 * direction
	 * @param pdf A non NULL pointer to store the probability of having
	 * sampled wi
	 * @param pdfBack A possibly NULL pointer to store the probability of
	 * sampling wo if wi were known
	 * @param reverse A flag to tell wether this a reverse light path
	 * (wo is towards the eye) or not (wo is towards the light)
	 * @return true if the sampling was successful, if false is returned
	 * wi, f, pdf and pdfBack should be considered undefined
	 */
	virtual bool SampleF(const SpectrumWavelengths &sw, const Vector &wo,
		Vector *wi, float u1, float u2, SWCSpectrum *const f,
		float *pdf, float *pdfBack = NULL, bool reverse = false) const;
	/**
	 * Computes the mean reflectance for a given direction.
	 * @param sw The spectral values sampled
	 * @param w The known direction
	 * @param nSamples The number of samples to be used if needed
	 * @param samples An array of 2*nSamples random values in [0,1)
	 * @return The mean reflectance
	 */
	virtual SWCSpectrum rho(const SpectrumWavelengths &sw, const Vector &w,
		u_int nSamples = 16, float *samples = NULL) const;
	/**
	 * Computes the mean reflectance for all possible directions.
	 * @param sw The spectral values sampled
	 * @param nSamples The number of samples to be used if needed
	 * @param samples An array of 4*nSamples random values in [0,1)
	 * @return The mean reflectance
	 */
	virtual SWCSpectrum rho(const SpectrumWavelengths &sw,
		u_int nSamples = 16, float *samples = NULL) const;
	/**
	 * A sampling weight to allow better BxDF selection from the BSDF given
	 * a direction.
	 * The value should be close to the amount of light expected to be
	 * scattered but should stay fast to compute.
	 * @param sw The spectral values sampled
	 * @param w The known direction
	 * @return a weight greater than 0
	 */
	virtual float Weight(const SpectrumWavelengths &sw, const Vector &w) const;
	/**
	 * The probability of sampling wi knowing wo.
	 * @param sw The spectral values sampled
	 * @param wo The known direction
	 * @param wi The sampled direction
	 * @return The probability of sampling wi knowing wo, 0 if impossible
	 */
	virtual float Pdf(const SpectrumWavelengths &sw, const Vector &wo,
		const Vector &wi) const;
	// BxDF Public Data
	const BxDFType type;
};

/**
 * The BSDF abstract class represents a full birectional scattering function.
 * It uses BxDF objects to define the various scattering components and allows
 * the user to select which components are to be used in the computation.
 * All vectors are in world coordinates.
 */
class  BSDF {
public:
	// BSDF Public Methods
	/**
	 * The BSDF constructor.
	 * @param dgs The differential shading geometry, used to convert world
	 * coordinates to surface local coordinates for the BxDF
	 * @param ngeom The geometric normal
	 * @param exterior The volume on the side pointed to by the geometric
	 * normal
	 * @param interior The volume on the opposite side
	 */
	BSDF(const DifferentialGeometry &dgs, const Normal &ngeom,
		const Volume *exterior, const Volume *interior);
	/**
	 * The number of BxDF composing the BSDF
	 */
	virtual u_int NumComponents() const = 0;
	/**
	 * The number of BxDF matches the given flags.
	 * @param flags Any combination of flags, only BxDF having all of their
	 * flags present in the flags parameter will be counted
	 * @return The number of BxDF matched by flags
	 */
	virtual u_int NumComponents(BxDFType flags) const = 0;
	/**
	 * Updates the compositing parameters block. Those paramters are
	 * currently only used by the distributed path integrator.
	 * WARNING: memory leak
	 * @param cp A pointer to the new compositing parameters block
	 */
	virtual inline void SetCompositingParams(const CompositingParams *cp) {
		compParams = cp;
	}
	/**
	 * Check if the surfaces uses normal tweaks (normal interpolation,
	 * bump mapping, ...
	 */
	bool HasShadingGeometry() const {
		return (dgShading.nn.x != ng.x || dgShading.nn.y != ng.y || dgShading.nn.z != ng.z);
	}
	/**
	 * Converts a vector in world space to a vector in local surface space
	 * @param wW The world vector
	 * @return The local vector
	 */
	Vector WorldToLocal(const Vector &wW) const {
		return Vector(Dot(wW, sn), Dot(wW, tn), Dot(wW, dgShading.nn));
	}
	/**
	 * Converts a vector in local surface space to a vector in world space
	 * @param w The local vector
	 * @return The world vector
	 */
	Vector LocalToWorld(const Vector &w) const {
		return Vector(sn * w.x + tn * w.y + Vector(dgShading.nn) * w.z);
	}
	/**
	 * Gets the volume corresponding to a direction relative to the
	 * geometric normal.
	 * @param wW The direction to check
	 * @return A pointer to the volume in the half space where w lies
	 */
	const Volume *GetVolume(const Vector &wW) const {
		return Dot(wW, ng) > 0.f ? exterior : interior;
	}
	/**
	 * Samples the BSDF.
	 * Returns the result of the BSDF for the sampled direction in f.
	 * Compared to the commonly used definition (PBRT definition)
	 * the result is multiplied by Dot(ns, wiW)/pdf if reverse==true
	 * and by Dot(ns,woW)*Dot(ng, wiW)/Dot(ng, woW)/pdf if reverse==false.
	 * In the reverse==true case, the rationale is that this is what
	 * always ends up being done and it allows computation simplifications
	 * in most cases and better numerical stability. Note that this is the
	 * same behaviour than BxDF.
	 * In the reverse==false case, the full multiplication factor is
	 * (Dot(ns, woW)/Dot(ng, woW))/(Dot(ns, wiW)/Dot(ng, wiW))*Dot(ns, woW)/pdf
	 * which is the same than the above factor after common factors
	 * elimination. The first part of the formula is a geometric correction
	 * factor to account for the fact that when using a shading normal the
	 * observed photon density (on the geometric surface) is not the
	 * expected photon density (what it would be on the shading surface).
	 * Note that in the reverse==true and reverse==false cases the
	 * remaining Dot(ns, w?W) factor corresponds to the cosine between the
	 * shading normal and the direction towards the light, this is the
	 * reason of the peculiar BxDF formulation.
	 * @param sw The spectral values sampled
	 * @param woW The known direction
	 * @param wiW A non NULL pointer to store the sampled direction
	 * @param u1 First random variable in [0,1) to sample the direction
	 * @param u2 Second random variable in [0,1) to sample the direction
	 * @param u3 Third random variable in [0,1) to sample the BxDF component
	 * @param f A non NULL pointer to store the BxDF value in the sampled
	 * direction
	 * @param pdf A non NULL pointer to store the probability of having
	 * sampled wiW
	 * @param flags The flags used to select the list of acceptable BxDF
	 * @param sampledType A possibly NULL pointer to return the type of
	 * the sampled BxDF
	 * @param pdfBack A possibly NULL pointer to store the probability of
	 * sampling woW if wiW were known
	 * @param reverse A flag to tell wether this a reverse light path
	 * (wo is towards the eye) or not (wo is towards the light)
	 * @return true if the sampling was successful, if false is returned
	 * wiW, f, pdf, sampledType and pdfBack should be considered undefined
	 */
	virtual bool SampleF(const SpectrumWavelengths &sw, const Vector &woW,
		Vector *wiW, float u1, float u2, float u3, SWCSpectrum *const f,
		float *pdf, BxDFType flags = BSDF_ALL,
		BxDFType *sampledType = NULL, float *pdfBack = NULL,
		bool reverse = false) const = 0;
	/**
	 * The probability of sampling wiW knowing woW.
	 * @param sw The spectral values sampled
	 * @param woW The known direction
	 * @param wiW The sampled direction
	 * @param flags The flags used to select the list of acceptable BxDF
	 * @return The probability of sampling wi knowing wo, 0 if impossible
	 */
	virtual float Pdf(const SpectrumWavelengths &sw, const Vector &woW,
		const Vector &wiW, BxDFType flags = BSDF_ALL) const = 0;
	/**
	 * Evaluates the BSDF.
	 * Compared to the commonly defined value (PBRT definition),
	 * the result is multiplied by Dot(ns, wlW) if reverse==true
	 * and by Dot(ns,wlW)*Dot(ng, weW)/Dot(ng, wlW) if reverse==false.
	 * In the reverse==true case, the rationale is that this is what
	 * always ends up being done and it allows computation simplifications
	 * in most cases and better numerical stability. Note that this is the
	 * same behaviour than BxDF.
	 * In the reverse==false case, the full multiplication factor is
	 * (Dot(ns, wlW)/Dot(ng, wlW))/(Dot(ns, weW)/Dot(ng, weW))*Dot(ns, wlW)/pdf
	 * which is the same than the above factor after common factors
	 * elimination. The first part of the formula is a geometric correction
	 * factor to account for the fact that when using a shading normal the
	 * observed photon density (on the geometric surface) is not the
	 * expected photon density (what it would be on the shading surface).
	 * @param sw The spectral values sampled
	 * @param wlW The direction towards the light (mandatory)
	 * @param weW The direction towards the eye (mandatory)
	 * @param flags The flags used to select the list of acceptable BxDF
	 * @return The BSDF value
	 */
	virtual SWCSpectrum F(const SpectrumWavelengths &sw, const Vector &wlW,
		const Vector &weW, bool reverse,
		BxDFType flags = BSDF_ALL) const = 0;
	/**
	 * Computes the mean reflectance for all directions.
	 * @param sw The spectral values sampled
	 * @param flags The flags used to select the list of acceptable BxDF
	 * @return The mean reflectance
	 */
	virtual SWCSpectrum rho(const SpectrumWavelengths &sw,
		BxDFType flags = BSDF_ALL) const = 0;
	/**
	 * Computes the mean reflectance for a given direction.
	 * @param sw The spectral values sampled
	 * @param wW The known direction
	 * @param flags The flags used to select the list of acceptable BxDF
	 * @return The mean reflectance
	 */
	virtual SWCSpectrum rho(const SpectrumWavelengths &sw, const Vector &wW,
		BxDFType flags = BSDF_ALL) const = 0;

	/**
	 * Apply a transformation to the BSDF
	 * This is useful for light instances.
	 * @param transform The transformation to be applied
	 * @return The volume defined by the transformed dpdu, dpdv and nn
	 */
	virtual float ApplyTransform(const Transform &transform);

	// BSDF Public Data
	/**
	 * @var const Normal nn
	 * @brief The shading normal
	 */
	/**
	 * @var const Normal ng
	 * @brief The geometric normal
	 */
	Normal ng;
	PartialDifferentialGeometry dgShading; /** The differential shading geometry */
	/**
	 * @var const Volume *exterior
	 * @brief The volume in the half space containing the geometric normal
	 */
	/**
	 * @var const Volume *interior
	 * @brief The volume in the opposite half space
	 */
	const Volume *exterior, *interior;

	const CompositingParams *compParams; /** Compositing parameters pointer */
	
protected:
	// BSDF Private Methods
	virtual ~BSDF() { }
	// BSDF Private Data
	/**
	 * @var Vector sn
	 * @brief The first coordinate axis for local space
	 */
	/**
	 * @var Vector tn
	 * @brief The second coordinate axis for local space
	 */
	Vector sn, tn;
};

}//namespace lux

#endif // LUX_BXDF_H
