#INCLUDE(FindFLTK)

PROJECT(lux)
OPTION(WITH_GUI "Enable the GUI." OFF)
OPTION(WITH_SSE "Enable SSE optimisations" ON)

# Find and setup OpenEXR
#IF(WIN32)
#SET(OPENEXR_INC ${CMAKE_SOURCE_DIR}/win32/openexr-1.4.0/include/OpenEXR)
#SET(LINK_DIRECTORIES ${CMAKE_SOURCE_DIR}/win32/openexr-1.4.0/lib)
#ELSE(WIN32)
  FIND_PATH(OPENEXR_INC
    ImfXdr.h
    PATHS
    /usr/local/include/OpenEXR
    /usr/include/OpenEXR
    /sw/include/OpenEXR
    /opt/local/include/OpenEXR
    /opt/csw/include/OpenEXR
    /opt/include/OpenEXR
  ) 

#ENDIF(WIN32)

SET(OPENEXR_LIB Half IlmImf Iex Imath)

#MESSAGE(${OPENEXR_INC})
#MESSAGE(${OPENEXR_LIB})
#MESSAGE(${LINK_DIRECTORIES})

FIND_PROGRAM(BISON_EXECUTABLE bison)
IF (NOT BISON_EXECUTABLE)
MESSAGE(FATAL_ERROR "bison not found - aborting")
ENDIF (NOT BISON_EXECUTABLE)

FIND_PROGRAM(FLEX_EXECUTABLE flex)
IF (NOT FLEX_EXECUTABLE)
MESSAGE(FATAL_ERROR "flex not found - aborting")
ENDIF (NOT FLEX_EXECUTABLE)

#FIND_PACKAGE(FLTK REQUIRED)
FIND_PATH(FLTK_INCLUDE_DIR
 	          NAMES FL/Fl.H
	          PATHS
	            /usr
 	            /usr/local
 	)
 	
 	IF (FLTK_INCLUDE_DIR)
 	  SET(FLTK_FOUND 1)
 	  SET(FLTK_LIB fltk)
 	  MESSAGE(STATUS "FLTK found")
 	  MESSAGE(${FLTK_INCLUDE_DIR})
 	ELSE (FLTK_INCLUDE_DIR)
 		MESSAGE(STATUS "FLTK *NOT* found")
 	ENDIF (FLTK_INCLUDE_DIR)


# Find and setup Boost library for this project
FIND_PACKAGE(Boost REQUIRED)

# Now some variables are defined in order to use Boost library:
# BOOST_FOUND - System has Boost
# BOOST_INCLUDE_DIRS - Boost include directory
# BOOST_LIBRARIES - Link these to use Boost
# BOOST_LIBRARY_DIRS - The path to where the Boost library files are.
# BOOST_DEFINITIONS - Compiler switches required for using Boost

IF(Boost_FOUND)
	MESSAGE(STATUS ${BOOST_LIBRARIES})
ELSE(Boost_FOUND)
	MESSAGE(STATUS "Boost NOT found")
ENDIF(Boost_FOUND)

IF(APPLE)
	SET(BOOST_LIB boost_thread-mt boost_program_options)
ELSE(APPLE)
	SET(BOOST_LIB boost_thread boost_program_options)
ENDIF(APPLE)


#All dependencies OK !

#Compiler flags
ADD_DEFINITIONS(-O3 -Wall -DHAVE_PTHREAD_H)
#ADD_DEFINITIONS(-O3 -Wall -msse -DLUX_USE_SSE -DHAVE_PTHREAD_H)
#ADD_DEFINITIONS(-g -Wall -msse -DHAVE_PTHREAD_H)

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
IF(APPLE)
ADD_CUSTOM_COMMAND(
TARGET LuxParser
#DEPENDS ${CMAKE_BINARY_DIR}/core/luxlex.cpp
SOURCE ${CMAKE_SOURCE_DIR}/core/luxparse.y
COMMAND ${BISON_EXECUTABLE} -y ${CMAKE_SOURCE_DIR}/core/luxparse.y -o ${CMAKE_BINARY_DIR}/luxparse.cpp -d
COMMAND mv ${CMAKE_SOURCE_DIR}/luxparse.cpp.h ${CMAKE_BINARY_DIR}/luxparse.hpp
OUTPUTS ${CMAKE_BINARY_DIR}/luxparse.cpp ${CMAKE_BINARY_DIR}/luxparse.hpp)
ELSE(APPLE)
ADD_CUSTOM_COMMAND(
SOURCE ${CMAKE_SOURCE_DIR}/core/luxparse.y
COMMAND ${BISON_EXECUTABLE} 
ARGS -y ${CMAKE_SOURCE_DIR}/core/luxparse.y
     -o ${CMAKE_BINARY_DIR}/luxparse.cpp
     -d
TARGET LuxParser
#DEPENDS ${CMAKE_BINARY_DIR}/core/luxlex.cpp
OUTPUTS ${CMAKE_BINARY_DIR}/luxparse.cpp ${CMAKE_BINARY_DIR}/luxparse.hpp)
ENDIF(APPLE)
SET_SOURCE_FILES_PROPERTIES(${CMAKE_BINARY_DIR}/core/luxparse.cpp GENERATED)

