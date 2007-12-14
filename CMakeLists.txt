PROJECT(lux)

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

#OPTION(WITH_GUI "Enable the GUI." OFF)
#OPTION(WITH_SSE "Enable SSE optimisations" ON)


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
########################### FLTK  LIBRARIES SETUP ###########################
#############################################################################
#############################################################################
#This FIND_PATH is a hack, FIND_PACKAGE doesn't find fltk otherwise
IF(APPLE)
	SET(FLTK_INCLUDE_DIR ${CMAKE_BINARY_DIR}/../macos/fltk-1.1/include)
	IF(APPLE_64)
		SET(FLTK_LIBRARIES_DIR ${CMAKE_BINARY_DIR}/../macos/fltk-1.1/lib/64bit)
	ELSE(APPLE_64)
		SET(FLTK_LIBRARIES_DIR ${CMAKE_BINARY_DIR}/../macos/fltk-1.1/lib)
	ENDIF(APPLE_64)
ELSE(APPLE)
	FIND_PATH(FLTK_INCLUDE_DIR
		Fl/Fl.H
		PATHS
		/usr/local/include/
		/usr/local/include/fltk-1.1
		/usr/include/
		/usr/include/fltk-1.1
		/opt/local/include/
		/opt/include/
	)
ENDIF(APPLE)

IF( FLTK_INCLUDE_DIR )
	SET(FLTK_LIBRARIES fltk fltk_gl GL)
ELSE( FLTK_INCLUDE_DIR )
        MESSAGE(FATAL_ERROR "Could not find FLTK")
ENDIF( FLTK_INCLUDE_DIR )


#INCLUDE(FindFLTK)
#FIND_PACKAGE( FLTK REQUIRED )
#IF( FLTK_FOUND )
#        MESSAGE( STATUS "Found FLTK" )
#ELSE( FLTK_FOUND )
#        MESSAGE(FATAL_ERROR "Could not find FLTK")
#ENDIF( FLTK_FOUND )



#############################################################################
#############################################################################
########################### BOOST LIBRARIES SETUP ###########################
#############################################################################
#############################################################################
IF(APPLE)
	SET(Boost_INCLUDE_DIRS ${CMAKE_BINARY_DIR}/../macos/boost/include)
	IF(APPLE_64)
		SET(Boost_LIBRARY_DIRS ${CMAKE_BINARY_DIR}/../macos/boost/lib/x64)
	ELSE(APPLE_64)
		SET(Boost_LIBRARY_DIRS ${CMAKE_BINARY_DIR}/../macos/boost/lib/x86)
	ENDIF(APPLE_64)
	SET(Boost_LIBRARIES boost_thread-mt-1_34_1 boost_program_options-mt-1_34_1 boost_filesystem-mt-1_34_1)
ELSE(APPLE)
	FIND_PACKAGE(Boost REQUIRED)
	IF(Boost_FOUND)
		MESSAGE(STATUS ${Boost_LIBRARY_DIRS})
		MESSAGE(STATUS ${Boost_INCLUDE_DIRS})
		SET(Boost_LIBRARIES boost_thread-mt boost_program_options-mt boost_filesystem-mt boost_serialization-mt)
	ELSE(Boost_FOUND)
		MESSAGE(FATAL_ERROR "Could not find Boost")
	ENDIF(Boost_FOUND)
ENDIF(APPLE)


#############################################################################
#############################################################################
######################### OPENEXR LIBRARIES SETUP ###########################
#############################################################################
#############################################################################
IF(APPLE)
	IF(APPLE_64)
		SET(OPENEXR_LIB_DIRS ${CMAKE_BINARY_DIR}/../macos/openexr-1.4.0/lib/64bit)
	ELSE(APPLE_64)
		SET(OPENEXR_LIB_DIRS ${CMAKE_BINARY_DIR}/../macos/openexr-1.4.0/lib)
	ENDIF(APPLE_64)
	SET(OPENEXR_INCLUDE_DIRS ${CMAKE_BINARY_DIR}/../macos/openexr-1.4.0/include)
	SET(OPENEXR_LIBRARIES Half IlmImf Iex Imath IlmThread)
