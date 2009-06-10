cmake_minimum_required(VERSION 2.4)
IF(COMMAND cmake_policy)
	cmake_policy(SET CMP0003 NEW)
ENDIF(COMMAND cmake_policy)

PROJECT(lux)
SET(VERSION 0.6)

# Dade - uncomment to obtain verbose building output
#SET(CMAKE_VERBOSE_MAKEFILE true)

IF(APPLE)
SET(APPLE_64 0)
ENDIF(APPLE)

#Setting Universal Binary Properties, only for Mac OS X
IF(APPLE)
	IF(APPLE_64)
		SET(CMAKE_OSX_ARCHITECTURES ppc64;x86_64)
		SET(CMAKE_OSX_SYSROOT /Developer/SDKs/MacOSX10.5.sdk)
	ELSE(APPLE_64)
		SET(CMAKE_OSX_ARCHITECTURES ppc;i386)
		SET(CMAKE_OSX_SYSROOT /Developer/SDKs/MacOSX10.4u.sdk)
	ENDIF(APPLE_64)
ENDIF(APPLE)


#############################################################################
#############################################################################
### check for the CPU we build for                                        ###
#############################################################################
#############################################################################

EXECUTE_PROCESS(
    COMMAND ${CMAKE_C_COMPILER} -dumpmachine
    OUTPUT_VARIABLE MACHINE
    OUTPUT_STRIP_TRAILING_WHITESPACE
)
MESSAGE(STATUS "Building for target ${MACHINE}")

STRING(REGEX MATCH "(i.86-*)|(athlon-*)|(pentium-*)" _mach_x86 ${MACHINE})
IF (_mach_x86)
    SET(ARCH_X86 1)
ENDIF (_mach_x86)

STRING(REGEX MATCH "(x86_64-*)|(X86_64-*)|(AMD64-*)|(amd64-*)" _mach_x86_64 ${MACHINE})
IF (_mach_x86_64)
    SET(ARCH_X86_64 1)
    SET(LIB_SUFFIX 64)
    #jromang - Hack to avoid boost bug on x64  Ubuntu 8.10 and Fedora 10 (http://www.luxrender.net/mantis/view.php?id=433)
    ADD_DEFINITIONS(-DBOOST_NO_INTRINSIC_INT64_T)
ENDIF (_mach_x86_64)

STRING(REGEX MATCH "(ppc-*)|(powerpc-*)" _mach_ppc ${MACHINE})
IF (_mach_ppc)
    SET(ARCH_PPC 1)
ENDIF (_mach_ppc)





#############################################################################
#############################################################################
###########################      Find BISON       ###########################
#############################################################################
#############################################################################
FIND_PROGRAM(BISON_EXECUTABLE bison)
IF (NOT BISON_EXECUTABLE)
	MESSAGE(FATAL_ERROR "bison not found - aborting")
ENDIF (NOT BISON_EXECUTABLE)


#############################################################################
#############################################################################
###########################      Find FLEX        ###########################
#############################################################################
#############################################################################
FIND_PROGRAM(FLEX_EXECUTABLE flex)
IF (NOT FLEX_EXECUTABLE)
	MESSAGE(FATAL_ERROR "flex not found - aborting")
ENDIF (NOT FLEX_EXECUTABLE)


#############################################################################
#############################################################################
###########################      Find OpenGL       ##########################
#############################################################################
#############################################################################
SET(OPENGL_LIBRARY ${CMAKE_ROOT}/Modules/FindOpenGL.cmake)
INCLUDE(${CMAKE_ROOT}/Modules/FindOpenGL.cmake)

MESSAGE(STATUS "OpenGL include directory: " ${OPENGL_INCLUDE_DIR})


#############################################################################
#############################################################################
########################### BOOST LIBRARIES SETUP ###########################
#############################################################################
#############################################################################
FIND_PACKAGE(Boost REQUIRED)
IF(Boost_FOUND)
	MESSAGE(STATUS "Boost library directory: " ${Boost_LIBRARY_DIRS})
	MESSAGE(STATUS "Boost include directory: " ${Boost_INCLUDE_DIRS})
	IF(CYGWIN)
		SET(Boost_LIBRARIES boost_thread-gcc-mt boost_program_options-gcc-mt boost_filesystem-gcc-mt boost_serialization-gcc-mt boost_iostreams-gcc-mt boost_regex-gcc-mt)
	ELSEIF(APPLE)
		SET(Boost_LIBRARIES boost_thread-mt-1_34_1 boost_program_options-mt-1_34_1 boost_filesystem-mt-1_34_1 boost_serialization-mt-1_34_1 boost_iostreams-mt-1_34_1 boost_regex-mt-1_34_1)
	ELSEIF(CMAKE_SYSTEM_NAME STREQUAL "FreeBSD")
		SET(Boost_LIBRARIES boost_thread boost_program_options boost_filesystem boost_serialization boost_iostreams boost_regex)
	ELSE(CMAKE_SYSTEM_NAME STREQUAL "FreeBSD")
		SET(Boost_LIBRARIES boost_thread-mt boost_program_options-mt boost_filesystem-mt boost_serialization-mt boost_iostreams-mt boost_regex-mt)
	ENDIF(CYGWIN)
