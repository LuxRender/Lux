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

#include "renderers/statistics/sppmstatistics.h"
#include "context.h"

#include <limits>
#include <string>

#include <boost/format.hpp>

using namespace lux;

SPPMRStatistics::SPPMRStatistics(SPPMRenderer* renderer)
	: renderer(renderer),
	windowPps(0.0),
	windowYps(0.0),
	windowPassCount(0.0),
	windowPhotonCount(0.0),
	windowPassStartTime(0.0),
	windowPhotonStartTime(0.0)
{
	formatted = new SPPMRStatistics::Formatted(this);

	AddDoubleAttribute(*this, "passCount", "Number of completed passes", &SPPMRStatistics::getPassCount);
	AddDoubleAttribute(*this, "passesPerSecond", "Average number of passes per second", &SPPMRStatistics::getAveragePassesPerSecond);
	AddDoubleAttribute(*this, "passesPerSecondWindow", "Average number of passes per second in current time window", &SPPMRStatistics::getAveragePassesPerSecondWindow);

	AddDoubleAttribute(*this, "haltPass", "Number of passes to complete before halting", &SPPMRStatistics::getHaltPass);
	AddDoubleAttribute(*this, "remainingPasses", "Number of passes remaining", &SPPMRStatistics::getRemainingPasses);
	AddDoubleAttribute(*this, "percentHaltPassesComplete", "Percent of halt passes completed", &SPPMRStatistics::getPercentHaltPassesComplete);

	AddDoubleAttribute(*this, "photonCount", "Current photon count", &SPPMRStatistics::getPhotonCount);
	AddDoubleAttribute(*this, "photonsPerSecond", "Average number of photons per second", &SPPMRStatistics::getAveragePhotonsPerSecond);
	AddDoubleAttribute(*this, "photonsPerSecondWindow", "Average number of photons per second in current time window", &SPPMRStatistics::getAveragePhotonsPerSecondWindow);
}

void SPPMRStatistics::resetDerived() {
	windowPps = 0.0;
	windowYps = 0.0;
	windowPassCount = 0.0;
	windowPhotonCount = 0.0;
	windowPassStartTime = 0.0;
	windowPhotonStartTime = 0.0;
}

void SPPMRStatistics::updateStatisticsWindowDerived()
{
	double passCount = getPassCount();
	double photonCount = getPhotonCount();

	if (passCount != windowPassCount)
	{
		windowPps = (passCount - windowPassCount) / (getElapsedTime() - windowPassStartTime);
		windowPassCount = passCount;
		windowPassStartTime = getElapsedTime();
	}

	if (photonCount != windowPhotonCount)
	{
		windowYps = (photonCount - windowPhotonCount) / (getElapsedTime() - windowPhotonStartTime);
		windowPhotonCount = photonCount;
		windowPhotonStartTime = getElapsedTime();
	}
}

double SPPMRStatistics::getAveragePassesPerSecond() {
	double et = getElapsedTime();
	return (et == 0.0) ? 0.0 : getPassCount() / et;
}

// Returns haltSamplesPerPixel if set, otherwise infinity
double SPPMRStatistics::getHaltPass() {
	double haltPass = 0.0;

	Queryable* filmRegistry = Context::GetActive()->registry["film"];
	if (filmRegistry)
		haltPass = (*filmRegistry)["haltSamplesPerPixel"].IntValue();

	return haltPass > 0.0 ? haltPass : std::numeric_limits<double>::infinity();
}

// Returns percent of haltSamplesPerPixel completed, zero if haltSamplesPerPixel is not set
double SPPMRStatistics::getPercentHaltPassesComplete() {
	return (getPassCount() / getHaltPass()) * 100.0;
}

double SPPMRStatistics::getAveragePhotonsPerSecond() {
	double et = getElapsedTime();
	return (et == 0.0) ? 0.0 : getPhotonCount() / et;
}

