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

#include "context.h"
#include "stats.h"
#include "queryable.h"

#include <ostream>
#include <boost/date_time/posix_time/posix_time.hpp>
#include <boost/format.hpp>
using std::ostream;

namespace lux
{

// String template to format local only, provides placeholders 1 to 7 and 20, 21
string StatsData::template_string_local = "%1% - %2%T: %3$0.2f %4%S/p  %5$0.2f %6%S/s  %7$0.0f%% Eff  %20$0.2f %21%C/s";
// String template to format network waiting, provides placeholder 8
string StatsData::template_string_network_waiting = " - %8%N: Waiting...";
// String template to format network rendering, provides placeholders 9 to 13 and 22, 23
string StatsData::template_string_network = " - %8%N: %9%%10$0.2f %11%S/p  %12$0.0f %13%S/s"; // 22, 23 removed to save space
// String template to format complete stats, provides placeholders 9 and 14 to 17 and 24, 25
string StatsData::template_string_total = " - Tot: %9%%14$0.2f %15%S/p  %16$0.0f %17%S/s"; // 24, 25 removed to save space
// String template to format percent samples completion, provides placeholder 18
string StatsData::template_string_haltspp = " - %9%%18$0.0f%% Complete (S/Px)";
// String template to format time to completion, provides placeholder 27
string StatsData::template_string_time_remaining = " - %27% Remaining";
// String template to format percent time completion, provides placeholder 19
string StatsData::template_string_halttime = " - %9%%19$0.0f%% Complete (sec)";
// String template to renderer stats, provides placeholder 26
string StatsData::template_string_renderer = " - %26%";

StatsData::StatsData(Context *_ctx) :
	formattedStatsString(""),
	previousNetworkSamplesSec(0),
	previousNetworkSamples(0),
	lastUpdateSecElapsed(0),
	percentComplete(0)
{
	ctx = _ctx;
}

/**
 * Format the input data into a readable string. Predict the network
 * and total rendering progress if necessary.
 */
void StatsData::update(const bool add_total)
{
	/*
	 * %N comments refer to placeholder values used in template strings
	 */

	try {
		// These 5 vars are neither stored nor placed into the output string
		int px = 0;
		int haltspp = -1;
		int halttime = -1;
		double localsamples = 0.0;
		double netsamples = 0.0;

		Queryable *film_registry = ctx->registry["film"];
		if (film_registry)
		{
			px = (*film_registry)["xResolution"].IntValue() * (*film_registry)["yResolution"].IntValue();
			localsamples = (*film_registry)["numberOfLocalSamples"].DoubleValue();
			netsamples = (*film_registry)["numberOfSamplesFromNetwork"].DoubleValue();
			haltspp = (*film_registry)["haltSamplesPerPixel"].IntValue();
			halttime = (*film_registry)["haltTime"].IntValue();
		}

		if (px == 0)
		{
			// Stats calculation is impossible, give up now.
			return;
		}

		int threadCount = ctx->Statistics("threadCount");	// %2
		double eff = ctx->Statistics("efficiency");			// %7

		std::ostringstream os;
		os << template_string_local;

		double secelapsed = ctx->Statistics("secElapsed");	// %1
		double local_cps = 0;								// %20
		double local_spp = 0;								// %3, %4
		double local_sps = 0;								// %5, %6
		if (secelapsed > 0)
		{
			local_spp = localsamples / px;
			local_sps = localsamples / secelapsed;
			local_cps = local_sps * (eff/100.f);
		}

		// This is so that completion_samples can be calculated if not networking or add_total==false
		double total_cps = local_cps;						// %24
		double total_spp = local_spp;						// %14, %15
		double total_sps = local_sps;						// %16, %17

		int serverCount = ctx->GetServerCount();			// %8
		bool network_predicted = false;						// %9
		double network_cps = 0.0;							// %22
		double network_spp = 0.0;							// %10, %11
		double network_sps = 0.0;							// %12, %13
		if (serverCount > 0)
		{
			if (netsamples > 0.0 && secelapsed > 0.0)
			{
				os << template_string_network;

				if (netsamples == previousNetworkSamples)
				{
					// Predict based on last reading
					netsamples = ((secelapsed - lastUpdateSecElapsed) * previousNetworkSamplesSec) +
									previousNetworkSamples;
					network_spp = netsamples / px;
					network_sps = previousNetworkSamplesSec;
					network_cps = network_sps * (eff / 100.0);

					network_predicted = true;
				}
				else
				{
					// Use real data
					network_spp = netsamples / px;
					network_sps = (netsamples - previousNetworkSamples) / (secelapsed - lastUpdateSecElapsed);
					network_cps = network_sps * (eff / 100.0);

					previousNetworkSamples = netsamples;
					previousNetworkSamplesSec = network_sps;
					lastUpdateSecElapsed = secelapsed;
				}
			}
			else
			{
				os << template_string_network_waiting;
			}
		}
		if (add_total && netsamples > 0.0)
		{
			os << template_string_total;

			total_spp = (localsamples + netsamples) / px;
			total_sps = (localsamples + netsamples) / secelapsed;
			total_cps = total_sps * (eff / 100.0);
		}


		double completion_samples = 0.0;					// %18
		if (haltspp > 0)
		{
			completion_samples = min(100.0, 100.0 * (total_spp / haltspp));
		}

		double completion_time = 0.0;						// %19
		if (halttime > 0)
		{
			completion_time = min(100.0, 100.0 * (secelapsed / halttime));
		}

		// calculate the time remaining in seconds
		double seconds_remaining = 0.0;						// %27
		if (haltspp > 0)
		{
			if (((localsamples + netsamples) / px) > 1000.0) {
				seconds_remaining = (haltspp - ((localsamples + netsamples) / px)) / (((localsamples + netsamples) / px) / secelapsed);
			} else {
				seconds_remaining = (haltspp - total_spp) / (total_spp / secelapsed);
			}
		}
		seconds_remaining = max(0.0, seconds_remaining);

		// Show either one of completion stats, depending on which is greatest
		static bool timebased; // determine type once at start and keep it
		if (completion_samples > completion_time)
		{
			timebased = false;
		}
		else if (completion_time > completion_samples)
		{
			timebased = true;
		}
		if (completion_samples > 0.0 && timebased == false)
		{
			percentComplete = completion_samples;
			os << template_string_haltspp;
			os << template_string_time_remaining;
		}
		else if (completion_time > 0.0 && timebased == true)
		{
			percentComplete = completion_time;
			os << template_string_halttime;
		}

		string rendererStats = "";
		Queryable *renderer_registry = ctx->registry["renderer"];
		if (renderer_registry) {
			if (renderer_registry->HasAttribute("stats")) {
				os << template_string_renderer;
				rendererStats = (*renderer_registry)["stats"].StringValue();
			}
		}

		boost::format stats_formatter = boost::format(os.str().c_str());
		stats_formatter.exceptions( boost::io::all_error_bits ^(boost::io::too_many_args_bit | boost::io::too_few_args_bit) ); // Ignore extra or missing args

		typedef boost::posix_time::time_duration::sec_type sec_type;

		formattedStatsString = str(stats_formatter
			/*  %1 */ % boost::posix_time::time_duration(0, 0, static_cast<sec_type>(secelapsed), 0)
			/*  %2 */ % threadCount
			/*  %3 */ % magnitude_reduce(local_spp)
			/*  %4 */ % magnitude_prefix(local_spp)
			/*  %5 */ % magnitude_reduce(local_sps)
			/*  %6 */ % magnitude_prefix(local_sps)
			/*  %7 */ % eff
			/*  %8 */ % serverCount
			/*  %9 */ % ((network_predicted)?"~":"")
			/* %10 */ % magnitude_reduce(network_spp)
			/* %11 */ % magnitude_prefix(network_spp)
			/* %12 */ % magnitude_reduce(network_sps)
			/* %13 */ % magnitude_prefix(network_sps)
			/* %14 */ % magnitude_reduce(total_spp)
			/* %15 */ % magnitude_prefix(total_spp)
			/* %16 */ % magnitude_reduce(total_sps)
			/* %17 */ % magnitude_prefix(total_sps)
			/* %18 */ % completion_samples
			/* %19 */ % completion_time
			/* %20 */ % magnitude_reduce(local_cps)
			/* %21 */ % magnitude_prefix(local_cps)
			/* %22 */ % magnitude_reduce(network_cps)
			/* %23 */ % magnitude_prefix(network_cps)
			/* %24 */ % magnitude_reduce(total_cps)
			/* %25 */ % magnitude_prefix(total_cps)
			/* %26 */ % rendererStats
			/* %27 */ % boost::posix_time::time_duration(0, 0, static_cast<sec_type>(seconds_remaining), 0)
		);

	} catch (std::runtime_error e) {
			LOG(LUX_ERROR, LUX_CONSISTENCY)<< e.what();
	}
}

void StatsData::updateSPPM(const bool add_total) {
	// SPPM Renderer statistics

	try {
		std::ostringstream os;
		os << "%1% - %2%T: %3$d %4%pass  %5$0.2f %6%P  %7$0.2f %8%P/s %9$0.0f%% PEff";
		boost::format stats_formatter = boost::format(os.str().c_str());
		stats_formatter.exceptions(boost::io::all_error_bits ^
			(boost::io::too_many_args_bit | boost::io::too_few_args_bit)); // Ignore extra or missing args

		double secelapsed = ctx->Statistics("secElapsed");	// %1
		int threadCount = ctx->Statistics("threadCount");	// %2
		u_int pass = ctx->Statistics("pass");	// %3
		double local_p = ctx->Statistics("photonCount"); // %5
		double local_pps = ctx->Statistics("photonPerSecond"); // %7
		double local_peff = ctx->Statistics("hitPointsUpdateEfficiency"); // %9

		formattedStatsString = str(stats_formatter
				/*  %1 */ % boost::posix_time::time_duration(0, 0, secelapsed, 0)
				/*  %2 */ % threadCount
				/*  %3 */ % magnitude_reduce(pass)
				/*  %4 */ % magnitude_prefix(pass)
				/*  %5 */ % magnitude_reduce(local_p)
				/*  %6 */ % magnitude_prefix(local_p)
				/*  %7 */ % magnitude_reduce(local_pps)
				/*  %8 */ % magnitude_prefix(local_pps)
				/*  %9 */ % local_peff
			);
	} catch (std::runtime_error e) {
		LOG(LUX_ERROR, LUX_CONSISTENCY)<< e.what();
	}
}

}