ELSE(Boost_FOUND)
	MESSAGE(FATAL_ERROR "Could not find Boost")
ENDIF(Boost_FOUND)


#############################################################################
#############################################################################
######################### OPENEXR LIBRARIES SETUP ###########################
#############################################################################
#############################################################################
FIND_PATH(OPENEXR_INCLUDE_DIRS
	ImfXdr.h
	PATHS
	/usr/local/include/OpenEXR
	/usr/include/OpenEXR
	/sw/include/OpenEXR
	/opt/local/include/OpenEXR
	/opt/csw/include/OpenEXR
	/opt/include/OpenEXR
) 
IF(APPLE)
   SET(OPENEXR_LIBRARIES Half IlmImf Iex Imath IlmThread)

ELSE(APPLE)
   SET(OPENEXR_LIBRARIES Half IlmImf Iex Imath)
ENDIF(APPLE)

#############################################################################
#############################################################################
########################### PNG   LIBRARIES SETUP ###########################
#############################################################################
#############################################################################
# - Find the native PNG includes and library
#
# This module defines
#  PNG_INCLUDE_DIR, where to find png.h, etc.
#  PNG_LIBRARIES, the libraries to link against to use PNG.
#  PNG_DEFINITIONS - You should ADD_DEFINITONS(${PNG_DEFINITIONS}) before compiling code that includes png library files.
#  PNG_FOUND, If false, do not try to use PNG.
# also defined, but not for general use are
#  PNG_LIBRARY, where to find the PNG library.
# None of the above will be defined unles zlib can be found.
# PNG depends on Zlib
INCLUDE(FindPNG)
IF(PNG_FOUND)
ELSE(PNG_FOUND)
	MESSAGE( STATUS "Warning : could not find PNG - building without png support")
ENDIF(PNG_FOUND)
#FIND_PACKAGE(PNG REQUIRED)


#############################################################################
#############################################################################
########################### TIFF  LIBRARIES SETUP ###########################
#############################################################################
#############################################################################
# - Find TIFF library
# Find the native TIFF includes and library
# This module defines
#  TIFF_INCLUDE_DIR, where to find tiff.h, etc.
#  TIFF_LIBRARIES, libraries to link against to use TIFF.
#  TIFF_FOUND, If false, do not try to use TIFF.
# also defined, but not for general use are
#  TIFF_LIBRARY, where to find the TIFF library.
INCLUDE(FindTIFF)
#FIND_PACKAGE(TIFF REQUIRED)
IF(TIFF_FOUND)
	MESSAGE( STATUS "Found TIFF: " ${TIFF_LIBRARIES} )
ELSE(TIFF_FOUND)
	MESSAGE( STATUS "Warning : could not find TIFF - building without tiff support")
ENDIF(TIFF_FOUND)


#############################################################################
#############################################################################
########################### JPEG  LIBRARIES SETUP ###########################
#############################################################################
#############################################################################
# - Find JPEG
# Find the native JPEG includes and library
# This module defines
#  JPEG_INCLUDE_DIR, where to find jpeglib.h, etc.
#  JPEG_LIBRARIES, the libraries needed to use JPEG.
#  JPEG_FOUND, If false, do not try to use JPEG.
# also defined, but not for general use are
#  JPEG_LIBRARY, where to find the JPEG library.
INCLUDE(FindJPEG)
#FIND_PACKAGE(JPEG REQUIRED)
IF(JPEG_FOUND)
	MESSAGE( STATUS "JPEG include directory: " ${JPEG_INCLUDE_DIR} )
ELSE(JPEG_FOUND)
	MESSAGE( STATUS "Warning : could not find JPEG - building without jpeg support")
ENDIF(JPEG_FOUND)


