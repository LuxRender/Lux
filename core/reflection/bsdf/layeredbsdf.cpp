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

// layeredbsdf.cpp*
#include "layeredbsdf.h"
#include "spectrum.h"
#include "spectrumwavelengths.h"
#include "mc.h"
#include "sampling.h"
#include "fresnel.h"
#include "volume.h"


using namespace lux;

// LayeredBSDF Method Definitions

LayeredBSDF::LayeredBSDF(const DifferentialGeometry &dgs, const Normal &ngeom,
	const Volume *exterior, const Volume *interior) :
	BSDF(dgs, ngeom, exterior, interior)
{
	nBSDFs = 0;
	maxNumBounces = 1; // Note this gets changed when layers are added
	probSampleSpec = .5f;
}

bool LayeredBSDF::SampleF(const SpectrumWavelengths &sw, const Vector &known, Vector *sampled,
	float u1, float u2, float u3, SWCSpectrum *const f_, float *pdf,
	BxDFType flags, BxDFType *sampledType, float *pdfBack,
	bool reverse) const
{
	// Currently this checks to see if there is an interaction with the layers based on the opacity settings
	// If there is no interaction - let the ray pass through and return a specular
	// Otherwise returns a glossy (regardless of underlying layer types)

	// Have two possible types to return - glossy or specular - see if either/both have been requested
	
	bool glossy= (flags & BSDF_GLOSSY) ? true : false;
	bool specular= (flags & BSDF_SPECULAR) ? true : false;

	bool reflect= (flags & BSDF_REFLECTION) ? true : false;
	bool transmit= (flags & BSDF_TRANSMISSION) ? true : false;

	if (!reflect && !transmit)
		return false;

	*pdf = 1.f;
	if (pdfBack)
		*pdfBack = 1.f;
	
	if (glossy && specular) { // then choose one
		if (u3 < probSampleSpec) {
			glossy = false;	// just do specular
			*pdf *= probSampleSpec;
			u3 /= probSampleSpec;
		} else {
			specular = false;	// just do glossy
			*pdf *= 1.f - probSampleSpec;
			u3 = (u3 - probSampleSpec) / (1.f - probSampleSpec);
		}
	}
	
	if (glossy) { // then random sample hemisphere and return F() value

		*sampled = UniformSampleHemisphere(u1, u2);
		
		bool doReflect = true;	// indicates if sampled ray is reflected/transmitted
		// adjust orientation
		if (transmit) {
			if (reflect) {
				*pdf *= .5f;
				if (u3 < .5f) // randomly pick one
					doReflect = false;	// transmit
			} else
				doReflect = false;
		}
		if (doReflect) { //adjust so that sampled ray is in the same hemisphere as known
			if (Dot(ng, known) < 0.f)
				sampled->z *= -1.f;
		} else { // make sure they're in different hemispheres
			if (Dot(ng, known) > 0.f)
				sampled->z *= -1.f;
		}
		
		*sampled = LocalToWorld(*sampled);	// convert to world coords

		// create flags and check if they match requested flags
		BxDFType flags2;
		if (doReflect) // calculate a reflected component
			flags2 = BxDFType(BSDF_GLOSSY|BSDF_REFLECTION);
		else // calculate a transmitted component
			flags2 = BxDFType(BSDF_GLOSSY|BSDF_TRANSMISSION);

		*pdf *= INV_TWOPI;

		if (pdfBack)
			*pdfBack = *pdf;

		if (reverse)
			*f_ = F(sw, *sampled, known, reverse, flags2);	// automatically includes any reverse==true/false adjustments
		else
			*f_ = F(sw, known, *sampled, reverse, flags2);	// automatically includes any reverse==true/false adjustments
		*f_ /= *pdf;

		if (sampledType)
			*sampledType = flags2;
		return true;
	}

	if (specular) { // then random sample a specular path and return it
		*f_ = SWCSpectrum(1.f);;

		int curLayer= (Dot(ng, known)<0) ? nBSDFs - 1 : 0;	// figure out which layer to hit first - back or front
		
		float pdfForward = 1.f;
		float pdfBackward = 1.f;
		
		SWCSpectrum newF(0.f);
		Vector curVin = known;
		Vector curVout = Vector();	

		// bounce around and calc accumlated L
		RandomGenerator rng(GetRandSeed());
		bool valid = false;
		for (int count = 0; count <= maxNumBounces * 2; ++count) {
			// try and get the next sample
			// u1 and u2 don't matter with specular BSDF
			if (bsdfs[curLayer]->SampleF(sw, curVin , &curVout,
				.5f, .5f, rng.floatValue(), &newF, &pdfForward,
				BxDFType(BSDF_SPECULAR|BSDF_TRANSMISSION|BSDF_REFLECTION), NULL , &pdfBackward, reverse)) { // then sampled ok
				*f_ *= newF;
				*pdf *= pdfForward;
				if (pdfBack)
					*pdfBack *= pdfBackward;
			} else // no valid sample
				return false;	// exit - no valid sample

			// calc next layer and adjust directions
			if (Dot(ng, curVout) > 0.f)
				--curLayer;	// ie, if woW is in same direction as normal - decrement layer index
			else
				++curLayer;
			if (curLayer < 0 ||
				curLayer >= static_cast<int>(nBSDFs)) {
				valid = true;
				break; // have reached the exit - stop bouncing
			}

			curVin = -curVout;	
		}
		if (!valid)
			return false;
		// ok, so if we get here then we've got a valid L - tidy up and exit

		*sampled = curVout;	
		
		// create flags and check if they match requested flags
		BxDFType flags2;
		if (reflect)	// calculate a reflected component
			flags2 = BxDFType(BSDF_SPECULAR|BSDF_REFLECTION);
		else // calculate a transmitted component
			flags2 = BxDFType(BSDF_SPECULAR|BSDF_TRANSMISSION);

		if (sampledType)
			*sampledType= flags2;
		return true;
	}
	return false; // no sample

}

