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
	// Window size in seconds
	static const unsigned int statisticsWindowSize = 5;

	RendererStatistics();
	virtual ~RendererStatistics() {};

	void reset();
	void updateStatisticsWindow();

	void start();
	void stop();

	// multithread safe while in running state
	double elapsedTime() const;

	class Formatted : public Queryable {
	public:
		virtual ~Formatted() {};

	protected:
		Formatted(RendererStatistics* rs, const std::string& name);

		RendererStatistics* rs;

		std::string getRecommendedString();
		std::string getStringFromTemplate(const std::string& t);

		virtual std::string getRecommendedStringTemplate() = 0;

		std::string getElapsedTime();
		std::string getRemainingTime();
		std::string getHaltTime();
		std::string getHaltThreshold();
	};

	class FormattedShort;	// Forward declaration
	class FormattedLong : public Formatted {
	public:
		virtual ~FormattedLong() {};

	protected:
		FormattedLong(RendererStatistics* rs);

		virtual std::string getRecommendedStringTemplate();

		std::string getPercentComplete();
		std::string getPercentHaltTimeComplete();
		std::string getPercentHaltThresholdComplete();
		std::string getPercentConvergence();

		virtual std::string getEfficiency();
		virtual std::string getEfficiencyWindow();
		virtual std::string getThreadCount();
		virtual std::string getSlaveNodeCount();

		friend class RendererStatistics;
		friend class RendererStatistics::FormattedShort;
	};

	class FormattedShort : public Formatted {
	public:
		virtual ~FormattedShort() {};

	protected:
		FormattedShort(RendererStatistics* rs);

		virtual std::string getRecommendedStringTemplate();

		std::string getPercentHaltTimeComplete();
		std::string getPercentHaltThresholdComplete();
		std::string getPercentConvergence();

		virtual std::string getEfficiency();
		virtual std::string getEfficiencyWindow();
		virtual std::string getThreadCount();
		virtual std::string getSlaveNodeCount();
	};

	FormattedLong* formattedLong;
	FormattedShort* formattedShort;

protected:
	Timer timer;
	boost::mutex windowMutex;
	double windowStartTime;
	double windowCurrentTime;

	double getElapsedTime() { return elapsedTime(); }
	double getHaltTime();
	double getPercentHaltTimeComplete();
	double getHaltThreshold();
	double getPercentHaltThresholdComplete();
	double getPercentConvergence();
	u_int getSlaveNodeCount();

	// These methods must be overridden for renderers
	// which provide alternative measurable halt conditions
	virtual double getRemainingTime();
	virtual double getPercentComplete();

	virtual double getEfficiency() = 0;
	virtual double getEfficiencyWindow() = 0;
	virtual u_int getThreadCount() = 0;

	virtual void resetDerived() = 0;
	virtual void updateStatisticsWindowDerived() = 0;
};

// Reduce the magnitude on the input number by dividing into kilo- or Mega- or Giga- units
// Used in conjuction with MagnitudePrefix
double MagnitudeReduce(double number);

// Return the magnitude prefix char for kilo- or Mega- or Giga-
const char* MagnitudePrefix(double number);

// Append the appropriate 's' or 'es' to a string based on value
std::string Pluralize(const std::string& label, u_int value);

} // namespace lux

#endif // LUX_RENDERERSTATISTICS_H