ELSE(APPLE)
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
IF(APPLE)
	SET(PNG_INCLUDE_DIR ${CMAKE_BINARY_DIR}/../macos/png/include)
	SET(PNG_LIBRARY ${CMAKE_BINARY_DIR}/../macos/png/lib)
	SET(PNG_LIBRARIES png png12)
ELSE(APPLE)
INCLUDE(FindPNG)
IF(PNG_FOUND)
ELSE(PNG_FOUND)
	MESSAGE( STATUS "Warning : could not find PNG - building without png support")
ENDIF(PNG_FOUND)
ENDIF(APPLE)
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
IF(APPLE)
	SET(TIFF_INCLUDE_DIR ${CMAKE_BINARY_DIR}/../macos/tiff/include)
	SET(TIFF_LIBRARY ${CMAKE_BINARY_DIR}/../macos/tiff/lib)
	SET(TIFF_LIBRARIES tiff tiffxx)
ELSE(APPLE)
	INCLUDE(FindTIFF)
	#FIND_PACKAGE(TIFF REQUIRED)
	IF(TIFF_FOUND)
		MESSAGE( STATUS "Found TIFF" )
	ELSE(TIFF_FOUND)
		MESSAGE( STATUS "Warning : could not find TIFF - building without tiff support")
	ENDIF(TIFF_FOUND)
ENDIF(APPLE)


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
IF(APPLE)
	SET(JPEG_INCLUDE_DIR ${CMAKE_BINARY_DIR}/../macos/jpeg/include)
	SET(JPEG_LIBRARY ${CMAKE_BINARY_DIR}/../macos/jpeg/lib)
	SET(JPEG_LIBRARIES libjpeg.a)
ELSE(APPLE)
	INCLUDE(FindJPEG)
	#FIND_PACKAGE(JPEG REQUIRED)
	IF(JPEG_FOUND)
		MESSAGE( STATUS ${JPEG_INCLUDE_DIR} )
	ELSE(JPEG_FOUND)
		MESSAGE( STATUS "Warning : could not find JPEG - building without jpeg support")
	ENDIF(JPEG_FOUND)
ENDIF(APPLE)


#All dependencies OK !

#Generate the config.h file
CONFIGURE_FILE(${CMAKE_SOURCE_DIR}/config.h.cmake ${CMAKE_BINARY_DIR}/config.h)
ADD_DEFINITIONS(-DLUX_USE_CONFIG_H)

#############################################################################
#############################################################################
#########################      COMPILER FLAGS     ###########################
#############################################################################
#############################################################################
ADD_DEFINITIONS( -O3 -Wall -DLUX_USE_OPENGL -DHAVE_PTHREAD_H )
#ADD_DEFINITIONS(-O3 -Wall -msse -DLUX_USE_SSE -DLUX_USE_OPENGL -DHAVE_PTHREAD_H)
#ADD_DEFINITIONS(-g -Wall -msse -DHAVE_PTHREAD_H)
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
	COMMAND mkdir LuxRender.app/MacOS 
	COMMAND mkdir LuxRender.app/Resource 
	COMMAND cp luxrender LuxRender.app/MacOS)
ENDIF(APPLE)


