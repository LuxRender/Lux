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

#include <boost/lexical_cast.hpp>
#include <boost/regex.hpp>
#include <fstream>

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
	if (units > 1)
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

static void PreprocessPrimitive(const Primitive *prim, Scene *scene,
		vector<luxrays::TriangleMesh *> *meshList, vector<const Primitive *> *primitiveList) {
	//LOG(LUX_DEBUG, LUX_NOERROR) << "Define primitive type: " << typeid(*prim).name();

	prim->Tesselate(meshList, primitiveList);
}

luxrays::DataSet *HybridRenderer::PreprocessGeometry(luxrays::Context *ctx, Scene *scene,
			vector<HybridInstancePrimitive *> &hybridPrims) {
	// Compile the scene geometries in a LuxRays compatible format

	LOG(LUX_INFO,LUX_NOERROR) << "Tesselating " << scene->primitives.size() << " primitives";

	vector<luxrays::Mesh *> meshList;
	// To keep track of all primitive mesh lists
	map<const Primitive *, vector<luxrays::TriangleMesh *> > primMeshLists;
	map<const Primitive *, vector<const Primitive *> > primTesselatedLists;
	// To keep track of the allocated instances
	vector<luxrays::InstanceTriangleMesh *> instList;

	for (size_t i = 0; i < scene->primitives.size(); ++i) {
		const Primitive *prim = scene->primitives[i].get();
		//LOG(LUX_DEBUG, LUX_NOERROR) << "Primitive type: " << typeid(*prim).name();

		// Instances require special care
		if (dynamic_cast<const InstancePrimitive *>(prim)) {
			const InstancePrimitive *instance = dynamic_cast<const InstancePrimitive *>(prim);

			const vector<boost::shared_ptr<Primitive> > &instanceSources = instance->GetInstanceSources();

			for (u_int i = 0; i < instanceSources.size(); ++i) {
				const Primitive *instancedSource = instanceSources[i].get();

				vector<luxrays::TriangleMesh *> primMeshList;
				vector<const Primitive *> primTesselatedList;
				// Check if I have already defined one of the original primitive
				if (primMeshLists.count(instancedSource) < 1) {
					// I have to define the instanced primitive
					PreprocessPrimitive(instancedSource, scene, &primMeshList, &primTesselatedList);
					primMeshLists[instancedSource] = primMeshList;
					primTesselatedLists[instancedSource] = primTesselatedList;
				} else {
					primMeshList = primMeshLists[instancedSource];
					primTesselatedList = primTesselatedLists[instancedSource];
				}

				if (primMeshList.size() == 0)
					continue;

				const Transform trans = instance->GetTransform();
				for (u_int i = 0; i < primMeshList.size(); ++i) {
					luxrays::InstanceTriangleMesh *itm = new luxrays::InstanceTriangleMesh(primMeshList[i], trans);
					meshList.push_back(itm);
					instList.push_back(itm);

					HybridInstancePrimitive *hip = new HybridInstancePrimitive(instance, primTesselatedList[i]);
					scene->tesselatedPrimitives.push_back(hip);
					hybridPrims.push_back(hip);
				}
			}
		} else {
			vector<luxrays::TriangleMesh *> primMeshList;
			vector<const Primitive *> primTesselatedList;
			// Check if I have already defined one of the original primitive
			if (primMeshLists.count(prim) < 1) {
				// I have to define the instanced primitive
				PreprocessPrimitive(prim, scene, &primMeshList, &primTesselatedList);
				primMeshLists[prim] = primMeshList;
				primTesselatedLists[prim] = primTesselatedList;
			} else {
				primMeshList = primMeshLists[prim];
				primTesselatedList = primTesselatedLists[prim];
			}

			if (primMeshList.size() == 0)
				continue;

			for (u_int i = 0; i < primMeshList.size(); ++i) {
				meshList.push_back(primMeshList[i]);
				scene->tesselatedPrimitives.push_back(primTesselatedList[i]);
			}
		}
	}

	if (meshList.empty())
		return NULL;

	// Create the DataSet
	luxrays::DataSet *dataSet = new luxrays::DataSet(ctx);

	// Add all mesh
	for (std::vector<luxrays::Mesh *>::const_iterator mesh = meshList.begin(); mesh != meshList.end(); ++mesh)
		dataSet->Add(*mesh);

	dataSet->Preprocess();
	scene->dataSet = dataSet;
	ctx->SetDataSet(dataSet);

	// I can free temporary data
	for (std::vector<luxrays::InstanceTriangleMesh *>::const_iterator it = instList.begin(); it != instList.end(); ++it)
		delete *it;
	for (std::map<const Primitive *, vector<luxrays::TriangleMesh *> >::const_iterator it = primMeshLists.begin(); it != primMeshLists.end(); ++it) {
		for (std::vector<luxrays::TriangleMesh *>::const_iterator mesh = (*it).second.begin(); mesh != (*it).second.end(); ++mesh)
			delete *mesh;
	}

	return dataSet;
}

void HybridRenderer::LoadCfgParams(const string &configFile, ParamSet *params) {
	// adds parameters from cfg file to paramset

	boost::smatch m;

	boost::regex cfg_comment_expr("^\\s*#");
	boost::regex cfg_param_expr("\\s*([\\w\\.]+)\\s*=\\s*([^\\s]+)\\s*[\\r\\n]?");
	boost::regex int_expr("\\d+");
	boost::regex float_expr("-?\\d*\\.?\\d+(?:[eE]-?\\d+)?");
	boost::regex bool_expr("(true|false)");

	LOG(LUX_INFO, LUX_NOERROR) << "Loading config file '" << configFile << "'";

	std::ifstream fs;
	fs.open(configFile.c_str());
	string line;

	if (!fs.is_open()) {
		LOG(LUX_WARNING, LUX_BADFILE) << "Error opening config file '" << configFile << "'";
		return;
	}

	int n = 0;

	while (getline(fs, line).good()) {
		if (boost::regex_search(line, m, cfg_comment_expr))
			continue;

		// use match instead of search to ensure valid syntax
		if (!boost::regex_match(line, m, cfg_param_expr)) {
			LOG(LUX_WARNING, LUX_BADTOKEN) << "Invalid syntax in config file: '" << line << "'";
			continue;
		}

		string name = m[1];
		string value = m[2];

		// duck typing
		if (boost::regex_match(value, m, int_expr)) {
			int i = boost::lexical_cast<int>(m[0]);
			params->AddInt(name, &i);
			// if 0 or 1 it may be a bool, add to be sure
			// user will know what to look for
			if (i == 0 || i == 1) {
				bool b = i == 1;
				params->AddBool(name, &b);
			}
		} else if (boost::regex_match(value, m, float_expr)) {
			float f = boost::lexical_cast<float>(m[0]);
			params->AddFloat(name, &f);
		} else if (boost::regex_match(value, m, bool_expr)) {			
			bool b = m[0] == "true" ? true : false;
			params->AddBool(name, &b);
		} else {
			params->AddString(name, &value);
		}
		n++;
	}

	fs.close();

	LOG(LUX_DEBUG, LUX_NOERROR) << "Read " << n << " parameters from config file";
}
