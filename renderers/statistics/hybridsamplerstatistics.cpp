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

#include "renderers/statistics/hybridsamplerstatistics.h"
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

using namespace lux;

#define MAX_DEVICE_COUNT 16

HSRStatistics::HSRStatistics(HybridSamplerRenderer* renderer)
	: renderer(renderer) {
	triangleCount = 0;

	resetDerived();

	formattedLong = new HSRStatistics::FormattedLong(this);
	formattedShort = new HSRStatistics::FormattedShort(this);

	AddDoubleAttribute(*this, "haltSamplesPerPixel", "Average number of samples per pixel to complete before halting", &HSRStatistics::getHaltSpp);
	AddDoubleAttribute(*this, "remainingSamplesPerPixel", "Average number of samples per pixel remaining", &HSRStatistics::getRemainingSamplesPerPixel);
	AddDoubleAttribute(*this, "percentHaltSppComplete", "Percent of halt S/p completed", &HSRStatistics::getPercentHaltSppComplete);
	AddDoubleAttribute(*this, "resumedSamplesPerPixel", "Average number of samples per pixel loaded from FLM", &HSRStatistics::getResumedAverageSamplesPerPixel);

	AddIntAttribute(*this, "gpuCount", "Number of GPUs in use", &HSRStatistics::getDeviceCount); // A deprecated alias (look for "deviceCount" instead)
	AddDoubleAttribute(*this, "gpuEfficiency", "Percent of time GPUs have rays available to trace", &HSRStatistics::getAverageGpuEfficiency);

	AddDoubleAttribute(*this, "pathEfficiency", "Efficiency of generated paths", &HSRStatistics::getPathEfficiency);
	AddDoubleAttribute(*this, "pathEfficiencyWindow", "Efficiency of generated paths", &HSRStatistics::getPathEfficiencyWindow);

	AddDoubleAttribute(*this, "samplesPerPixel", "Average number of samples per pixel by local node", &HSRStatistics::getAverageSamplesPerPixel);
	AddDoubleAttribute(*this, "samplesPerSecond", "Average number of samples per second by local node", &HSRStatistics::getAverageSamplesPerSecond);
	AddDoubleAttribute(*this, "samplesPerSecondWindow", "Average number of samples per second by local node in current time window", &HSRStatistics::getAverageSamplesPerSecondWindow);
	AddDoubleAttribute(*this, "contributionsPerSecond", "Average number of contributions per second by local node", &HSRStatistics::getAverageContributionsPerSecond);
	AddDoubleAttribute(*this, "contributionsPerSecondWindow", "Average number of contributions per second by local node in current time window", &HSRStatistics::getAverageContributionsPerSecondWindow);

	AddDoubleAttribute(*this, "netSamplesPerPixel", "Average number of samples per pixel by slave nodes", &HSRStatistics::getNetworkAverageSamplesPerPixel);
	AddDoubleAttribute(*this, "netSamplesPerSecond", "Average number of samples per second by slave nodes", &HSRStatistics::getNetworkAverageSamplesPerSecond);

	AddDoubleAttribute(*this, "totalSamplesPerPixel", "Average number of samples per pixel", &HSRStatistics::getTotalAverageSamplesPerPixel);
	AddDoubleAttribute(*this, "totalSamplesPerSecond", "Average number of samples per second", &HSRStatistics::getTotalAverageSamplesPerSecond);
	AddDoubleAttribute(*this, "totalSamplesPerSecondWindow", "Average number of samples per second in current time window", &HSRStatistics::getTotalAverageSamplesPerSecondWindow);

	AddIntAttribute(*this, "deviceCount", "Number of OpenCL devices in use", &HSRStatistics::getDeviceCount);
	AddStringAttribute(*this, "deviceNames", "A comma separated list of names of OpenCL devices in use", &HSRStatistics::getDeviceNames);

	AddDoubleAttribute(*this, "triangleCount", "total number of triangles in the scene", &HSRStatistics::getTriangleCount);

	// Register up to MAX_DEVICE_COUNT devices

	AddDoubleAttribute(*this, "device.0.raysecs", "OpenCL device 0 ray traced per second", &HSRStatistics::getDevice00RaySecs);
	AddDoubleAttribute(*this, "device.1.raysecs", "OpenCL device 1 ray traced per second", &HSRStatistics::getDevice01RaySecs);
	AddDoubleAttribute(*this, "device.2.raysecs", "OpenCL device 2 ray traced per second", &HSRStatistics::getDevice02RaySecs);
	AddDoubleAttribute(*this, "device.3.raysecs", "OpenCL device 3 ray traced per second", &HSRStatistics::getDevice03RaySecs);
	AddDoubleAttribute(*this, "device.4.raysecs", "OpenCL device 4 ray traced per second", &HSRStatistics::getDevice04RaySecs);
	AddDoubleAttribute(*this, "device.5.raysecs", "OpenCL device 5 ray traced per second", &HSRStatistics::getDevice05RaySecs);
	AddDoubleAttribute(*this, "device.6.raysecs", "OpenCL device 6 ray traced per second", &HSRStatistics::getDevice06RaySecs);
	AddDoubleAttribute(*this, "device.7.raysecs", "OpenCL device 7 ray traced per second", &HSRStatistics::getDevice07RaySecs);
	AddDoubleAttribute(*this, "device.8.raysecs", "OpenCL device 8 ray traced per second", &HSRStatistics::getDevice08RaySecs);
	AddDoubleAttribute(*this, "device.9.raysecs", "OpenCL device 9 ray traced per second", &HSRStatistics::getDevice09RaySecs);
	AddDoubleAttribute(*this, "device.10.raysecs", "OpenCL device 10 ray traced per second", &HSRStatistics::getDevice10RaySecs);
	AddDoubleAttribute(*this, "device.11.raysecs", "OpenCL device 11 ray traced per second", &HSRStatistics::getDevice11RaySecs);
	AddDoubleAttribute(*this, "device.12.raysecs", "OpenCL device 12 ray traced per second", &HSRStatistics::getDevice12RaySecs);
	AddDoubleAttribute(*this, "device.13.raysecs", "OpenCL device 13 ray traced per second", &HSRStatistics::getDevice13RaySecs);
	AddDoubleAttribute(*this, "device.14.raysecs", "OpenCL device 14 ray traced per second", &HSRStatistics::getDevice14RaySecs);
	AddDoubleAttribute(*this, "device.15.raysecs", "OpenCL device 15 ray traced per second", &HSRStatistics::getDevice15RaySecs);

	AddDoubleAttribute(*this, "device.0.maxmem", "OpenCL device 0 memory available", &HSRStatistics::getDevice00MaxMemory);
	AddDoubleAttribute(*this, "device.1.maxmem", "OpenCL device 1 memory available", &HSRStatistics::getDevice01MaxMemory);
	AddDoubleAttribute(*this, "device.2.maxmem", "OpenCL device 2 memory available", &HSRStatistics::getDevice02MaxMemory);
	AddDoubleAttribute(*this, "device.3.maxmem", "OpenCL device 3 memory available", &HSRStatistics::getDevice03MaxMemory);
	AddDoubleAttribute(*this, "device.4.maxmem", "OpenCL device 4 memory available", &HSRStatistics::getDevice04MaxMemory);
	AddDoubleAttribute(*this, "device.5.maxmem", "OpenCL device 5 memory available", &HSRStatistics::getDevice05MaxMemory);
	AddDoubleAttribute(*this, "device.6.maxmem", "OpenCL device 6 memory available", &HSRStatistics::getDevice06MaxMemory);
	AddDoubleAttribute(*this, "device.7.maxmem", "OpenCL device 7 memory available", &HSRStatistics::getDevice07MaxMemory);
	AddDoubleAttribute(*this, "device.8.maxmem", "OpenCL device 8 memory available", &HSRStatistics::getDevice08MaxMemory);
	AddDoubleAttribute(*this, "device.9.maxmem", "OpenCL device 9 memory available", &HSRStatistics::getDevice09MaxMemory);
	AddDoubleAttribute(*this, "device.10.maxmem", "OpenCL device 10 memory available", &HSRStatistics::getDevice10MaxMemory);
	AddDoubleAttribute(*this, "device.11.maxmem", "OpenCL device 11 memory available", &HSRStatistics::getDevice11MaxMemory);
	AddDoubleAttribute(*this, "device.12.maxmem", "OpenCL device 12 memory available", &HSRStatistics::getDevice12MaxMemory);
	AddDoubleAttribute(*this, "device.13.maxmem", "OpenCL device 13 memory available", &HSRStatistics::getDevice13MaxMemory);
	AddDoubleAttribute(*this, "device.14.maxmem", "OpenCL device 14 memory available", &HSRStatistics::getDevice14MaxMemory);
	AddDoubleAttribute(*this, "device.15.maxmem", "OpenCL device 15 memory available", &HSRStatistics::getDevice15MaxMemory);

	AddDoubleAttribute(*this, "device.0.memusage", "OpenCL device 0 memory used", &HSRStatistics::getDevice00MemoryUsed);
	AddDoubleAttribute(*this, "device.1.memusage", "OpenCL device 1 memory used", &HSRStatistics::getDevice01MemoryUsed);
	AddDoubleAttribute(*this, "device.2.memusage", "OpenCL device 2 memory used", &HSRStatistics::getDevice02MemoryUsed);
	AddDoubleAttribute(*this, "device.3.memusage", "OpenCL device 3 memory used", &HSRStatistics::getDevice03MemoryUsed);
	AddDoubleAttribute(*this, "device.4.memusage", "OpenCL device 4 memory used", &HSRStatistics::getDevice04MemoryUsed);
	AddDoubleAttribute(*this, "device.5.memusage", "OpenCL device 5 memory used", &HSRStatistics::getDevice05MemoryUsed);
	AddDoubleAttribute(*this, "device.6.memusage", "OpenCL device 6 memory used", &HSRStatistics::getDevice06MemoryUsed);
	AddDoubleAttribute(*this, "device.7.memusage", "OpenCL device 7 memory used", &HSRStatistics::getDevice07MemoryUsed);
	AddDoubleAttribute(*this, "device.8.memusage", "OpenCL device 8 memory used", &HSRStatistics::getDevice08MemoryUsed);
	AddDoubleAttribute(*this, "device.9.memusage", "OpenCL device 9 memory used", &HSRStatistics::getDevice09MemoryUsed);
	AddDoubleAttribute(*this, "device.10.memusage", "OpenCL device 10 memory used", &HSRStatistics::getDevice10MemoryUsed);
	AddDoubleAttribute(*this, "device.11.memusage", "OpenCL device 11 memory used", &HSRStatistics::getDevice11MemoryUsed);
	AddDoubleAttribute(*this, "device.12.memusage", "OpenCL device 12 memory used", &HSRStatistics::getDevice12MemoryUsed);
	AddDoubleAttribute(*this, "device.13.memusage", "OpenCL device 13 memory used", &HSRStatistics::getDevice13MemoryUsed);
	AddDoubleAttribute(*this, "device.14.memusage", "OpenCL device 14 memory used", &HSRStatistics::getDevice14MemoryUsed);
	AddDoubleAttribute(*this, "device.15.memusage", "OpenCL device 15 memory used", &HSRStatistics::getDevice15MemoryUsed);
}

