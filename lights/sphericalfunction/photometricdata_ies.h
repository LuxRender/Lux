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

#ifndef LUX_PHOTOMETRICDATAIES_H
#define LUX_PHOTOMETRICDATAIES_H

#include "lux.h"
#include <fstream>
#include <map>

#include <cstring>
using std::memcpy;

namespace lux {

class PhotometricDataIES {
public:
	PhotometricDataIES();
	PhotometricDataIES( const char * );

	~PhotometricDataIES();

	///////////////////////////////////////////////
	// Methods.
	
	bool IsValid() { return m_bValid; }
	void Reset();
	bool Load( const char* );
	
	void inline ReadLine( std::string & sLine )
	{
		memset( &sLine[0], 0, sLine.size() );
		m_fsIES.getline( &sLine[0], sLine.size(), 0x0A );
	}

	//////////////////////////////////////////////
	// Keywords and light descriptions.

	std::string m_Version;
	std::map<std::string,std::string> m_Keywords;

	//////////////////////////////////////////////
	// Light data.

	enum PhotometricType {
		PHOTOMETRIC_TYPE_C = 1,
		PHOTOMETRIC_TYPE_B = 2,
		PHOTOMETRIC_TYPE_A = 3
	};

	unsigned int 	m_NumberOfLamps;
	double			m_LumensPerLamp;
	double			m_CandelaMultiplier;
	unsigned int	m_NumberOfVerticalAngles;
	unsigned int	m_NumberOfHorizontalAngles;
	PhotometricType m_PhotometricType;
	unsigned int 	m_UnitsType;
	double			m_LuminaireWidth;
	double			m_LuminaireLength;
	double			m_LuminaireHeight;

	double			BallastFactor;
	double			BallastLampPhotometricFactor;
	double			InputWatts;

	std::vector<double>	m_VerticalAngles; 
	std::vector<double>	m_HorizontalAngles; 

	std::vector< std::vector<double> > m_CandelaValues;

private:
	bool PrivateLoad( const char* );

	bool 			BuildKeywordList();
	void 			BuildDataLine( std::stringstream &, unsigned int, std::vector<double>& );
	bool 			BuildLightData();
	
	bool 			m_bValid;
	std::ifstream	m_fsIES;
};

} //namespace lux

#endif // LUX_PHOTOMETRICDATAIES_H
