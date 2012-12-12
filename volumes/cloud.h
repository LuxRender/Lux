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

// Cloud.cpp*
#include "volume.h"
#include "texture.h"

namespace lux
{
//sphere for cumulus shape
class CumulusSphere {
public:
	void setPosition( Point p ) { position = p; };
	void setRadius( float r ) { radius = r; };
	Point getPosition() const { return position; };
	float getRadius() const { return radius; };

private:
	Point position;
	float radius;
};

// Cloud Volume
class CloudVolume : public DensityVolume<RGBVolume> {
public:
	// CloudVolume Public Methods
	CloudVolume(const RGBColor &sa, const RGBColor &ss, float gg,
		const RGBColor &emit, const BBox &e, const float &r,
		const Transform &v2w, const float &noiseScale, const float &t,
		const float &sharp, const float &v, const float &baseflatness,
		const u_int &octaves, const float &o, const float &offSet,
		const u_int &numspheres, const float &spheresize);
	virtual ~CloudVolume() {
		delete sphereCentre;
		delete[] spheres;
	}
			
	virtual float Density(const Point &p) const;
			
private:
	// CloudVolume Private Data
	bool SphereFunction(const Point &p) const;
	float CloudShape(const Point &p) const;
	float NoiseMask(const Point &p) const;
	Vector Turbulence(const Point &p, float noiseScale, u_int octaves) const;
	Vector Turbulence(const Vector &v, float &noiseScale, u_int &octaves) const;
	float CloudNoise(const Point &p, const float &omegaValue, u_int octaves) const;

	Transform VolumeToWorld;
	Vector scale;
	Point *sphereCentre;
	float inputRadius, radius;

	bool cumulus;
	u_int numSpheres;
	float sphereSize;
	CumulusSphere *spheres;