HSRStatistics::~HSRStatistics()
{
	delete formattedLong;
	delete formattedShort;
}

void HSRStatistics::resetDerived() {
	windowSampleCount = 0.0;
	exponentialMovingAverage = 0.0;
	windowEffSampleCount = 0.0;
	windowEffBlackSampleCount = 0.0;
	windowPEffSampleCount = 0.0;
	windowPEffBlackSampleCount = 0.0;
}

void HSRStatistics::updateStatisticsWindowDerived()
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

double HSRStatistics::getRemainingTime() {
	double remainingTime = RendererStatistics::getRemainingTime();
	double remainingSamples = std::max(0.0, getHaltSpp() - getTotalAverageSamplesPerPixel()) * getPixelCount();

	return std::min(remainingTime, remainingSamples / getTotalAverageSamplesPerSecondWindow());
}

// Returns haltSamplesPerPixel if set, otherwise infinity
double HSRStatistics::getHaltSpp() {
	double haltSpp = 0.0;

	Queryable* filmRegistry = Context::GetActive()->registry["film"];
	if (filmRegistry)
		haltSpp = (*filmRegistry)["haltSamplesPerPixel"].IntValue();

	return haltSpp > 0.0 ? haltSpp : std::numeric_limits<double>::infinity();
}

// Returns percent of haltSamplesPerPixel completed, zero if haltSamplesPerPixel is not set
double HSRStatistics::getPercentHaltSppComplete() {
	return (getTotalAverageSamplesPerPixel() / getHaltSpp()) * 100.0;
}