#############################################################################
#############################################################################
######################### wxWidgets LIBRARIES SETUP #########################
#############################################################################
#############################################################################
# Here you can define what libraries of wxWidgets you need for your
# application. You can figure out what libraries you need here;
# http://www.wxwidgets.org/manuals/2.8/wx_librarieslist.html
SET(wxWidgets_USE_LIBS base core gl aui adv )
FIND_PACKAGE(wxWidgets)
IF(wxWidgets_FOUND)
    # Include wxWidgets macros
    INCLUDE(${wxWidgets_USE_FILE})
		MESSAGE( STATUS "wxWidgets include directory: " ${wxWidgets_INCLUDE_DIRS} )
ELSE(wxWidgets_FOUND)
		MESSAGE( STATUS "Warning : could not find wxWidgets - not building GUI")
ENDIF(wxWidgets_FOUND)


#############################################################################
#############################################################################
############################ THREADING LIBRARIES ############################
#############################################################################
#############################################################################

FIND_PACKAGE(Threads REQUIRED)


#############################################################################
#############################################################################
###########################   SYSTEM LIBRARIES    ###########################
#############################################################################
#############################################################################
IF(APPLE)
	SET(SYS_LIBRARIES z )
ENDIF(APPLE)
IF(CYGWIN)
	SET(SYS_LIBRARIES wsock32 ws2_32)
	ADD_DEFINITIONS(-D_WIN32_WINNT=0x0551 -D__USE_W32_SOCKETS -D_GLIBCXX__PTHREADS)
ENDIF(CYGWIN)


#############################################################################
#All dependencies OK !
#############################################################################

#Generate the config.h file
CONFIGURE_FILE(${CMAKE_SOURCE_DIR}/config.h.cmake ${CMAKE_BINARY_DIR}/config.h)
ADD_DEFINITIONS(-DLUX_USE_CONFIG_H)

#############################################################################
#############################################################################
#########################      COMPILER FLAGS     ###########################
#############################################################################
#############################################################################
# Dade - default compiler options
ADD_DEFINITIONS(-O3 -msse2 -mfpmath=sse -ftree-vectorize -funroll-loops -Wall -DLUX_USE_OPENGL -DHAVE_PTHREAD_H)


#############################################################################
# Compiler flags for specific setup
#############################################################################

# Jens - default Mac compiler options with OpenGL
#ADD_DEFINITIONS(-O3 -ftree-vectorize -msse -msse2 -msse3 -mssse3 -fvariable-expansion-in-unroller -Wall -DLUX_USE_OPENGL -DHAVE_PTHREAD_H )
# Jens - testing Mac compiler options (debug)
#ADD_DEFINITIONS(-DHAVE_PTHREAD_H)

# Dade - GCC Profiling (remember to uncomment the line at the end of file too)
#ADD_DEFINITIONS(-pg -g -O2 -march=-msse -mfpmath=sse -ftree-vectorize -Wall -DLUX_USE_OPENGL -DHAVE_PTHREAD_H)

# Dade - GCC 2 pass optimization (remember to uncomment the line at the end of file too)
#ADD_DEFINITIONS(-O3 --coverage -march=prescott -mfpmath=sse -ftree-vectorize -funroll-loops -ffast-math -Wall -DLUX_USE_OPENGL -DHAVE_PTHREAD_H)
#ADD_DEFINITIONS(-O3 -fbranch-probabilities -march=prescott -mfpmath=sse -ftree-vectorize -funroll-loops -ffast-math -Wall -DLUX_USE_OPENGL -DHAVE_PTHREAD_H)

# Dade - my settings
#ADD_DEFINITIONS(-g -O0 -DLUX_USE_OPENGL -DHAVE_PTHREAD_H)
#ADD_DEFINITIONS(-O3 -march=prescott -msse2 -mfpmath=sse -ftree-vectorize -funroll-loops -Wall -DLUX_USE_OPENGL -DHAVE_PTHREAD_H)
#ADD_DEFINITIONS(-O3 -march=athlon-xp -m3dnow -msse2 -mfpmath=sse -ftree-vectorize -funroll-loops -Wall -DLUX_USE_OPENGL -DHAVE_PTHREAD_H )

