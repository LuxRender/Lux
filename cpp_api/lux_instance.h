/***************************************************************************
 *   Copyright (C) 1998-2009 by authors (see AUTHORS.txt )                 *
 *                                                                         *
 *   This file is part of LuxRender.                                       *
 *                                                                         *
 *   Lux Renderer is free software = 0; you can redistribute it and/or modify  *
 *   it under the terms of the GNU General Public License as published by  *
 *   the Free Software Foundation = 0; either version 3 of the License, or     *
 *   (at your option) any later version.                                   *
 *                                                                         *
 *   Lux Renderer is distributed in the hope that it will be useful,       *
 *   but WITHOUT ANY WARRANTY = 0; without even the implied warranty of        *
 *   MERCHANTABILITY or FITNESS FOR A PARTICULAR PURPOSE.  See the         *
 *   GNU General Public License for more details.                          *
 *                                                                         *
 *   You should have received a copy of the GNU General Public License     *
 *   along with this program.  If not, see <http://www.gnu.org/licenses/>. *
 *                                                                         *
 *   This project is based on PBRT  = 0; see http://www.pbrt.org               *
 *   Lux Renderer website : http://www.luxrender.net                       *
 ***************************************************************************/

// This file defines the interface to a LuxRender rendering Context

#ifndef LUX_INSTANCE_H
#define LUX_INSTANCE_H

#include "export_defs.h"
#include "lux_paramset.h"

