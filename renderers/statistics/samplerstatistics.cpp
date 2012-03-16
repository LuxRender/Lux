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

#include "renderers/statistics/samplerstatistics.h"
#include "camera.h"
#include "context.h"
#include "film.h"
#include "scene.h"

#include <limits>
#include <string>

#include <boost/format.hpp>

using namespace lux;

SRStatistics::SRStatistics(SamplerRenderer* renderer)
	: renderer(renderer),
	windowSps(0.0), windowSampleCount(0.0),
	windowNetworkSps(0.0), windowNetworkStartTime(0.0), windowNetworkSampleCount(0.0)
{
	formatted = new SRStatistics::Formatted(this);

	AddDoubleAttribute(*this, "haltSamplesPerPixel", "Average number of samples per pixel to complete before halting", &SRStatistics::getHaltSpp);
	AddDoubleAttribute(*this, "remainingSamplesPerPixel", "Average number of samples per pixel remaining", &SRStatistics::getRemainingSamplesPerPixel);
	AddDoubleAttribute(*this, "percentHaltSppComplete", "Percent of halt S/p completed", &SRStatistics::getPercentHaltSppComplete);

	AddDoubleAttribute(*this, "samplesPerPixel", "Average number of samples per pixel by local node", &SRStatistics::getAverageSamplesPerPixel);
	AddDoubleAttribute(*this, "samplesPerSecond", "Average number of samples per second by local node", &SRStatistics::getAverageSamplesPerSecond);
	AddDoubleAttribute(*this, "samplesPerSecondWindow", "Average number of samples per second by local node in current time window", &SRStatistics::getAverageSamplesPerSecondWindow);
	AddDoubleAttribute(*this, "contributionsPerSecond", "Average number of contributions per second by local node", &SRStatistics::getAverageContributionsPerSecond);
	AddDoubleAttribute(*this, "contributionsPerSecondWindow", "Average number of contributions per second by local node in current time window", &SRStatistics::getAverageContributionsPerSecondWindow);

	AddDoubleAttribute(*this, "netSamplesPerPixel", "Average number of samples per pixel by slave nodes", &SRStatistics::getNetworkAverageSamplesPerPixel);
	AddDoubleAttribute(*this, "netSamplesPerSecond", "Average number of samples per second by slave nodes", &SRStatistics::getNetworkAverageSamplesPerSecond);
	AddDoubleAttribute(*this, "netSamplesPerSecondWindow", "Average number of samples per second by slave nodes in current time window", &SRStatistics::getNetworkAverageSamplesPerSecondWindow);

	AddDoubleAttribute(*this, "totalSamplesPerPixel", "Average number of samples per pixel", &SRStatistics::getTotalAverageSamplesPerPixel);
	AddDoubleAttribute(*this, "totalSamplesPerSecond", "Average number of samples per second", &SRStatistics::getTotalAverageSamplesPerSecond);
	AddDoubleAttribute(*this, "totalSamplesPerSecondWindow", "Average number of samples per second in current time window", &SRStatistics::getTotalAverageSamplesPerSecondWindow);
}

void SRStatistics::resetDerived() {
	windowSps = 0.0;
	windowSampleCount = 0.0;

	windowNetworkSps = 0.0;
	windowNetworkStartTime = 0.0;
	windowNetworkSampleCount = 0.0;
}

void SRStatistics::updateStatisticsWindowDerived()
{
	// Get local sample count
	double sampleCount = getSampleCount();
	windowSps = (sampleCount - windowSampleCount) / (getElapsedTime() - windowStartTime);
	windowSampleCount = sampleCount;

	// Get network sample count
	double networkSampleCount = 0.0;
	Queryable* filmRegistry = Context::GetActive()->registry["film"];
	if (filmRegistry)
		networkSampleCount = (*filmRegistry)["numberOfSamplesFromNetwork"].DoubleValue();

	if (networkSampleCount != windowNetworkSampleCount)
	{
		windowNetworkSps = (networkSampleCount - windowNetworkSampleCount) / (getElapsedTime() - windowNetworkStartTime);
		windowNetworkSampleCount = networkSampleCount;
		windowNetworkStartTime = getElapsedTime();
	}
}

// Returns haltSamplesPerPixel if set, otherwise infinity
double SRStatistics::getHaltSpp() {
	double haltSpp = 0.0;

	Queryable* filmRegistry = Context::GetActive()->registry["film"];
	if (filmRegistry)
		haltSpp = (*filmRegistry)["haltSamplesPerPixel"].IntValue();

	return haltSpp > 0.0 ? haltSpp : std::numeric_limits<double>::infinity();
}

