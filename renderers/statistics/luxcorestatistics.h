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

#ifndef LUX_LUXCORESTATISTICS_H
#define LUX_LUXCORESTATISTICS_H

#include <boost/function.hpp>
#include <boost/bind.hpp>

#include "renderers/luxcorerenderer.h"
#include "rendererstatistics.h"

#include <algorithm>

namespace lux
{

class LuxCoreStatistics : public RendererStatistics {
public:
	LuxCoreStatistics(LuxCoreRenderer *renderer);
	~LuxCoreStatistics();

	class FormattedLong : public RendererStatistics::FormattedLong {
	public:
		FormattedLong(LuxCoreStatistics *rs);

	private:
		LuxCoreStatistics *rs;

		virtual std::string getRecommendedStringTemplate();
		virtual std::string getProgress() { return getTotalAverageSamplesPerPixel(); }

		std::string getHaltSpp();
		std::string getRemainingSamplesPerPixel();
		std::string getPercentHaltSppComplete();
		std::string getResumedAverageSamplesPerPixel();

		std::string getDeviceCount();
		std::string getDeviceMemoryUsed();

		std::string getAverageSamplesPerPixel();
		std::string getAverageSamplesPerSecond();

		std::string getNetworkAverageSamplesPerPixel();
		std::string getNetworkAverageSamplesPerSecond();

		std::string getTotalAverageSamplesPerPixel();
		std::string getTotalAverageSamplesPerSecond();

		friend class LuxCoreStatistics;
		friend class LuxCoreStatistics::FormattedShort;
	};

	class FormattedShort : public RendererStatistics::FormattedShort {
	public:
		FormattedShort(LuxCoreStatistics *rs);

	private:
		LuxCoreStatistics *rs;

		virtual std::string getRecommendedStringTemplate();
		virtual std::string getProgress();
	};

	friend class LuxCoreRenderer;

protected:
	double averageSampleSec;
	size_t triangleCount;
	u_int deviceCount;
	string deviceNames;
	vector<size_t> deviceMaxMemory, deviceMemoryUsed;
	vector<double> deviceRaySecs;

private:
	LuxCoreRenderer *renderer;

	virtual void resetDerived() { }
	virtual void updateStatisticsWindowDerived() { }

	virtual double getRemainingTime();
	virtual double getPercentComplete() { return std::max(getPercentHaltTimeComplete(), getPercentHaltSppComplete()); }
	virtual u_int getThreadCount() { return getDeviceCount(); }

	double getHaltSpp();
	double getRemainingSamplesPerPixel() { return std::max(0.0, getHaltSpp() - getTotalAverageSamplesPerPixel()); }
	double getPercentHaltSppComplete();
	double getResumedAverageSamplesPerPixel() { return getResumedSampleCount() / getPixelCount(); }

	u_int getDeviceCount() { return deviceCount; }
	string getDeviceNames() { return deviceNames; }
	double getTriangleCount() { return triangleCount; }

	// It looks like this kind of problem can not be solved with boost::bind
	double getDevice00MemoryUsed() { return deviceMemoryUsed[0]; }
	double getDevice01MemoryUsed() { return deviceMemoryUsed[1]; }
	double getDevice02MemoryUsed() { return deviceMemoryUsed[2]; }
	double getDevice03MemoryUsed() { return deviceMemoryUsed[3]; }
	double getDevice04MemoryUsed() { return deviceMemoryUsed[4]; }
	double getDevice05MemoryUsed() { return deviceMemoryUsed[5]; }
	double getDevice06MemoryUsed() { return deviceMemoryUsed[6]; }
	double getDevice07MemoryUsed() { return deviceMemoryUsed[7]; }
	double getDevice08MemoryUsed() { return deviceMemoryUsed[8]; }
	double getDevice09MemoryUsed() { return deviceMemoryUsed[9]; }
	double getDevice10MemoryUsed() { return deviceMemoryUsed[10]; }
	double getDevice11MemoryUsed() { return deviceMemoryUsed[11]; }
	double getDevice12MemoryUsed() { return deviceMemoryUsed[12]; }
	double getDevice13MemoryUsed() { return deviceMemoryUsed[13]; }
	double getDevice14MemoryUsed() { return deviceMemoryUsed[14]; }
	double getDevice15MemoryUsed() { return deviceMemoryUsed[15]; }

