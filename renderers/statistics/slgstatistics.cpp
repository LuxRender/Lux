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

#include "renderers/statistics/slgstatistics.h"
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

SLGStatistics::SLGStatistics(SLGRenderer* renderer)
	: deviceMaxMemory(MAX_DEVICE_COUNT, 0), deviceMemoryUsed(MAX_DEVICE_COUNT, 0),
	deviceRaySecs(MAX_DEVICE_COUNT, 0.0), renderer(renderer) {
	resetDerived();

	averageSampleSec = 0.0;
	triangleCount = 0;
	deviceCount = 0;
	deviceNames = "";

	formattedLong = new SLGStatistics::FormattedLong(this);
	formattedShort = new SLGStatistics::FormattedShort(this);

	AddDoubleAttribute(*this, "haltSamplesPerPixel", "Average number of samples per pixel to complete before halting", &SLGStatistics::getHaltSpp);
	AddDoubleAttribute(*this, "remainingSamplesPerPixel", "Average number of samples per pixel remaining", &SLGStatistics::getRemainingSamplesPerPixel);
	AddDoubleAttribute(*this, "percentHaltSppComplete", "Percent of halt S/p completed", &SLGStatistics::getPercentHaltSppComplete);
	AddDoubleAttribute(*this, "resumedSamplesPerPixel", "Average number of samples per pixel loaded from FLM", &SLGStatistics::getResumedAverageSamplesPerPixel);

	AddDoubleAttribute(*this, "samplesPerPixel", "Average number of samples per pixel by local node", &SLGStatistics::getAverageSamplesPerPixel);
	AddDoubleAttribute(*this, "samplesPerSecond", "Average number of samples per second by local node", &SLGStatistics::getAverageSamplesPerSecond);

	AddDoubleAttribute(*this, "netSamplesPerPixel", "Average number of samples per pixel by slave nodes", &SLGStatistics::getNetworkAverageSamplesPerPixel);
	AddDoubleAttribute(*this, "netSamplesPerSecond", "Average number of samples per second by slave nodes", &SLGStatistics::getNetworkAverageSamplesPerSecond);

	AddDoubleAttribute(*this, "totalSamplesPerPixel", "Average number of samples per pixel", &SLGStatistics::getTotalAverageSamplesPerPixel);
	AddDoubleAttribute(*this, "totalSamplesPerSecond", "Average number of samples per second", &SLGStatistics::getTotalAverageSamplesPerSecond);

	AddIntAttribute(*this, "deviceCount", "Number of OpenCL devices in use", &SLGStatistics::getDeviceCount);
	AddStringAttribute(*this, "deviceNames", "A comma separated list of names of OpenCL devices in use", &SLGStatistics::getDeviceNames);

	AddDoubleAttribute(*this, "triangleCount", "total number of triangles in the scene", &SLGStatistics::getTriangleCount);

	// Register up to MAX_DEVICE_COUNT devices

	AddDoubleAttribute(*this, "device.0.raysecs", "OpenCL device 0 ray traced per second", &SLGStatistics::getDevice00RaySecs);
	AddDoubleAttribute(*this, "device.1.raysecs", "OpenCL device 1 ray traced per second", &SLGStatistics::getDevice01RaySecs);
	AddDoubleAttribute(*this, "device.2.raysecs", "OpenCL device 2 ray traced per second", &SLGStatistics::getDevice02RaySecs);
	AddDoubleAttribute(*this, "device.3.raysecs", "OpenCL device 3 ray traced per second", &SLGStatistics::getDevice03RaySecs);
	AddDoubleAttribute(*this, "device.4.raysecs", "OpenCL device 4 ray traced per second", &SLGStatistics::getDevice04RaySecs);
	AddDoubleAttribute(*this, "device.5.raysecs", "OpenCL device 5 ray traced per second", &SLGStatistics::getDevice05RaySecs);
	AddDoubleAttribute(*this, "device.6.raysecs", "OpenCL device 6 ray traced per second", &SLGStatistics::getDevice06RaySecs);
	AddDoubleAttribute(*this, "device.7.raysecs", "OpenCL device 7 ray traced per second", &SLGStatistics::getDevice07RaySecs);
	AddDoubleAttribute(*this, "device.8.raysecs", "OpenCL device 8 ray traced per second", &SLGStatistics::getDevice08RaySecs);
	AddDoubleAttribute(*this, "device.9.raysecs", "OpenCL device 9 ray traced per second", &SLGStatistics::getDevice09RaySecs);
	AddDoubleAttribute(*this, "device.10.raysecs", "OpenCL device 10 ray traced per second", &SLGStatistics::getDevice10RaySecs);
	AddDoubleAttribute(*this, "device.11.raysecs", "OpenCL device 11 ray traced per second", &SLGStatistics::getDevice11RaySecs);
	AddDoubleAttribute(*this, "device.12.raysecs", "OpenCL device 12 ray traced per second", &SLGStatistics::getDevice12RaySecs);
	AddDoubleAttribute(*this, "device.13.raysecs", "OpenCL device 13 ray traced per second", &SLGStatistics::getDevice13RaySecs);
	AddDoubleAttribute(*this, "device.14.raysecs", "OpenCL device 14 ray traced per second", &SLGStatistics::getDevice14RaySecs);
	AddDoubleAttribute(*this, "device.15.raysecs", "OpenCL device 15 ray traced per second", &SLGStatistics::getDevice15RaySecs);

	AddDoubleAttribute(*this, "device.0.maxmem", "OpenCL device 0 memory available", &SLGStatistics::getDevice00MaxMemory);
	AddDoubleAttribute(*this, "device.1.maxmem", "OpenCL device 1 memory available", &SLGStatistics::getDevice01MaxMemory);
	AddDoubleAttribute(*this, "device.2.maxmem", "OpenCL device 2 memory available", &SLGStatistics::getDevice02MaxMemory);
	AddDoubleAttribute(*this, "device.3.maxmem", "OpenCL device 3 memory available", &SLGStatistics::getDevice03MaxMemory);
	AddDoubleAttribute(*this, "device.4.maxmem", "OpenCL device 4 memory available", &SLGStatistics::getDevice04MaxMemory);
	AddDoubleAttribute(*this, "device.5.maxmem", "OpenCL device 5 memory available", &SLGStatistics::getDevice05MaxMemory);
	AddDoubleAttribute(*this, "device.6.maxmem", "OpenCL device 6 memory available", &SLGStatistics::getDevice06MaxMemory);
	AddDoubleAttribute(*this, "device.7.maxmem", "OpenCL device 7 memory available", &SLGStatistics::getDevice07MaxMemory);
	AddDoubleAttribute(*this, "device.8.maxmem", "OpenCL device 8 memory available", &SLGStatistics::getDevice08MaxMemory);
	AddDoubleAttribute(*this, "device.9.maxmem", "OpenCL device 9 memory available", &SLGStatistics::getDevice09MaxMemory);
	AddDoubleAttribute(*this, "device.10.maxmem", "OpenCL device 10 memory available", &SLGStatistics::getDevice10MaxMemory);
	AddDoubleAttribute(*this, "device.11.maxmem", "OpenCL device 11 memory available", &SLGStatistics::getDevice11MaxMemory);
	AddDoubleAttribute(*this, "device.12.maxmem", "OpenCL device 12 memory available", &SLGStatistics::getDevice12MaxMemory);
	AddDoubleAttribute(*this, "device.13.maxmem", "OpenCL device 13 memory available", &SLGStatistics::getDevice13MaxMemory);
	AddDoubleAttribute(*this, "device.14.maxmem", "OpenCL device 14 memory available", &SLGStatistics::getDevice14MaxMemory);
	AddDoubleAttribute(*this, "device.15.maxmem", "OpenCL device 15 memory available", &SLGStatistics::getDevice15MaxMemory);

	AddDoubleAttribute(*this, "device.0.memusage", "OpenCL device 0 memory used", &SLGStatistics::getDevice00MemoryUsed);
	AddDoubleAttribute(*this, "device.1.memusage", "OpenCL device 1 memory used", &SLGStatistics::getDevice01MemoryUsed);
	AddDoubleAttribute(*this, "device.2.memusage", "OpenCL device 2 memory used", &SLGStatistics::getDevice02MemoryUsed);
	AddDoubleAttribute(*this, "device.3.memusage", "OpenCL device 3 memory used", &SLGStatistics::getDevice03MemoryUsed);
	AddDoubleAttribute(*this, "device.4.memusage", "OpenCL device 4 memory used", &SLGStatistics::getDevice04MemoryUsed);
	AddDoubleAttribute(*this, "device.5.memusage", "OpenCL device 5 memory used", &SLGStatistics::getDevice05MemoryUsed);
	AddDoubleAttribute(*this, "device.6.memusage", "OpenCL device 6 memory used", &SLGStatistics::getDevice06MemoryUsed);
	AddDoubleAttribute(*this, "device.7.memusage", "OpenCL device 7 memory used", &SLGStatistics::getDevice07MemoryUsed);
	AddDoubleAttribute(*this, "device.8.memusage", "OpenCL device 8 memory used", &SLGStatistics::getDevice08MemoryUsed);
	AddDoubleAttribute(*this, "device.9.memusage", "OpenCL device 9 memory used", &SLGStatistics::getDevice09MemoryUsed);
	AddDoubleAttribute(*this, "device.10.memusage", "OpenCL device 10 memory used", &SLGStatistics::getDevice10MemoryUsed);
	AddDoubleAttribute(*this, "device.11.memusage", "OpenCL device 11 memory used", &SLGStatistics::getDevice11MemoryUsed);
	AddDoubleAttribute(*this, "device.12.memusage", "OpenCL device 12 memory used", &SLGStatistics::getDevice12MemoryUsed);
	AddDoubleAttribute(*this, "device.13.memusage", "OpenCL device 13 memory used", &SLGStatistics::getDevice13MemoryUsed);
	AddDoubleAttribute(*this, "device.14.memusage", "OpenCL device 14 memory used", &SLGStatistics::getDevice14MemoryUsed);
	AddDoubleAttribute(*this, "device.15.memusage", "OpenCL device 15 memory used", &SLGStatistics::getDevice15MemoryUsed);
}

