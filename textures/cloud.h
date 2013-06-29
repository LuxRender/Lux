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

// cloud.h*
#include "lux.h"
#include "spectrum.h"
#include "texture.h"
#include "geometry/raydifferential.h"
#include "paramset.h"
#include "randomgen.h"

namespace lux
{

// Sphere for cumulus shape
class CumulusSphere {
public:
	Point position;
	float radius;
};

// CloudTexture Declarations
class CloudTexture : public Texture<float> {
public:
	// CloudTexture Public Methods
	CloudTexture(const float r, const float noiseScale, const float t,
		const float sharp, const float v, const float baseflatness,
		const u_int octaves, const float o, const float offset,
		const u_int numspheres, const float spheresize,
		TextureMapping3D *map) :
		Texture("CloudTexture-" + boost::lexical_cast<string>(this)),
		radius(r), numSpheres(numspheres), sphereSize(spheresize),
		sharpness(sharp), baseFlatness(baseflatness), variability(v),
		omega(o), firstNoiseScale(noiseScale), noiseOffset(offset),
		turbulenceAmount(t), numOctaves(octaves), mapping(map) {
		cumulus = numSpheres > 0;

		baseFadeDistance = 1.f - baseFlatness;

		// Centre of main hemisphere shape
		sphereCentre = Point(.5f, .5f, 1.f / 3.f);

		if (cumulus) {
			RandomGenerator rng(static_cast<unsigned long>(std::numeric_limits<unsigned long>::max() * noiseOffset));
			// Create spheres for cumulus shape.
			// Each one should be on the edge of the hemisphere
			spheres = new CumulusSphere[numSpheres];
		
			for (u_int i = 0; i < numSpheres; ++i) {
				spheres[i].radius = (CloudRand(rng, 10) / 2.f + 0.5f) *
					sphereSize;
				Vector onEdge(radius / 2.f * CloudRand(rng, 1000), 0.f, 0.f);
				const float angley = -180.f * CloudRand(rng, 1000);
				const float anglez = 360.f * CloudRand(rng, 1000);
				onEdge = RotateZ(anglez) * (RotateY(angley) * onEdge);
				Point finalPosition(sphereCentre + onEdge);
				finalPosition += Turbulence(finalPosition +
					Vector(noiseOffset * 4.f, 0.f, 0.f),
					radius * 0.7f, 2) * radius * 1.5f;
				spheres[i].position = finalPosition;
			}
		} else
			spheres = NULL;
	}
	virtual ~CloudTexture() {
		delete[] spheres;
		delete mapping;
	}
	virtual float Evaluate(const SpectrumWavelengths &sw,
		const DifferentialGeometry &dg) const {
		const Point P(mapping->Map(dg));
		const float amount = CloudShape(P +
			turbulenceAmount * Turbulence(P, firstNoiseScale, numOctaves));

		const float finalValue = powf(amount * powf(10.f, .7f), sharpness);

		return min(finalValue, 1.f);
	}
	virtual float Y() const { return .5f; }
	virtual void GetDuv(const SpectrumWavelengths &sw,
		const DifferentialGeometry &dg, float delta,
		float *du, float *dv) const {
		//FIXME: Generic derivative computation, replace with exact
		DifferentialGeometry dgTemp = dg;
		// Calculate bump map value at intersection point
		const float base = Evaluate(sw, dg);

		// Compute offset positions and evaluate displacement texture
		const Point origP(dgTemp.p);
		const Normal origN(dgTemp.nn);
		const float origU = dgTemp.u;

		// Shift _dgTemp_ _du_ in the $u$ direction and calculate value
		const float uu = delta / dgTemp.dpdu.Length();
		dgTemp.p += uu * dgTemp.dpdu;
		dgTemp.u += uu;
		dgTemp.nn = Normalize(origN + uu * dgTemp.dndu);
		*du = (Evaluate(sw, dgTemp) - base) / uu;

		// Shift _dgTemp_ _dv_ in the $v$ direction and calculate value
		const float vv = delta / dgTemp.dpdv.Length();
		dgTemp.p = origP + vv * dgTemp.dpdv;
		dgTemp.u = origU;
		dgTemp.v += vv;
		dgTemp.nn = Normalize(origN + vv * dgTemp.dndv);
		*dv = (Evaluate(sw, dgTemp) - base) / vv;
	}
	virtual void GetMinMaxFloat(float *minValue, float *maxValue) const {
		*minValue = 0.f;
		*maxValue = 1.f;
	}

	const TextureMapping3D *GetTextureMapping3D() const { return mapping; }

	static Texture<float> * CreateFloatTexture(const Transform &tex2world, const ParamSet &tp);
private:
	inline float CloudRand(RandomGenerator &rng, int resolution) const {
		return static_cast<float>(rng.uintValue() % resolution) / resolution;
	}

