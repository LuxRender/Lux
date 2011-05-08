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
 *   Lux Renderer website : http://www.luxrender.org                       *
 ***************************************************************************/

#ifndef LUX_STATS_H
#define LUX_STATS_H

// stats.h
#include "lux.h"

#include <ostream>
using std::ostream;

namespace lux
{

/**
 * Store a few pieces of stats info in order to
 * allow a simple bit of prediction between network
 * updates. One of these objects is kept per Context
 * instance.
 */
class StatsData {
public:
	StatsData(Context *_ctx);
	~StatsData() {};

	void update(const bool add_total);

	// Outputs
	string formattedStatsString;

	// Inputs
	static string template_string_local;
	static string template_string_network_waiting;
	static string template_string_network;
	static string template_string_total;
	static string template_string_haltspp;
	static string template_string_time_remaining;
	static string template_string_halttime;
	static string template_string_renderer;

private:
	Context *ctx;									// Reference to context that created this StatsData object
	float previousNetworkSamplesSec;				// Last known network_sps
	double previousNetworkSamples;					// Last known netsamples
	double lastUpdateSecElapsed;					// secelapsed value when previous* members were updated

	/**
	 * Select an appropriate number of decimal places for the given number
	 */
	/*
	int magnitude_dp(const float number) {
		// 1.01
		// 9.91
		// 10.1
		// 99.1
		// 101.1
		// 991.1
		// 1011
		// 9911
		float reduced = magnitude_reduce(number);

		if (reduced < 10)
			return 2;

		if (reduced < 1000)
			return 1;

		return 0;
	}
	*/

	/**
	 * Reduce the magnitude on the input number by dividing into kilo- or Mega- or Giga- units
	 */
	inline float magnitude_reduce(const float number) {
		if (number < 1e3)
			return number;

		if ( number < 1e6)
			return number / 1e3;

		if ( number < 1e9)
			return number / 1e6;

		return number / 1e9;
	}

	/**
	 * Return the magnitude prefix char for kilo- or Mega- or Giga-
	 */
	inline const char* magnitude_prefix(float number) {
		if (number < 1e3)
			return "";

		if ( number < 1e6)
			return "k";

		if ( number < 1e9)
			return "M";

		return "G";
	}
};

void StatsPrint(ostream &dest);
void StatsCleanup();

class ProgressReporter {
public:
	// ProgressReporter Public Methods
	ProgressReporter(u_int totalWork, const string &title_, u_int barLength=58);
	~ProgressReporter();
	void Update(u_int num = 1) const;
	void Done() const;
	// ProgressReporter Data
	const u_int totalPlusses;
	float frequency;
	string title;
	mutable float count;
	mutable u_int plussesPrinted;
	mutable Timer *timer;
};
class StatsCounter {
public:
	// StatsCounter Public Methods
	StatsCounter(const string &category, const string &name);
	void operator++() { ++num; }
	void operator++(int) { ++num; }
	void Max(StatsCounterType val) { num = max(val, num); }
	void Min(StatsCounterType val) { num = min(val, num); }
	operator double() const { return static_cast<double>(num); }
private:
	// StatsCounter Private Data
	StatsCounterType num;
};
class StatsRatio {
public:
	// StatsRatio Public Methods
	StatsRatio(const string &category, const string &name);
	void Add(int a, int b) { na += a; nb += b; }
private:
	// StatsRatio Private Data
	StatsCounterType na, nb;
};
class StatsPercentage {
public:
	// StatsPercentage Public Methods
	void Add(int a, int b) { na += a; nb += b; }
	StatsPercentage(const string &category, const string &name);
private:
	// StatsPercentage Private Data
	StatsCounterType na, nb;
};

}

#endif // LUX_STATS_H

