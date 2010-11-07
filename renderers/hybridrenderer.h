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

#ifndef LUX_HYBRIDRENDERER_H
#define LUX_HYBRIDRENDERER_H

#include <vector>
#include <boost/thread.hpp>

#include "lux.h"
#include "renderer.h"
#include "fastmutex.h"
#include "timer.h"
#include "dynload.h"

#include "luxrays/luxrays.h"
#include "luxrays/core/device.h"
#include "luxrays/core/intersectiondevice.h"

namespace lux
{

class HybridRenderer;
class HybridSamplerRenderer;
class HRHostDescription;

//------------------------------------------------------------------------------
// HRDeviceDescription
//------------------------------------------------------------------------------

class HRDeviceDescription : protected RendererDeviceDescription {
public:
	const string &GetName() const { return name; }

	friend class HybridRenderer;
	friend class HybridSamplerRenderer;
	friend class HRHostDescription;

protected:
	HRDeviceDescription(HRHostDescription *h, const string &n) : host(h), name(n) { }

	HRHostDescription *host;
	string name;
};

class HRHardwareDeviceDescription : protected HRDeviceDescription {
public:
	unsigned int GetAvailableUnitsCount() const { return 1;	}
	unsigned int GetUsedUnitsCount() const { return enabled ? 1 : 0; }
	void SetUsedUnitsCount(const unsigned int units);

	friend class HybridRenderer;
	friend class HybridSamplerRenderer;
	friend class HRHostDescription;

private:
	HRHardwareDeviceDescription(HRHostDescription *h, luxrays::DeviceDescription *desc);
	~HRHardwareDeviceDescription() { }

	luxrays::DeviceDescription *devDesc;
	bool enabled;
};

class HRVirtualDeviceDescription : protected HRDeviceDescription {
public:
	unsigned int GetAvailableUnitsCount() const {
		return max(boost::thread::hardware_concurrency(), 1u);
	}
	unsigned int GetUsedUnitsCount() const;
	void SetUsedUnitsCount(const unsigned int units);

	friend class HybridRenderer;
	friend class HybridSamplerRenderer;
	friend class HRHostDescription;

private:
	HRVirtualDeviceDescription(HRHostDescription *h, const string &n);
	~HRVirtualDeviceDescription() { }
};

//------------------------------------------------------------------------------
// HRHostDescription
//------------------------------------------------------------------------------

class HRHostDescription : protected RendererHostDescription {
public:
	const string &GetName() const { return name; }

	vector<RendererDeviceDescription *> &GetDeviceDescs() { return devs; }

	friend class HybridRenderer;
	friend class HybridSamplerRenderer;
	friend class HRDeviceDescription;
	friend class HRHardwareDeviceDescription;
	friend class HRVirtualDeviceDescription;

private:
	HRHostDescription(HybridRenderer *r, const string &n);
	~HRHostDescription();

	void AddDevice(HRDeviceDescription *devDesc);

	HybridRenderer *renderer;
	string name;
	vector<RendererDeviceDescription *> devs;
};

//------------------------------------------------------------------------------
// HybridRenderer
//------------------------------------------------------------------------------

class HybridRenderer : public Renderer {
protected:
	virtual void CreateRenderThread() = 0;
	virtual void RemoveRenderThread() = 0;
	virtual size_t GetRenderThreadCount() const  = 0;

	mutable boost::mutex classWideMutex;
	vector<RendererHostDescription *> hosts;

	friend class HRDeviceDescription;
	friend class HRHardwareDeviceDescription;
	friend class HRVirtualDeviceDescription;
	friend class HRHostDescription;
};

}//namespace lux

#endif // LUX_HYBRIDRENDERER_H