# Dade - Intel CC settings (double pass, 32bit, remember to uncomment the line at the end of file too)
#  rm -rf CMakeCache.txt CMakeFiles
#  CC=/opt/intel/cc/10.1.015/bin/icc CXX=/opt/intel/cc/10.1.015/bin/icpc cmake lux
# Pass 1
#ADD_DEFINITIONS(-prof-gen -prof-dir /tmp  -O3 -ipo -mtune=core2 -xT -unroll -fp-model fast=2 -rcd -no-prec-div -DLUX_USE_OPENGL -DHAVE_PTHREAD_H '-D"__sync_fetch_and_add(ptr,addend)=_InterlockedExchangeAdd(const_cast<void*>(reinterpret_cast<volatile void*>(ptr)), addend)"')
# Pass 2
#ADD_DEFINITIONS(-prof-use -prof-dir /tmp  -O3 -ipo -mtune=core2 -xT -unroll -fp-model fast=2 -rcd -no-prec-div -DLUX_USE_OPENGL -DHAVE_PTHREAD_H '-D"__sync_fetch_and_add(ptr,addend)=_InterlockedExchangeAdd(const_cast<void*>(reinterpret_cast<volatile void*>(ptr)), addend)"')
 
# Dade - Intel CC settings (single pass, 32bit)
#ADD_DEFINITIONS(-O3 -ip -mtune=core2 -xT -unroll -fp-model fast=2 -rcd -no-prec-div -DLUX_USE_OPENGL -DHAVE_PTHREAD_H '-D"__sync_fetch_and_add(ptr,addend)=_InterlockedExchangeAdd(const_cast<void*>(reinterpret_cast<volatile void*>(ptr)), addend)"')

# Dade - Intel CC settings (single pass, 64bit)
#ADD_DEFINITIONS(-O3 -ip -mtune=core2 -xT -unroll -fp-model fast=2 -rcd -no-prec-div -DLUX_USE_OPENGL -DHAVE_PTHREAD_H '-D"__sync_fetch_and_add(ptr,addend)=_InterlockedExchangeAdd(const_cast<void*>(reinterpret_cast<volatile void*>(ptr)), addend)"' -DBOOST_NO_INTRINSIC_INT64_T "-D'__builtin_vsnprintf(__out, __size, __fmt, __args)'='__builtin_vsnprintf(__out, __size, __fmt, (char *) __args)'") 

#ADD_DEFINITIONS(-O3 -msse -mfpmath=sse -ftree-vectorize -Wall -DLUX_USE_OPENGL -DHAVE_PTHREAD_H)
#ADD_DEFINITIONS(-O3 -march=athlon-xp -mmmx -m3dnow -msse -mfpmath=sse -ftree-vectorize -Wall -DLUX_USE_OPENGL -DHAVE_PTHREAD_H )
#ADD_DEFINITIONS(-O3 -Wall -msse -msse2 -DLUX_USE_SSE -DLUX_USE_OPENGL -DHAVE_PTHREAD_H)
#ADD_DEFINITIONS(-g -Wall -msse -DLUX_USE_OPENGL -DHAVE_PTHREAD_H)
#ICC definitions
#ADD_DEFINITIONS(-O3 -ipo -no-prec-div -static -fp-model fast=2 -rcd)

#############################################################################
#############################################################################
#########################      CUSTOM COMMAND     ###########################
#############################################################################
#############################################################################
# Create custom command for flex/lex (note the outputs)
ADD_CUSTOM_COMMAND(
SOURCE ${CMAKE_SOURCE_DIR}/core/luxlex.l
COMMAND ${FLEX_EXECUTABLE} 
ARGS -o${CMAKE_BINARY_DIR}/luxlex.cpp
      ${CMAKE_SOURCE_DIR}/core/luxlex.l
TARGET LuxLexer
#DEPENDS ${CMAKE_BINARY_DIR}/core/pbrtparse.cpp ${lux_BINARY_DIR}/core/pbrtparse.h
OUTPUTS ${CMAKE_BINARY_DIR}/luxlex.cpp)
SET_SOURCE_FILES_PROPERTIES(${CMAKE_BINARY_DIR}/luxlex.cpp GENERATED)

# Create custom command for bison/yacc (note the DEPENDS)
IF(APPLE AND !APPLE_64)
	ADD_CUSTOM_COMMAND(
	TARGET LuxParser
	#DEPENDS ${CMAKE_BINARY_DIR}/core/luxlex.cpp
	SOURCE ${CMAKE_SOURCE_DIR}/core/luxparse.y
	COMMAND ${BISON_EXECUTABLE} -y ${CMAKE_SOURCE_DIR}/core/luxparse.y -o ${CMAKE_BINARY_DIR}/luxparse.cpp -d
	COMMAND mv ${CMAKE_SOURCE_DIR}/luxparse.cpp.h ${CMAKE_BINARY_DIR}/luxparse.hpp
	OUTPUTS ${CMAKE_BINARY_DIR}/luxparse.cpp ${CMAKE_BINARY_DIR}/luxparse.hpp)
