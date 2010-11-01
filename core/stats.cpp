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

StatsData::StatsData(Context *_ctx) :
	template_string_local("%1% - %2%T: %3$0.2f %4%S/p %5$0.2f %6%S/s %7$0.2f%% Eff"),
	template_string_network_waiting(" - %8%N: Waiting for first update..."),
	template_string_network(" - %8%N: %9%%10$0.2f %11%S/p %12$0.2f %13%S/s"),
	template_string_total(" - Tot: %9%%14$0.2f %15%S/p %16$0.2f %17%S/s"),
	template_string_haltspp(" - %18$0.2f%% Complete (S/Px)"),
	template_string_halttime(" - %19$0.2f%% Complete (sec)"),
	network_predicted(false),
	previousNetworkSamplesSec(0),
	previousNetworkSamples(0),
	lastUpdateSecElapsed(0)
{
	ctx = _ctx;
};

void StatsData::update(const bool _add_total)
{
	try {
		Queryable *film_registry = ctx->registry["film"];
		if (film_registry)
		{
			int xres = (*film_registry)["xResolution"].IntValue();
			int yres = (*film_registry)["yResolution"].IntValue();
			px = xres * yres;

			localsamples = (*film_registry)["numberOfLocalSamples"].DoubleValue();
			netsamples = (*film_registry)["numberOfSamplesFromNetwork"].DoubleValue();
			haltspp = (*film_registry)["haltSamplePerPixel"].IntValue();
			halttime = (*film_registry)["haltTime"].IntValue();
		}

		secelapsed = ctx->Statistics("secElapsed");
		eff = ctx->Statistics("efficiency");
		threadCount = ctx->Statistics("threadCount");

		serverCount = ctx->GetServerCount();

		add_total = _add_total;

		FormatStatsString();

	} catch (std::runtime_error e) {
		LOG(LUX_ERROR,LUX_CONSISTENCY)<< e.what();
	}
};

/**
 * Format the input data into a readable string. Predict the network
 * rendering progress if necessary.
 *
 */

void StatsData::FormatStatsString()
{
	std::ostringstream os;
	os << template_string_local;

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

	// This is so that completion_samples can be calculated if not networking or add_total==false
	total_spp = local_spp;
	total_sps = local_sps;

	if (serverCount > 0)
	{
		if (netsamples > 0 && secelapsed > 0)
		{
			os << template_string_network;

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
				os << template_string_total;

				total_spp = (localsamples + netsamples) / px;
				total_sps = (localsamples + netsamples) / secelapsed;
			}
		}
		else
		{
			os << template_string_network_waiting;
		}
	}

	if (haltspp > 0)
	{
		completion_samples = 100.f * (total_spp / haltspp);
		if (completion_samples > 100.f)
			completion_samples = 100.f; // keep at 100%
	}
	else
	{
		completion_samples = 0.f;
	}

	if (halttime > 0)
	{
		completion_time = 100.f * (secelapsed / halttime);
		if (completion_time > 100.f)
			completion_time = 100.f; // keep at 100%
	}
	else
	{
		completion_time = 0.f;
	}

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
	if (completion_samples > 0.f && timebased == false)
	{
		os << template_string_haltspp;
	}
	else if (completion_time > 0.f && timebased == true)
	{
		os << template_string_halttime;
	}

	boost::format stats_formatter = boost::format(os.str().c_str());
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
		/* 18 */ % completion_samples
		/* 19 */ % completion_time
	);
}


}
