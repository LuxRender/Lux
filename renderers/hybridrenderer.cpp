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

#include <iomanip>

#include "api.h"
#include "scene.h"
#include "camera.h"
#include "film.h"
#include "sampling.h"
#include "hybridrenderer.h"
#include "randomgen.h"
#include "context.h"

#include "luxrays/core/context.h"
#include "luxrays/core/virtualdevice.h"

using namespace lux;

void lux::LuxRaysDebugHandler(const char *msg) {
	LOG(LUX_DEBUG, LUX_NOERROR) << "[LuxRays] " << msg;
}

//------------------------------------------------------------------------------
// HRDeviceDescription
//------------------------------------------------------------------------------

HRHardwareDeviceDescription::HRHardwareDeviceDescription(HRHostDescription *h, luxrays::DeviceDescription *desc) :
	HRDeviceDescription(h, desc->GetName()) {
	devDesc = desc;
	enabled = true;
}

void HRHardwareDeviceDescription::SetUsedUnitsCount(const unsigned int units) {
	// I assume there is only one single virtual device in use
	if ((units < 0) || (units > 1))
		throw std::runtime_error("A not valid amount of units used in HRDeviceDescription::SetUsedUnitsCount()");

	enabled = (units == 1);
}

HRVirtualDeviceDescription::HRVirtualDeviceDescription(HRHostDescription *h, const string &name) :
	HRDeviceDescription(h, name) {
}

unsigned int HRVirtualDeviceDescription::GetUsedUnitsCount() const {
	// I assume there is only one single virtual device in use
	boost::mutex::scoped_lock lock(host->renderer->classWideMutex);
	return host->renderer->GetRenderThreadCount();
}

void HRVirtualDeviceDescription::SetUsedUnitsCount(const unsigned int units) {
	// I assume there is only one single virtual device in use
	boost::mutex::scoped_lock lock(host->renderer->classWideMutex);

	unsigned int target = max(units, 1u);
	size_t current = host->renderer->GetRenderThreadCount();

	if (current > target) {
		for (unsigned int i = 0; i < current - target; ++i)
			host->renderer->RemoveRenderThread();
	} else if (current < target) {
		for (unsigned int i = 0; i < target - current; ++i)
			host->renderer->CreateRenderThread();
	}
}

//------------------------------------------------------------------------------
// HRHostDescription
//------------------------------------------------------------------------------

HRHostDescription::HRHostDescription(HybridRenderer *r, const string &n) : renderer(r), name(n) {
}

HRHostDescription::~HRHostDescription() {
	for (size_t i = 0; i < devs.size(); ++i)
		delete devs[i];
}

void HRHostDescription::AddDevice(HRDeviceDescription *devDesc) {
	devs.push_back(devDesc);
}

//------------------------------------------------------------------------------

luxrays::DataSet *HybridRenderer::PreprocessGeometry(luxrays::Context *ctx, Scene *scene) {
	// Compile the scene geometries in a LuxRays compatible format

	vector<luxrays::TriangleMesh *> meshList;

	LOG(LUX_INFO,LUX_NOERROR) << "Tesselating " << scene->primitives.size() << " primitives";

	for (size_t i = 0; i < scene->primitives.size(); ++i)
		scene->primitives[i]->Tesselate(&meshList, &scene->tesselatedPrimitives);

	// Create the DataSet
	luxrays::DataSet *dataSet = new luxrays::DataSet(ctx);

	// Add all mesh
	for (std::vector<luxrays::TriangleMesh *>::const_iterator obj = meshList.begin(); obj != meshList.end(); ++obj)
		dataSet->Add(*obj);

	dataSet->Preprocess();
	scene->dataSet = dataSet;
	ctx->SetDataSet(dataSet);

	// I can free temporary data
	for (std::vector<luxrays::TriangleMesh *>::const_iterator obj = meshList.begin(); obj != meshList.end(); ++obj)
		delete *obj;

	return dataSet;
}