ELSE(APPLE AND !APPLE_64)
	ADD_CUSTOM_COMMAND(
	SOURCE ${CMAKE_SOURCE_DIR}/core/luxparse.y
	COMMAND ${BISON_EXECUTABLE} 
	ARGS -y ${CMAKE_SOURCE_DIR}/core/luxparse.y
	     -o ${CMAKE_BINARY_DIR}/luxparse.cpp
	     -d
	TARGET LuxParser
	#DEPENDS ${CMAKE_BINARY_DIR}/core/luxlex.cpp
	OUTPUTS ${CMAKE_BINARY_DIR}/luxparse.cpp ${CMAKE_BINARY_DIR}/luxparse.hpp)
ENDIF(APPLE AND !APPLE_64)
SET_SOURCE_FILES_PROPERTIES(${CMAKE_BINARY_DIR}/core/luxparse.cpp GENERATED)


#############################################################################
#############################################################################
#########################      APPLE CUSTOM PKG   ###########################
#############################################################################
#############################################################################
IF(APPLE)
	ADD_CUSTOM_TARGET(dist DEPENDS luxrender)
	ADD_CUSTOM_COMMAND(
	TARGET dist POST_BUILD
	COMMAND rm -rf LuxRender.app 
	COMMAND mkdir LuxRender.app 
	COMMAND mkdir LuxRender.app/Contents 
	COMMAND mkdir LuxRender.app/Contents/MacOS 
	COMMAND mkdir LuxRender.app/Contents/Resources
	COMMAND cp ../macos/Info.plist LuxRender.app/Contents
	COMMAND cp LuxRender.icns LuxRender.app/Contents/Resources
	COMMAND echo APPLnone > LuxRender.app/Contents/PkgInfo
	COMMAND cp luxrender LuxRender.app/Contents/MacOS/LuxRender)
ENDIF(APPLE)


