/***************************************************************************
 *   Copyright (C) 1998-2013 by authors (see AUTHORS.txt)                  *
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

#include "luxrays/luxrays.h"
#include "luxrays/core/device.h"
#include "luxrays/core/intersectiondevice.h"

#include "lux.h"
#include "renderer.h"
#include "fastmutex.h"
#include "timer.h"
#include "dynload.h"
#include "primitive.h"

namespace lux
{

class HybridRenderer;
class HybridSamplerRenderer;
class HybridSPPMRenderer;
class HRHostDescription;

//------------------------------------------------------------------------------
// HRDeviceDescription
//------------------------------------------------------------------------------

class HRDeviceDescription : protected RendererDeviceDescription {
public:
	const string &GetName() const { return name; }

	friend class HybridRenderer;
	friend class HybridSamplerRenderer;
	friend class HybridSPPMRenderer;
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
	friend class HybridSPPMRenderer;
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
	friend class HybridSPPMRenderer;
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
	friend class HybridSPPMRenderer;
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

// A dummy primitive used for instance support
class HybridInstancePrimitive : public Primitive {
public:
	HybridInstancePrimitive(const InstancePrimitive *instPrim, const Primitive *basePrim) : instance(instPrim), base(basePrim) { }
	~HybridInstancePrimitive() { }

	BBox WorldBound() const {
		throw std::runtime_error("Internal error: called HybridInstancePrimitive::WorldBound().");
	}
	bool CanIntersect() const {
		throw std::runtime_error("Internal error: called HybridInstancePrimitive::CanIntersect().");
	}
	bool CanSample() const {
		throw std::runtime_error("Internal error: called HybridInstancePrimitive::CanSample().");
	}
	void GetIntersection(const luxrays::RayHit &rayHit, const u_int index, Intersection *in) const {
		base->GetIntersection(rayHit, index, in);

		const Transform &InstanceToWorld = instance->GetTransform();
		in->ObjectToWorld = InstanceToWorld * in->ObjectToWorld;
		// Transform instance's differential geometry to world space
		in->dg *= InstanceToWorld;
		in->dg.handle = instance;
		in->primitive = instance;
		if (instance->GetMaterial())
			in->material = instance->GetMaterial();
		if (instance->GetExterior())
			in->exterior = instance->GetExterior();
		if (instance->GetInterior())
			in->interior = instance->GetInterior();
	}

	virtual Transform GetLocalToWorld(float time) const  {
		throw std::runtime_error("Internal error: called HybridInstancePrimitive::GetLocalToWorld().");
	}

private:
	const InstancePrimitive *instance;
	const Primitive *base;
};

class HybridRenderer : public Renderer {
public:
	static luxrays::DataSet *PreprocessGeometry(luxrays::Context *ctx, Scene *scene,
			// Used later to free allocated memory
			vector<HybridInstancePrimitive *> &hybridPrims);

	static void LoadCfgParams(const string &configFile, ParamSet *params);
protected:
	HybridRenderer() : Renderer() { }

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

extern void LuxRaysDebugHandler(const char *msg);

}//namespace lux

#endif // LUX_HYBRIDRENDERER_H