SLGStatistics::~SLGStatistics() {
	delete formattedLong;
	delete formattedShort;
}

double SLGStatistics::getRemainingTime() {
	double remainingTime = RendererStatistics::getRemainingTime();
	double remainingSamples = std::max(0.0, getHaltSpp() - getTotalAverageSamplesPerPixel()) * getPixelCount();

	return std::min(remainingTime, remainingSamples / getTotalAverageSamplesPerSecond());
}

// Returns haltSamplesPerPixel if set, otherwise infinity
double SLGStatistics::getHaltSpp() {
	double haltSpp = 0.0;

	Queryable *filmRegistry = Context::GetActive()->registry["film"];
	if (filmRegistry)
		haltSpp = (*filmRegistry)["haltSamplesPerPixel"].IntValue();

	return haltSpp > 0.0 ? haltSpp : std::numeric_limits<double>::infinity();
}

// Returns percent of haltSamplesPerPixel completed, zero if haltSamplesPerPixel is not set
double SLGStatistics::getPercentHaltSppComplete() {
	return (getTotalAverageSamplesPerPixel() / getHaltSpp()) * 100.0;
}

double SLGStatistics::getAverageSamplesPerSecond() {
	return averageSampleSec;
}

double SLGStatistics::getNetworkAverageSamplesPerSecond() {
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

u_int SLGStatistics::getPixelCount() {
	int xstart, xend, ystart, yend;

	renderer->scene->camera()->film->GetSampleExtent(&xstart, &xend, &ystart, &yend);

	return ((xend - xstart) * (yend - ystart));
}

double SLGStatistics::getResumedSampleCount() {
	double resumedSampleCount = 0.0;

	Queryable *filmRegistry = Context::GetActive()->registry["film"];
	if (filmRegistry)
		resumedSampleCount = (*filmRegistry)["numberOfResumedSamples"].DoubleValue();

	return resumedSampleCount;
}

double SLGStatistics::getSampleCount() {
	double sampleCount = 0.0;

	Queryable *filmRegistry = Context::GetActive()->registry["film"];
	if (filmRegistry)
		sampleCount = (*filmRegistry)["numberOfLocalSamples"].DoubleValue();

	return sampleCount;
}

double SLGStatistics::getNetworkSampleCount(bool estimate) {
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

SLGStatistics::FormattedLong::FormattedLong(SLGStatistics* rs)
	: RendererStatistics::FormattedLong(rs), rs(rs)
{
	typedef SLGStatistics::FormattedLong FL;

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

std::string SLGStatistics::FormattedLong::getRecommendedStringTemplate()
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

std::string SLGStatistics::FormattedLong::getHaltSpp() {
	return boost::str(boost::format("%1% S/p") % rs->getHaltSpp());
}

std::string SLGStatistics::FormattedLong::getRemainingSamplesPerPixel() {
	double rspp = rs->getRemainingSamplesPerPixel();
	return boost::str(boost::format("%1$0.2f %2%S/p") % MagnitudeReduce(rspp) % MagnitudePrefix(rspp));
}

std::string SLGStatistics::FormattedLong::getPercentHaltSppComplete() {
	return boost::str(boost::format("%1$0.0f%% S/p") % rs->getPercentHaltSppComplete());
}

std::string SLGStatistics::FormattedLong::getResumedAverageSamplesPerPixel() {
	double spp = rs->getResumedAverageSamplesPerPixel();
	return boost::str(boost::format("%1$0.2f %2%S/p") % MagnitudeReduce(spp) % MagnitudePrefix(spp));
}

std::string SLGStatistics::FormattedLong::getAverageSamplesPerPixel() {
	double spp = rs->getAverageSamplesPerPixel();
	return boost::str(boost::format("%1$0.2f %2%S/p") % MagnitudeReduce(spp) % MagnitudePrefix(spp));
}

std::string SLGStatistics::FormattedLong::getAverageSamplesPerSecond() {
	double sps = rs->getAverageSamplesPerSecond();
	return boost::str(boost::format("%1$0.2f %2%S/s") % MagnitudeReduce(sps) % MagnitudePrefix(sps));
}

std::string SLGStatistics::FormattedLong::getNetworkAverageSamplesPerPixel() {
	double spp = rs->getNetworkAverageSamplesPerPixel();
	return boost::str(boost::format("%1$0.2f %2%S/p") % MagnitudeReduce(spp) % MagnitudePrefix(spp));
}

std::string SLGStatistics::FormattedLong::getNetworkAverageSamplesPerSecond() {
	double sps = rs->getNetworkAverageSamplesPerSecond();
	return boost::str(boost::format("%1$0.2f %2%S/s") % MagnitudeReduce(sps) % MagnitudePrefix(sps));
}

std::string SLGStatistics::FormattedLong::getTotalAverageSamplesPerPixel() {
	double spp = rs->getTotalAverageSamplesPerPixel();
	return boost::str(boost::format("%1$0.2f %2%S/p") % MagnitudeReduce(spp) % MagnitudePrefix(spp));
}

std::string SLGStatistics::FormattedLong::getTotalAverageSamplesPerSecond() {
	double sps = rs->getTotalAverageSamplesPerSecond();
	return boost::str(boost::format("%1$0.2f %2%S/s") % MagnitudeReduce(sps) % MagnitudePrefix(sps));
}

std::string SLGStatistics::FormattedLong::getDeviceCount() {
	const u_int dc = rs->deviceCount;
	return boost::str(boost::format(boost::format("%1% %2%") % dc % Pluralize("Device", dc)));
}

std::string SLGStatistics::FormattedLong::getDeviceMemoryUsed() {
	// I assume the amount of used memory is the same on all devices. It is
	// always true for the moment.
	const size_t m = rs->deviceMemoryUsed[0];
	return boost::str(boost::format(boost::format("%1$0.2f %2%bytes") % MagnitudeReduce(m) % MagnitudePrefix(m)));
}

SLGStatistics::FormattedShort::FormattedShort(SLGStatistics* rs)
	: RendererStatistics::FormattedShort(rs), rs(rs) {
	FormattedLong* fl = static_cast<SLGStatistics::FormattedLong *>(rs->formattedLong);

	typedef SLGStatistics::FormattedLong FL;

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

std::string SLGStatistics::FormattedShort::getRecommendedStringTemplate() {
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

std::string SLGStatistics::FormattedShort::getProgress() { 
	return static_cast<SLGStatistics::FormattedLong*>(rs->formattedLong)->getTotalAverageSamplesPerPixel();
}
