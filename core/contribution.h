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

#ifndef LUX_CONTRIBUTION_H
#define LUX_CONTRIBUTION_H
// contribution.h*
#include "lux.h"
#include "memory.h"
#include "color.h"
#include "fastmutex.h"

#include <boost/thread/mutex.hpp>

namespace lux
{

// Size of a contribution buffer
// 4096 seems best.
#define CONTRIB_BUF_SIZE 4096

// Minimum number of buffers to keep alive/reuse
// In practice twice this amount stays allocated
#define CONTRIB_BUF_KEEPALIVE 8

// Switch on to get feedback in the log about allocation
#define CONTRIB_DEBUG false

class Contribution {
public:
	Contribution(float x=0.f, float y=0.f, const XYZColor &c=0.f, float a=0.f, float zd=0.f,
		float v=0.f, u_int b=0, u_int g=0) :
		imageX(x), imageY(y), color(c), alpha(a), zdepth(zd), variance(v),
		buffer(b), bufferGroup(g) { }

	float imageX, imageY;
	XYZColor color;
	float alpha, zdepth, variance;
	u_int buffer, bufferGroup;
};

class ContributionBuffer {
	class Buffer {
	public:
		Buffer() : pos(0) {
			contribs = AllocAligned<Contribution>(CONTRIB_BUF_SIZE);
		}

		~Buffer() {
			FreeAligned(contribs);
		}

		bool Add(Contribution* c, float weight=1.f) {
			if(pos == CONTRIB_BUF_SIZE)
				return false;

			contribs[pos] = *c;
			contribs[pos].variance = weight;

			++pos;

			return true;
		}

		void Splat(Film *film);

	private:
		u_int pos;
		Contribution *contribs;
	};
public:
	ContributionBuffer() : sampleCount(0.f), buffers(0) { }

	~ContributionBuffer() {
		for (u_int i = 0; i < buffers.size(); ++i) {
			for (u_int j = 0; j < buffers[i].size(); ++j)
				delete buffers[i][j];
		}
	}

	bool Add(Contribution* c, float weight=1.f) {
		while (c->bufferGroup >= buffers.size())
			buffers.push_back(vector<Buffer *>(0));
		while (c->buffer >= buffers[c->bufferGroup].size())
			buffers[c->bufferGroup].push_back(new Buffer());
		return buffers[c->bufferGroup][c->buffer]->Add(c, weight);
	}

	void AddSampleCount(float c) {
		sampleCount += c;
	}

	void Splat(Film *film);

private:
	float sampleCount;
	vector<vector<Buffer *> > buffers;
};

class ContributionPool {
public:

	ContributionPool();

	void SetFilm(Film *f) { film = f; }

	void End(ContributionBuffer *c);

	ContributionBuffer* Next(ContributionBuffer *c);

	// Flush() and Delete() are not thread safe,
	// they can only be called by Scene after rendering is finished.
	void Flush();
	void Delete();

private:
	unsigned int total;
	vector<ContributionBuffer*> CFree; // Emptied/available buffers
	vector<ContributionBuffer*> CFull; // Full buffers
	vector<ContributionBuffer*> CSplat; // Buffers being splat

	Film *film;
	fast_mutex poolMutex;
	boost::mutex splattingMutex;
};

}//namespace lux

#endif // LUX_CONTRIBUTION_H
