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
#include "error.h"
// Timer Method Definitions
void Timer::Start() {
	if (!running ) {
		time0 = boost::posix_time::microsec_clock::universal_time();
		running = true;
	}
}

void Timer::Stop() {
	if (running) {
		elapsed = Time();
		running = false;
	}
}

void Timer::Reset() {
	running = false;
	elapsed = 0.0;
}
	
double Timer::Time() const {
	if (running) {
		boost::posix_time::time_duration td = boost::posix_time::microsec_clock::universal_time() - time0;
		return elapsed + td.total_milliseconds() / 1000.0;
	} else
		return elapsed;
}