#############################################################################
#############################################################################
#####################      SOURCE FILE FOR lux_lib     ######################
#############################################################################
#############################################################################
SET(lux_lib_src
		${CMAKE_BINARY_DIR}/luxparse.cpp ${CMAKE_BINARY_DIR}/luxlex.cpp
		core/api.cpp core/camera.cpp core/color.cpp core/spectrum.cpp core/context.cpp core/dynload.cpp core/exrio.cpp core/film.cpp
		core/pngio.cpp core/tgaio.cpp
		core/geometry.cpp core/igiio.cpp core/light.cpp core/material.cpp core/mc.cpp
        core/paramset.cpp core/parser.cpp core/primitive.cpp core/reflection.cpp core/renderfarm.cpp core/sampling.cpp
        core/scene.cpp core/shape.cpp core/texture.cpp core/timer.cpp core/photonmap.cpp
        core/transport.cpp core/util.cpp core/volume.cpp core/imagereader.cpp core/motionsystem.cpp
        core/geometry/transform.cpp core/geometry/matrix4x4.cpp core/geometry/quaternion.cpp core/geometry/raydifferential.cpp
        core/reflection/microfacetdistribution.cpp core/reflection/fresnel.cpp core/reflection/bxdf.cpp
        core/reflection/bxdf/lafortune.cpp core/reflection/bxdf/cooktorrance.cpp core/reflection/bxdf/specularreflection.cpp
        core/reflection/bxdf/fresnelblend.cpp core/reflection/bxdf/lambertian.cpp core/reflection/bxdf/microfacet.cpp
        core/reflection/bxdf/orennayar.cpp core/reflection/bxdf/speculartransmission.cpp core/reflection/bxdf/nulltransmission.cpp
        core/reflection/fresnel/fresnelnoop.cpp core/reflection/fresnel/fresneldielectric.cpp
        core/reflection/fresnel/fresnelconductor.cpp core/reflection/fresnel/fresnelslick.cpp 
	core/reflection/fresnel/fresnelgeneral.cpp 
        core/reflection/microfacetdistribution/beckmann.cpp core/reflection/microfacetdistribution/blinn.cpp
        core/reflection/microfacetdistribution/wardisotropic.cpp core/reflection/microfacetdistribution/anisotropic.cpp
		core/osfunc.cpp
        shapes/cone.cpp shapes/cylinder.cpp shapes/disk.cpp shapes/heightfield.cpp shapes/torus.cpp 
        shapes/hyperboloid.cpp shapes/loopsubdiv.cpp shapes/nurbs.cpp shapes/quad.cpp shapes/quadrilateral.cpp
        shapes/paraboloid.cpp shapes/plymesh.cpp shapes/sphere.cpp
        shapes/plymesh/rply.c shapes/lenscomponent.cpp shapes/mesh.cpp shapes/meshwaldtriangle.cpp shapes/meshbarytriangle.cpp
        shapes/meshquadrilateral.cpp samplers/lowdiscrepancy.cpp samplers/metrosampler.cpp samplers/erpt.cpp
        samplers/random.cpp
        accelerators/grid.cpp accelerators/unsafekdtree.cpp accelerators/tabreckdtree.cpp accelerators/bruteforce.cpp
        accelerators/bvhaccel.cpp accelerators/qbvhaccel.cpp
        cameras/environment.cpp cameras/orthographic.cpp cameras/perspective.cpp cameras/realistic.cpp film/fleximage.cpp
        filters/box.cpp filters/gaussian.cpp filters/mitchell.cpp filters/sinc.cpp filters/triangle.cpp
        integrators/emission.cpp
        integrators/path.cpp integrators/single.cpp
	integrators/distributedpath.cpp 
	integrators/bidirectional.cpp
	integrators/directlighting.cpp
	integrators/exphotonmap.cpp
        lights/area.cpp lights/distant.cpp lights/infinite.cpp
        lights/point.cpp lights/projection.cpp lights/spot.cpp lights/infinitesample.cpp
        lights/sun.cpp lights/sky.cpp 
		lights/sphericalfunction/sphericalfunction.cpp lights/sphericalfunction/sphericalfunction_ies.cpp
		lights/sphericalfunction/photometricdata_ies.cpp
        materials/glass.cpp materials/glossy.cpp materials/matte.cpp materials/mattetranslucent.cpp materials/mirror.cpp
        materials/shinymetal.cpp
        materials/carpaint.cpp materials/metal.cpp materials/roughglass.cpp
        materials/mixmaterial.cpp materials/null.cpp
        textures/bilerp.cpp textures/checkerboard.cpp textures/constant.cpp textures/dots.cpp
        textures/fbm.cpp textures/imagemap.cpp textures/marble.cpp textures/mix.cpp
        textures/scale.cpp textures/uv.cpp textures/windy.cpp textures/wrinkled.cpp textures/brick.cpp
        textures/blender_wood.cpp textures/blender_musgrave.cpp textures/blender_marble.cpp textures/blender_noiselib.cpp textures/blender_texlib.cpp
		textures/blender_clouds.cpp textures/blender_blend.cpp textures/blender_distortednoise.cpp textures/blender_magic.cpp textures/blender_noise.cpp
		textures/blender_stucci.cpp textures/harlequin.cpp textures/blender_voronoi.cpp
	textures/blackbody.cpp textures/equalenergy.cpp textures/frequencytexture.cpp textures/gaussiantexture.cpp textures/lampspectrum.cpp textures/irregulardata.cpp textures/regulardata.cpp
        tonemaps/contrast.cpp tonemaps/highcontrast.cpp tonemaps/maxwhite.cpp tonemaps/nonlinear.cpp tonemaps/reinhard.cpp tonemaps/lineartonemap.cpp 
        volumes/exponential.cpp volumes/homogeneous.cpp volumes/volumegrid.cpp volumes/cloud.cpp
        pixelsamplers/linear.cpp pixelsamplers/vegas.cpp pixelsamplers/lowdiscrepancypx.cpp
		pixelsamplers/tilepx.cpp pixelsamplers/hilbertpx.cpp
        spds/blackbodyspd.cpp spds/irregular.cpp spds/regular.cpp spds/rgbillum.cpp spds/rgbrefl.cpp spds/equalspd.cpp spds/frequencyspd.cpp spds/gaussianspd.cpp
        core/spectrumwavelengths.cpp core/spd.cpp
        renderer/renderserver.cpp
	)

