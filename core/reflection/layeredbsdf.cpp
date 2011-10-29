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
	max_num_bounces=1; // Note this gets changed when layers are added
	prob_sample_spec=0.5f;

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
	if (!reflect && !transmit) {		// new convention
		reflect=true;
		transmit=true;
	}

	*pdf = 1.0f;
	if (pdfBack) {
		*pdfBack = *pdf;
	}
	RandomGenerator rng(getRandSeed());	

	
	if (glossy&&specular) { // then choose one
		if (rng.floatValue()<prob_sample_spec) { // REALLY NEED TO CHOOSE A BETTER VALUE!
			glossy=false;	// just do specular
			*pdf *= prob_sample_spec;
		}
		else {
			specular=false;	// just do glossy
			*pdf *= (1.0f-prob_sample_spec);
		}
	}
	
	if (glossy) { // then random sample hemisphere and return F() value

		*sampled=UniformSampleHemisphere(u1, u2);
		
		bool did_reflect=true;	// indicates if sampled ray is reflected/transmitted
		// adjust orientation
		if (!reflect && transmit )  {// then transmitting
			did_reflect=false;
		}
		else if (reflect && transmit)  { 
			*pdf *=0.5;
			if (u3>0.5f) { // randomly pick one
				did_reflect=false;	// transmit
			}	
		
		}
		if (did_reflect) { //adjust so that sampled ray is in the same hemisphere as known
			if ( Dot(ng,known) <0) {
				sampled->z*=-1.0f;
			}
		}
		else { // make sure they're in different hemispheres
			if ( Dot(ng,known) >0) {
				sampled->z*=-1.0f;
			}
		}
		
		*sampled=LocalToWorld(*sampled);	// convert to world coords

		// create flags and check if they match requested flags
		BxDFType flags2;
		if (did_reflect) { // calculate a reflected component
			flags2=BxDFType(BSDF_GLOSSY|BSDF_REFLECTION);
		}
		else { // calculate a transmitted component
			flags2=BxDFType(BSDF_GLOSSY|BSDF_TRANSMISSION);
		}

		*pdf *= INV_PI*0.5f;

		if (pdfBack) {
			*pdfBack = *pdf;
		}
		
		*f_=F(sw, known, *sampled,reverse,flags);	// automatically includes any reverse==true/false adjustments
		//*f_=SWCSpectrum(1.f);// DEBUG
		*f_ /= *pdf;

		if (!reverse) {
			*f_ *= fabs( Dot(*sampled, ng) / Dot(known, ng) );
		}

		if (sampledType) {
			*sampledType= flags2;
		}
		return true;
	}

	if (specular) { // then random sample a specular path and return it
		SWCSpectrum L(1.f);

		if (reflect && transmit)  { // choose one at random
			*pdf *=0.5;
			if (u3>0.5f) { // randomly pick one
				reflect=false;
			}	
			else {
				transmit=false;
			}
		
		}
		

		int curlayer= (Dot(ng,known)<0) ? nBSDFs-1 : 0;	// figure out which layer to hit first - back or front
		// now find exit layer
		int exit_layer=0;
		if (transmit && curlayer==0 ) {
			exit_layer=nBSDFs-1;
		}
		if (reflect && curlayer>0 ) {
			exit_layer=nBSDFs-1;
		}
		
		float pdf_forward=1.0f;
		float pdf_back=1.0f;
		
		SWCSpectrum newF(0.f);
		Vector cur_vin=known;	// 
		Vector cur_vout=Vector();	

		int count=0;
		
		// bounce around and calc accumlated L
		bool bouncing=true;
		while (bouncing) {

			// try and get the next sample
			if ( bsdfs[curlayer]->SampleF(sw , cur_vin , &cur_vout,
					rng.floatValue() ,rng.floatValue() , rng.floatValue() ,
					&newF , &pdf_forward , BxDFType(BSDF_SPECULAR|BSDF_TRANSMISSION|BSDF_REFLECTION), NULL , &pdf_back, true ) ) { // then sampled ok
				L *= newF *pdf_forward;	// remove pdf adjustment
				*pdf *= pdf_forward;
			}
			else { // no valid sample
				return false;	// exit - no valid sample
			}
	
			// calc next layer and adjust directions
			if (Dot(ng,cur_vout)>0) { curlayer--;	}// ie, if woW is in same direction as normal - decrement layer index
			else { curlayer++; }
			cur_vin=-cur_vout;	

			// check for exit conditions
			count++;
			if (count > max_num_bounces*2 ) {
				bouncing=false;
			}

			if ( curlayer<0 ) {
				if (exit_layer>0) { // wrong exit surface - no valid sample
					return false;
				}
				bouncing= false	; // have reached the exit - stop bouncing
			}

			
			if ( curlayer >= (int) nBSDFs ) { // exited the material
				if (exit_layer==0) { // wrong exit surface - no valid sample
					return false;
				}
				bouncing=false; // have reached the exit - stop bouncing
			}

		}
		// ok, so if we get here then we've got a valid L - tidy up and exit

		*sampled=cur_vout;	
		
		// create flags and check if they match requested flags
		BxDFType flags2;
		if (reflect) { // calculate a reflected component
			flags2=BxDFType(BSDF_SPECULAR|BSDF_REFLECTION);
		}
		else { // calculate a transmitted component
			flags2=BxDFType(BSDF_SPECULAR|BSDF_TRANSMISSION);
		}

		// no pdf adjustment needed
		
		if (pdfBack) {
			*pdfBack = *pdf;
		}
		
		*f_= L/ *pdf;	
		
		if (!reverse) {
			*f_ *= fabs( Dot(*sampled, ng) / Dot(known, ng) );
		}
		
		if (sampledType) {
			*sampledType= flags2;
		}
		return true;

		}
	return false; // no sample

}