SWCSpectrum LayeredBSDF::F(const SpectrumWavelengths &sw, const Vector &woW,
		const Vector &wiW, bool reverse, BxDFType flags) const
{
	// Uses a modifed bidir to sample the layers
	// by convention woW is the light direction, wiW is the eye direction 
	// so reverse==false corresponds to the physical situation of light->surface->eye (PBRT default)

	if ((flags & BSDF_GLOSSY) == 0) // nothing to sample
		return SWCSpectrum(0.f);

	// Create storage for incoming/outgoing paths
	vector<int> eyeLayer;
	vector<Vector> eyeVector;
	vector<float> eyePdfForward;
	vector<float> eyePdfBack;
	vector<SWCSpectrum> eyeL;
	vector<BxDFType> eyeType;

	vector<int> lightLayer;
	vector<Vector> lightVector;
	vector<float> lightPdfForward;
	vector<float> lightPdfBack;
	vector<SWCSpectrum> lightL;
	vector<BxDFType> lightType;

	// now create the two paths
	int lightIndex = (Dot(ng,woW) < 0.f) ? nBSDFs - 1 : 0;
	int eyeIndex = (Dot(ng, wiW) < 0.f) ? nBSDFs - 1 : 0;

	GetPath(sw, woW, lightIndex, &lightL, &lightVector, &lightLayer,
		&lightPdfForward, &lightPdfBack, &lightType); //light
	GetPath(sw, wiW, eyeIndex, &eyeL, &eyeVector, &eyeLayer,
		&eyePdfForward, &eyePdfBack, &eyeType); //eye

	// now connect them
	SWCSpectrum L(0.f);	// this is the accumulated L value for the current path 
	SWCSpectrum newF(0.f);

	const u_int maxVertices = eyeLayer.size() + lightLayer.size() - 1;
	float* fwdProb = new float[maxVertices];
	float* backProb = new float[maxVertices];
	bool* spec = new bool[maxVertices];
	for (size_t i = 0; i < eyeLayer.size(); ++i) { // for every vertex in the eye path
		for (size_t j = 0;j < lightLayer.size(); ++j) { // try to connect to every vert in the light path
			if (eyeLayer[i] != lightLayer[j]) // then pass the "visibility test" so connect them
				continue;
			int curLayer = eyeLayer[i];
			// First calculate the total L for the path

			SWCSpectrum Lpath = bsdfs[curLayer]->F(sw, eyeVector[i], lightVector[j], true, BxDFType(BSDF_ALL)) / AbsDot(eyeVector[i], bsdfs[curLayer]->dgShading.nn); // calc how much goes between them
			// NOTE: used reverse==True to get F=f*|wo.ns| = f * cos(theta_in)

			Lpath = eyeL[i] * Lpath * lightL[j];

			if (Lpath.Black())	// if it is black we may have a specular connection
				continue;
			float pgapFwd = bsdfs[curLayer]->Pdf(sw, lightVector[j], eyeVector[i], BxDFType(BSDF_ALL)); // should be prob of sampling eye vector given light vector
			float pgapBack = bsdfs[curLayer]->Pdf(sw, eyeVector[i], lightVector[j], BxDFType(BSDF_ALL));

			// Now calc the probability of sampling this path (surely there must be a better way!!!)
			float totProb = 0.f;
			float pathProb = 1.f;

			// construct the list of fwd/back probs
			for (size_t k = 0; k < j; ++k) {
				fwdProb[k] = lightPdfForward[k + 1];
				backProb[k] = lightPdfBack[k + 1];
				spec[k] = (BSDF_SPECULAR & lightType[k + 1]) != 0;
			}
			fwdProb[j] = pgapFwd;
			backProb[j] = pgapBack;
			spec[j] = false;		// if this is true then the bsdf above will ==0 and cancel it out anyway
			for (size_t k = 0; k < i; ++k) {	
				fwdProb[j + i - k] = eyePdfBack[k];
				backProb[j + i - k] = eyePdfForward[k];
				spec[j + i - k] = (BSDF_SPECULAR & eyeType[k]) != 0;
			}

			for (size_t join = 0; join <= i + j; ++join) {
				if (spec[join]) // can't use it -  if these terms are specular then the delta functions make the L term 0
					continue;
				float curProb = 1.f;
				for (size_t k = 0; k < join; ++k)
					curProb *= fwdProb[k];
				for (size_t k = join + 1; k <= i + j; ++k)
					curProb *= backProb[k];
				totProb += curProb;
				if (join == j)
					pathProb = curProb;
			}
			if (totProb > 0.f)
				L += Lpath * (pathProb / totProb);
		}
	}
	delete[] fwdProb;
	delete[] backProb;
	delete[] spec;

	// Apply the geometric correction factor if the result is used for
	// a light path (reverse=false)
	if (!reverse)
		L *= fabsf(Dot(wiW, ng) / Dot(woW, ng));
	return L * AbsDot(woW, dgShading.nn);
}

