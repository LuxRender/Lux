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

#include "renderers/statistics/luxcorestatistics.h"
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

LuxCoreStatistics::LuxCoreStatistics(LuxCoreRenderer *renderer)
	: deviceMaxMemory(MAX_DEVICE_COUNT, 0), deviceMemoryUsed(MAX_DEVICE_COUNT, 0),
	deviceRaySecs(MAX_DEVICE_COUNT, 0.0), renderer(renderer) {
	resetDerived();

	averageSampleSec = 0.0;
	triangleCount = 0;
	deviceCount = 0;
	deviceNames = "";

	formattedLong = new LuxCoreStatistics::FormattedLong(this);
	formattedShort = new LuxCoreStatistics::FormattedShort(this);

	AddDoubleAttribute(*this, "haltSamplesPerPixel", "Average number of samples per pixel to complete before halting", &LuxCoreStatistics::getHaltSpp);
	AddDoubleAttribute(*this, "remainingSamplesPerPixel", "Average number of samples per pixel remaining", &LuxCoreStatistics::getRemainingSamplesPerPixel);
	AddDoubleAttribute(*this, "percentHaltSppComplete", "Percent of halt S/p completed", &LuxCoreStatistics::getPercentHaltSppComplete);
	AddDoubleAttribute(*this, "resumedSamplesPerPixel", "Average number of samples per pixel loaded from FLM", &LuxCoreStatistics::getResumedAverageSamplesPerPixel);

	AddDoubleAttribute(*this, "samplesPerPixel", "Average number of samples per pixel by local node", &LuxCoreStatistics::getAverageSamplesPerPixel);
	AddDoubleAttribute(*this, "samplesPerSecond", "Average number of samples per second by local node", &LuxCoreStatistics::getAverageSamplesPerSecond);

	AddDoubleAttribute(*this, "netSamplesPerPixel", "Average number of samples per pixel by slave nodes", &LuxCoreStatistics::getNetworkAverageSamplesPerPixel);
	AddDoubleAttribute(*this, "netSamplesPerSecond", "Average number of samples per second by slave nodes", &LuxCoreStatistics::getNetworkAverageSamplesPerSecond);

	AddDoubleAttribute(*this, "totalSamplesPerPixel", "Average number of samples per pixel", &LuxCoreStatistics::getTotalAverageSamplesPerPixel);
	AddDoubleAttribute(*this, "totalSamplesPerSecond", "Average number of samples per second", &LuxCoreStatistics::getTotalAverageSamplesPerSecond);

	AddIntAttribute(*this, "deviceCount", "Number of OpenCL devices in use", &LuxCoreStatistics::getDeviceCount);
	AddStringAttribute(*this, "deviceNames", "A comma separated list of names of OpenCL devices in use", &LuxCoreStatistics::getDeviceNames);

	AddDoubleAttribute(*this, "triangleCount", "total number of triangles in the scene", &LuxCoreStatistics::getTriangleCount);

	// Register up to MAX_DEVICE_COUNT devices

	AddDoubleAttribute(*this, "device.0.raysecs", "OpenCL device 0 ray traced per second", &LuxCoreStatistics::getDevice00RaySecs);
	AddDoubleAttribute(*this, "device.1.raysecs", "OpenCL device 1 ray traced per second", &LuxCoreStatistics::getDevice01RaySecs);
	AddDoubleAttribute(*this, "device.2.raysecs", "OpenCL device 2 ray traced per second", &LuxCoreStatistics::getDevice02RaySecs);
	AddDoubleAttribute(*this, "device.3.raysecs", "OpenCL device 3 ray traced per second", &LuxCoreStatistics::getDevice03RaySecs);
	AddDoubleAttribute(*this, "device.4.raysecs", "OpenCL device 4 ray traced per second", &LuxCoreStatistics::getDevice04RaySecs);
	AddDoubleAttribute(*this, "device.5.raysecs", "OpenCL device 5 ray traced per second", &LuxCoreStatistics::getDevice05RaySecs);
	AddDoubleAttribute(*this, "device.6.raysecs", "OpenCL device 6 ray traced per second", &LuxCoreStatistics::getDevice06RaySecs);
	AddDoubleAttribute(*this, "device.7.raysecs", "OpenCL device 7 ray traced per second", &LuxCoreStatistics::getDevice07RaySecs);
	AddDoubleAttribute(*this, "device.8.raysecs", "OpenCL device 8 ray traced per second", &LuxCoreStatistics::getDevice08RaySecs);
	AddDoubleAttribute(*this, "device.9.raysecs", "OpenCL device 9 ray traced per second", &LuxCoreStatistics::getDevice09RaySecs);
	AddDoubleAttribute(*this, "device.10.raysecs", "OpenCL device 10 ray traced per second", &LuxCoreStatistics::getDevice10RaySecs);
	AddDoubleAttribute(*this, "device.11.raysecs", "OpenCL device 11 ray traced per second", &LuxCoreStatistics::getDevice11RaySecs);
	AddDoubleAttribute(*this, "device.12.raysecs", "OpenCL device 12 ray traced per second", &LuxCoreStatistics::getDevice12RaySecs);
	AddDoubleAttribute(*this, "device.13.raysecs", "OpenCL device 13 ray traced per second", &LuxCoreStatistics::getDevice13RaySecs);
	AddDoubleAttribute(*this, "device.14.raysecs", "OpenCL device 14 ray traced per second", &LuxCoreStatistics::getDevice14RaySecs);
	AddDoubleAttribute(*this, "device.15.raysecs", "OpenCL device 15 ray traced per second", &LuxCoreStatistics::getDevice15RaySecs);

	AddDoubleAttribute(*this, "device.0.maxmem", "OpenCL device 0 memory available", &LuxCoreStatistics::getDevice00MaxMemory);
	AddDoubleAttribute(*this, "device.1.maxmem", "OpenCL device 1 memory available", &LuxCoreStatistics::getDevice01MaxMemory);
	AddDoubleAttribute(*this, "device.2.maxmem", "OpenCL device 2 memory available", &LuxCoreStatistics::getDevice02MaxMemory);
	AddDoubleAttribute(*this, "device.3.maxmem", "OpenCL device 3 memory available", &LuxCoreStatistics::getDevice03MaxMemory);
	AddDoubleAttribute(*this, "device.4.maxmem", "OpenCL device 4 memory available", &LuxCoreStatistics::getDevice04MaxMemory);
	AddDoubleAttribute(*this, "device.5.maxmem", "OpenCL device 5 memory available", &LuxCoreStatistics::getDevice05MaxMemory);
	AddDoubleAttribute(*this, "device.6.maxmem", "OpenCL device 6 memory available", &LuxCoreStatistics::getDevice06MaxMemory);
	AddDoubleAttribute(*this, "device.7.maxmem", "OpenCL device 7 memory available", &LuxCoreStatistics::getDevice07MaxMemory);
	AddDoubleAttribute(*this, "device.8.maxmem", "OpenCL device 8 memory available", &LuxCoreStatistics::getDevice08MaxMemory);
	AddDoubleAttribute(*this, "device.9.maxmem", "OpenCL device 9 memory available", &LuxCoreStatistics::getDevice09MaxMemory);
	AddDoubleAttribute(*this, "device.10.maxmem", "OpenCL device 10 memory available", &LuxCoreStatistics::getDevice10MaxMemory);
	AddDoubleAttribute(*this, "device.11.maxmem", "OpenCL device 11 memory available", &LuxCoreStatistics::getDevice11MaxMemory);
	AddDoubleAttribute(*this, "device.12.maxmem", "OpenCL device 12 memory available", &LuxCoreStatistics::getDevice12MaxMemory);
	AddDoubleAttribute(*this, "device.13.maxmem", "OpenCL device 13 memory available", &LuxCoreStatistics::getDevice13MaxMemory);
	AddDoubleAttribute(*this, "device.14.maxmem", "OpenCL device 14 memory available", &LuxCoreStatistics::getDevice14MaxMemory);
	AddDoubleAttribute(*this, "device.15.maxmem", "OpenCL device 15 memory available", &LuxCoreStatistics::getDevice15MaxMemory);

	AddDoubleAttribute(*this, "device.0.memusage", "OpenCL device 0 memory used", &LuxCoreStatistics::getDevice00MemoryUsed);
	AddDoubleAttribute(*this, "device.1.memusage", "OpenCL device 1 memory used", &LuxCoreStatistics::getDevice01MemoryUsed);
	AddDoubleAttribute(*this, "device.2.memusage", "OpenCL device 2 memory used", &LuxCoreStatistics::getDevice02MemoryUsed);
	AddDoubleAttribute(*this, "device.3.memusage", "OpenCL device 3 memory used", &LuxCoreStatistics::getDevice03MemoryUsed);
	AddDoubleAttribute(*this, "device.4.memusage", "OpenCL device 4 memory used", &LuxCoreStatistics::getDevice04MemoryUsed);
	AddDoubleAttribute(*this, "device.5.memusage", "OpenCL device 5 memory used", &LuxCoreStatistics::getDevice05MemoryUsed);
	AddDoubleAttribute(*this, "device.6.memusage", "OpenCL device 6 memory used", &LuxCoreStatistics::getDevice06MemoryUsed);
	AddDoubleAttribute(*this, "device.7.memusage", "OpenCL device 7 memory used", &LuxCoreStatistics::getDevice07MemoryUsed);
	AddDoubleAttribute(*this, "device.8.memusage", "OpenCL device 8 memory used", &LuxCoreStatistics::getDevice08MemoryUsed);
	AddDoubleAttribute(*this, "device.9.memusage", "OpenCL device 9 memory used", &LuxCoreStatistics::getDevice09MemoryUsed);
	AddDoubleAttribute(*this, "device.10.memusage", "OpenCL device 10 memory used", &LuxCoreStatistics::getDevice10MemoryUsed);
	AddDoubleAttribute(*this, "device.11.memusage", "OpenCL device 11 memory used", &LuxCoreStatistics::getDevice11MemoryUsed);
	AddDoubleAttribute(*this, "device.12.memusage", "OpenCL device 12 memory used", &LuxCoreStatistics::getDevice12MemoryUsed);
	AddDoubleAttribute(*this, "device.13.memusage", "OpenCL device 13 memory used", &LuxCoreStatistics::getDevice13MemoryUsed);
	AddDoubleAttribute(*this, "device.14.memusage", "OpenCL device 14 memory used", &LuxCoreStatistics::getDevice14MemoryUsed);
	AddDoubleAttribute(*this, "device.15.memusage", "OpenCL device 15 memory used", &LuxCoreStatistics::getDevice15MemoryUsed);
}