SWCSpectrum LayeredBSDF::F(const SpectrumWavelengths &sw, const Vector &woW,
		const Vector &wiW, bool reverse, BxDFType flags) const
{
	// Uses a modifed bidir to sample the layers
	// by convention woW is the light direction, wiW is the eye direction 
	// so reverse==true corresponds to the physical situation of light->surface->eye (PBRT default)

	if ( (flags&BSDF_GLOSSY)==0 ) { // nothing to sample
		SWCSpectrum L(0.f);
		return L;
	}
		
	// Create storage for incoming/outgoing paths

	vector<int> eye_layer;
	vector<Vector> eye_vector;
	vector<float> eye_pdf_forward;
	vector<float> eye_pdf_back;
	vector<SWCSpectrum> eye_L;
	vector<BxDFType> eye_type;
	
	vector<int> light_layer;
	vector<Vector> light_vector;
	vector<float> light_pdf_forward;
	vector<float> light_pdf_back;
	vector<SWCSpectrum> light_L;
	vector<BxDFType> light_type;
	
	// now create the two paths
	
	if (reverse) { // then woW is outgoing(eye) dir, wiW is incoming
		int light_index= (Dot(ng,wiW)<0) ? nBSDFs-1 : 0;
		int eye_index=(Dot(ng,woW)<0) ? nBSDFs-1 : 0;

		getPath(sw, wiW, light_index, &light_L, &light_vector,&light_layer, &light_pdf_forward, &light_pdf_back , &light_type, false); //light
		getPath(sw, woW, eye_index, &eye_L, &eye_vector,&eye_layer, &eye_pdf_forward, &eye_pdf_back, &eye_type, true); //eye
	}
	else { // woW is INCOMING(light) dir, wiW is OUTGOING
		int light_index= (Dot(ng,woW)<0) ? nBSDFs-1 : 0;
		int eye_index=(Dot(ng,wiW)<0) ? nBSDFs-1 : 0;

		getPath(sw, woW, light_index, &light_L, &light_vector,&light_layer, &light_pdf_forward, &light_pdf_back , &light_type,false); //light
		getPath(sw, wiW, eye_index, &eye_L, &eye_vector,&eye_layer, &eye_pdf_forward, &eye_pdf_back, &eye_type,true); //eye
	}
	// now connect them
	SWCSpectrum L(0.f);	// this is the accumulated L value for the current path 
	SWCSpectrum newF(0.f);
			
	for (int i=0;i< (int)eye_layer.size();i++) { // for every vertex in the eye path
		for (int j=0;j< (int) light_layer.size();j++) { // try to connect to every vert in the light path
			if (eye_layer[i]==light_layer[j]) { // then pass the "visibility test" so connect them
				int curlayer=eye_layer[i];
				// First calculate the total L for the path
				
				SWCSpectrum Lpath =bsdfs[curlayer]->F(sw,eye_vector[i],light_vector[j],true,BxDFType(BSDF_ALL)); // calc how much goes between them
				// NOTE: used reverse==True to get F=f*|wo.ns| = f * cos(theta_in)
			
				Lpath = eye_L[i] * Lpath * light_L[j];

				if (!Lpath.Black() ) {	// if it is black we may have a specular connection
					float pgap_fwd=bsdfs[curlayer]->Pdf(sw,light_vector[j],eye_vector[i],BxDFType(BSDF_ALL)); // should be prob of sampling eye vector given light vector
					float pgap_back=bsdfs[curlayer]->Pdf(sw,eye_vector[i],light_vector[j],BxDFType(BSDF_ALL));
				
					
					// Now calc the probability of sampling this path (surely there must be a better way!!!)

					float totprob=0.0f;

					// construct the list of fwd/back probs
					float* fwdprob=new float[i+j+3];
					float* backprob= new float[i+j+3];
					bool* spec = new bool[i+j+3];
					for (int k=0;k<=j;k++) {
						fwdprob[k]=light_pdf_forward[k];
						backprob[k]=light_pdf_back[k];
						spec[k]=(BSDF_SPECULAR & light_type[k]) > 0 ;
					}
					fwdprob[j+1]=pgap_fwd;
					backprob[j+1]=pgap_back;
					spec[j+1]=false;		// if this is true then the bsdf above will ==0 and cancel it out anyway
					for (int k=i;k>=0;k--) {	
						fwdprob[j+2+(i-k)]=eye_pdf_back[k];
						backprob[j+2+(i-k)]=eye_pdf_forward[k];
						spec[j+2+(i-k)]=(BSDF_SPECULAR & eye_type[k]) > 0;
					}

					int join=i+j;
					while (join>=0) {
						float curprob=1.0f;
						for (int k=0;k<=join;k++) {
							curprob *= fwdprob[k];
						}
						for (int k=join+2 ; k< i+j+3 ;k++) {
							curprob *= backprob[k];
						}
						if (!spec[join+1] ) { // can use it -  if these terms are specular then the delta functions make the L term 0
							totprob+=curprob;
						}
						join--;
					}
					if (totprob>0.0f) {
						L+=Lpath/ totprob;
					}
					delete[] fwdprob;
					delete[] backprob;
					delete[] spec;
				}
			}
		}
	}
	
	return L;
}