	float baseFadeDistance, sharpness, baseFlatness, variability;
	float omega, firstNoiseScale, noiseOffSet, turbulenceAmount;
	u_int numOctaves;
};

class Cloud {
public:
	static Region *CreateVolumeRegion(const Transform &volume2world, const ParamSet &params);
};

static inline float CloudRand(int resolution)
{
	return static_cast<float>(rand() % resolution) / resolution;
}

CloudVolume::CloudVolume(const RGBColor &sa, const RGBColor &ss,
	float gg, const RGBColor &emit, const BBox &e, const float &r,
	const Transform &v2w, const float &noiseScale, const float &t,
	const float &sharp, const float &v, const float &baseflatness,
	const u_int &octaves, const float &o, const float &offSet,
	const u_int &numspheres, const float &spheresize) :
	DensityVolume<RGBVolume>("CloudVolume-"  + boost::lexical_cast<string>(this),
		RGBVolume(sa, ss, emit, gg)),
	VolumeToWorld(v2w), inputRadius(r), numSpheres(numspheres),
	sphereSize(spheresize), sharpness(sharp), baseFlatness(baseflatness),
	variability(v), omega(o), firstNoiseScale(noiseScale),
	noiseOffSet(offSet), turbulenceAmount(t), numOctaves(octaves)
{
	if (numSpheres == 0)
		cumulus = false;
	else
		cumulus = true;

	// Radius begins as width of box
	radius = (e.pMax.x - e.pMin.x);

	firstNoiseScale *= radius;
	turbulenceAmount *= radius;

	// Multiply by size given by user
	radius *= inputRadius;

	baseFadeDistance = (e.pMax.z - e.pMin.z) *
		(1.f - baseFlatness);

	// Centre of main hemisphere shape
	sphereCentre = new Point((e.pMax.x + e.pMin.x) / 2.f,
		 (e.pMax.y + e.pMin.y) / 2.f,
		 (e.pMax.z + 2.f * e.pMin.z) / 3.f);

	if (cumulus) {
		//create spheres for cumulus shape. each should be at a point on the edge of the hemisphere
		spheres = new CumulusSphere[numSpheres];
		
		srand(noiseOffSet);
		
		for (u_int i = 0; i < numSpheres; ++i) {
			spheres[i].setRadius((CloudRand(10) / 2.f + 0.5f) *
				sphereSize);
			Vector onEdge = Vector(radius / 2.f * CloudRand(1000),
				0.f, 0.f);
			const float angley = -180.f * CloudRand(1000);
			const float anglez = 360.f * CloudRand(1000);
			onEdge = RotateZ(anglez) * (RotateY(angley) * onEdge);
			Point finalPosition = *sphereCentre + onEdge;
			finalPosition += Turbulence(finalPosition +
				Vector(noiseOffSet * 4.f, 0.f, 0.f),
				radius * 0.7f, 2) * radius * 1.5f;
			spheres[i].setPosition(finalPosition);
		}
	}
}

float CloudVolume::Density(const Point &p) const
{
	const Point pp(Inverse(VolumeToWorld) * p);
	float amount = CloudShape(pp +
		turbulenceAmount * Turbulence(pp, firstNoiseScale, numOctaves));

	float finalValue = powf(amount, sharpness) *
		powf(10.f, sharpness * 0.7f);

	return min(finalValue, 1.f);
}

Vector CloudVolume::Turbulence(const Point &p, float noiseScale, u_int octaves) const
{
	Point noiseCoords[3];
	noiseCoords[0] = Point(p.x / noiseScale, p.y / noiseScale, p.z / noiseScale);
	noiseCoords[1] = Point(noiseCoords[0].x + noiseOffSet, noiseCoords[0].y + noiseOffSet, noiseCoords[0].z + noiseOffSet);
	noiseCoords[2] = Point(noiseCoords[1].x + noiseOffSet, noiseCoords[1].y + noiseOffSet, noiseCoords[1].z + noiseOffSet);

	float noiseAmount = 1.f;

	if (variability < 1.f)
		noiseAmount = Lerp(variability, 1.f,
			NoiseMask(p + Vector(noiseOffSet * 4.f, 0.f, 0.f)));	//make sure 0 < noiseAmount < 1

	noiseAmount = Clamp(noiseAmount, 0.f, 1.f);

	Vector turbulence;

	turbulence.x = CloudNoise(noiseCoords[0], omega, octaves) - 0.15f;
	turbulence.y = CloudNoise(noiseCoords[1], omega, octaves) - 0.15f;
	if (p.z >= sphereCentre->z + baseFadeDistance)
		turbulence.z = -CloudNoise(noiseCoords[2], omega, octaves);
	else
		turbulence.z = -CloudNoise(noiseCoords[2], omega, octaves) *
			(p.z - sphereCentre->z) / baseFadeDistance / 2.f;	

	turbulence *= noiseAmount;	
		
	return turbulence;
}

Vector CloudVolume::Turbulence(const Vector &v, float &noiseScale, u_int &octaves) const
{
	return Turbulence(Point(v.x, v.y, v.z), noiseScale, octaves);
}

float CloudVolume::CloudShape(const Point &p) const
{
	if (cumulus) {
		if (SphereFunction(p))		//shows cumulus spheres
			return 1.f;
		else
			return 0.f;
	}

	Vector fromCentre(p - *sphereCentre);

	float amount = 1.f - fromCentre.Length() / radius;
	if (amount < 0.f)
		amount = 0.f;

	if (p.z < sphereCentre->z) {		//the base below the cloud's height fades out
		if (p.z < sphereCentre->z - radius * 0.4f)
			amount = 0.f;

		amount *= 1.f - cosf((fromCentre.z + baseFadeDistance) / baseFadeDistance * M_PI * 0.5f);   //cosine interpolation
	}
	return amount > 0.f ? amount : 0.f;
}

float CloudVolume::NoiseMask(const Point &p) const
{
	return CloudNoise(p / radius * 1.4f, omega, 1);
}

//returns whether a point is inside one of the cumulus spheres
bool CloudVolume::SphereFunction(const Point &p) const
{
	for (u_int i = 0; i < numSpheres; ++i) {
		if ((p-spheres[i].getPosition()).Length() < spheres[i].getRadius())
			return true;
	}
	return false;
}

float CloudVolume::CloudNoise(const Point &p, const float &omegaValue, u_int octaves) const
{
// Compute sum of octaves of noise
	float sum = 0.f, lambda = 1.f, o = 1.f;
	for (u_int i = 0; i < octaves; ++i) {
		sum += o * Noise(lambda * p);
		lambda *= 1.99f;
		o *= omegaValue;
	}
	return sum;
}

}//namespace lux

