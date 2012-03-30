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

#include <boost/bind.hpp>
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
	formattedLong = new SPPMRStatistics::FormattedLong(this);
	formattedShort = new SPPMRStatistics::FormattedShort(this);

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

SPPMRStatistics::~SPPMRStatistics()
{
	delete formattedLong;
	delete formattedShort;
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

SPPMRStatistics::FormattedLong::FormattedLong(SPPMRStatistics* rs)
	: RendererStatistics::FormattedLong(rs), rs(rs)
{
	AddStringAttribute(*this, "passCount", "Number of completed passes", &SPPMRStatistics::FormattedLong::getPassCount);
	AddStringAttribute(*this, "passesPerSecond", "Average number of passes per second", &SPPMRStatistics::FormattedLong::getAveragePassesPerSecond);
	AddStringAttribute(*this, "passesPerSecondWindow", "Average number of passes per second in current time window", &SPPMRStatistics::FormattedLong::getAveragePassesPerSecondWindow);

	AddStringAttribute(*this, "haltPass", "Number of passes to complete before halting", &SPPMRStatistics::FormattedLong::getHaltPass);
	AddStringAttribute(*this, "remainingPasses", "Number of passes remaining", &SPPMRStatistics::FormattedLong::getRemainingPasses);
	AddStringAttribute(*this, "percentHaltPassesComplete", "Percent of halt passes completed", &SPPMRStatistics::FormattedLong::getPercentHaltPassesComplete);

	AddStringAttribute(*this, "photonCount", "Current photon count", &SPPMRStatistics::FormattedLong::getPhotonCount);
	AddStringAttribute(*this, "photonsPerSecond", "Average number of photons per second", &SPPMRStatistics::FormattedLong::getAveragePhotonsPerSecond);
	AddStringAttribute(*this, "photonsPerSecondWindow", "Average number of photons per second in current time window", &SPPMRStatistics::FormattedLong::getAveragePhotonsPerSecondWindow);
}

std::string SPPMRStatistics::FormattedLong::getRecommendedStringTemplate()
{
	std::string stringTemplate = RendererStatistics::FormattedLong::getRecommendedStringTemplate();
	stringTemplate += ": %passCount%";
	if (rs->getHaltPass() != std::numeric_limits<double>::infinity())
		stringTemplate += " (%percentHaltPassesComplete%)";
	stringTemplate += " %passesPerSecondWindow% %photonCount% %photonsPerSecondWindow% %efficiency%";

	return stringTemplate;
}

std::string SPPMRStatistics::FormattedLong::getPassCount() {
	double pc = rs->getPassCount();
	return boost::str(boost::format("%1% %2%") % pc % Pluralize("Pass", pc));
}

std::string SPPMRStatistics::FormattedLong::getAveragePassesPerSecond() {
	double pps = rs->getAveragePassesPerSecond();
	return boost::str(boost::format("%1$0.2f%2% P/s") % MagnitudeReduce(pps) % MagnitudePrefix(pps));
}

std::string SPPMRStatistics::FormattedLong::getAveragePassesPerSecondWindow() {
	double pps = rs->getAveragePassesPerSecondWindow();
	return boost::str(boost::format("%1$0.2f%2% P/s") % MagnitudeReduce(pps) % MagnitudePrefix(pps));
}

std::string SPPMRStatistics::FormattedLong::getHaltPass() {
	double hp = rs->getHaltPass();
	return boost::str(boost::format("%1% %2%") % hp % Pluralize("Pass", hp));
}

std::string SPPMRStatistics::FormattedLong::getRemainingPasses() {
	double rp = rs->getRemainingPasses();
	return boost::str(boost::format("%1% %2%") % rp % Pluralize("Pass", rp));
}

std::string SPPMRStatistics::FormattedLong::getPercentHaltPassesComplete() {
	return boost::str(boost::format("%1$0.0f%% Passes Complete") % rs->getPercentHaltPassesComplete());
}

std::string SPPMRStatistics::FormattedLong::getPhotonCount() {
	double pc = rs->getPhotonCount();
	return boost::str(boost::format("%1$0.2f%2% %3%") % MagnitudeReduce(pc) % MagnitudePrefix(pc) % Pluralize("Photon", pc));
}

std::string SPPMRStatistics::FormattedLong::getAveragePhotonsPerSecond() {
	double pps = rs->getAveragePhotonsPerSecond();
	return boost::str(boost::format("%1$0.2f%2% Y/s") % MagnitudeReduce(pps) % MagnitudePrefix(pps));
}

std::string SPPMRStatistics::FormattedLong::getAveragePhotonsPerSecondWindow() {
	double pps = rs->getAveragePhotonsPerSecondWindow();
	return boost::str(boost::format("%1$0.2f%2% Y/s") % MagnitudeReduce(pps) % MagnitudePrefix(pps));
}

SPPMRStatistics::FormattedShort::FormattedShort(SPPMRStatistics* rs)
	: RendererStatistics::FormattedShort(rs), rs(rs)
{
	FormattedLong* fl = static_cast<SPPMRStatistics::FormattedLong*>(rs->formattedLong);

	AddStringAttribute(*this, "passCount", "Number of completed passes", &SPPMRStatistics::FormattedShort::getPassCount);
	AddStringAttribute(*this, "passesPerSecond", "Average number of passes per second", boost::bind(&SPPMRStatistics::FormattedLong::getAveragePassesPerSecond, fl));
	AddStringAttribute(*this, "passesPerSecondWindow", "Average number of passes per second in current time window", boost::bind(&SPPMRStatistics::FormattedLong::getAveragePassesPerSecondWindow, fl));

	AddStringAttribute(*this, "haltPass", "Number of passes to complete before halting", &SPPMRStatistics::FormattedShort::getHaltPass);
	AddStringAttribute(*this, "remainingPasses", "Number of passes remaining", &SPPMRStatistics::FormattedShort::getRemainingPasses);
	AddStringAttribute(*this, "percentHaltPassesComplete", "Percent of halt passes completed", &SPPMRStatistics::FormattedShort::getPercentHaltPassesComplete);

	AddStringAttribute(*this, "photonCount", "Current photon count", &SPPMRStatistics::FormattedShort::getPhotonCount);
	AddStringAttribute(*this, "photonsPerSecond", "Average number of photons per second", boost::bind(&SPPMRStatistics::FormattedLong::getAveragePhotonsPerSecond, fl));
	AddStringAttribute(*this, "photonsPerSecondWindow", "Average number of photons per second in current time window", boost::bind(&SPPMRStatistics::FormattedLong::getAveragePhotonsPerSecondWindow, fl));
}

std::string SPPMRStatistics::FormattedShort::getRecommendedStringTemplate()
{
	std::string stringTemplate = RendererStatistics::FormattedShort::getRecommendedStringTemplate();
	stringTemplate += ": %passCount%";
	if (rs->getHaltPass() != std::numeric_limits<double>::infinity())
		stringTemplate += " (%percentHaltPassesComplete%)";
	stringTemplate += " %passesPerSecondWindow% %photonCount% %photonsPerSecondWindow% %efficiency%";

	return stringTemplate;
}

std::string SPPMRStatistics::FormattedShort::getPassCount() {
	return boost::str(boost::format("%1% Pass") % rs->getPassCount());
}

std::string SPPMRStatistics::FormattedShort::getHaltPass() {
	return boost::str(boost::format("%1% Pass") % rs->getHaltPass());
}

std::string SPPMRStatistics::FormattedShort::getRemainingPasses() {
	return boost::str(boost::format("%1% Pass") % rs->getRemainingPasses());
}

std::string SPPMRStatistics::FormattedShort::getPercentHaltPassesComplete() {
	return boost::str(boost::format("%1$0.0f%% Pass Cmplt") % rs->getPercentHaltPassesComplete());
}

std::string SPPMRStatistics::FormattedShort::getPhotonCount() {
	double pc = rs->getPhotonCount();
	return boost::str(boost::format("%1$0.2f%2% Y") % MagnitudeReduce(pc) % MagnitudePrefix(pc));
}
