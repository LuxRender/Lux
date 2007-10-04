/***************************************************************************
 *   Copyright (C) 1998-2007 by authors (see AUTHORS.txt )                 *
 *                                                                         *
 *   This file is part of Lux Renderer.                                    *
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
 *   Lux Renderer website : http://www.luxrender.org                       *
 ***************************************************************************/

#ifndef LUX_API_H
#define LUX_API_H
// api.h*
#include "lux.h"
// API Function Declarations
extern COREDLL void luxIdentity();
extern COREDLL void luxTranslate(float dx, float dy, float dz);
extern COREDLL void luxRotate(float angle,
                               float ax,
							   float ay,
							   float az);
extern COREDLL void luxScale(float sx,
                              float sy,
							  float sz);
extern COREDLL void luxLookAt(float ex,
                               float ey,
							   float ez,
							   float lx,
							   float ly,
							   float lz,
							   float ux,
							   float uy,
							   float uz);
extern COREDLL
	void luxConcatTransform(float transform[16]);
extern COREDLL
	void luxTransform(float transform[16]);
extern COREDLL void luxCoordinateSystem(const string &);
extern COREDLL void luxCoordSysTransform(const string &);
extern COREDLL void luxPixelFilter(const string &name, const ParamSet &params);
extern COREDLL void luxFilm(const string &type,
                            const ParamSet &params);
extern COREDLL void luxSampler(const string &name,
                               const ParamSet &params);
extern COREDLL void luxAccelerator(const string &name,
	                               const ParamSet &params);
extern COREDLL
	void luxSurfaceIntegrator(const string &name,
							  const ParamSet &params);
extern COREDLL
	void luxVolumeIntegrator(const string &name,
							 const ParamSet &params);
extern COREDLL void luxCamera(const string &, const ParamSet &cameraParams);
extern COREDLL void luxSearchPath(const string &path);
extern COREDLL void luxWorldBegin();
extern COREDLL void luxAttributeBegin();
extern COREDLL void luxAttributeEnd();
extern COREDLL void luxTransformBegin();
extern COREDLL void luxTransformEnd();
extern COREDLL void luxTexture(const string &name, const string &type,
	const string &texname, const ParamSet &params);
extern COREDLL void luxMaterial(const string &name,
                               const ParamSet &params);
extern COREDLL void luxLightSource(const string &name, const ParamSet &params);
extern COREDLL void luxAreaLightSource(const string &name, const ParamSet &params);
extern COREDLL void luxShape(const string &name, const ParamSet &params);
extern COREDLL void luxReverseOrientation();
extern COREDLL void luxVolume(const string &name, const ParamSet &params);
extern COREDLL void luxObjectBegin(const string &name);
extern COREDLL void luxObjectEnd();
extern COREDLL void luxObjectInstance(const string &name);
extern COREDLL void luxWorldEnd();


//CORE engine control
//user interactive thread functions
extern void luxStart();
extern void luxPause();
extern void luxExit();

//controlling number of threads
extern int luxAddThread();
extern void luxRemoveThread();

//framebuffer access
extern void luxUpdateFramebuffer();
extern unsigned char* luxFramebuffer();
extern int luxDisplayInterval();
extern int luxFilmXres();
extern int luxFilmYres();

//statistics
extern double luxStatistics(char *statName);

#endif // LUX_API_H
