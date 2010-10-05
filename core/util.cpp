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

// util.cpp*
#include "lux.h"
#include "stats.h"
#include "timer.h"

#include <map>
using std::map;
#include <iomanip>
using std::endl;
using std::setw;
using std::left;
using std::setfill;
using std::setprecision;
using std::flush;
using std::cout;

namespace lux
{

// Matrix Method Definitions
bool SolveLinearSystem2x2(const float A[2][2], const float B[2], float x[2])
{
	float det = A[0][0]*A[1][1] - A[0][1]*A[1][0];
	if (fabsf(det) < 1e-5)
		return false;
	float invDet = 1.0f/det;
	x[0] = (A[1][1]*B[0] - A[0][1]*B[1]) * invDet;
	x[1] = (A[0][0]*B[1] - A[1][0]*B[0]) * invDet;
	return true;
}

// Statistics Definitions
struct  StatTracker {
	StatTracker(const string &cat, const string &n,
	            StatsCounterType *pa, StatsCounterType *pb = NULL,
		    bool percentage = true);
	string category, name;
	StatsCounterType *ptra, *ptrb;
	bool percentage;
};
typedef map<std::pair<string, string>, StatTracker *> TrackerMap;
static TrackerMap trackers;
static void addTracker(StatTracker *newTracker)
{
	std::pair<string, string> s = std::make_pair(newTracker->category, newTracker->name);
	if (trackers.find(s) != trackers.end()) {
		newTracker->ptra = trackers[s]->ptra;
		newTracker->ptrb = trackers[s]->ptrb;
		return;
	}
	trackers[s] = newTracker;
}
static void StatsPrintVal(ostream &f, StatsCounterType v);
static void StatsPrintVal(ostream &f, StatsCounterType v1, StatsCounterType v2);
// Statistics Functions
StatTracker::StatTracker(const string &cat, const string &n,
	StatsCounterType *pa, StatsCounterType *pb, bool p)
{
	category = cat;
	name = n;
	ptra = pa;
	ptrb = pb;
	percentage = p;
}
StatsCounter::StatsCounter(const string &category, const string &name)
{
	num = 0;
	addTracker(new StatTracker(category, name, &num));
}
StatsRatio::StatsRatio(const string &category, const string &name)
{
	na = nb = 0;
	addTracker(new StatTracker(category, name, &na, &nb, false));
}
StatsPercentage::StatsPercentage(const string &category, const string &name)
{
	na = nb = 0;
	addTracker(new StatTracker(category, name, &na, &nb, true));
}

void StatsPrint(ostream &dest)
{
	TrackerMap::iterator iter = trackers.begin();
    if(iter != trackers.end())
        dest << "Statistics:" << endl;

	string lastCategory;
	while (iter != trackers.end()) {
		// Print statistic
		StatTracker *tr = iter->second;
		if (tr->category != lastCategory) {
			dest << tr->category.c_str() << endl;
			lastCategory = tr->category;
		}
		dest << "    " << setw(56) << left << tr->name;
		if (tr->ptrb == NULL)
			StatsPrintVal(dest, *tr->ptra);
		else {
			StatsPrintVal(dest, *tr->ptra, *tr->ptrb);
			if (*tr->ptrb > 0) {
				const float ratio = static_cast<float>(*tr->ptra) /
					static_cast<float>(*tr->ptrb);
				dest << setprecision(2);
				if (tr->percentage)
					dest << " (" << setw(3) << (100.f * ratio) << "%)";
				else
					dest << " (" << ratio << "x)";
			}
		}
		dest << endl;
		++iter;
	}
}
static void StatsPrintVal(ostream &f, StatsCounterType v)
{
	if (v > 1e9f)
		f << setprecision(3) << (v * 1e-9f) << "B";
	else if (v > 1e6f)
		f << setprecision(3) << (v * 1e-6f) << "M";
	else if (v > 1e4f)
		f << setprecision(1) << (v * 1e-3f) << "k";
	else
		f << setprecision(0) << v;
}
static void StatsPrintVal(ostream &f, StatsCounterType v1, StatsCounterType v2)
{
	StatsCounterType m = min(v1, v2);
	if (m > 1e9f)
		f << setprecision(3) << (v1 * 1e-9f) << "B:" << (v2 * 1e-9f) << "B";
	else if (m > 1e6f)
		f << setprecision(3) << (v1 * 1e-6f) << "M:" << (v2 * 1e-6f) << "M";
	else if (m > 1e4f)
		f << setprecision(1) << (v1 * 1e-3f) << "k:" << (v2 * 1e-3f) << "k";
	else
		f << setprecision(0) << v1 << ":" << v2;
}
void StatsCleanup() {
	TrackerMap::iterator iter = trackers.begin();
	string lastCategory;
	while (iter != trackers.end()) {
		delete iter->second;
		++iter;
	}
	trackers.erase(trackers.begin(), trackers.end());
}

// ProgressReporter Method Definitions
ProgressReporter::ProgressReporter(u_int totalWork, const string &title_, u_int bar_length)
	: totalPlusses(bar_length - title.size()), title(title_) {
	plussesPrinted = 0;
	frequency = static_cast<float>(totalWork) / static_cast<float>(totalPlusses);
	count = frequency;
	timer = new Timer();
	timer->Start();
	// Initialize progress string
	cout << "\r" << title << ": [" << string(totalPlusses, ' ') << "]" << flush;
}
ProgressReporter::~ProgressReporter() { delete timer; }
void ProgressReporter::Update(u_int num) const {
	count -= static_cast<float>(num);
	bool updatedAny = false;
	while (count <= 0.f && plussesPrinted < totalPlusses) {
		count += frequency;
		++plussesPrinted;
		updatedAny = true;
	}
	if (updatedAny) {
		cout << "\r" << title << ": [";
		if (plussesPrinted > 0)
			cout << string(plussesPrinted, '+');
		if (plussesPrinted < totalPlusses)
			cout << string(totalPlusses - plussesPrinted, ' ');
		cout << "] (";
		// Update elapsed time and estimated time to completion
		const float percentDone = static_cast<float>(plussesPrinted) /
			static_cast<float>(totalPlusses);
		const float seconds = static_cast<float>(timer->Time());
		const float estRemaining = seconds / percentDone - seconds;
		cout << setprecision(1) << seconds;
		if (percentDone == 1.f)
			cout << "s)";
		else
			cout << "s|" << estRemaining << "s)  ";
		cout << flush;
	}
}
void ProgressReporter::Done() const {
	timer->Stop();
	const float seconds = static_cast<float>(timer->Time());
	cout << "\r" << title << ": [" << string(totalPlusses, '+') << "] (" << setprecision(1) << seconds << "s)       " << flush;
}

/* string hashing function
 * An algorithm produced by Professor Daniel J. Bernstein and shown first to the world on the usenet newsgroup comp.lang.c. It is one of the most efficient hash functions ever published.
 */
unsigned int DJBHash(const std::string& str)
{
   unsigned int hash = 5381;

   for(std::size_t i = 0; i < str.length(); i++)
   {
      hash = ((hash << 5) + hash) + static_cast<unsigned int>(str[i]);
   }

   return hash;
}

//error.h nullstream
struct nullstream : std::ostream
{
	nullstream(): std::ios(0), std::ostream(0) {}
} nullStream;

}


