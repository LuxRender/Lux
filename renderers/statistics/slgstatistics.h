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

#ifndef LUX_SLGSTATISTICS_H
#define LUX_SLGSTATISTICS_H

#include "renderers/slgrenderer.h"
#include "rendererstatistics.h"

#include <algorithm>

namespace lux
{

class SLGStatistics : public RendererStatistics {
public:
	SLGStatistics(SLGRenderer *renderer);
	~SLGStatistics();

	class FormattedLong : public RendererStatistics::FormattedLong {
	public:
		FormattedLong(SLGStatistics *rs);

	private:
		SLGStatistics *rs;

		virtual std::string getRecommendedStringTemplate();
		virtual std::string getProgress() { return getTotalAverageSamplesPerPixel(); }

		std::string getHaltSpp();
		std::string getRemainingSamplesPerPixel();
		std::string getPercentHaltSppComplete();
		std::string getResumedAverageSamplesPerPixel();

		std::string getDeviceCount();
		std::string getDeviceMemoryUsage();

		std::string getAverageSamplesPerPixel();
		std::string getAverageSamplesPerSecond();

		std::string getNetworkAverageSamplesPerPixel();
		std::string getNetworkAverageSamplesPerSecond();

		std::string getTotalAverageSamplesPerPixel();
		std::string getTotalAverageSamplesPerSecond();

		friend class SLGStatistics;
		friend class SLGStatistics::FormattedShort;
	};

	class FormattedShort : public RendererStatistics::FormattedShort {
	public:
		FormattedShort(SLGStatistics *rs);

	private:
		SLGStatistics *rs;

		virtual std::string getRecommendedStringTemplate();
		virtual std::string getProgress();
	};

	friend class SLGRenderer;

protected:
	double averageSampleSec;
	u_int deviceCount;
	size_t deviceMemoryUsed;

private:
	SLGRenderer *renderer;

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

#endif // LUX_SLGSTATISTICS_H