INCLUDE_DIRECTORIES(${OPENGL_INCLUDE_DIR}
                    ${JPEG_INCLUDE_DIR}
					${TIFF_INCLUDE_DIR}
					${PNG_INCLUDE_DIR}
					${OPENEXR_INCLUDE_DIRS}
					${Boost_INCLUDE_DIRS}
					${CMAKE_SOURCE_DIR}/accelerators ${CMAKE_SOURCE_DIR}/cameras ${CMAKE_SOURCE_DIR}/core
					${CMAKE_SOURCE_DIR}/core/reflection ${CMAKE_SOURCE_DIR}/core/reflection/bxdf ${CMAKE_SOURCE_DIR}/spds
					${CMAKE_SOURCE_DIR}/core/reflection/fresnel ${CMAKE_SOURCE_DIR}/core/reflection/microfacetdistribution
					${CMAKE_SOURCE_DIR}/film ${CMAKE_SOURCE_DIR}/filters ${CMAKE_SOURCE_DIR}/integrators
					${CMAKE_SOURCE_DIR}/lights ${CMAKE_SOURCE_DIR}/lights/sphericalfunction
					${CMAKE_SOURCE_DIR}/materials ${CMAKE_SOURCE_DIR}/samplers
					${CMAKE_SOURCE_DIR}/shapes ${CMAKE_SOURCE_DIR}/textures ${CMAKE_SOURCE_DIR}/tonemaps
					${CMAKE_SOURCE_DIR}/volumes ${CMAKE_SOURCE_DIR}/integrationsamplers ${CMAKE_SOURCE_DIR}/pixelsamplers
					${CMAKE_SOURCE_DIR}/renderer/include
					${CMAKE_BINARY_DIR}
					)


#############################################################################
#############################################################################
#####################            LINKER INFO           ######################
#############################################################################
#############################################################################

LINK_DIRECTORIES(${LINK_DIRECTORIES} ${Boost_LIBRARY_DIRS} ${OPENEXR_LIB_DIRS} ${PNG_LIBRARY} ${TIFF_LIBRARY} ${JPEG_LIBRARY})

#Here we build the core library
ADD_LIBRARY(lux STATIC ${lux_lib_src} )
#TARGET_LINK_LIBRARIES(lux ${OPENEXR_LIBRARIES} ${Boost_LIBRARIES} ${PNG_LIBRARIES} ${TIFF_LIBRARIES} ${JPEG_LIBRARIES} )


#############################################################################
#Here we build the console executable
#############################################################################
ADD_EXECUTABLE(luxconsole renderer/luxconsole.cpp)
IF(APPLE)
	TARGET_LINK_LIBRARIES(luxconsole -Wl,-undefined -Wl,dynamic_lookup -all_load lux -noall_load ${CMAKE_THREAD_LIBS_INIT} ${OPENEXR_LIBRARIES} z ${Boost_LIBRARIES} ${PNG_LIBRARIES} ${TIFF_LIBRARIES} ${JPEG_LIBRARIES})
ELSE(APPLE)
	TARGET_LINK_LIBRARIES(luxconsole -Wl,--whole-archive lux -Wl,--no-whole-archive ${CMAKE_THREAD_LIBS_INIT} ${OPENEXR_LIBRARIES} ${Boost_LIBRARIES} ${PNG_LIBRARIES} ${TIFF_LIBRARIES} ${JPEG_LIBRARIES} ${SYS_LIBRARIES})
ENDIF(APPLE)

IF(APPLE)
	SET(GUI_TYPE MACOSX_BUNDLE)
	# SET(MACOSX_BUNDLE_LONG_VERSION_STRING "${OPENSCENEGRAPH_MAJOR_VERSION}.${OPENSCENEGRAPH_MINOR_VERSION}.${OPENSCENEGRAPH_PATCH_VERSION}")
	# Short Version is the "marketing version". It is the version
	# the user sees in an information panel.
	SET(MACOSX_BUNDLE_SHORT_VERSION_STRING "${VERSION}")
	# Bundle version is the version the OS looks at.
	SET(MACOSX_BUNDLE_BUNDLE_VERSION "${VERSION}")
	SET(MACOSX_BUNDLE_GUI_IDENTIFIER "org.luxrender.luxrender" )
	SET(MACOSX_BUNDLE_BUNDLE_NAME "luxrender" )
	SET(MACOSX_BUNDLE_ICON_FILE "LuxRender.icns")
	# SET(MACOSX_BUNDLE_COPYRIGHT "")
	# SET(MACOSX_BUNDLE_INFO_STRING "Info string, localized?")
ENDIF(APPLE)