double HSRStatistics::getEfficiency() {
	double sampleCount = 0.0;
	double blackSampleCount = 0.0;

	// Get the current counts from the renderthreads
	// Cannot just use getSampleCount() because the blackSampleCount is necessary
	boost::mutex::scoped_lock lock(renderer->classWideMutex);
	for (u_int i = 0; i < renderer->renderThreads.size(); ++i) {
		fast_mutex::scoped_lock lockStats(renderer->renderThreads[i]->statLock);
		sampleCount += renderer->renderThreads[i]->samples;
		blackSampleCount += renderer->renderThreads[i]->blackSamples;
	}

	return sampleCount ? (100.0 * blackSampleCount) / sampleCount : 0.0;
}

double HSRStatistics::getEfficiencyWindow() {
	double sampleCount = 0.0 - windowEffSampleCount;
	double blackSampleCount = 0.0 - windowEffBlackSampleCount;

	// Get the current counts from the renderthreads
	// Cannot just use getSampleCount() because the blackSampleCount is necessary
	boost::mutex::scoped_lock lock(renderer->classWideMutex);
	for (u_int i = 0; i < renderer->renderThreads.size(); ++i) {
		fast_mutex::scoped_lock lockStats(renderer->renderThreads[i]->statLock);
		sampleCount += renderer->renderThreads[i]->samples;
		blackSampleCount += renderer->renderThreads[i]->blackSamples;
	}

	windowPEffSampleCount += sampleCount;
	windowPEffBlackSampleCount += blackSampleCount;
	
	return sampleCount ? (100.0 * blackSampleCount) / sampleCount : 0.0;
}

