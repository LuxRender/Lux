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

#ifndef LUX_HYBRIDSAMPLERSTATISTICS_H
#define LUX_HYBRIDSAMPLERSTATISTICS_H

#include "rendererstatistics.h"
#include "renderers/hybridsamplerrenderer.h"

#include <algorithm>

namespace lux
{

class HSRStatistics : public RendererStatistics {
public:
	HSRStatistics(HybridSamplerRenderer* renderer);
	~HSRStatistics() { delete formatted; }

	class Formatted : public RendererStatistics::Formatted {
	public:
		Formatted(HSRStatistics* rs);

	private:
		HSRStatistics* rs;

		virtual std::string getRecommendedStringTemplate();

		std::string getHaltSpp();
		std::string getRemainingSamplesPerPixel();
		std::string getPercentHaltSppComplete();

		std::string getGpuCount();
		std::string getAverageGpuEfficiency();

		std::string getAverageSamplesPerPixel();
		std::string getAverageSamplesPerSecond();
		std::string getAverageSamplesPerSecondWindow();
		std::string getAverageContributionsPerSecond();
		std::string getAverageContributionsPerSecondWindow();

		std::string getNetworkAverageSamplesPerPixel();
		std::string getNetworkAverageSamplesPerSecond();
		std::string getNetworkAverageSamplesPerSecondWindow();

		std::string getTotalAverageSamplesPerPixel();
		std::string getTotalAverageSamplesPerSecond();
		std::string getTotalAverageSamplesPerSecondWindow();

		std::string getPercentHaltSppCompleteShort();

		std::string getGpuCountShort();
		std::string getAverageGpuEfficiencyShort();
	};

private:
	HybridSamplerRenderer* renderer;

	double windowSps;
	double windowSampleCount;
	double windowNetworkSps;
	double windowNetworkStartTime;
	double windowNetworkSampleCount;

	virtual void resetDerived();
	virtual void updateStatisticsWindowDerived();

	virtual double getPercentComplete() { return (std::max)(getPercentHaltTimeComplete(), getPercentHaltSppComplete()); }
	virtual u_int getThreadCount() { return renderer->renderThreads.size(); }

	double getHaltSpp();
	double getEfficiency();
	double getRemainingSamplesPerPixel() { return (std::max)(0.0, getHaltSpp() - (getAverageSamplesPerPixel() + getNetworkAverageSamplesPerPixel())); }
	double getPercentHaltSppComplete();

	u_int getGpuCount() { return renderer->hardwareDevices.size(); };
	double getAverageGpuEfficiency();

	double getAverageSamplesPerPixel() { return getSampleCount() / getPixelCount(); }
	double getAverageSamplesPerSecond();
	double getAverageSamplesPerSecondWindow() { return windowSps; }
	double getAverageContributionsPerSecond() { return getAverageSamplesPerSecond() * (getEfficiency() / 100.0); }
	double getAverageContributionsPerSecondWindow() { return windowSps * (getEfficiency() / 100.0); }

	double getNetworkAverageSamplesPerPixel() { return getNetworkSampleCount() / getPixelCount(); }
	double getNetworkAverageSamplesPerSecond();
	double getNetworkAverageSamplesPerSecondWindow() { return windowNetworkSps; }

	double getTotalAverageSamplesPerPixel() { return getAverageSamplesPerPixel() + getNetworkAverageSamplesPerPixel(); }
	double getTotalAverageSamplesPerSecond() { return getAverageSamplesPerSecond() + getNetworkAverageSamplesPerSecond(); }
	double getTotalAverageSamplesPerSecondWindow() { return getAverageSamplesPerSecondWindow() + getNetworkAverageSamplesPerSecondWindow(); }

	u_int getPixelCount();
	double getSampleCount();
	double getNetworkSampleCount(bool estimate = true);
};

}//namespace lux

#endif // LUX_HYBRIDSAMPLERSTATISTICS_H
