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

#ifndef LUX_HYBRIDSAMPLERSTATISTICS_H
#define LUX_HYBRIDSAMPLERSTATISTICS_H

#include <string>
#include <algorithm>

#include <boost/algorithm/string/replace.hpp>

#include <luxrays/core/virtualdevice.h>
#include "rendererstatistics.h"
#include "renderers/hybridsamplerrenderer.h"


namespace lux
{

class HSRStatistics : public RendererStatistics {
public:
	HSRStatistics(HybridSamplerRenderer* renderer);
	~HSRStatistics();

	class FormattedLong : public RendererStatistics::FormattedLong {
	public:
		FormattedLong(HSRStatistics* rs);

	private:
		HSRStatistics* rs;

		virtual std::string getRecommendedStringTemplate();
		virtual std::string getProgress() { return getTotalAverageSamplesPerPixel(); }

		std::string getHaltSpp();
		std::string getRemainingSamplesPerPixel();
		std::string getPercentHaltSppComplete();
		std::string getResumedAverageSamplesPerPixel();

		std::string getGpuCount();
		std::string getAverageGpuEfficiency();

		std::string getPathEfficiency();
		std::string getPathEfficiencyWindow();

		std::string getAverageSamplesPerPixel();
		std::string getAverageSamplesPerSecond();
		std::string getAverageSamplesPerSecondWindow();
		std::string getAverageContributionsPerSecond();
		std::string getAverageContributionsPerSecondWindow();

		std::string getNetworkAverageSamplesPerPixel();
		std::string getNetworkAverageSamplesPerSecond();

		std::string getTotalAverageSamplesPerPixel();
		std::string getTotalAverageSamplesPerSecond();
		std::string getTotalAverageSamplesPerSecondWindow();

		friend class HSRStatistics;
		friend class HSRStatistics::FormattedShort;
	};

	class FormattedShort : public RendererStatistics::FormattedShort {
	public:
		FormattedShort(HSRStatistics* rs);

	private:
		HSRStatistics* rs;

		virtual std::string getRecommendedStringTemplate();
		virtual std::string getProgress();

		std::string getGpuCount();
		std::string getAverageGpuEfficiency();

		std::string getPathEfficiency();
		std::string getPathEfficiencyWindow();
	};

	friend class HybridSamplerRenderer;

protected:
	u_longlong triangleCount;

private:
	HybridSamplerRenderer* renderer;

	double windowSampleCount;
	double exponentialMovingAverage;
	double windowEffSampleCount;
	double windowEffBlackSampleCount;
	double windowPEffSampleCount;
	double windowPEffBlackSampleCount;

	virtual void resetDerived();
	virtual void updateStatisticsWindowDerived();

	virtual double getRemainingTime();
	virtual double getPercentComplete() { return std::max(getPercentHaltTimeComplete(), getPercentHaltSppComplete()); }
	virtual u_int getThreadCount() { return renderer->renderThreads.size(); }

	double getHaltSpp();
	double getRemainingSamplesPerPixel() { return std::max(0.0, getHaltSpp() - getTotalAverageSamplesPerPixel()); }
	double getPercentHaltSppComplete();
	double getResumedAverageSamplesPerPixel() { return getResumedSampleCount() / getPixelCount(); }

	double getEfficiency();
	double getEfficiencyWindow();
	double getPathEfficiency();
	double getPathEfficiencyWindow();

	double getAverageSamplesPerPixel() { return getSampleCount() / getPixelCount(); }
	double getAverageSamplesPerSecond();
	double getAverageSamplesPerSecondWindow();
	double getAverageContributionsPerSecond() { return getAverageSamplesPerSecond() * (getEfficiency() / 100.0); }
	double getAverageContributionsPerSecondWindow() { return getAverageSamplesPerSecondWindow() * (getEfficiency() / 100.0); }

	double getNetworkAverageSamplesPerPixel() { return getNetworkSampleCount() / getPixelCount(); }
	double getNetworkAverageSamplesPerSecond();

	double getTotalAverageSamplesPerPixel() { return (getResumedSampleCount() + getSampleCount() + getNetworkSampleCount()) / getPixelCount(); }
	double getTotalAverageSamplesPerSecond() { return getAverageSamplesPerSecond() + getNetworkAverageSamplesPerSecond(); }
	double getTotalAverageSamplesPerSecondWindow() { return getAverageSamplesPerSecondWindow() + getNetworkAverageSamplesPerSecond(); }

	u_int getPixelCount();
	double getResumedSampleCount();
	double getSampleCount();
	double getNetworkSampleCount(bool estimate = true);
	
	u_int getDeviceCount() {
		luxrays::VirtualIntersectionDevice *vdev = dynamic_cast<luxrays::VirtualIntersectionDevice *>(renderer->intersectionDevice);
		if (vdev)
			return vdev->GetRealDevices().size();
		else
			return 1;
	}
	double getAverageGpuEfficiency();
	string getDeviceNames() {
		vector<luxrays::IntersectionDevice *> realDevices;

		luxrays::VirtualIntersectionDevice *vdev = dynamic_cast<luxrays::VirtualIntersectionDevice *>(renderer->intersectionDevice);
		if (vdev) {
			const vector<luxrays::IntersectionDevice *> &realDevs = vdev->GetRealDevices();
			realDevices.insert(realDevices.end(), realDevs.begin(), realDevs.end());
		} else
			realDevices.push_back(renderer->intersectionDevice);

		// Build the list of device names used
		std::stringstream ss;
		for (u_int i = 0; i < realDevices.size(); ++i) {
			if (i != 0)
				ss << ",";

			// I'm paranoid...
			string name = realDevices[i]->GetName();
			boost::replace_all(name, ",", "_");
			ss << name;
		}

		return ss.str();
	}
	double getTriangleCount() { return triangleCount; }

