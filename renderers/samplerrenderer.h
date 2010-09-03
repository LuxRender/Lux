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

#ifndef LUX_SAMPLERRENDERER_H
#define LUX_SAMPLERRENDERER_H

#include <vector>
#include <boost/thread.hpp>

#include "lux.h"
#include "renderer.h"
#include "fastmutex.h"
#include "timer.h"
#include "dynload.h"

namespace lux
{

class SamplerRenderer;
class SRHostDescription;

//------------------------------------------------------------------------------
// SRDeviceDescription
//------------------------------------------------------------------------------

class SRDeviceDescription : protected RendererDeviceDescription {
public:
	const string &GetName() const { return name; }

	unsigned int GetAvailableUnitsCount() const {
		return max(boost::thread::hardware_concurrency(), 1u);
	}
	unsigned int GetUsedUnitsCount() const;
	void SetUsedUnitsCount(const unsigned int units);

	friend class SamplerRenderer;
	friend class SRHostDescription;

private:
	SRDeviceDescription(SRHostDescription *h, const string &n) :
		host(h), name(n) { }
	~SRDeviceDescription() { }

	SRHostDescription *host;
	string name;
};

//------------------------------------------------------------------------------
// SRHostDescription
//------------------------------------------------------------------------------

class SRHostDescription : protected RendererHostDescription {
public:
	const string &GetName() const { return name; }

	vector<RendererDeviceDescription *> &GetDeviceDescs() { return devs; }

	friend class SamplerRenderer;
	friend class SRDeviceDescription;

private:
	SRHostDescription(SamplerRenderer *r, const string &n);
	~SRHostDescription();

	SamplerRenderer *renderer;
	string name;
	vector<RendererDeviceDescription *> devs;
};

//------------------------------------------------------------------------------
// SamplerRenderer
//------------------------------------------------------------------------------

class SamplerRenderer : public Renderer {
public:
	SamplerRenderer();
	~SamplerRenderer();

	RendererType GetType() const;

	RendererState GetState() const;
	vector<RendererHostDescription *> &GetHostDescs();
	void SuspendWhenDone(bool v);

	double Statistics(const string &statName);

	void Render(Scene *scene);

	void Pause();
	void Resume();
	void Terminate();

	friend class SRDeviceDescription;
	friend class SRHostDescription;

	static Renderer *CreateRenderer(const ParamSet &params);

private:
	//--------------------------------------------------------------------------
	// RenderThread
	//--------------------------------------------------------------------------

	class RenderThread : public boost::noncopyable {
	public:
		RenderThread(u_int index, SamplerRenderer *renderer);
		~RenderThread();

		static void RenderImpl(RenderThread *r);

		u_int  n;
		SamplerRenderer *renderer;
		boost::thread *thread; // keep pointer to delete the thread object
		double samples, blackSamples;
		fast_mutex statLock;
	};

	void CreateRenderThread();
	void RemoveRenderThread();

	double Statistics_GetNumberOfSamples();
	double Statistics_SamplesPSec();
	double Statistics_SamplesPTotSec();
	double Statistics_Efficiency();
	double Statistics_SamplesPPx();

	//--------------------------------------------------------------------------

	mutable boost::mutex classWideMutex;

	RendererState state;
	vector<RendererHostDescription *> hosts;
	vector<RenderThread *> renderThreads;
	Scene *scene;

	fast_mutex sampPosMutex;
	u_int sampPos;

	Timer s_Timer;
	double lastSamples, lastTime;
	double stat_Samples, stat_blackSamples;

	// Put them last for better data alignment
	// used to suspend render threads until the preprocessing phase is done
	bool preprocessDone;
	bool suspendThreadsWhenDone;
};

}//namespace lux

#endif // LUX_SAMPLERRENDERER_H