#############################################################################
#Here we build the wxWidgets GUI executable
#############################################################################
IF(wxWidgets_FOUND)
	ADD_EXECUTABLE(luxrender ${GUI_TYPE} renderer/wxluxapp.cpp renderer/wxluxgui.cpp renderer/wxluxframe.cpp renderer/wxglviewer.cpp renderer/wxviewer.cpp)
	INCLUDE_DIRECTORIES( ${wxWidgets_INCLUDE_DIRS} )

	IF(APPLE)
		INCLUDE_DIRECTORIES ( /Developer/Headers/FlatCarbon /usr/local/include)
		FIND_LIBRARY(CARBON_LIBRARY Carbon)
		FIND_LIBRARY(QUICKTIME_LIBRARY QuickTime )
		FIND_LIBRARY(OPENGL_LIBRARY OpenGL )
		FIND_LIBRARY(AGL_LIBRARY AGL )
		FIND_LIBRARY(APP_SERVICES_LIBRARY ApplicationServices )

		MESSAGE(STATUS ${CARBON_LIBRARY})
		MARK_AS_ADVANCED (CARBON_LIBRARY)
		MARK_AS_ADVANCED (QUICKTIME_LIBRARY)
		MARK_AS_ADVANCED (OPENGL_LIBRARY)
		MARK_AS_ADVANCED (AGL_LIBRARY)
		MARK_AS_ADVANCED (APP_SERVICES_LIBRARY)
		SET(EXTRA_LIBS ${CARBON_LIBRARY} ${QUICKTIME_LIBRARY} ${OPENGL_LIBRARY} ${AGL_LIBRARY} ${APP_SERVICES_LIBRARY})
		TARGET_LINK_LIBRARIES(luxrender -Wl,-undefined -Wl,dynamic_lookup -all_load lux -noall_load ${wxWidgets_LIBRARIES} ${OPENEXR_LIBRARIES} ${Boost_LIBRARIES} ${EXTRA_LIBS} z ${PNG_LIBRARIES} ${TIFF_LIBRARIES} ${JPEG_LIBRARIES})
	ELSE(APPLE)
		TARGET_LINK_LIBRARIES(luxrender -Wl,--whole-archive lux -Wl,--no-whole-archive ${wxWidgets_LIBRARIES} ${OPENGL_LIBRARY} ${OPENEXR_LIBRARIES} ${Boost_LIBRARIES} ${PNG_LIBRARIES} ${TIFF_LIBRARIES} ${JPEG_LIBRARIES} ${SYS_LIBRARIES})
	ENDIF(APPLE)
ENDIF(wxWidgets_FOUND)



#############################################################################

#Install target
INSTALL(TARGETS luxrender luxconsole
        RUNTIME DESTINATION bin
)

#Install API/Library
INSTALL(FILES ${CMAKE_SOURCE_DIR}/core/api.h DESTINATION include/luxrender/)
INSTALL(TARGETS lux DESTINATION lib${LIB_SUFFIX})
#	ELSE(FIND_LIBRARY_USE_LIB64_PATHS) ARCHIVE DESTINATION lib
#	ENDIF(FIND_LIBRARY_USE_LIB64_PATHS)


#Install Desktop files
INSTALL(FILES ${CMAKE_SOURCE_DIR}/renderer/luxrender.svg DESTINATION share/pixmaps/)
INSTALL(FILES ${CMAKE_SOURCE_DIR}/renderer/luxrender.desktop DESTINATION share/applications/)

#Source package target
ADD_CUSTOM_TARGET(package mkdir lux-${VERSION}
	COMMAND cp -R ${CMAKE_SOURCE_DIR}/* lux-${VERSION}
	COMMAND tar -cf ${CMAKE_BINARY_DIR}/lux-${VERSION}.tar lux-${VERSION}
	COMMAND bzip2 --best ${CMAKE_BINARY_DIR}/lux-${VERSION}.tar
	WORKING_DIRECTORY ${CMAKE_BINARY_DIR}
	DEPENDS ${lux_lib_src}
	COMMENT Building source package
)

# Dade - GCC Profiling (remember to uncomment the line in the middle of file too)
#SET_TARGET_PROPERTIES(luxconsole luxrender PROPERTIES LINK_FLAGS "-pg")

# Dade - GCC 2 pass optimization (remember to uncomment the line in the middle of file too)
#SET_TARGET_PROPERTIES(luxconsole luxrender PROPERTIES LINK_FLAGS "--coverage")

# Dade - Intel Compiler optimization
#REMOVE_DEFINITIONS(-ipo)