double SRStatistics::getEfficiency() {
	double sampleCount = 0.0;
	double blackSampleCount = 0.0;

	// Get the current counts from the renderthreads
	// Cannot just use getSampleCount() because the blackSampleCount is necessary
	boost::mutex::scoped_lock lock(renderer->renderThreadsMutex);
	for (u_int i = 0; i < renderer->renderThreads.size(); ++i) {
		fast_mutex::scoped_lock lockStats(renderer->renderThreads[i]->statLock);
		sampleCount += renderer->renderThreads[i]->samples;
		blackSampleCount += renderer->renderThreads[i]->blackSamples;
	}

	return sampleCount ? (100.0 * blackSampleCount) / sampleCount : 0.0;
}

// Returns percent of haltSamplesPerPixel completed, zero if haltSamplesPerPixel is not set
double SRStatistics::getPercentHaltSppComplete() {
	return ((getAverageSamplesPerPixel() + getNetworkAverageSamplesPerPixel()) / getHaltSpp()) * 100.0;
}

double SRStatistics::getAverageSamplesPerSecond() {
	double et = getElapsedTime();
	return (et == 0.0) ? 0.0 : getSampleCount() / et;
}

double SRStatistics::getNetworkAverageSamplesPerSecond() {
	double et = getElapsedTime();
	return (et == 0.0) ? 0.0 : getNetworkSampleCount() / et;
}

u_int SRStatistics::getPixelCount() {
	int xstart, xend, ystart, yend;

	renderer->scene->camera->film->GetSampleExtent(&xstart, &xend, &ystart, &yend);

	return ((xend - xstart) * (yend - ystart));
}

double SRStatistics::getSampleCount() {
	double sampleCount = 0.0;

	Queryable* filmRegistry = Context::GetActive()->registry["film"];
	if (filmRegistry)
		sampleCount = (*filmRegistry)["numberOfLocalSamples"].DoubleValue();

	return sampleCount;
}

double SRStatistics::getNetworkSampleCount(bool estimate) {
	double networkSampleCount = 0.0;

	Queryable* filmRegistry = Context::GetActive()->registry["film"];
	if (filmRegistry)
		networkSampleCount = (*filmRegistry)["numberOfSamplesFromNetwork"].DoubleValue();

	// Add estimated network sample count
	if (estimate && networkSampleCount == windowNetworkSampleCount)
		networkSampleCount += ((getElapsedTime() - windowNetworkStartTime) * windowNetworkSps);

	return networkSampleCount;
}

SRStatistics::Formatted::Formatted(SRStatistics* rs)
	: RendererStatistics::Formatted(rs), rs(rs)
{
	AddStringAttribute(*this, "haltSamplesPerPixel", "Average number of samples per pixel to complete before halting", &SRStatistics::Formatted::getHaltSpp);
	AddStringAttribute(*this, "remainingSamplesPerPixel", "Average number of samples per pixel remaining", &SRStatistics::Formatted::getRemainingSamplesPerPixel);
	AddStringAttribute(*this, "percentHaltSppComplete", "Percent of halt S/p completed", &SRStatistics::Formatted::getPercentHaltSppComplete);
	AddStringAttribute(*this, "_percentHaltSppComplete_short", "Percent of halt S/p completed", &SRStatistics::Formatted::getPercentHaltSppCompleteShort);

	AddStringAttribute(*this, "samplesPerPixel", "Average number of samples per pixel by local node", &SRStatistics::Formatted::getAverageSamplesPerPixel);
	AddStringAttribute(*this, "samplesPerSecond", "Average number of samples per second by local node", &SRStatistics::Formatted::getAverageSamplesPerSecond);
	AddStringAttribute(*this, "samplesPerSecondWindow", "Average number of samples per second by local node in current time window", &SRStatistics::Formatted::getAverageSamplesPerSecondWindow);
	AddStringAttribute(*this, "contributionsPerSecond", "Average number of contributions per second by local node", &SRStatistics::Formatted::getAverageContributionsPerSecond);
	AddStringAttribute(*this, "contributionsPerSecondWindow", "Average number of contributions per second by local node in current time window", &SRStatistics::Formatted::getAverageContributionsPerSecondWindow);

	AddStringAttribute(*this, "netSamplesPerPixel", "Average number of samples per pixel by slave nodes", &SRStatistics::Formatted::getNetworkAverageSamplesPerPixel);
	AddStringAttribute(*this, "netSamplesPerSecond", "Average number of samples per second by slave nodes", &SRStatistics::Formatted::getNetworkAverageSamplesPerSecond);
	AddStringAttribute(*this, "netSamplesPerSecondWindow", "Average number of samples per second by slave nodes in current time window", &SRStatistics::Formatted::getNetworkAverageSamplesPerSecondWindow);

	AddStringAttribute(*this, "totalSamplesPerPixel", "Average number of samples per pixel", &SRStatistics::Formatted::getTotalAverageSamplesPerPixel);
	AddStringAttribute(*this, "totalSamplesPerSecond", "Average number of samples per second", &SRStatistics::Formatted::getTotalAverageSamplesPerSecond);
	AddStringAttribute(*this, "totalSamplesPerSecondWindow", "Average number of samples per second in current time window", &SRStatistics::Formatted::getTotalAverageSamplesPerSecondWindow);
}