SPPMRStatistics::Formatted::Formatted(SPPMRStatistics* rs)
	: RendererStatistics::Formatted(rs), rs(rs)
{
	AddStringAttribute(*this, "passCount", "Number of completed passes", &SPPMRStatistics::Formatted::getPassCount);
	AddStringAttribute(*this, "_passCount_short", "Number of completed passes", &SPPMRStatistics::Formatted::getPassCountShort);
	AddStringAttribute(*this, "passesPerSecond", "Average number of passes per second", &SPPMRStatistics::Formatted::getAveragePassesPerSecond);
	AddStringAttribute(*this, "passesPerSecondWindow", "Average number of passes per second in current time window", &SPPMRStatistics::Formatted::getAveragePassesPerSecondWindow);

	AddStringAttribute(*this, "haltPass", "Number of passes to complete before halting", &SPPMRStatistics::Formatted::getHaltPass);
	AddStringAttribute(*this, "_haltPass_short", "Number of passes to complete before halting", &SPPMRStatistics::Formatted::getHaltPassShort);
	AddStringAttribute(*this, "remainingPasses", "Number of passes remaining", &SPPMRStatistics::Formatted::getRemainingPasses);
	AddStringAttribute(*this, "_remainingPasses_short", "Number of passes remaining", &SPPMRStatistics::Formatted::getRemainingPassesShort);
	AddStringAttribute(*this, "percentHaltPassesComplete", "Percent of halt passes completed", &SPPMRStatistics::Formatted::getPercentHaltPassesComplete);
	AddStringAttribute(*this, "_percentHaltPassesComplete_short", "Percent of halt passes completed", &SPPMRStatistics::Formatted::getPercentHaltPassesCompleteShort);

	AddStringAttribute(*this, "photonCount", "Current photon count", &SPPMRStatistics::Formatted::getPhotonCount);
	AddStringAttribute(*this, "_photonCount_short", "Current photon count", &SPPMRStatistics::Formatted::getPhotonCountShort);
	AddStringAttribute(*this, "photonsPerSecond", "Average number of photons per second", &SPPMRStatistics::Formatted::getAveragePhotonsPerSecond);
	AddStringAttribute(*this, "photonsPerSecondWindow", "Average number of photons per second in current time window", &SPPMRStatistics::Formatted::getAveragePhotonsPerSecondWindow);
}

std::string SPPMRStatistics::Formatted::getRecommendedStringTemplate()
{
	std::string stringTemplate = RendererStatistics::Formatted::getRecommendedStringTemplate();
	stringTemplate += ": %passCount%";
	if (rs->getHaltPass() != std::numeric_limits<double>::infinity())
		stringTemplate += " (%percentHaltPassesComplete%)";
	stringTemplate += " %passesPerSecondWindow% %photonCount% %photonsPerSecondWindow% %efficiency%";

	return stringTemplate;
}

std::string SPPMRStatistics::Formatted::getPassCount() {
	double pc = rs->getPassCount();
	return boost::str(boost::format("%1% %2%") % pc % pluralize("Pass", pc));
}

std::string SPPMRStatistics::Formatted::getPassCountShort() {
	return boost::str(boost::format("%1% Pass") % rs->getPassCount());
}

std::string SPPMRStatistics::Formatted::getAveragePassesPerSecond() {
	double pps = rs->getAveragePassesPerSecond();
	return boost::str(boost::format("%1$0.2f%2% P/s") % MagnitudeReduce(pps) % MagnitudePrefix(pps));
}

std::string SPPMRStatistics::Formatted::getAveragePassesPerSecondWindow() {
	double pps = rs->getAveragePassesPerSecondWindow();
	return boost::str(boost::format("%1$0.2f%2% P/s") % MagnitudeReduce(pps) % MagnitudePrefix(pps));
}

std::string SPPMRStatistics::Formatted::getHaltPass() {
	double hp = rs->getHaltPass();
	return boost::str(boost::format("%1% %2%") % hp % pluralize("Pass", hp));
}

std::string SPPMRStatistics::Formatted::getHaltPassShort() {
	return boost::str(boost::format("%1% Pass") % rs->getHaltPass());
}

std::string SPPMRStatistics::Formatted::getRemainingPasses() {
	double rp = rs->getRemainingPasses();
	return boost::str(boost::format("%1% %2%") % rp % pluralize("Pass", rp));
}

std::string SPPMRStatistics::Formatted::getRemainingPassesShort() {
	return boost::str(boost::format("%1% Pass") % rs->getRemainingPasses());
}

std::string SPPMRStatistics::Formatted::getPercentHaltPassesComplete() {
	return boost::str(boost::format("%1$0.0f%% Passes Complete") % rs->getPercentHaltPassesComplete());
}

std::string SPPMRStatistics::Formatted::getPercentHaltPassesCompleteShort() {
	return boost::str(boost::format("%1$0.0f%% Pass Cmplt") % rs->getPercentHaltPassesComplete());
}

std::string SPPMRStatistics::Formatted::getPhotonCount() {
	double pc = rs->getPhotonCount();
	return boost::str(boost::format("%1$0.2f%2% %3%") % MagnitudeReduce(pc) % MagnitudePrefix(pc) % pluralize("Photon", pc));
}

std::string SPPMRStatistics::Formatted::getPhotonCountShort() {
	double pc = rs->getPhotonCount();
	return boost::str(boost::format("%1$0.2f%2% Y") % MagnitudeReduce(pc) % MagnitudePrefix(pc));
}

std::string SPPMRStatistics::Formatted::getAveragePhotonsPerSecond() {
	double pps = rs->getAveragePhotonsPerSecond();
	return boost::str(boost::format("%1$0.2f%2% Y/s") % MagnitudeReduce(pps) % MagnitudePrefix(pps));
}

std::string SPPMRStatistics::Formatted::getAveragePhotonsPerSecondWindow() {
	double pps = rs->getAveragePhotonsPerSecondWindow();
	return boost::str(boost::format("%1$0.2f%2% Y/s") % MagnitudeReduce(pps) % MagnitudePrefix(pps));
}
