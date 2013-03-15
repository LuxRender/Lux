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

#ifndef LUX_CAMERA_H
#define LUX_CAMERA_H
// camera.h*
#include "lux.h"
#include "geometry/transform.h"
#include "motionsystem.h"
#include "queryable.h"

namespace lux
{
// Camera Declarations
/**
 * The base Camera class, all camera models should be derived from this class
 */
class  Camera {
public:
	// Camera Interface
	/**
	 * Camera constructor
	 * @param w2c The world to camera transform, possibly animated
	 * @param hither The lower clipping distance
	 * @param yon The farther clipping distance
	 * @param sopen Shutter open time in seconds
	 * @param sclose Sutter closure time in seconds
	 * @param sdist The shutter sampling mode TODO: list modes
	 * @param film A pointer to the film
	 */
	Camera(const MotionSystem &w2c, float hither, float yon, 
		float sopen, float sclose, int sdist, Film *film);
	virtual ~Camera() { }
	virtual void AddAttributes(Queryable *q) const;
	/**
	 * Returns the volume the camera is in.
	 */
	const Volume *GetVolume() const { return volume.get(); }
	/**
	 * Sets the volume the camera is in.
	 *
	 * @param v A shared pointer to the volume definition
	 */
	void SetVolume(boost::shared_ptr<Volume> &v) {
		// Create a temporary to increase shared count
		// The assignment is just a swap
		boost::shared_ptr<Volume> vol(v);
		volume = vol;
	}
	/**
	 * Simple way to generate a ray.
	 * This interface is no longer the prefered way to sample a ray,
	 * you should instead use SampleW to sample the ray origin and use
	 * the returned BSDF to sample the ray direction.
	 *
	 * @param scene The current scene being rendered
	 * @param sample The sample to be used to generate the ray
	 * @param ray A pointer to return the sampled ray
	 * @param x The sampled x position on screen in pixels
	 * @param y The sampled y position on screen in pixels
	 * @return he ray weighting
	 */
	float GenerateRay(const Scene &scene, const Sample &sample,
		Ray *ray, float *x, float *y) const;
	/**
	 * Samples the origin of a ray.
	 * Depending on the value returned by IsLensBased(), the expected values
	 * for parameters u1 and u2 will differ:
	 * - if IsLensBased() returns true, u1 and u2 are expected to be the
	 *   lens sampling values u and v in the [0,1) range
	 * - if IsLensBased() returns false, u1 and u2 are expected to be the
	 *   screen sampling values x and y in pixels
	 *
	 * @param arena A memory arena to allocate the camera BSDF
	 * @param sw The definition of the sampled wavelengths
	 * @param scene The current scene being rendered
	 * @param u1 First sampling random variable (see above for details)
	 * @param u2 Second sampling random variable (see above for details)
	 * @param u3 Component sampling random variable, currently not used by
	 * any camera model, added to match Light::SampleL prototype
	 * @param bsdf A pointer to return a BSDF pointer to the camera transfer
	 * function, usually allocated in the arena, so it shouldn't be kept
	 * longer than the arena; (*bsdf)->dgShading.p is the ray origin
	 * @param pdf A pointer to return the probability of having sampled that
	 * point in m^-2 or 1 if IsDelta() returns true
	 * @param We A pointer to a SWCSpectrum to return the camera energy
	 * filtering divided by *pdf
	 * @return true if the sampling is correct and the returned values
	 * can be used, false otherwise
	 */
	virtual bool SampleW(MemoryArena &arena, const SpectrumWavelengths &sw,
		const Scene &scene, float u1, float u2, float u3, BSDF **bsdf,
		float *pdf, SWCSpectrum *We) const = 0;
	/**
	 * Samples the origin of a ray So that a given point can be seen if not
	 * otherwise occluded. That's next event estimation for the light path
	 * towards the camera.
	 * Depending on the value returned by IsLensBased(), the expected values
	 * for parameters u1 and u2 will differ:
	 * - if IsLensBased() returns true, u1 and u2 are expected to be the
	 *   lens sampling values u and v in the [0,1) range
	 * - if IsLensBased() returns false, u1 and u2 are expected to be the
	 *   screen sampling values x and y in pixels
	 *
	 * @param arena A memory arena to allocate the camera BSDF
	 * @param sw The definition of the sampled wavelengths
	 * @param scene The current scene being rendered
	 * @param p The reference point that should be seen
	 * @param n The surface normal at p
	 * @param u1 First sampling random variable (see above for details)
	 * @param u2 Second sampling random variable (see above for details)
	 * @param u3 Component sampling random variable, currently not used by
	 * any camera model, added to match Light::SampleL prototype
	 * @param bsdf A pointer to return a BSDF pointer to the camera transfer
	 * function, usually allocated in the arena, so it shouldn't be kept
	 * longer than the arena; (*bsdf)->dgShading.p is the ray origin
	 * @param pdf A pointer to return the probability of having sampled that
	 * point in m^-2 or 1 if IsDelta() returns true, but without accounting
	 * for p; pdf can be NULL if the caller is not interested in the value
	 * @param pdfDirect The probability of having sampled that point in m^-2
	 * or 1 if IsDelta() returns true, accounting for p
	 * @param We A pointer to a SWCSpectrum to return the camera energy
	 * filtering divided by *pdfDirect
	 * @return true if the sampling is correct and the returned values
	 * can be used, false otherwise
	 */
	virtual bool SampleW(MemoryArena &arena, const SpectrumWavelengths &sw,
		const Scene &scene, const Point &p, const Normal &n,
		float u1, float u2, float u3, BSDF **bsdf, float *pdf,
		float *pdfDirect, SWCSpectrum *We) const = 0;
	/**
	 * Checks the on screen position of a point.
	 * This method will be used to connect a light path vertex to an already
	 * sampled point on the camera. the direction and distance are thus
	 * easily computed by the caller and will ease depth of field
	 * computations.
	 *
	 * @param p The point to check against the camera
	 * @param wi The direction along which the point is to be seen
	 * @param distance The distance between p and the camera
	 * @param x A pointer to return the screen x position in pixels
	 * @param y A ppointer to return the screen y position in pixels
	 * @return true if the point is visible and honours the hither and yon
	 * constraints (if applicable), false otherwise
	 */
	virtual bool GetSamplePosition(const Point &p, const Vector &wi,
		float distance, float *x, float *y) const = 0;
	/**
	 * If applicable, modifies the ray mint and maxt members to honour
	 * the hither and yon parameters.
	 *
	 * @param ray The ray to update
	 */
	virtual void ClampRay(Ray &ray) const { }
	/**
	 * Returns true if all sampled ray origins will be identical,
	 * false otherwise.
	 */
	virtual bool IsDelta() const = 0;
	/**
	 * Returns wether the lens random variables or the screen random
	 * variables will be used to sample the ray origin (see SampleW).
	 * @return true if lens variables should be used (perspective camera),
	 * false if screen variables should be used (orthographic camera)
	 */
	virtual bool IsLensBased() const = 0;
	/**
	 * Updates the camera depth of field values so that the provided scene
	 * will be in focus.
	 *
	 * @param scene The current scene being rendered
	 */
	virtual void AutoFocus(const Scene &scene) { }
	/**
	 * Returns the bounding box of all possible ray origins for this camera.
	 */
	virtual BBox Bounds() const = 0;