	// It looks like this kind of problem can not be solved with boost::bind
	double getDevice00MaxMemory() { return deviceMaxMemory[0]; }
	double getDevice01MaxMemory() { return deviceMaxMemory[1]; }
	double getDevice02MaxMemory() { return deviceMaxMemory[2]; }
	double getDevice03MaxMemory() { return deviceMaxMemory[3]; }
	double getDevice04MaxMemory() { return deviceMaxMemory[4]; }
	double getDevice05MaxMemory() { return deviceMaxMemory[5]; }
	double getDevice06MaxMemory() { return deviceMaxMemory[6]; }
	double getDevice07MaxMemory() { return deviceMaxMemory[7]; }
	double getDevice08MaxMemory() { return deviceMaxMemory[8]; }
	double getDevice09MaxMemory() { return deviceMaxMemory[9]; }
	double getDevice10MaxMemory() { return deviceMaxMemory[10]; }
	double getDevice11MaxMemory() { return deviceMaxMemory[11]; }
	double getDevice12MaxMemory() { return deviceMaxMemory[12]; }
	double getDevice13MaxMemory() { return deviceMaxMemory[13]; }
	double getDevice14MaxMemory() { return deviceMaxMemory[14]; }
	double getDevice15MaxMemory() { return deviceMaxMemory[15]; }

	// It looks like this kind of problem can not be solved with boost::bind
	double getDevice00RaySecs() { return deviceRaySecs[0]; }
	double getDevice01RaySecs() { return deviceRaySecs[1]; }
	double getDevice02RaySecs() { return deviceRaySecs[2]; }
	double getDevice03RaySecs() { return deviceRaySecs[3]; }
	double getDevice04RaySecs() { return deviceRaySecs[4]; }
	double getDevice05RaySecs() { return deviceRaySecs[5]; }
	double getDevice06RaySecs() { return deviceRaySecs[6]; }
	double getDevice07RaySecs() { return deviceRaySecs[7]; }
	double getDevice08RaySecs() { return deviceRaySecs[8]; }
	double getDevice09RaySecs() { return deviceRaySecs[9]; }
	double getDevice10RaySecs() { return deviceRaySecs[10]; }
	double getDevice11RaySecs() { return deviceRaySecs[11]; }
	double getDevice12RaySecs() { return deviceRaySecs[12]; }
	double getDevice13RaySecs() { return deviceRaySecs[13]; }
	double getDevice14RaySecs() { return deviceRaySecs[14]; }
	double getDevice15RaySecs() { return deviceRaySecs[15]; }

	double getEfficiency() { return 0.0; }
	double getEfficiencyWindow() { return 0.0; }
	double getAverageSamplesPerPixel() { return getSampleCount() / getPixelCount(); }
	double getAverageSamplesPerSecond();

	double getNetworkAverageSamplesPerPixel() { return getNetworkSampleCount() / getPixelCount(); }
	double getNetworkAverageSamplesPerSecond();

	double getTotalAverageSamplesPerPixel() { return (getResumedSampleCount() + getSampleCount() + getNetworkSampleCount()) / getPixelCount(); }
	double getTotalAverageSamplesPerSecond() { return getAverageSamplesPerSecond() + getNetworkAverageSamplesPerSecond(); }

	u_int getPixelCount();
	double getResumedSampleCount();
	double getSampleCount();
	double getNetworkSampleCount(bool estimate = true);
};

}//namespace lux

#endif // LUX_LUXCORESTATISTICS_H