#############################################################################
#############################################################################
#####################      SOURCE FILE FOR lux_lib     ######################
#############################################################################
#############################################################################
SET(lux_lib_src
		${CMAKE_BINARY_DIR}/luxparse.cpp ${CMAKE_BINARY_DIR}/luxlex.cpp
		core/api.cpp core/camera.cpp core/color.cpp core/dynload.cpp core/exrio.cpp core/film.cpp
		core/geometry.cpp core/igiio.cpp core/light.cpp core/material.cpp core/mc.cpp
        core/paramset.cpp core/parser.cpp core/primitive.cpp core/reflection.cpp core/renderfarm.cpp core/sampling.cpp
        core/scene.cpp core/shape.cpp core/texture.cpp core/timer.cpp
        core/transport.cpp core/util.cpp core/volume.cpp
        core/SFMT/SFMT.cpp
        core/geometry/transform.cpp
        shapes/cone.cpp shapes/cylinder.cpp shapes/disk.cpp shapes/heightfield.cpp
        shapes/hyperboloid.cpp shapes/loopsubdiv.cpp shapes/nurbs.cpp
        shapes/paraboloid.cpp shapes/plymesh.cpp shapes/sphere.cpp shapes/trianglemesh.cpp
        shapes/plymesh/rply.c
        samplers/bestcandidate.cpp samplers/lowdiscrepancy.cpp
        samplers/random.cpp samplers/stratified.cpp samplers/halton.cpp
        accelerators/grid.cpp accelerators/kdtree.cpp accelerators/bruteforce.cpp
        cameras/environment.cpp cameras/orthographic.cpp cameras/perspective.cpp
        film/image.cpp film/multiimage.cpp
        filters/box.cpp filters/gaussian.cpp filters/mitchell.cpp filters/sinc.cpp filters/triangle.cpp
        integrators/directlighting.cpp integrators/emission.cpp integrators/irradiancecache.cpp
        integrators/path.cpp integrators/photonmap.cpp integrators/single.cpp
        integrators/whitted.cpp integrators/igi.cpp
        integrators/debug.cpp integrators/exphotonmap.cpp integrators/bidirectional.cpp integrationsamplers/metropolis.cpp
        lights/area.cpp lights/distant.cpp lights/goniometric.cpp lights/infinite.cpp
        lights/point.cpp lights/projection.cpp lights/spot.cpp lights/infinitesample.cpp
        lights/sun.cpp lights/sky.cpp lights/sun2.cpp lights/sun3.cpp
        materials/bluepaint.cpp materials/brushedmetal.cpp materials/clay.cpp materials/felt.cpp
        materials/glass.cpp materials/matte.cpp materials/mirror.cpp materials/plastic.cpp materials/primer.cpp
        materials/shinymetal.cpp materials/skin.cpp materials/substrate.cpp
        materials/translucent.cpp materials/uber.cpp materials/carpaint.cpp materials/metal.cpp materials/roughglass.cpp
        textures/bilerp.cpp textures/checkerboard.cpp textures/constant.cpp textures/dots.cpp
        textures/fbm.cpp textures/imagemap.cpp textures/marble.cpp textures/mix.cpp
        textures/scale.cpp textures/uv.cpp textures/windy.cpp textures/wrinkled.cpp
        tonemaps/contrast.cpp tonemaps/highcontrast.cpp tonemaps/maxwhite.cpp tonemaps/nonlinear.cpp tonemaps/reinhard.cpp
        volumes/exponential.cpp volumes/homogeneous.cpp volumes/volumegrid.cpp
        pixelsamplers/linear.cpp pixelsamplers/randompx.cpp pixelsamplers/vegas.cpp
	)

