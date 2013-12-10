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

#include <boost/shared_ptr.hpp>
#include <boost/enable_shared_from_this.hpp>
#include <boost/version.hpp>

#include "luxrays/luxrays.h"
#include "luxrays/core/epsilon.h"
#include "luxrays/core/geometry/transform.h"
#include "luxrays/core/color/swcspectrum.h"
#include "luxrays/core/utils.h"
#include "luxrays/utils/memory.h"

// boost version starting from 1.50 defined TIME_UTC_ instead of TIME_UTC because of a conflict with libc and c++ 2011
// https://svn.boost.org/trac/boost/ticket/6940
// glibc > 1.16 includes a TIME_UTC macro, so boost renamed to TIME_UTC_
//
// This hook allows to build with boost < 1.50 and glibc < 1.16
//
#if (BOOST_VERSION < 105000)
#define TIME_UTC_ TIME_UTC
#endif

// Platform-specific definitions
#if defined(WIN32) && !defined(__CYGWIN__)
#  include <float.h>
#  pragma warning (disable: 4244) // conversion from double to float (VS2005) - Radiance
#  pragma warning (disable: 4305) // truncation from double to float (VS2005) - Radiance
#  pragma warning (disable: 4996) // deprecated functions (VS2005) - Radiance
#  pragma warning (disable: 4267) // conversion from 'size_t' [asio\detail\socket_ops.hpp; boost\serialization\collections_save_imp.hpp] - zcott
#  pragma warning (disable: 4311) // pointer truncation from 'void *' to 'long' [Fl_Widget.H; Fl_Menu_Item.H;; asio\detail\win_iocp_socket_service.hpp] - zcott
#  pragma warning (disable : 4312) // conversion from 'long' to 'void *' of greater size [Fl_Widget.H; Fl_Menu_Item.H; asio\detail\win_iocp_socket_service.hpp] - zcott
//note: the above are duplicated in compiler options, kept here for reference only - zcott
#  pragma warning (disable: 4267 4251 4065 4102)
#  pragma warning (disable: 4190) // extern "C" nonsense when returning a template
#  pragma warning (disable: 4290) // C++ exception specification ignored except to indicate a function is not __declspec(nothrow) ; pointless warning
#  pragma warning (disable: 4355) // 'this' used in base member initializer list
//#define WIN32_LEAN_AND_MEAN //defined in project properties
#  include <windows.h>

#include <boost/cstdint.hpp>
using boost::int8_t;
using boost::int16_t;
using boost::int32_t;
using boost::int64_t;
using boost::uint8_t;
using boost::uint16_t;
using boost::uint32_t;
using boost::uint64_t;

inline float atanhf(float x) {
	// if outside of domain, return NaN
	// not 100% correct but should be good for now
	if(x <= -1.f || x >= 1.f) 
		return sqrtf(-1.f); 
  
	return logf((1.f + x) / (1.f - x)) / 2.f;
}


namespace w32util
{
# include <stdio.h>
# include <wincon.h>
#define FOREGROUND_YELLOW FOREGROUND_RED | FOREGROUND_GREEN
#define FOREGROUND_WHITE FOREGROUND_RED | FOREGROUND_GREEN | FOREGROUND_BLUE
inline void ChangeConsoleColor(WORD col)
{
	HANDLE hConsole = GetStdHandle(STD_OUTPUT_HANDLE);

	// keep user-defined background and foreground intensity
	CONSOLE_SCREEN_BUFFER_INFO screenBufferInfo;
	GetConsoleScreenBufferInfo(hConsole, &screenBufferInfo);
	col |= screenBufferInfo.wAttributes & static_cast<WORD>(FOREGROUND_INTENSITY | BACKGROUND_INTENSITY);

	SetConsoleTextAttribute(hConsole, col);
}
}
#endif

#if defined (__INTEL_COMPILER) && !defined(WIN32)
// Dade - to fix a problem with expf undefined with Intel CC
inline float expf(float a) { return exp(a); }
#endif

// Global Constants
#if defined(WIN32) && !defined(__CYGWIN__)
#  define LUX_PATH_SEP ";"
#else
#  define LUX_PATH_SEP ":"
#endif

// Global Type Declarations
#define BC_GRID_SIZE 40
typedef vector<int> SampleGrid[BC_GRID_SIZE][BC_GRID_SIZE];
#define GRID(v) (int((v) * BC_GRID_SIZE))

// Global Forward Declarations
class Timer;

namespace lux
{
  class ParamSet;
  template <class T> struct ParamSetItem;
  using luxrays::BBox;
  using luxrays::MachineEpsilon;
  using luxrays::Matrix4x4;
  using luxrays::Normal;
  using luxrays::Point;
  using luxrays::Ray;
  using luxrays::Transform;
  using luxrays::Vector;
  class RayDifferential;
  class DifferentialGeometry;
  class Renderer;
  class Scene;
  class Primitive;
  class AreaLightPrimitive;
  class InstancePrimitive;
  class MotionPrimitive;
  class Aggregate;
  class Intersection;
  class ImageData;
  class MIPMap;
  using luxrays::SWCSpectrum;
  using luxrays::SpectrumWavelengths;
  using luxrays::RGBColor;
  using luxrays::RGBAColor;
  using luxrays::XYZColor;
  using luxrays::ColorSystem;
  using luxrays::ColorAdaptator;
  using luxrays::SPD;
  class Camera;
  class CameraResponse;
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
  class FresnelGeneral;
  class FresnelGeneric;
  class FresnelNoOp;
  class FresnelSlick;
  class SpecularReflection;
  class SpecularTransmission;
  class Lambertian;
  class OrenNayar;
  class Microfacet;
  class MicrofacetDistribution;
  class BSDF;
  class Material;
  struct CompositingParams;
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
  class Volume;
  class Region;
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
  class ContributionPool;
  class ContributionSystem;
  class InterpolatedTransform;
  class MotionSystem;
  class MotionTransform;
  class SampleableSphericalFunction;

  class Context;

// Global Function Declarations
  //string hashing function
  unsigned int DJBHash(const std::string& str);

  // accepts platform-specific filenames and performs fallback
  ImageData *ReadImage(const string &name);

  // converts paths to portable format and 
  // provides fallback mechanism for missing files
  string AdjustFilename(const string filename, bool silent = false);

}

#endif // LUX_LUX_H
