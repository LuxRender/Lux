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
	class Cloud : public DensityRegion {
		public:
	// Cloud Public Methods
			Cloud(const RGBColor &sa, const RGBColor &ss,
					float gg, const RGBColor &emit, const BBox &e, const float &r, const Transform &v2w, 
					const float &noiseScale, const float &t, const float &sharp, const float &v, const float &baseflatness, 
					const int &octaves, const float &o, const float &offSet, const int &numspheres, const float &spheresize );
			virtual ~Cloud() {
				delete sphereCentre;
				delete[] spheres;
			}
			
			virtual bool IntersectP(const Ray &r, float *t0, float *t1) const {
				Ray ray = WorldToVolume(r);
				return extent.IntersectP(ray, t0, t1);
			}
			virtual BBox WorldBound() const { return WorldToVolume.GetInverse()(extent); }
			virtual float Density(const Point &p) const;
			static VolumeRegion *CreateVolumeRegion(const Transform &volume2world, const ParamSet &params);
			
		private:
	// Cloud Private Data
			bool SphereFunction(const Point &p ) const;
			float CloudShape(const Point &p) const;
			float NoiseMask(const Point &p ) const;
			Vector Turbulence(const Point p, float noiseScale, int octaves) const;
			Vector Turbulence(const Vector &v, float &noiseScale, int &octaves) const;
			float CloudNoise(Point p, const float &omegaValue, int octaves) const;
			
			BBox extent;
			Vector scale;
			Point *sphereCentre;
			float inputRadius, radius;
			
			bool cumulus;
			int numSpheres;
			float sphereSize;
			CumulusSphere *spheres;
			
			float baseFadeDistance, sharpness, baseFlatness, variability;
			float omega, firstNoiseScale, noiseOffSet, turbulenceAmount;
			int numOctaves;
	};

	Cloud::Cloud(const RGBColor &sa, const RGBColor &ss,
						float gg, const RGBColor &emit, const BBox &e, const float &r,
						const Transform &v2w, const float &noiseScale, const float &t, const float &sharp, const float &v, 
						const float &baseflatness, const int &octaves, const float &o, const float &offSet, const int &numspheres, const float &spheresize )
							: DensityRegion(sa, ss, gg, emit, v2w),
						extent(e), inputRadius(r), numSpheres(numspheres), sphereSize(spheresize), sharpness(sharp),baseFlatness(baseflatness), variability(v),
						omega(o), firstNoiseScale(noiseScale), noiseOffSet(offSet), turbulenceAmount(t), numOctaves(octaves) { 
		
		if( numSpheres == 0 )
			cumulus = false;
		else
			cumulus = true;
		
		radius = (extent.pMax.x-extent.pMin.x);	//radius begins as width of box
		
		firstNoiseScale *= radius;
		turbulenceAmount *= radius;
		
		radius *= inputRadius;	//multiply by size given by user
		
		baseFadeDistance = float(extent.pMax.z-extent.pMin.z)*(1.0-baseFlatness);
	
		sphereCentre = new Point( 
														 (extent.pMax.x+extent.pMin.x)*0.5, 
														 (extent.pMax.y+extent.pMin.y)*0.5,
														 (extent.pMax.z+2*extent.pMin.z)*0.33 );	//centre of main hemisphere shape
		
		float angley, anglez;	//for positioning spheres in random spots around the main cloud hemisphere
		Vector onEdge;				//the position of the cumulus sphere relative to the position of the main hemisphere
		Point finalPosition;
				
		if( cumulus )
		{
			//create spheres for cumulus shape. each should be at a point on the edge of the hemisphere
			spheres = new CumulusSphere[numSpheres];
		
			srand(int(noiseOffSet));
		
			for( int i = 0; i < numSpheres; i++ )
			{
				spheres[i].setRadius( (float(rand()%10)*0.05+0.5)*sphereSize );
				onEdge = Vector( radius*0.5*(rand()%1000)/1000.0, 0, 0 );
				angley = float(rand()%1000)/1000.0*(-180);
				anglez = float(rand()%1000)/1000.0*360;
				onEdge = Vector(RotateY(angley)(onEdge));
				onEdge = Vector(RotateZ(anglez)(onEdge));
				finalPosition = Point(sphereCentre->x+onEdge.x,sphereCentre->y+onEdge.y,sphereCentre->z+onEdge.z);
				finalPosition += Turbulence( finalPosition + Point(noiseOffSet*4,0,0), radius*0.7, 2 )*radius*1.5;
				spheres[i].setPosition( finalPosition );
			}
		}
	}
												
	float Cloud::Density(const Point &p) const {
		if (!extent.Inside(p)) return 0;
		float amount;
				
		amount = CloudShape( p + turbulenceAmount * Turbulence(p,firstNoiseScale, numOctaves) );
		
		float finalValue = pow(amount,sharpness)*pow(10,sharpness*0.7);
		
		return finalValue <= 1.0 ? finalValue : 1.0;
	}

	Vector Cloud::Turbulence(const Point p, float noiseScale, int octaves) const {
		
		Point noiseCoords[3];
		noiseCoords[0] = Point( p.x/noiseScale, p.y/noiseScale, p.z/noiseScale );
		noiseCoords[1] = Point( noiseCoords[0].x+noiseOffSet, noiseCoords[0].y+noiseOffSet, noiseCoords[0].z+noiseOffSet );
		noiseCoords[2] = Point( noiseCoords[1].x+noiseOffSet, noiseCoords[1].y+noiseOffSet, noiseCoords[1].z+noiseOffSet );
		
		float noiseAmount = 1.0;
		
		if( variability < 1.0 )
			noiseAmount = (1.0-variability)+variability*NoiseMask( p + Point(noiseOffSet*4,0,0) );	//make sure 0 < noiseAmount < 1
		
		if( noiseAmount > 1.0 ) noiseAmount = 1.0;
		if( noiseAmount < 0.0 ) noiseAmount = 0.0;
		
		Vector turbulence;
		
		turbulence.x =  (CloudNoise(noiseCoords[0], omega, octaves)-0.15);		
		
		turbulence.y =  (CloudNoise(noiseCoords[1], omega, octaves)-0.15);
		
		if( p.z >= sphereCentre->z + baseFadeDistance )
			turbulence.z = -CloudNoise(noiseCoords[2], omega, octaves)*1.0;
		else
			turbulence.z = -CloudNoise(noiseCoords[2], omega, octaves)*1.0 *	(p.z-sphereCentre->z)/baseFadeDistance/2;
										
		
			turbulence *= noiseAmount;	
		
		return turbulence;
	}
	
	Vector Cloud::Turbulence(const Vector &v, float &noiseScale, int &octaves) const {
		return Turbulence(Point(v.x,v.y,v.z),noiseScale,octaves);
	}
			
	
	float Cloud::CloudShape(const Point &p) const {
		if( cumulus )
		{
			if( SphereFunction( p ) )		//shows cumulus spheres
				return 1.0;
			else
				return 0.0;
		}
		
		
		Vector fromCentre(
											float(p.x-sphereCentre->x),
											 float(p.y-sphereCentre->y),
												float(p.z-sphereCentre->z)
										 );
		
		float amount = 1.0 - fromCentre.Length() / radius;
		if( amount < 0.0 )	amount = 0.0;
													
		if( p.z < sphereCentre->z )		//the base below the cloud's height fades out
		{
			if( p.z < sphereCentre->z-radius*0.4 )
				amount = 0.0;
					
			amount *= 1.0-cos(float(p.z-sphereCentre->z+baseFadeDistance)/float(baseFadeDistance)*3.1415927*0.5);   //cosine interpolation
		}
		return amount > 0.0 ? amount : 0.0;
	}
	
	float Cloud::NoiseMask(const Point &p ) const {
		return CloudNoise(p/radius*1.4,omega,1);
	}
	
	//returns whether a point is inside one of the cumulus spheres
	bool Cloud::SphereFunction(const Point &p ) const {
		for( int i = 0; i < numSpheres; i++ )
		{
			if( Vector(p-spheres[i].getPosition()).Length() < spheres[i].getRadius() )
				return true;
		}
		return false;
	}
	
	float Cloud::CloudNoise( Point p, const float &omegaValue, int octaves) const {
	// Compute sum of octaves of noise
		float sum = 0., lambda = 1., o = 1.;
		for (int i = 0; i < octaves; ++i) {
			sum += o * Noise(lambda * p);
			lambda *= 1.99f;
			o *= omegaValue;
			}
		return sum;
	}

}//namespace lux

