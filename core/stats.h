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
#include <iomanip>
#include <boost/date_time/posix_time/posix_time.hpp>
#include <boost/format.hpp>
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
	StatsData() :
		template_string_local("%1% - %2%T: %3$0.2f %4%S/p %5$0.2f %6%S/s %7$0.2f%% Eff"),
		template_string_network_waiting("%1% - %2%T: %3$0.2f %4%S/p %5$0.2f %6%S/s %7$0.2f%% Eff - %8%N: Waiting for first update..."),
		template_string_network("%1% - %2%T: %3$0.2f %4%S/p %5$0.2f %6%S/s %7$0.2f%% Eff - %8%N: %9%%10$0.2f %11%S/p %12$0.2f %13%S/s"),
		template_string_total("%1% - %2%T: %3$0.2f %4%S/p %5$0.2f %6%S/s %7$0.2f%% Eff - %8%N: %9%%10$0.2f %11%S/p %12$0.2f %13%S/s - Tot: %9%%14$0.2f %15%S/p %16$0.2f %17%S/s"),
		network_predicted(false),
		previousNetworkSamplesSec(0),
		previousNetworkSamples(0),
		lastUpdateSecElapsed(0)
	{};
	~StatsData() {};

	/**
	 * Set the input data and format the output string
	 *
	 */
	void updateData(
		u_int _px,
		double _secelapsed,
		double _localsamples,
		float _eff,
		int _threadCount,
		u_int _serverCount,
		double _netsamples,
		bool _add_total
	) {
		px = _px;
		secelapsed = _secelapsed;
		localsamples = _localsamples;
		eff = _eff;
		threadCount = _threadCount;
		serverCount = _serverCount;
		netsamples = _netsamples;
		add_total = _add_total;

		FormatStatsString();
	}

	void setTemplateString(int type, const string template_string)
	{
		switch(type)
		{
			case FORMAT_LOCAL:
				template_string_local = template_string;
				break;
			case FORMAT_NETWORK_WAITING:
				template_string_network_waiting = template_string;
				break;
			case FORMAT_NETWORK:
				template_string_network = template_string;
				break;
			case FORMAT_TOTAL:
				template_string_total = template_string;
				break;
		}
	}

	// Outputs
	string formattedStatsString;

