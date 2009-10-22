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

// timer.cpp*
#include "timer.h"
// Timer Method Definitions
Timer::Timer()
{
#if defined( IRIX ) || defined( IRIX64 )
	// IRIX Timer Initialization
	__psunsigned_t phys_addr, raddr;
	int poffmask = getpagesize() - 1;
	int counterSize = syssgi(SGI_CYCLECNTR_SIZE);
	
	phys_addr = syssgi(SGI_QUERY_CYCLECNTR, &(cycleval));
	if (phys_addr == ENODEV) {
		Severe( "Sorry, this SGI doesn't support timers." );
	}
	
	raddr = phys_addr & ~poffmask;
	fd = open("/dev/mmem", O_RDONLY);
	
	if (counterSize == 64) {
		iotimer_addr64 =
			(volatile iotimer64_t *)mmap(0, poffmask, PROT_READ,
			MAP_PRIVATE, fd, (off_t)raddr);
		unmapLocation = (void *)iotimer_addr64;
		unmapSize = poffmask;
		iotimer_addr64 = (iotimer64_t *)((__psunsigned_t)iotimer_addr64 +
				(phys_addr & poffmask));
	}
	else if (counterSize == 32) {
		iotimer_addr32 = (volatile iotimer32_t *)mmap(0, poffmask, PROT_READ,
				MAP_PRIVATE, fd,
				(off_t)raddr);
		unmapLocation = (void *)iotimer_addr32;
		unmapSize = poffmask;
		iotimer_addr32 = (iotimer32_t *)((__psunsigned_t)iotimer_addr32 +
				(phys_addr & poffmask));
	}
	else {
		Severe( "Fatal timer init error" );
	}
#elif defined( WIN32 ) || defined(__CYGWIN__)
	// Windows Timer Initialization
	QueryPerformanceFrequency( &performance_frequency );
	one_over_frequency = 1. / static_cast<double>(performance_frequency.QuadPart);
#endif
	time0 = elapsed = 0;
	running = 0;
}


double Timer::GetTime()
{
#if defined( IRIX ) || defined( IRIX64 )
	// IRIX GetTime
	if (iotimer_addr64) {
		volatile iotimer64_t counter_value;
		counter_value = *(iotimer_addr64);
		return ((double) counter_value * .000000000001) * (double) cycleval;
	}
	else {
		volatile iotimer32_t counter_value;
		counter_value = *(iotimer_addr32);
		return ((double) counter_value * .000000000001) * (double) cycleval;
	}
#elif defined( WIN32 ) || defined(__CYGWIN__)
	// Windows GetTime
	QueryPerformanceCounter( &performance_counter );
	return static_cast<double>(performance_counter.QuadPart) * one_over_frequency;
#else
	// UNIX GetTime
	gettimeofday( &timeofday, NULL );
	return timeofday.tv_sec + timeofday.tv_usec / 1000000.0;
#endif
}

Timer::~Timer()
{
#if defined( IRIX ) || defined( IRIX64 )
	close( fd );
#endif
}

void Timer::Start()
{
	BOOST_ASSERT( !running );
	running = 1;
	time0 = GetTime();
}

void Timer::Stop()
{
	BOOST_ASSERT( running );
	running = 0;

	elapsed += GetTime() - time0;
}

void Timer::Reset()
{
    running = 0;
    elapsed = 0;
}

double Timer::Time()
{
	if (running) {
		Stop();
		Start();
	}
	return elapsed;
}
