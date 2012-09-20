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

#ifndef LUX_SAMPLERTBBRENDERER_H
#define LUX_SAMPLERTBBRENDERER_H

#include <vector>
#include <boost/thread.hpp>

#include "lux.h"
#include "renderer.h"
#include "fastmutex.h"
#include "timer.h"
#include "dynload.h"
#include <tbb/tbb.h>

namespace lux
{

class SamplerTBBRenderer;
class SRTBBHostDescription;

//------------------------------------------------------------------------------
// SRTBBDeviceDescription
//------------------------------------------------------------------------------

class SRTBBDeviceDescription : protected RendererDeviceDescription {
public:
	const string &GetName() const { return name; }

	unsigned int GetAvailableUnitsCount() const {
		return 1u; // not implemented with TBB
	}
	unsigned int GetUsedUnitsCount() const;
	void SetUsedUnitsCount(const unsigned int units);

	friend class SamplerTBBRenderer;
	friend class SRTBBHostDescription;

private:
	SRTBBDeviceDescription(SRTBBHostDescription *h, const string &n) :
		host(h), name(n) { }
	~SRTBBDeviceDescription() { }

	SRTBBHostDescription *host;
	string name;
};

//------------------------------------------------------------------------------
// SRTBBHostDescription
//------------------------------------------------------------------------------

class SRTBBHostDescription : protected RendererHostDescription {
public:
	const string &GetName() const { return name; }

	vector<RendererDeviceDescription *> &GetDeviceDescs() { return devs; }

	friend class SamplerTBBRenderer;
	friend class SRTBBDeviceDescription;

private:
	SRTBBHostDescription(SamplerTBBRenderer *r, const string &n);
	~SRTBBHostDescription();

	SamplerTBBRenderer *renderer;
	string name;
	vector<RendererDeviceDescription *> devs;
};

//------------------------------------------------------------------------------
// SamplerTBBRenderer
//------------------------------------------------------------------------------

class SamplerTBBRenderer : public Renderer {
public:
	SamplerTBBRenderer();
	~SamplerTBBRenderer();

	RendererType GetType() const;

	RendererState GetState() const;
	vector<RendererHostDescription *> &GetHostDescs();
	void SuspendWhenDone(bool v);

	void Render(Scene *scene);

	void Pause();
	void Resume();
	void Terminate();

	friend class SRTBBDeviceDescription;
	friend class SRTBBHostDescription;
	friend class SRTBBStatistics;

	static Renderer *CreateRenderer(const ParamSet &params);

	struct Impl
	{
		SamplerTBBRenderer *renderer;
		Impl(SamplerTBBRenderer *renderer_): renderer(renderer_) {}
		void operator()(unsigned int i, tbb::parallel_do_feeder<unsigned int>& feeder) const;
	};

private:
	mutable boost::mutex classWideMutex;

	RendererState state;
	vector<RendererHostDescription *> hosts;
	Scene *scene;

	fast_mutex sampPosMutex;
	u_int sampPos;
	unsigned int numberOfThreads;
	bool mustChangeNumberOfThreads;

	// Put them last for better data alignment
	// used to suspend render threads until the preprocessing phase is done
	bool preprocessDone;
	bool suspendThreadsWhenDone;

	struct LocalStorage
	{
		Sample *sample;

		double samples, blackSamples, blackSamplePaths;

		~LocalStorage();
	};

	static LocalStorage LocalStorageCreate(Scene *scene);


	// For local storage
	typedef tbb::enumerable_thread_specific<LocalStorage> LocalStoragePool;

	mutable LocalStoragePool *localStoragePool;
};

}//namespace lux

#endif // LUX_SAMPLERRENDERER_H
