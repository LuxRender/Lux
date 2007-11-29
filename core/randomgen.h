/***************************************************************************
 *   Copyright (C) 1998-2007 by authors (see AUTHORS.txt )                 *
 *                                                                         *
 *   This file is part of Lux Renderer.                                    *
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
 *   Lux Renderer website : http://www.luxrender.org                       *
 ***************************************************************************/


#ifndef LUX_RANDOM_H
#define LUX_RANDOM_H

#ifndef LUX_USE_BOOST_RANDOM 

#include <ctime>
#include <climits>
#define MEXP 216091
#include "./SFMT/SFMT.h"

#else //LUX_USE_BOOST_RANDOM 
#include <boost/random.hpp>
#include <boost/random/mersenne_twister.hpp>
#endif //LUX_USE_BOOST_RANDOM 

namespace lux
{


namespace random
{

#ifndef LUX_USE_BOOST_RANDOM 
	inline void init()
	{
		init_gen_rand(std::clock());
	}

	inline float floatValue()
	{
		//return genrand_real2();
		return gen_rand32()*(1.f/4294967296.f);
	}
		
	inline unsigned long uintValue()
	{
		return gen_rand32();
	}
#else

	class RandomGenerator
	{
	public:
		RandomGenerator() : engine(static_cast<unsigned int>(std::time(0))), uni_dist(0,1), uni(engine, uni_dist), intdist(0,INT_MAX), intgen(engine, intdist)
		{ }
		
		inline float generateFloat()
		{
			return uni();
		}
		
		inline unsigned long generateUInt()
		{
			return intgen();
		}
		
	private:
		boost::mt19937 engine;
		
		boost::uniform_real<> uni_dist;
		boost::variate_generator<boost::mt19937, boost::uniform_real<> > uni;
		
		boost::uniform_int<> intdist;
		boost::variate_generator<boost::mt19937, boost::uniform_int<> > intgen;
		
	};
	
	extern RandomGenerator *myGen;
	
	inline void init ()
	{
		init_gen_rand(std::clock());
		myGen=new RandomGenerator();
	}
	
	inline float floatValue()
	{ 
		return myGen->generateFloat();
	}
	inline unsigned long uintValue()
	{ 
		return myGen->generateUInt();
	}
	
	
#endif //LUX_USE_BOOST_RANDOM */
	

}

}

#endif //LUX_RANDOM_H

