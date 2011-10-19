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
//#include <ctime>		// for rand seed

using namespace lux;

//static RandomGenerator rng(1);

// LayeredBSDF Method Definitions

LayeredBSDF::LayeredBSDF(const DifferentialGeometry &dgs, const Normal &ngeom,
	const Volume *exterior, const Volume *interior) :
	BSDF(dgs, ngeom, exterior, interior)
{
	nBSDFs = 0;
	geom_norm=Vector(ng);

	tn_geom=Normalize(Cross(ng, sn));
	sn_geom=Normalize(Cross(tn_geom,ng));

	max_num_bounces=3;
	num_f_samples=10;
	//type=BSDF_DIFFUSE;
}

bool LayeredBSDF::SampleF(const SpectrumWavelengths &sw, const Vector &known, Vector *sampled,
	float u1, float u2, float u3, SWCSpectrum *const f_, float *pdf,
	BxDFType flags, BxDFType *sampledType, float *pdfBack,
	bool reverse) const
{
	// Have two possible types to return - glossy or specular - see if either/both have been requested
	bool glossy= (flags & BSDF_GLOSSY) ? true : false;
	bool specular= (flags & BSDF_SPECULAR) ? true : false;
	
	// now check reflect/transmit
	bool reflect= (flags & BSDF_REFLECTION) > 0 ? true:false;
	bool transmit= (flags & BSDF_TRANSMISSION) > 0 ? true:false;

	if (!reflect && !transmit ) { // if neither set convert to both set 
		reflect=true;
		transmit=true;
	}
	
	float pspec=1.0;		// will be total prob of not interacting with any of the layers
	if (specular) { // calc prob
		for (u_int i=0;i<nBSDFs;i++) {
			pspec*=(1.0f-opacity[i]);
		}
	}	
	
	RandomGenerator rng(GetTickCount()*clock());	// until random is more stable
	
	*pdf=1.0f;
	if (specular&&transmit) {
		bool do_spec=false;
		if (glossy) { // then need to choose one
			if (rng.floatValue()<pspec) { // do specular
				do_spec=true;
				*pdf *= pspec;
			}
			else { // do glossy
				*pdf *= 1.0f-pspec;
			}
		}
		else { // just do specular
			do_spec=true;
			// no need to adjust pdf
		}
		if (do_spec) { // calculate specular transmision bits
			//
			*sampled=-known;
			if (pdfBack) {
				*pdfBack = *pdf;
			}
			*f_ = SWCSpectrum(1.f);
			if (!reverse) {
				*f_ *= fabs( Dot(*sampled, ng) / Dot(known, ng) );
			}
			if (sampledType) {
				*sampledType= BxDFType(BSDF_SPECULAR&BSDF_TRANSMISSION);
			}
			return true;
		}
	}

	// if get here then check glossy
	if (glossy) {
		// sample a hemisphere to start with
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
		
		*sampled=LocalToWorldGeom(*sampled);	// convert to world coords

		// create flags and check if they match requested flags
		BxDFType flags2;
		if (did_reflect) { // calculate a reflected component
			flags2=BxDFType(BSDF_GLOSSY&BSDF_REFLECTION);
		}
		else { // calculate a transmitted component
			flags2=BxDFType(BSDF_GLOSSY&BSDF_TRANSMISSION);
		}

		*pdf *= INV_PI*0.5f;

		if (pdfBack) {
			*pdfBack = *pdf;
		}
		
		*f_=F(sw, known, *sampled,reverse,flags);	// automatically includes any reverse==true/false adjustments
		*f_ /= *pdf;

		if (sampledType) {
			*sampledType= flags2;
		}
		return true;
	}
	// if we get here - nothing to sample
	return false;

}

float LayeredBSDF::Pdf(const SpectrumWavelengths &sw, const Vector &woW,
	const Vector &wiW, BxDFType flags) const
{
	if ((flags & BSDF_GLOSSY) ==0 ) { // can't sample
		return 0.0f;
	}

	bool reflect= (flags & BSDF_REFLECTION) > 0 ? true:false;
	bool transmit= (flags & BSDF_TRANSMISSION) > 0 ? true:false;

	if (!reflect && !transmit ) { // these are now the same as both being true
		return INV_PI*0.25f;
	}
	if (reflect && transmit ) {
		return INV_PI*0.25f;
	}
	return INV_PI*0.5f;
}


