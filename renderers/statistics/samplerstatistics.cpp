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

#include <boost/bind.hpp>
#include <boost/format.hpp>

using namespace lux;

SRStatistics::SRStatistics(SamplerRenderer* renderer)
	: renderer(renderer),
	windowSps(0.0), windowSampleCount(0.0),
	windowNetworkSps(0.0), windowNetworkStartTime(0.0), windowNetworkSampleCount(0.0)
{
	formattedLong = new SRStatistics::FormattedLong(this);
	formattedShort = new SRStatistics::FormattedShort(this);

	AddDoubleAttribute(*this, "haltSamplesPerPixel", "Average number of samples per pixel to complete before halting", &SRStatistics::getHaltSpp);
	AddDoubleAttribute(*this, "remainingSamplesPerPixel", "Average number of samples per pixel remaining", &SRStatistics::getRemainingSamplesPerPixel);
	AddDoubleAttribute(*this, "percentHaltSppComplete", "Percent of halt S/p completed", &SRStatistics::getPercentHaltSppComplete);

	AddDoubleAttribute(*this, "resumedSamplesPerPixel", "Average number of samples per pixel loaded from FLM", &SRStatistics::getResumedAverageSamplesPerPixel);

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

SRStatistics::~SRStatistics()
{
	delete formattedLong;
	delete formattedShort;
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
	double networkSampleCount = getNetworkSampleCount(false);
	if (networkSampleCount != windowNetworkSampleCount)
	{
		windowNetworkSps = (networkSampleCount - windowNetworkSampleCount) / (getElapsedTime() - windowNetworkStartTime);
		windowNetworkSampleCount = networkSampleCount;
		windowNetworkStartTime = getElapsedTime();
	}
	else if (getSlaveNodeCount() == 0 && windowNetworkStartTime != 0.0)
	{
		windowNetworkSps = 0.0;
		windowNetworkStartTime = 0.0;
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

double SRStatistics::getResumedSampleCount() {
	double resumedSampleCount = 0.0;

	Queryable* filmRegistry = Context::GetActive()->registry["film"];
	if (filmRegistry)
		resumedSampleCount = (*filmRegistry)["numberOfResumedSamples"].DoubleValue();

	return resumedSampleCount;
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
	if (estimate && getSlaveNodeCount() != 0 && networkSampleCount == windowNetworkSampleCount)
		networkSampleCount += ((getElapsedTime() - windowNetworkStartTime) * windowNetworkSps);

	return networkSampleCount;
}

SRStatistics::FormattedLong::FormattedLong(SRStatistics* rs)
	: RendererStatistics::FormattedLong(rs), rs(rs)
{
	typedef SRStatistics::FormattedLong FL;
	AddStringAttribute(*this, "haltSamplesPerPixel", "Average number of samples per pixel to complete before halting", &FL::getHaltSpp);
	AddStringAttribute(*this, "remainingSamplesPerPixel", "Average number of samples per pixel remaining", &FL::getRemainingSamplesPerPixel);
	AddStringAttribute(*this, "percentHaltSppComplete", "Percent of halt S/p completed", &FL::getPercentHaltSppComplete);

	AddStringAttribute(*this, "resumedSamplesPerPixel", "Average number of samples per pixel loaded from FLM", &FL::getResumedAverageSamplesPerPixel);

	AddStringAttribute(*this, "samplesPerPixel", "Average number of samples per pixel by local node", &FL::getAverageSamplesPerPixel);
	AddStringAttribute(*this, "samplesPerSecond", "Average number of samples per second by local node", &FL::getAverageSamplesPerSecond);
	AddStringAttribute(*this, "samplesPerSecondWindow", "Average number of samples per second by local node in current time window", &FL::getAverageSamplesPerSecondWindow);
	AddStringAttribute(*this, "contributionsPerSecond", "Average number of contributions per second by local node", &FL::getAverageContributionsPerSecond);
	AddStringAttribute(*this, "contributionsPerSecondWindow", "Average number of contributions per second by local node in current time window", &FL::getAverageContributionsPerSecondWindow);

	AddStringAttribute(*this, "netSamplesPerPixel", "Average number of samples per pixel by slave nodes", &FL::getNetworkAverageSamplesPerPixel);
	AddStringAttribute(*this, "netSamplesPerSecond", "Average number of samples per second by slave nodes", &FL::getNetworkAverageSamplesPerSecond);
	AddStringAttribute(*this, "netSamplesPerSecondWindow", "Average number of samples per second by slave nodes in current time window", &FL::getNetworkAverageSamplesPerSecondWindow);

	AddStringAttribute(*this, "totalSamplesPerPixel", "Average number of samples per pixel", &FL::getTotalAverageSamplesPerPixel);
	AddStringAttribute(*this, "totalSamplesPerSecond", "Average number of samples per second", &FL::getTotalAverageSamplesPerSecond);
	AddStringAttribute(*this, "totalSamplesPerSecondWindow", "Average number of samples per second in current time window", &FL::getTotalAverageSamplesPerSecondWindow);
}

std::string SRStatistics::FormattedLong::getRecommendedStringTemplate()
{
	std::string stringTemplate = RendererStatistics::FormattedLong::getRecommendedStringTemplate();
	stringTemplate += ": %samplesPerPixel%";
	if (rs->getHaltSpp() != std::numeric_limits<double>::infinity())
		stringTemplate += " (%percentHaltSppComplete%)";
	stringTemplate += " %samplesPerSecondWindow% %contributionsPerSecondWindow% %efficiency%";

	if (rs->getNetworkSampleCount() != 0.0)
	{
		if (rs->getSlaveNodeCount() != 0)
			stringTemplate += " | Net: ~%netSamplesPerPixel% ~%netSamplesPerSecondWindow%";
		else
			stringTemplate += " | Net: %netSamplesPerPixel% %netSamplesPerSecondWindow%";
	}

	if (rs->getNetworkSampleCount() != 0.0 && rs->getSlaveNodeCount())
		stringTemplate += " | Tot: ~%totalSamplesPerPixel% ~%totalSamplesPerSecondWindow%";
	else if (rs->getResumedSampleCount() != 0.0)
		stringTemplate += " | Tot: %totalSamplesPerPixel% %totalSamplesPerSecondWindow%";

	return stringTemplate;
}

std::string SRStatistics::FormattedLong::getHaltSpp() {
	return boost::str(boost::format("%1% S/p") % rs->getHaltSpp());
}

std::string SRStatistics::FormattedLong::getRemainingSamplesPerPixel() {
	double rspp = rs->getRemainingSamplesPerPixel();
	return boost::str(boost::format("%1$0.2f %2%S/p") % MagnitudeReduce(rspp) % MagnitudePrefix(rspp));
}

std::string SRStatistics::FormattedLong::getPercentHaltSppComplete() {
	return boost::str(boost::format("%1$0.0f%% S/p Complete") % rs->getPercentHaltSppComplete());
}

std::string SRStatistics::FormattedLong::getResumedAverageSamplesPerPixel() {
	double spp = rs->getResumedAverageSamplesPerPixel();
	return boost::str(boost::format("%1$0.2f %2%S/p") % MagnitudeReduce(spp) % MagnitudePrefix(spp));
}

std::string SRStatistics::FormattedLong::getAverageSamplesPerPixel() {
	double spp = rs->getAverageSamplesPerPixel();
	return boost::str(boost::format("%1$0.2f %2%S/p") % MagnitudeReduce(spp) % MagnitudePrefix(spp));
}

std::string SRStatistics::FormattedLong::getAverageSamplesPerSecond() {
	double sps = rs->getAverageSamplesPerSecond();
	return boost::str(boost::format("%1$0.2f %2%S/s") % MagnitudeReduce(sps) % MagnitudePrefix(sps));
}

std::string SRStatistics::FormattedLong::getAverageSamplesPerSecondWindow() {
	double spsw = rs->getAverageSamplesPerSecondWindow();
	return boost::str(boost::format("%1$0.2f %2%S/s") % MagnitudeReduce(spsw) % MagnitudePrefix(spsw));
}

std::string SRStatistics::FormattedLong::getAverageContributionsPerSecond() {
	double cps = rs->getAverageContributionsPerSecond();
	return boost::str(boost::format("%1$0.2f %2%C/s") % MagnitudeReduce(cps) % MagnitudePrefix(cps));
}

std::string SRStatistics::FormattedLong::getAverageContributionsPerSecondWindow() {
	double cpsw = rs->getAverageContributionsPerSecondWindow();
	return boost::str(boost::format("%1$0.2f %2%C/s") % MagnitudeReduce(cpsw) % MagnitudePrefix(cpsw));
}

std::string SRStatistics::FormattedLong::getNetworkAverageSamplesPerPixel() {
	double spp = rs->getNetworkAverageSamplesPerPixel();
	return boost::str(boost::format("%1$0.2f %2%S/p") % MagnitudeReduce(spp) % MagnitudePrefix(spp));
}

std::string SRStatistics::FormattedLong::getNetworkAverageSamplesPerSecond() {
	double sps = rs->getNetworkAverageSamplesPerSecond();
	return boost::str(boost::format("%1$0.2f %2%S/s") % MagnitudeReduce(sps) % MagnitudePrefix(sps));
}

std::string SRStatistics::FormattedLong::getNetworkAverageSamplesPerSecondWindow() {
	double spsw = rs->getNetworkAverageSamplesPerSecondWindow();
	return boost::str(boost::format("%1$0.2f %2%S/s") % MagnitudeReduce(spsw) % MagnitudePrefix(spsw));
}

std::string SRStatistics::FormattedLong::getTotalAverageSamplesPerPixel() {
	double spp = rs->getTotalAverageSamplesPerPixel();
	return boost::str(boost::format("%1$0.2f %2%S/p") % MagnitudeReduce(spp) % MagnitudePrefix(spp));
}

std::string SRStatistics::FormattedLong::getTotalAverageSamplesPerSecond() {
	double sps = rs->getTotalAverageSamplesPerSecond();
	return boost::str(boost::format("%1$0.2f %2%S/s") % MagnitudeReduce(sps) % MagnitudePrefix(sps));
}

std::string SRStatistics::FormattedLong::getTotalAverageSamplesPerSecondWindow() {
	double spsw = rs->getTotalAverageSamplesPerSecondWindow();
	return boost::str(boost::format("%1$0.2f %2%S/s") % MagnitudeReduce(spsw) % MagnitudePrefix(spsw));
}

SRStatistics::FormattedShort::FormattedShort(SRStatistics* rs)
	: RendererStatistics::FormattedShort(rs), rs(rs)
{
	FormattedLong* fl = static_cast<SRStatistics::FormattedLong*>(rs->formattedLong);

	typedef SRStatistics::FormattedLong FL;
	AddStringAttribute(*this, "haltSamplesPerPixel", "Average number of samples per pixel to complete before halting", boost::bind(&FL::getHaltSpp, fl));
	AddStringAttribute(*this, "remainingSamplesPerPixel", "Average number of samples per pixel remaining", boost::bind(&FL::getRemainingSamplesPerPixel, fl));
	AddStringAttribute(*this, "percentHaltSppComplete", "Percent of halt S/p completed", &SRStatistics::FormattedShort::getPercentHaltSppComplete);

	AddStringAttribute(*this, "resumedSamplesPerPixel", "Average number of samples per pixel loaded from FLM", boost::bind(&FL::getResumedAverageSamplesPerPixel, fl));

	AddStringAttribute(*this, "samplesPerPixel", "Average number of samples per pixel by local node", boost::bind(&FL::getAverageSamplesPerPixel, fl));
	AddStringAttribute(*this, "samplesPerSecond", "Average number of samples per second by local node", boost::bind(&FL::getAverageSamplesPerSecond, fl));
	AddStringAttribute(*this, "samplesPerSecondWindow", "Average number of samples per second by local node in current time window", boost::bind(&FL::getAverageSamplesPerSecondWindow, fl));
	AddStringAttribute(*this, "contributionsPerSecond", "Average number of contributions per second by local node", boost::bind(&FL::getAverageContributionsPerSecond, fl));
	AddStringAttribute(*this, "contributionsPerSecondWindow", "Average number of contributions per second by local node in current time window", boost::bind(&FL::getAverageContributionsPerSecondWindow, fl));

	AddStringAttribute(*this, "netSamplesPerPixel", "Average number of samples per pixel by slave nodes", boost::bind(&FL::getNetworkAverageSamplesPerPixel, fl));
	AddStringAttribute(*this, "netSamplesPerSecond", "Average number of samples per second by slave nodes", boost::bind(&FL::getNetworkAverageSamplesPerSecond, fl));
	AddStringAttribute(*this, "netSamplesPerSecondWindow", "Average number of samples per second by slave nodes in current time window", boost::bind(&FL::getNetworkAverageSamplesPerSecondWindow, fl));

	AddStringAttribute(*this, "totalSamplesPerPixel", "Average number of samples per pixel", boost::bind(&FL::getTotalAverageSamplesPerPixel, fl));
	AddStringAttribute(*this, "totalSamplesPerSecond", "Average number of samples per second", boost::bind(&FL::getTotalAverageSamplesPerSecond, fl));
	AddStringAttribute(*this, "totalSamplesPerSecondWindow", "Average number of samples per second in current time window", boost::bind(&FL::getTotalAverageSamplesPerSecondWindow, fl));
}

std::string SRStatistics::FormattedShort::getRecommendedStringTemplate()
{
	std::string stringTemplate = RendererStatistics::FormattedShort::getRecommendedStringTemplate();
	stringTemplate += ": %samplesPerPixel%";
	if (rs->getHaltSpp() != std::numeric_limits<double>::infinity())
		stringTemplate += " (%percentHaltSppComplete%)";
	stringTemplate += " %samplesPerSecondWindow% %contributionsPerSecondWindow% %efficiency%";

	if (rs->getNetworkSampleCount() != 0.0)
	{
		if (rs->getSlaveNodeCount() != 0)
			stringTemplate += " | Net: ~%netSamplesPerPixel% ~%netSamplesPerSecondWindow%";
		else
			stringTemplate += " | Net: %netSamplesPerPixel% %netSamplesPerSecondWindow%";
	}

	if (rs->getNetworkSampleCount() != 0.0 && rs->getSlaveNodeCount())
		stringTemplate += " | Tot: ~%totalSamplesPerPixel% ~%totalSamplesPerSecondWindow%";
	else if (rs->getResumedSampleCount() != 0.0)
		stringTemplate += " | Tot: %totalSamplesPerPixel% %totalSamplesPerSecondWindow%";

	return stringTemplate;
}

std::string SRStatistics::FormattedShort::getPercentHaltSppComplete() {
	return boost::str(boost::format("%1$0.0f%% S/p Cmplt") % rs->getPercentHaltSppComplete());
}