private:
	enum {
		FORMAT_LOCAL,
		FORMAT_NETWORK_WAITING,
		FORMAT_NETWORK,
		FORMAT_TOTAL
	} TemplateType;

	/**
	 * Format the input data into a readable string. Predict the network
	 * rendering progress if necessary.
	 *
	 */
	void FormatStatsString()
	{
		int FORMAT_TEMPLATE = FORMAT_LOCAL;

		if (secelapsed > 0)
		{
			local_spp = localsamples / px;
			local_sps = localsamples / secelapsed;
		}
		else
		{
			local_spp = 0.f;
			local_sps = 0.f;
		}

		if (serverCount > 0)
		{
			if (netsamples > 0 && secelapsed > 0)
			{
				FORMAT_TEMPLATE = FORMAT_NETWORK;

				if (netsamples == previousNetworkSamples)
				{
					// Predict based on last reading
					netsamples = ((secelapsed - lastUpdateSecElapsed) * previousNetworkSamplesSec) +
									previousNetworkSamples;
					network_spp = netsamples / px;
					network_sps = previousNetworkSamplesSec;

					network_predicted = true;
				}
				else
				{
					// Use real data
					network_spp = netsamples / px;
					network_sps = netsamples / secelapsed;

					previousNetworkSamples = netsamples;
					previousNetworkSamplesSec = network_sps;
					lastUpdateSecElapsed = secelapsed;

					network_predicted = false;
				}

				if (add_total)
				{
					FORMAT_TEMPLATE = FORMAT_TOTAL;

					total_spp = (localsamples + netsamples) / px;
					total_sps = (localsamples + netsamples) / secelapsed;
				}
			}
			else
			{
				FORMAT_TEMPLATE = FORMAT_NETWORK_WAITING;
			}
		}

		boost::format stats_formatter;
		switch (FORMAT_TEMPLATE)
		{
			default:
			case FORMAT_LOCAL:
				stats_formatter = boost::format(template_string_local);
				break;

			case FORMAT_NETWORK_WAITING:
				stats_formatter = boost::format(template_string_network_waiting);
				break;

			case FORMAT_NETWORK:
				stats_formatter = boost::format(template_string_network);
				break;

			case FORMAT_TOTAL:
				stats_formatter = boost::format(template_string_total);
				break;
		}

		stats_formatter.exceptions( boost::io::all_error_bits ^(boost::io::too_many_args_bit | boost::io::too_few_args_bit) ); // Ignore extra or missing args

		formattedStatsString = str(stats_formatter
			/*  1 */ % boost::posix_time::time_duration(0, 0, secelapsed, 0)
			/*  2 */ % threadCount
			/*  3 */ % magnitude_reduce(local_spp)
			/*  4 */ % magnitude_prefix(local_spp)
			/*  5 */ % magnitude_reduce(local_sps)
			/*  6 */ % magnitude_prefix(local_sps)
			/*  7 */ % eff
			/*  8 */ % serverCount
			/*  9 */ % ((network_predicted)?"~":"")
			/* 10 */ % magnitude_reduce(network_spp)
			/* 11 */ % magnitude_prefix(network_spp)
			/* 12 */ % magnitude_reduce(network_sps)
			/* 13 */ % magnitude_prefix(network_sps)
			/* 14 */ % magnitude_reduce(total_spp)
			/* 15 */ % magnitude_prefix(total_spp)
			/* 16 */ % magnitude_reduce(total_sps)
			/* 17 */ % magnitude_prefix(total_sps)
		);
	}

	/**
	 * Select an appropriate number of decimal places for the given number
	 *
	 */
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

	/**
	 * Reduce the magnitude on the input number by dividing into kilo- or Mega- units
	 *
	 */
	float magnitude_reduce(const float number) {
		if (number < 1024.f)
			return number;

		if ( number < 1048576.f)
			return number / 1024.f;

		return number / 1048576.f;
	}

	/**
	 * Return the magnitude prefix char for kilo- or Mega-
	 *
	 */
	const char* magnitude_prefix(float number) {
		if (number < 1024.f)
			return "";

		if ( number < 1048576.f)
			return "k";

		return "M";
	}

public:
	// Inputs
	string template_string_local;			// String template to format the data into, local only, needs 7 placeholders
	string template_string_network_waiting;	// String template to format the data into, network waiting, needs 8 placeholders
	string template_string_network;			// String template to format the data into, network rendering, needs 13 placeholders
	string template_string_total;			// String template to format the data into, complete stats, needs 17 placeholders

	u_int px;								// Number of pixels in film
	u_int serverCount;						// Number of connected net slaves
	u_int threadCount;						// Number of local threads

	double secelapsed;						// Seconds since rendering started

	double localsamples;					// Number of film samples generated locally
	double netsamples;						// Number of film samples generated by net slaves

	float local_spp;						// Local Samples / Px
	float local_sps;						// Local Samples / Sec

	float network_spp;						// Network Samples / Px		(might be predicted; check network_predicted value)
	float network_sps;						// Network Samples / Sec	(might be predicted; check network_predicted value)

	float total_spp;						// Total Samples / Px		(might be predicted; check network_predicted value)
	float total_sps;						// Total Samples / Sec		(might be predicted; check network_predicted value)

	float eff;								// Rendering efficiency

	bool add_total;							// Add total (local+net) stats to output?

	// Storage
	bool network_predicted;					// Network stats were predicted at last update ?

private:
	float previousNetworkSamplesSec;		// Last known network_sps
	double previousNetworkSamples;			// Last known netsamples
	double lastUpdateSecElapsed;			// secelapsed value when previous* members were updated
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

