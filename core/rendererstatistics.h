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

#ifndef LUX_RENDERERSTATISTICS_H
#define LUX_RENDERERSTATISTICS_H

// rendererstatistics.h
#include "lux.h"
#include "queryable.h"
#include "timer.h"

#include <string>

#include <boost/thread/mutex.hpp>

namespace lux
{

class Renderer;

class RendererStatistics : public Queryable {
public:
	RendererStatistics();
	virtual ~RendererStatistics() {};

	void reset();
	void updateStatisticsWindow();

	Timer timer;

	class Formatted : public Queryable {
	public:
		Formatted(RendererStatistics* rs);
		virtual ~Formatted() {};

	protected:
		RendererStatistics* rs;

		virtual std::string getRecommendedStringTemplate();

		std::string getStringFromTemplate(const std::string& t, bool shortStrings = false);

		std::string getRecommendedString();
		std::string getRecommendedStringShort();

		std::string getElapsedTime();
		std::string getRemainingTime();
		std::string getHaltTime();
		std::string getPercentHaltTimeComplete();
		std::string getPercentComplete();
		std::string getEfficiency();
		std::string getThreadCount();
		std::string getSlaveNodeCount();

		std::string getPercentHaltTimeCompleteShort();
		std::string getPercentCompleteShort();
		std::string getEfficiencyShort();
		std::string getThreadCountShort();
		std::string getSlaveNodeCountShort();

		std::string pluralize(const std::string& label, u_int value) const;
	};

	Formatted* formatted;

protected:
	boost::mutex windowMutex;
	double windowStartTime;

	double getElapsedTime() { return timer.Time(); }
	double getRemainingTime();
	double getHaltTime();
	double getPercentHaltTimeComplete();
	u_int getSlaveNodeCount();

	// This method must be overridden for renderers
	// which provide alternative measurable halt conditions
	virtual double getPercentComplete();

	virtual double getEfficiency() = 0;
	virtual u_int getThreadCount() = 0;

	virtual void resetDerived() = 0;
	virtual void updateStatisticsWindowDerived() = 0;
};

// Reduce the magnitude on the input number by dividing into kilo- or Mega- or Giga- units
// Used in conjuction with MagnitudePrefix
double MagnitudeReduce(double number);

// Return the magnitude prefix char for kilo- or Mega- or Giga-
const char* MagnitudePrefix(double number);

} // namespace lux

#endif // LUX_RENDERERSTATISTICS_H
