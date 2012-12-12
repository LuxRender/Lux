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
#include <numeric>
#include <string>
#include <vector>

#include <boost/bind.hpp>
#include <boost/format.hpp>
#include <boost/thread/mutex.hpp>

#include <renderers/samplerrenderer.h>
#include <renderers/samplertbbrenderer.h>

namespace lux
{

template<typename Renderer>
SRStatistics<Renderer>::SRStatistics(Renderer* renderer)
	: renderer(renderer)
{
	resetDerived();

	formattedLong = new SRStatistics<Renderer>::FormattedLong(this);
	formattedShort = new SRStatistics<Renderer>::FormattedShort(this);

	AddDoubleAttribute(*this, "haltSamplesPerPixel", "Average number of samples per pixel to complete before halting", &SRStatistics<Renderer>::getHaltSpp);
	AddDoubleAttribute(*this, "remainingSamplesPerPixel", "Average number of samples per pixel remaining", &SRStatistics<Renderer>::getRemainingSamplesPerPixel);
	AddDoubleAttribute(*this, "percentHaltSppComplete", "Percent of halt S/p completed", &SRStatistics<Renderer>::getPercentHaltSppComplete);
	AddDoubleAttribute(*this, "resumedSamplesPerPixel", "Average number of samples per pixel loaded from FLM", &SRStatistics<Renderer>::getResumedAverageSamplesPerPixel);

	AddDoubleAttribute(*this, "pathEfficiency", "Efficiency of generated paths", &SRStatistics<Renderer>::getPathEfficiency);
	AddDoubleAttribute(*this, "pathEfficiencyWindow", "Efficiency of generated paths", &SRStatistics<Renderer>::getPathEfficiencyWindow);

	AddDoubleAttribute(*this, "samplesPerPixel", "Average number of samples per pixel by local node", &SRStatistics<Renderer>::getAverageSamplesPerPixel);
	AddDoubleAttribute(*this, "samplesPerSecond", "Average number of samples per second by local node", &SRStatistics<Renderer>::getAverageSamplesPerSecond);
	AddDoubleAttribute(*this, "samplesPerSecondWindow", "Average number of samples per second by local node in current time window", &SRStatistics<Renderer>::getAverageSamplesPerSecondWindow);
	AddDoubleAttribute(*this, "contributionsPerSecond", "Average number of contributions per second by local node", &SRStatistics<Renderer>::getAverageContributionsPerSecond);
	AddDoubleAttribute(*this, "contributionsPerSecondWindow", "Average number of contributions per second by local node in current time window", &SRStatistics<Renderer>::getAverageContributionsPerSecondWindow);

	AddDoubleAttribute(*this, "netSamplesPerPixel", "Average number of samples per pixel by slave nodes", &SRStatistics<Renderer>::getNetworkAverageSamplesPerPixel);
	AddDoubleAttribute(*this, "netSamplesPerSecond", "Average number of samples per second by slave nodes", &SRStatistics<Renderer>::getNetworkAverageSamplesPerSecond);

	AddDoubleAttribute(*this, "totalSamplesPerPixel", "Average number of samples per pixel", &SRStatistics<Renderer>::getTotalAverageSamplesPerPixel);
	AddDoubleAttribute(*this, "totalSamplesPerSecond", "Average number of samples per second", &SRStatistics<Renderer>::getTotalAverageSamplesPerSecond);
	AddDoubleAttribute(*this, "totalSamplesPerSecondWindow", "Average number of samples per second in current time window", &SRStatistics<Renderer>::getTotalAverageSamplesPerSecondWindow);
}

template<typename Renderer>
SRStatistics<Renderer>::~SRStatistics()
{
	delete formattedLong;
	delete formattedShort;
}

template<typename Renderer>
void SRStatistics<Renderer>::resetDerived() {
	windowSampleCount = 0.0;
	exponentialMovingAverage = 0.0;
	windowEffSampleCount = 0.0;
	windowEffBlackSampleCount = 0.0;
	windowPEffSampleCount = 0.0;
	windowPEffBlackSampleCount = 0.0;
}

template<typename Renderer>
void SRStatistics<Renderer>::updateStatisticsWindowDerived()
{
	// Get local sample count
	double sampleCount = getSampleCount();
	double elapsedTime = windowCurrentTime - windowStartTime;

	if (elapsedTime != 0.0)
	{
		double sps = (sampleCount - windowSampleCount) / elapsedTime;

		if (exponentialMovingAverage == 0.0)
			exponentialMovingAverage = sps;
		exponentialMovingAverage += min(1.0, elapsedTime / statisticsWindowSize) * (sps - exponentialMovingAverage);
	}
	windowSampleCount = sampleCount;
}

template<typename Renderer>
double SRStatistics<Renderer>::getRemainingTime() {
	double remainingTime = RendererStatistics::getRemainingTime();
	double remainingSamples = std::max(0.0, getHaltSpp() - getTotalAverageSamplesPerPixel()) * getPixelCount();

	return std::min(remainingTime, remainingSamples / getTotalAverageSamplesPerSecondWindow());
}

// Returns haltSamplesPerPixel if set, otherwise infinity
template<typename Renderer>
double SRStatistics<Renderer>::getHaltSpp() {
	double haltSpp = 0.0;

	Queryable* filmRegistry = Context::GetActive()->registry["film"];
	if (filmRegistry)
		haltSpp = (*filmRegistry)["haltSamplesPerPixel"].IntValue();

	return haltSpp > 0.0 ? haltSpp : std::numeric_limits<double>::infinity();
}

// Returns percent of haltSamplesPerPixel completed, zero if haltSamplesPerPixel is not set
template<typename Renderer>
double SRStatistics<Renderer>::getPercentHaltSppComplete() {
	return (getTotalAverageSamplesPerPixel() / getHaltSpp()) * 100.0;
}

template<typename Renderer>
double SRStatistics<Renderer>::getEfficiency() {
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

template<typename Renderer>
double SRStatistics<Renderer>::getEfficiencyWindow() {
	double sampleCount = 0.0 - windowEffSampleCount;
	double blackSampleCount = 0.0 - windowEffBlackSampleCount;

	// Get the current counts from the renderthreads
	// Cannot just use getSampleCount() because the blackSampleCount is necessary
	boost::mutex::scoped_lock lock(renderer->renderThreadsMutex);
	for (u_int i = 0; i < renderer->renderThreads.size(); ++i) {
		fast_mutex::scoped_lock lockStats(renderer->renderThreads[i]->statLock);
		sampleCount += renderer->renderThreads[i]->samples;
		blackSampleCount += renderer->renderThreads[i]->blackSamples;
	}

	windowPEffSampleCount += sampleCount;
	windowPEffBlackSampleCount += blackSampleCount;
	
	return sampleCount ? (100.0 * blackSampleCount) / sampleCount : 0.0;
}

template<typename Renderer>
double SRStatistics<Renderer>::getPathEfficiency() {
	double sampleCount = 0.0;
	double blackSamplePathCount = 0.0;

	// Get the current counts from the renderthreads
	// Cannot just use getSampleCount() because the blackSamplePathCount is necessary
	boost::mutex::scoped_lock lock(renderer->renderThreadsMutex);
	for (u_int i = 0; i < renderer->renderThreads.size(); ++i) {
		fast_mutex::scoped_lock lockStats(renderer->renderThreads[i]->statLock);
		sampleCount += renderer->renderThreads[i]->samples;
		blackSamplePathCount += renderer->renderThreads[i]->blackSamplePaths;
	}

	return sampleCount ? (100.0 * blackSamplePathCount) / sampleCount : 0.0;
}

template<typename Renderer>
double SRStatistics<Renderer>::getPathEfficiencyWindow() {
	double sampleCount = 0.0 - windowPEffSampleCount;
	double blackSamplePathCount = 0.0 - windowPEffBlackSampleCount;

	// Get the current counts from the renderthreads
	// Cannot just use getSampleCount() because the blackSamplePathCount is necessary
	boost::mutex::scoped_lock lock(renderer->renderThreadsMutex);
	for (u_int i = 0; i < renderer->renderThreads.size(); ++i) {
		fast_mutex::scoped_lock lockStats(renderer->renderThreads[i]->statLock);
		sampleCount += renderer->renderThreads[i]->samples;
		blackSamplePathCount += renderer->renderThreads[i]->blackSamplePaths;
	}

	windowPEffSampleCount += sampleCount;
	windowPEffBlackSampleCount += blackSamplePathCount;
	
	return sampleCount ? (100.0 * blackSamplePathCount) / sampleCount : 0.0;
}

// template specialisation for SamplerTBBRenderer

template<>
double SRStatistics<SamplerTBBRenderer>::getEfficiency() {
	double sampleCount = 0.0;
	double blackSampleCount = 0.0;

	// Get the current counts from the renderthreads
	// Cannot just use getSampleCount() because the blackSampleCount is necessary
	// boost::mutex::scoped_lock lock(renderer->localStoragePoolMutex); // TBB TODO: is this needed ?
	for (SamplerTBBRenderer::LocalStoragePool::iterator i = renderer->localStoragePool->begin(); i < renderer->localStoragePool->end(); ++i) {
		// fast_mutex::scoped_lock lockStats(i->statLock); // TBB TODO: is this needed ?
		sampleCount += i->samples;
		blackSampleCount += i->blackSamples;
	}

	return sampleCount ? (100.0 * blackSampleCount) / sampleCount : 0.0;
}

template<>
double SRStatistics<SamplerTBBRenderer>::getEfficiencyWindow() {
	double sampleCount = 0.0 - windowEffSampleCount;
	double blackSampleCount = 0.0 - windowEffBlackSampleCount;

	// Get the current counts from the renderthreads
	// Cannot just use getSampleCount() because the blackSampleCount is necessary
	// boost::mutex::scoped_lock lock(renderer->localStoragePoolMutex); // TBB TODO: is this needed ?
	for (SamplerTBBRenderer::LocalStoragePool::iterator i = renderer->localStoragePool->begin(); i < renderer->localStoragePool->end(); ++i) {
		// fast_mutex::scoped_lock lockStats(i->statLock); // TBB TODO: is this needed ?
		sampleCount += i->samples;
		blackSampleCount += i->blackSamples;
	}

	windowPEffSampleCount += sampleCount;
	windowPEffBlackSampleCount += blackSampleCount;

	return sampleCount ? (100.0 * blackSampleCount) / sampleCount : 0.0;
}

template<>
double SRStatistics<SamplerTBBRenderer>::getPathEfficiency() {
	double sampleCount = 0.0;
	double blackSamplePathCount = 0.0;

	// Get the current counts from the renderthreads
	// Cannot just use getSampleCount() because the blackSamplePathCount is necessary
	// boost::mutex::scoped_lock lock(renderer->localStoragePoolMutex); // TBB TODO: is this needed ?
	for (SamplerTBBRenderer::LocalStoragePool::iterator i = renderer->localStoragePool->begin(); i < renderer->localStoragePool->end(); ++i) {
		// fast_mutex::scoped_lock lockStats(i->statLock); // TBB TODO: is this needed ?
		sampleCount += i->samples;
		blackSamplePathCount += i->blackSamplePaths;
	}

	return sampleCount ? (100.0 * blackSamplePathCount) / sampleCount : 0.0;
}

template<>
double SRStatistics<SamplerTBBRenderer>::getPathEfficiencyWindow() {
	double sampleCount = 0.0 - windowPEffSampleCount;
	double blackSamplePathCount = 0.0 - windowPEffBlackSampleCount;

	// Get the current counts from the renderthreads
	// Cannot just use getSampleCount() because the blackSamplePathCount is necessary
	// boost::mutex::scoped_lock lock(renderer->localStoragePoolMutex); // TBB TODO: is this needed ?
	for (SamplerTBBRenderer::LocalStoragePool::iterator i = renderer->localStoragePool->begin(); i < renderer->localStoragePool->end(); ++i) {
		// fast_mutex::scoped_lock lockStats(i->statLock); // TBB TODO: is this needed ?
		sampleCount += i->samples;
		blackSamplePathCount += i->blackSamplePaths;
	}

	windowPEffSampleCount += sampleCount;
	windowPEffBlackSampleCount += blackSamplePathCount;

	return sampleCount ? (100.0 * blackSamplePathCount) / sampleCount : 0.0;
}


template<typename Renderer>
double SRStatistics<Renderer>::getAverageSamplesPerSecond() {
	double et = getElapsedTime();
	return (et == 0.0) ? 0.0 : getSampleCount() / et;
}

template<typename Renderer>
double SRStatistics<Renderer>::getAverageSamplesPerSecondWindow() {
	boost::mutex::scoped_lock window_mutex(windowMutex);
	return exponentialMovingAverage;
}

template<typename Renderer>
double SRStatistics<Renderer>::getNetworkAverageSamplesPerSecond() {
	double nsps = 0.0;

	size_t nServers = getSlaveNodeCount();
	if (nServers > 0)
	{
		std::vector<RenderingServerInfo> nodes(nServers);
		nServers = luxGetRenderingServersStatus (&nodes[0], nServers);

		for (size_t n = 0; n < nServers; n++)
			nsps += nodes[n].calculatedSamplesPerSecond;
	}

	return nsps;
}

template<typename Renderer>
u_int SRStatistics<Renderer>::getPixelCount() {
	int xstart, xend, ystart, yend;

	renderer->scene->camera->film->GetSampleExtent(&xstart, &xend, &ystart, &yend);

	return ((xend - xstart) * (yend - ystart));
}

template<typename Renderer>
double SRStatistics<Renderer>::getResumedSampleCount() {
	double resumedSampleCount = 0.0;

	Queryable* filmRegistry = Context::GetActive()->registry["film"];
	if (filmRegistry)
		resumedSampleCount = (*filmRegistry)["numberOfResumedSamples"].DoubleValue();

	return resumedSampleCount;
}

template<typename Renderer>
double SRStatistics<Renderer>::getSampleCount() {
	double sampleCount = 0.0;

	Queryable* filmRegistry = Context::GetActive()->registry["film"];
	if (filmRegistry)
		sampleCount = (*filmRegistry)["numberOfLocalSamples"].DoubleValue();

	return sampleCount;
}

template<typename Renderer>
double SRStatistics<Renderer>::getNetworkSampleCount(bool estimate) {
	double networkSampleCount = 0.0;

	Queryable* filmRegistry = Context::GetActive()->registry["film"];
	if (filmRegistry)
		networkSampleCount = (*filmRegistry)["numberOfSamplesFromNetwork"].DoubleValue();

	// Add estimated network sample count
	size_t nServers = getSlaveNodeCount();
	if (estimate && nServers > 0) {
		std::vector<RenderingServerInfo> nodes(nServers);
		nServers = luxGetRenderingServersStatus (&nodes[0], nServers);

		for (size_t n = 0; n < nServers; n++)
			networkSampleCount += nodes[n].calculatedSamplesPerSecond * nodes[n].secsSinceLastSamples;
	}

	return networkSampleCount;
}

template<typename Renderer>
SRStatistics<Renderer>::FormattedLong::FormattedLong(SRStatistics<Renderer>* rs)
	: RendererStatistics::FormattedLong(rs), rs(rs)
{
	typedef SRStatistics<Renderer>::FormattedLong FL;

	AddStringAttribute(*this, "haltSamplesPerPixel", "Average number of samples per pixel to complete before halting", &FL::getHaltSpp);
	AddStringAttribute(*this, "remainingSamplesPerPixel", "Average number of samples per pixel remaining", &FL::getRemainingSamplesPerPixel);
	AddStringAttribute(*this, "percentHaltSppComplete", "Percent of halt S/p completed", &FL::getPercentHaltSppComplete);
	AddStringAttribute(*this, "resumedSamplesPerPixel", "Average number of samples per pixel loaded from FLM", &FL::getResumedAverageSamplesPerPixel);

	AddStringAttribute(*this, "pathEfficiency", "Efficiency of generated paths", &FL::getPathEfficiency);
	AddStringAttribute(*this, "pathEfficiencyWindow", "Efficiency of generated paths", &FL::getPathEfficiencyWindow);

	AddStringAttribute(*this, "samplesPerPixel", "Average number of samples per pixel by local node", &FL::getAverageSamplesPerPixel);
	AddStringAttribute(*this, "samplesPerSecond", "Average number of samples per second by local node", &FL::getAverageSamplesPerSecond);
	AddStringAttribute(*this, "samplesPerSecondWindow", "Average number of samples per second by local node in current time window", &FL::getAverageSamplesPerSecondWindow);
	AddStringAttribute(*this, "contributionsPerSecond", "Average number of contributions per second by local node", &FL::getAverageContributionsPerSecond);
	AddStringAttribute(*this, "contributionsPerSecondWindow", "Average number of contributions per second by local node in current time window", &FL::getAverageContributionsPerSecondWindow);

	AddStringAttribute(*this, "netSamplesPerPixel", "Average number of samples per pixel by slave nodes", &FL::getNetworkAverageSamplesPerPixel);
	AddStringAttribute(*this, "netSamplesPerSecond", "Average number of samples per second by slave nodes", &FL::getNetworkAverageSamplesPerSecond);

	AddStringAttribute(*this, "totalSamplesPerPixel", "Average number of samples per pixel", &FL::getTotalAverageSamplesPerPixel);
	AddStringAttribute(*this, "totalSamplesPerSecond", "Average number of samples per second", &FL::getTotalAverageSamplesPerSecond);
	AddStringAttribute(*this, "totalSamplesPerSecondWindow", "Average number of samples per second in current time window", &FL::getTotalAverageSamplesPerSecondWindow);
}

template<typename Renderer>
std::string SRStatistics<Renderer>::FormattedLong::getRecommendedStringTemplate()
{
	std::string stringTemplate = RendererStatistics::FormattedLong::getRecommendedStringTemplate();
	stringTemplate += ": %samplesPerPixel%";
	if (rs->getHaltSpp() != std::numeric_limits<double>::infinity())
		stringTemplate += " (%percentHaltSppComplete%)";
	stringTemplate += " %samplesPerSecondWindow% %contributionsPerSecondWindow% %efficiency%";

	if (rs->getNetworkSampleCount() != 0.0)
	{
		if (rs->getSlaveNodeCount() != 0)
			stringTemplate += " | Net: ~%netSamplesPerPixel% ~%netSamplesPerSecond%";
		else
			stringTemplate += " | Net: %netSamplesPerPixel% %netSamplesPerSecond%";
	}

	if (rs->getNetworkSampleCount() != 0.0 && rs->getSlaveNodeCount())
		stringTemplate += " | Tot: ~%totalSamplesPerPixel% ~%totalSamplesPerSecondWindow%";
	else if (rs->getResumedSampleCount() != 0.0)
		stringTemplate += " | Tot: %totalSamplesPerPixel% %totalSamplesPerSecondWindow%";

	return stringTemplate;
}

template<typename Renderer>
std::string SRStatistics<Renderer>::FormattedLong::getHaltSpp() {
	return boost::str(boost::format("%1% S/p") % rs->getHaltSpp());
}

template<typename Renderer>
std::string SRStatistics<Renderer>::FormattedLong::getRemainingSamplesPerPixel() {
	double rspp = rs->getRemainingSamplesPerPixel();
	return boost::str(boost::format("%1$0.2f %2%S/p") % MagnitudeReduce(rspp) % MagnitudePrefix(rspp));
}

template<typename Renderer>
std::string SRStatistics<Renderer>::FormattedLong::getPercentHaltSppComplete() {
	return boost::str(boost::format("%1$0.0f%% S/p") % rs->getPercentHaltSppComplete());
}

template<typename Renderer>
std::string SRStatistics<Renderer>::FormattedLong::getResumedAverageSamplesPerPixel() {
	double spp = rs->getResumedAverageSamplesPerPixel();
	return boost::str(boost::format("%1$0.2f %2%S/p") % MagnitudeReduce(spp) % MagnitudePrefix(spp));
}

template<typename Renderer>
std::string SRStatistics<Renderer>::FormattedLong::getPathEfficiency() {
	return boost::str(boost::format("%1$0.0f%% Path Efficiency") % rs->getPathEfficiency());
}

template<typename Renderer>
std::string SRStatistics<Renderer>::FormattedLong::getPathEfficiencyWindow() {
	return boost::str(boost::format("%1$0.0f%% Path Efficiency") % rs->getPathEfficiencyWindow());
}

template<typename Renderer>
std::string SRStatistics<Renderer>::FormattedLong::getAverageSamplesPerPixel() {
	double spp = rs->getAverageSamplesPerPixel();
	return boost::str(boost::format("%1$0.2f %2%S/p") % MagnitudeReduce(spp) % MagnitudePrefix(spp));
}

template<typename Renderer>
std::string SRStatistics<Renderer>::FormattedLong::getAverageSamplesPerSecond() {
	double sps = rs->getAverageSamplesPerSecond();
	return boost::str(boost::format("%1$0.2f %2%S/s") % MagnitudeReduce(sps) % MagnitudePrefix(sps));
}

template<typename Renderer>
std::string SRStatistics<Renderer>::FormattedLong::getAverageSamplesPerSecondWindow() {
	double spsw = rs->getAverageSamplesPerSecondWindow();
	return boost::str(boost::format("%1$0.2f %2%S/s") % MagnitudeReduce(spsw) % MagnitudePrefix(spsw));
}

template<typename Renderer>
std::string SRStatistics<Renderer>::FormattedLong::getAverageContributionsPerSecond() {
	double cps = rs->getAverageContributionsPerSecond();
	return boost::str(boost::format("%1$0.2f %2%C/s") % MagnitudeReduce(cps) % MagnitudePrefix(cps));
}

template<typename Renderer>
std::string SRStatistics<Renderer>::FormattedLong::getAverageContributionsPerSecondWindow() {
	double cpsw = rs->getAverageContributionsPerSecondWindow();
	return boost::str(boost::format("%1$0.2f %2%C/s") % MagnitudeReduce(cpsw) % MagnitudePrefix(cpsw));
}

template<typename Renderer>
std::string SRStatistics<Renderer>::FormattedLong::getNetworkAverageSamplesPerPixel() {
	double spp = rs->getNetworkAverageSamplesPerPixel();
	return boost::str(boost::format("%1$0.2f %2%S/p") % MagnitudeReduce(spp) % MagnitudePrefix(spp));
}

template<typename Renderer>
std::string SRStatistics<Renderer>::FormattedLong::getNetworkAverageSamplesPerSecond() {
	double sps = rs->getNetworkAverageSamplesPerSecond();
	return boost::str(boost::format("%1$0.2f %2%S/s") % MagnitudeReduce(sps) % MagnitudePrefix(sps));
}

template<typename Renderer>
std::string SRStatistics<Renderer>::FormattedLong::getTotalAverageSamplesPerPixel() {
	double spp = rs->getTotalAverageSamplesPerPixel();
	return boost::str(boost::format("%1$0.2f %2%S/p") % MagnitudeReduce(spp) % MagnitudePrefix(spp));
}

template<typename Renderer>
std::string SRStatistics<Renderer>::FormattedLong::getTotalAverageSamplesPerSecond() {
	double sps = rs->getTotalAverageSamplesPerSecond();
	return boost::str(boost::format("%1$0.2f %2%S/s") % MagnitudeReduce(sps) % MagnitudePrefix(sps));
}

template<typename Renderer>
std::string SRStatistics<Renderer>::FormattedLong::getTotalAverageSamplesPerSecondWindow() {
	double spsw = rs->getTotalAverageSamplesPerSecondWindow();
	return boost::str(boost::format("%1$0.2f %2%S/s") % MagnitudeReduce(spsw) % MagnitudePrefix(spsw));
}

template<typename Renderer>
SRStatistics<Renderer>::FormattedShort::FormattedShort(SRStatistics<Renderer>* rs)
	: RendererStatistics::FormattedShort(rs), rs(rs)
{
	FormattedLong* fl = static_cast<SRStatistics<Renderer>::FormattedLong*>(rs->formattedLong);

	typedef SRStatistics<Renderer>::FormattedLong FL;
	typedef SRStatistics<Renderer>::FormattedShort FS;

	AddStringAttribute(*this, "haltSamplesPerPixel", "Average number of samples per pixel to complete before halting", boost::bind(boost::mem_fn(&FL::getHaltSpp), fl));
	AddStringAttribute(*this, "remainingSamplesPerPixel", "Average number of samples per pixel remaining", boost::bind(boost::mem_fn(&FL::getRemainingSamplesPerPixel), fl));
	AddStringAttribute(*this, "percentHaltSppComplete", "Percent of halt S/p completed", boost::bind(boost::mem_fn(&FL::getPercentHaltSppComplete), fl));
	AddStringAttribute(*this, "resumedSamplesPerPixel", "Average number of samples per pixel loaded from FLM", boost::bind(boost::mem_fn(&FL::getResumedAverageSamplesPerPixel), fl));

	AddStringAttribute(*this, "pathEfficiency", "Efficiency of generated paths", &FS::getPathEfficiency);
	AddStringAttribute(*this, "pathEfficiencyWindow", "Efficiency of generated paths", &FS::getPathEfficiencyWindow);

	AddStringAttribute(*this, "samplesPerPixel", "Average number of samples per pixel by local node", boost::bind(boost::mem_fn(&FL::getAverageSamplesPerPixel), fl));
	AddStringAttribute(*this, "samplesPerSecond", "Average number of samples per second by local node", boost::bind(boost::mem_fn(&FL::getAverageSamplesPerSecond), fl));
	AddStringAttribute(*this, "samplesPerSecondWindow", "Average number of samples per second by local node in current time window", boost::bind(boost::mem_fn(&FL::getAverageSamplesPerSecondWindow), fl));
	AddStringAttribute(*this, "contributionsPerSecond", "Average number of contributions per second by local node", boost::bind(boost::mem_fn(&FL::getAverageContributionsPerSecond), fl));
	AddStringAttribute(*this, "contributionsPerSecondWindow", "Average number of contributions per second by local node in current time window", boost::bind(boost::mem_fn(&FL::getAverageContributionsPerSecondWindow), fl));

	AddStringAttribute(*this, "netSamplesPerPixel", "Average number of samples per pixel by slave nodes", boost::bind(boost::mem_fn(&FL::getNetworkAverageSamplesPerPixel), fl));
	AddStringAttribute(*this, "netSamplesPerSecond", "Average number of samples per second by slave nodes", boost::bind(boost::mem_fn(&FL::getNetworkAverageSamplesPerSecond), fl));

	AddStringAttribute(*this, "totalSamplesPerPixel", "Average number of samples per pixel", boost::bind(boost::mem_fn(&FL::getTotalAverageSamplesPerPixel), fl));
	AddStringAttribute(*this, "totalSamplesPerSecond", "Average number of samples per second", boost::bind(boost::mem_fn(&FL::getTotalAverageSamplesPerSecond), fl));
	AddStringAttribute(*this, "totalSamplesPerSecondWindow", "Average number of samples per second in current time window", boost::bind(boost::mem_fn(&FL::getTotalAverageSamplesPerSecondWindow), fl));
}

template<typename Renderer>
std::string SRStatistics<Renderer>::FormattedShort::getRecommendedStringTemplate()
{
	std::string stringTemplate = RendererStatistics::FormattedShort::getRecommendedStringTemplate();
	stringTemplate += ": %samplesPerPixel%";
	if (rs->getHaltSpp() != std::numeric_limits<double>::infinity())
		stringTemplate += " (%percentHaltSppComplete%)";
	stringTemplate += " %samplesPerSecondWindow% %contributionsPerSecondWindow% %efficiency%";

	if (rs->getNetworkSampleCount() != 0.0)
	{
		if (rs->getSlaveNodeCount() != 0)
			stringTemplate += " | Net: ~%netSamplesPerPixel% ~%netSamplesPerSecond%";
		else
			stringTemplate += " | Net: %netSamplesPerPixel% %netSamplesPerSecond%";
	}

	if (rs->getNetworkSampleCount() != 0.0 && rs->getSlaveNodeCount())
		stringTemplate += " | Tot: ~%totalSamplesPerPixel% ~%totalSamplesPerSecondWindow%";
	else if (rs->getResumedSampleCount() != 0.0)
		stringTemplate += " | Tot: %totalSamplesPerPixel% %totalSamplesPerSecondWindow%";

	return stringTemplate;
}

template<typename Renderer>
std::string SRStatistics<Renderer>::FormattedShort::getProgress() { 
	return static_cast<SRStatistics::FormattedLong*>(rs->formattedLong)->getTotalAverageSamplesPerPixel();
}

template<typename Renderer>
std::string SRStatistics<Renderer>::FormattedShort::getPathEfficiency() {
	return boost::str(boost::format("%1$0.0f%% PEff") % rs->getPathEfficiency());
}

template<typename Renderer>
std::string SRStatistics<Renderer>::FormattedShort::getPathEfficiencyWindow() {
	return boost::str(boost::format("%1$0.0f%% PEff") % rs->getPathEfficiencyWindow());
}

template<>
u_int SRStatistics<SamplerRenderer>::getThreadCount()
{
	return renderer->renderThreads.size();
}

template<>
u_int SRStatistics<SamplerTBBRenderer>::getThreadCount()
{
	return renderer->numberOfThreads;
}

template class SRStatistics<SamplerRenderer>;
template class SRStatistics<SamplerTBBRenderer>;
}