	double getDeviceMemoryUsed(const u_int deviceIndex) {
		luxrays::VirtualIntersectionDevice *vdev = dynamic_cast<luxrays::VirtualIntersectionDevice *>(renderer->intersectionDevice);
		if (vdev)
			return vdev->GetRealDevices()[deviceIndex]->GetUsedMemory();
		else
			return renderer->intersectionDevice->GetUsedMemory();
	}
	// It looks like this kind of problem can not be solved with boost::bind
	double getDevice00MemoryUsed() { return getDeviceMemoryUsed(0); }
	double getDevice01MemoryUsed() { return getDeviceMemoryUsed(0); }
	double getDevice02MemoryUsed() { return getDeviceMemoryUsed(0); }
	double getDevice03MemoryUsed() { return getDeviceMemoryUsed(3); }
	double getDevice04MemoryUsed() { return getDeviceMemoryUsed(4); }
	double getDevice05MemoryUsed() { return getDeviceMemoryUsed(5); }
	double getDevice06MemoryUsed() { return getDeviceMemoryUsed(6); }
	double getDevice07MemoryUsed() { return getDeviceMemoryUsed(7); }
	double getDevice08MemoryUsed() { return getDeviceMemoryUsed(8); }
	double getDevice09MemoryUsed() { return getDeviceMemoryUsed(9); }
	double getDevice10MemoryUsed() { return getDeviceMemoryUsed(10); }
	double getDevice11MemoryUsed() { return getDeviceMemoryUsed(11); }
	double getDevice12MemoryUsed() { return getDeviceMemoryUsed(12); }
	double getDevice13MemoryUsed() { return getDeviceMemoryUsed(13); }
	double getDevice14MemoryUsed() { return getDeviceMemoryUsed(14); }
	double getDevice15MemoryUsed() { return getDeviceMemoryUsed(15); }

	double getDeviceMaxMemory(const u_int deviceIndex) {
		luxrays::VirtualIntersectionDevice *vdev = dynamic_cast<luxrays::VirtualIntersectionDevice *>(renderer->intersectionDevice);
		if (vdev)
			return vdev->GetRealDevices()[deviceIndex]->GetMaxMemory();
		else
			return renderer->intersectionDevice->GetMaxMemory();
	}
	// It looks like this kind of problem can not be solved with boost::bind
	double getDevice00MaxMemory() { return getDeviceMaxMemory(0); }
	double getDevice01MaxMemory() { return getDeviceMaxMemory(1); }
	double getDevice02MaxMemory() { return getDeviceMaxMemory(2); }
	double getDevice03MaxMemory() { return getDeviceMaxMemory(3); }
	double getDevice04MaxMemory() { return getDeviceMaxMemory(4); }
	double getDevice05MaxMemory() { return getDeviceMaxMemory(5); }
	double getDevice06MaxMemory() { return getDeviceMaxMemory(6); }
	double getDevice07MaxMemory() { return getDeviceMaxMemory(7); }
	double getDevice08MaxMemory() { return getDeviceMaxMemory(8); }
	double getDevice09MaxMemory() { return getDeviceMaxMemory(9); }
	double getDevice10MaxMemory() { return getDeviceMaxMemory(10); }
	double getDevice11MaxMemory() { return getDeviceMaxMemory(11); }
	double getDevice12MaxMemory() { return getDeviceMaxMemory(12); }
	double getDevice13MaxMemory() { return getDeviceMaxMemory(13); }
	double getDevice14MaxMemory() { return getDeviceMaxMemory(14); }
	double getDevice15MaxMemory() { return getDeviceMaxMemory(15); }

	double getDeviceRaySecs(const u_int deviceIndex) {
		luxrays::VirtualIntersectionDevice *vdev = dynamic_cast<luxrays::VirtualIntersectionDevice *>(renderer->intersectionDevice);
		if (vdev)
			return vdev->GetRealDevices()[deviceIndex]->GetSerialPerformance() +
					vdev->GetRealDevices()[deviceIndex]->GetDataParallelPerformance();
		else
			return renderer->intersectionDevice->GetSerialPerformance() +
					renderer->intersectionDevice->GetDataParallelPerformance();
	}
	// It looks like this kind of problem can not be solved with boost::bind
	double getDevice00RaySecs() { return getDeviceRaySecs(0); }
	double getDevice01RaySecs() { return getDeviceRaySecs(1); }
	double getDevice02RaySecs() { return getDeviceRaySecs(2); }
	double getDevice03RaySecs() { return getDeviceRaySecs(3); }
	double getDevice04RaySecs() { return getDeviceRaySecs(4); }
	double getDevice05RaySecs() { return getDeviceRaySecs(5); }
	double getDevice06RaySecs() { return getDeviceRaySecs(6); }
	double getDevice07RaySecs() { return getDeviceRaySecs(7); }
	double getDevice08RaySecs() { return getDeviceRaySecs(8); }
	double getDevice09RaySecs() { return getDeviceRaySecs(9); }
	double getDevice10RaySecs() { return getDeviceRaySecs(10); }
	double getDevice11RaySecs() { return getDeviceRaySecs(11); }
	double getDevice12RaySecs() { return getDeviceRaySecs(12); }
	double getDevice13RaySecs() { return getDeviceRaySecs(13); }
	double getDevice14RaySecs() { return getDeviceRaySecs(14); }
	double getDevice15RaySecs() { return getDeviceRaySecs(15); }
};

}//namespace lux

#endif // LUX_HYBRIDSAMPLERSTATISTICS_H