LuxCoreStatistics::~LuxCoreStatistics() {
	delete formattedLong;
	delete formattedShort;
}

double LuxCoreStatistics::getRemainingTime() {
	double remainingTime = RendererStatistics::getRemainingTime();
	double remainingSamples = std::max(0.0, getHaltSpp() - getTotalAverageSamplesPerPixel()) * getPixelCount();

	return std::min(remainingTime, remainingSamples / getTotalAverageSamplesPerSecond());
}

// Returns haltSamplesPerPixel if set, otherwise infinity
double LuxCoreStatistics::getHaltSpp() {
	double haltSpp = 0.0;

	Queryable *filmRegistry = Context::GetActive()->registry["film"];
	if (filmRegistry)
		haltSpp = (*filmRegistry)["haltSamplesPerPixel"].IntValue();

	return haltSpp > 0.0 ? haltSpp : std::numeric_limits<double>::infinity();
}

// Returns percent of haltSamplesPerPixel completed, zero if haltSamplesPerPixel is not set
double LuxCoreStatistics::getPercentHaltSppComplete() {
	return (getTotalAverageSamplesPerPixel() / getHaltSpp()) * 100.0;
}

double LuxCoreStatistics::getAverageSamplesPerSecond() {
	return averageSampleSec;
}

double LuxCoreStatistics::getNetworkAverageSamplesPerSecond() {
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

u_int LuxCoreStatistics::getPixelCount() {
	int xstart, xend, ystart, yend;

	renderer->scene->camera()->film->GetSampleExtent(&xstart, &xend, &ystart, &yend);

	return ((xend - xstart) * (yend - ystart));
}

double LuxCoreStatistics::getResumedSampleCount() {
	double resumedSampleCount = 0.0;

	Queryable *filmRegistry = Context::GetActive()->registry["film"];
	if (filmRegistry)
		resumedSampleCount = (*filmRegistry)["numberOfResumedSamples"].DoubleValue();

	return resumedSampleCount;
}

double LuxCoreStatistics::getSampleCount() {
	double sampleCount = 0.0;

	Queryable *filmRegistry = Context::GetActive()->registry["film"];
	if (filmRegistry)
		sampleCount = (*filmRegistry)["numberOfLocalSamples"].DoubleValue();

	return sampleCount;
}

double LuxCoreStatistics::getNetworkSampleCount(bool estimate) {
	double networkSampleCount = 0.0;

	Queryable *filmRegistry = Context::GetActive()->registry["film"];
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

LuxCoreStatistics::FormattedLong::FormattedLong(LuxCoreStatistics* rs)
	: RendererStatistics::FormattedLong(rs), rs(rs)
{
	typedef LuxCoreStatistics::FormattedLong FL;

	AddStringAttribute(*this, "haltSamplesPerPixel", "Average number of samples per pixel to complete before halting", &FL::getHaltSpp);
	AddStringAttribute(*this, "remainingSamplesPerPixel", "Average number of samples per pixel remaining", &FL::getRemainingSamplesPerPixel);
	AddStringAttribute(*this, "percentHaltSppComplete", "Percent of halt S/p completed", &FL::getPercentHaltSppComplete);
	AddStringAttribute(*this, "resumedSamplesPerPixel", "Average number of samples per pixel loaded from FLM", &FL::getResumedAverageSamplesPerPixel);

	AddStringAttribute(*this, "samplesPerPixel", "Average number of samples per pixel by local node", &FL::getAverageSamplesPerPixel);
	AddStringAttribute(*this, "samplesPerSecond", "Average number of samples per second by local node", &FL::getAverageSamplesPerSecond);

	AddStringAttribute(*this, "netSamplesPerPixel", "Average number of samples per pixel by slave nodes", &FL::getNetworkAverageSamplesPerPixel);
	AddStringAttribute(*this, "netSamplesPerSecond", "Average number of samples per second by slave nodes", &FL::getNetworkAverageSamplesPerSecond);

	AddStringAttribute(*this, "totalSamplesPerPixel", "Average number of samples per pixel", &FL::getTotalAverageSamplesPerPixel);
	AddStringAttribute(*this, "totalSamplesPerSecond", "Average number of samples per second", &FL::getTotalAverageSamplesPerSecond);

	AddStringAttribute(*this, "deviceCount", "Number of LuxRays devices in use", &FL::getDeviceCount);
	AddStringAttribute(*this, "memoryUsage", "LuxRays devices memory used", &FL::getDeviceMemoryUsed);
}

std::string LuxCoreStatistics::FormattedLong::getRecommendedStringTemplate()
{
	std::string stringTemplate = RendererStatistics::FormattedLong::getRecommendedStringTemplate();
	stringTemplate += ": %samplesPerPixel%";
	if (rs->getHaltSpp() != std::numeric_limits<double>::infinity())
		stringTemplate += " (%percentHaltSppComplete%)";
	stringTemplate += " %samplesPerSecond%";

	stringTemplate += " %deviceCount%";
	if ((rs->deviceMemoryUsed.size() > 0) && (rs->deviceMemoryUsed[0] > 0))
		stringTemplate += " %memoryUsage%";

	if (rs->getNetworkSampleCount() != 0.0)
	{
		if (rs->getSlaveNodeCount() != 0)
			stringTemplate += " | Net: ~%netSamplesPerPixel% ~%netSamplesPerSecond%";
		else
			stringTemplate += " | Net: %netSamplesPerPixel% %netSamplesPerSecond%";
	}

	if (rs->getNetworkSampleCount() != 0.0 && rs->getSlaveNodeCount())
		stringTemplate += " | Tot: ~%totalSamplesPerPixel% ~%totalSamplesPerSecond%";
	else if (rs->getResumedSampleCount() != 0.0)
		stringTemplate += " | Tot: %totalSamplesPerPixel% %totalSamplesPerSecond%";

	return stringTemplate;
}

std::string LuxCoreStatistics::FormattedLong::getHaltSpp() {
	return boost::str(boost::format("%1% S/p") % rs->getHaltSpp());
}

std::string LuxCoreStatistics::FormattedLong::getRemainingSamplesPerPixel() {
	double rspp = rs->getRemainingSamplesPerPixel();
	return boost::str(boost::format("%1$0.2f %2%S/p") % MagnitudeReduce(rspp) % MagnitudePrefix(rspp));
}

std::string LuxCoreStatistics::FormattedLong::getPercentHaltSppComplete() {
	return boost::str(boost::format("%1$0.0f%% S/p") % rs->getPercentHaltSppComplete());
}

std::string LuxCoreStatistics::FormattedLong::getResumedAverageSamplesPerPixel() {
	double spp = rs->getResumedAverageSamplesPerPixel();
	return boost::str(boost::format("%1$0.2f %2%S/p") % MagnitudeReduce(spp) % MagnitudePrefix(spp));
}

std::string LuxCoreStatistics::FormattedLong::getAverageSamplesPerPixel() {
	double spp = rs->getAverageSamplesPerPixel();
	return boost::str(boost::format("%1$0.2f %2%S/p") % MagnitudeReduce(spp) % MagnitudePrefix(spp));
}

std::string LuxCoreStatistics::FormattedLong::getAverageSamplesPerSecond() {
	double sps = rs->getAverageSamplesPerSecond();
	return boost::str(boost::format("%1$0.2f %2%S/s") % MagnitudeReduce(sps) % MagnitudePrefix(sps));
}

std::string LuxCoreStatistics::FormattedLong::getNetworkAverageSamplesPerPixel() {
	double spp = rs->getNetworkAverageSamplesPerPixel();
	return boost::str(boost::format("%1$0.2f %2%S/p") % MagnitudeReduce(spp) % MagnitudePrefix(spp));
}

std::string LuxCoreStatistics::FormattedLong::getNetworkAverageSamplesPerSecond() {
	double sps = rs->getNetworkAverageSamplesPerSecond();
	return boost::str(boost::format("%1$0.2f %2%S/s") % MagnitudeReduce(sps) % MagnitudePrefix(sps));
}

std::string LuxCoreStatistics::FormattedLong::getTotalAverageSamplesPerPixel() {
	double spp = rs->getTotalAverageSamplesPerPixel();
	return boost::str(boost::format("%1$0.2f %2%S/p") % MagnitudeReduce(spp) % MagnitudePrefix(spp));
}

std::string LuxCoreStatistics::FormattedLong::getTotalAverageSamplesPerSecond() {
	double sps = rs->getTotalAverageSamplesPerSecond();
	return boost::str(boost::format("%1$0.2f %2%S/s") % MagnitudeReduce(sps) % MagnitudePrefix(sps));
}

std::string LuxCoreStatistics::FormattedLong::getDeviceCount() {
	const u_int dc = rs->deviceCount;
	return boost::str(boost::format(boost::format("%1% %2%") % dc % Pluralize("Device", dc)));
}

std::string LuxCoreStatistics::FormattedLong::getDeviceMemoryUsed() {
	// I assume the amount of used memory is the same on all devices. It is
	// always true for the moment.
	const size_t m = rs->deviceMemoryUsed[0];
	return boost::str(boost::format(boost::format("%1$0.2f %2%bytes") % MagnitudeReduce(m) % MagnitudePrefix(m)));
}

LuxCoreStatistics::FormattedShort::FormattedShort(LuxCoreStatistics* rs)
	: RendererStatistics::FormattedShort(rs), rs(rs) {
	FormattedLong* fl = static_cast<LuxCoreStatistics::FormattedLong *>(rs->formattedLong);

	typedef LuxCoreStatistics::FormattedLong FL;

	AddStringAttribute(*this, "haltSamplesPerPixel", "Average number of samples per pixel to complete before halting", boost::bind(boost::mem_fn(&FL::getHaltSpp), fl));
	AddStringAttribute(*this, "remainingSamplesPerPixel", "Average number of samples per pixel remaining", boost::bind(boost::mem_fn(&FL::getRemainingSamplesPerPixel), fl));
	AddStringAttribute(*this, "percentHaltSppComplete", "Percent of halt S/p completed", boost::bind(boost::mem_fn(&FL::getPercentHaltSppComplete), fl));
	AddStringAttribute(*this, "resumedSamplesPerPixel", "Average number of samples per pixel loaded from FLM", boost::bind(boost::mem_fn(&FL::getResumedAverageSamplesPerPixel), fl));

	AddStringAttribute(*this, "samplesPerPixel", "Average number of samples per pixel by local node", boost::bind(boost::mem_fn(&FL::getAverageSamplesPerPixel), fl));
	AddStringAttribute(*this, "samplesPerSecond", "Average number of samples per second by local node", boost::bind(boost::mem_fn(&FL::getAverageSamplesPerSecond), fl));

	AddStringAttribute(*this, "netSamplesPerPixel", "Average number of samples per pixel by slave nodes", boost::bind(boost::mem_fn(&FL::getNetworkAverageSamplesPerPixel), fl));
	AddStringAttribute(*this, "netSamplesPerSecond", "Average number of samples per second by slave nodes", boost::bind(boost::mem_fn(&FL::getNetworkAverageSamplesPerSecond), fl));

	AddStringAttribute(*this, "totalSamplesPerPixel", "Average number of samples per pixel", boost::bind(boost::mem_fn(&FL::getTotalAverageSamplesPerPixel), fl));
	AddStringAttribute(*this, "totalSamplesPerSecond", "Average number of samples per second", boost::bind(boost::mem_fn(&FL::getTotalAverageSamplesPerSecond), fl));

	AddStringAttribute(*this, "deviceCount", "Number of LuxRays devices in use", boost::bind(boost::mem_fn(&FL::getDeviceCount), fl));
	AddStringAttribute(*this, "memoryUsage", "LuxRays devices memory used", boost::bind(boost::mem_fn(&FL::getDeviceMemoryUsed), fl));
}

std::string LuxCoreStatistics::FormattedShort::getRecommendedStringTemplate() {
	std::string stringTemplate = RendererStatistics::FormattedShort::getRecommendedStringTemplate();
	stringTemplate += ": %samplesPerPixel%";
	if (rs->getHaltSpp() != std::numeric_limits<double>::infinity())
		stringTemplate += " (%percentHaltSppComplete%)";
	stringTemplate += " %samplesPerSecond%";

	stringTemplate += " %deviceCount%";
	if ((rs->deviceMemoryUsed.size() > 0) && (rs->deviceMemoryUsed[0] > 0))
		stringTemplate += " %memoryUsage%";
	
	if (rs->getNetworkSampleCount() != 0.0)
	{
		if (rs->getSlaveNodeCount() != 0)
			stringTemplate += " | Net: ~%netSamplesPerPixel% ~%netSamplesPerSecond%";
		else
			stringTemplate += " | Net: %netSamplesPerPixel% %netSamplesPerSecond%";
	}

	if (rs->getNetworkSampleCount() != 0.0 && rs->getSlaveNodeCount())
		stringTemplate += " | Tot: ~%totalSamplesPerPixel% ~%totalSamplesPerSecond%";
	else if (rs->getResumedSampleCount() != 0.0)
		stringTemplate += " | Tot: %totalSamplesPerPixel% %totalSamplesPerSecond%";

	return stringTemplate;
}

std::string LuxCoreStatistics::FormattedShort::getProgress() { 
	return static_cast<LuxCoreStatistics::FormattedLong*>(rs->formattedLong)->getTotalAverageSamplesPerPixel();
}