double HSRStatistics::getPathEfficiency() {
	double sampleCount = 0.0;
	double blackSamplePathCount = 0.0;

	// Get the current counts from the renderthreads
	// Cannot just use getSampleCount() because the blackSamplePathCount is necessary
	boost::mutex::scoped_lock lock(renderer->classWideMutex);
	for (u_int i = 0; i < renderer->renderThreads.size(); ++i) {
		fast_mutex::scoped_lock lockStats(renderer->renderThreads[i]->statLock);
		sampleCount += renderer->renderThreads[i]->samples;
		blackSamplePathCount += renderer->renderThreads[i]->blackSamplePaths;
	}

	return sampleCount ? (100.0 * blackSamplePathCount) / sampleCount : 0.0;
}

double HSRStatistics::getPathEfficiencyWindow() {
	double sampleCount = 0.0 - windowPEffSampleCount;
	double blackSamplePathCount = 0.0 - windowPEffBlackSampleCount;

	// Get the current counts from the renderthreads
	// Cannot just use getSampleCount() because the blackSamplePathCount is necessary
	boost::mutex::scoped_lock lock(renderer->classWideMutex);
	for (u_int i = 0; i < renderer->renderThreads.size(); ++i) {
		fast_mutex::scoped_lock lockStats(renderer->renderThreads[i]->statLock);
		sampleCount += renderer->renderThreads[i]->samples;
		blackSamplePathCount += renderer->renderThreads[i]->blackSamplePaths;
	}

	windowPEffSampleCount += sampleCount;
	windowPEffBlackSampleCount += blackSamplePathCount;
	
	return sampleCount ? (100.0 * blackSamplePathCount) / sampleCount : 0.0;
}