std::string SRStatistics::Formatted::getRecommendedStringTemplate()
{
	std::string stringTemplate = RendererStatistics::Formatted::getRecommendedStringTemplate();
	stringTemplate += ": %samplesPerPixel%";
	if (rs->getHaltSpp() != std::numeric_limits<double>::infinity())
		stringTemplate += " (%percentHaltSppComplete%)";
	stringTemplate += " %samplesPerSecondWindow% %contributionsPerSecondWindow% %efficiency%";
	if (rs->getSlaveNodeCount() != 0 && rs->getNetworkAverageSamplesPerPixel() != 0.0)
		stringTemplate += " Net: ~%netSamplesPerPixel% ~%netSamplesPerSecondWindow% Tot: ~%totalSamplesPerPixel% ~%totalSamplesPerSecondWindow%";

	return stringTemplate;
}

std::string SRStatistics::Formatted::getHaltSpp() {
	return boost::str(boost::format("%1% S/p") % rs->getHaltSpp());
}

std::string SRStatistics::Formatted::getRemainingSamplesPerPixel() {
	double rspp = rs->getRemainingSamplesPerPixel();
	return boost::str(boost::format("%1$0.2f%2% S/p") % MagnitudeReduce(rspp) % MagnitudePrefix(rspp));
}

std::string SRStatistics::Formatted::getPercentHaltSppComplete() {
	return boost::str(boost::format("%1$0.0f%% S/p Complete") % rs->getPercentHaltSppComplete());
}

std::string SRStatistics::Formatted::getPercentHaltSppCompleteShort() {
	return boost::str(boost::format("%1$0.0f%% S/p Cmplt") % rs->getPercentHaltSppComplete());
}

std::string SRStatistics::Formatted::getAverageSamplesPerPixel() {
	double spp = rs->getAverageSamplesPerPixel();
	return boost::str(boost::format("%1$0.2f%2% S/p") % MagnitudeReduce(spp) % MagnitudePrefix(spp));
}

std::string SRStatistics::Formatted::getAverageSamplesPerSecond() {
	double sps = rs->getAverageSamplesPerSecond();
	return boost::str(boost::format("%1$0.2f%2% S/s") % MagnitudeReduce(sps) % MagnitudePrefix(sps));
}

std::string SRStatistics::Formatted::getAverageSamplesPerSecondWindow() {
	double spsw = rs->getAverageSamplesPerSecondWindow();
	return boost::str(boost::format("%1$0.2f%2% S/s") % MagnitudeReduce(spsw) % MagnitudePrefix(spsw));
}

std::string SRStatistics::Formatted::getAverageContributionsPerSecond() {
	double cps = rs->getAverageContributionsPerSecond();
	return boost::str(boost::format("%1$0.2f%2% C/s") % MagnitudeReduce(cps) % MagnitudePrefix(cps));
}

std::string SRStatistics::Formatted::getAverageContributionsPerSecondWindow() {
	double cpsw = rs->getAverageContributionsPerSecondWindow();
	return boost::str(boost::format("%1$0.2f%2% C/s") % MagnitudeReduce(cpsw) % MagnitudePrefix(cpsw));
}

std::string SRStatistics::Formatted::getNetworkAverageSamplesPerPixel() {
	double spp = rs->getNetworkAverageSamplesPerPixel();
	return boost::str(boost::format("%1$0.2f%2% S/p") % MagnitudeReduce(spp) % MagnitudePrefix(spp));
}

std::string SRStatistics::Formatted::getNetworkAverageSamplesPerSecond() {
	double sps = rs->getNetworkAverageSamplesPerSecond();
	return boost::str(boost::format("%1$0.2f%2% S/s") % MagnitudeReduce(sps) % MagnitudePrefix(sps));
}

std::string SRStatistics::Formatted::getNetworkAverageSamplesPerSecondWindow() {
	double spsw = rs->getNetworkAverageSamplesPerSecondWindow();
	return boost::str(boost::format("%1$0.2f%2% S/s") % MagnitudeReduce(spsw) % MagnitudePrefix(spsw));
}

std::string SRStatistics::Formatted::getTotalAverageSamplesPerPixel() {
	double spp = rs->getTotalAverageSamplesPerPixel();
	return boost::str(boost::format("%1$0.2f%2% S/p") % MagnitudeReduce(spp) % MagnitudePrefix(spp));
}

std::string SRStatistics::Formatted::getTotalAverageSamplesPerSecond() {
	double sps = rs->getTotalAverageSamplesPerSecond();
	return boost::str(boost::format("%1$0.2f%2% S/s") % MagnitudeReduce(sps) % MagnitudePrefix(sps));
}

std::string SRStatistics::Formatted::getTotalAverageSamplesPerSecondWindow() {
	double spsw = rs->getTotalAverageSamplesPerSecondWindow();
	return boost::str(boost::format("%1$0.2f%2% S/s") % MagnitudeReduce(spsw) % MagnitudePrefix(spsw));
}
