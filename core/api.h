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
//#include "lux.h"
#include <string>
#include "../renderer/include/asio.hpp"

using std::string;
using asio::ip::tcp;

namespace lux
{
class ParamSet;
}//namespace lux
using namespace lux;

// API Function Declarations
extern  void luxIdentity();
extern  void luxTranslate(float dx, float dy, float dz);
extern  void luxRotate(float angle,
                               float ax,
							   float ay,
							   float az);
extern  void luxScale(float sx,
                              float sy,
							  float sz);
extern  void luxLookAt(float ex,
                               float ey,
							   float ez,
							   float lx,
							   float ly,
							   float lz,
							   float ux,
							   float uy,
							   float uz);
extern 
	void luxConcatTransform(float transform[16]);
extern 
	void luxTransform(float transform[16]);
extern  void luxCoordinateSystem(const string &);
extern  void luxCoordSysTransform(const string &);
extern  void luxPixelFilter(const string &name, const ParamSet &params);
extern  void luxFilm(const string &type,
                            const ParamSet &params);
extern  void luxSampler(const string &name,
                               const ParamSet &params);
extern  void luxAccelerator(const string &name,
	                               const ParamSet &params);
extern 
	void luxSurfaceIntegrator(const string &name,
							  const ParamSet &params);
extern 
	void luxVolumeIntegrator(const string &name,
							 const ParamSet &params);
extern  void luxCamera(const string &, const ParamSet &cameraParams);
extern  void luxSearchPath(const string &path);
extern  void luxWorldBegin();
extern  void luxAttributeBegin();
extern  void luxAttributeEnd();
extern  void luxTransformBegin();
extern  void luxTransformEnd();
extern  void luxTexture(const string &name, const string &type,
	const string &texname, const ParamSet &params);
extern  void luxMaterial(const string &name,
                               const ParamSet &params);
extern  void luxLightSource(const string &name, const ParamSet &params);
extern  void luxAreaLightSource(const string &name, const ParamSet &params);
extern  void luxPortalShape(const string &name, const ParamSet &params);
extern  void luxShape(const string &name, const ParamSet &params);
extern  void luxReverseOrientation();
extern  void luxVolume(const string &name, const ParamSet &params);
extern  void luxObjectBegin(const string &name);
extern  void luxObjectEnd();
extern  void luxObjectInstance(const string &name);
extern  void luxWorldEnd();


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

//film access (networking)
extern void luxGetFilm(tcp::iostream &stream);
extern void luxUpdateFilmFromNetwork();

//statistics
extern double luxStatistics(char *statName);

extern void luxAddServer(const string &name);



//Error Handlers
extern int luxLastError; //!< Keeps track of the last error code
typedef void  (*LuxErrorHandler)(int code, int severity, const char *msg);
extern void luxErrorHandler (LuxErrorHandler handler);
extern void luxErrorAbort (int code, int severity, const char *message);
extern void luxErrorIgnore (int code, int severity, const char *message);
extern void luxErrorPrint (int code, int severity, const char *message);

/*
    Error codes

     1 - 10     System and File errors
    11 - 20     Program Limitations
    21 - 40     State Errors
    41 - 60     Parameter and Protocol Errors
    61 - 80     Execution Errors
*/

#define LUX_NOERROR         0

#define LUX_NOMEM           1       /* Out of memory */
#define LUX_SYSTEM          2       /* Miscellaneous system error */
#define LUX_NOFILE          3       /* File nonexistant */
#define LUX_BADFILE         4       /* Bad file format */
#define LUX_BADVERSION      5       /* File version mismatch */
#define LUX_DISKFULL        6       /* Target disk is full */

#define LUX_UNIMPLEMENT    12       /* Unimplemented feature */
#define LUX_LIMIT          13       /* Arbitrary program limit */
#define LUX_BUG            14       /* Probably a bug in renderer */

#define LUX_NOTSTARTED     23       /* luxInit() not called */
#define LUX_NESTING        24       /* Bad begin-end nesting - jromang will be used in API v2 */
#define LUX_NOTOPTIONS     25       /* Invalid state for options - jromang will be used in API v2 */
#define LUX_NOTATTRIBS     26       /* Invalid state for attributes - jromang will be used in API v2 */
#define LUX_NOTPRIMS       27       /* Invalid state for primitives - jromang will be used in API v2 */
#define LUX_ILLSTATE       28       /* Other invalid state - jromang will be used in API v2 */
#define LUX_BADMOTION      29       /* Badly formed motion block - jromang will be used in API v2 */
#define LUX_BADSOLID       30       /* Badly formed solid block - jromang will be used in API v2 */

#define LUX_BADTOKEN       41       /* Invalid token for request */
#define LUX_RANGE          42       /* Parameter out of range */
#define LUX_CONSISTENCY    43       /* Parameters inconsistent */
#define LUX_BADHANDLE      44       /* Bad object/light handle */
#define LUX_NOPLUGIN       45       /* Can't load requested plugin */
#define LUX_MISSINGDATA    46       /* Required parameters not provided */
#define LUX_SYNTAX         47       /* Declare type syntax error */

#define LUX_MATH           61       /* Zerodivide, noninvert matrix, etc. */


/* Error severity levels */

#define LUX_INFO            0       /* Rendering stats & other info */
#define LUX_WARNING         1       /* Something seems wrong, maybe okay */
#define LUX_ERROR           2       /* Problem.  Results may be wrong */
#define LUX_SEVERE          3       /* So bad you should probably abort */




#endif // LUX_API_H