float LayeredBSDF::ApplyTransform(const Transform &transform)
{
	for (u_int i = 0; i < nBSDFs; ++i)
		bsdfs[i]->ApplyTransform(transform);
	return this->BSDF::ApplyTransform(transform);
}

int LayeredBSDF::GetPath(const SpectrumWavelengths &sw, const Vector &vin,
	const int startIndex, vector<SWCSpectrum> *pathL,
	vector<Vector> *pathVec, vector<int> *pathLayer, 
	vector<float> *pathPdfForward, vector<float> *pathPdfBack,
	vector<BxDFType> *pathSampleType) const
{
	// returns the number of bounces used in this path
	// if eye==TRUE we are calculating the eye path

	RandomGenerator rng(GetRandSeed());

	int curLayer = startIndex;	// curlayer is the layer we will be sampling
	Vector curVin = vin;
	Vector curVout = Vector();				
	float pdfForward = 1.f;
	float pdfBack = 1.f;
	BxDFType sampledType = BSDF_GLOSSY;	// anything but specular really

	SWCSpectrum L(1.f);	// this is the accumulated L value for the current path 

	for (int i = 0; i < maxNumBounces; ++i) {	// this will introduce a small bias ?Need to fix with russian roulette
		if (curLayer < 0 || curLayer >= static_cast<int>(nBSDFs)) // have exited the material
			return i;

		// STORE THE CURRENT PATH INFO
		pathL->push_back(L);
		pathVec->push_back(curVin);
		pathLayer->push_back(curLayer);
		pathPdfForward->push_back(pdfForward);
		pathPdfBack->push_back(pdfBack);
		pathSampleType->push_back(sampledType);

		// get the next sample

		SWCSpectrum newF(0.f);

		// geometric correction factor for light paths is not applicable so always sample with reverse=true
		if (!bsdfs[curLayer]->SampleF(sw, curVin , &curVout,
			rng.floatValue(), rng.floatValue() ,rng.floatValue(),
			&newF , &pdfForward , BxDFType(BSDF_ALL),
			&sampledType, &pdfBack, true)) // then couldn't sample
				return i;

		L *= newF;

		if (Dot(ng, curVout) > 0.f)
			--curLayer;	// ie, if woW is in same direction as normal - decrement layer index
		else
			++curLayer;
		curVin = -curVout;
	}
	// run out of bounces!
	return maxNumBounces;
}



float LayeredBSDF::Pdf(const SpectrumWavelengths &sw, const Vector &woW,
	const Vector &wiW, BxDFType flags) const
{
	float p = 1.f;
	
	bool glossy= (flags & BSDF_GLOSSY) ? true : false;
	bool specular= (flags & BSDF_SPECULAR) ? true : false;

	if (!glossy)
		return 0.f;

	if (glossy && specular) 
		p = p * (1.f - probSampleSpec);		// prob of glossy

	bool reflect = (flags & BSDF_REFLECTION) > 0 ? true:false;
	bool transmit = (flags & BSDF_TRANSMISSION) > 0 ? true:false;
	
	if (!reflect && !transmit )
		return 0.f;
	if (reflect && transmit)
		return p * INV_PI * 0.25f;
	return p * INV_TWOPI;
}

		
SWCSpectrum LayeredBSDF::rho(const SpectrumWavelengths &sw, BxDFType flags) const
{
	// NOTE: not implemented yet - do they really make a difference?
	SWCSpectrum ret(1.f);
	return ret ;
}

SWCSpectrum LayeredBSDF::rho(const SpectrumWavelengths &sw, const Vector &woW,
	BxDFType flags) const
{
	// NOTE: not implemented yet - do they really make a difference?
	SWCSpectrum ret(1.f);
	return ret ;
}

// Threadsafe random seed generator - won't crash but seed may get corrupted

unsigned int layeredRandseed;

unsigned int LayeredBSDF::GetRandSeed() const
{
	return layeredRandseed++;
}