	/**
	 * Samples a time value in the shutter open to shutter closure interval.
	 *
	 * @param u1 A random variable in the [0,1) range
	 * @return A time value in seconds in the [ShutterOpen,ShuterClose) range
	 */
	float GetTime(float u1) const;

	/**
	 * Updates the world to camera transform for a given time.
	 *
	 * @param time The time at which the transform should be evaluated in
	 * seconds
	 */
	virtual void SampleMotion(float time);

	/**
	 * Due to the fact that the camera holds a reference to the world to
	 * camera transform at a given time, it has to be cloned for each
	 * sample to avoid multithread issues. This members allows such cloning
	 * to happen.
	 *
	 * @return A pointer to a new camera structure identical to this one
	 */
	virtual Camera* Clone() const = 0;

	// Camera Public Data
	Film *film;
	Transform CameraToWorld;
protected:
	bool GenerateRay(MemoryArena &arena, const SpectrumWavelengths &sw,
		const Scene &scene, float o1, float o2, float d1, float d2,
		Ray *ray) const;
	// Camera Protected Data
	MotionSystem CameraMotion;
	float ClipHither, ClipYon;
	float ShutterOpen, ShutterClose;
	int ShutterDistribution;
	boost::shared_ptr<Volume> volume;
};
class  ProjectiveCamera : public Camera {
public:
	// ProjectiveCamera Public Methods
	ProjectiveCamera(const MotionSystem &world2cam,
		const Transform &proj, const float screen[4],
		float hither, float yon,
		float sopen, float sclose, int sdist,
		float lensr, float focald, Film *film);
	virtual ~ProjectiveCamera() { }
	virtual void AddAttributes(Queryable *q) const;

	virtual void SampleMotion(float time);

protected:
	// ProjectiveCamera Protected Data
	Transform ScreenToCamera, ScreenToWorld;
	Transform RasterToScreen, RasterToWorld;

	float ScreenWindow[4];

public:
	Transform RasterToCamera;
	float LensRadius, FocalDistance;
};

class SceneCamera : public Queryable {
public:
	SceneCamera(Camera *cam);
	~SceneCamera();
	Camera *operator()() const { return camera; }

private:
	Camera *camera;
};

}//namespace lux

#endif // LUX_CAMERA_H
