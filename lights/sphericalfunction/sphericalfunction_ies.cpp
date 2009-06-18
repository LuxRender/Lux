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

#include "sphericalfunction_ies.h"
#include "error.h"
#include "mcdistribution.h"

namespace lux {
	IESSphericalFunction::IESSphericalFunction() {
		initDummy();
	}

	IESSphericalFunction::IESSphericalFunction(const PhotometricDataIES& data, bool flipZ) {
		if( data.m_PhotometricType != PhotometricDataIES::PHOTOMETRIC_TYPE_C ) {
			luxError( LUX_UNIMPLEMENT, LUX_WARNING, "unsupported photometric type IES file, result may be wrong" );
		}
		vector<double> vertAngles = data.m_VerticalAngles;
		vector<double> horizAngles = data.m_HorizontalAngles;
		vector< vector<double> > values = data.m_CandelaValues;

		// Add a begin/end vertical angle with 0 emission if necessary
		if( vertAngles[0] < 0.0 ) {
			for( u_int i = 0; i < vertAngles.size(); i++ ) {
				vertAngles[i] = vertAngles[i] + 90.0;
			}
		}
		if( vertAngles[0] > 0.0 ) {
			vertAngles.insert( vertAngles.begin(), vertAngles[0] - 1e-3 );
			for( u_int i = 0; i < values.size(); i++ ) {
				values[i].insert( values[i].begin(), 0.0 );
			}
		}
		if( vertAngles[vertAngles.size() - 1] < 180.0 ) {
			vertAngles.push_back( vertAngles[vertAngles.size() - 1] + 1e-3 );
			for( u_int i = 0; i < values.size(); i++ ) {
				values[i].push_back( 0.0 );
			}
		}
		// Generate missing horizontal angles
		if( horizAngles[0] == 0.0 && horizAngles.size() == 1 ) {
			// OK
		}
		else if( horizAngles[0] == 0.0 ) {
			if( horizAngles[ horizAngles.size() - 1] == 90.0 ) {
				for( int i = horizAngles.size() - 2; i >= 0; i--) {
					horizAngles.push_back(180.0 - horizAngles[i]);
					vector<double> tmpVals = values[i]; // copy before adding!
					values.push_back(tmpVals);
				}
			}
			if( horizAngles[ horizAngles.size() - 1] == 180.0 ) {
				for( int i = horizAngles.size() - 2; i >= 0; i--) {
					horizAngles.push_back(360.0 - horizAngles[i]);
					vector<double> tmpVals = values[i]; // copy before adding!
					values.push_back(tmpVals);
				}
			}
			if( horizAngles[ horizAngles.size() - 1] != 360.0 ) {
				luxError( LUX_UNIMPLEMENT, LUX_ERROR, "unsupported horizontal angles in IES file" );
				initDummy();
				return;
			}
		}
		else {
			luxError( LUX_BADFILE, LUX_ERROR, "invalid horizontal angles in IES file" );
			initDummy();
			return;
		}

		// Initialize irregular functions
		float valueScale = data.m_CandelaMultiplier * 
			   data.BallastFactor * 
			   data.BallastLampPhotometricFactor;
		int nVFuncs = horizAngles.size();
		IrregularFunction1D** vFuncs = new IrregularFunction1D*[ nVFuncs ];
		int vFuncLength = vertAngles.size();
		float* vFuncX = new float[vFuncLength];
		float* vFuncY = new float[vFuncLength];
		float* uFuncX = new float[nVFuncs];
		float* uFuncY = new float[nVFuncs];
		for( int i = 0; i < nVFuncs; i++ ) {
			for( int j = 0; j < vFuncLength; j++ ) {
				vFuncX[ j ] = Clamp( Radians( vertAngles[ j ] ) * INV_PI, 0.f, 1.f );
				vFuncY[ j ] = values[ i ][ j ] * valueScale;
			}

			vFuncs[ i ] = new IrregularFunction1D( vFuncX, vFuncY, vFuncLength );

			uFuncX[ i ] = Clamp( Radians( horizAngles[ i ] ) * INV_TWOPI, 0.f, 1.f );
			uFuncY[ i ] = i;
		}
		delete[] vFuncX;
		delete[] vFuncY;

		IrregularFunction1D* uFunc = new IrregularFunction1D( uFuncX, uFuncY, nVFuncs );
		delete[] uFuncX;
		delete[] uFuncY;

		// Resample the irregular functions
		int xRes = 512;
		int yRes = 256;
		TextureColor<float, 1>* img = new TextureColor<float, 1>[xRes*yRes];
		for( int y = 0; y < yRes; y++ ) {
			for( int x = 0; x < xRes; x++ ) {
				float s = ( x + .5f ) / float(xRes);
				float t = ( y + .5f ) / float(yRes);
				float du;
				int u1 = uFunc->IndexOf(s, &du);
				int u2 = min(nVFuncs - 1, u1 + 1);
				int tgtY = flipZ ? (yRes-1)-y : y;
				img[ x + tgtY*xRes ] = 
					vFuncs[u1]->Eval(t) * (1.f - du) + vFuncs[u2]->Eval(t) * du;
			}
		}
		delete uFunc;
		for( int i = 0; i < nVFuncs; i++)
			delete vFuncs[i];
		delete[] vFuncs;

		boost::shared_ptr< MIPMap<RGBColor> > ptr(
			new MIPMapFastImpl< RGBColor, TextureColor<float, 1> >( BILINEAR, xRes, yRes, &img[0] )
		);
		SetMipMap(ptr);

		delete[] img;
	}

	void IESSphericalFunction::initDummy() {
		TextureColor<float, 1> img[1] = {1.f};
		boost::shared_ptr< MIPMap<RGBColor> > ptr(
			new MIPMapFastImpl< RGBColor, TextureColor<float, 1> >( NEAREST, 1, 1, &img[0] )
		);
		SetMipMap(ptr);
	}

} //namespace lux
