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
#include <cstdlib>
#include <cstdio>
#include <iomanip>
#include <fstream>
#include <string>
#include <sstream>
#include <iostream>
#include <vector>
#include <map>

//////////////////////////////////////////////////////////////////////////

#include "photometricdata_ies.h"

namespace lux {

//////////////////////////////////////////////////////////////////////////
PhotometricDataIES::PhotometricDataIES() 
{ 
	Reset(); 
}

PhotometricDataIES::PhotometricDataIES( const char *sFileName )
{
	Reset();

	Load( sFileName );
}

PhotometricDataIES::~PhotometricDataIES()
{
	if( m_fsIES.is_open() )
		m_fsIES.close();
}


//////////////////////////////////////////////////////////////////////////

void PhotometricDataIES::Reset()
{
	m_bValid 	= false;
	m_Version 	= "NONE";

	m_Keywords.clear();
	m_VerticalAngles.clear(); 
	m_HorizontalAngles.clear(); 
	m_CandelaValues.clear();

	if( m_fsIES.is_open() )
		m_fsIES.close();
	m_fsIES.clear();
}

//////////////////////////////////////////////////////////////////////////

bool PhotometricDataIES::Load(  const char *sFileName )
{
	bool ok = PrivateLoad( sFileName );
	if( m_fsIES.is_open() )
		m_fsIES.close();
	m_fsIES.clear();
	return ok;
}

bool PhotometricDataIES::PrivateLoad(  const char *sFileName )
{
	Reset();

	m_fsIES.open( sFileName ); 

	if ( !m_fsIES.good() ) return false;

	std::string templine( 256, 0 );

	///////////////////////////////////////////////
	// Check for valid IES file and get version

	ReadLine( templine );

	size_t vpos = templine.find_first_of( "IESNA" );

	if ( vpos != std::string::npos )
	{
		m_Version = templine.substr( templine.find_first_of( ":" ) + 1 );	
	}
	else return false;

	///////////////////////////////////////////////

	if ( !BuildKeywordList() ) return false;
	if ( !BuildLightData() ) return false;

	///////////////////////////////////////////////

	m_bValid = true;

	return true;
}

//////////////////////////////////////////////////////////////////////////

bool PhotometricDataIES::BuildKeywordList()
{
	if ( !m_fsIES.good() ) return false;

	m_Keywords.clear();

	std::string templine( 256, 0 );

	//////////////////////////////////////////////////////////////////	
	// Find the start of the keyword section...

	m_fsIES.seekg( 0 );
	ReadLine( templine );

	if ( templine.find( "IESNA" ) == std::string::npos )
	{
		return false; // Invalid file.
	}	
	
	/////////////////////////////////////////////////////////////////
	// Build std::map from the keywords

	std::string sKey, sVal;

	while( m_fsIES.good() )
	{
		ReadLine( templine );

		if ( templine.find( "TILT" ) != std::string::npos ) break;

		size_t kwStartPos 	= templine.find_first_of( "[" );
		size_t kwEndPos 	= templine.find_first_of( "]" );

		if( kwStartPos != std::string::npos && 
			kwEndPos != std::string::npos && 
			kwEndPos > kwStartPos )
		{
			std::string sTemp = templine.substr( kwStartPos + 1, ( kwEndPos - kwStartPos ) - 1 ); 

			if ( templine.find( "MORE" ) == std::string::npos && !sTemp.empty() )
			{	
				if ( !sVal.empty() )
				{		
				    m_Keywords.insert( std::pair<std::string,std::string>(sKey,sVal) );
				}

				sKey = sTemp;
				sVal = templine.substr( kwEndPos + 1, templine.size() - ( kwEndPos + 1 ) );

			}
			else
			{
				sVal += " " + templine.substr( kwEndPos + 1, templine.size() - ( kwEndPos + 1 ) );
			}
		}
	}

	if ( !m_fsIES.good() ) return false;
	
    m_Keywords.insert( std::pair<std::string,std::string>(sKey,sVal) );

	return true;
}

//////////////////////////////////////////////////////////////////////////

void PhotometricDataIES::BuildDataLine( std::stringstream & ssLine, unsigned int nDoubles, std::vector <double> & vLine )
{
	double dTemp = 0.0;

	unsigned int count = 0;

	while( count < nDoubles && ssLine.good() )
	{
		ssLine >> dTemp;

		vLine.push_back( dTemp ); 
				
		count++;
	}
}

bool PhotometricDataIES::BuildLightData()
{
	if ( !m_fsIES.good() ) return false;

	std::string templine( 1024, 0 );

	//////////////////////////////////////////////////////////////////	
	// Find the start of the light data...

	m_fsIES.seekg( 0 );

	while( m_fsIES.good() )
	{
		ReadLine( templine );

		if ( templine.find( "TILT" ) != std::string::npos ) break;
	}
	
	////////////////////////////////////////
	// Only supporting TILT=NONE right now

	if ( templine.find( "TILT=NONE" ) == std::string::npos ) return false;

	//////////////////////////////////////////////////////////////////	
	// clean the data fields, some IES files use comma's in data 
	// fields which breaks ifstreams >> op. 

	int spos = (int)m_fsIES.tellg(); 

	m_fsIES.seekg( 0, std::ios_base::end );

	int epos = (int)m_fsIES.tellg() - spos; 

	m_fsIES.seekg( spos );
	
	std::string strIES( epos, 0 );

	int nChar;
	int n1 = 0;

	for ( int n = 0; n < epos; n++ )
	{
		if ( m_fsIES.eof() ) break;

		nChar = m_fsIES.get();

		if ( nChar != ',' ) 
		{
			strIES[n1] = (char)nChar; 
			n1++;
		}
	}

	m_fsIES.close(); // done with the file.

	strIES.resize( n1 );

	std::stringstream ssIES( strIES, std::stringstream::in );

	ssIES.seekg( 0, std::ios_base::beg );

	//////////////////////////////////////////////////////////////////	
	// Read first two lines containing light vars.

	ssIES >> m_NumberOfLamps;
	ssIES >> m_LumensPerLamp;
	ssIES >> m_CandelaMultiplier;
	ssIES >> m_NumberOfVerticalAngles;
	ssIES >> m_NumberOfHorizontalAngles;
	unsigned int photometricTypeInt;
	ssIES >> photometricTypeInt;
	m_PhotometricType = PhotometricType(photometricTypeInt);
	ssIES >> m_UnitsType;
	ssIES >> m_LuminaireWidth;
	ssIES >> m_LuminaireLength;
	ssIES >> m_LuminaireHeight;

	ssIES >> BallastFactor;
	ssIES >> BallastLampPhotometricFactor;
	ssIES >> InputWatts;

	//////////////////////////////////////////////////////////////////	
	// Read angle data

	m_VerticalAngles.clear(); 
	BuildDataLine( ssIES, m_NumberOfVerticalAngles, m_VerticalAngles );

	m_HorizontalAngles.clear(); 
	BuildDataLine( ssIES, m_NumberOfHorizontalAngles, m_HorizontalAngles );
	
	m_CandelaValues.clear();

	std::vector <double> vTemp;

	for ( unsigned int n1 = 0; n1 < m_NumberOfHorizontalAngles; n1++ )
	{
		vTemp.clear();

		BuildDataLine( ssIES, m_NumberOfVerticalAngles, vTemp );

	    m_CandelaValues.push_back( vTemp );
	}

	return true;
}
} //namespace lux