// Returns percent of GPU efficiency, zero if no GPUs
double HSRStatistics::getAverageGpuEfficiency() {
	return 100.0 * renderer->intersectionDevice->GetLoad();
}

double HSRStatistics::getAverageSamplesPerSecond() {
	double et = getElapsedTime();
	return (et == 0.0) ? 0.0 : getSampleCount() / et;
}

double HSRStatistics::getAverageSamplesPerSecondWindow() {
	boost::mutex::scoped_lock window_mutex(windowMutex);
	return exponentialMovingAverage;
}

double HSRStatistics::getNetworkAverageSamplesPerSecond() {
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

u_int HSRStatistics::getPixelCount() {
	int xstart, xend, ystart, yend;

	renderer->scene->camera()->film->GetSampleExtent(&xstart, &xend, &ystart, &yend);

	return ((xend - xstart) * (yend - ystart));
}

double HSRStatistics::getResumedSampleCount() {
	double resumedSampleCount = 0.0;

	Queryable* filmRegistry = Context::GetActive()->registry["film"];
	if (filmRegistry)
		resumedSampleCount = (*filmRegistry)["numberOfResumedSamples"].DoubleValue();

	return resumedSampleCount;
}

double HSRStatistics::getSampleCount() {
	double sampleCount = 0.0;

	Queryable* filmRegistry = Context::GetActive()->registry["film"];
	if (filmRegistry)
		sampleCount = (*filmRegistry)["numberOfLocalSamples"].DoubleValue();

	return sampleCount;
}

double HSRStatistics::getNetworkSampleCount(bool estimate) {
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

HSRStatistics::FormattedLong::FormattedLong(HSRStatistics* rs)
	: RendererStatistics::FormattedLong(rs), rs(rs)
{
	typedef HSRStatistics::FormattedLong FL;

	AddStringAttribute(*this, "haltSamplesPerPixel", "Average number of samples per pixel to complete before halting", &FL::getHaltSpp);
	AddStringAttribute(*this, "remainingSamplesPerPixel", "Average number of samples per pixel remaining", &FL::getRemainingSamplesPerPixel);
	AddStringAttribute(*this, "percentHaltSppComplete", "Percent of halt S/p completed", &FL::getPercentHaltSppComplete);
	AddStringAttribute(*this, "resumedSamplesPerPixel", "Average number of samples per pixel loaded from FLM", &FL::getResumedAverageSamplesPerPixel);

	AddStringAttribute(*this, "gpuCount", "Number of GPUs in use", &FL::getGpuCount);
	AddStringAttribute(*this, "gpuEfficiency", "Percent of time GPUs have rays available to trace", &FL::getAverageGpuEfficiency);

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

std::string HSRStatistics::FormattedLong::getRecommendedStringTemplate()
{
	std::string stringTemplate = RendererStatistics::FormattedLong::getRecommendedStringTemplate();
	if (rs->getDeviceCount() != 0)
		stringTemplate += " %gpuCount%";
	stringTemplate += ": %samplesPerPixel%";
	if (rs->getHaltSpp() != std::numeric_limits<double>::infinity())
		stringTemplate += " (%percentHaltSppComplete%)";
	stringTemplate += " %samplesPerSecondWindow% %contributionsPerSecondWindow% %efficiency%";
	if (rs->getDeviceCount() != 0)
		stringTemplate += " %gpuEfficiency%";

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

std::string HSRStatistics::FormattedLong::getHaltSpp() {
	return boost::str(boost::format("%1% S/p") % rs->getHaltSpp());
}

std::string HSRStatistics::FormattedLong::getRemainingSamplesPerPixel() {
	double rspp = rs->getRemainingSamplesPerPixel();
	return boost::str(boost::format("%1$0.2f %2%S/p") % MagnitudeReduce(rspp) % MagnitudePrefix(rspp));
}

std::string HSRStatistics::FormattedLong::getPercentHaltSppComplete() {
	return boost::str(boost::format("%1$0.0f%% S/p") % rs->getPercentHaltSppComplete());
}

std::string HSRStatistics::FormattedLong::getResumedAverageSamplesPerPixel() {
	double spp = rs->getResumedAverageSamplesPerPixel();
	return boost::str(boost::format("%1$0.2f %2%S/p") % MagnitudeReduce(spp) % MagnitudePrefix(spp));
}

std::string HSRStatistics::FormattedLong::getGpuCount() {
	u_int gc = rs->getDeviceCount();
	return boost::str(boost::format("%1% %2%") % gc % Pluralize("GPU", gc));
}

std::string HSRStatistics::FormattedLong::getAverageGpuEfficiency() {
	return boost::str(boost::format("%1$0.0f%% GPU Efficiency") % rs->getAverageGpuEfficiency());
}

std::string HSRStatistics::FormattedLong::getPathEfficiency() {
	return boost::str(boost::format("%1$0.0f%% Path Efficiency") % rs->getPathEfficiency());
}

std::string HSRStatistics::FormattedLong::getPathEfficiencyWindow() {
	return boost::str(boost::format("%1$0.0f%% Path Efficiency") % rs->getPathEfficiencyWindow());
}

std::string HSRStatistics::FormattedLong::getAverageSamplesPerPixel() {
	double spp = rs->getAverageSamplesPerPixel();
	return boost::str(boost::format("%1$0.2f %2%S/p") % MagnitudeReduce(spp) % MagnitudePrefix(spp));
}

std::string HSRStatistics::FormattedLong::getAverageSamplesPerSecond() {
	double sps = rs->getAverageSamplesPerSecond();
	return boost::str(boost::format("%1$0.2f %2%S/s") % MagnitudeReduce(sps) % MagnitudePrefix(sps));
}

std::string HSRStatistics::FormattedLong::getAverageSamplesPerSecondWindow() {
	double spsw = rs->getAverageSamplesPerSecondWindow();
	return boost::str(boost::format("%1$0.2f %2%S/s") % MagnitudeReduce(spsw) % MagnitudePrefix(spsw));
}

std::string HSRStatistics::FormattedLong::getAverageContributionsPerSecond() {
	double cps = rs->getAverageContributionsPerSecond();
	return boost::str(boost::format("%1$0.2f %2%C/s") % MagnitudeReduce(cps) % MagnitudePrefix(cps));
}

std::string HSRStatistics::FormattedLong::getAverageContributionsPerSecondWindow() {
	double cpsw = rs->getAverageContributionsPerSecondWindow();
	return boost::str(boost::format("%1$0.2f %2%C/s") % MagnitudeReduce(cpsw) % MagnitudePrefix(cpsw));
}

std::string HSRStatistics::FormattedLong::getNetworkAverageSamplesPerPixel() {
	double spp = rs->getNetworkAverageSamplesPerPixel();
	return boost::str(boost::format("%1$0.2f %2%S/p") % MagnitudeReduce(spp) % MagnitudePrefix(spp));
}

std::string HSRStatistics::FormattedLong::getNetworkAverageSamplesPerSecond() {
	double sps = rs->getNetworkAverageSamplesPerSecond();
	return boost::str(boost::format("%1$0.2f %2%S/s") % MagnitudeReduce(sps) % MagnitudePrefix(sps));
}

std::string HSRStatistics::FormattedLong::getTotalAverageSamplesPerPixel() {
	double spp = rs->getTotalAverageSamplesPerPixel();
	return boost::str(boost::format("%1$0.2f %2%S/p") % MagnitudeReduce(spp) % MagnitudePrefix(spp));
}

std::string HSRStatistics::FormattedLong::getTotalAverageSamplesPerSecond() {
	double sps = rs->getTotalAverageSamplesPerSecond();
	return boost::str(boost::format("%1$0.2f %2%S/s") % MagnitudeReduce(sps) % MagnitudePrefix(sps));
}

std::string HSRStatistics::FormattedLong::getTotalAverageSamplesPerSecondWindow() {
	double spsw = rs->getTotalAverageSamplesPerSecondWindow();
	return boost::str(boost::format("%1$0.2f %2%S/s") % MagnitudeReduce(spsw) % MagnitudePrefix(spsw));
}

HSRStatistics::FormattedShort::FormattedShort(HSRStatistics* rs)
	: RendererStatistics::FormattedShort(rs), rs(rs)
{
	FormattedLong* fl = static_cast<HSRStatistics::FormattedLong*>(rs->formattedLong);

	typedef HSRStatistics::FormattedLong FL;
	typedef HSRStatistics::FormattedShort FS;

	AddStringAttribute(*this, "haltSamplesPerPixel", "Average number of samples per pixel to complete before halting", boost::bind(boost::mem_fn(&FL::getHaltSpp), fl));
	AddStringAttribute(*this, "remainingSamplesPerPixel", "Average number of samples per pixel remaining", boost::bind(boost::mem_fn(&FL::getRemainingSamplesPerPixel), fl));
	AddStringAttribute(*this, "percentHaltSppComplete", "Percent of halt S/p completed", boost::bind(boost::mem_fn(&FL::getPercentHaltSppComplete), fl));
	AddStringAttribute(*this, "resumedSamplesPerPixel", "Average number of samples per pixel loaded from FLM", boost::bind(boost::mem_fn(&FL::getResumedAverageSamplesPerPixel), fl));

	AddStringAttribute(*this, "gpuCount", "Number of GPUs in use", &FS::getGpuCount);
	AddStringAttribute(*this, "gpuEfficiency", "Percent of time GPUs have rays available to trace", &FS::getAverageGpuEfficiency);

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

std::string HSRStatistics::FormattedShort::getRecommendedStringTemplate()
{
	std::string stringTemplate = RendererStatistics::FormattedShort::getRecommendedStringTemplate();
	if (rs->getDeviceCount() != 0)
		stringTemplate += " %gpuCount%";
	stringTemplate += ": %samplesPerPixel%";
	if (rs->getHaltSpp() != std::numeric_limits<double>::infinity())
		stringTemplate += " (%percentHaltSppComplete%)";
	stringTemplate += " %samplesPerSecondWindow% %contributionsPerSecondWindow% %efficiency%";
	if (rs->getDeviceCount() != 0)
		stringTemplate += " %gpuEfficiency%";

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

std::string HSRStatistics::FormattedShort::getProgress() { 
	return static_cast<HSRStatistics::FormattedLong*>(rs->formattedLong)->getTotalAverageSamplesPerPixel();
}

std::string HSRStatistics::FormattedShort::getGpuCount() {
	return boost::str(boost::format("%1% G") % rs->getDeviceCount());
}

std::string HSRStatistics::FormattedShort::getAverageGpuEfficiency() {
	return boost::str(boost::format("%1$0.0f%% GEff") % rs->getAverageGpuEfficiency());
}

std::string HSRStatistics::FormattedShort::getPathEfficiency() {
	return boost::str(boost::format("%1$0.0f%% PEff") % rs->getPathEfficiency());
}

std::string HSRStatistics::FormattedShort::getPathEfficiencyWindow() {
	return boost::str(boost::format("%1$0.0f%% PEff") % rs->getPathEfficiencyWindow());
}