INCLUDE_DIRECTORIES(${JPEG_INCLUDE_DIR}
					${TIFF_INCLUDE_DIR}
					${PNG_INCLUDE_DIR}
					${OPENEXR_INCLUDE_DIRS}
					${Boost_INCLUDE_DIRS} ${FLTK_INCLUDE_DIR}
					${CMAKE_SOURCE_DIR}/accelerators ${CMAKE_SOURCE_DIR}/cameras ${CMAKE_SOURCE_DIR}/core
					${CMAKE_SOURCE_DIR}/film ${CMAKE_SOURCE_DIR}/filters ${CMAKE_SOURCE_DIR}/integrators
					${CMAKE_SOURCE_DIR}/lights ${CMAKE_SOURCE_DIR}/materials ${CMAKE_SOURCE_DIR}/samplers
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

LINK_DIRECTORIES(${LINK_DIRECTORIES} ${Boost_LIBRARY_DIRS} ${OPENEXR_LIB_DIRS} ${FLTK_LIBRARIES_DIR} /usr/lib/fltk-1.1 /usr/lib64/fltk-1.1 ${PNG_LIBRARY} ${TIFF_LIBRARY} ${JPEG_LIBRARY})
					
#Here we build the core library
ADD_LIBRARY(lux STATIC ${lux_lib_src} )
TARGET_LINK_LIBRARIES(lux ${OPENEXR_LIBRARIES} ${Boost_LIBRARIES} ${PNG_LIBRARIES} ${TIFF_LIBRARIES} ${JPEG_LIBRARIES})

#Needed for ICC
#REMOVE_DEFINITIONS(-ipo)

#Here we build the console executable
ADD_EXECUTABLE(luxconsole renderer/luxconsole.cpp)
IF(APPLE)
TARGET_LINK_LIBRARIES(luxconsole lux ${OPENEXR_LIBRARIES} z ${Boost_LIBRARIES} ${PNG_LIBRARIES} ${TIFF_LIBRARIES} ${JPEG_LIBRARIES})
ELSE(APPLE)
TARGET_LINK_LIBRARIES(luxconsole lux ${OPENEXR_LIBRARIES} ${Boost_LIBRARIES} ${PNG_LIBRARIES} ${TIFF_LIBRARIES} ${JPEG_LIBRARIES})
ENDIF(APPLE)

#Here we build the GUI executable
ADD_EXECUTABLE(luxrender renderer/luxgui.cpp renderer/renderwindow.cpp)
INCLUDE_DIRECTORIES(${FLTK_INCLUDE_DIR} /usr/local/include)

IF(APPLE)
SET(GUI_TYPE MACOSX_BUNDLE)
INCLUDE_DIRECTORIES ( /Developer/Headers/FlatCarbon /usr/local/include)
FIND_LIBRARY(CARBON_LIBRARY Carbon)
FIND_LIBRARY(QUICKTIME_LIBRARY QuickTime )
FIND_LIBRARY(APP_SERVICES_LIBRARY ApplicationServices )
MESSAGE(STATUS ${CARBON_LIBRARY})
MARK_AS_ADVANCED (CARBON_LIBRARY)
MARK_AS_ADVANCED (QUICKTIME_LIBRARY)
MARK_AS_ADVANCED (APP_SERVICES_LIBRARY)
SET(EXTRA_LIBS ${CARBON_LIBRARY} ${QUICKTIME_LIBRARY} ${APP_SERVICES_LIBRARY})
TARGET_LINK_LIBRARIES(luxrender lux ${FLTK_LIBRARIES} ${Boost_LIBRARIES} ${EXTRA_LIBS} z ${Boost_LIBRARIES} ${PNG_LIBRARIES} ${TIFF_LIBRARIES} ${JPEG_LIBRARIES})
ELSE(APPLE)
TARGET_LINK_LIBRARIES(luxrender lux ${OPENEXR_LIBRARIES} ${FLTK_LIBRARIES} ${Boost_LIBRARIES} ${PNG_LIBRARIES} ${TIFF_LIBRARIES} ${JPEG_LIBRARIES})
ENDIF(APPLE)

#Install target
INSTALL(TARGETS luxrender luxconsole
        RUNTIME DESTINATION bin
)

#Source package target
SET(VERSION 0.1)
ADD_CUSTOM_TARGET(package mkdir lux-${VERSION}
	COMMAND cp -R ${CMAKE_SOURCE_DIR}/* lux-${VERSION}
	COMMAND tar -cf ${CMAKE_BINARY_DIR}/lux-${VERSION}.tar lux-${VERSION}
	COMMAND bzip2 --best ${CMAKE_BINARY_DIR}/lux-${VERSION}.tar
	WORKING_DIRECTORY ${CMAKE_BINARY_DIR}
	DEPENDS ${lux_lib_src}
	COMMENT Building source package
)