	Vector Turbulence(const Point &p, float noiseScale, u_int octaves) const
{
		Point noiseCoords[3];
		noiseCoords[0] = p / noiseScale;
		noiseCoords[1] = noiseCoords[0] +
			Vector(noiseOffset, noiseOffset, noiseOffset);
		noiseCoords[2] = noiseCoords[1] +
			Vector(noiseOffset, noiseOffset, noiseOffset);

		float noiseAmount = 1.f;

		if (variability < 1.f)
			noiseAmount = Lerp(variability, 1.f,
				NoiseMask(p + Vector(noiseOffset * 4.f, 0.f, 0.f)));

		noiseAmount = Clamp(noiseAmount, 0.f, 1.f);

		Vector turbulence;

		turbulence.x = CloudNoise(noiseCoords[0], omega, octaves) - 0.15f;
		turbulence.y = CloudNoise(noiseCoords[1], omega, octaves) - 0.15f;
		turbulence.z = -CloudNoise(noiseCoords[2], omega, octaves);
		if (p.z < sphereCentre.z + baseFadeDistance)
			turbulence.z *= (p.z - sphereCentre.z) /
				(2.f * baseFadeDistance);

		turbulence *= noiseAmount;	
		
		return turbulence;
	}

	Vector Turbulence(const Vector &v, float noiseScale, u_int octaves) const {
		return Turbulence(Point(v.x, v.y, v.z), noiseScale, octaves);
	}

	float CloudShape(const Point &p) const {
		if (cumulus) {
			if (SphereFunction(p))		//shows cumulus spheres
				return 1.f;
			else
				return 0.f;
		}

		const Vector fromCentre(p - sphereCentre);

		float amount = 1.f - fromCentre.Length() / radius;
		if (amount < 0.f)
			return 0.f;

		// The base below the cloud's height fades out
		if (p.z < sphereCentre.z) {
			if (p.z < sphereCentre.z - radius * 0.4f)
				return 0.f;

			amount *= 1.f - cosf((fromCentre.z + baseFadeDistance) /
				baseFadeDistance * M_PI * 0.5f);
		}
		return max(amount, 0.f);
	}

	float NoiseMask(const Point &p) const {
		return CloudNoise(p / radius * 1.4f, omega, 1);
	}

	// Returns whether a point is inside one of the cumulus spheres
	bool SphereFunction(const Point &p) const {
		for (u_int i = 0; i < numSpheres; ++i) {
			if ((p - spheres[i].position).Length() < spheres[i].radius)
				return true;
		}
		return false;
	}

	float CloudNoise(const Point &p, float omegaValue, u_int octaves) const {
		// Compute sum of octaves of noise
		float sum = 0.f, lambda = 1.f, o = 1.f;
		for (u_int i = 0; i < octaves; ++i) {
			sum += o * Noise(lambda * p);
			lambda *= 1.99f;
			o *= omegaValue;
		}
		return sum;
	}

	// CloudTexture Private Data
	Vector scale;
	Point sphereCentre;
	float radius;

	bool cumulus;
	u_int numSpheres;
	float sphereSize;
	CumulusSphere *spheres;

	float baseFadeDistance, sharpness, baseFlatness, variability;
	float omega, firstNoiseScale, noiseOffset, turbulenceAmount;
	u_int numOctaves;
	TextureMapping3D *mapping;
};


// CloudTexture Method Definitions
inline Texture<float> * CloudTexture::CreateFloatTexture(const Transform &tex2world,
	const ParamSet &tp) {
	// Read mapping coordinates
	TextureMapping3D *imap = TextureMapping3D::Create(tex2world, tp);
	float radius = tp.FindOneFloat("radius", 0.5f);
	float noiseScale = tp.FindOneFloat("noisescale", 0.5f);
	float turbulence = tp.FindOneFloat("turbulence", 0.01f);
	float sharpness = tp.FindOneFloat("sharpness", 6.0f);
	float offSet = tp.FindOneFloat("noiseoffset", 0.f);
	int numSpheres = tp.FindOneInt("spheres", 0);
	int octaves = tp.FindOneInt("octaves", 1);
	float omega = tp.FindOneFloat("omega", 0.75f);
	float variability = tp.FindOneFloat("variability", 0.9f);
	float baseflatness = tp.FindOneFloat("baseflatness", 0.8f);
	float spheresize = tp.FindOneFloat("spheresize", 0.15f);
	return new CloudTexture(radius, noiseScale, turbulence, sharpness,
		variability, baseflatness, octaves, omega, offSet, numSpheres,
		spheresize, imap);
}

}//namespace lux

