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

// stats.h*

#include "lux.h"
#include <ostream>
#include <boost/date_time/posix_time/posix_time.hpp>
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
	StatsData() : previousNetworkSamplesSec(0), previousNetworkSamples(0), lastUpdateSecElapsed(0) {};
	~StatsData() {};

	void updateData(
		int _px,
		double _secelapsed,
		double _localsamples,
		float _eff,
		int _threadCount,
		u_int _serverCount,
		double _netsamples,
		bool add_total
	) {
		px = _px;
		secelapsed = _secelapsed;
		localsamples = _localsamples;
		eff = _eff;
		threadCount = _threadCount;
		serverCount = _serverCount;
		netsamples = _netsamples;

		FormatStatsString(add_total);
	}

	// Outputs
	string formattedStatsString;

private:

	void FormatStatsString(const bool add_total)
	{
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

		boost::posix_time::time_duration td(0, 0, secelapsed, 0);

		std::stringstream ss;
		ss.setf(std::stringstream::fixed);

		ss.precision(0);
		ss << td << " - " << threadCount << "T: ";
		ss.precision( magnitude_dp(local_spp) );
		ss << magnitude_reduce(local_spp) << magnitude_prefix(local_spp) << "S/p ";

		ss.precision( magnitude_dp(local_sps) );
		ss	<< magnitude_reduce(local_sps) << magnitude_prefix(local_sps) << "S/s ";

		ss.precision(0);
		ss << eff << "% Eff";

		if (serverCount > 0)
		{
			ss << " - " << serverCount << "N: ";

			if (netsamples > 0 && secelapsed > 0)
			{
				bool network_predicted = true;	// default to true since prediction will occur more frequently

				if (netsamples == previousNetworkSamples)
				{
					// Predict based on last reading
					netsamples = ((secelapsed - lastUpdateSecElapsed) * previousNetworkSamplesSec) +
									previousNetworkSamples;
					network_spp = netsamples / px;
					network_sps = previousNetworkSamplesSec;

					ss << "~";
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

				ss.precision( magnitude_dp(network_spp) );
				ss<< magnitude_reduce(network_spp) << magnitude_prefix(network_spp) << "S/p ";
				ss.precision( magnitude_dp(network_sps) );
				ss << magnitude_reduce(network_sps) << magnitude_prefix(network_sps) << "S/s";

				if (add_total)
				{
					float total_spp = (localsamples + netsamples) / px;
					float total_sps = (localsamples + netsamples) / secelapsed;

					ss << " - Tot: ";

					if (network_predicted) ss << "~";

					ss.precision( magnitude_dp(total_spp) );
					ss << magnitude_reduce(total_spp) << magnitude_prefix(total_spp) << "S/p ";
					ss.precision( magnitude_dp(total_sps) );
					ss << magnitude_reduce(total_sps) << magnitude_prefix(total_sps) << "S/s";
				}
			}
			else
			{
				ss << "Waiting for first update...";
			}
		}

		formattedStatsString = ss.str();
	}

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

	float magnitude_reduce(const float number) {
		if (number < 1024.f)
			return number;

		if ( number < 1048576.f)
			return number / 1024.f;

		return number / 1048576.f;
	}

	const char* magnitude_prefix(float number) {
		if (number < 1024.f)
			return "";

		if ( number < 1048576.f)
			return "k";

		return "M";
	}

	// Inputs
	int px;
	double secelapsed;
	double localsamples;
	float local_spp;
	float local_sps;
	float eff;
	int threadCount;
	u_int serverCount;
	double netsamples;
	float network_spp;
	float network_sps;

	// Storage
	float previousNetworkSamplesSec;
	double previousNetworkSamples;
	double lastUpdateSecElapsed;
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