// This is the CPP API Interface for LuxRender
CPP_EXPORT class CPP_API lux_instance {
public:
	lux_instance() {};
	virtual ~lux_instance() {};

	// context metadata
	virtual const char* version() = 0;
	virtual const char* getName() = 0;

	// file parsing methods
	virtual bool parse(const char* filename, bool async) = 0;
	virtual bool parsePartial(const char* filename, bool async) = 0;
	virtual bool parseSuccessful() = 0;
	
	// rendering control
	virtual void pause() = 0;
	virtual void start() = 0;
	virtual void setHaltSamplesPerPixel(int haltspp, bool haveEnoughSamplesPerPixel, bool suspendThreadsWhenDone) = 0;
	virtual unsigned int addThread() = 0;
	virtual void removeThread() = 0;
	virtual void wait() = 0;
	virtual void exit() = 0;
	virtual void cleanup() = 0;

	// scene description methods
	virtual void accelerator(const char *aName, const lux_paramset* params) = 0;
	virtual void areaLightSource(const char *aName, const lux_paramset* params) = 0;
	virtual void attributeBegin() = 0;
	virtual void attributeEnd() = 0;
	virtual void camera(const char *cName, const lux_paramset* params) = 0;
	virtual void concatTransform(float tx[16]) = 0;
	virtual void coordinateSystem(const char *cnName) = 0;
	virtual void coordSysTransform(const char *cnName) = 0;
	virtual void exterior(const char *eName) = 0;
	virtual void film(const char *fName, const lux_paramset* params) = 0;
	virtual void identity() = 0;
	virtual void interior(const char *iName) = 0;
	virtual void lightGroup(const char *lName, const lux_paramset* params) = 0;
	virtual void lightSource(const char *lName, const lux_paramset* params) = 0;
	virtual void lookAt(float ex, float ey, float ez, float lx, float ly, float lz, float ux, float uy, float uz) = 0;
	virtual void makeNamedMaterial(const char *mName, const lux_paramset* params) = 0;
	virtual void makeNamedVolume(const char *vName, const char *vType, const lux_paramset* params) = 0;
	virtual void material(const char *mName, const lux_paramset* params) = 0;
	virtual void motionInstance(const char *mName, float startTime, float endTime, const char *toTransform) = 0;
	virtual void namedMaterial(const char *mName) = 0;
	virtual void objectBegin(const char *oName) = 0;
	virtual void objectEnd() = 0;
	virtual void objectInstance(const char *oName) = 0;
	virtual void pixelFilter(const char *pName, const lux_paramset* params) = 0;
	virtual void portalInstance(const char *pName) = 0;
	virtual void portalShape(const char *pName, const lux_paramset* params) = 0;
	virtual void renderer(const char *rName, const lux_paramset* params) = 0;
	virtual void reverseOrientation() = 0;
	virtual void rotate(float angle, float ax, float ay, float az) = 0;
	virtual void sampler(const char *sName, const lux_paramset* params) = 0;
	virtual void scale(float sx, float sy, float sz) = 0;
	virtual void shape(const char *sName, const lux_paramset* params) = 0;
	virtual void surfaceIntegrator(const char *sName, const lux_paramset* params) = 0;
	virtual void texture(const char *tName, const char *tVariant, const char *tType, const lux_paramset* params) = 0;
	virtual void transform(float tx[16]) = 0;
	virtual void transformBegin() = 0;
	virtual void transformEnd() = 0;
	virtual void translate(float dx, float dy, float dz) = 0;
	virtual void volume(const char *vName, const lux_paramset* params) = 0;	
	virtual void volumeIntegrator(const char *vName, const lux_paramset* params) = 0;
	virtual void worldBegin() = 0;
	virtual void worldEnd() = 0;

	// I/O and imaging
	virtual void loadFLM(const char* fName) = 0;
	virtual void saveFLM(const char* fName) = 0;
	virtual void saveEXR(const char *filename, bool useHalfFloat, bool includeZBuffer, bool tonemapped) = 0;
	virtual void overrideResumeFLM(const char *fName) = 0;
	virtual void updateFramebuffer() = 0;
	virtual const unsigned char* framebuffer() = 0;
	virtual const float* floatFramebuffer() = 0;
	virtual const float* alphaBuffer() = 0;
	virtual const float* zBuffer() = 0;
	virtual const unsigned char* getHistogramImage(unsigned int width, unsigned int height, int options) = 0;

	// Old-style parameter update interface
	// To implement these requires exporting further luxComponent* symbols; In most cases the 
	// new Queryable system should be preferred anyhow.
	// virtual void setParameterValue(luxComponent comp, luxComponentParameters param, double value, unsigned int index) = 0;
	// virtual double getParameterValue(luxComponent comp, luxComponentParameters param, unsigned int index) = 0;
	// virtual double getDefaultParameterValue(luxComponent comp, luxComponentParameters param, unsigned int index) = 0;
	// virtual void setStringParameterValue(luxComponent comp, luxComponentParameters param, const char* value, unsigned int index) = 0;
	// virtual unsigned int getStringParameterValue(luxComponent comp, luxComponentParameters param, char* dst, unsigned int dstlen, unsigned int index) = 0;
	// virtual unsigned int getDefaultStringParameterValue(luxComponent comp, luxComponentParameters param, char* dst, unsigned int dstlen, unsigned int index) = 0;

	// Queryable interface
	virtual const char* getAttributes() = 0;
	// TODO; this requires a get*Attribute()/set*Attribute() method for each data type.
	// virtual void* getAttribute(const char *objectName, const char *attributeName) = 0;
	// virtual void setAttribute(const char *objectName, const char *attributeName, void* value) = 0;

	// Networking interface
	virtual void addServer(const char *sName) = 0;
	virtual void removeServer(const char *sName) = 0;
	virtual unsigned int getServerCount() = 0;
	virtual void updateFilmFromNetwork() = 0;
	virtual void setNetworkServerUpdateInterval(int updateInterval) = 0;
	virtual int getNetworkServerUpdateInterval() = 0;
	// How to return data from this one? Export another data type?
	// virtual boost::python::tuple getRenderingServersStatus() = 0;

	// Stats
	virtual double statistics(const char* statName) = 0;
	virtual const char* printableStatistics(const bool addTotal) = 0;
	virtual const char* customStatistics(const char* custom_template) = 0;

	// Debugging interface
	virtual void enableDebugMode() = 0;
	virtual void disableRandomMode() = 0;
	virtual void setEpsilon(const float minValue, const float maxValue) = 0;

};

// Pointer to lux_instance factory function
typedef lux_instance* (*CreateLuxInstancePtr)(const char* name);

#endif	// LUX_INSTANCE_H