SET(lux_lib_src
		${CMAKE_BINARY_DIR}/luxparse.cpp ${CMAKE_BINARY_DIR}/luxlex.cpp
		core/api.cpp core/camera.cpp core/color.cpp core/dynload.cpp core/exrio.cpp core/film.cpp
		core/geometry.cpp core/light.cpp core/material.cpp core/mc.cpp
        core/paramset.cpp core/parser.cpp core/primitive.cpp core/reflection.cpp core/sampling.cpp
        core/scene.cpp core/shape.cpp core/texture.cpp core/timer.cpp core/transform.cpp
        core/transport.cpp core/util.cpp core/volume.cpp
        shapes/cone.cpp shapes/cylinder.cpp shapes/disk.cpp shapes/heightfield.cpp
        shapes/hyperboloid.cpp shapes/loopsubdiv.cpp shapes/nurbs.cpp
        shapes/paraboloid.cpp shapes/sphere.cpp shapes/trianglemesh.cpp
        samplers/bestcandidate.cpp samplers/lowdiscrepancy.cpp
        samplers/random.cpp samplers/stratified.cpp
        accelerators/grid.cpp accelerators/kdtree.cpp
        cameras/environment.cpp cameras/orthographic.cpp cameras/perspective.cpp
        film/image.cpp film/multiimage.cpp
        filters/box.cpp filters/gaussian.cpp filters/mitchell.cpp filters/sinc.cpp filters/triangle.cpp
        integrators/directlighting.cpp integrators/emission.cpp integrators/irradiancecache.cpp
        integrators/path.cpp integrators/photonmap.cpp integrators/single.cpp
        integrators/whitted.cpp integrators/igi.cpp
        integrators/debug.cpp integrators/exphotonmap.cpp integrators/bidirectional.cpp
        lights/area.cpp lights/distant.cpp lights/goniometric.cpp lights/infinite.cpp
        lights/point.cpp lights/projection.cpp lights/spot.cpp lights/infinitesample.cpp
        lights/sun.cpp lights/sky.cpp
        materials/bluepaint.cpp materials/brushedmetal.cpp materials/clay.cpp materials/felt.cpp
        materials/glass.cpp materials/matte.cpp materials/mirror.cpp materials/plastic.cpp materials/primer.cpp
        materials/shinymetal.cpp materials/skin.cpp materials/substrate.cpp
        materials/translucent.cpp materials/uber.cpp
        textures/bilerp.cpp textures/checkerboard.cpp textures/constant.cpp textures/dots.cpp
        textures/fbm.cpp textures/imagemap.cpp textures/marble.cpp textures/mix.cpp
        textures/scale.cpp textures/uv.cpp textures/windy.cpp textures/wrinkled.cpp
        tonemaps/contrast.cpp tonemaps/highcontrast.cpp tonemaps/maxwhite.cpp tonemaps/nonlinear.cpp tonemaps/reinhard.cpp
        volumes/exponential.cpp volumes/homogeneous.cpp volumes/volumegrid.cpp
	)

INCLUDE_DIRECTORIES(${OPENEXR_INC}
					${BOOST_INCLUDE_DIRS}
					${CMAKE_SOURCE_DIR}/accelerators ${CMAKE_SOURCE_DIR}/cameras ${CMAKE_SOURCE_DIR}/core
					${CMAKE_SOURCE_DIR}/film ${CMAKE_SOURCE_DIR}/filters ${CMAKE_SOURCE_DIR}/integrators
					${CMAKE_SOURCE_DIR}/lights ${CMAKE_SOURCE_DIR}/materials ${CMAKE_SOURCE_DIR}/samplers
					${CMAKE_SOURCE_DIR}/shapes ${CMAKE_SOURCE_DIR}/textures ${CMAKE_SOURCE_DIR}/tonemaps
					${CMAKE_SOURCE_DIR}/volumes
					${CMAKE_BINARY_DIR}
					)
ADD_LIBRARY(lux STATIC ${lux_lib_src} )
TARGET_LINK_LIBRARIES(lux ${OPENEXR_LIB} ${BOOST_LIB})

INCLUDE_DIRECTORIES(${FLTK_INCLUDE_DIR})

ADD_EXECUTABLE(luxrender renderer/luxgui.cpp)
IF(APPLE)
SET(GUI_TYPE MACOSX_BUNDLE)
INCLUDE_DIRECTORIES ( /Developer/Headers/FlatCarbon )
FIND_LIBRARY(CARBON_LIBRARY Carbon)
FIND_LIBRARY(QUICKTIME_LIBRARY QuickTime )
FIND_LIBRARY(APP_SERVICES_LIBRARY ApplicationServices )
MESSAGE(STATUS ${CARBON_LIBRARY})
MARK_AS_ADVANCED (CARBON_LIBRARY)
MARK_AS_ADVANCED (QUICKTIME_LIBRARY)
MARK_AS_ADVANCED (APP_SERVICES_LIBRARY)
SET(EXTRA_LIBS ${CARBON_LIBRARY} ${QUICKTIME_LIBRARY} ${APP_SERVICES_LIBRARY})
TARGET_LINK_LIBRARIES(luxrender lux ${OPENEXR_LIB} ${FLTK_LIB} ${BOOST_LIB} ${EXTRA_LIBS})
ELSE(APPLE)
TARGET_LINK_LIBRARIES(luxrender lux ${OPENEXR_LIB} ${FLTK_LIB} ${BOOST_LIB})
ENDIF(APPLE)

ADD_EXECUTABLE(luxconsole renderer/luxconsole.cpp)
TARGET_LINK_LIBRARIES(luxconsole lux ${OPENEXR_LIB} ${BOOST_LIB})
