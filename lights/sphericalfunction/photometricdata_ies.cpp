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

void PhotometricDataIES::BuildDataLine( unsigned int nDoubles, std::vector <double> & vLine )
{
	double dTemp = 0.0;

	unsigned int count = 0;

	while( count < nDoubles && m_fsIES.good() )
	{
		m_fsIES >> dTemp;

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
	// Read first two lines containing light vars.

	ReadLine( templine );

	unsigned int photometricTypeInt;
	sscanf( &templine[0], "%u %lf %lf %u %u %u %u %lf %lf %lf", 
			&m_NumberOfLamps,
			&m_LumensPerLamp,
			&m_CandelaMultiplier,
			&m_NumberOfVerticalAngles,
			&m_NumberOfHorizontalAngles,
			&photometricTypeInt,
			&m_UnitsType,
			&m_LuminaireWidth,
			&m_LuminaireLength,
			&m_LuminaireHeight );
	m_PhotometricType = PhotometricType(photometricTypeInt);

	ReadLine( templine );

	sscanf( &templine[0], "%lf %lf %lf", 
			&BallastFactor,
			&BallastLampPhotometricFactor,
			&InputWatts );

	//////////////////////////////////////////////////////////////////	
	// Read angle data

	// Vertical Angles

	m_VerticalAngles.clear(); 
	BuildDataLine( m_NumberOfVerticalAngles, m_VerticalAngles );

	m_HorizontalAngles.clear(); 
	BuildDataLine( m_NumberOfHorizontalAngles, m_HorizontalAngles );
	
	m_CandelaValues.clear();

	std::vector <double> vTemp;

	for ( unsigned int n1 = 0; n1 < m_NumberOfHorizontalAngles; n1++ )
	{
		vTemp.clear();

		BuildDataLine( m_NumberOfVerticalAngles, vTemp );

	    m_CandelaValues.push_back( vTemp );
	}

	return true;
}

//////////////////////////////////////////////////////////////////////////
// Test...

/*int main(int ac, char *av[])
{
	if ( ac < 2 )
	{
		std::cout << "Useage: IEStest <file name> all|<IES key word> Vertical|Horizontal|Candela" << std::endl;

		return 1;
	}

	std::cout << "--------------------------------------------------------------------" << std::endl;
	std::cout << "Loading File: " << av[1] << "..." << std::endl;

	PhotometricDataIES cIES( av[1] );

	if ( !cIES.IsValid() )
	{
		std::cout << "Error loading: " << av[1] << " - Aborting" << std::endl;

		return 1;
	}

	std::cout << std::endl;
	std::cout << "IES Version: " << cIES.m_Version << std::endl;
	std::cout << "--------------------------------------------------------------------" << std::endl;

	std::cout << "Lamp Keywords..." << std::endl;

	if ( ac > 2 )
	{
		std::string as = av[2];
	
		if ( as == "all" )
		{
			for( std::map <std::string,std::string>::iterator it = cIES.m_Keywords.begin(); it != cIES.m_Keywords.end(); it++ )
			{
				std::cout << std::endl;
				std::cout << it->first << ": " << it->second << std::endl;
			}
		}
		else
		{
			std::cout << std::endl;
			std::cout << as << ": " << cIES.m_Keywords[as] << std::endl;
		}
	}

	std::cout << "--------------------------------------------------------------------" << std::endl;
	std::cout << "Lamp Data..." << std::endl;
	std::cout << std::endl;
	std::cout << "Number of Lamps: " << cIES.m_NumberOfLamps << std::endl;
	std::cout << "Lumens Per Lamp: " << cIES.m_LumensPerLamp << std::endl;
	std::cout << "Candela Multiplier: " << cIES.m_CandelaMultiplier << std::endl;
	std::cout << "Number Of Vertical Angles: " << cIES.m_NumberOfVerticalAngles << std::endl;
	std::cout << "Number Of Horizontal Angles: " << cIES.m_NumberOfHorizontalAngles << std::endl;
	std::cout << "Photometric Type: " << cIES.m_PhotometricType << std::endl;
	std::cout << "Units Type: " << cIES.m_UnitsType << std::endl;
	std::cout << "Luminaire Width: " << cIES.m_LuminaireWidth << std::endl;
	std::cout << "Luminaire Length: " << cIES.m_LuminaireLength << std::endl;
	std::cout << "Luminaire Height: " << cIES.m_LuminaireHeight << std::endl;
	std::cout << std::endl;
	std::cout << "Ballast Factor: " << cIES.BallastFactor << std::endl;
	std::cout << "Ballast-Lamp Photometric Factor: " << cIES.BallastLampPhotometricFactor << std::endl;
	std::cout << "Input Watts: " << cIES.InputWatts << std::endl;
	std::cout << std::endl;
	std::cout << "--------------------------------------------------------------------" << std::endl;

	if ( ac > 3 ) 
	{
		std::string as = av[3];

		unsigned int n1, n2;
	
		if ( as == "Vertical" )
		{
			std::cout << std::endl;
			std::cout << "Vertical Angles: " << cIES.m_NumberOfVerticalAngles << std::endl;
	
			for( n1 = 0; n1 < cIES.m_NumberOfVerticalAngles; n1++ )
			{
				std::cout << cIES.m_VerticalAngles[n1] << " ";
			}

			std::cout << std::endl;
		}
		else if ( as == "Horizontal" )
		{
			std::cout << std::endl;
			std::cout << "Horizontal Angles: " << cIES.m_NumberOfHorizontalAngles << std::endl;

			for( n1 = 0; n1 < cIES.m_NumberOfHorizontalAngles; n1++ )
			{
				std::cout << cIES.m_HorizontalAngles[n1] << " ";
			}

			std::cout << std::endl;
		}
		else if ( as == "Candela" )
		{
			std::cout << std::endl;
			std::cout << "Candela Values: " << std::endl;

			for( n1 = 0; n1 < cIES.m_NumberOfHorizontalAngles; n1++ )
			{
				std::cout << "Angle " << n1 << " : " << cIES.m_HorizontalAngles[n1] << std::endl;

				for( n2 = 0; n2 < cIES.m_NumberOfVerticalAngles; n2++ )
				{
					std::cout << cIES.m_CandelaValues[n1][n2] << " ";
				}

				std::cout << std::endl;
			}
		}
	}

	return 0;
}*/

} //namespace lux