int LayeredBSDF::getPath(const SpectrumWavelengths &sw, const Vector &vin, const int start_index, 
				vector<SWCSpectrum> *path_L, vector<Vector>* path_vec, vector<int>* path_layer, 
				vector<float>* path_pdf_forward,vector<float>* path_pdf_back,
				vector<BxDFType> * path_sample_type,
				bool eye) const {
	// returns the number of bounces used in this path
	// if eye==TRUE we are calculating the eye path

	RandomGenerator rng(getRandSeed());
	
	int curlayer=start_index;	// curlayer is the layer we will be sampling
	Vector cur_vin=vin;	// 
	Vector cur_vout=Vector();				
	float pdf_forward=1.0f;
	float pdf_back=1.0f;
	BxDFType sampled_type=BSDF_GLOSSY;	// anything but specular really

	
	SWCSpectrum L(1.f);	// this is the accumulated L value for the current path 
		
	for (int i=0;i<max_num_bounces;i++) {	// this will introduce a small bias ?Need to fix with russian roulette
		if ( (curlayer<0) || ( curlayer >= (int) nBSDFs ) ) { // have exited the material
			return i;
		}

		// STORE THE CURRENT PATH INFO
		path_L->push_back(L);
		path_vec->push_back(cur_vin);
		path_layer->push_back(curlayer);
		path_pdf_forward->push_back(pdf_forward);
		path_pdf_back->push_back(pdf_back);
		path_sample_type->push_back(sampled_type);
	
		// get the next sample

		SWCSpectrum newF(0.f);
		
		if (eye) {
			if ( !bsdfs[curlayer]->SampleF(sw , cur_vin , &cur_vout,
					rng.floatValue() ,rng.floatValue() , rng.floatValue() ,
					&newF , &pdf_forward , BxDFType(BSDF_ALL), &sampled_type , &pdf_back, false ) ) { // then couldn't sample
						// no valid sample - terminate path
						return i;
			}
			newF*=fabs(Dot(ng,cur_vin)/Dot(ng,cur_vout));	// the mysterious correction factor from the black lagoon (for rev=false)
		}
		else { // light paths
		if ( !bsdfs[curlayer]->SampleF(sw , cur_vin , &cur_vout,
					rng.floatValue() ,rng.floatValue() , rng.floatValue() ,
					&newF , &pdf_forward , BxDFType(BSDF_ALL), &sampled_type , &pdf_back, true ) ) { // then couldn't sample
						// no valid sample - terminate path
						return i;
			}
		}

		L*=newF*pdf_forward;	// NOTE: remove pdf adjustment
		
		if (Dot(ng,cur_vout)>0) { curlayer--;	}// ie, if woW is in same direction as normal - decrement layer index
		else { curlayer++; }
		cur_vin=-cur_vout;		
		
	}
	// run out of bounces!
	return max_num_bounces;
}



float LayeredBSDF::Pdf(const SpectrumWavelengths &sw, const Vector &woW,
	const Vector &wiW, BxDFType flags) const
{
	float p=1.0f;
	
	bool glossy= (flags & BSDF_GLOSSY) ? true : false;
	bool specular= (flags & BSDF_SPECULAR) ? true : false;


	if (!glossy && !specular) {
		return 0.0f;
	}

	if (glossy && specular) { 
		p=p*(1.0f-prob_sample_spec);		// prob of glossy
	}
	

	bool reflect= (flags & BSDF_REFLECTION) > 0 ? true:false;
	bool transmit= (flags & BSDF_TRANSMISSION) > 0 ? true:false;
	
	if (!reflect && !transmit ) { // these are now the same as both being true (jeanphi convention?)
		return p*INV_PI*0.25f;
	}
	if (reflect && transmit ) {	// same as above
		return p*INV_PI*0.25f;
	}
	return p*INV_PI*0.5f;
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

// Threadsafe random seed generator

unsigned int layered_randseed;

unsigned int LayeredBSDF::getRandSeed() const { 
	extern unsigned int layered_randseed;
	return layered_randseed++;
}