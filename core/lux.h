/***************************************************************************
 *   Copyright (C) 1998-2008 by authors (see AUTHORS.txt )                 *
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

#ifndef LUX_LUX_H
#define LUX_LUX_H

// lux.h*
// Global Include Files
#include <cmath>
#ifdef __CYGWIN__
#include <ieeefp.h>
#endif

#include <boost/archive/text_oarchive.hpp>
#include <boost/archive/text_iarchive.hpp>
#include <boost/serialization/vector.hpp>
#include <boost/serialization/string.hpp>
#include <boost/serialization/split_member.hpp>

#if defined (__INTEL_COMPILER) && !defined(WIN32)
// Dade - to fix a problem with expf undefined with Intel CC
inline float expf(float a) { return exp(a); }
#endif

#if !defined(__APPLE__) && !defined(__OpenBSD__) && !defined(__FreeBSD__)
#  include <malloc.h> // for _alloca, memalign
#  if !defined(WIN32) || defined(__CYGWIN__)
#    include <alloca.h>
#  endif
#endif
#if defined(__FreeBSD__)
#  define memalign(A,B)  malloc(B)
#endif
#if defined(WIN32) && !defined(__CYGWIN__)
#  include <float.h>
#  pragma warning (disable: 4244) // conversion from double to float (VS2005) - Radiance
#  pragma warning (disable: 4305) // truncation from double to float (VS2005) - Radiance
#  pragma warning (disable: 4996) // deprecated functions (VS2005) - Radiance
#  pragma warning (disable: 4267) // conversion from 'size_t' [asio\detail\socket_ops.hpp; boost\serialization\collections_save_imp.hpp] - zcott
#  pragma warning (disable: 4311) // pointer truncation from 'void *' to 'long' [Fl_Widget.H; Fl_Menu_Item.H;; asio\detail\win_iocp_socket_service.hpp] - zcott
#  pragma warning(disable : 4312) // conversion from 'long' to 'void *' of greater size [Fl_Widget.H; Fl_Menu_Item.H; asio\detail\win_iocp_socket_service.hpp] - zcott
//note: the above are duplicated in compiler options, kept here for reference only - zcott
#  pragma warning (disable: 4267 4251 4065 4102)
#  pragma warning (disable: 4190) // extern "C" nonsense when returning a template
//#define WIN32_LEAN_AND_MEAN //defined in project properties
#  include <windows.h>
#endif
#include <stdlib.h>
#define _GNU_SOURCE 1 //NOBOOK
#include <stdio.h>
#include <string.h>
#include <string>
using std::string;
#include <vector>
using std::vector;
#include <iostream>
using std::ostream;
#include <algorithm>
using std::min;
using std::max;
using std::swap;
using std::sort;
#include <assert.h>

// Platform-specific definitions
#if defined(WIN32) && !defined(__CYGWIN__)
#  define memalign(a,b) _aligned_malloc(b, a)
#  define alloca _alloca
#  define isnan _isnan
#  define isinf(f) (!_finite((f)))
#elif defined(__APPLE__)
#  define memalign(a,b) valloc(b)
#  define powf pow
#  define sinf sin
#  define cosf cos
#  define tanf tan
#  define asinf asin
#  define acosf acos
#  define atanf atan
#  define atan2f atan2
#  define logf log
#  define log10f log10
// Dade - commented out because of Jim's suggestion in bug #177
//#  define expf exp
#  define sqrtf sqrt
#  if (__GNUC__ == 3) || (__GNUC__ == 4)
extern "C" {
  int isinf(double);
  int isnan(double);
}
#  endif // ONLY GCC 3
#elif defined(__OpenBSD__)
#  define memalign(a,b) malloc(b)
#elif defined(sgi)
#  define for if (0) ; else for
#endif

// Global Type Declarations
typedef double StatsCounterType;
typedef unsigned char u_char;
typedef unsigned short u_short;
typedef unsigned int u_int;
typedef unsigned long u_long;
#define BC_GRID_SIZE 40
typedef vector<int> SampleGrid[BC_GRID_SIZE][BC_GRID_SIZE];
#define GRID(v) (int((v) * BC_GRID_SIZE))

// Global Forward Declarations
class Timer;
class MemoryArena;
template<class T, int logBlockSize = 2> class BlockedArray;
class ProgressReporter;
class StatsCounter;
class StatsRatio;
class StatsPercentage;

#include "randomgen.h"

namespace lux
{
  class Matrix4x4;
  class ParamSet;
  template <class T> struct ParamSetItem;
  class Vector;
  class Point;
  class Normal;
  class Ray;
  class RayDifferential;
  class BBox;
  class Transform;
  class DifferentialGeometry;
  class TextureParams;
  class Scene;
  class Primitive;
  class AreaLightPrimitive;
  class InstancePrimitive;
  class MotionPrimitive;
  class Aggregate;
  class Intersection;
  class RGBColor;
  class ImageData;
  class SWCSpectrum;
  class SpectrumWavelengths;
  class Color;
  class RGBColor;
  class XYZColor;
  class SPD;
  class Camera;
  class ProjectiveCamera;
  class Sampler;
  class PixelSampler;
  class IntegrationSampler;
  class Sample;
  class Filter;
  class Film;
  class ToneMap;
  class BxDF;
  class BRDF;
  class BTDF;
  class Fresnel;
  class FresnelConductor;
  class FresnelDielectric;
  class FresnelNoOp;
  class SpecularReflection;
  class SpecularTransmission;
  class Lambertian;
  class OrenNayar;
  class Microfacet;
  class MicrofacetDistribution;
  class BSDF;
  class Material;
  class TextureMapping2D;
  class UVMapping2D;
  class SphericalMapping2D;
  class CylindricalMapping2D;
  class PlanarMapping2D;
  class TextureMapping3D;
  class IdentityMapping3D;
  class TriangleMesh;
  class PlyMesh;
  template <class T> class Texture;
  class VolumeRegion;
  class Light;
  struct VisibilityTester;
  class AreaLight;
  class Shape;
  class PrimitiveSet;
  class Integrator;
  class SurfaceIntegrator;
  class VolumeIntegrator;
  class RandomGenerator;
  class RenderFarm;
  class Contribution;
  class ContributionBuffer;
  class ContributionSystem;
  class MotionSystem;
  class Distribution1D;
  class IrregularDistribution1D;
}

// Global Constants
#ifdef M_PI
#  undef M_PI
#endif
#define M_PI           3.14159265358979323846f
#define INV_PI  0.31830988618379067154f
#define INV_TWOPI  0.15915494309189533577f
#ifndef INFINITY
#  define INFINITY HUGE_VAL
//#define INFINITY std::numeric_limits<float>::max()
#endif
#define LUX_VERSION 0.6
#define LUX_VERSION_STRING "0.6 (RC2)"
#define COLOR_SAMPLES 3
#if defined(WIN32) && !defined(__CYGWIN__)
#  define LUX_PATH_SEP ";"
#else
#  define LUX_PATH_SEP ":"
#endif

// Global Function Declarations
extern "C" void luxInit();
extern "C" void luxCleanup();
bool ParseFile(const char *filename);
namespace lux
{
  //string hashing function
  unsigned int DJBHash(const std::string& str);

  bool SolveLinearSystem2x2(const float A[2][2], const float B[2], float x[2]);

	ImageData *ReadImage(const string &name);
}

#include <boost/shared_ptr.hpp>
#include <boost/enable_shared_from_this.hpp>

// Radiance - Thread specific pack of pointers to eliminate use of boost tss smart pointers.
// Initialized per thread in scene.cpp/RenderThread::RenderThread and passed where needed.
namespace lux {

	struct TsPack {
		SpectrumWavelengths *swl;
		RandomGenerator *rng;
		MemoryArena *arena;
		Camera *camera;
		float time;
	};

}

// Global Inline Functions
template<class T> inline T Lerp(float t, T v1, T v2) {
	return (1.f - t) * v1 + t * v2;
}
template<class T> inline T Clamp(T val, T low, T high) {
	return val > low ? (val < high ? val : high) : low;
}
inline int Mod(int a, int b) {
	// note - radiance - added 0 check to prevent divide by zero error(s)
	if(a==0) a=1;
	if(b==0) b=1;

    int n = int(a/b);
    a -= n*b;
    if (a < 0)
        a += b;
    return a;
}
inline float Radians(float deg) {
	return ((float)M_PI/180.f) * deg;
}
inline float Degrees(float rad) {
	return (180.f/(float)M_PI) * rad;
}
inline float Log2(float x) {
	return logf(x) / logf(2.f);
}
inline int Log2Int(float v) {
#if 0
 	return ((*reinterpret_cast<int *>(&v)) >> 23) - 127;
#else
#define _doublemagicroundeps	      (.5-1.4e-11)
	return int(Log2(v) + _doublemagicroundeps);
#endif
}
inline bool IsPowerOf2(int v) {
	return (v & (v - 1)) == 0;
}
inline u_int RoundUpPow2(u_int v) {
	v--;
	v |= v >> 1;
	v |= v >> 2;
	v |= v >> 4;
	v |= v >> 8;
	v |= v >> 16;
	return v+1;
}
#if (defined(__linux__) && defined(__i386__)) || defined(WIN32)
//#define FAST_INT 1
#endif
#define _doublemagicroundeps	      (.5-1.4e-11)
	//almost .5f = .5f - 1e^(number of exp bit)
inline int Round2Int(double val) {
#ifdef FAST_INT
#define _doublemagic			double (6755399441055744.0)
	//2^52 * 1.5,  uses limited precision to floor
	val		= val + _doublemagic;
	return (reinterpret_cast<long*>(&val))[0];
#else
	return int (val+_doublemagicroundeps);
#endif
}
template<class T> inline int Float2Int(T val) {
#ifdef FAST_INT
	return (val<0) ?  Round2Int(val+_doublemagicroundeps) :
		   Round2Int(val-_doublemagicroundeps);
#else
	return (int)val;
#endif
}
template<class T> inline int Floor2Int(T val) {
#ifdef FAST_INT
	return Round2Int(val - _doublemagicroundeps);
#else
	return (int)floor(val);
#endif
}
template<class T> inline int Ceil2Int(T val) {
#ifdef FAST_INT
	return Round2Int(val + _doublemagicroundeps);
#else
	return (int)ceil(val);
#endif
}
inline bool Quadratic(float A, float B, float C, float *t0,
		float *t1) {
	// Find quadratic discriminant
	float discrim = B * B - 4.f * A * C;
	if (discrim < 0.) return false;
	float rootDiscrim = sqrtf(discrim);
	// Compute quadratic _t_ values
	float q;
	if (B < 0) q = -.5f * (B - rootDiscrim);
	else       q = -.5f * (B + rootDiscrim);
	*t0 = q / A;
	*t1 = C / q;
	if (*t0 > *t1) swap(*t0, *t1);
	return true;
}
inline float SmoothStep(float min, float max, float value) {
	float v = Clamp((value - min) / (max - min), 0.f, 1.f);
	return v * v * (-2.f * v  + 3.f);
}

#endif // LUX_LUX_H
