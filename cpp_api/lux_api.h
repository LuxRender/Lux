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

#ifndef LUX_CPP_API_H	// LUX_API_H already used by Lux core/api.h
#define LUX_CPP_API_H

#include <vector>

#include <boost/thread.hpp>

// LuxRender core includes
#include "context.h"
#include "queryable.h"
#include "paramset.h"
#include "queryableregistry.h"

// CPP API Includes
#include "lux_instance.h"
#include "lux_paramset.h"

class lux_wrapped_context : public lux_instance {
public:
	lux_wrapped_context(const char* _name);
	~lux_wrapped_context();

	// context metadata
	const char* version();
	const char* getName();

	// file parsing methods
	bool parse(const char* filename, bool async);
	bool parsePartial(const char* filename, bool async);
	bool parseSuccessful();
	
	// rendering control
	void pause();
	void start();
	void setHaltSamplesPerPixel(int haltspp, bool haveEnoughSamplesPerPixel, bool suspendThreadsWhenDone);
	unsigned int addThread();
	void removeThread();
	void abort();
	void wait();
	void exit();
	void cleanup();

	// scene description methods
	void accelerator(const char *aName, const lux_paramset* params);
	void areaLightSource(const char *aName, const lux_paramset* params);
	void attributeBegin();
	void attributeEnd();
	void camera(const char *cName, const lux_paramset* params);
	void concatTransform(float tx[16]);
	void coordinateSystem(const char *cnName);
	void coordSysTransform(const char *cnName);
	void exterior(const char *eName);
	void film(const char *fName, const lux_paramset* params);
	void identity();
	void interior(const char *iName);
	void lightGroup(const char *lName, const lux_paramset* params);
	void lightSource(const char *lName, const lux_paramset* params);
	void lookAt(float ex, float ey, float ez, float lx, float ly, float lz, float ux, float uy, float uz);
	void makeNamedMaterial(const char *mName, const lux_paramset* params);
	void makeNamedVolume(const char *vName, const char *vType, const lux_paramset* params);
	void material(const char *mName, const lux_paramset* params);
	void motionBegin(u_int n, float *t);
	void motionEnd();
	void motionInstance(const char *mName, float startTime, float endTime, const char *toTransform);
	void namedMaterial(const char *mName);
	void objectBegin(const char *oName);
	void objectEnd();
	void objectInstance(const char *oName);
	void pixelFilter(const char *pName, const lux_paramset* params);
	void portalInstance(const char *pName);
	void portalShape(const char *pName, const lux_paramset* params);
	void renderer(const char *rName, const lux_paramset* params);
	void reverseOrientation();
	void rotate(float angle, float ax, float ay, float az);
	void sampler(const char *sName, const lux_paramset* params);
	void scale(float sx, float sy, float sz);
	void shape(const char *sName, const lux_paramset* params);
	void surfaceIntegrator(const char *sName, const lux_paramset* params);
	void texture(const char *tName, const char *tVariant, const char *tType, const lux_paramset* params);
	void transform(float tx[16]);
	void transformBegin();
	void transformEnd();
	void translate(float dx, float dy, float dz);
	void volume(const char *vName, const lux_paramset* params);	
	void volumeIntegrator(const char *vName, const lux_paramset* params);
	void worldBegin();
	void worldEnd();

	// I/O and imaging
	void loadFLM(const char* fName);
	void saveFLM(const char* fName);
	void saveEXR(const char *filename, bool useHalfFloat, bool includeZBuffer, bool tonemapped);
	void overrideResumeFLM(const char *fName);
	void updateFramebuffer();
	const unsigned char* framebuffer();
	const float* floatFramebuffer();
	const float* alphaBuffer();
	const float* zBuffer();
	const unsigned char* getHistogramImage(unsigned int width, unsigned int height, int options);

	// Old-style parameter update interface
	// To implement these requires exporting further luxComponent* symbols; In most cases the 
	// new Queryable system should be preferred anyhow.
	// void setParameterValue(luxComponent comp, luxComponentParameters param, double value, unsigned int index);
	// double getParameterValue(luxComponent comp, luxComponentParameters param, unsigned int index);
	// double getDefaultParameterValue(luxComponent comp, luxComponentParameters param, unsigned int index);
	// void setStringParameterValue(luxComponent comp, luxComponentParameters param, const char* value, unsigned int index);
	// unsigned int getStringParameterValue(luxComponent comp, luxComponentParameters param, char* dst, unsigned int dstlen, unsigned int index);
	// unsigned int getDefaultStringParameterValue(luxComponent comp, luxComponentParameters param, char* dst, unsigned int dstlen, unsigned int index);

	// Queryable interface
	const char* getAttributes();
	// TODO; this requires a get*Attribute()/set*Attribute() method for each data type.
	// void* getAttribute(const char *objectName, const char *attributeName);
	// void setAttribute(const char *objectName, const char *attributeName, void* value);

	// Networking interface
	void addServer(const char *sName);
	void removeServer(const char *sName);
	unsigned int getServerCount();
	void updateFilmFromNetwork();
	void setNetworkServerUpdateInterval(int updateInterval);
	int getNetworkServerUpdateInterval();
	// How to return data from this one? Export another data type?
	// boost::python::tuple getRenderingServersStatus();

	// Stats
	double statistics(const char* statName);
	const char* printableStatistics(const bool addTotal);	// Deprecated
	void updateStatisticsWindow();

	// Debugging interface
	void enableDebugMode();
	void disableRandomMode();
	void setEpsilon(const float minValue, const float maxValue);

private:
	const char* name;
	lux::Context* ctx;
	std::vector<boost::thread*> render_threads;
	void checkContext()
	{
		if (ctx == NULL)
			ctx = new lux::Context(name);

		lux::Context::SetActive(ctx);
	}
	void world_end_thread()
	{
		ctx->WorldEnd();
	}
};

class lux_wrapped_paramset : public lux_paramset {
public:
	lux_wrapped_paramset();
	~lux_wrapped_paramset();

	lux::ParamSet* GetParamSet() { return ps; };

	void AddFloat(const char*, const float *, u_int nItems = 1);
	void AddInt(const char*, const int *, u_int nItems = 1);
	void AddBool(const char*, const bool *, u_int nItems = 1);
	void AddPoint(const char*, const float *, u_int nItems = 1);
	void AddVector(const char*, const float *, u_int nItems = 1);
	void AddNormal(const char*, const float *, u_int nItems = 1);
	void AddRGBColor(const char*, const float *, u_int nItems = 1);
	void AddString(const char*, const char**, u_int nItems = 1);
	void AddString(const char*, const std::string *, unsigned int nItems = 1);
	void AddTexture(const char*, const char*);

private:
	lux::ParamSet* ps;
};

// exported factory and cleanup functions
CPP_EXPORT CPP_API lux_instance* CreateLuxInstance(const char* _name);
CPP_EXPORT CPP_API void DestroyLuxInstance(lux_instance* inst);

CPP_EXPORT CPP_API lux_paramset* CreateLuxParamSet();
CPP_EXPORT CPP_API void DestroyLuxParamSet(lux_paramset* ps);

#endif	// LUX_CPP_API_H