SWCSpectrum LayeredBSDF::F(const SpectrumWavelengths &sw, const Vector &woW,
		const Vector &wiW, bool reverse, BxDFType flags) const
{
	
	int seed=GetTickCount(); // program starts
	RandomGenerator rng(seed*clock());	// until random is more stable
	SWCSpectrum ret(0.f);
	
	int enter_index= (Dot(ng,wiW)<0) ? nBSDFs-1 : 0;
	int exit_index=(Dot(ng,woW)<0) ? nBSDFs-1 : 0;
	
	for (int i=0;i<num_f_samples;i++) {
		int curlayer=enter_index;
		Vector cur_woW=Vector(woW);	
		Vector cur_wiW=Vector(wiW);				
		float pdf=1.0f;
		float totprob=1.0f;
		SWCSpectrum L(1.f);	// this is the current accumulated L value
		SWCSpectrum Ltot(0.f);	// this is the current accumulated L value for the current sample
		
		bool init_enter=true;	// flags that we are entering the material (so currently outside)
		
		for (int j=0;j<max_num_bounces;j++) { // on each entry curlayer is layer light is hitting, from direction wiW
			bool interact=true;
			SWCSpectrum newF(0.f);
			// Check how much light is heading in dir woW

			// calc prob of no interaction
			float p_nointeract=1.0;
			int dir= curlayer<exit_index ? -1 : 1;	// direction to head to exit
			int cl=exit_index;
			while (cl!=curlayer) {
				p_nointeract*=(1.0f-opacity[cl]);
				cl+=dir;
			}
			if (p_nointeract > .0f ) { // then some light might get out
				newF=bsdfs[curlayer]->F(sw,woW,cur_wiW,false,BxDFType(BSDF_ALL));
				SWCSpectrum newL(L);
				newL*=newF;
				newL*=Ftof_rev_false(woW,cur_wiW)*AbsDot(nn,cur_wiW)/totprob*p_nointeract*opacity[curlayer];
				Ltot+=newL;
			}

			// now check how to sample
			
			if (init_enter) { // then we are entering the material - need to transmit
				// check prob of interaction
				if (rng.floatValue()<=opacity[curlayer]) { // interact
				
					if (!bsdfs[curlayer]->SampleF(sw , cur_wiW , &cur_woW,
						rng.floatValue() ,rng.floatValue() , rng.floatValue() ,
						&newF , &pdf , BxDFType(BSDF_ALL_TRANSMISSION), NULL , NULL, false ) ) { // then couldn't sample
							break;	// end bounces
					}
					pdf*=opacity[curlayer];
				}
				else { // no interaction
					pdf=(1.0f-opacity[curlayer]);
					interact=false;
				}
				init_enter=false;
			}
			
			else if ( (curlayer==0) || (curlayer==nBSDFs-1 ) ) { // reflect it back into the material
				// set reflection flags
				if (!bsdfs[curlayer]->SampleF(sw , cur_wiW , &cur_woW,
					rng.floatValue() ,rng.floatValue() , rng.floatValue() ,
					&newF , &pdf , BxDFType(BSDF_ALL_REFLECTION), NULL , NULL, false ) ) { // then couldn't sample
						break;	// end bounces
				}
				pdf*=opacity[curlayer];	// ignore possibility of no-interaction - it's a dead end anyway
			}

			else { // otherwise random sample (never called if only 2 bsdfs)
				// check if no interaction
				if (rng.floatValue()<=opacity[curlayer]) { // interact
					if (!bsdfs[curlayer]->SampleF(sw , cur_wiW , &cur_woW,
						rng.floatValue() ,rng.floatValue() , rng.floatValue() ,
						&newF , &pdf , BxDFType(BSDF_ALL), NULL , NULL, false ) ) { // then couldn't sample
							break;	// end bounces
					}
				}
				else { // no interaction
					pdf=(1.0f-opacity[curlayer]);
					interact=false;
				}
			}
			
			// have a sample
			if (interact) { // interaction - adjust L
				SWCSpectrum Lfactor(0.0f);
				Lfactor.AddWeighted(sample_Ftof_revfalse(cur_wiW,cur_woW,pdf)*AbsDot(nn,cur_wiW),newF);
				L*=Lfactor;
			}
			else {
				cur_woW=-cur_wiW;
			}
			totprob*=pdf;	// this is valid for both

			// now progress our layers based on woW
			if (Dot(ng,cur_woW)>0) { curlayer--;	}// ie, if woW is in same direction as normal - decrement layer index
			else curlayer++;
			
			cur_wiW=-cur_woW;		// 
		}
		// now have finished bouncing
		ret+=Ltot;
	}
	ret /= num_f_samples;
	// ret is the estimated outgoing radiance
	ret /= AbsDot(nn,wiW);// convert to f (which is Lo/(Li incoming.nn ) )
	// now to convert f to F
	SWCSpectrum F_out;
	if (reverse) {
		F_out=ret*ftoF_rev_true(woW,wiW);
	}
	else {
		F_out=ret*ftoF_rev_false(woW,wiW); 
	}
	return F_out;
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

