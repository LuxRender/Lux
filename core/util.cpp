/***************************************************************************
 *   Copyright (C) 1998-2008 by authors (see AUTHORS.txt )                 *
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
// Error Reporting Includes
#include <cstdarg>


using namespace lux;

#if 0
// Error Reporting Definitions
#define LUX_ERROR_IGNORE 0
#define LUX_ERROR_CONTINUE 1
#define LUX_ERROR_ABORT 2
// Error Reporting Functions
static void processError(const char *format, va_list args,
		const char *message, int disposition) {
#ifndef WIN32
	char *errorBuf;
	vasprintf(&errorBuf, format, args);
#else
	char errorBuf[2048];
	_vsnprintf(errorBuf, sizeof(errorBuf), format, args);
#endif
	// Report error
	switch (disposition) {
	case LUX_ERROR_IGNORE:
		return;
	case LUX_ERROR_CONTINUE:
		fprintf(stderr, "%s: %s\n", message, errorBuf);
		// Print scene file and line number, if appropriate
		extern int line_num;
		if (line_num != 0) {
			extern string current_file;
			fprintf(stderr, "\tLine %d, file %s\n", line_num,
				current_file.c_str());
		}
		break;
	case LUX_ERROR_ABORT:
		fprintf(stderr, "%s: %s\n", message, errorBuf);
		// Print scene file and line number, if appropriate
		extern int line_num;
		if (line_num != 0) {
			extern string current_file;
			fprintf(stderr, "\tLine %d, file %s\n", line_num,
				current_file.c_str());
		}
		exit(1);
	}
#ifndef WIN32
	free(errorBuf);
#endif
}
#endif

namespace lux
{
#if 0
 void Info(const char *format, ...) {
	va_list args;
	va_start(args, format);
	processError(format, args, "Notice", LUX_ERROR_CONTINUE);
	va_end(args);
}
 void Warning(const char *format, ...) {
	va_list args;
	va_start(args, format);
	processError(format, args, "Warning", LUX_ERROR_CONTINUE);
	va_end(args);
}
 void Error(const char *format, ...) {
	va_list args;
	va_start(args, format);
	processError(format, args, "Error", LUX_ERROR_CONTINUE);
	va_end(args);
}
 void Severe(const char *format, ...) {
	va_list args;
	va_start(args, format);
	processError(format, args, "Fatal Error", LUX_ERROR_ABORT);
	va_end(args);
}
#endif
// Matrix Method Definitions
 bool SolveLinearSystem2x2(const float A[2][2],
		const float B[2], float x[2]) {
	float det = A[0][0]*A[1][1] - A[0][1]*A[1][0];
	if (fabsf(det) < 1e-5)
		return false;
	float invDet = 1.0f/det;
	x[0] = (A[1][1]*B[0] - A[0][1]*B[1]) * invDet;
	x[1] = (A[0][0]*B[1] - A[1][0]*B[0]) * invDet;
	return true;
}

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
static void addTracker(StatTracker *newTracker) {
	std::pair<string, string> s = std::make_pair(newTracker->category, newTracker->name);
	if (trackers.find(s) != trackers.end()) {
		newTracker->ptra = trackers[s]->ptra;
		newTracker->ptrb = trackers[s]->ptrb;
		return;
	}
	trackers[s] = newTracker;
}
namespace lux {
static void StatsPrintVal(FILE *f, StatsCounterType v);
static void StatsPrintVal(FILE *f, StatsCounterType v1, StatsCounterType v2);
}
// Statistics Functions
StatTracker::StatTracker(const string &cat, const string &n,
                         StatsCounterType *pa, StatsCounterType *pb, bool p) {
	category = cat;
	name = n;
	ptra = pa;
	ptrb = pb;
	percentage = p;
}
StatsCounter::StatsCounter(const string &category, const string &name) {
	num = 0;
	addTracker(new StatTracker(category, name, &num));
}
StatsRatio::StatsRatio(const string &category, const string &name) {
	na = nb = 0;
	addTracker(new StatTracker(category, name, &na, &nb, false));
}
StatsPercentage::StatsPercentage(const string &category, const string &name) {
	na = nb = 0;
	addTracker(new StatTracker(category, name, &na, &nb, true));
}

namespace lux {

void StatsPrint(FILE *dest) {
	TrackerMap::iterator iter = trackers.begin();
    if(iter != trackers.end())
        fprintf(dest, "Statistics:\n");

	string lastCategory;
	while (iter != trackers.end()) {
		// Print statistic
		StatTracker *tr = iter->second;
		if (tr->category != lastCategory) {
			fprintf(dest, "%s\n", tr->category.c_str());
			lastCategory = tr->category;
		}
		fprintf(dest, "    %s", tr->name.c_str());
		// Pad out to results column
		int resultsColumn = 56;
		int paddingSpaces = resultsColumn - (int) tr->name.size();
		while (paddingSpaces-- > 0)
			putc(' ', dest);
		if (tr->ptrb == NULL)
			StatsPrintVal(dest, *tr->ptra);
		else {
			if (*tr->ptrb > 0) {
				float ratio = (float)*tr->ptra / (float)*tr->ptrb;
				StatsPrintVal(dest, *tr->ptra, *tr->ptrb);
				if (tr->percentage)
					fprintf(dest, " (%3.2f%%)", 100. * ratio);
				else
					fprintf(dest, " (%.2fx)", ratio);
			}
			else
				StatsPrintVal(dest, *tr->ptra, *tr->ptrb);
		}
		fprintf(dest, "\n");
		++iter;
	}
}
static void StatsPrintVal(FILE *f, StatsCounterType v) {
	if (v > 1e9) fprintf(f, "%.3fB", v / 1e9f);
	else if (v > 1e6) fprintf(f, "%.3fM", v / 1e6f);
	else if (v > 1e4) fprintf(f, "%.1fk", v / 1e3f);
	else fprintf(f, "%.0f", (float)v);
}
static void StatsPrintVal(FILE *f, StatsCounterType v1,
		StatsCounterType v2) {
	StatsCounterType m = min(v1, v2);
	if (m > 1e9) fprintf(f, "%.3fB:%.3fB", v1 / 1e9f, v2 / 1e9f);
	else if (m > 1e6) fprintf(f, "%.3fM:%.3fM", v1 / 1e6f, v2 / 1e6f);
	else if (m > 1e4) fprintf(f, "%.1fk:%.1fk", v1 / 1e3f, v2 / 1e3f);
	else fprintf(f, "%.0f:%.0f", v1, v2);
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

}

// ProgressReporter Method Definitions
ProgressReporter::ProgressReporter(int totalWork, const string &title, int bar_length)
	: totalPlusses(bar_length - title.size()) {
	plussesPrinted = 0;
	frequency = (float)totalWork / (float)totalPlusses;
	count = frequency;
	timer = new Timer();
	timer->Start();
	outFile = stdout;
	// Initialize progress string
	const int bufLen = title.size() + totalPlusses + 64;
	buf = new char[bufLen];
	snprintf(buf, bufLen, "\r%s: [", title.c_str());
	curSpace = buf + strlen(buf);
	char *s = curSpace;
	for (int i = 0; i < totalPlusses; ++i)
		*s++ = ' ';
	*s++ = ']';
	*s++ = ' ';
	*s++ = '\0';
	fprintf(outFile, buf);
	fflush(outFile);
}
ProgressReporter::~ProgressReporter() { delete[] buf; delete timer; }
void ProgressReporter::Update(int num) const {
	count -= num;
	bool updatedAny = false;
	while (count <= 0) {
		count += frequency;
		if (plussesPrinted++ < totalPlusses)
			*curSpace++ = '+';
		updatedAny = true;
	}
	if (updatedAny) {
		fputs(buf, outFile);
		// Update elapsed time and estimated time to completion
		float percentDone = (float)plussesPrinted / (float)totalPlusses;
		float seconds = (float) timer->Time();
		float estRemaining = seconds / percentDone - seconds;
		if (percentDone == 1.f)
			fprintf(outFile, " (%.1fs)       ", seconds);
		else
			fprintf(outFile, " (%.1fs|%.1fs)  ", seconds, max(0.f, estRemaining));
		fflush(outFile);
	}
}
void ProgressReporter::Done() const {
	while (plussesPrinted++ < totalPlusses)
		*curSpace++ = '+';
	fputs(buf, outFile);
	float seconds = (float) timer->Time();
	fprintf(outFile, " (%.1fs)       \n", seconds);
	fflush(outFile);
}

namespace lux
{
/* string hashing function
 * An algorithm produced by Professor Daniel J. Bernstein and shown first to the world on the usenet newsgroup comp.lang.c. It is one of the most efficient hash functions ever published.
 */
unsigned int DJBHash(const std::string& str)
{
   unsigned int hash = 5381;

   for(std::size_t i = 0; i < str.length(); i++)
   {
      hash = ((hash << 5) + hash) + str[i];
   }

   return hash;
}
}
