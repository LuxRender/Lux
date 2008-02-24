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


#ifndef LUX_RANDOM_H
#define LUX_RANDOM_H

#include <boost/random.hpp>
#include <boost/random/mersenne_twister.hpp>

#include "../renderer/include/asio.hpp"
#include <boost/thread/tss.hpp>

namespace lux
{

namespace random
{
#if 0
inline unsigned int seed()
{
	int dummy;
	unsigned int seed=static_cast<unsigned int>(std::time(0));
	//if two copies run on the same machine, dummy adress will make the seed unique
	seed=seed^(unsigned int)((unsigned long)(&dummy)); 
	//also use the hostname to make the seed unique
	/*std::string s=asio::ip::host_name();
	for(unsigned int i=0;i<s.size();i++)
		seed=seed^(((unsigned int)s.at(i))<<((i%4)*8));*/
	seed=seed^DJBHash(asio::ip::host_name());
	//std::cout<<"using seed :"<<seed<<std::endl;
	return seed;
}
#endif
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
	
	// thread local pointer to boost random generator
	extern boost::thread_specific_ptr<RandomGenerator> myGen;
	
	inline void init ()
	{
		myGen.reset(new RandomGenerator);
	}
	
	inline float floatValue()
	{ 
		if(!myGen.get())
			init();
		return myGen->generateFloat();
	}
	inline unsigned long uintValue()
	{ 
		if(!myGen.get())
			init();
		return myGen->generateUInt();
	}
	
}

}

#endif //LUX_RANDOM_H
